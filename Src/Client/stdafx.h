
// Globals.h

// This file gets included from every module in the project, so that global symbols may be introduced easily
// Also used for precompiled header generation in MSVC environments


#pragma once
#include <Globals.h>
#include <Common.h>

#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#include <objbase.h>
#include <atlstr.h>
#include <atltypes.h>

#define			WM_TRAYICON_NOTIFY		WM_USER + 1		// 系统托盘图标通知消息
#define			WM_NETWORK				WM_USER + 2		// 网络相关的消息
#define			WM_UPDATEIMG			WM_USER + 4		// 刷新相关的消息
#define			UI_THREAD_CALLBACK		WM_USER + 5

#include "UIlib.h"

#ifndef NO_USING_DUILIB_NAMESPACE
using namespace DuiLib;
#endif
 
#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif
