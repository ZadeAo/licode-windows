#pragma once

#include "../OSSupport/IsThread.h"

class cRoom;
typedef std::map<AString, cRoom*> cRooms;

class cChannelObserver
{
public:
	virtual void SendMsgToPeer(const int &a_TargetID, const int &a_FromID, const AString &a_Channel, const AString &a_Msg) = 0;
};


class cManager :
	public cIsThread, public cChannelObserver
{
	typedef cIsThread super;

public:
	cManager();
	~cManager();

	/** Starts the authenticator thread. The thread may be started and stopped repeatedly */
	void Start();

	/** Stops the authenticator thread. The thread may be started and stopped repeatedly */
	void Stop(void);

	void doUserJoinChannel(const int &a_ClientID, const AString &a_Channel);
	void doUserLeftChannel(const int &a_ClientID, const AString &a_Channel);
	void doUserOffline(const int &a_ClientID);

	void OnMsgFromPeer(const AString &a_Channel, const int &a_FromID, const int &a_TargetID, const AString &a_Msg);
	void SendMsgToPeer(const int &a_TargetID, const int &a_FromID, const AString &a_Channel, const AString &a_Msg);

	/** cIsThread override: */
	virtual void Execute(void) override;

private:
	class cMediaEvent
	{
	public:

		enum Type
		{
			NONE = 0,
			JOIN,
			LEAVE,
			MEDIA,
		};

		int     m_ClientID;
		AString m_Channel;
		Type    m_Type = NONE;
		AString m_Msg = "";
		int     m_TargetID = 0;
		

		cMediaEvent(const int &a_ClientID,const AString & a_Channel)
			: m_ClientID(a_ClientID)
			, m_Channel(a_Channel)
		{
		}

		void SetType(const Type &type)
		{
			m_Type = type;
		}

		void SetMsg(const AString &a_Msg)
		{
			m_Msg = a_Msg;
		}

		void SetTarget(const int &a_FromID)
		{
			m_TargetID = a_FromID;
		}
	};

	typedef std::deque<cMediaEvent> cEventList;

protected:
	void addEvent(cMediaEvent &event);
	void dealEvent(cMediaEvent &event);

	cRoom *doGetRoom(const AString &name);
	cRoom *doGetRoomSet(const AString &name);

	void doReleaseRoom(const AString &name);

private:
	cCriticalSection m_CSChannels;
	cRooms     rooms;

	cCriticalSection m_CS;
	cEventList       m_Queue;
	cEvent           m_QueueNonempty;
};

