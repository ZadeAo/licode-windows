#include "stdafx.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "ClientHandle.h"
#include "Server.h"
#include "Root.h"
#include "Protocol/cProtocol_impl.h"
#include "Protocol/Authenticator.h"
#include "polarssl/md5.h"

#include "ErrorCode.h"

/** Maximum number of explosions to send this tick, server will start dropping if exceeded */
#define MAX_EXPLOSIONS_PER_TICK 20

/** Maximum number of block change interactions a player can perform per tick - exceeding this causes a kick */
#define MAX_BLOCK_CHANGE_INTERACTIONS 20

/** The interval for sending pings to clients.
Server sends one ping every 30 second. */
static const std::chrono::milliseconds PING_TIME_MS = std::chrono::milliseconds(1000*15);//15s






int cClientHandle::s_ClientCount = 0;





////////////////////////////////////////////////////////////////////////////////
// cClientHandle:

cClientHandle::cClientHandle(const AString & a_IPString) :
	m_IPString(a_IPString),
	m_HasSentDC(false),
	m_TicksSinceLastPacket(0),
	m_Ping(1000),
	m_PingID(1),
	m_State(csConnected),
	m_UniqueID(0)
{
	m_Protocol = new cProtocol_impl(this);
	
	s_ClientCount++;  // Not protected by CS because clients are always constructed from the same thread
	m_UniqueID = s_ClientCount;
	m_PingStartTime = std::chrono::steady_clock::now();

	LOGD("New ClientHandle created at %p", this);
}





cClientHandle::~cClientHandle()
{
	ASSERT(m_State == csDestroyed);  // Has Destroy() been called?
	
	LOGD("Deleting client \"%s\" at %p", GetUsername().c_str(), this);

	if (!m_HasSentDC)
	{
		SendDisconnect(ERROR_CODE_SERVER_SHUTDOWN);
	}
	
	delete m_Protocol;
	m_Protocol = nullptr;
	
	LOGD("ClientHandle at %p deleted", this);
}





void cClientHandle::Destroy(void)
{
	{
		cCSLock Lock(m_CSOutgoingData);
		m_Link.reset();
	}
	{
		cCSLock Lock(m_CSDestroyingState);
		if (m_State >= csDestroying)
		{
			// Already called
			return;
		}
		m_State = csDestroying;
	}
	
	// DEBUG:
	LOGD("%s: client %p, \"%s\"", __FUNCTION__, this, m_Username.c_str());
	

	m_State = csDestroyed;
}


void cClientHandle::Kick(const int & a_Reason)
{
	if (m_State >= csAuthenticating)  // Don't log pings
	{
		LOGINFO("Kicking player %s for error code: \"%d\"", m_Username.c_str(), a_Reason);
	}
	SendDisconnect(a_Reason);
}

void cClientHandle::onJoinChannelResult(const AString &a_Channel, const int & a_Result)
{
	m_Protocol->onJoinChannelResult(a_Channel, a_Result);
}

void cClientHandle::onLeaveChannelResult(const AString &a_Channel, const int & a_Result)
{
	m_Protocol->onLeaveChannelResult(a_Channel, a_Result);
}

void cClientHandle::onChannelMsg(const AString &a_Channel, const int &a_FromID, const AString &a_Msg)
{
	m_Protocol->onChannelMsg(a_FromID, a_Channel, a_Msg);
}

void cClientHandle::Authenticate(const AString & a_Name, const AString & a_PassWord, const Json::Value & a_Properties)
{
	if (m_State != csAuthenticating)
	{
		return;
	}


// 	if (!CheckMultiLogin(a_Name))
// 	{
// 		return;
// 	}

	m_Username = a_Name;
	
	// Only assign UUID and properties if not already pre-assigned (BungeeCord sends those in the Handshake packet):
	if (m_UUID.empty())
	{
		m_UUID = a_PassWord;
	}

	
	// Send login success
	m_Protocol->SendLoginSuccess();




	m_State = csAuthenticated;


// 	// Delay the first ping until the client "settles down"
// 	// This should fix #889, "BadCast exception, cannot convert bit to fm" error in client
 	m_PingStartTime = std::chrono::steady_clock::now() + std::chrono::seconds(3);  // Send the first KeepAlive packet in 3 seconds

	LOGINFO("User %s authenticated with IP: %s", a_Name.c_str(), m_IPString.c_str());
}



void cClientHandle::HandlePing(void)
{
	// Somebody tries to retrieve information about the server
	AString Reply;
//	const cServer & Server = *cRoot::Get()->GetServer();

//	Kick(Reply);
}

bool cClientHandle::HandleLogin(const AString & a_Username, const AString & a_Password)
{
	// Schedule for authentication; until then, let the player wait (but do not block)
	m_State = csAuthenticating;
	cRoot::Get()->GetAuthenticator().Authenticate(GetUniqueID(), a_Username, a_Password);
	return true;
}



void cClientHandle::HandleKeepAlive(int a_KeepAliveID)
{
	if (a_KeepAliveID == m_PingID)
	{
		m_Ping = std::chrono::steady_clock::now() - m_PingStartTime;
	}
}





bool cClientHandle::CheckMultiLogin(const AString & a_Username)
{
	// Check if the player is waiting to be transferred to the World.
	if (cRoot::Get()->GetServer()->IsPlayerInQueue(a_Username))
	{
		Kick(ERROR_CODE_ACCOUNT_ALREADY_LOGIN);
		return false;
	}

// 	class cCallback :
// 		public cPlayerListCallback
// 	{
// 		virtual bool Item(cPlayer * a_Player) override
// 		{
// 			return true;
// 		}
// 	} Callback;
// 
// 	// Check if the player is in any World.
// 	if (cRoot::Get()->DoWithPlayer(a_Username, Callback))
// 	{
// 		Kick("A player of the username is already logged in");
// 		return false;
// 	}
	return true;
}





bool cClientHandle::HandleHandshake(const AString & a_Username)
{

	return CheckMultiLogin(a_Username);
}




void cClientHandle::SendData(const char * a_Data, size_t a_Size)
{
	if (m_HasSentDC)
	{
		// This could crash the client, because they've already unloaded the world etc., and suddenly a wild packet appears (#31)
		return;
	}

	cCSLock Lock(m_CSOutgoingData);
	m_OutgoingData.append(a_Data, a_Size);
}


void cClientHandle::ServerTick(float a_Dt)
{
	// Process received network data:
	AString IncomingData;
	{
		cCSLock Lock(m_CSIncomingData);
		std::swap(IncomingData, m_IncomingData);
	}
	if (!IncomingData.empty())
	{
		m_Protocol->DataReceived(IncomingData.data(), IncomingData.size());
	}
	
	// Send any queued outgoing data:
	AString OutgoingData;
	{
		cCSLock Lock(m_CSOutgoingData);
		std::swap(OutgoingData, m_OutgoingData);
	}
	if ((m_Link != nullptr) && !OutgoingData.empty())
	{
		m_Link->Send(OutgoingData.data(), OutgoingData.size());
	}	

	// If the chunk the player's in was just sent, spawn the player:
	if (m_State == csAuthenticated)
	{
		m_State = csWorking;
	}

	// Send a ping packet:
	if (m_State == csWorking)
	{
		if ((m_PingStartTime + PING_TIME_MS <= std::chrono::steady_clock::now()))
		{
			m_PingID++;
			m_PingStartTime = std::chrono::steady_clock::now();
			m_Protocol->SendKeepAlive(m_PingID);
		}
	}

	m_TicksSinceLastPacket += 1;
	if (m_TicksSinceLastPacket > 600)  // 30 seconds
	{
		SendDisconnect(ERROR_CODE_CLIENT_TIMEOUT);
	}
}


const AString & cClientHandle::GetUsername(void) const
{
	return m_Username;
}


void cClientHandle::SetUsername( const AString & a_Username)
{
	m_Username = a_Username;
}



void cClientHandle::PacketBufferFull(void)
{
	// Too much data in the incoming queue, the server is probably too busy, kick the client:
	LOGERROR("Too much data in queue for client \"%s\" @ %s, kicking them.", m_Username.c_str(), m_IPString.c_str());
	SendDisconnect(ERROR_CODE_SERVER_BUSY);
}





void cClientHandle::PacketUnknown(UInt32 a_PacketType)
{
	LOGERROR("Unknown packet type 0x%x from client \"%s\" @ %s", a_PacketType, m_Username.c_str(), m_IPString.c_str());

	AString Reason;
	Printf(Reason, "Unknown [C->S] PacketType: 0x%x", a_PacketType);
	SendDisconnect(ERROR_CODE_PACKET_UNKONWN);
}





void cClientHandle::PacketError(UInt32 a_PacketType)
{
	LOGERROR("Protocol error while parsing packet type 0x%02x; disconnecting client \"%s\"", a_PacketType, m_Username.c_str());
	SendDisconnect(ERROR_CODE_PACKET_ERROR);
}





void cClientHandle::SocketClosed(void)
{
	// The socket has been closed for any reason

	if (!m_Username.empty())  // Ignore client pings
	{
		
	}
	
	Destroy();
}




void cClientHandle::OnLinkCreated(cTCPLinkPtr a_Link)
{
	m_Link = a_Link;
}

void cClientHandle::SendDisconnect(const int & a_Reason)
{
	// Destruction (Destroy()) is called when the client disconnects, not when a disconnect packet (or anything else) is sent
	// Otherwise, the cClientHandle instance is can be unexpectedly removed from the associated player - Core/#142
	AString strReason;
	switch (a_Reason)
	{
	case  ERROR_CODE_ACCOUNT_NOT_MATCH:
		strReason = "Account and password does not match!";
		break;
	case ERROR_CODE_ACCOUNT_NOT_EXIST:
		strReason = "Account does not exist!";
		break;
	case ERROR_CODE_PACKET_ERROR:
		strReason = "Protocol error";
		break;
	case ERROR_CODE_PACKET_UNKONWN:
		strReason = "Protocol unkonwn";
		break;
	case ERROR_CODE_SERVER_BUSY:
		strReason = "Server busy";
		break;
	case ERROR_CODE_CLIENT_TIMEOUT:
		strReason = "Nooooo!! You timed out! D: Come back!";
		break;
	case  ERROR_CODE_ACCOUNT_ALREADY_LOGIN:
		strReason = "A player of the username is already logged in";
		break;
	case ERROR_CODE_ACCOUNT_OTHER_LOGIN:
		strReason = "other login";
		break;
	case ERROR_CODE_SERVER_SHUTDOWN:
		strReason = "Server shut down?";
		break;
	case ERROR_CODE_BAD_APP:
		strReason = "Bad app";
		break;
	case  ERROR_CODE_INVALID_ENCKEYLENGTH:
		strReason = "Invalid EncKeyLength";
		break;
	case  ERROR_CODE_INVALID_ENCNONCELENGTH:
		strReason = "Invalid EncNonceLength";
		break;
	case  ERROR_CODE_HACKED_CLIENT:
		strReason = "Hacked client";
		break;
	default:
		break;
	}

	if (!m_HasSentDC)
	{
		LOGD("Sending a DC: \"%s\"", StripColorCodes(strReason).c_str());
		m_Protocol->SendDisconnect(a_Reason);
		m_HasSentDC = true;
	}
}



void cClientHandle::OnReceivedData(const char * a_Data, size_t a_Length)
{
	// Reset the timeout:
	m_TicksSinceLastPacket = 0;

	// Queue the incoming data to be processed in the tick thread:
	cCSLock Lock(m_CSIncomingData);
	m_IncomingData.append(a_Data, a_Length);
}





void cClientHandle::OnRemoteClosed(void)
{
	{
		cCSLock Lock(m_CSOutgoingData);
		m_Link.reset();
	}
	SocketClosed();
}





void cClientHandle::OnError(int a_ErrorCode, const AString & a_ErrorMsg)
{
	LOGD("An error has occurred on client link for %s @ %s: %d (%s). Client disconnected.",
		m_Username.c_str(), m_IPString.c_str(), a_ErrorCode, a_ErrorMsg.c_str()
	);
	{
		cCSLock Lock(m_CSOutgoingData);
		m_Link.reset();
	}
	SocketClosed();
}
