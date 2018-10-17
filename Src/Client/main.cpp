
#include "stdafx.h"
#include "conductor.h"
#include "MainFrame.h"
#include "ConnectionClient.h"
#include "setdebugnew.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/refcount.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"

//共享节
#pragma data_seg("Shared")
HWND g_Wnd = 0;
#pragma data_seg()

#pragma comment(linker,"/section:Shared,rws")


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	::_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	::_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
#endif

// 	if (g_Wnd)
// 	{
// 		::ShowWindow(g_Wnd, SW_SHOW);
// 		return 0;
// 	}

	CPaintManagerUI::SetInstance(hInstance);
	CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath());

	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);


	MainFrame* pFrame = new MainFrame();
	if (pFrame == NULL) return 0;
#if defined(WIN32) && !defined(UNDER_CE)
	pFrame->Create(NULL, _T("launcher"), UI_WNDSTYLE_FRAME, 0, 0, 0, 0, 0, NULL);
#else
	pFrame->Create(NULL, _T("launcher"), UI_WNDSTYLE_FRAME, WS_EX_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
#endif
	pFrame->CenterWindow();
	::ShowWindow(*pFrame, SW_SHOW);

	//赋值给跨进程共享节数据
	g_Wnd = *pFrame;

	rtc::InitializeSSL();

	ConnectionClient client;
	Conductor conductor(&client, pFrame);

	CPaintManagerUI::MessageLoop();


	return 0;
}


