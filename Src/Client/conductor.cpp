
#include "stdafx.h"
#include "Packetizer.h"
#include "conductor.h"
#include "defaults.h"
#include <fstream>

#include "setdebugnew.h"

#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/app/webrtc/test/fakeconstraints.h"
#include "talk/media/devices/devicemanager.h"
#include "webrtc/base/common.h"
#include "webrtc/base/json.h"

#include "DesktopCapturer.h"
#include "PCObserver.h"



class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static DummySetSessionDescriptionObserver* Create() {
		return
			new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() {
		LOG(INFO) << __FUNCTION__;
	}
	virtual void OnFailure(const AString& error) {
		LOG(INFO) << __FUNCTION__ << " " << error;
	}

protected:
	DummySetSessionDescriptionObserver() {}
	~DummySetSessionDescriptionObserver() {}
};


// Sye Bye
const char kConnectSyeBye[] = "BYE";

#define DTLS_ON  true
#define DTLS_OFF false

Conductor::Conductor(ConnectionClient* client, MainWindow* main_wnd)
	: client_(client),
	main_wnd_(main_wnd) 
{
	client_->RegisterObserver(this);
	main_wnd->RegisterObserver(this);

	if (!Initialize())
	{
		main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
	}

	client->Init(SERVER, PORT, GetPeerName());
}

Conductor::~Conductor() {
	
}


void Conductor::Close() {
	client_->SignOut();
}

void Conductor::SendMessage(UInt32 id, const AString& json_object) {


	RoomMsg *msg = new RoomMsg;
	msg->id = id;
	msg->msg = json_object;

	main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, msg);
}

void Conductor::QueueUIThreadCallback(int msg_id, void* data)
{
	main_wnd_->QueueUIThreadCallback(msg_id, data);
}

void Conductor::SetLocalDescription(UInt32 id, webrtc::SessionDescriptionInterface* desc)
{
	auto it = peer_connections_.find(id);
	if (it != peer_connections_.end())
	{
		it->second->SetLocalDescription(DummySetSessionDescriptionObserver::Create(), desc);
	}
}


bool Conductor::Initialize() {
	
	ASSERT(peer_connection_factory_.get() == NULL);
	//ASSERT(peer_connection_.get() == NULL);

	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory();

	if (!peer_connection_factory_.get()) {
		main_wnd_->MessageBox("Error",
			"Failed to initialize PeerConnectionFactory", true);
		return false;
	}

	return true;
}

bool Conductor::CreatePeerConnection(UInt32 id, bool dtls) 
{
	ASSERT(peer_connection_factory_.get() != NULL);
	ASSERT(peer_connections_[id].get() == NULL);

	webrtc::PeerConnectionInterface::IceServers servers;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = "stun:admin.mice007.com:3478";
	servers.push_back(server);

	webrtc::PeerConnectionInterface::IceServer server1;
	server1.uri = "turn:admin.mice007.com:3478";
	server1.username = "username2";
	server1.password = "password2";
	servers.push_back(server1);

	webrtc::FakeConstraints constraints;
	if (dtls) {
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"true");
	}
	else
	{
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"false");
	}

	bool bSelf = id == client_->GetID();

	rtc::scoped_refptr<PCObserver> pcObserver(new rtc::RefCountedObject<PCObserver>(id, this, bSelf));

	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection =
		peer_connection_factory_->CreatePeerConnection(servers,
		&constraints,
		NULL,
		NULL,
		pcObserver);


	peer_connections_[id] = peer_connection;
	pcObservers_[id] = pcObserver;

	if (id ==client_->GetID())
	{
		AddStreams();

		peer_connection->AddStream(active_stream_);
		peer_connection->CreateOffer(pcObserver, NULL);
	}

	return peer_connection.get() != NULL;
}


void Conductor::DeletePeerConnection(UInt32 id) {

	if (id == client_->GetID())
	{
		active_stream_ = nullptr;

		for (auto &it: peer_connections_)
		{
			main_wnd_->OnUserLeftChannel(client_->GetChannel(), it.first);
			it.second->Close();
			it.second = nullptr;
		}
		peer_connections_.clear();

		for (auto &itPc : pcObservers_)
		{
			itPc.second = nullptr;
		}
		pcObservers_.clear();
	}
	else
	{
		auto it = peer_connections_.find(id);
		if (it != peer_connections_.end())
		{
			it->second->Close();
			it->second = nullptr;
			peer_connections_.erase(it);
		}

		auto itPc = pcObservers_.find(id);
		if (itPc != pcObservers_.end())
		{
			itPc->second = nullptr;
			pcObservers_.erase(itPc);
		}
	}
}

void Conductor::AddStreams() 
{
	if (active_stream_.get())
		return;  // Already existed.

	active_stream_ = peer_connection_factory_->CreateLocalMediaStream(kStreamLabel);

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
		peer_connection_factory_->CreateAudioTrack(
		kAudioLabel, peer_connection_factory_->CreateAudioSource(NULL)));	

	active_stream_->AddTrack(audio_track);

	cricket::VideoCapturer *video = OpenVideoCaptureDevice();
	if (video)
	{
		webrtc::FakeConstraints constraints;
// 		constraints.AddOptional(webrtc::MediaConstraintsInterface::kMaxWidth, "320");
// 		constraints.AddOptional(webrtc::MediaConstraintsInterface::kMaxHeight, "240");

		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
			peer_connection_factory_->CreateVideoTrack(
			kVideoLabel,
			peer_connection_factory_->CreateVideoSource(video, &constraints)));

		rtc::scoped_refptr<PCObserver> pc = pcObservers_[client_->GetID()];
// 		if (pc)
// 		{
// 			pc->StartRenderer(video_track);
// 		}

		active_stream_->AddTrack(video_track);
	}

// 	cricket::VideoCapturer *desktop = OpenDesktopCaptureDevice();
// 	if (desktop)
// 	{
// 		webrtc::FakeConstraints constraints;
// 		// 		constraints.AddOptional(webrtc::MediaConstraintsInterface::kMaxWidth, "320");
// 		// 		constraints.AddOptional(webrtc::MediaConstraintsInterface::kMaxHeight, "240");
// 
// 		rtc::scoped_refptr<webrtc::VideoTrackInterface> desktop_track(
// 			peer_connection_factory_->CreateVideoTrack(
// 			kDesktopLabel,
// 			peer_connection_factory_->CreateVideoSource(desktop, &constraints)));
// 
// 		rtc::scoped_refptr<PCObserver> pc = pcObservers_[client_->GetID()];
// 		if (pc)
// 		{
// 			pc->StartRenderer(desktop_track);
// 		}
// 
// 		active_stream_->AddTrack(desktop_track);
// 	}
}

cricket::VideoCapturer* Conductor::OpenVideoCaptureDevice()
{
	rtc::scoped_ptr<cricket::DeviceManagerInterface> dev_manager(
		cricket::DeviceManagerFactory::Create());
	if (!dev_manager->Init()) {
		LOG(LS_ERROR) << "Can't create device manager";
		return NULL;
	}
	std::vector<cricket::Device> devs;
	if (!dev_manager->GetVideoCaptureDevices(&devs)) {
		LOG(LS_ERROR) << "Can't enumerate video devices";
		return NULL;
	}

	std::vector<cricket::Device>::iterator dev_it = devs.begin();
	cricket::VideoCapturer* capturer = NULL;
	for (; dev_it != devs.end(); ++dev_it) {
		capturer = dev_manager->CreateVideoCapturer(*dev_it);
		if (capturer != NULL)
			break;
	}
	
	return capturer;
}

cricket::VideoCapturer* Conductor::OpenDesktopCaptureDevice()
{
	rtc::scoped_ptr<cricket::DeviceManagerInterface> dev_manager(
		cricket::DeviceManagerFactory::Create());
	if (!dev_manager->Init()) {
		LOG(LS_ERROR) << "Can't create device manager";
		return NULL;
	}

	std::vector<rtc::DesktopDescription> desktops;
	if (!dev_manager->GetDesktops(&desktops)) {
		LOG(LS_ERROR) << "Can't enumerate desktop devices";
		return NULL;
	}
	std::vector<rtc::DesktopDescription>::iterator desk_it = desktops.begin();
	cricket::VideoCapturer* desktopCapturer = NULL;
	for (; desk_it != desktops.end(); ++desk_it) {
		if (desk_it->primary())
		{
			desktopCapturer = new cricket::DesktopCapturer(cricket::ScreencastId(desk_it->id()));
		}
		// 		desktopCapturer = dev_manager->CreateScreenCapturer(cricket::ScreencastId(desk_it->id()));
		// 		if (desktopCapturer != NULL)
		// 			break;
	}

	return desktopCapturer;
}

HWND Conductor::handle()
{
	return main_wnd_->handle();
}

void Conductor::OnSignedIn()
{
	main_wnd_->OnSignedIn();
}

void Conductor::OnLoginError()
{
	client_->SignOut();
}

void Conductor::OnDisconnected()
{
	main_wnd_->OnDisconnected();
}

void Conductor::OnMessageSent(int err)
{
	// Process the next pending a_Msg if any.
	main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnJoinedChannel(const AString &room)
{
	if (CreatePeerConnection(client_->GetID(), DTLS_ON))
	{
		main_wnd_->OnJoinChannel(room, client_->GetID());
	}
}

void Conductor::OnLeaveChannel(const AString &room)
{
	DeletePeerConnection(client_->GetID());
	main_wnd_->OnLeftChannel(room);

	client_->SignOut();
}

void Conductor::OnServerConnectionFailure() 
{
	main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),
		true);
}

void Conductor::OnMessageFromChannel(const AString &room, const UInt32 &a_FromID, const AString& a_Msg)
{
	ASSERT(!a_Msg.empty());

	if (a_Msg.compare(kConnectSyeBye) == 0)
	{
		return;
	}

	LOG(WARNING) << "OnMessageFromChannel: "
		<< "was: " << a_Msg;

	
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(a_Msg, jmessage)) {
		LOG(WARNING) << "Received unknown a_Msg. " << a_Msg;
		return;
	}
	AString type;
	AString json_object;

	rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
	if (type == "online")
	{
		
	}
	else if (type == "publisher")
	{
		int clientID;
		if (!rtc::GetIntFromJsonObject(jmessage, "clientID",
			&clientID)) {
			LOG(WARNING) << "Can't parse received clientID a_Msg.";
			return;
		}

		if (!CreatePeerConnection(a_FromID, DTLS_ON)) {
			LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
			return;
		}

		main_wnd_->OnUserJoinChannel(room, a_FromID);

		//发送订阅消息
		Json::StyledWriter writer;
		Json::Value jmessage;

		jmessage[kSessionDescriptionTypeName] = "subscribe";
		jmessage["clientID"] = client_->GetID();
		SendMessage(clientID,writer.write(jmessage));
	}
	else if (type == "offline" || type == "unpublisher")
	{
		main_wnd_->OnUserLeftChannel(room, a_FromID);

		int clientID;
		if (!rtc::GetIntFromJsonObject(jmessage, "clientID",
			&clientID)) {
			LOG(WARNING) << "Can't parse received clientID a_Msg.";
			return;
		}

		DeletePeerConnection(clientID);

	}
	else if (type == "offer" || type == "answer") 
	{

		AString sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
			&sdp)) {
			LOG(WARNING) << "Can't parse received session description a_Msg.";
			return;
		}
		webrtc::SdpParseError error;
		webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(type, sdp, &error));
		if (!session_description) 
		{
			LOG(WARNING) << "Can't parse received session description a_Msg. "
				<< "SdpParseError was: " << error.description;
			return;
		}
		LOG(INFO) << " Received session description :" << a_Msg;

		auto it = peer_connections_.find(a_FromID);
		if (it != peer_connections_.end())
		{
			auto peer_connection = peer_connections_[a_FromID].get();

			peer_connection->SetRemoteDescription(
				DummySetSessionDescriptionObserver::Create(), session_description);

			if (session_description->type() == webrtc::SessionDescriptionInterface::kOffer)
			{
				peer_connection->CreateAnswer(pcObservers_[a_FromID], NULL);
			}
		}

		
		return;
	}
	else {
		AString sdp_mid;
		int sdp_mlineindex = 0;
		AString sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
			&sdp_mid) ||
			!rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
			&sdp_mlineindex) ||
			!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
			LOG(WARNING) << "Can't parse received a_Msg.";
			return;
		}
		webrtc::SdpParseError error;
		rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
		if (!candidate.get()) {
			LOG(WARNING) << "Can't parse received candidate a_Msg. "
				<< "SdpParseError was: " << error.description;
			return;
		}

		auto it = peer_connections_.find(a_FromID);
		if (it != peer_connections_.end())
		{
			auto peer_connection = peer_connections_[a_FromID].get();

			if (!peer_connection->AddIceCandidate(candidate.get())) {
				LOG(WARNING) << "Failed to apply the received candidate";
				return;
			}
			LOG(INFO) << " Received candidate :" << a_Msg;
		}
		
		return;
	}
}

void Conductor::PostNotification(CString msg, COLORREF color /*= RGB(0, 0, 0)*/)
{
	main_wnd_->PostNotification(msg, color);
}

//
// MainWndCallback implementation.
//

void Conductor::DisconnectFromServer() 
{
	if (client_->is_connected())
		client_->SignOut();
}

void Conductor::JoinChannel(const AString &server, const AString &room)
{
	client_->JoinChannel(server,room);
}

void Conductor::LeaveChannel(const AString &room)
{
	client_->LeaveChannel(room);
}


void Conductor::UIThreadCallback(int msg_id, void* data) {
	switch (msg_id) {
	case PEER_CONNECTION_CLOSED:
		LOG(INFO) << "PEER_CONNECTION_CLOSED";
// 		DeletePeerConnection();
// 
// 		ASSERT(!active_stream_.get());
// 
// 		if (main_wnd_->IsWindow()) {
// 			if (client_->is_connected()) {
// 				main_wnd_->SwitchToWorkUI();
// 			}
// 			else {
// 				main_wnd_->SwitchToLoginUI();
// 			}
// 		}
// 		else {
// 			DisconnectFromServer();
// 		}
 		break;

	case SEND_MESSAGE_TO_PEER: {
		LOG(INFO) << "SEND_MESSAGE_TO_PEER";
		RoomMsg* msg = reinterpret_cast<RoomMsg*>(data);

		if (msg)
		{
			if (client_->is_connected())
			{
 				if (!client_->SendToChannel(client_->GetChannel(),msg->id, msg->msg)) {
 					LOG(LS_ERROR) << "SendToRoom failed";
 					DisconnectFromServer();
				}
			}
			delete msg;
		}
		break;
	}

	case PEER_CONNECTION_ERROR:
		main_wnd_->MessageBox("Error", "an unknown error occurred", true);
		break;

	case NEW_STREAM_ADDED: 
	{
		MediaMsg* msg = reinterpret_cast<MediaMsg*>(data);
		webrtc::MediaStreamInterface* stream = msg->media;
		webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
		// Only render the first track.
		if (!tracks.empty()) {
			webrtc::VideoTrackInterface* track = tracks[0];
			rtc::scoped_refptr<PCObserver> pc = pcObservers_[msg->id];
			if (msg->id != client_->GetID() &&  pc)
			{
				pc->StartRenderer(track);
			}

		}
		stream->Release();
		delete msg;
		break;
	}

	case STREAM_REMOVED: {
		// Remote peer stopped sending a stream.
		MediaMsg* msg = reinterpret_cast<MediaMsg*>(data);
		webrtc::MediaStreamInterface* stream = msg->media;
		rtc::scoped_refptr<PCObserver> pc = pcObservers_[msg->id];
		if (pc)
		{
			pc->StopRenderer();
		}
		stream->Release();
		break;
	}

	default:
		ASSERT(false);
		break;
	}
}

LRESULT Conductor::OnNetwork(WPARAM wParam, LPARAM lParam)
{
	return client_->OnNetwork(wParam, lParam);
}

void Conductor::SetLocalRender(Renderer *render)
{
	rtc::scoped_refptr<PCObserver> pc = pcObservers_[client_->GetID()];
	if (pc)
	{
		pc->RegisterRenderer(render);
	}
}

void Conductor::SetRemoteRender(UInt32 id, Renderer *render)
{
	rtc::scoped_refptr<PCObserver> pc = pcObservers_[id];
	if (pc)
	{
		pc->RegisterRenderer(render);
	}
}
