// JsonTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "JsonHelper.h"
#include "StringHelper.h"
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")


char valueToChar(const int nValue)
{
	char result = '\0';
	if (nValue >= 0 && nValue <= 9) {
		result = (char)(nValue + 48); //48为ascii编码的‘0’字符编码值
	}
	else if (nValue >= 10 && nValue <= 15) {
		result = (char)(nValue - 10 + 65); //减去10则找出其在16进制的偏移量，65为ascii的'A'的字符编码值
	}
	return result;
}

void ByteToHexStr(const unsigned char* pbSrc, char* pszDesStr, int nLen)
{
	char chl, chh;
	for (int i = 0; i < nLen; i++)
	{
		chh = 48 + pbSrc[i] / 16;
		chl = 48 + pbSrc[i] % 16;
		if (chh > 57) chh = chh + 7;
		if (chl > 57) chl = chl + 7;
		pszDesStr[i * 2] = chh;
		pszDesStr[i * 2 + 1] = chl;
	}
	pszDesStr[nLen * 2] = '\0';
}


int StrToHexStr(string strRes, string& strHexString)
{
	int nHeigh, nLow;
	if (strRes.compare("") == 0)
	{
		return -1;
	}
	strHexString = "";
	for (UINT i = 0; i < strRes.length(); i++)
	{
		int nTemp = (int)strRes[i];
		nHeigh = nTemp >> 4;
		nLow = nTemp % 16;
		strHexString += valueToChar(nHeigh);
		strHexString += valueToChar(nLow);
	}
	return 0;
}

string Unicode2AnsiCode(wstring str)
{
	const wchar_t *p = str.c_str();
	UACODE ua;
	string sResult(str.length() * 6 + 1, '\0');
	char*  pa = &sResult[0];
	for (wstring::const_iterator it = str.begin(); it != str.end(); it++)
	{
		ua.ch = *it;
		/*if (ua.HighByte)
		{
			sprintf_s(pa, 7, "\\u%02x%02x", ua.HighByte, ua.LowByte);
			pa += 6;
		}
		else {
			*pa = ua.LowByte;
			pa++;
		}*/
		sprintf_s(pa, 7, "\\u%02x%02x", ua.HighByte, ua.LowByte);
		pa += 6;

	}
	return sResult.c_str();
}

wstring GetUStr(const string & str)
{
	std::string showname = str;//\u6211\u7231\u5317\u4eac\u5929\u5b89\u95e8
	int len = strlen(showname.c_str()) + 1;
	WCHAR * wChar = new WCHAR[len];
	wmemset(wChar, 0, len);
	MultiByteToWideChar(CP_UTF8, 0, showname.c_str(), len, wChar, len);
	wstring strRet = wChar;//转化结果 我爱北京天安门
	delete[] wChar;
	return strRet;
}
wchar_t* CharToWChar(const char* szChar)
{
	int nLen = MultiByteToWideChar(CP_UTF8, 0, szChar, -1, NULL, 0);
	wchar_t* chRtn = new wchar_t[nLen + 1];
	memset(chRtn, 0, nLen + 1);

	MultiByteToWideChar(CP_UTF8, 0, szChar, -1, chRtn, nLen);
	chRtn[nLen] = '\0';
	return chRtn;
}
bool ChineseToUnicode(wstring cstr, string & str)
{
	int i = 0;
	int strlen = 0;
	int hexlen = 0;
	long hexlong = 0;

	strlen = cstr.length();
	if (strlen <= 0)
	{
		return false;
	}

	wchar_t* wchs = new wchar_t[strlen + 1];
	memset(wchs, 0, sizeof(wchar_t) * (strlen + 1));
	wcscpy_s(wchs, strlen + 1, cstr.c_str());

	hexlen = strlen * 7;
	char* hexstr = new char[hexlen + 1];
	memset(hexstr, 0, hexlen + 1);

	char tchar[7];
	wchar_t* szHex = wchs;

	for (i = 0; i < strlen; i++)
	{
		hexlong = (long)(*szHex++);
		sprintf_s(tchar, "\\u%04x", hexlong);
		strcat_s(hexstr, hexlen, tchar);
	}

	str = (string)hexstr;

	if (wchs)
	{
		delete[] wchs;
	}
	if (hexstr)
	{
		delete[] hexstr;
	}
	return true;
}

bool UnicodeToChinese(string str, wstring&  cstr)
{
	int i = 0;
	int j = 0;
	int len = 0;

	len = str.length();
	if (len <= 0)
	{
		return false;
	}

	int nValue = 0;
	WCHAR * pWchar;
	wchar_t* szHex;
	char strchar[6] = { '0','x','\0' };
	for (i = 0; i < len; i++)
	{
		if (str[i] == 'u')
		{
			for (j = 2; j < 6; j++)
			{
				i++;
				strchar[j] = str[i];
			}
			szHex = CharToWChar(strchar);

			StrToIntExW(szHex, STIF_SUPPORT_HEX, &nValue);
			pWchar = (WCHAR*)& nValue;

			cstr = cstr + pWchar;
		}
	}
	return true;
}

int main()
{
	wstring wstr = L"{\"version\":\"1.0.0.12\",\"url\":\"http://browser.1314.com/download/FFBrowser_Setup_1.0.0.12.exe\",\"description\":\"1\u3001\u4f18\u5316\u6807\u7b7e\u5185\u5bb92\u3001\u589e\u52a0\u9f20\u6807\u624b\u52bf\u529f\u80fd3\u3001\u6d4b\u8bd5\u66f4\u65b0\",\"force_update\":0}";
	
	GenericDocument<UTF16<>> dom;
	dom.Parse(wstr.c_str());
	if (dom.HasParseError() || !dom.IsObject())
	{
		cout << "json error!" << endl;
	}
	wstring strDes = L"";
	if (dom.HasMember(L"description"))
	{
		GenericValue<UTF16<> > &jValue = dom[L"description"];
		strDes = jValue.GetString();
		MessageBox(NULL, strDes.c_str(), L"wstring", MB_OK);
	}

	return 0;
}

