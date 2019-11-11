
// add scheduled tasks for win7

#include "stdafx.h"
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")

bool AddSystemScheduledTasks(LPCTSTR lpstrTaskName, LPCTSTR lpstrPath, LPCTSTR lpstrArguments)
{
	HRESULT hr;
	CComPtr<ITaskService> spTaskService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&spTaskService);
	if (FAILED(hr))
	{
		return false;
	}

	hr = spTaskService->Connect(CComVariant(), CComVariant(), CComVariant(), CComVariant());
	if (FAILED(hr))
	{
		return false;
	}

	//get task root folder
	CComPtr<ITaskFolder> spRootFolder = NULL;
	hr = spTaskService->GetFolder(CComBSTR(L"\\"), &spRootFolder);
	if (FAILED(hr))
	{
		return false;
	}

	//delete old task
	spRootFolder->DeleteTask(CComBSTR(lpstrTaskName), 0);

	//create new task
	CComPtr<ITaskDefinition> spTaskDefinition = NULL;
	hr = spTaskService->NewTask(0, &spTaskDefinition);
	if (FAILED(hr))
	{
		return false;
	}

	//设置任务安全配置
	CComPtr<IPrincipal> spPrincipal = NULL;
	hr = spTaskDefinition->get_Principal(&spPrincipal);
	if (FAILED(hr))
	{
		return false;
	}
	spPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

	//设置任务动作
	CComPtr<IActionCollection> spActionCollection = NULL;
	CComPtr<IAction> spAction = NULL;
	IExecAction* pExecAction = NULL;
	hr = spTaskDefinition->get_Actions(&spActionCollection);
	if (FAILED(hr))
	{
		return false;
	}
	hr = spActionCollection->Create(TASK_ACTION_EXEC, &spAction);
	if (FAILED(hr))
	{
		return false;
	}
	hr = spAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
	if (FAILED(hr))
	{
		return false;
	}
	pExecAction->put_Path(CComBSTR(lpstrPath));
	pExecAction->put_Arguments(CComBSTR(lpstrArguments));

	//设置创建者信息
	CComPtr<IRegistrationInfo> spRegInfo = NULL;
	hr = spTaskDefinition->get_RegistrationInfo(&spRegInfo);
	if (NULL != spRegInfo)
	{
		spRegInfo->put_Description(CComBSTR());
	}

	//创建计划任务触发器
	CComPtr<ITriggerCollection> spTriggerCollection = NULL;
	CComPtr<ITrigger> spTrigger = NULL;
	spTaskDefinition->get_Triggers(&spTriggerCollection);
	////创建并设置登录触发器
	CComPtr<ILogonTrigger> spLogonTrigger = NULL;
	spTriggerCollection->Create(TASK_TRIGGER_LOGON, &spTrigger);
	spTrigger->QueryInterface(IID_ILogonTrigger, (void**)&spLogonTrigger);
	//spLogonTrigger->put_Delay(CComBSTR());
	//
	//创建并设置日触发器
	CComPtr<IDailyTrigger> spDailyTrigger = NULL;
	spTrigger.Release();
	spTriggerCollection->Create(TASK_TRIGGER_DAILY, &spTrigger);
	spTrigger->QueryInterface(IID_IDailyTrigger, (void**)&spDailyTrigger);

	//save task scheduler
	CComPtr<IRegisteredTask> spRegisteredTask = NULL;
	hr = spRootFolder->RegisterTaskDefinition(CComBSTR(lpstrTaskName), spTaskDefinition, TASK_CREATE_OR_UPDATE, CComVariant(),
		CComVariant(), TASK_LOGON_INTERACTIVE_TOKEN, CComVariant(), &spRegisteredTask);
	if (FAILED(hr))
	{
		return false;
	}
	return true;


}

bool DeleteSystemScheduledTasks(LPCTSTR lpstrTaskName)
{
	HRESULT hr;
	CComPtr<ITaskService> spTaskService = NULL;
	hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&spTaskService);
	if (FAILED(hr))
	{
		return false;
	}

	hr = spTaskService->Connect(CComVariant(), CComVariant(), CComVariant(), CComVariant());
	if (FAILED(hr))
	{
		return false;
	}

	//get task root folder
	CComPtr<ITaskFolder> spRootFolder = NULL;
	hr = spTaskService->GetFolder(CComBSTR(L"\\"), &spRootFolder);
	if (FAILED(hr))
	{
		return false;
	}

	//delete old task
	hr = spRootFolder->DeleteTask(CComBSTR(lpstrTaskName), 0);
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}