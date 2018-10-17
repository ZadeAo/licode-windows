#include "StdAfx.h"
#include "Room.h"
#include "Peer.h"
#include "Manager.h"

cRoom::cRoom(const AString &name, cChannelObserver *callback)
	: m_ChannelName(name)
	, m_Callback(callback)
{
	LOGINFO("channel: %s created", m_ChannelName.c_str());
}


cRoom::~cRoom()
{
	LOGINFO("channel: %s destroyed", m_ChannelName.c_str());
}

AString cRoom::getName()
{
	return m_ChannelName;
}

int cRoom::getSize()
{
	return m_Clients.size();
}

bool cRoom::onPeerJoined(const int &a_ClientID)
{
	cParticipantPtr NewHandle = std::make_shared<cPeer>(a_ClientID,this);
	if (!NewHandle->doJoin())
	{
		return false;
	}


	for (auto client : m_Clients)
	{
		//通知用户上线
		{
			//通知其他用户我上线了
			AString msg = Printf("{\"type\":\"online\",\"clientID\":%d}", a_ClientID);
			m_Callback->SendMsgToPeer(client.first, a_ClientID, m_ChannelName, msg);
		}
		{
			//通知我目前在线的用户
			AString msg = Printf("{\"type\":\"online\",\"clientID\":%d}", client.first);
			m_Callback->SendMsgToPeer(a_ClientID, client.first, m_ChannelName, msg);
		}
	}

	m_Clients.insert(std::make_pair(a_ClientID, NewHandle));

	return true;
}

bool cRoom::onPeerLeaved(const int &a_ClientID)
{
	cParticipantPtr NewHandle = nullptr;

	auto it = m_Clients.find(a_ClientID);
	if (it != m_Clients.end())
	{
		NewHandle = it->second;
		m_Clients.erase(a_ClientID);
	}

	if (!NewHandle)
	{
		return true;
	}

	NewHandle->doLeave();

	AString msg = Printf("{\"type\":\"offline\",\"clientID\":%d}", a_ClientID);

	for (auto client : m_Clients)
	{
		NewHandle->onUnSubscribed(client.first);//依次被存在的取消订阅
		//依次取消订阅存在的用户
		client.second->onUnSubscribed(a_ClientID);

		m_Callback->SendMsgToPeer(client.first, a_ClientID, m_ChannelName, msg);
	}

	return true;
}

void cRoom::onMsgFromPeer(const int &a_FromID, const int &a_TargetID, const AString &a_Msg)
{
	auto it = m_Clients.find(a_TargetID);
	if (it != m_Clients.end())
	{
		it->second->onMsgFromPeer(a_FromID, a_Msg);
	}
}

void cRoom::SendMsgToPeer(const int &a_FromID, const int &a_TargetID, const AString &a_Msg)
{
	m_Callback->SendMsgToPeer(a_TargetID, a_FromID, m_ChannelName, a_Msg);
}

void cRoom::BroadcastJoinChannel(const int &a_ClientID)
{
	auto NewHandle = m_Clients[a_ClientID];
	if (!NewHandle)
	{
		return;
	}

	

	for (auto client : m_Clients)
	{
		if (client.first != a_ClientID)
		{
			{
				//通知其他人我发布了视频
				AString msg = Printf("{\"type\":\"publisher\",\"clientID\":%d}", a_ClientID);
				m_Callback->SendMsgToPeer(client.first, a_ClientID, m_ChannelName, msg);
			}
			{
				//通知我谁发布了视频
				AString msg = Printf("{\"type\":\"publisher\",\"clientID\":%d}", client.first);
				m_Callback->SendMsgToPeer(a_ClientID,client.first,m_ChannelName, msg);
			}
		}		
	}
}
