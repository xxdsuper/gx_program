// DongLSP.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <ws2spi.h>
#pragma comment(lib,"Ws2_32.lib")

//不建议使用硬编码格式，在实际需要时自动生成
GUID filterguid = { 0xd3c21122, 0x85e1, 0x48f3,
{ 0x9a, 0xb6, 0x23, 0xd9, 0x0c, 0x73, 0x07, 0xef }
};

WSPPROC_TABLE g_pNextProcTable;
WSPUPCALLTABLE g_pUpCallTable;      // 上层函数列表。如果LSP创建了自己的伪句柄，才使用这个函数列表
LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols)
{
	//遍历所有LSP协议
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;
	int nError;
	DWORD dwSize = 0;
	//获取所需要的长度
	if (WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError) == SOCKET_ERROR)
	{
		if (nError != WSAENOBUFS)
		{
			OutputDebugString(L"First WSCEnumProtocols Error!\r\n");
			return NULL;
		}
	}
	if ((pProtoInfo = (LPWSAPROTOCOL_INFOW)GlobalAlloc(GPTR, dwSize)) == NULL)
	{
		OutputDebugString(L"GlobalAlloc Error!\r\n");
		return NULL;
	}
	if ((*lpnTotalProtocols = WSCEnumProtocols(NULL, pProtoInfo, &dwSize, &nError)) == SOCKET_ERROR)
	{
		OutputDebugString(L"Second WSCEnumProtocols Error!\r\n");
		return NULL;
	}
	return pProtoInfo;
}

void FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo)
{
	GlobalFree(pProtoInfo);
}

int WSPAPI WSPConnect(
	__in   SOCKET s,
	__in   const struct sockaddr* name,
	__in   int namelen,
	__in   LPWSABUF lpCallerData,
	__out  LPWSABUF lpCalleeData,
	__in   LPQOS lpSQOS,
	__in   LPQOS lpGQOS,
	__out  LPINT lpErrno) {
	OutputDebugString(L"WSPConnect……\r\n");
	return g_pNextProcTable.lpWSPConnect(s, name, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS, lpErrno);
}

int WSPAPI WSPSend(
	__in   SOCKET s,
	__in   LPWSABUF lpBuffers,
	__in   DWORD dwBufferCount,
	__out  LPDWORD lpNumberOfBytesSent,
	__in   DWORD dwFlags,
	__in   LPWSAOVERLAPPED lpOverlapped,
	__in   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	__in   LPWSATHREADID lpThreadId,
	__out  LPINT lpErrno) {
	OutputDebugString(L"WSPSend……\r\n");
	return g_pNextProcTable.lpWSPSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WSPAPI WSPSendTo(
	__in   SOCKET s,
	__in   LPWSABUF lpBuffers,
	__in   DWORD dwBufferCount,
	__out  LPDWORD lpNumberOfBytesSent,
	__in   DWORD dwFlags,
	__in   const struct sockaddr* lpTo,
	__in   int iTolen,
	__in   LPWSAOVERLAPPED lpOverlapped,
	__in   LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	__in   LPWSATHREADID lpThreadId,
	__out  LPINT lpErrno) {
	OutputDebugString(L"WSPSendTo……\r\n");
	return g_pNextProcTable.lpWSPSendTo(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpTo, iTolen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WSPAPI WSPRecv(
	__in     SOCKET s,
	__inout  LPWSABUF lpBuffers,
	__in     DWORD dwBufferCount,
	__out    LPDWORD lpNumberOfBytesRecvd,
	__inout  LPDWORD lpFlags,
	__in     LPWSAOVERLAPPED lpOverlapped,
	__in     LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	__in     LPWSATHREADID lpThreadId,
	__out    LPINT lpErrno) {
	OutputDebugString(L"WSPRecvFrom……\r\n");
	return g_pNextProcTable.lpWSPRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WINAPI WSPRecvFrom(
	__in SOCKET s,
	__inout LPWSABUF lpBuffers,
	__in DWORD dwBufferCount,
	__out LPDWORD lpNumberOfBytesRecvd,
	__inout LPDWORD lpFlags,
	__out struct sockaddr *lpFrom,
	__inout LPINT lpFromlen,
	__in LPWSAOVERLAPPED lpOverlapped,
	__in LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
	__in LPWSATHREADID lpThreadId,
	__inout LPINT lpErrno
)
{
	OutputDebugString(L"WSPRecvFrom……\r\n");
	return g_pNextProcTable.lpWSPRecvFrom(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpFrom, lpFromlen, lpOverlapped, lpCompletionRoutine, lpThreadId, lpErrno);
}

int WSPAPI WSPStart(
	__in   WORD wVersionRequested,
	__out  LPWSPDATA lpWSPData,
	__in   LPWSAPROTOCOL_INFO lpProtocolInfo,
	__in   WSPUPCALLTABLE UpcallTable,
	__out  LPWSPPROC_TABLE lpProcTable
) {
	OutputDebugString(L"IPFilter WSPStart……\r\n");
	if (lpProtocolInfo->ProtocolChain.ChainLen <= 1)
	{
		//无法加载或初始化请求的服务提供程序
		OutputDebugString(L"ChainLen<=1……\r\n");
		return WSAEPROVIDERFAILEDINIT;
	}

	//找到下层协议的WSAPROTOCOL_INFOW结构体
	WSAPROTOCOL_INFOW NextProtocolInfo;
	int nTotalProtos = 0;
	LPWSAPROTOCOL_INFOW pProtoInfo = GetProvider(&nTotalProtos);
	//下层入口
	DWORD dwBaseEntryId = 0;
	DWORD dwNextEntryId = 0;
	for (size_t i = 0; i < nTotalProtos; i++)
	{
		if (memcmp(&pProtoInfo[i].ProviderId, &filterguid, sizeof(GUID)) == 0)
		{
			dwBaseEntryId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}

	//DWORD dwBaseEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[1];
	/*
	Setting ChainLen to zero indicates a layered protocol				ChainLen = 0,表示为分层协议
	Setting ChainLen to one indicates a base protocol					ChainLen = 1,表示为基础协议， TCP/UDP/IP
	Setting ChainLen to greater than one indicates a protocol chain		ChainLen > 1,表示为协议链
	*/
	for (size_t i = 0; i < lpProtocolInfo->ProtocolChain.ChainLen; i++)
	{
		if (lpProtocolInfo->ProtocolChain.ChainEntries[i] == dwBaseEntryId)
		{
			dwNextEntryId = lpProtocolInfo->ProtocolChain.ChainEntries[i + 1];
			break;
		}
	}

	for (size_t i = 0; i < nTotalProtos; i++)
	{
		if (pProtoInfo[i].dwCatalogEntryId == dwNextEntryId)
		{
			memcpy(&NextProtocolInfo, &pProtoInfo[i], sizeof(NextProtocolInfo));
			break;
		}
	}
	//加载下层协议
	int nError;
	TCHAR szBaseProviderDll[MAX_PATH];
	int nLen = MAX_PATH;
	/*filterpathlen = MAX_PATH;
	filterpath = (TCHAR*)GlobalAlloc(GPTR, filterpathlen);*/
	if (WSCGetProviderPath(&NextProtocolInfo.ProviderId, szBaseProviderDll, &nLen, &nError) == SOCKET_ERROR)
	{
		OutputDebugString(L"WSCGetProviderPath Error!\r\n");
		return WSAEPROVIDERFAILEDINIT;
	}
	if (!ExpandEnvironmentStrings(szBaseProviderDll, szBaseProviderDll, MAX_PATH))
	{
		OutputDebugString(L"ExpandEnvironmentStrings Error!\r\n");
		return WSAEPROVIDERFAILEDINIT;
	}

	//加载下层协议
	HMODULE hFilter = ::LoadLibrary(szBaseProviderDll);
	if (hFilter == NULL)
	{
		OutputDebugString(L"LoadLibrary Error!\r\n");
		return WSAEPROVIDERFAILEDINIT;
	}
	LPWSPSTARTUP  pfnWSPStartup = NULL;
	pfnWSPStartup = (LPWSPSTARTUP)::GetProcAddress(hFilter, "WSPStartup");
	if (pfnWSPStartup == NULL)
	{
		OutputDebugString(L"GetProcessAddress Error!\r\n");
		return WSAEPROVIDERFAILEDINIT;
	}
	//调用下层提供程序的WSPStartup函数
	nError = pfnWSPStartup(wVersionRequested, lpWSPData, lpProtocolInfo, UpcallTable, lpProcTable);
	if (nError != ERROR_SUCCESS)
	{
		OutputDebugString(L"wspstartupfunc Error!\r\n");
		return nError;
	}

	g_pNextProcTable = *lpProcTable; //保存原来的入口函数表
	lpProcTable->lpWSPConnect = WSPConnect;
	lpProcTable->lpWSPSend = WSPSend;
	lpProcTable->lpWSPSendTo = WSPSendTo;
	lpProcTable->lpWSPRecv = WSPRecv;
	lpProcTable->lpWSPRecvFrom = WSPRecvFrom;

	FreeProvider(pProtoInfo);
	return nError;
}
