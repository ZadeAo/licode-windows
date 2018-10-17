// ReDucTor is an awesome guy who helped me a lot

#include "stdafx.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Server.h"
#include "ClientHandle.h"
#include "Root.h"
#include "CommandOutput.h"

#include "IniFile.h"

#include <fstream>
#include <sstream>
#include <iostream>

extern "C"
{
	#include "zlib/zlib.h"
}




/** Enable the memory leak finder - needed for the "dumpmem" server command:
Synchronize this with main.cpp - the leak finder needs initialization before it can be used to dump memory
_X 2014_02_20: Disabled for canon repo, it makes the debug version too slow in MSVC2013
and we haven't had a memory leak for over a year anyway. */
// #define ENABLE_LEAK_FINDER

#if defined(_MSC_VER) && defined(_DEBUG) && defined(ENABLE_LEAK_FINDER)
	#pragma warning(push)
	#pragma warning(disable:4100)
	#include "LeakFinder.h"
	#pragma warning(pop)
#endif





typedef std::list< cClientHandle* > ClientList;





////////////////////////////////////////////////////////////////////////////////
// cServerListenCallbacks:

class cServerListenCallbacks:
	public cNetwork::cListenCallbacks
{
	cServer & m_Server;
	UInt16 m_Port;

	virtual cTCPLink::cCallbacksPtr OnIncomingConnection(const AString & a_RemoteIPAddress, UInt16 a_RemotePort) override
	{
		return m_Server.OnConnectionAccepted(a_RemoteIPAddress);
	}

	virtual void OnAccepted(cTCPLink & a_Link) override {}

	virtual void OnError(int a_ErrorCode, const AString & a_ErrorMsg) override
	{
		LOGWARNING("Cannot listen on port %d: %d (%s).", m_Port, a_ErrorCode, a_ErrorMsg.c_str());
	}

public:
	cServerListenCallbacks(cServer & a_Server, UInt16 a_Port):
		m_Server(a_Server),
		m_Port(a_Port)
	{
	}
};





////////////////////////////////////////////////////////////////////////////////
// cServer::cTickThread:

cServer::cTickThread::cTickThread(cServer & a_Server) :
	super("ServerTickThread"),
	m_Server(a_Server)
{
}





void cServer::cTickThread::Execute(void)
{
	auto LastTime = std::chrono::steady_clock::now();
	static const auto msPerTick = std::chrono::milliseconds(50);

	while (!m_ShouldTerminate)
	{
		auto NowTime = std::chrono::steady_clock::now();
		auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(NowTime - LastTime).count();
		m_ShouldTerminate = !m_Server.Tick(static_cast<float>(msec));
		auto TickTime = std::chrono::steady_clock::now() - NowTime;

		if (TickTime < msPerTick)
		{
			// Stretch tick time until it's at least msPerTick
			std::this_thread::sleep_for(msPerTick - TickTime);
		}

		LastTime = NowTime;
	}
}





////////////////////////////////////////////////////////////////////////////////
// cServer:

cServer::cServer(void) :
	m_PlayerCount(0),
	m_PlayerCountDiff(0),
	m_bIsConnected(false),
	m_bRestarting(false),
	m_TickThread(*this)
{
}



void cServer::PlayerCreated(const cPlayer * a_Player)
{
	UNUSED(a_Player);
	// To avoid deadlocks, the player count is not handled directly, but rather posted onto the tick thread
	cCSLock Lock(m_CSPlayerCountDiff);
	m_PlayerCountDiff += 1;
}





void cServer::PlayerDestroying(const cPlayer * a_Player)
{
	UNUSED(a_Player);
	// To avoid deadlocks, the player count is not handled directly, but rather posted onto the tick thread
	cCSLock Lock(m_CSPlayerCountDiff);
	m_PlayerCountDiff -= 1;
}





bool cServer::InitServer(cSettingsRepositoryInterface & a_Settings, bool a_ShouldAuth)
{
// 	LOG("Create database pool...");
// 	if (!CDBManager::Instance()->Init(a_Settings))
// 	{
// 		CDBManager::UnInstance();
// 		LOGERROR("Database pool init failed, aborting...");
// 		return false;
// 	}

	m_PlayerCount = 0;
	m_PlayerCountDiff = 0;

	if (m_bIsConnected)
	{
		LOGERROR("ERROR: Trying to initialize server while server is already running!");
		return false;
	}

	m_Port = a_Settings.GetValueSetI("Server", "Port", 6666);

	m_bIsConnected = true;

	m_ServerID = "-";



	PrepareKeys();

	return true;
}


int cServer::GetNumPlayers(void) const
{
	cCSLock Lock(m_CSPlayerCount);
	return m_PlayerCount;
}





bool cServer::IsPlayerInQueue(AString a_Username)
{
	cCSLock Lock(m_CSClients);
	for (auto client : m_Clients)
	{
		if ((client->GetUsername()).compare(a_Username) == 0)
		{
			return true;
		}
	}
	return false;
}





void cServer::PrepareKeys(void)
{
	LOGD("Generating protocol encryption keypair...");
	VERIFY(m_PrivateKey.Generate(1024));
	m_PublicKeyDER = m_PrivateKey.GetPubKeyDER();
}





cTCPLink::cCallbacksPtr cServer::OnConnectionAccepted(const AString & a_RemoteIPAddress)
{
	LOGD("Client \"%s\" connected!", a_RemoteIPAddress.c_str());
	cClientHandlePtr NewHandle = std::make_shared<cClientHandle>(a_RemoteIPAddress);
	cCSLock Lock(m_CSClients);
	m_Clients.push_back(NewHandle);
	return NewHandle;
}





bool cServer::Tick(float a_Dt)
{
	// Apply the queued playercount adjustments (postponed to avoid deadlocks)
	int PlayerCountDiff = 0;
	{
		cCSLock Lock(m_CSPlayerCountDiff);
		std::swap(PlayerCountDiff, m_PlayerCountDiff);
	}
	{
		cCSLock Lock(m_CSPlayerCount);
		m_PlayerCount += PlayerCountDiff;
	}
	// Let the Root process all the queued commands:
	cRoot::Get()->TickCommands();

	// Tick all clients not yet assigned to a world:
	TickClients(a_Dt);

	if (!m_bRestarting)
	{
		return true;
	}
	else
	{
		m_bRestarting = false;
		m_RestartEvent.Set();
		return false;
	}
}





void cServer::TickClients(float a_Dt)
{
	cClientHandlePtrs RemoveClients;
	{
		cCSLock Lock(m_CSClients);

		// Remove clients that have moved to a world (the world will be ticking them from now on)
		for (auto itr = m_ClientsToRemove.begin(), end = m_ClientsToRemove.end(); itr != end; ++itr)
		{
			for (auto itrC = m_Clients.begin(), endC = m_Clients.end(); itrC != endC; ++itrC)
			{
				if (itrC->get() == *itr)
				{
					m_Clients.erase(itrC);
					break;
				}
			}
		}  // for itr - m_ClientsToRemove[]
		m_ClientsToRemove.clear();

		// Tick the remaining clients, take out those that have been destroyed into RemoveClients
		for (auto itr = m_Clients.begin(); itr != m_Clients.end();)
		{
			if ((*itr)->IsDestroyed())
			{
				// Delete the client later, when CS is not held, to avoid deadlock: http://forum.mc-server.org/showthread.php?tid=374
				RemoveClients.push_back(*itr);
				itr = m_Clients.erase(itr);
				continue;
			}
			(*itr)->ServerTick(a_Dt);
			++itr;
		}  // for itr - m_Clients[]
	}

	// Delete the clients that have been destroyed
	RemoveClients.clear();
}





bool cServer::Start(void)
{
	auto Handle = cNetwork::Listen(m_Port, std::make_shared<cServerListenCallbacks>(*this, m_Port));
	if (Handle->IsListening())
	{
		m_ServerHandle = Handle;
	}
	else
	{
		LOGERROR("Couldn't open port. Aborting the server");
		return false;
	}
	if (!m_TickThread.Start())
	{
		return false;
	}
	return true;
}


void cServer::ExecuteConsoleCommand(const AString & a_Cmd, cCommandOutputCallback & a_Output)
{
	AStringVector split = StringSplit(a_Cmd, " ");
	if (split.empty())
	{
		return;
	}

	// "stop" and "restart" are handled in cRoot::ExecuteConsoleCommand, our caller, due to its access to controlling variables

	// "help" and "reload" are to be handled by MCS, so that they work no matter what
	if (split[0] == "help")
	{
		PrintHelp(split, a_Output);
		a_Output.Finished();
		return;
	}
	else if (split[0] == "reload")
	{
		a_Output.Finished();
		return;
	}
	else if (split[0] == "reloadplugins")
	{
		a_Output.Out("Plugins reloaded");
		a_Output.Finished();
		return;
	}


	a_Output.Out("Unknown command, type 'help' for all commands.");
	a_Output.Finished();
}





void cServer::PrintHelp(const AStringVector & a_Split, cCommandOutputCallback & a_Output)
{
	UNUSED(a_Split);
	typedef std::pair<AString, AString> AStringPair;
	typedef std::vector<AStringPair> AStringPairs;
}



void cServer::Shutdown(void)
{
	// Stop listening on all sockets:
	m_ServerHandle->Close();
	m_ServerHandle.reset();

// 	LOGD("Shutting down database pool...");
// 	CDBManager::UnInstance();

	// Notify the tick thread and wait for it to terminate:
	m_bRestarting = true;
	m_RestartEvent.Wait();

	// Remove all clients:
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		(*itr)->Destroy();
	}
	m_Clients.clear();
}





void cServer::KickUser(int a_ClientID, const int & a_Reason)
{
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		if ((*itr)->GetUniqueID() == a_ClientID)
		{
			(*itr)->Kick(a_Reason);
		}
	}  // for itr - m_Clients[]
}


void cServer::AuthenticateUser(int a_ClientID, const AString & a_Name, const AString & a_Password, const Json::Value & a_Properties)
{
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		if ((*itr)->GetUniqueID() == a_ClientID)
		{
			(*itr)->Authenticate(a_Name, a_Password, a_Properties);
			return;
		}
	}  // for itr - m_Clients[]
}

void cServer::onJoinChannelResult(const int a_ClientID, const AString &a_Channel, const int & a_Result)
{
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		if ((*itr)->GetUniqueID() == a_ClientID)
		{
			(*itr)->onJoinChannelResult(a_Channel, a_Result);
		}
	}  // for itr - m_Clients[]
}

void cServer::onLeaveChannelResult(const int a_ClientID, const AString &a_Channel, const int & a_Result)
{
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		if ((*itr)->GetUniqueID() == a_ClientID)
		{
			(*itr)->onLeaveChannelResult(a_Channel, a_Result);
		}
	}  // for itr - m_Clients[]
}

void cServer::onChannelMsg(const AString &a_Channel, const int &a_FromID, const int &a_TargetID, const AString &a_Msg)
{
	cCSLock Lock(m_CSClients);
	for (auto itr = m_Clients.begin(); itr != m_Clients.end(); ++itr)
	{
		if ((*itr)->GetUniqueID() == a_TargetID)
		{
			(*itr)->onChannelMsg(a_Channel,a_FromID, a_Msg);
		}
	}  // for itr - m_Clients[]
}
