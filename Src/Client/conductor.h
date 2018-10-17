
#ifndef PEERCONNECTION_SAMPLES_CLIENT_CONDUCTOR_H_
#define PEERCONNECTION_SAMPLES_CLIENT_CONDUCTOR_H_
#pragma once

#include "interface.h"
#include "ConnectionClient.h"

#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "webrtc/base/scoped_ptr.h"

class MainWindow;
class PCObserver;

namespace webrtc {
	class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
	class VideoRenderer;
}  // namespace cricket

enum CallbackID {
	MEDIA_CHANNELS_INITIALIZED = 1,
	PEER_CONNECTION_CLOSED,
	SEND_MESSAGE_TO_PEER,
	PEER_CONNECTION_ERROR,
	NEW_STREAM_ADDED,
	STREAM_REMOVED,
};


class Conductor
	: public ConnectionClientObserver,
	public PCbserverCallBack,
	public MainWndCallback {
public:

	Conductor(ConnectionClient* client, MainWindow* main_wnd);
	~Conductor();
	virtual void Close();

	//PCbserverCallBack
	// Send a message to the remote peer.
	virtual void SendMessage(UInt32 id,const AString& json_object);
	virtual void QueueUIThreadCallback(int msg_id, void* data);
	virtual void SetLocalDescription(UInt32 id, webrtc::SessionDescriptionInterface* desc);
protected:
	
	bool Initialize();
	bool CreatePeerConnection(UInt32 id,bool dtls);
	void DeletePeerConnection(UInt32 id);
	void EnsureStreamingUI();
	void AddStreams();


	cricket::VideoCapturer* OpenVideoCaptureDevice();
	cricket::VideoCapturer* OpenDesktopCaptureDevice();

	//
	// ConnectionClientObserver implementation.
	//

	virtual HWND handle();

	virtual void OnSignedIn();

	virtual void OnLoginError();

	virtual void OnDisconnected();

	virtual void OnMessageSent(int err);

	virtual void OnJoinedChannel(const AString &room);

	virtual void OnLeaveChannel(const AString &room);

	virtual void OnServerConnectionFailure();

	virtual void OnMessageFromChannel(const AString &room, const UInt32 &a_FromID, const AString& a_Msg);

	virtual void PostNotification(CString msg, COLORREF color = RGB(0, 0, 0));
	//
	// MainWndCallback implementation.
	//

	virtual void JoinChannel(const AString &server, const AString &room);

	virtual void LeaveChannel(const AString &room);

	virtual void DisconnectFromServer();

	virtual void UIThreadCallback(int msg_id, void* data);

	virtual LRESULT OnNetwork(WPARAM wParam, LPARAM lParam);

	virtual void SetLocalRender(Renderer *render);

	virtual void SetRemoteRender(UInt32 id, Renderer *render);


protected:

	ConnectionClient* client_;
	MainWindow* main_wnd_;
	AString server_;

	std::map < UInt32, rtc::scoped_refptr<webrtc::PeerConnectionInterface> > peer_connections_;
	std::map < UInt32, rtc::scoped_refptr<PCObserver> > pcObservers_;
	rtc::scoped_refptr < webrtc::PeerConnectionFactoryInterface > peer_connection_factory_;
	rtc::scoped_refptr<webrtc::MediaStreamInterface> active_stream_;
};

#endif  // PEERCONNECTION_SAMPLES_CLIENT_CONDUCTOR_H_
