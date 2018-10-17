#include "StdAfx.h"
#include "Manager.h"
#include "Room.h"
#include "../Root.h"
#include "../Server.h"
#include "../ClientHandle.h"


cManager::cManager()
	: super("cManager")
{
}

cManager::~cManager()
{
	Stop();
}

void cManager::Start()
{
	m_ShouldTerminate = false;
	super::Start();
}

void cManager::Stop(void)
{
	m_ShouldTerminate = true;
	m_QueueNonempty.Set();
	Wait();
}


void cManager::doUserJoinChannel(const int &a_ClientID, const AString &a_Channel)
{
	cMediaEvent event(a_ClientID, a_Channel);
	event.SetType(cMediaEvent::JOIN);

	addEvent(event);
}

void cManager::doUserLeftChannel(const int &a_ClientID, const AString &a_Channel)
{
	cMediaEvent event(a_ClientID, a_Channel);
	event.SetType(cMediaEvent::LEAVE);

	addEvent(event);
}

void cManager::doUserOffline(const int &a_ClientID)
{
	cCSLock LOCK(m_CSChannels);

	for (auto channel : rooms)
	{
		auto channelPtr = channel.second;

		cMediaEvent event(a_ClientID, channelPtr->getName());
		event.SetType(cMediaEvent::LEAVE);

		addEvent(event);
	}
}

void cManager::OnMsgFromPeer(const AString &a_Channel, const int &a_FromID, const int &a_TargetID, const AString &a_Msg)
{
	cMediaEvent event(a_FromID, a_Channel);
	event.SetType(cMediaEvent::MEDIA);
	event.SetMsg(a_Msg);
	event.SetTarget(a_TargetID);
	addEvent(event);
}

void cManager::SendMsgToPeer(const int &a_TargetID, const int &a_FromID, const AString &a_Channel, const AString &a_Msg)
{
	cServer *server = cRoot::Get()->GetServer();
	server->onChannelMsg(a_Channel, a_FromID, a_TargetID, a_Msg);
}

void cManager::addEvent(cMediaEvent &event)
{
	cCSLock LOCK(m_CS);
	m_Queue.push_back(event);
	m_QueueNonempty.Set();
}

void cManager::dealEvent(cMediaEvent &event)
{
	cServer *server = cRoot::Get()->GetServer();

	switch (event.m_Type)
	{
	case cMediaEvent::JOIN:
	{
		cCSLock LOCK(m_CSChannels);

		cRoom *room = doGetRoomSet(event.m_Channel);

		int ret = room->onPeerJoined(event.m_ClientID);

		server->onJoinChannelResult(event.m_ClientID, event.m_Channel, ret);

	}
		break;
	case cMediaEvent::LEAVE:
	{
		cCSLock LOCK(m_CSChannels);

		cRoom *room = doGetRoomSet(event.m_Channel);

		int ret = room->onPeerLeaved(event.m_ClientID);

		server->onLeaveChannelResult(event.m_ClientID, event.m_Channel, ret);

		if (room->getSize() == 0)
		{
			doReleaseRoom(event.m_Channel);
		}
	}
		break;
	case cMediaEvent::MEDIA:
	{
		cCSLock LOCK(m_CSChannels);

		cRoom *room = doGetRoom(event.m_Channel);
		if (room)
		{
			room->onMsgFromPeer(event.m_ClientID, event.m_TargetID, event.m_Msg);
		}
	}
		break;
	case cMediaEvent::NONE:
	default:
		break;
	}
}

void cManager::Execute(void)
{
	for (;;)
	{
		cCSLock Lock(m_CS);
		while (!m_ShouldTerminate && (m_Queue.size() == 0))
		{
			cCSUnlock Unlock(Lock);
			m_QueueNonempty.Wait();
		}
		if (m_ShouldTerminate)
		{
			return;
		}
		ASSERT(!m_Queue.empty());

		cMediaEvent event = m_Queue.front();

		m_Queue.pop_front();
		Lock.Unlock();

		//TODO...
		dealEvent(event);

	}  // for (-ever)
}


cRoom *cManager::doGetRoom(const AString &name)
{
	auto it = rooms.find(name);
	if (it != rooms.end())
	{
		return it->second;
	}

	return nullptr;
}

cRoom *cManager::doGetRoomSet(const AString &name)
{
	cRoom *room = nullptr;
	auto it = rooms.find(name);
	if (it != rooms.end())
	{
		room = it->second;
	}
	else
	{
		room = new cRoom(name, this);
		rooms.insert(std::make_pair(name, room));
	}

	return room;
}

void cManager::doReleaseRoom(const AString & name)
{
	cRoom *room = nullptr;
	auto it = rooms.find(name);
	if (it != rooms.end())
	{
		room = it->second;

		delete room;
		room = nullptr;
	}

	rooms.erase(name);
}
