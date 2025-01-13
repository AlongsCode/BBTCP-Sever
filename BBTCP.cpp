#ifndef WINVER
#define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410
#endif
#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <mswsock.h>
#include <ws2tcpip.h>
typedef void(__stdcall*ser_fun)(HANDLE n_cli, SOCKET * n_ser, int n_type, char* n_data, ULONG n_size);

extern HANDLE g_iocp;
extern bool g_open;
extern DWORD g_ser_thread;

#define GlobalFrees(x){if(x !=NULL){GlobalFree(x);x = NULL;}}
#define CloseHandles(x){if(x !=NULL){CloseHandle(x);x = NULL;}}

void int2char(char* data, int val);
void closesockets(SOCKET x);
class client;

typedef struct tcpop
{
	OVERLAPPED	op;
	client		*so;
	int			state;
	char		*buf;
	DWORD		slen;
	DWORD		cb;
	DWORD		bufSize;
	DWORD		bufOffset;
	ULONG		m_so;
}S_tcpop, *P_tcpop;

class client {
public:
	bool cli_connect(ser_fun n_fun, char* n_ip, USHORT n_port, int n_modle, int n_time, int n_blen, DWORD n_buflen,
		int n_proxy, char* n_proxyip, USHORT n_proxyport, char* n_pacc, char* n_pass, bool ipv6);
	bool cli_connect_fun(P_tcpop op);
	bool cli_SoRecv(P_tcpop op);
	bool cli_recv(P_tcpop op);
	bool cli_send(char* n_buf, DWORD n_len);
	bool cli_close();
	bool cli_socks4(PCHAR host, WORD port, PCHAR username, PCHAR userpass);
	bool cli_socks5(PCHAR host, WORD port, PCHAR username, PCHAR userpass);
	bool cli_http(PCHAR host, WORD port, PCHAR username, PCHAR userpass);
	char cli_b2c(BYTE b);
	int cli_base64(LPBYTE srcbuf, int cbsrc, LPBYTE dstbuf, int cbdst);
public:
	SOCKET m_so;
	int m_modle;
	int m_blen;
	DWORD m_buflen;
	ser_fun m_fun;
	bool m_close;
	int m_record_int;
	char* m_record_char;
};
bool client::cli_connect(ser_fun n_fun, char* n_ip, USHORT n_port, int n_modle, int n_time, int n_blen, DWORD n_buflen,
	int n_proxy, char* n_proxyip, USHORT n_proxyport, char* n_pacc, char* n_pass, bool ipv6) {
	m_fun = n_fun;
	m_modle = n_modle;
	m_blen = n_blen;
	m_buflen = n_buflen;
	m_close = false;
	m_record_int = 0;
	m_record_char = NULL;
	m_so = WSASocket(ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == m_so) {
		return false;
	}
	
	HANDLE n_iocp = CreateIoCompletionPort((HANDLE)m_so, g_iocp, NULL, 0);
	if (n_iocp != g_iocp) {
		closesockets(m_so);
		return false;
	}
	
	
	DWORD to = 40960;
	setsockopt(m_so, SOL_SOCKET, SO_RCVTIMEO, (char*)&to, sizeof(to));
	setsockopt(m_so, SOL_SOCKET, SO_SNDTIMEO, (char*)&to, sizeof(to));

	unsigned long ul = 1;
	ioctlsocket(m_so, FIONBIO, (unsigned long *)&ul);

	if (ipv6)
	{
		struct addrinfo hints, *AddrInfo = NULL;
		memset(&hints, 0, sizeof(hints));

		hints.ai_family = AF_INET6;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;
		char nport[4];
		int2char(nport, n_port);
		if (getaddrinfo(n_ip, nport, &hints, &AddrInfo) != 0) {
			closesockets(m_so);
			return NULL;
		}
		connect(m_so, AddrInfo->ai_addr, AddrInfo->ai_addrlen);
	} 
	else
	{
		sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_addr.S_un.S_addr = 0;
		sin.sin_port = 0;
		if (SOCKET_ERROR == bind(m_so, (SOCKADDR *)&sin, 16)) {
			closesockets(m_so);
			return false;
		}
		hostent *pHost = gethostbyname(0!=n_proxy?n_proxyip:n_ip);
		if (NULL != pHost) {
			WriteProcessMemory(INVALID_HANDLE_VALUE, &sin.sin_addr.S_un.S_addr, pHost->h_addr_list[0], pHost->h_length, NULL);
			sin.sin_addr.S_un.S_addr = inet_addr(inet_ntoa(sin.sin_addr));
		}
		else {
			sin.sin_addr.S_un.S_addr = inet_addr(0!=n_proxy?n_proxyip:n_ip);
		}
		sin.sin_port = htons(0!=n_proxy?n_proxyport:n_port);

		connect(m_so, (sockaddr*)&sin, sizeof(sockaddr));
	}
	
	struct timeval timeout;
	fd_set r;
	FD_ZERO(&r);
	FD_SET(m_so, &r);
	timeout.tv_sec = n_time;
	timeout.tv_usec = 0;
	int ret = select((int)m_so, 0, &r, 0, &timeout);
	if (ret <= 0) {
		closesockets(m_so);
		return false;
	}

	if (0 != n_proxy) {
		bool ret = false;
		switch (n_proxy) {
		case 1:	ret = cli_socks4(n_ip, n_port, n_pacc, n_pass); break;
		case 2:	ret = cli_socks5(n_ip, n_port, n_pacc, n_pass); break;
		case 3:	ret = cli_http(n_ip, n_port, n_pacc, n_pass); break;
		default:
			closesockets(m_so);
			return false;
		}

		if (!ret) {
			closesockets(m_so);
			return false;
		}
	}
	tcpop *op = (tcpop*)GlobalAlloc(GMEM_ZEROINIT, sizeof(tcpop));
	if (NULL == op) {
		closesockets(m_so);
		return false;
	}
	op->so = this;
	op->state = 1;
	op->buf = NULL;
	op->bufSize = 0;
	op->bufOffset = 0;
	op->m_so = m_so;
	
	
	PostQueuedCompletionStatus(g_iocp, 0, NULL, (LPOVERLAPPED)op);

	return true;
}

bool client::cli_connect_fun(P_tcpop op)
{
	setsockopt(m_so, SOL_SOCKET, 0x7010, NULL, 0);
	GlobalFrees(op->buf);
	if (m_modle)
	{
		op->buf = (char*)GlobalAlloc(GMEM_ZEROINIT, 4);
		if (NULL == op->buf) {
			return false;
		}
		op->bufSize = sizeof(DWORD);
	}
	else
	{
		op->buf = (char*)GlobalAlloc(GMEM_ZEROINIT, m_blen);
		if (NULL == op->buf) {
			return false;
		}
		op->bufSize = m_blen;
	}
	op->state = 2;
	op->bufOffset = 0;
	op->so = this;
	WSABUF wsabuf;
	wsabuf.buf = op->buf + op->bufOffset;
	wsabuf.len = op->bufSize - op->bufOffset;

	DWORD Flg = 0;
	if (WSARecv(m_so, &wsabuf, 1, &op->cb, &Flg, (LPWSAOVERLAPPED)op, NULL))
	{
		int ercode = WSAGetLastError();
		if (ercode != WSA_IO_PENDING) {
			return false;
		}
	}
	return true;
}
bool client::cli_SoRecv(P_tcpop op) {
	op->so = this;
	op->state = 2;
	WSABUF wsabuf;
	wsabuf.buf = op->buf + op->bufOffset;
	wsabuf.len = op->bufSize - op->bufOffset;

	DWORD Flg = 0;
	if (WSARecv(m_so, &wsabuf, 1, &op->cb, &Flg, (LPWSAOVERLAPPED)op, NULL)) {
		int ercode = WSAGetLastError();
		if (ercode != WSA_IO_PENDING && ercode != WSAEFAULT) {
			return false;
		}
	}
	return true;
}
bool client::cli_recv(P_tcpop op) {
	if (1 == m_modle) {
		op->bufOffset += op->cb; //更新大小

		if (op->bufOffset < op->bufSize) {
			return cli_SoRecv(op);
		}

		DWORD size = *((DWORD*)(op->buf));

		if (size > m_buflen || size <= 0) {
			return false;
		}

		if (op->bufOffset < 4 + size) {
			GlobalFrees(op->buf);
			op->buf = (char*)GlobalAlloc(GMEM_ZEROINIT, 4 + size);
			if (NULL == op->buf) {
				return false;
			}
			op->bufSize = 4+size;
			op->bufOffset = 4;
			WriteProcessMemory(INVALID_HANDLE_VALUE, op->buf, &size, 4, NULL);
			return cli_SoRecv(op);
		}

		if (op->bufSize - 4 > 0) {
			if (NULL != m_fun) {
				m_fun(this, &m_so, 2, op->buf + 4, op->bufSize - 4);
			}
			
		}

		GlobalFrees(op->buf);
		op->buf = (char*)GlobalAlloc(GMEM_ZEROINIT, 4);
		if (NULL == op->buf) {
			return false;
		}
		op->bufSize = 4;

	} else {
		if (NULL != m_fun) {
			m_fun(this, &m_so, 2, op->buf, op->cb);
		}
	}
	op->bufOffset = 0;
	return cli_SoRecv(op);
}
bool client::cli_send(char* n_buf, DWORD n_len) {
	char* buf = NULL;
	int len = 0;
	if (1 == m_modle) {
		len = 4 + n_len;
		buf = (char*)GlobalAlloc(GMEM_ZEROINIT, len);
		if (NULL == buf) {
			return false;
		}
		WriteProcessMemory(INVALID_HANDLE_VALUE, buf, &n_len, 4, NULL);
		WriteProcessMemory(INVALID_HANDLE_VALUE, buf + 4, n_buf, n_len, NULL);
	}
	else {
		buf = n_buf;
		len = n_len;
	}

	send(m_so, buf, len, 0);
	if (1 == m_modle) {
		GlobalFrees(buf);
	}

	return true;
}
bool client::cli_close() {
	m_close = true;
	closesockets(m_so);
	m_so = INVALID_SOCKET;
	m_so = INVALID_SOCKET;
	GlobalFrees(m_record_char);
	return true;
}
bool client::cli_socks4(PCHAR host, WORD port, PCHAR username, PCHAR userpass)
{
	unsigned long ul = 0;
	ioctlsocket(m_so, FIONBIO, (unsigned long *)&ul);
	userpass = userpass;
	PHOSTENT ph = gethostbyname(host);

	if (NULL == ph) {
		return 0;
	}

	DWORD ip = *LPDWORD(ph->h_addr_list[0]);

	int cb = 0;

	BYTE buffer[1024];

	buffer[cb] = 0x04; cb++;
	buffer[cb] = 0x01; cb++;
	buffer[cb] = BYTE(port >> 8); cb++;
	buffer[cb] = BYTE(port >> 0); cb++;
	buffer[cb] = BYTE(ip >> 0x00); cb++;
	buffer[cb] = BYTE(ip >> 0x08); cb++;
	buffer[cb] = BYTE(ip >> 0x10); cb++;
	buffer[cb] = BYTE(ip >> 0x18); cb++;

	lstrcpyA((PCHAR)buffer + cb, username); cb += 1 + lstrlenA(username);

	cb = send(m_so, (PCHAR)buffer, cb, 0);

	if (cb <= 0){
		return 0;
	}

	cb = recv(m_so, (PCHAR)buffer, sizeof(buffer), 0);

	if (cb <= 0){
		return 0;
	}

	if (buffer[0] != 0x00){
		return 0;
	}

	if (buffer[1] != 0x5A){
		return 0;
	}

	return 1;
}
bool client::cli_socks5(PCHAR host, WORD port, PCHAR username, PCHAR userpass)
{
	unsigned long ul = 0;
	ioctlsocket(m_so, FIONBIO, (unsigned long *)&ul);
	BYTE buffer[1024];

	buffer[0x00] = 0x05;//VER
	buffer[0x01] = 0x02;//NMETHODS 
	buffer[0x02] = 0x00;//X'00' NO AUTHENTICATION REQUIRED
	buffer[0x03] = 0x02;//X'02' USERNAME/PASSWORD

	int cb;

	cb = send(m_so, (PCHAR)buffer, 0x04, 0);

	if (cb <= 0){
		return 0;//WSAGetLastError();
	}

	cb = recv(m_so, (PCHAR)buffer, sizeof(buffer), 0);

	if (cb <= 0){
		return 0;//WSAGetLastError();
	}

	if (buffer[0x00] != 0x05) {
		return 0;//(WSAECONNRESET);
	}

	if (buffer[0x01] == 0x02){
		int cbs = 0;
		int cbuser = lstrlenA(username);
		int cbpass = lstrlenA(userpass);

		buffer[cbs] = 0x01;		cbs++; //密码验证
		buffer[cbs] = (BYTE)cbuser;	cbs++; cbs += (wsprintfA((PCHAR)buffer + cbs, username, cbuser));
		buffer[cbs] = (BYTE)cbpass;	cbs++; cbs += (wsprintfA((PCHAR)buffer + cbs, userpass, cbpass));

		cb = send(m_so, (PCHAR)buffer, cbs, 0);

		if (cb <= 0){
			return 0;//WSAGetLastError();
		}


		cb = recv(m_so, (PCHAR)buffer, sizeof(buffer), 0);

		if (cb <= 0){
			return 0;//WSAGetLastError();
		}

		if (buffer[0x00] != 0x05){
			//RFC 1929 NO EXPLAIN(所以忽略验证)
		}

		if (buffer[0x01] != 0x00){
			return 0;//WSAECONNRESET;
		}

	}
	else if (buffer[0x01] == 0x00){
	}
	else{
		return 0;//WSAECONNRESET;
	}

	int cbs = 0;
	int cbhost = lstrlenA(host);

	buffer[cbs] = 0x05;			cbs++;
	buffer[cbs] = 0x01;			cbs++;
	buffer[cbs] = 0x00;			cbs++;
	buffer[cbs] = 0x03;			cbs++;//DOMAIN
	buffer[cbs] = (BYTE)cbhost;		cbs++; cbs += (wsprintfA((PCHAR)buffer + cbs, host, cbhost));
	buffer[cbs] = (port >> 8);	cbs++;
	buffer[cbs] = (BYTE)(port >> 0);	cbs++;

	cb = send(m_so, (PCHAR)buffer, cbs, 0);

	if (cb <= 0){
		return 0;//WSAGetLastError();
	}

	cb = recv(m_so, (PCHAR)buffer, sizeof(buffer), 0);

	if (cb <= 0){
		return 0;//WSAGetLastError();
	}


	if (buffer[0x00] != 0x05){
		return 0;//WSAECONNRESET;
	}

	if (buffer[0x01] != 0x00){
		return 0;//WSAECONNRESET;
	}

	return 1;
}
bool client::cli_http(PCHAR host, WORD port, PCHAR username, PCHAR userpass)
{
	int cb = 0;
	int cbhdr;
	int cbrecv;

	char http[8192];

	cb += (wsprintfA((PCHAR)http + cb, "CONNECT %s:%d HTTP/1.1\r\n", host, port));
	cb += (wsprintfA((PCHAR)http + cb, "Host: %s:%d \r\n", host, port));
	cb += (wsprintfA((PCHAR)http + cb, "Proxy-Connection: Keep-Alive\r\n"));


	if (username && username[0])
	{
		char b64[128];
		char b64encode[256];

		lstrcpyA(b64, username);
		lstrcatA(b64, ":");
		lstrcatA(b64, userpass);

		if (cli_base64((PBYTE)b64, lstrlenA(b64), (PBYTE)b64encode, 256))
		{
			cb += (wsprintfA((PCHAR)http + cb, "Proxy-Authorization: Basic %s\r\n", b64encode));
		}

	}
	else
	{
		cb += (wsprintfA((PCHAR)http + cb, "Proxy-Authorization: Basic *\r\n"));
	}

	cb += (wsprintfA((PCHAR)http + cb, "Content-length: 0\r\n\r\n"));

	unsigned long ul = 0;
	ioctlsocket(m_so, FIONBIO, (unsigned long *)&ul);
	cb = send(m_so, (PCHAR)http, cb, 0);


	if (cb <= 0){
		return false;
	}


	cb = 0;
	cb = 0;
	cb = 0;

	while (1){

		cbrecv = recv(m_so, (PCHAR)http, 8192 - cb, 0);

		if (cbrecv <= 0){
			return false;
		}

		cb = cb + cbrecv;

		for (cbhdr = 4; cbhdr <= cb; cbhdr++){
			if (http[cbhdr - 4] == '\r')
			if (http[cbhdr - 3] == '\n')
			if (http[cbhdr - 2] == '\r')
			if (http[cbhdr - 1] == '\n'){
				break;
			}
		}

		if (cbhdr <= cb){
			break;
		}
	}

	if (0 != (memcmp(http, "HTTP/1.0 200 ", 13)))
	if (0 != (memcmp(http, "HTTP/1.1 200 ", 13))){
		return 0;
	}
	return true;
}
char client::cli_b2c(BYTE b) {
	if ((00 <= b) && (b <= 25)) { return char(b + 'A' - 00); }
	else if ((26 <= b) && (b <= 51)) { return char(b + 'a' - 26); }
	else if ((52 <= b) && (b <= 61)) { return char(b + '0' - 52); }
	else if (62 == b) { return '+'; }
	else if (63 == b) { return '/'; }
	else{
		return '=';
	}
}
int client::cli_base64(LPBYTE srcbuf, int cbsrc, LPBYTE dstbuf, int cbdst) {
	cbdst = 256;
	int i, cnt = cbsrc / 3;

	for (i = 0; i < cnt + 1; i++){
		dstbuf[i * 4 + 0] = cli_b2c(((srcbuf[i * 3 + 0] >> 2)));
		dstbuf[i * 4 + 1] = cli_b2c(((srcbuf[i * 3 + 0] & 0x03) << 4) + (srcbuf[i * 3 + 1] >> 4));
		dstbuf[i * 4 + 2] = cli_b2c(((srcbuf[i * 3 + 1] & 0x0F) << 2) + (srcbuf[i * 3 + 2] >> 6));
		dstbuf[i * 4 + 3] = cli_b2c(((srcbuf[i * 3 + 2] & 0x3F)));
	}

	if (cbsrc % 3 == 1){
		dstbuf[cnt * 4 + 2] = '=';
		dstbuf[cnt * 4 + 3] = '=';

		cnt++;
	}
	else if (cbsrc % 3 == 2){
		dstbuf[cnt * 4 + 3] = '=';

		cnt++;
	}

	dstbuf[cnt * 4] = 0;

	return cnt * 4;
}
