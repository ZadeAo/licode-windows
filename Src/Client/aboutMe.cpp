#include "stdafx.h"
#include "aboutMe.h"
#include "defaults.h"


AboutMe::AboutMe()
{
	
}

LPCTSTR AboutMe::GetWindowClassName() const 
{ 
	return _T("AboutMe");
}

void AboutMe::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);
	delete this;
}

void AboutMe::Notify(TNotifyUI& msg)
{
	if (_tcsicmp(msg.sType, _T("click")) == 0)
	{
		if (_tcsicmp(msg.pSender->GetName(), _T("closebtn")) == 0)
		{
			Close(IDCANCEL);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("btn_web")) == 0)
		{
			::ShellExecute(*this, _T("open"), WEBURL, NULL, NULL, SW_SHOW);
		}
	}
}

void AboutMe::InitWindow()
{

}

CDuiString AboutMe::GetSkinFile()
{
	return _T("aboutMe.xml");
}

CDuiString AboutMe::GetSkinFolder()
{
	return  _T("skin\\");
}
