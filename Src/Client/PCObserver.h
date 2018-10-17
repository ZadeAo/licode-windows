

#pragma once

#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "webrtc/base/scoped_ptr.h"
#include "VideoRenderer.h"

class Renderer;
class Conductor;
class PCbserverCallBack;

class  PCObserver
	: public webrtc::PeerConnectionObserver
	, public webrtc::CreateSessionDescriptionObserver
{
public:
	PCObserver(UInt32 id, PCbserverCallBack *callback,bool bSelf = false);
	virtual ~PCObserver();

	//
	// PeerConnectionObserver implementation.
	//
	virtual void OnError();
	virtual void OnStateChange(
		webrtc::PeerConnectionObserver::StateType state_changed) {}
	virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
	virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
	virtual void OnDataChannel(webrtc::DataChannelInterface* channel) {}
	virtual void OnRenegotiationNeeded() {}
	virtual void OnIceChange() {}
	virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

	// CreateSessionDescriptionObserver implementation.
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
	virtual void OnFailure(const AString& error);

	virtual void RegisterRenderer(Renderer *renderer);
	virtual void StartRenderer(webrtc::VideoTrackInterface* local_video);
	virtual void StopRenderer();

private:

	void SendMessage(const AString& json_object);

private:
	UInt32 m_nId;
	bool m_bSelf;
	Renderer *renderer_;
	PCbserverCallBack *callback_;
	rtc::scoped_ptr<VideoRenderer> videoRenderer_;
};