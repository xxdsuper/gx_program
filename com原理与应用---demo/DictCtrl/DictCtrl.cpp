// DictCtrl.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include <stdio.h>
#include <comutil.h>

#include "IDictionary.h"
#include "ISpellCheck.h"

// {15EE4167-930E-4D7F-84F0-3A42CDE3E839}
extern "C" const GUID CLSID_Dictionary =
{ 0x15EE4167, 0x930E, 0x4D7F,
{ 0x84, 0xF0, 0x3A, 0x42, 0xCD, 0xE3, 0xE8, 0x39} };

// {5352937A-918C-40B5-B6F6-72471BF3EFC6}
extern "C" const GUID IID_Dictionary =
{ 0x5352937A, 0x918C, 0x40B5,
{ 0xB6, 0xF6, 0x72, 0x47, 0x1B, 0xF3, 0xEF, 0xC6} };

// {FBBC716F-FD34-4082-8918-063248016471}
extern "C" const GUID IID_SpellCheck =
{ 0xFBBC716F, 0xFD34, 0x4082,
{ 0x89, 0x18, 0x06, 0x32, 0x48, 0x01, 0x64, 0x71} };


int main(int argc, char* argv[])
{
	IUnknown *pUnknown;
	IDictionary *pDictionary;
	ISpellCheck *pSpellCheck;
	String stringResult;
	BOOL bResult;
	HRESULT hResult;

	if (CoInitialize(NULL) != S_OK) {
		printf("Initialize COM library failed!\n");
		return -1;
	}

	GUID dictionaryCLSID;
	hResult = ::CLSIDFromProgID(L"Dictionary.Object", &dictionaryCLSID);
	if (hResult != S_OK) 
	{
		printf("Can't find the dictionary CLSID!\n");
		return -2;
	}
	
	hResult = CoCreateInstance(dictionaryCLSID, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&pUnknown);
	if (hResult != S_OK) 
	{
		printf("Create object failed!\n");
		return -2;
	}

	hResult = pUnknown->QueryInterface(IID_Dictionary, (void **)&pDictionary);
	if (hResult != S_OK) {
		pUnknown->Release();
		printf("QueryInterface IDictionary failed!\n");
		return -3;
	}
	bResult = pDictionary->LoadLibrary(L"animal.dict");
	if (bResult) {
		String stringResult;
		bResult = pDictionary->LookupWord(L"tiger", &stringResult);
		
		if (bResult) {
			char *pTiger = _com_util::ConvertBSTRToString(stringResult);
			printf("find the word \"tiger\" -- %s\n", pTiger);
			delete pTiger;
		}

		pDictionary->InsertWord(L"elephant", L"Ïó");
		bResult = pDictionary->LookupWord(L"elephant", &stringResult);
		if (bResult) {

			pDictionary->RestoreLibrary(L"animal1.dict");
		}
	} else {
		printf("Load Library \"animal.dict\"\n");
	}
	
	hResult = pDictionary->QueryInterface(IID_SpellCheck, (void **)&pSpellCheck);
	pDictionary->Release();
	if (hResult != S_OK) {
		pUnknown->Release();
		printf("QueryInterface IDictionary failed!\n");
		return -4;
	}

	bResult = pSpellCheck->CheckWord(L"lion", &stringResult);
	if (bResult) {
		printf("Word \"lion\" spelling right.\n");
	} else {
		char *pLion = _com_util::ConvertBSTRToString(stringResult);
		printf("Word \"lion\" spelling is wrong. Maybe it is %s.\n", pLion);
		delete pLion;
	}
	bResult = pSpellCheck->CheckWord(L"dot", &stringResult);
	if (bResult) {
		printf("Word \"dot\" spelling right.\n");
	} else {
		char *pDot = _com_util::ConvertBSTRToString(stringResult);
		printf("Word \"dot\" spelling is wrong. Maybe it is %s.\n", pDot);
		delete pDot;
	}

	pSpellCheck->Release();
	if (pUnknown->Release()== 0) 
		printf("The reference count of dictionary object is zero.");

	CoUninitialize();
	system("pause");
	return 0;
}
