#include "stdafx.h"
HANDLE g_iocp = NULL;
bool g_open = true;
DWORD g_ser_thread = 0;
void int2char(char* data, int val)
{
	wsprintfA(data, "%d", val);
}
void closesockets(SOCKET x) {
	BOOL bDontLinger = FALSE;
	setsockopt(x, SOL_SOCKET, SO_DONTLINGER, (const char*)&bDontLinger, sizeof(BOOL));
	



	linger m_sLinger = {1,0};
	setsockopt(x, SOL_SOCKET, SO_LINGER, (const char*)&m_sLinger, sizeof(linger));
	closesocket(x);
	x = INVALID_SOCKET;
}