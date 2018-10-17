
// Protocol17x.cpp

/*
Implements the 1.7.x protocol classes:
	- cProtocol_impl
		- release 1.7.2 protocol (#4)
	- cProtocol176
		- release 1.7.6 protocol (#5)
*/

#include "stdafx.h"
#include "cProtocol_impl.h"
#include "PolarSSL++/Sha1Checksum.h"
#include "Packetizer.h"

#include "../ClientHandle.h"
#include "../Root.h"
#include "../Server.h"


#include "func.h"
#include "ErrorCode.h"

#include "Channel/Manager.h"


/** The slot number that the client uses to indicate "outside the window". */
static const Int16 SLOT_NUM_OUTSIDE = -999;





#define HANDLE_READ(ByteBuf, Proc, Type, Var) \
	Type Var; \
	if (!ByteBuf.Proc(Var))\
	{\
		return;\
	}





#define HANDLE_PACKET_READ(ByteBuf, Proc, Type, Var) \
	Type Var; \
	{ \
		if (!ByteBuf.Proc(Var)) \
		{ \
			ByteBuf.CheckValid(); \
			return false; \
		} \
		ByteBuf.CheckValid(); \
	}





const int MAX_ENC_LEN = 512;  // Maximum size of the encrypted message; should be 128, but who knows...





// fwd: main.cpp:
extern bool g_ShouldLogCommIn, g_ShouldLogCommOut;





////////////////////////////////////////////////////////////////////////////////
// cProtocol_impl:

cProtocol_impl::cProtocol_impl(cClientHandle * a_Client) :
	super(a_Client),
	m_ReceivedData(64 KiB),
	m_IsEncrypted(false),
	m_State(1)
{
	
}

cProtocol_impl::~cProtocol_impl()
{
	cRoot::Get()->GetChannelManager().doUserOffline(m_Client->GetUniqueID());
}


bool cProtocol_impl::HandlePacket(cByteBuffer & a_ByteBuffer, UInt32 a_PacketType)
{
	switch (m_State)
	{
	case 1:
	{
		// Status
		switch (a_PacketType)
		{
		case 0x00: HandlePacketStatusRequest(a_ByteBuffer); return true;
		case 0x01: HandlePacketStatusPing(a_ByteBuffer); return true;
		}
		break;
	}

	case 2:
	{
		// Login
		switch (a_PacketType)
		{
		case 0x00: HandlePacketLoginStart(a_ByteBuffer); return true;
		case 0x01: HandlePacketLoginEncryptionResponse(a_ByteBuffer); return true;
		case 0x02: HandlePacketLoginInfo(a_ByteBuffer); return true;
		}
		break;
	}
	case 3:
	{
		// Work
		switch (a_PacketType)
		{
		case 0x00: HandlePacketKeepAlive(a_ByteBuffer); return true;
		case 0x01: HandlePacketJoinRoom(a_ByteBuffer); return true;
		case 0x02: HandlePacketMsgRoom(a_ByteBuffer); return true;
		case 0x09: HandlePacketLeftRoom(a_ByteBuffer); return true;
		}
		break;
	}
	default:
	{
		// Received a packet in an unknown state, report:
		LOGWARNING("Received a packet in an unknown protocol state %d. Ignoring further packets.", m_State);

		// Cannot kick the client - we don't know this state and thus the packet number for the kick packet

		// Switch to a state when all further packets are silently ignored:
		m_State = 255;
		return false;
	}
	case 255:
	{
		// This is the state used for "not processing packets anymore" when we receive a bad packet from a client.
		// Do not output anything (the caller will do that for us), just return failure
		return false;
	}
	}  // switch (m_State)

	// Unknown packet type, report to the ClientHandle:
	m_Client->PacketUnknown(a_PacketType);
	return false;
}

void cProtocol_impl::HandlePacketStatusRequest(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, appName);
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, server);
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, int, port);
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, int, state);
	if (appName != "mcu" || state < 1 || state >2)
	{
		m_Client->Kick(ERROR_CODE_BAD_APP);
		return;
	}
	//下一个状态
	m_State = state;  //wait for client login 

	cPacketizer pkg(*this, 0x00); 	//StatusRequest
	pkg.WriteBEInt32(m_State);		//state
	pkg.WriteBool(1);			//zip
	pkg.WriteBool(1);			//encrypt
	pkg.WriteString(m_Client->GetIPString());

}

void cProtocol_impl::HandlePacketStatusPing(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadBEInt64, Int64, Timestamp);

	cPacketizer Pkt(*this, 0x01);  // Ping packet
	Pkt.WriteBEInt64(Timestamp);
}

void cProtocol_impl::HandlePacketLoginStart(cByteBuffer & a_ByteBuffer)
{
	cServer *Server = cRoot::Get()->GetServer();

	cPacketizer Pkt(*this, 0x01);
	Pkt.WriteString(Server->GetServerID());
	const AString & PubKeyDer = Server->GetPublicKeyDER();
	Pkt.WriteBEInt16(static_cast<UInt16>(PubKeyDer.size()));
	Pkt.WriteBuf(PubKeyDer.data(), PubKeyDer.size());
	Pkt.WriteBEInt16(4);
	// Using 'this' as the cryptographic nonce, so that we don't have to generate one each time :)
	Pkt.WriteBEInt32(static_cast<Int32>(reinterpret_cast<uintptr_t>(this)));

	return;
}

void cProtocol_impl::HandlePacketLoginEncryptionResponse(cByteBuffer & a_ByteBuffer)
{
	Int16 EncKeyLength, EncNonceLength;
	if (!a_ByteBuffer.ReadBEInt16(EncKeyLength))
	{
		return;
	}
	if (EncKeyLength > MAX_ENC_LEN)
	{
		LOGD("Invalid Encryption Key length: %u (0x%04x). Kicking client.", EncKeyLength, EncKeyLength);
		m_Client->Kick(ERROR_CODE_INVALID_ENCKEYLENGTH);
		return;
	}
	AString EncKey;
	if (!a_ByteBuffer.ReadString(EncKey, EncKeyLength))
	{
		return;
	}
	if (!a_ByteBuffer.ReadBEInt16(EncNonceLength))
	{
		return;
	}
	if (EncNonceLength > MAX_ENC_LEN)
	{
		LOGD("Invalid Encryption Nonce length: %u (0x%04x). Kicking client.", EncNonceLength, EncNonceLength);
		m_Client->Kick(ERROR_CODE_INVALID_ENCNONCELENGTH);
		return;
	}
	AString EncNonce;
	if (!a_ByteBuffer.ReadString(EncNonce, EncNonceLength))
	{
		return;
	}

	// Decrypt EncNonce using privkey
	cRsaPrivateKey & rsaDecryptor = cRoot::Get()->GetServer()->GetPrivateKey();
	UInt32 DecryptedNonce[MAX_ENC_LEN / sizeof(UInt32)];
	int res = rsaDecryptor.Decrypt(
		reinterpret_cast<const Byte *>(EncNonce.data()), EncNonce.size(),
		reinterpret_cast<Byte *>(DecryptedNonce), sizeof(DecryptedNonce)
		);
	if (res != 4)
	{
		LOGD("Bad nonce length: got %d, exp %d", res, 4);
		m_Client->Kick(ERROR_CODE_HACKED_CLIENT);
		return;
	}
	if (DecryptedNonce[0] != static_cast<UInt32>(reinterpret_cast<uintptr_t>(this)))
	{
		LOGD("Bad nonce value");
		m_Client->Kick(ERROR_CODE_HACKED_CLIENT);
		return;
	}

	// Decrypt the symmetric encryption key using privkey:
	Byte DecryptedKey[MAX_ENC_LEN];
	res = rsaDecryptor.Decrypt(
		reinterpret_cast<const Byte *>(EncKey.data()), EncKey.size(),
		DecryptedKey, sizeof(DecryptedKey)
		);
	if (res != 16)
	{
		LOGD("Bad key length");
		m_Client->Kick(ERROR_CODE_HACKED_CLIENT);
		return;
	}

	//success
	StartEncryption(DecryptedKey);

	cPacketizer Pkt(*this, 0x02);// loginAllow
}

void cProtocol_impl::HandlePacketLoginInfo(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, username);
	m_Client->HandleLogin(username, username);

}

void cProtocol_impl::HandlePacketKeepAlive(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, KeepAliveID);
	m_Client->HandleKeepAlive(KeepAliveID);
}

void cProtocol_impl::HandlePacketJoinRoom(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channelName);

	cRoot::Get()->GetChannelManager().doUserJoinChannel(m_Client->GetUniqueID(), channelName);
}

void cProtocol_impl::HandlePacketLeftRoom(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channelName);
	cRoot::Get()->GetChannelManager().doUserLeftChannel(m_Client->GetUniqueID(), channelName);
}

void cProtocol_impl::HandlePacketMsgRoom(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channelName);
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, TargetID);
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, msg);

	cRoot::Get()->GetChannelManager().OnMsgFromPeer(channelName, m_Client->GetUniqueID(), TargetID, msg);
}


void cProtocol_impl::SendKeepAlive(UInt32 a_PingID)
{
	// Drop the packet if the protocol is not in the Game state yet (caused a client crash):
	if (m_State != 3)
	{
		LOGWARNING("Trying to send a KeepAlive packet to a player who's not yet fully logged in (%d). The protocol class prevented the packet.", m_State);
		return;
	}

	cPacketizer Pkt(*this, 0x00);  // Keep Alive packet
	Pkt.WriteVarInt32(a_PingID);
}

void cProtocol_impl::SendData(const char * a_Data, size_t a_Size)
{
	if (m_IsEncrypted)
	{
		Byte Encrypted[8192];  // Larger buffer, we may be sending lots of data (chunks)
		while (a_Size > 0)
		{
			size_t NumBytes = (a_Size > sizeof(Encrypted)) ? sizeof(Encrypted) : a_Size;
			m_Encryptor.ProcessData(Encrypted, (Byte *)a_Data, NumBytes);
			m_Client->SendData((const char *)Encrypted, NumBytes);
			a_Size -= NumBytes;
			a_Data += NumBytes;
		}
	}
	else
	{
		m_Client->SendData(a_Data, a_Size);
	}
}

void cProtocol_impl::SendPacket(cPacketizer & a_Packet)
{
	AString LenToSend;
	AString DataToSend;

	// Send the packet length
	UInt32 PacketLen = static_cast<UInt32>(m_OutPacketBuffer.GetUsedSpace());

	m_OutPacketLenBuffer.WriteVarInt32(PacketLen);
	m_OutPacketLenBuffer.ReadAll(LenToSend);
	m_OutPacketLenBuffer.CommitRead();

	// Send the packet data:
	m_OutPacketBuffer.ReadAll(DataToSend);

	DataToSend = LenToSend + DataToSend;
	SendData(DataToSend.data(), DataToSend.size());
	m_OutPacketBuffer.CommitRead();
}

void cProtocol_impl::SendDisconnect(const int & a_Reason)
{
	switch (m_State)
	{
	case 2:
	{
		// During login:
		cPacketizer Pkt(*this, 0);
		Pkt.WriteBEInt32(a_Reason);
		break;
	}
	case 3:
	{
		// In-Work:
		cPacketizer Pkt(*this, 0x40);
		Pkt.WriteBEUInt32(a_Reason);
		break;
	}
	}
}

void cProtocol_impl::DataReceived(const char * a_Data, size_t a_Size)
{
	if (m_IsEncrypted)
	{
		Byte Decrypted[512];
		while (a_Size > 0)
		{
			size_t NumBytes = (a_Size > sizeof(Decrypted)) ? sizeof(Decrypted) : a_Size;
			m_Decryptor.ProcessData(Decrypted, (Byte *)a_Data, NumBytes);
			AddReceivedData((const char *)Decrypted, NumBytes);
			a_Size -= NumBytes;
			a_Data += NumBytes;
		}
	}
	else
	{
		AddReceivedData(a_Data, a_Size);
	}
}

void cProtocol_impl::AddReceivedData(const char * a_Data, size_t a_Size)
{
	if (!m_ReceivedData.Write(a_Data, a_Size))
	{
		// Too much data in the incoming queue, report to caller:
		m_Client->PacketBufferFull();
		return;
	}

	// Handle all complete packets:
	for (;;)
	{
		UInt32 PacketLen;
		if (!m_ReceivedData.ReadVarInt(PacketLen))
		{
			// Not enough data
			m_ReceivedData.ResetRead();
			break;
		}
		if (!m_ReceivedData.CanReadBytes(PacketLen))
		{
			// The full packet hasn't been received yet
			m_ReceivedData.ResetRead();
			break;
		}
		cByteBuffer bb(PacketLen + 1);
		VERIFY(m_ReceivedData.ReadToByteBuffer(bb, static_cast<size_t>(PacketLen)));
		m_ReceivedData.CommitRead();

		UInt32 PacketType;
		if (!bb.ReadVarInt(PacketType))
		{
			// Not enough data
			break;
		}

		// Write one NUL extra, so that we can detect over-reads
		bb.Write("\0", 1);


		if (!HandlePacket(bb, PacketType))
		{
			// Unknown packet, already been reported, but without the length. Log the length here:
			LOGWARNING("Unhandled packet: type 0x%x, state %d, length %u", PacketType, m_State, PacketLen);

#ifdef _DEBUG
			// Dump the packet contents into the log:
			bb.ResetRead();
			AString Packet;
			bb.ReadAll(Packet);
			Packet.resize(Packet.size() - 1);  // Drop the final NUL pushed there for over-read detection
			AString Out;
			CreateHexDump(Out, Packet.data(), Packet.size(), 24);
			LOGD("Packet contents:\n%s", Out.c_str());
#endif  // _DEBUG


			return;
		}
		int sizes = bb.GetReadableSpace();
		if (sizes != 1)
		{
			// Read more or less than packet length, report as error
			LOGWARNING("Protocol: Wrong number of bytes read for packet 0x%x, state %d. Read " SIZE_T_FMT " bytes, packet contained %u bytes",
				PacketType, m_State, bb.GetUsedSpace() - bb.GetReadableSpace(), PacketLen
				);

			ASSERT(!"Read wrong number of bytes!");
			m_Client->PacketError(PacketType);
		}
	}  // for (ever)
}


void cProtocol_impl::StartEncryption(const Byte * a_Key)
{
	m_Encryptor.Init(a_Key, a_Key);
	m_Decryptor.Init(a_Key, a_Key);
	m_IsEncrypted = true;
}

void cProtocol_impl::SendLoginSuccess(void)
{
	ASSERT(m_State == 2);  // State: login?

	{
		cPacketizer Pkt(*this, 0x03);  // Login success 
		Pkt.WriteVarInt32(m_Client->GetUniqueID());
	}

	m_State = 3;  // State = Work
}

AString cProtocol_impl::GetName()
{
	return m_Client->GetUsername();
}

AString cProtocol_impl::GetIPString()
{
	return m_Client->GetIPString();
}

void cProtocol_impl::onJoinChannelResult(const AString &a_Channel, const int & a_Result)
{
	//返回加入房间结果
	cPacketizer Pkt(*this, 0x01);
	Pkt.WriteString(a_Channel);
	Pkt.WriteVarInt32(a_Result);
}

void cProtocol_impl::onLeaveChannelResult(const AString &a_Channel, const int & a_Result)
{
	//返回离开房间结果
	cPacketizer Pkt(*this, 0x09);
	Pkt.WriteString(a_Channel);
	Pkt.WriteVarInt32(a_Result);
}

void cProtocol_impl::onChannelMsg(const int &a_FromID, const AString &a_Channel, const AString & a_Msg)
{
	//返回房间消息
	cPacketizer Pkt(*this, 0x02);
	Pkt.WriteVarInt32(a_FromID);
	Pkt.WriteString(a_Channel);
	Pkt.WriteString(a_Msg);
}