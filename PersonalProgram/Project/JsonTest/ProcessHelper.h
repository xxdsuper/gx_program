#pragma once
#include <string>
#include <vector>
#include <tchar.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <Winternl.h>
using namespace std;

typedef struct _PROCESS_INFO_ITEM
{
	DWORD	dwProcessId;            // ����ID
	wstring strProcessName;        // ��������
	wstring strProcessFile;        // �����ļ�
	DWORD	dwSessionId;			//��������session
	wstring	strUserName;			//�����û���
	wstring	strCmdParam;
}PROCESS_INFO_ITEM, *LPPROCESS_INFO_ITEM;

typedef BOOL(WINAPI *pfnOpenProcessToken)(
	HANDLE ProcessHandle,
	DWORD DesiredAccess,
	PHANDLE TokenHandle);

typedef BOOL(WINAPI *pfnLookupPrivilegeValue)(LPCWSTR lpSystemName,
	LPCWSTR lpName, PLUID  lpLuid);

typedef BOOL(WINAPI *pfnAdjustTokenPrivileges)(
	HANDLE TokenHandle,
	BOOL DisableAllPrivileges,
	PTOKEN_PRIVILEGES NewState,
	DWORD BufferLength,
	PTOKEN_PRIVILEGES PreviousState,
	PDWORD ReturnLength);

// NtQueryInformationProcess for pure 32 and 64-bit processes
typedef NTSTATUS(NTAPI *_NtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN	PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL);

//typedef NTSTATUS(NTAPI *_NtReadVirtualMemory)(
//	IN HANDLE ProcessHandle,
//	IN PVOID BaseAddress,
//	OUT PVOID Buffer,
//	IN SIZE_T Size,
//	OUT PSIZE_T NumberOfBytesRead);

// NtWow64QueryInformationProcess64 for 32-bit process get 64-bit process info
typedef NTSTATUS(WINAPI *_NtWow64QueryInformationProcess64)(
	IN       HANDLE ProcessHandle,
	IN       PROCESSINFOCLASS ProcessInformationClass,
	OUT      PVOID ProcessInformation,
	IN       ULONG ProcessInformationLength,
	OUT  PULONG ReturnLength);

typedef NTSTATUS(NTAPI *_NtWow64ReadVirtualMemory64)(
	IN HANDLE ProcessHandle,
	IN PVOID64 BaseAddress,
	OUT PVOID Buffer,
	IN ULONG64 Size,
	OUT PULONG64 NumberOfBytesRead);

// PROCESS_BASIC_INFORMATION for 32-bit process on WOW64
// The definition is quite funky, as we just lazily doubled sizes to match offsets...
typedef struct _PROCESS_BASIC_INFORMATION_WOW64 {
	PVOID64 Reserved1;
	PVOID64 PebBaseAddress;
	PVOID64 Reserved2[2];
	ULONG_PTR UniqueProcessId[2];
	PVOID64 Reserved3;
} PROCESS_BASIC_INFORMATION_WOW64;

typedef struct _UNICODE_STRING_WOW64 {
	USHORT Length;
	USHORT MaximumLength;
	PVOID64 Buffer;
} UNICODE_STRING_WOW64;

typedef struct _RTL_USER_PROCESS_PARAMETERS64 {
	BYTE Reserved1[16];
	PVOID64 Reserved2[10];
	UNICODE_STRING_WOW64 ImagePathName;
	UNICODE_STRING_WOW64 CommandLine;
} RTL_USER_PROCESS_PARAMETERS64, *PRTL_USER_PROCESS_PARAMETERS64;

typedef struct _PEB_LDR_DATA64 {
	BYTE Reserved1[8];
	PVOID64 Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA64, *PPEB_LDR_DATA64;

typedef struct _PEB64 {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID64 Reserved3[2];
	ULONG64 Ldr;
	ULONG64 ProcessParameters;
	BYTE Reserved4[104];
	PVOID64 Reserved5[52];
	ULONG64 PostProcessInitRoutine;
	BYTE Reserved6[128];
	PVOID64 Reserved7[1];
	ULONG SessionId;
} PEB64;


class CProcessHelper {
public:
	CProcessHelper(){}
	~CProcessHelper(){}
public:
	/**
	**ͨ������id��ȡ��������
	**@param[in] dwProcID Ҫ��ȡ���ƵĽ���id
	**return	��������
	**/
	wstring GetProcessNameByID(DWORD dwProcID);

	/**
	**ͨ���������ƻ�ȡ����id
	**@param[in] strProName Ҫ��ȡid�Ľ�������
	**return	-1��ʾ��ȡ����idʧ��
	**/
	DWORD GetProcessIDByName(wstring strProName);

	/**
	**ͨ������id��ȡ������id
	**@param[in] dwProcID Ҫ��ȡ���ƵĽ���id
	**return	-1��ʾ��ȡ����idʧ��
	**/
	DWORD GetParentProcessID(DWORD dwProcID);

	/**
	**ͨ������idɱ������
	**@param[in] dwProcID Ҫ�����Ľ���id
	**@param[out] dwErrCode ִ�н���������
	**return	ture��ʾ�������̳ɹ���false��ʾʧ��
	**/
	bool KillProcessByID(DWORD dwProcID, DWORD& dwErrCode);

	/**
	**ͨ����������ɱ������
	**@param[in] dwProcID Ҫ�����Ľ�������
	**@param[out] dwErrCode ִ�н���������
	**return	ture��ʾ�������̳ɹ���false��ʾʧ��
	**/
	bool KillProcessByName(wstring strProName, DWORD& dwErrCode);

	/**
	**ͨ�����������жϽ����Ƿ����
	**@param[in] strProName Ҫ��ȡid�Ľ�������
	**return	 ture��ʾ���ڣ�false��ʾ������
	**/
	bool IsProcessExist(const wstring strProName);

	/**
	** ��ȡָ�����Ƶ�ģ��
	** @param[in] szModule     ģ�������
	** @return                 ָ��ģ��ı�־
	**/
	HMODULE GetModule(const wchar_t* szModule);

	/**
	**��ȡ���н�����Ϣ
	**@param[out] vcProcessInfo	������Ϣ����
	**return					��������
	**/
	int	 GetProcessInfo(vector<PROCESS_INFO_ITEM>& vcProcessInfo);
	wstring GetProcessUserName(DWORD dwProcessID);
	wstring GetProcessCmdParam(DWORD dwProcessID);

	/**
	** ����Ȩ��(Ŀǰwin7ϵͳ��֧��Ȩ������)
	** @param[in] Privilege           Ȩ�޼������ƣ�������������(winnt.h�ļ���7812��)
	** @param[in] bEnablePrivilege    �Ƿ���������Ȩ��
	** return						  ture��ʾ�ɹ���false��ʾʧ��
	**/
	bool SetPrivilege(LPCTSTR lpstrPrivilege, BOOL bEnablePrivilege);

	LPVOID	GetProcessTokenInformation(DWORD dwProcessID, TOKEN_INFORMATION_CLASS TokenInformationClass);

	/**
	** ����Ȩ��(Ŀǰwin7ϵͳ��֧��Ȩ������)
	** @param[in] hProcess           ���������
	** return						 ture��ʾ������32λ����������64λ����ϵͳ�£�false ��ʾ32λ����������32λ����ϵͳ�»�64λ����������64λ����ϵͳ��
	** tips��64λ���򲻿���������32λ����ϵͳ��
	**/
	bool IsWow64(HANDLE hProcess);

	bool Is64bitOS();
};