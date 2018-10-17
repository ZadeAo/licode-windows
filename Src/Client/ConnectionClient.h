
#pragma once
#include "interface.h"
#include "ByteBuffer.h"
#include "PolarSSL++/AesCfb128Decryptor.h"
#include "PolarSSL++/AesCfb128Encryptor.h"




class ConnectionClient
{
	friend class cPacketizer;

public:
	ConnectionClient();
	virtual ~ConnectionClient();

public:
	void RegisterObserver(ConnectionClientObserver* callback);
	bool is_connected() const;

	bool Init(const AString& a_Server, const int &a_Port, const AString &a_UserName);
	bool UnInit();

	UInt32 GetID();
	AString GetChannel();

	void ConnectToServer();
	bool SendToChannel(const AString &room, const UInt32 &a_ClientID, const AString &a_Msg);
	void JoinChannel(const AString &server, const AString &room);
	void LeaveChannel(const AString &room);
	bool SendHangUp(int peer_id);
	void SignOut();

	LRESULT OnNetwork(WPARAM wParam, LPARAM lParam);	

protected:
	void SendData(const char *a_Data, size_t a_Size);

	void DataReceived(const char *a_Data, size_t a_Size);

	BOOL SetTimeout(int nTime, BOOL bRecv);

	void OnConnect(SOCKET sock);

	bool OnRead(SOCKET sock);

	void OnClose(SOCKET sock);

	void AddReceivedData(const char * a_Data, size_t a_Size);

	bool HandlePacket(cByteBuffer &a_ByteBuffer, UInt32 a_PacketType);
private:

	void HandlePacketStatusRequest(cByteBuffer & a_ByteBuffer);

	void HandlePacketLoginDisconnect(cByteBuffer & a_ByteBuffer);
	void HandlePacketLoginEncryptionKeyRequest(cByteBuffer & a_ByteBuffer);
	void HandlePacketLoginAllow(cByteBuffer & a_ByteBuffer);
	void HandlePacketLoginSuccess(cByteBuffer & a_ByteBuffer);

	void HandlePacketKeepAlive(cByteBuffer & a_ByteBuffer);
	void HandlePacketRoomJoin(cByteBuffer & a_ByteBuffer);
	void HandlePacketRoomLeft(cByteBuffer & a_ByteBuffer);
	void HandlePacketChannelMsg(cByteBuffer & a_ByteBuffer);
	void HandlePacketErrorCode(cByteBuffer & a_ByteBuffer);

private:
	void SendEncryptionKeyResponse(const string & a_PublicKey, const UInt32 & a_Nonce);
	void HandleErrorCode(const int &a_Code);
	
protected:
	
	ConnectionClientObserver* callback_;

	SOCKET			sock_;
	AString			server_;
	short			port_;

	/** Buffer for the received data */
	cByteBuffer m_ReceivedData;

	cCriticalSection m_CSPacket;
	/** Buffer for composing the outgoing packets, through cPacketizer */
	cByteBuffer m_OutPacketBuffer;

	/** State of the protocol. 1 = status, 2 = login, 3 = work */
	UInt32 m_State;

	bool				m_IsEncrypted;
	bool				m_bConnect;
	UInt32				m_ClientID;

	cAesCfb128Decryptor *m_Decryptor;
	cAesCfb128Encryptor *m_Encryptor;

	AString				m_rUserName;
	AString				m_rChannel;
};

