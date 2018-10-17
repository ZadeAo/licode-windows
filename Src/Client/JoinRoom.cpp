#include "stdafx.h"
#include "JoinRoom.h"
#include "defaults.h"
#include "func.h"

JoinRoom::JoinRoom()
	: edit_server(0)
	, edit_room(0)
{
	
}

AString JoinRoom::getServer()
{
	return m_server;
}

AString JoinRoom::getRoomName()
{
	return m_room;
}

LPCTSTR JoinRoom::GetWindowClassName() const 
{ 
	return _T("JoinRoom");
}

void JoinRoom::OnFinalMessage(HWND hWnd)
{
	WindowImplBase::OnFinalMessage(hWnd);
}

void JoinRoom::Notify(TNotifyUI& msg)
{
	if (_tcsicmp(msg.sType, _T("click")) == 0)
	{
		if (_tcsicmp(msg.pSender->GetName(), _T("closebtn")) == 0)
		{
			Close(IDCANCEL);
		}
		else if (_tcsicmp(msg.pSender->GetName(), _T("btn_join")) == 0)
		{
			
			m_server = CW2A(edit_server->GetText());
			m_room = CW2A(edit_room->GetText());

			if (m_server.empty())
			{
				edit_server->SetFocus();
				::MessageBeep(0xFFFFFFFF);
				return;
			}
			if (m_room.empty())
			{
				edit_room->SetFocus();
				::MessageBeep(0xFFFFFFFF);
				return;
			}

			Close();
		}
	}
}

void JoinRoom::InitWindow()
{
	edit_server = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_server")));
	edit_room = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_room")));

	edit_server->SetText(CA2W(SERVER.c_str()));
	edit_room->SetText(CA2W(GenRoomName().c_str()));
}

CDuiString JoinRoom::GetSkinFile()
{
	return _T("JoinRoom.xml");
}

CDuiString JoinRoom::GetSkinFolder()
{
	return  _T("skin\\");
}

