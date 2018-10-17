
// Protocol.h

// Interfaces to the cProtocol class representing the generic interface that a protocol
// parser and serializer must implement





#pragma once

#include "../Endianness.h"
#include "../ByteBuffer.h"

#include <array>



class cPacketizer;
class cClientHandle;
class cRoom;
typedef unsigned char Byte;





class cProtocol
{
public:
	cProtocol(cClientHandle * a_Client) :
		m_Client(a_Client),
		m_OutPacketBuffer(64 KiB),
		m_OutPacketLenBuffer(20)  // 20 bytes is more than enough for one VarInt
	{
	}

	virtual ~cProtocol() {}
	
	/** Called when client sends some data */
	virtual void DataReceived(const char * a_Data, size_t a_Size) = 0;
	// Sending stuff to clients (alphabetically sorted):
	virtual void SendDisconnect(const int & a_Reason) = 0;
	virtual void SendLoginSuccess(void) = 0;
	virtual void SendKeepAlive(UInt32 a_PingID) = 0;
	virtual AString GetName() = 0;

	virtual void onJoinChannelResult(const AString &a_Channel, const int & a_Result) = 0;

	virtual void onLeaveChannelResult(const AString &a_Channel, const int & a_Result) = 0;

	virtual void onChannelMsg(const int &a_FromID, const AString &a_Channel, const AString & a_Msg) = 0;

	virtual AString GetIPString() = 0;

protected:
	friend class cPacketizer;

	cClientHandle * m_Client;

	/** Provides synchronization for sending the entire packet at once.
	Each SendXYZ() function must acquire this CS in order to send the whole packet at once.
	Automated via cPacketizer class. */
	cCriticalSection m_CSPacket;

	/** Buffer for composing the outgoing packets, through cPacketizer */
	cByteBuffer m_OutPacketBuffer;
	
	/** Buffer for composing packet length (so that each cPacketizer instance doesn't allocate a new cPacketBuffer) */
	cByteBuffer m_OutPacketLenBuffer;
	
	/** A generic data-sending routine, all outgoing packet data needs to be routed through this so that descendants may override it. */
	virtual void SendData(const char * a_Data, size_t a_Size) = 0;

	/** Sends a single packet contained within the cPacketizer class.
	The cPacketizer's destructor calls this to send the contained packet; protocol may transform the data (compression in 1.8 etc). */
	virtual void SendPacket(cPacketizer & a_Packet) = 0;
} ;





