
#include "stdafx.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "ErrorCode.h"

#include "Authenticator.h"
#include "../Root.h"
#include "../Server.h"
#include "../ClientHandle.h"
#include "../md5.h"
#include "../IniFile.h"
#include "PolarSSL++/BlockingSslClientSocket.h"


cAuthenticator::cAuthenticator(void) :
	super("cAuthenticator")
{
}





cAuthenticator::~cAuthenticator()
{
	Stop();
}





void cAuthenticator::ReadSettings(cSettingsRepositoryInterface & a_Settings)
{

	m_Table		= a_Settings.GetValueSet("Authentication", "Table", "tb_users");
	m_Username	= a_Settings.GetValueSet("Authentication", "sername", "username");
	m_Password	= a_Settings.GetValueSet("Authentication", "Password", "password");
	m_Salt      = a_Settings.GetValueSet("Authentication", "Salt", "salt");
}





void cAuthenticator::Authenticate(int a_ClientID, const AString & a_UserName, const AString & a_PassWord)
{
	cCSLock LOCK(m_CS);
	m_Queue.push_back(cUser(a_ClientID, a_UserName, a_PassWord));
	m_QueueNonempty.Set();
}





void cAuthenticator::Start(cSettingsRepositoryInterface & a_Settings)
{
	ReadSettings(a_Settings);
	m_ShouldTerminate = false;
	super::Start();
}





void cAuthenticator::Stop(void)
{
	m_ShouldTerminate = true;
	m_QueueNonempty.Set();
	Wait();
}





void cAuthenticator::Execute(void)
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

		cAuthenticator::cUser & User = m_Queue.front();
		int ClientID = User.m_ClientID;
		AString UserName = User.m_Username;
		AString Password = User.m_Password;
		m_Queue.pop_front();
		Lock.Unlock();

		AString UUID;
		Json::Value Properties;
		int ret = AuthFromDBPoll(UserName, Password, UUID, Properties);
		if (ret == ERROR_CODE_ACCOUNT_AUTH_OK)
		{
			cRoot::Get()->AuthenticateUser(ClientID, UserName, UUID, Properties);
		}
		else
		{
			cRoot::Get()->KickUser(ClientID, ret);
		}
	}  // for (-ever)
}





int cAuthenticator::AuthFromDBPoll(AString & a_UserName, const AString & a_PassWord, AString & a_UUID, Json::Value & a_Properties)
{
 	LOGD("Trying to authenticate user %s", a_UserName.c_str());


	return a_UserName == a_PassWord;
}


