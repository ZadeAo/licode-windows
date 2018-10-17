#ifndef __INTERFACE_H__
#define  __INTERFACE_H__
#pragma once

class cVideoDialog;
namespace webrtc {
	class VideoTrackInterface;
	class SessionDescriptionInterface;
	class MediaStreamInterface;
}  // namespace webrtc

class Renderer
{
public:
	virtual void RenderImage(const BITMAPINFO& bmi, const UInt8 *image) = 0;
};

class PCbserverCallBack
{
public:
	virtual void SendMessage(UInt32 id, const AString& json_object) = 0;
	virtual void SetLocalDescription(UInt32 id, webrtc::SessionDescriptionInterface* desc) = 0;
	virtual void QueueUIThreadCallback(int msg_id, void* data) = 0;
};

class ConnectionClientObserver
{
public:
	virtual HWND handle() = 0;
	
	virtual void OnSignedIn() = 0;
	virtual void OnLoginError() = 0;
	virtual void OnDisconnected() = 0;
	virtual void PacketBufferFull(){};
	virtual void PacketError(UInt32){};
	virtual void PacketUnknown(UInt32){};
	virtual void OnJoinedChannel(const AString &room) = 0;
	virtual void OnLeaveChannel(const AString &room) = 0;
	virtual void OnMessageSent(int err) = 0;
	virtual void OnMessageFromChannel(const AString &room, const UInt32 &a_FromID, const AString& a_Msg) = 0;
	virtual void OnServerConnectionFailure() = 0;

	virtual void PostNotification(CString msg, COLORREF color = RGB(0, 0, 0)) = 0;

protected:
	virtual ~ConnectionClientObserver() {}
};

class MainWndCallback 
{
public:
	virtual void JoinChannel(const AString &server, const AString &room) = 0;
	virtual void LeaveChannel(const AString &room) = 0;
	virtual void DisconnectFromServer() = 0;
	virtual void UIThreadCallback(int msg_id, void* data) = 0;
	virtual void Close() = 0;
	virtual LRESULT OnNetwork(WPARAM wParam, LPARAM lParam) = 0;
	virtual void SetLocalRender(Renderer *render) = 0;
	virtual void SetRemoteRender(UInt32 id, Renderer *render) = 0;
	
protected:
	virtual ~MainWndCallback() {}
};

class MainWindow 
{
public:
	virtual ~MainWindow() {}

	virtual HWND handle() = 0;

	virtual void RegisterObserver(MainWndCallback* callback) = 0;

	virtual bool IsWindow() = 0;
	virtual void MessageBox(const char* caption, const char* text,
		bool is_error) = 0;

	virtual void OnJoinChannel(const AString &room, UInt32 id) = 0;
	virtual void OnLeftChannel(const AString &room) = 0;
	virtual void OnUserJoinChannel(const AString &room,UInt32 id) = 0;
	virtual void OnUserLeftChannel(const AString &room, UInt32 id) = 0;

	virtual void QueueUIThreadCallback(int msg_id, void* data) = 0;
	virtual void PostNotification(CString msg, COLORREF color = RGB(0, 0, 0)) = 0;

	virtual void OnDisconnected() = 0;
	virtual void PacketBufferFull() = 0;
	virtual void PacketError(UInt32) = 0;
	virtual void PacketUnknown(UInt32) = 0;
	virtual void OnSignedIn() = 0;
};

struct RoomMsg
{
	UInt32 id;
	AString msg;
};

struct MediaMsg
{
	UInt32 id;
	webrtc::MediaStreamInterface* media;
};


#endif // !__INTERFACE_H__

