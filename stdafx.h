#pragma once

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