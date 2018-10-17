
#include "stdafx.h"
#include "func.h"
#include <random>
#include "setdebugnew.h"

//逐个比较字符
bool isAllDigit(const string& str)
{
	int i;
	for (i = 0; i != str.length(); i++)
	{
		if (!isdigit(str[i]))
		{
			return false;
		}
	}
	return true;
}


// 判断操作系统类型是否是64位
BOOL OSVerIs64Bit()
{
	BOOL b64 = FALSE;
	HANDLE h = GetCurrentProcess();
	IsWow64Process(h, &b64);
	return b64;
}


AString UTF8ToGBK(const AString& strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	WCHAR* wszGBK = new WCHAR[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);

	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char *szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	AString strTemp(szGBK);
	delete[]szGBK;
	delete[]wszGBK;
	return strTemp;
}


AString GBKToUTF8(const AString& strGBK)
{
	string strOutUTF8 = "";
	WCHAR * str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char * str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
}

AString& replace_all(AString& str, const AString& old_value, const AString& new_value)
{
	while (true)   {
		string::size_type   pos(0);
		if ((pos = str.find(old_value)) != string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return   str;
}
AString& replace_all_distinct(AString& str, const AString& old_value, const AString& new_value)
{
	for (string::size_type pos(0); pos != string::npos; pos += new_value.length())   {
		if ((pos = str.find(old_value, pos)) != string::npos)
			str.replace(pos, old_value.length(), new_value);
		else   break;
	}
	return   str;
}



AString CreateUUID()
{
	GUID guid;
	if (CoCreateGuid(&guid))
	{
		LOGERROR("create guid error\n");
		return "";
	}
	return Printf("%08X-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
}


 AString GenRoomName(const int &size)
{
	AString allChar = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	std::random_device rd;
	std::default_random_engine e(rd());
	std::uniform_int_distribution<> u(0, allChar.length()-1);

	AString name;

	for (size_t i = 0; i < size; i++)
	{
		name += allChar[u(e)];
	}
	
	return name;
}