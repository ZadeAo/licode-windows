
// Protocol17x.h



#pragma once

#include "Protocol.h"
#include "../ByteBuffer.h"

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable:4127)
	#pragma warning(disable:4244)
	#pragma warning(disable:4231)
	#pragma warning(disable:4189)
	#pragma warning(disable:4702)
#endif

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

#include "PolarSSL++/AesCfb128Decryptor.h"
#include "PolarSSL++/AesCfb128Encryptor.h"





// fwd:
namespace Json
{
	class Value;
}





class cProtocol_impl :
	public cProtocol
{
	typedef cProtocol super;
	
public:

	cProtocol_impl(cClientHandle * a_Client);
	~cProtocol_impl();


	/** Reads and handles the packet. The packet length and type have already been read.
	Returns true if the packet was understood, false if it was an unknown packet
	*/
	virtual bool HandlePacket(cByteBuffer & a_ByteBuffer, UInt32 a_PacketType);
	/** Called when client sends some data: */
	virtual void DataReceived(const char * a_Data, size_t a_Size) override;
	/** Sending stuff to clients (alphabetically sorted): */
	virtual void SendDisconnect(const int & a_Reason);
	virtual void SendLoginSuccess(void);
	virtual void SendKeepAlive(UInt32 a_PingID);


	virtual void onJoinChannelResult(const AString &a_Channel, const int & a_Result);

	virtual void onLeaveChannelResult(const AString &a_Channel, const int & a_Result);

	virtual void onChannelMsg(const int &a_FromID, const AString &a_Channel, const AString & a_Msg);

	virtual AString GetName();
	virtual AString GetIPString();

	enum eConnectionState
	{
		csUnencrypted,           // The connection is not encrypted. Packets must be decoded in order to be able to start decryption.
		csEncryptedUnderstood,   // The communication is encrypted and so far all packets have been understood, so they can be still decoded
		csEncryptedUnknown,      // The communication is encrypted, but an unknown packet has been received, so packets cannot be decoded anymore
		csWaitingForEncryption,  // The communication is waiting for the other line to establish encryption
	};
	
protected:
	// Packet handlers while in the Status state (m_State == 1):
	void HandlePacketStatusPing(cByteBuffer & a_ByteBuffer);

	// Packet handlers while in the Login state (m_State == 2):
	void HandlePacketLoginEncryptionResponse(cByteBuffer & a_ByteBuffer);
	void HandlePacketLoginStart(cByteBuffer & a_ByteBuffer);
	
	// Packet handlers while in the Game state (m_State == 3):
	void HandlePacketStatusRequest(cByteBuffer & a_ByteBuffer);
	void HandlePacketLoginInfo(cByteBuffer & a_ByteBuffer);

	void HandlePacketKeepAlive(cByteBuffer & a_ByteBuffer);
	void HandlePacketJoinRoom(cByteBuffer & a_ByteBuffer);	
	void HandlePacketMsgRoom(cByteBuffer & a_ByteBuffer);
	void HandlePacketLeftRoom(cByteBuffer & a_ByteBuffer);
	void StartEncryption(const Byte * a_Key);

private:
	
	/** Adds the received (unencrypted) data to m_ReceivedData, parses complete packets */
	void AddReceivedData(const char * a_Data, size_t a_Size);

	/** Sends the data to the client, encrypting them if needed. */
	virtual void SendData(const char * a_Data, size_t a_Size) override;

	/** Sends the packet to the client. Called by the cPacketizer's destructor. */
	virtual void SendPacket(cPacketizer & a_Packet) override;	

protected:

	AString m_ServerAddress;

	UInt16 m_ServerPort;

	/** State of the protocol. 1 = status, 2 = login, 3 = game */
	UInt32 m_State;

	/** Buffer for the received data */
	cByteBuffer m_ReceivedData;

	bool m_IsEncrypted;

	cAesCfb128Decryptor m_Decryptor;
	cAesCfb128Encryptor m_Encryptor;

} ;


