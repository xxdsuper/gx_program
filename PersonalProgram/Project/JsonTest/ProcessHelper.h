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
	DWORD	dwProcessId;            // 进程ID
	wstring strProcessName;        // 进程名字
	wstring strProcessFile;        // 进程文件
	DWORD	dwSessionId;			//进程所属session
	wstring	strUserName;			//进程用户名
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
	**通过进程id获取进程名称
	**@param[in] dwProcID 要获取名称的进程id
	**return	进程名称
	**/
	wstring GetProcessNameByID(DWORD dwProcID);

	/**
	**通过进程名称获取进程id
	**@param[in] strProName 要获取id的进程名字
	**return	-1表示获取进程id失败
	**/
	DWORD GetProcessIDByName(wstring strProName);

	/**
	**通过进程id获取父进程id
	**@param[in] dwProcID 要获取名称的进程id
	**return	-1表示获取进程id失败
	**/
	DWORD GetParentProcessID(DWORD dwProcID);

	/**
	**通过进程id杀掉进程
	**@param[in] dwProcID 要结束的进程id
	**@param[out] dwErrCode 执行结果错误代码
	**return	ture表示结束进程成功，false表示失败
	**/
	bool KillProcessByID(DWORD dwProcID, DWORD& dwErrCode);

	/**
	**通过进程名称杀掉进程
	**@param[in] dwProcID 要结束的进程名称
	**@param[out] dwErrCode 执行结果错误代码
	**return	ture表示结束进程成功，false表示失败
	**/
	bool KillProcessByName(wstring strProName, DWORD& dwErrCode);

	/**
	**通过进程名称判断进程是否存在
	**@param[in] strProName 要获取id的进程名字
	**return	 ture表示存在，false表示不存在
	**/
	bool IsProcessExist(const wstring strProName);

	/**
	** 获取指定名称的模块
	** @param[in] szModule     模块的名称
	** @return                 指定模块的标志
	**/
	HMODULE GetModule(const wchar_t* szModule);

	/**
	**获取所有进程信息
	**@param[out] vcProcessInfo	进程信息数组
	**return					进程数量
	**/
	int	 GetProcessInfo(vector<PROCESS_INFO_ITEM>& vcProcessInfo);
	wstring GetProcessUserName(DWORD dwProcessID);
	wstring GetProcessCmdParam(DWORD dwProcessID);

	/**
	** 设置权限(目前win7系统不支持权限设置)
	** @param[in] Privilege           权限级别名称，这个名称详情见(winnt.h文件的7812行)
	** @param[in] bEnablePrivilege    是否启用特殊权限
	** return						  ture表示成功，false表示失败
	**/
	bool SetPrivilege(LPCTSTR lpstrPrivilege, BOOL bEnablePrivilege);

	LPVOID	GetProcessTokenInformation(DWORD dwProcessID, TOKEN_INFORMATION_CLASS TokenInformationClass);

	/**
	** 设置权限(目前win7系统不支持权限设置)
	** @param[in] hProcess           程序的名柄
	** return						 ture表示程序是32位程序，运行在64位操作系统下，false 表示32位程序运行在32位操作系统下或64位程序运行在64位操作系统下
	** tips：64位程序不可能运行在32位操作系统下
	**/
	bool IsWow64(HANDLE hProcess);

	bool Is64bitOS();
};