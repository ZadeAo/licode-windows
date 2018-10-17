#include "stdafx.h"
#include "PCObserver.h"
#include "conductor.h"
#include "defaults.h"

#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/test/fakeconstraints.h"
#include "talk/media/devices/devicemanager.h"
#include "webrtc/base/common.h"
#include "webrtc/base/json.h"


Json::Value& Json::Value::operator=(Json::Value other) {
	swap(other);
	return *this;
}

PCObserver::PCObserver(UInt32 id, PCbserverCallBack *callback, bool bSelf)
	: m_nId(id)
	, m_bSelf(bSelf)
	, renderer_(nullptr)
	, callback_(callback)
{
}


PCObserver::~PCObserver()
{
}

//
// PeerConnectionObserver implementation.
//

void PCObserver::OnError() {
	LOG(LS_ERROR) << __FUNCTION__;
	callback_->QueueUIThreadCallback(PEER_CONNECTION_ERROR, NULL);
}

// Called when a remote stream is added
void PCObserver::OnAddStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();

	if (m_bSelf)
	{
		return;
	}

	stream->AddRef();

// 	MediaMsg *msg = new MediaMsg;
// 	msg->id = m_nId;
// 	msg->media = stream;
// 
// 	callback_->QueueUIThreadCallback(NEW_STREAM_ADDED, msg);

	webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
	if (!tracks.empty()) {
		webrtc::VideoTrackInterface* track = tracks[0];
		StartRenderer(track);
	}

	stream->Release();
}

void PCObserver::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();
	stream->AddRef();

// 	MediaMsg *msg = new MediaMsg;
// 	msg->id = m_nId;
// 	msg->media = stream;
// 
// 	callback_->QueueUIThreadCallback(STREAM_REMOVED, msg);

	StopRenderer();

	stream->Release();
}

void PCObserver::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
 	LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();

	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
	jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
	AString sdp;
	if (!candidate->ToString(&sdp)) {
		LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}
	sdp = "a=" + sdp;
	jmessage[kCandidateSdpName] = sdp;
 	SendMessage(writer.write(jmessage));
}

void PCObserver::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	callback_->SetLocalDescription(m_nId, desc);

	AString sdp;
	desc->ToString(&sdp);

	LOG(INFO) << "type: " << desc->type() << "sdp: " << sdp;

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage[kSessionDescriptionTypeName] = desc->type();
	jmessage[kSessionDescriptionSdpName] = sdp;
	SendMessage(writer.write(jmessage));
}

void PCObserver::OnFailure(const AString& error)
{
	LOG(LERROR) << error;
}



void PCObserver::SendMessage(const AString& json_object) {

	callback_->SendMessage(m_nId, json_object);
}

void PCObserver::RegisterRenderer(Renderer *renderer)
{
	renderer_ = renderer;
	if (videoRenderer_)
	{
		videoRenderer_->RegisterRenderer(renderer);
	}
}

void PCObserver::StartRenderer(webrtc::VideoTrackInterface* remote_video)
{
	videoRenderer_.reset(new VideoRenderer(renderer_, 1, 1, remote_video));
}

void PCObserver::StopRenderer()
{
	videoRenderer_.reset();
}