#include "StdAfx.h"
#include "Peer.h"
#include "Room.h"
#include <json/json.h>
#include <OneToManyProcessor.h>

#define  MAX_PORT 59999
#define  MIN_PORT 10000

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";
// Sye Bye
const char kConnectSyeBye[] = "BYE";


cPeer::WebRtcEventListener::WebRtcEventListener(int id, cPeer*a_Participant)
	: id(id)
	, peer(a_Participant)
{

}

void cPeer::WebRtcEventListener::notifyEvent(erizo::WebRTCEvent newEvent, const AString& message)
{
	if (state == newEvent)
	{
		return;
	}

	state = newEvent;

	switch (newEvent)
	{
	case erizo::CONN_INITIAL:
		break;
	case erizo::CONN_STARTED:
		break;
	case erizo::CONN_SDP:
	case erizo::CONN_GATHERED:
	{
		AString localsdp = message;
		Json::StyledWriter writer;
		Json::Value jmessage;
		jmessage[kSessionDescriptionSdpName] = localsdp;
		jmessage[kSessionDescriptionTypeName] = peer->getId() == id ? "answer" : "offer";
		peer->SendMsgToPeer(id, writer.write(jmessage));

//		::MessageBoxA(nullptr, localsdp.c_str(), "", MB_OK);
	}
	break;
	case erizo::CONN_READY:
	{
		if (peer->getId() == id)
		{
			peer->onJoinedChannel();
		}
		else
		{
			peer->onSubscribedSuccess(id);
		}
	}
		break;
	case erizo::CONN_FINISHED:
		delete this;
		break;
	case erizo::CONN_CANDIDATE:
	{
		peer->SendMsgToPeer(id, message);
	}
	break;
	case erizo::CONN_FAILED:
		break;
	default:
		break;
	}
}

cPeer::cPeer(const int &id, cListener *listener)
	: id(id)
	, listener(listener)
{
	muxer = new erizo::OneToManyProcessor();
}


cPeer::~cPeer()
{
	for (auto listener : eventListeners)
	{
		if (listener.first != id)
		{
			muxer->removeSubscriber(Printf("%d", listener.first));
			delete listener.second;
		}
	}

	eventListeners.clear();

	delete muxer;
	muxer = nullptr;
}

int cPeer::getId()
{
	return id;
}

bool cPeer::isReady()
{
	return ready;
}

bool cPeer::doJoin()
{
	erizo::IceConfig iceConfig;
	iceConfig.turnServer = "admin.mice007.com";
	iceConfig.turnUsername = "username1";
	iceConfig.turnPass = "password1";
	iceConfig.stunServer = "admin.mice007.com";
	iceConfig.stunPort = 3478;
	iceConfig.turnPort = 3478;
	iceConfig.minPort = MIN_PORT;
	iceConfig.maxPort = MAX_PORT;
	eventListener = new WebRtcEventListener(id, this);
	publisher = new erizo::WebRtcConnection(Printf("%d", id), true, true, iceConfig, eventListener);

	publisher->setAudioSink(muxer);//音频接收者
	publisher->setVideoSink(muxer);//视频接收者
	muxer->setPublisher(publisher);

	return publisher->init();
}

bool cPeer::doLeave()
{
	ready = false;
	return true;
}

void cPeer::onJoinedChannel()
{
	ready = true;

	listener->BroadcastJoinChannel(id);
}

void cPeer::onSubscribedSuccess(const int &a_TargetID)
{
// 	auto it = subscribers.find(a_TargetID);
// 	if (it != subscribers.end())
// 	{
// 		erizo::WebRtcConnection *newConn = it->second;
// 		if (newConn)
// 		{
// 			muxer->addSubscriber(newConn, Printf("%d", a_TargetID));
// 		}
// 	}
}

bool cPeer::onSubscribed(const int &a_TargetID)
{
	erizo::IceConfig iceConfig;
	iceConfig.turnServer = "admin.mice007.com";
	iceConfig.turnUsername = "username1";
	iceConfig.turnPass = "password1";
	iceConfig.stunServer = "admin.mice007.com";
	iceConfig.stunPort = 3478;
	iceConfig.turnPort = 3478;
	iceConfig.minPort = MIN_PORT;
	iceConfig.maxPort = MAX_PORT;
	WebRtcEventListener *listener = new WebRtcEventListener(a_TargetID, this);
	erizo::WebRtcConnection *newConn = new erizo::WebRtcConnection(Printf("%d", a_TargetID), true, true, iceConfig, listener);
	//解决音频没声音问题;
	newConn->localSdp_.setOfferSdp(publisher->localSdp_);

	//新增新的订阅者和事件观测者
	subscribers[a_TargetID] = newConn;
	eventListeners[a_TargetID] = listener;
	newConn->init();
	
	muxer->addSubscriber(newConn, Printf("%d", a_TargetID));
	newConn->createOffer();
	return true;
}

bool cPeer::onUnSubscribed(const int &a_TargetID)
{
	if (muxer)
	{
		muxer->removeSubscriber(Printf("%d", a_TargetID));
	}

	//移除存在的订阅者
	auto itSub = subscribers.find(a_TargetID);
	if (itSub != subscribers.end())
	{
		subscribers.erase(itSub);
	}

	//移除存在的事件观测者
	auto it = eventListeners.find(a_TargetID);
	if (it != eventListeners.end())
	{
		delete it->second;
		eventListeners.erase(it);
	}

	return true;
}

void cPeer::onMsgFromPeer(const int &a_TargetID, const AString &msg)
{
	ASSERT(!msg.empty());

	if (msg.compare(kConnectSyeBye) == 0)
	{
		return;
	}

	
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(msg, jmessage)) {
		LOGWARNING("Received unknown message. ", msg.c_str());
		return;
	}

	AString type = jmessage[kSessionDescriptionTypeName].asString();

	if (type == "subscribe")
	{
		int clientID = jmessage["clientID"].asInt();
		onSubscribed(clientID);
	}
	else if (type == "offer" || type == "answer") 
	{
		AString sdp = jmessage[kSessionDescriptionSdpName].asString();
		if (sdp.empty()) {
			LOGWARNING("Can't parse received session description message.");
			return;
		}

		erizo::WebRtcConnection * newConn = getConnectionById(a_TargetID);
		if (!newConn)
		{
			return;
		}

		newConn->setRemoteSdp(sdp);
	}
	else 
	{
		AString sdp_mid = jmessage[kCandidateSdpMidName].asString();
		int sdp_mlineindex = jmessage[kCandidateSdpMlineIndexName].asInt();
		AString sdp = jmessage[kCandidateSdpName].asString();
		if (sdp_mid.empty() || sdp.empty()) {
			LOGWARNING("Can't parse received message.");
			return;
		}

		erizo::WebRtcConnection * newConn = getConnectionById(a_TargetID);
		if (!newConn)
		{
			return;
		}

		newConn->addRemoteCandidate(sdp_mid, sdp_mlineindex, sdp);
	}
}

void cPeer::SendMsgToPeer(const int &a_TargetID, const AString &a_Msg)
{
	listener->SendMsgToPeer(id, a_TargetID, a_Msg);
}

erizo::WebRtcConnection *cPeer::getConnectionById(const int &a_TargetID)
{
	if (a_TargetID == id)
	{
		return publisher;
	}
	else
	{
		auto it = subscribers.find(a_TargetID);
		if (it != subscribers.end())
		{
			return it->second;
		}
	}

	return nullptr;
}
