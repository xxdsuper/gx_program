#pragma once
#include <windows.h>
#include <string.h>
#include <tchar.h>
#include <vector>

using namespace std;
typedef union _uacode
{
	struct {
		BYTE LowByte;
		BYTE HighByte;
	}DUMMYSTRUCTNAME;
	struct {
		BYTE LowByte;
		BYTE HighByte;
	} u;
	wchar_t ch;
}UACODE;
//utf8��unicode֮���໥ת��
wstring UTF8_To_Unicode(const string& utf8_str);
string Unicode_To_UTF8(const wstring& unicode_wstr);

//���ֽں�խ�ֽ��໥ת��
wstring Ascii_To_Unicode(const string& ascii_str);
string Unicode_To_Ascii(const wstring& unicode_wstr);

//utf8��խ�ֽ��໥ת��
string Ascii_To_UTF8(const string& ascii_str);
string UTF8_To_Ascii(const string& utf8_str);

//unicode encode/decode
wstring UnicodeEncode(const wstring& unicode_wstr, bool bOnlyCN = false);
wstring UnicodeDecode(const wstring& unicode_wstr);

//
string GBK_TO_UTF8(const string& gbk_str);