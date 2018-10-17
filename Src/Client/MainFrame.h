#ifndef MAINFRAME_HPP
#define MAINFRAME_HPP

#pragma once

#include "interface.h"
#include "TrayIcon.h"
#include "IniFile.h"
#include "VideoRenderer.h"


class cVideoDialog;

class MainFrame : public WindowImplBase, public MainWindow
{
public:
	MainFrame();
	~MainFrame();

public:
	virtual HWND handle();
	virtual void RegisterObserver(MainWndCallback* callback);
	virtual bool IsWindow();
	virtual void OnJoinChannel(const AString &room, UInt32 id);
	virtual void OnLeftChannel(const AString &room);
	virtual void OnUserJoinChannel(const AString &room, UInt32 id);
	virtual void OnUserLeftChannel(const AString &room, UInt32 id);

	virtual void MessageBox(const char* caption, const char* text,bool is_error);
	virtual void QueueUIThreadCallback(int msg_id, void* data);
	virtual void PostNotification(CString msg, COLORREF color = RGB(0, 0, 0));
public:
	LPCTSTR GetWindowClassName() const;
	virtual void InitWindow();
	virtual void OnFinalMessage(HWND hWnd);	
	virtual LRESULT ResponseDefaultKeyEvent(WPARAM wParam);
	virtual CDuiString GetSkinFile();
	virtual CDuiString GetSkinFolder();
	virtual UILIB_RESOURCETYPE GetResourceType() const;
	virtual CControlUI* CreateControl(LPCTSTR pstrClass);
	virtual LRESULT OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
 	virtual LRESULT OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
// 	virtual LRESULT OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
// 	virtual LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LRESULT HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	virtual LPCTSTR GetResourceID() const;

public:
	virtual void OnDisconnected();
	virtual void PacketBufferFull();
	virtual void PacketError(UInt32);
	virtual void PacketUnknown(UInt32);
	virtual void OnSignedIn();
protected:
	void Notify(TNotifyUI& msg);
	void OnExit(TNotifyUI& msg);
	void OnTimer(TNotifyUI& msg);

private:
	LRESULT OnTimer(WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnTrayMessage(WPARAM wParam, LPARAM lParam);
private://action
	void doJoinRoom();
	void doLeftRoom();
	void doAboutMe();
private:
	CTrayIcon	m_TrayIcon;

private://UI
	CButtonUI	*pBtnJoin;
private:
	std::map<UInt32, cVideoDialog*> m_Dlgs;
	DWORD ui_thread_id_;
	MainWndCallback *callback_;
	AString roomName_;
};

#endif // MAINFRAME_HPP

