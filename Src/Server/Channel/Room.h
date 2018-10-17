#pragma once

class cPeer;
typedef std::shared_ptr<cPeer> cParticipantPtr;
typedef std::map<int,cParticipantPtr> cParticipantPtrs;
typedef std::map<int,cPeer*> cParticipants;

class cChannelObserver;

#include "Peer.h"

class cRoom : public cPeer::cListener
{
public:
	class Listener
	{
	public:
		virtual void onRoomClosed() = 0;
	};

public:
	cRoom(const AString &name, cChannelObserver *callback);
	~cRoom();
	AString getName();
	int getSize();

	bool onPeerJoined(const int &a_ClientID);
	bool onPeerLeaved(const int &a_ClientID);	
	
	void onMsgFromPeer(const int &a_FromID, const int &a_TargetID, const AString &a_Msg);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
public:
	virtual void SendMsgToPeer(const int &a_FromID, const int &a_TargetID, const AString &a_Msg);
	virtual void BroadcastJoinChannel(const int &a_ClientID);

private:
	/** Clients that are connected to the server and not yet assigned to a cWorld. */
	cParticipantPtrs m_Clients;

	AString m_ChannelName;
	cChannelObserver *m_Callback;
};

