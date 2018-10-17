
// cClientHandle.h

// Interfaces to the cClientHandle class representing a client connected to this server. The client need not be a player yet





#pragma once

#include "OSSupport/Network.h"
#include "ByteBuffer.h"
#include "json/json.h"


#include <array>
#include <atomic>


// fwd:
class cProtocol;
class cFallingBlock;
class cItemHandler;
class cClientHandle;
typedef std::shared_ptr<cClientHandle> cClientHandlePtr;
class cRoom;




class cClientHandle  // tolua_export
	: public cTCPLink::cCallbacks
{  // tolua_export
public:  // tolua_export


	/** Creates a new client with the specified IP address in its description and the specified initial view distance. */
	cClientHandle(const AString & a_IPString);

	virtual ~cClientHandle();

	const AString & GetIPString(void) const { return m_IPString; }  // tolua_export
	
	/** Sets the IP string that the client is using. Overrides the IP string that was read from the socket.
	Used mainly by BungeeCord compatibility code. */
	void SetIPString(const AString & a_IPString) { m_IPString = a_IPString; }

	/** Returns the player's UUID, as used by the protocol, in the short form (no dashes) */
	const AString & GetUUID(void) const { return m_UUID; }  // tolua_export
	

	void Kick(const int & a_Reason);

	/** Authenticates the specified user, called by cAuthenticator */
	void Authenticate(const AString & a_Name, const AString & a_UUID, const Json::Value & a_Properties);

	void onJoinChannelResult(const AString &a_Channel, const int & a_Result);

	void onLeaveChannelResult(const AString &a_Channel, const int & a_Result);

	void onChannelMsg(const AString &a_Channel, const int &a_FromID, const AString &a_Msg);

	
	inline bool IsLoggedIn(void) const { return (m_State >= csAuthenticating); }


	/** Called while the client is being ticked from the cServer object */
	void ServerTick(float a_Dt);

	void Destroy(void);
	
	bool IsWorking(void) const { return (m_State == csWorking); }
	bool IsDestroyed (void) const { return (m_State == csDestroyed); }
	bool IsDestroying(void) const { return (m_State == csDestroying); }

	// The following functions send the various packets:
	// (Please keep these alpha-sorted)

	// tolua_begin
	const AString & GetUsername(void) const;
	void SetUsername( const AString & a_Username);
	
	inline short GetPing(void) const { return static_cast<short>(std::chrono::duration_cast<std::chrono::milliseconds>(m_Ping).count()); }
	

	int GetUniqueID(void) const { return m_UniqueID; }
	

	
	// Calls that cProtocol descendants use to report state:
	void PacketBufferFull(void);
	void PacketUnknown(UInt32 a_PacketType);
	void PacketError(UInt32 a_PacketType);

	void SendDisconnect(const int & a_Reason);

	/** Kicks the client if the same username is already logged in.
	Returns false if the client has been kicked, true otherwise. */
	bool CheckMultiLogin(const AString & a_Username);
	
	/** Called when the protocol handshake has been received (for protocol versions that support it;
	otherwise the first instant when a username is received).
	Returns true if the player is to be let in, false if they were disconnected
	*/
	bool HandleHandshake        (const AString & a_Username);
	
	void HandleKeepAlive        (int a_KeepAliveID);
	
	void HandlePing             (void);

	/** Called when the protocol has finished logging the user in.
	Return true to allow the user in; false to kick them.
	*/
	bool HandleLogin(const AString & a_Username, const AString & a_Password);

	void SendData(const char * a_Data, size_t a_Size);
	
private:

	friend class cServer;  // Needs access to SetSelf()


	/** The type used for storing the names of registered plugin channels. */
	typedef std::set<AString> cChannels;

	AString m_IPString;

	AString m_Username;
	AString m_Password;
	
	cProtocol * m_Protocol;

	/** Protects m_IncomingData against multithreaded access. */
	cCriticalSection m_CSIncomingData;

	/** Queue for the incoming data received on the link until it is processed in Tick().
	Protected by m_CSIncomingData. */
	AString m_IncomingData;

	/** Protects m_OutgoingData against multithreaded access. */
	cCriticalSection m_CSOutgoingData;

	/** Buffer for storing outgoing data from any thread; will get sent in Tick() (to prevent deadlocks).
	Protected by m_CSOutgoingData. */
	AString m_OutgoingData;

	bool m_HasSentDC;  ///< True if a Disconnect packet has been sent in either direction

	/** Number of ticks since the last network packet was received (increased in Tick(), reset in OnReceivedData()) */
	int m_TicksSinceLastPacket;
	
	/** Duration of the last completed client ping. */
	std::chrono::steady_clock::duration m_Ping;

	/** ID of the last ping request sent to the client. */
	int m_PingID;

	/** Time of the last ping request sent to the client. */
	std::chrono::steady_clock::time_point m_PingStartTime;

	enum eState
	{
		csConnected,         ///< The client has just connected, waiting for their handshake / login
		csAuthenticating,    ///< The client has logged in, waiting for external authentication
		csAuthenticated,     ///< The client has been authenticated, will start streaming chunks in the next tick
		csWorking,           ///< Normal gameplay
		csDestroying,        ///< The client is being destroyed, don't queue any more packets / don't add to chunks
		csDestroyed,         ///< The client has been destroyed, the destructor is to be called from the owner thread
		
		// TODO: Add Kicking here as well
	} ;
	
	std::atomic<eState> m_State;
	
	/** m_State needs to be locked in the Destroy() function so that the destruction code doesn't run twice on two different threads */
	cCriticalSection m_CSDestroyingState;


	static int s_ClientCount;
	
	/** ID used for identification during authenticating. Assigned sequentially for each new instance. */
	int m_UniqueID;
	
	/** Contains the UUID used by Mojang to identify the player's account. Short UUID stored here (without dashes) */
	AString m_UUID;
		

	/** The link that is used for network communication.
	m_CSOutgoingData is used to synchronize access for sending data. */
	cTCPLinkPtr m_Link;

	/** Called when the network socket has been closed. */
	void SocketClosed(void);

	// cTCPLink::cCallbacks overrides:
	virtual void OnLinkCreated(cTCPLinkPtr a_Link) override;
	virtual void OnReceivedData(const char * a_Data, size_t a_Length) override;
	virtual void OnRemoteClosed(void) override;
	virtual void OnError(int a_ErrorCode, const AString & a_ErrorMsg) override;
};  // tolua_export





