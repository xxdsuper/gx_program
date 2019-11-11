#pragma once

#include <WS2spi.h>
#include <SpOrder.h>
#include <Windows.h>
#include <stdio.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib, "Rpcrt4.lib")

GUID ProviderGuid = { 0xd3c21122, 0x85e1, 0x48f3,
{ 0x9a, 0xb6, 0x23, 0xd9, 0x0c, 0x73, 0x07, 0xef }
};

class CLSPInstaller {
public:
	CLSPInstaller();
	~CLSPInstaller();

	BOOL InstallProvider(WCHAR* wszDllPath);
	BOOL RemoveProvider();
private:
	LPWSAPROTOCOL_INFOW GetProvider(LPINT lpnTotalProtocols);
	void FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo);
};
