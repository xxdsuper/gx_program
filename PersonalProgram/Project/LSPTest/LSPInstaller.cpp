#include "LSPInstaller.h"

CLSPInstaller::CLSPInstaller() {

}

CLSPInstaller::~CLSPInstaller() {

}

LPWSAPROTOCOL_INFOW CLSPInstaller::GetProvider(LPINT lpnTotalProtocols)
{
	//��������LSPЭ��
	LPWSAPROTOCOL_INFOW pProtoInfo = NULL;
	int nError;
	DWORD dwSize = 0;
	//��ȡ����Ҫ�ĳ���
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
	WCHAR wszLSPName[] = L"DongLSP"; //LSPЭ������

	LPWSAPROTOCOL_INFOW pProtoInfo;
	int nProtocols;
	WSAPROTOCOL_INFOW OriginalProtocolInfo[3]; //�����ԱΪTCP��UDP��ԭʼ��Ŀ¼�����Ϣ
	DWORD dwOrigCatalogId[3]; //
	int nArrayCount = 0;
	DWORD dwLayeredCatalogId; // ���Ƿֲ�Э���Ŀ¼ID��
	int nError = 0;

	//�ҵ��Լ������Э��ŵ�Э��������
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
			//ȥ��XP1_IFS_HANDLES��־,��ֹ�ṩ�߷��صľ���������Ĳ���ϵͳ���
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

	/*��װ�Լ��ķֲ�Э��*/
	//�����Լ��ķֲ�Э��
	WSAPROTOCOL_INFOW LayeredProtocolInfo;
	memcpy(&LayeredProtocolInfo, &OriginalProtocolInfo[0], sizeof(WSAPROTOCOL_INFOW));
	//�޸�Э������ƣ����ͣ���ʽ
	wcscpy_s(LayeredProtocolInfo.szProtocol, wszLSPName);
	LayeredProtocolInfo.ProtocolChain.ChainLen = LAYERED_PROTOCOL; //��ʾ�ֲ�Э��
	LayeredProtocolInfo.dwProviderFlags |= PFL_HIDDEN; //��ʾ��ʽΪ���ṩ���Լ�����
	//��װ�ֲ�Э��
	if (WSCInstallProvider(&ProviderGuid, wszDllPath, &LayeredProtocolInfo, 1, &nError) == SOCKET_ERROR)
	{
		FreeProvider(pProtoInfo);
		return FALSE;
	}
	FreeProvider(pProtoInfo);
	//����ö��Э�飬��ȡ�ֲ�Э��Ŀ¼ID
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
	//��װЭ����
	//�޸�Э�����ƣ�����
	WCHAR wszChainName[WSAPROTOCOL_LEN + 1]; //�·ֲ�Э�������
	for (size_t i = 0; i < nArrayCount; i++)
	{
		swprintf_s(wszChainName, L"%s over %s", wszLSPName, OriginalProtocolInfo[i].szProtocol);
		wcscpy_s(OriginalProtocolInfo[i].szProtocol, wszChainName);
		if (OriginalProtocolInfo[i].ProtocolChain.ChainLen == 1) //���ǻ���Э���ģ��
		{
			//�޸Ļ���Э��ģ���Э����, ��Э����[1]д������UDP[����Э��]�����ID
			OriginalProtocolInfo[i].ProtocolChain.ChainEntries[1] = dwOrigCatalogId[i];
		}
		else //�������1,�൱���Ǹ�Э����,��ʾ����Э�����е����ID,ȫ�������һ��,����[0]
		{
			for (size_t j = OriginalProtocolInfo[i].ProtocolChain.ChainLen; j > 0; j--)
			{
				OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j] = OriginalProtocolInfo[i].ProtocolChain.ChainEntries[j - 1];
			}
		}
		//���·ֲ�Э�����ڻ���Э���ǰ�棨���ΪЭ�����ž����ڿ�ͷ�ˣ�
		OriginalProtocolInfo[i].ProtocolChain.ChainLen++;
		OriginalProtocolInfo[i].ProtocolChain.ChainEntries[0] = dwLayeredCatalogId;
	}
	//һ�ΰ�װ����Э����
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

	// ��������WinsockĿ¼�������ǵ�Э������ǰ,��ϵͳ�ȵ������ǵ�Э�飨��Э�����ŵ�һ,Э������[0]���·ֲ�Э��,[1]����UDPЭ�飩
	// ����ö�ٰ�װ��Э��
	FreeProvider(pProtoInfo);
	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;
	PDWORD dwIds = (PDWORD)malloc(sizeof(DWORD) * nProtocols);
	int nIndex = 0;
	//����Լ���Э����
	for (size_t i = 0; i < nProtocols; i++)
	{
		//������Լ���Э����
		if ((pProtoInfo[i].ProtocolChain.ChainLen > 1) &&
			(pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}
	//�������Э����
	for (size_t i = 0; i < nProtocols; i++)
	{
		//����ǻ���Э��,�ֲ�Э��(���������ǵ�Э����,���������ǵķֲ�Э��)
		if ((pProtoInfo[i].ProtocolChain.ChainLen <= 1) &&
			(pProtoInfo[i].ProtocolChain.ChainEntries[0] != dwLayeredCatalogId))
			dwIds[nIndex++] = pProtoInfo[i].dwCatalogEntryId;
	}

	//��������winsockĿ¼
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
	DWORD dwLayeredCatalogId = 0; //�ֲ�Э���ṩ�ߵ����ID

	pProtoInfo = GetProvider(&nProtocols);
	if (nProtocols < 1 || pProtoInfo == NULL)
		return FALSE;

	int nError = 0;
	BOOL bFind = FALSE;
	for (size_t i = 0; i < nProtocols; i++)
	{
		//���ҷֲ�Э���ṩ��
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
			//����Э����(���Э������[0]Ϊ�ֲ�Э���ṩ��)
			if ((pProtoInfo[i].ProtocolChain.ChainLen > 1) && (pProtoInfo[i].ProtocolChain.ChainEntries[0] == dwLayeredCatalogId))
			{
				//ж��Э����
				WSCDeinstallProvider(&pProtoInfo[i].ProviderId, &nError);
				break;
			}
		}
		//�Ƴ��ֲ�Э��
		WSCDeinstallProvider(&ProviderGuid, &nError);
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}