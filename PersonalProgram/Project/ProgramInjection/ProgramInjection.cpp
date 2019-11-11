// ProgramInjection.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <Windows.h>
using namespace std;
//线程代码注入
//声明所需函数
typedef HMODULE(WINAPI *lpLoadLibraryA)(char* filename);
typedef FARPROC(WINAPI *lpGetProcAddress)(HMODULE hModule, char* funcName);
typedef int(WINAPI *lpMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

typedef struct _thread_param {
	lpLoadLibraryA loadFunc;
	lpGetProcAddress GetPFunc;
	char data[4][MAX_PATH]; //保存所有参数

}thread_param;

DWORD WINAPI thread_proc(LPVOID lpVoid)
{
	thread_param* tParam = (thread_param*)lpVoid;
	HMODULE hModule = tParam->loadFunc(tParam->data[0]);
	lpMessageBoxA msgbox = (lpMessageBoxA)tParam->GetPFunc(hModule, tParam->data[1]);
	msgbox(NULL, tParam->data[2], tParam->data[2], MB_OK);
	return 0;
}

DWORD codeInject(DWORD dwPID)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwPID);
	if (INVALID_HANDLE_VALUE == hProcess || hProcess == 0)
	{
		cout << "open tag process error:" << GetLastError() << endl;
		return -1;
	}
	thread_param param;
	memset(&param, 0, sizeof(param));
	param.loadFunc = (lpLoadLibraryA)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
	param.GetPFunc = (lpGetProcAddress)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetProcAddress");
	memcpy(&param.data[0], "user32.dll", 11);
	memcpy(&param.data[1], "MessageBoxA", 12);
	memcpy(&param.data[2], "freesec", 8);
	memcpy(&param.data[3], "inject", 7);
	
	//
	DWORD dwWrite = 0;
	
	LPVOID lpDataBase = VirtualAllocEx(hProcess, 0, sizeof(thread_param), MEM_COMMIT, PAGE_READWRITE);
	if (lpDataBase == 0)
	{
		CloseHandle(hProcess);
		cout << "VirtualAllocEx DataBase Error:" << GetLastError() << endl;
		return -1;
	}
	//把线程参数写入目标进程的地址空间
	WriteProcessMemory(hProcess, lpDataBase, &param, sizeof(thread_param), &dwWrite);

	DWORD dwCodeSize = (DWORD)codeInject - (DWORD)thread_proc;//计算纯程函数的代码大小
	LPVOID lpCodeBase = VirtualAllocEx(hProcess, 0, dwCodeSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (lpCodeBase == 0)
	{
		VirtualFreeEx(hProcess, lpDataBase, sizeof(thread_param), MEM_FREE);
		CloseHandle(hProcess);
		return -1;
	}
	WriteProcessMemory(hProcess, lpCodeBase, thread_proc, dwCodeSize, &dwWrite);
	DWORD dwThreadId;
	HANDLE hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)lpCodeBase, lpDataBase, 0, &dwThreadId);
	if (hThread != NULL)
	{
		cout << "CreateRemoteThread Success :" << dwThreadId << endl;
		CloseHandle(hThread);
	}
	VirtualFreeEx(hProcess, lpDataBase, sizeof(thread_param), MEM_FREE);
	VirtualFreeEx(hProcess, lpCodeBase, dwCodeSize, MEM_FREE);
	CloseHandle(hProcess);

	return 0;
}
int main(int argc, char** argv)
{

    std::cout << "Hello World!\n"; 
	cout << "please input process id:";
	DWORD dwPID = 0;
	cin >> dwPID;
	codeInject(dwPID);
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
