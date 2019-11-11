#include "LSPInstaller.h"

CLSPInstaller::CLSPInstaller() {

}

CLSPInstaller::~CLSPInstaller() {

}

LPWSAPROTOCOL_INFOW CLSPInstaller::GetProvider(LPINT lpnTotalProtocols)
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

void CLSPInstaller::FreeProvider(LPWSAPROTOCOL_INFOW pProtoInfo)
{
	GlobalFree(pProtoInfo);
}

BOOL CLSPInstaller::InstallProvider(WCHAR* wszDllPath)
{
	WCHAR wszLSPName[] = L"DongLSP"; //LSP协议名称

	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW OriginalProtocolInfo[3]; //数组成员为TCP、UDP、原始的目录入口信息
	DWORD dwOrigCatalogId[3]; //
	int nArrayCount = 0;
	DWORD dwLayeredCatalogId; // 我们分层协议的目录ID号
	int nError = 0;

	//找到自己定义的协议放到协议数组中
	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;

	BOOL bFindTcp = FALSE;
	BOOL bFindUdp = FALSE;
	BOOL bFindRaw = FALSE;

	for (size_t i = 0; i < nProtocols; i++)
	{
		if (pProtoInfo[i].iAddressFamily == AF_INET)
		{
			//去除XP1_IFS_HANDLES标志,防止提供者返回的句柄是真正的操作系统句柄
			if (!bFindUdp && (pProtoInfo[i].iProtocol == IPPROTO_UDP))
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES);
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
				bFindUdp = TRUE;
			}
			if (!bFindTcp && (pProtoInfo[i].iProtocol == IPPROTO_TCP))
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES);
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
				bFindTcp = TRUE;
			}
			if (!bFindRaw && (pProtoInfo[i].iProtocol == IPPROTO_IP))
			{
				memcpy(&OriginalProtocolInfo[nArrayCount], &pProtoInfo[i], sizeof(WSAPROTOCOL_INFOW));
				OriginalProtocolInfo[nArrayCount].dwServiceFlags1 = OriginalProtocolInfo[nArrayCount].dwServiceFlags1 & (~XP1_IFS_HANDLES);
				dwOrigCatalogId[nArrayCount++] = pProtoInfo[i].dwCatalogEntryId;
				bFindRaw = TRUE;
			}
		}
	}

	/*安装自己的分层协议*/
	//构造自己的分层协议
	WSAPROTOCOL_INFOW LayeredProtocolInfo;
	memcpy(&LayeredProtocolInfo, &OriginalProtocolInfo[0], sizeof(WSAPROTOCOL_INFOW));
	//修改协议的名称，类型，方式
	wcscpy_s(LayeredProtocolInfo.szProtocol, wszLSPName);
	LayeredProtocolInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL; //表示分层协议
	LayeredProtocolInfo.dwProviderFlags |= PFL_HIDDEN; //表示方式为由提供者自己设置
	//安装分层协议
	if (WSCInstallProvider(&ProviderGuid, wszDllPath, &LayeredProtocolInfo, 1, &nError) == SOCKET_ERROR)
	{
		FreeProvider(pProtoInfo);
		return FALSE;
	}
	FreeProvider(pProtoInfo);
	//重新枚举协议，获取分层协议目录ID
	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;
	for (size_t i = 0; i < nProtocols; i++)
	{
		if (memcmp(&pProtoInfo[i].ProviderId, &ProviderGuid, sizeof(GUID)) == 0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			break;
		}
	}
	//安装协议链
	//修改协议名称，类型
	WCHAR wszChainName[WSAPROTOCOL_LEN + 1]; //新分层协议的名称
	for (size_t i = 0; i < nArrayCount; i++)
	{
		swprintf_s(wszChainName, L"%s over %s", wszLSPName, OriginalProtocolInfo[i].szProtocol);
		wcscpy_s(OriginalProtocolInfo[i].szProtocol, wszChainName);
		if (OriginalProtocolInfo[i].ProtocolChain.ChainLen == 1) //这是基础协议的模板
		{
			//修改基础协议模板的协议链, 在协议链[1]写入真正UDP[基础协议]的入口ID
			OriginalProtocolInfo[i].ProtocolChain.ChainEntries[1] = dwOrigCatalogId[i];
		}
		else //如果大于1,相当于是个协议链,表示：将协议链中的入口ID,全部向后退一格,留出[0]
		{
			for (size_t j = OriginalProtocolInfo[i].ProtocolChain.ChainLen; j > 0; j--)
			{
				OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j] = OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j - 1];
			}
		}
		//让新分层协议排在基础协议的前面（如果为协议链排就排在开头了）
		OriginalProtocolInfo[i].ProtocolChain.ChainLen++;
		OriginalProtocolInfo[i].ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;
	}
	//一次安装三个协议链
	GUID ProviderChainGuid;
	if (::UuidCreate(&ProviderChainGuid) == RPC_S_OK)
	{
		if (WSCInstallProvider(&ProviderChainGuid,
			wszDllPath, OriginalProtocolInfo, nArrayCount, &nError) == SOCKET_ERROR)
		{
			FreeProvider(pProtoInfo);
			return FALSE;
		}
	}
	else
	{
		FreeProvider(pProtoInfo);
		return FALSE;
	}

	// 重新排序Winsock目录，将我们的协议链提前,让系统先调用我们的协议（让协议链排第一,协议链中[0]是新分层协议,[1]基础UDP协议）
	// 重新枚举安装的协议
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;
	PDWORD dwIds = (PDWORD)malloc(sizeof(DWORD) * nProtocols);
	int nIndex = 0;
	//添加自己的协议链
	for (size_t i = 0; i < nProtocols; i++)
	{
		//如果是自己的协议链
		if ((pProtoInfo[i].ProtocolChain.ChainLen > 1) &&
			(pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}
	//添加其他协议链
	for (size_t i = 0; i < nProtocols; i++)
	{
		//如果是基础协议,分层协议(不包括我们的协议链,但包括我们的分层协议)
		if ((pProtoInfo[i].ProtocolChain.ChainLen <= 1) &&
			(pProtoInfo[i].ProtocolChain.ChainEntries[0] != dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}

	//重新排序winsock目录
	if ((nError = WSCWriteProviderOrder(dwIds, nIndex)) != ERROR_SUCCESS)
	{
		return FALSE;
	}
	FreeProvider(pProtoInfo);
	return TRUE;
}


BOOL CLSPInstaller::RemoveProvider()
{
	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	DWORD dwLayeredCatalogId = 0; //分层协议提供者的入口ID

	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;

	int nError = 0;
	BOOL bFind = FALSE;
	for (size_t i = 0; i < nProtocols; i++)
	{
		//查找分层协议提供者
		if (memcmp(&ProviderGuid, &pProtoInfo[i].ProviderId, sizeof(ProviderGuid)) == 0)
		{
			dwLayeredCatalogId = pProtoInfo[i].dwCatalogEntryId;
			bFind = TRUE;
			break;
		}
	}

	if (bFind)
	{
		for (size_t i = 0; i < nProtocols; i++)
		{
			//查找协议链(这个协议链的[0]为分层协议提供者)
			if ((pProtoInfo[i].ProtocolChain.ChainLen > 1) && (pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			{
				//卸载协议链
				WSCDeinstallProvider(&pProtoInfo[i].ProviderId, &nError);
				break;
			}
		}
		//移除分层协议
		WSCDeinstallProvider(&ProviderGuid, &nError);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}