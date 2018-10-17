#pragma once

namespace erizo
{
	class WebRtcConnection;
	class OneToManyProcessor;

	typedef std::map<int, WebRtcConnection*> WebRtcConnections;
}

#include <WebRtcConnection.h>

class cPeer
{

public:
	class cListener
	{
	public:
		//virtual void onPeerClosed() = 0;
		virtual void SendMsgToPeer(const int &a_FromID, const int &a_TargetID, const AString &a_Msg) = 0;
		virtual void BroadcastJoinChannel(const int &a_ClientID) = 0;
	};

public:
	cPeer(const int &a_Id, cListener *listener);
	~cPeer();



public:
	int getId();
	bool isReady();
	bool doJoin();
	bool doLeave();
	void onJoinedChannel();	//加入频道成功，并成功进行连接
	void onSubscribedSuccess(const int &a_TargetID);	//加入频道成功，并成功进行连接
	void onMsgFromPeer(const int &a_TargetID, const AString &msg);
	void SendMsgToPeer(const int &a_TargetID, const AString &msg);

public:
	bool onSubscribed(const int &a_TargetID);
	bool onUnSubscribed(const int &a_TargetID);

private:
	erizo::WebRtcConnection *getConnectionById(const int &a_TargetID);

protected:
	class WebRtcEventListener :public erizo::WebRtcConnectionEventListener
	{

	public:
		WebRtcEventListener(int id, cPeer*a_Participant);
		virtual void notifyEvent(erizo::WebRTCEvent newEvent, const AString& message);

	private:
		int id;
		cPeer *peer = nullptr;
		erizo::WebRTCEvent state;
	};

	typedef std::map<int, WebRtcEventListener*> WebRtcEventListeners;

private:
	erizo::OneToManyProcessor	*muxer = nullptr;		//一对多流程

	erizo::WebRtcConnection		*publisher = nullptr;		//视频发布者（本人）
	WebRtcEventListener			*eventListener = nullptr;

	erizo::WebRtcConnections	subscribers;		//视频订阅者
	WebRtcEventListeners		eventListeners;	//回掉事件订阅者	

	// Passed by argument.
	cListener* listener = nullptr;
	int id;
	bool ready = false;
};

