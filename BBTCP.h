#pragma once
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