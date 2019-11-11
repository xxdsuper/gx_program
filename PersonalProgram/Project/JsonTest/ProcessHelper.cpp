#include "ProcessHelper.h"

wstring CProcessHelper::GetProcessNameByID(DWORD dwProcID)
{
	wstring strProcessName = L"";
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return strProcessName;
	}
	BOOL bRun = Process32First(hSnapshot, &pe32);
	while (bRun)
	{
		if (pe32.th32ProcessID == dwProcID)
		{
			strProcessName = pe32.szExeFile;
		}
		bRun = Process32Next(hSnapshot, &pe32);
	}
	CloseHandle(hSnapshot);
	return strProcessName;
}

DWORD CProcessHelper::GetProcessIDByName(wstring strProName)
{
	DWORD dwProId = -1;
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return dwProId;
	}
	BOOL bRun = Process32First(hSnapshot, &pe32);
	while (bRun)
	{
		if (!_tcscmp(pe32.szExeFile, strProName.c_str()))
		{

			dwProId = pe32.th32ProcessID;
			break;
		}
		bRun = Process32Next(hSnapshot, &pe32);
	}
	CloseHandle(hSnapshot);
	return dwProId;
}

DWORD CProcessHelper::GetParentProcessID(DWORD dwProcID)
{
	typedef LONG(__stdcall *PROCNTQSIP)(HANDLE, UINT, PVOID, ULONG, PULONG);
	LONG							status;
	DWORD							dwParentPID = (DWORD)-1;
	HANDLE							hProcess;
	PROCESS_BASIC_INFORMATION		pbi;

	PROCNTQSIP NtQueryInformationProcess = (PROCNTQSIP)GetProcAddress(
		GetModuleHandle(L"ntdll"), "NtQueryInformationProcess");

	if (NULL == NtQueryInformationProcess)
	{
		return (DWORD)-1;
	}
	// Get process handle
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcID);
	if (!hProcess)
	{
		return (DWORD)-1;
	}

	// Retrieve information
	status = NtQueryInformationProcess(hProcess,
		SystemBasicInformation,
		(PVOID)&pbi,
		sizeof(PROCESS_BASIC_INFORMATION),
		NULL
	);

	// Copy parent Id on success
	if (!status)
	{
		dwParentPID = (ULONG_PTR)pbi.Reserved3;//pbi.InheritedFromUniqueProcessId;
	}

	CloseHandle(hProcess);

	return dwParentPID;
}

bool CProcessHelper::KillProcessByID(DWORD dwProcID, DWORD & dwErrCode)
{
	bool bRes = false;
	HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcID);
	if (hProcess != NULL)
	{
		::TerminateProcess(hProcess, 0);
		WaitForSingleObject(hProcess, INFINITE);
		::CloseHandle(hProcess);
		dwErrCode = 0;
		bRes = true;
	}
	else
	{
		dwErrCode = GetLastError();
	}
	return bRes;
}

bool CProcessHelper::KillProcessByName(wstring strProName, DWORD & dwErrCode)
{
	bool bRes = false;
	DWORD dwProcessID = GetProcessIDByName(strProName);
	bRes = KillProcessByID(dwProcessID, dwErrCode);
	return bRes;
}

bool CProcessHelper::IsProcessExist(const wstring strProName)
{
	bool bRes = false;
	//int nProcessCount = 0;
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(pe32);
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		//nProcessCount += 0;
		return bRes;
	}
	BOOL bRun = Process32First(hSnapshot, &pe32);
	while (bRun)
	{
		if (!_tcscmp(pe32.szExeFile, strProName.c_str()))
		{
			//nProcessCount += 1;
			bRes = true;
			break;
		}
		bRun = Process32Next(hSnapshot, &pe32);
	}
	CloseHandle(hSnapshot);
	return bRes;
}

HMODULE CProcessHelper::GetModule(const wchar_t * szModule)
{
	if (szModule == NULL)
	{
		return NULL;
	}
	HMODULE hModule = GetModuleHandleW(szModule);
	if (hModule == NULL)
	{
		hModule = LoadLibraryW(szModule);
	}
	if (hModule == NULL)
	{
		DWORD dwErr = GetLastError();
	}
	return hModule;
}

int CProcessHelper::GetProcessInfo(vector<PROCESS_INFO_ITEM>& vcProcessInfo)
{
	int nItemCount = 0;
	vcProcessInfo.clear();
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(pe32);
	HANDLE hSnapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		//nProcessCount += 0;
		return nItemCount;
	}
	BOOL bRun = Process32First(hSnapshot, &pe32);
	while (bRun)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
		if (hProcess)
		{
			wstring strProcessFile;
			TCHAR buff[MAX_PATH] = { 0 };
			DWORD dwLen = MAX_PATH;
			if (QueryFullProcessImageName(hProcess, 0, buff, &dwLen))
			{
				strProcessFile = buff;
			}

			DWORD sessionId;
			ProcessIdToSessionId(pe32.th32ProcessID, &sessionId);
			PROCESS_INFO_ITEM processItem;
			memset(&processItem, 0, sizeof(PROCESS_INFO_ITEM));
			processItem.dwProcessId = pe32.th32ProcessID;
			processItem.strProcessName = pe32.szExeFile;
			processItem.strProcessFile = strProcessFile;
			processItem.dwSessionId = sessionId;
			processItem.strUserName = GetProcessUserName(pe32.th32ProcessID);
			processItem.strCmdParam = GetProcessCmdParam(pe32.th32ProcessID);
			vcProcessInfo.push_back(processItem);
			nItemCount++;
		}
		bRun = Process32Next(hSnapshot, &pe32);
	}
	CloseHandle(hSnapshot);
	return nItemCount;
}

wstring CProcessHelper::GetProcessUserName(DWORD dwProcessID)
{
	TCHAR szUserName[MAX_PATH] = { 0 };
	TCHAR szDomain[MAX_PATH] = { 0 };
	DWORD dwDomainSize = MAX_PATH;
	DWORD dwNameSize = MAX_PATH;
	SID_NAME_USE    SNU;
	PTOKEN_USER pTokenUser = NULL;
	pTokenUser = (PTOKEN_USER)GetProcessTokenInformation(dwProcessID, TokenUser);
	if (pTokenUser == NULL)
	{
		return L"";
	}
	if (LookupAccountSid(NULL, pTokenUser->User.Sid, szUserName, &dwNameSize, szDomain, &dwDomainSize, &SNU) != 0)
	{
		return szUserName;
	}
}

wstring CProcessHelper::GetProcessCmdParam(DWORD dwProcessID)
{
	wstring strParam = L"";
	HANDLE							hProcess;
	// Get process handle
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	if (!hProcess)
	{
		return L"";
	}

	NTSTATUS	status;
	BOOL bWow64 = IsWow64(hProcess);
	if (!bWow64)
	{
		//32位程序获取64位程序的peb信息
		_NtWow64QueryInformationProcess64 NtWow64QueryInformationProcess64 = (_NtWow64QueryInformationProcess64)GetProcAddress(
			GetModuleHandle(L"ntdll"), "NtWow64QueryInformationProcess64");

		PROCESS_BASIC_INFORMATION_WOW64 pbi{};
		status = NtWow64QueryInformationProcess64(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
		if (!NT_SUCCESS(status))
		{
			return L"";
		}
		PEB64 peb;
		_NtWow64ReadVirtualMemory64 NtWow64ReadVirtualMemory64 = (_NtWow64ReadVirtualMemory64)GetProcAddress(
			GetModuleHandle(L"ntdll.dll"), "NtWow64ReadVirtualMemory64");
		status = NtWow64ReadVirtualMemory64(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL);

		RTL_USER_PROCESS_PARAMETERS64 param;
		NtWow64ReadVirtualMemory64(hProcess, (PVOID)peb.ProcessParameters, &param, sizeof(param), NULL);
		TCHAR szCommandLine[MAX_PATH] = { 0 };
		NtWow64ReadVirtualMemory64(hProcess, param.CommandLine.Buffer, szCommandLine, MAX_PATH, NULL);
		strParam = szCommandLine;
	}
	else
	{
		//32位程序获取32位程序peb或者64位程序获取64位程序peb
		_NtQueryInformationProcess NtQueryInformationProcess = (_NtQueryInformationProcess)GetProcAddress(
			GetModuleHandle(L"ntdll"), "NtQueryInformationProcess");
		PROCESS_BASIC_INFORMATION		pbi;
		PEB peb;
		RTL_USER_PROCESS_PARAMETERS param;
		status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
		ReadProcessMemory(hProcess, pbi.PebBaseAddress, &peb, sizeof(peb), NULL);
		ReadProcessMemory(hProcess, peb.ProcessParameters, &param, sizeof(param), NULL);

		TCHAR szCommandLine[MAX_PATH] = { 0 };
		ReadProcessMemory(hProcess, param.CommandLine.Buffer, szCommandLine, MAX_PATH, NULL);
		strParam = szCommandLine;
	}
	return strParam;
}

bool CProcessHelper::SetPrivilege(LPCTSTR lpstrPrivilege, BOOL bEnablePrivilege)
{
	bool bRes = false;
	pfnOpenProcessToken			pOpenProcessToken = NULL;
	pfnLookupPrivilegeValue		pLookupPrivilegeValue = NULL;
	pfnAdjustTokenPrivileges	pAdjustTokenPrivileges = NULL;

	HANDLE hToken = NULL;
	HANDLE hCurProcess = GetCurrentProcess();
	LUID luid;
	TOKEN_PRIVILEGES tokenPrivilegesNew;
	TOKEN_PRIVILEGES tokenPrivilgegesPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
	HMODULE hAdvAPI = GetModule(L"advapi32.dll");
	if (hAdvAPI == NULL)
	{
		return FALSE;
	}

	pOpenProcessToken = (pfnOpenProcessToken)GetProcAddress(hAdvAPI, "OpenProcessToken");
	if (pOpenProcessToken == NULL)
	{
		goto clean;
	}

	pLookupPrivilegeValue = (pfnLookupPrivilegeValue)GetProcAddress(hAdvAPI, "LookupPrivilegeValue");
	if (pLookupPrivilegeValue == NULL)
	{
		goto clean;
	}

	pAdjustTokenPrivileges = (pfnAdjustTokenPrivileges)GetProcAddress(hAdvAPI, "AdjustTokenPrivileges");
	if (pAdjustTokenPrivileges == NULL)
	{
		goto clean;
	}

	bRes = pOpenProcessToken(hCurProcess, TOKEN_WRITE | TOKEN_READ, &hToken);
	if (!bRes)
	{
		goto clean;
	}

	bRes = pLookupPrivilegeValue(NULL, lpstrPrivilege, &luid);
	if (!bRes)
	{
		goto clean;
	}

	tokenPrivilegesNew.PrivilegeCount = 1;
	tokenPrivilegesNew.Privileges[0].Luid = luid;
	tokenPrivilegesNew.Privileges[0].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
	bRes = pAdjustTokenPrivileges(hToken, FALSE, &tokenPrivilegesNew, sizeof(TOKEN_PRIVILEGES), &tokenPrivilgegesPrevious, &cbPrevious);
	if (!bRes)
	{
		goto clean;
	}
	tokenPrivilgegesPrevious.PrivilegeCount = 1;
	tokenPrivilgegesPrevious.Privileges[0].Luid = luid;
	if (bEnablePrivilege)
	{
		tokenPrivilgegesPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	}
	else
	{
		tokenPrivilgegesPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
			tokenPrivilgegesPrevious.Privileges[0].Attributes);
	}
	bRes = pAdjustTokenPrivileges(hToken, FALSE, &tokenPrivilgegesPrevious, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	if (!bRes)
	{
		goto clean;
	}
clean:
	if (hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}
	if (hCurProcess)
	{
		CloseHandle(hCurProcess);
		hCurProcess = NULL;
	}
	return bRes;
}

LPVOID CProcessHelper::GetProcessTokenInformation(DWORD dwProcessID, TOKEN_INFORMATION_CLASS TokenInformationClass)
{
	LPVOID TokenInformation = NULL;
	HANDLE hToken = NULL;
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
	HMODULE hAdvAPI = GetModule(L"advapi32.dll");

	pfnOpenProcessToken	 pOpenProcessToken = NULL;
	DWORD dwLength = 0;
	bool bRes = false;
	if (hAdvAPI == NULL)
	{
		goto clean;
	}
	
	pOpenProcessToken = (pfnOpenProcessToken)GetProcAddress(hAdvAPI, "OpenProcessToken");
	if (pOpenProcessToken == NULL)
	{
		goto clean;
	}
	if (!pOpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
	{
		goto clean;
	}
	
	bRes = GetTokenInformation(hToken, TokenInformationClass, NULL, dwLength, &dwLength);
	if (!bRes)
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			goto clean;
		}


		TokenInformation = (LPVOID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwLength);
		if (TokenInformation == NULL)
		{
			goto clean;
		}
	}
	bRes = GetTokenInformation(hToken, TokenInformationClass, TokenInformation, dwLength, &dwLength);
	if (!bRes)
	{
		HeapFree(GetProcessHeap(), 0, (LPVOID)TokenInformation);
		goto clean;
	}
clean:
	if (hToken)
	{
		CloseHandle(hToken);
		hToken = NULL;
	}
	if (hProcess)
	{
		CloseHandle(hProcess);
		hProcess = NULL;
	}
	return TokenInformation;
}

bool CProcessHelper::IsWow64(HANDLE hProcess)
{
	//32位exe运行在64位操作系统下，Wow64Process被设置为true
	//32位exe运行在32位操作系统下或64位exe运行在64位操作系统下 Wow64Process 被设置为false;
	//现在32位操作系统下kernel32中也包含有IsWow64Process函数，故不能以是否有IsWow64Process函数来判断是否是64位操作系统，这将是不可靠的
	bool bIsWow64 = false;
	typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		BOOL bRet = FALSE;
		fnIsWow64Process(hProcess, &bRet);
		bIsWow64 = bRet;
	}
	return bIsWow64;
}

bool CProcessHelper::Is64bitOS()
{
	bool bBit64 = false;
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		bBit64 = true;
	}
	return bBit64;
}
