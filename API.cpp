#include "stdafx.h"
#include "BBTCP.h"

ULONG CALLBACK bbtcp_client(void * pParam) {
	while (g_open) {
		DWORD cb = NULL;
		ULONG_PTR key = NULL;
		P_tcpop op = NULL;
		if (!GetQueuedCompletionStatus(g_iocp, &cb, (ULONG_PTR*)&key, (LPOVERLAPPED*)&op, INFINITE)) {
			if (NULL == op) {
				continue;
			}
			if (NULL != op->so->m_fun && op->so->m_close == false) {
				op->so->m_fun(op->so, &op->so->m_so, 3, 0, 0);
			}
			GlobalFrees(op->buf);
			GlobalFrees(op->so->m_record_char);
			op->so->m_close = false;
			GlobalFrees(op);
			continue;
		}
		
		if (NULL == op) {
			continue;
		}
		if (op->so->m_close) 	{
			GlobalFrees(op->buf);
			GlobalFrees(op->so->m_record_char);
			op->so->m_close = false;
			GlobalFrees(op);
			continue;
		}
		op->cb = cb;
		switch (op->state) {
		case 1:
			if (NULL != op->so->m_fun) {
				op->so->m_fun(op->so, &op->so->m_so, 1, 0, 0);
			}
			if (!op->so->cli_connect_fun(op)) {
				if (NULL != op->so->m_fun && op->so->m_close == false) {
					op->so->m_fun(op->so, &op->so->m_so, 3, 0, 0);
				}
				if (op->so->m_close) {
					GlobalFrees(op->buf);
					GlobalFrees(op->so->m_record_char);
					op->so->m_close = false;
					GlobalFrees(op);
					continue;
				}
				closesockets(op->so->m_so);
 				GlobalFrees(op->buf);
				GlobalFrees(op->so->m_record_char);
				op->so->m_close = false;
 				GlobalFrees(op);
				continue;
			}
			
			break;
		case 2:
			if (cb <= 0) {
				op->so->m_fun(op->so, &op->so->m_so, 3, 0, 0);
				closesockets(op->so->m_so);
				GlobalFrees(op->buf);
				GlobalFrees(op->so->m_record_char);
				op->so->m_close = false;
				GlobalFrees(op);
				continue;
			}
			if (op->so->cli_recv(op) == 0) {
				if (NULL != op->so->m_fun && op->so->m_close == false) {
					op->so->m_fun(op->so, &op->so->m_so, 3, 0, 0);
				}
				if (op->so->m_close) {
					GlobalFrees(op->buf);
					GlobalFrees(op->so->m_record_char);
					op->so->m_close = false;
					GlobalFrees(op);
					continue;
				}
				closesockets(op->so->m_so);
				GlobalFrees(op->buf);
				GlobalFrees(op->so->m_record_char);
				op->so->m_close = false;
				GlobalFrees(op);
				continue;
			}
			break;
		}
	}

	CloseHandles(g_iocp);
	return 0;
}
int WINAPI bbtcp_cli_load() {
	if (NULL != g_iocp) {
		return 1;
	}
	WSAData wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		return 0;
	}
	if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2) {
		WSACleanup();
		return 0;
	}
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (NULL == g_iocp) {
		WSACleanup();
		return 0;
	}
	return 1;
}

int WINAPI bbtcp_cli_free() {
	g_open = false;
	return 1;
}
int WINAPI bbtcp_cli_init(DWORD Thread = 0) {
	if (g_ser_thread > 0) {
		return 0;
	}
	if (Thread <= 0) {
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		g_ser_thread = info.dwNumberOfProcessors * 2 + 2;
		if (g_ser_thread < 2) {
			g_ser_thread = 2;
		}
	} else {
		g_ser_thread = Thread;
	}
	for (DWORD i = 0; i < g_ser_thread; i++) {
		CloseHandle(CreateThread(NULL, 0, &bbtcp_client, NULL, 0, NULL));
	} 
	return 1;
}
client* WINAPI bbtcp_cli_connect(ser_fun n_fun, char* n_ip, USHORT n_port, int n_modle, int n_time, int n_blen, DWORD n_buflen,
	int n_proxy, char* n_proxyip, USHORT n_proxyport, char* n_pacc, char* n_pass, bool ipv6) {
	client* cli = (client*)GlobalAlloc(GMEM_ZEROINIT, sizeof(client));
	if (NULL == cli) {
		return 0;
	}
	bool ret = cli->cli_connect(n_fun, n_ip, n_port, n_modle, n_time, n_blen, n_buflen, n_proxy, n_proxyip, n_proxyport, n_pacc, n_pass, ipv6);
	if (ret) {
		return cli;
	}
	GlobalFrees(cli);
	return 0;
}
int WINAPI bbtcp_cli_send(client* cli, char* ndata, ULONG nsize) {
	return cli->cli_send(ndata, nsize);
}
bool WINAPI bbtcp_cli_close(client* cli) {
	cli->cli_close();
	while (cli->m_close) {
		Sleep(1);
	}
	GlobalFrees(cli);
	return true;
}
void WINAPI bbtcp_cli_set_record_char(client* clithis, char* nrecord) {
	if (clithis->m_record_char != NULL) {
		GlobalFrees(clithis->m_record_char);
	}
	clithis->m_record_char = (char*)GlobalAlloc(GMEM_ZEROINIT, strlen(nrecord)+1);
	if (NULL == clithis->m_record_char) {
		return;
	}
	WriteProcessMemory(INVALID_HANDLE_VALUE, clithis->m_record_char, nrecord, strlen(nrecord), NULL);
	WriteProcessMemory(INVALID_HANDLE_VALUE, clithis->m_record_char + strlen(nrecord), 0, 1, NULL);
}
char* WINAPI bbtcp_cli_get_record_char(client* clithis) {
	return clithis->m_record_char;
}
void WINAPI bbtcp_cli_set_record_int(client* clithis, int nrecord) {
	clithis->m_record_int = nrecord;
}
int WINAPI bbtcp_cli_get_record_int(client* clithis) {
	return clithis->m_record_int;
}
BOOL APIENTRY main( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	return TRUE;
}

