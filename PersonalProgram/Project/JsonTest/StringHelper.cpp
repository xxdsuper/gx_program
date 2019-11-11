#include "StringHelper.h"

wstring UTF8_To_Unicode(const string& utf8_str)
{
	int nLen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
	if (nLen <= 0)
	{
		return wstring();
	}
	wchar_t* szRes = new wchar_t[(nLen+1) * sizeof(wchar_t)];
	memset(szRes, 0, nLen+1);
	MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, szRes, nLen);
	wstring strRtn(szRes);
	delete[] szRes;
	return strRtn;
}

string Unicode_To_UTF8(const wstring& unicode_wstr)
{
	int nLen = WideCharToMultiByte(CP_UTF8, 0, unicode_wstr.c_str(), -1, NULL, 0, NULL, NULL);
	if (nLen <= 0)
	{
		return string();
	}

	char* szRes = new char[(nLen + 1) * sizeof(char)];
	memset(szRes, 0, (nLen + 1));
	WideCharToMultiByte(CP_UTF8, 0, unicode_wstr.c_str(), -1, szRes, nLen, NULL, NULL);
	string strRtn(szRes);
	delete[] szRes;
	return strRtn;
}

wstring Ascii_To_Unicode(const string & ascii_str)
{
	int nLen = MultiByteToWideChar(CP_ACP, 0, ascii_str.c_str(), -1, NULL, 0);
	if (nLen <= 0)
	{
		return wstring();
	}
	wchar_t* szRes = new wchar_t[(nLen + 1) * sizeof(wchar_t)];
	memset(szRes, 0, (nLen + 1));
	MultiByteToWideChar(CP_ACP, 0, ascii_str.c_str(), -1, szRes, nLen);
	wstring strRtn(szRes);
	delete[] szRes;
	return strRtn;
}


string Unicode_To_Ascii(const wstring & unicode_wstr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, unicode_wstr.c_str(), -1, NULL, 0, NULL, NULL);
	if (nLen <= 0)
	{
		return string();
	}

	char* szRes = new char[(nLen + 1) * sizeof(char)];
	memset(szRes, 0, (nLen + 1));
	WideCharToMultiByte(CP_ACP, 0, unicode_wstr.c_str(), -1, szRes, nLen, NULL, NULL);
	string strRtn(szRes);
	delete[] szRes;
	return strRtn;
}

string Ascii_To_UTF8(const string & ascii_str)
{
	//先将ascii编码转unicode编码
	wstring strUnicode = Ascii_To_Unicode(ascii_str);
	//再将unicode编码转为utf8编码
	return Unicode_To_UTF8(strUnicode);
}

string UTF8_To_Ascii(const string & utf8_str)
{
	//先将utf8编码转unicode编码
	wstring strUnicode = Ascii_To_Unicode(utf8_str);
	//再将unicode编码转为ascii编码
	return Unicode_To_Ascii(strUnicode);
}

wstring UnicodeEncode(const wstring & unicode_wstr, bool bOnlyCN /*= false*/)
{
	const wchar_t *p = unicode_wstr.c_str();
	UACODE ua;
	wstring sResult(unicode_wstr.length() * 6 + 1, '\0');
	wchar_t*  pa = &sResult[0];
	for (wstring::const_iterator it = unicode_wstr.begin(); it != unicode_wstr.end(); it++)
	{
		ua.ch = *it;
		if (bOnlyCN)
		{
			if (ua.HighByte)
			{
				swprintf_s(pa, 7, L"\\u%02x%02x", ua.HighByte, ua.LowByte);
				pa += 6;
			}
			else {
				*pa = ua.LowByte;
				pa++;
			}
		}
		else
		{
			swprintf_s(pa, 7, L"\\u%02x%02x", ua.HighByte, ua.LowByte);
			pa += 6;
		}
	}
	return sResult.c_str();
}

wstring UnicodeDecode(const wstring & unicode_wstr)
{
	return wstring();
}

string GBK_TO_UTF8(const string & gbk_str)
{
	wstring wstrGBK = Ascii_To_Unicode(gbk_str);
	return Unicode_To_UTF8(wstrGBK);
}
