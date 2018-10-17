
// cAuthenticator.h

// Interfaces to the cAuthenticator class representing the thread that authenticates users against the official MC server
// Authentication prevents "hackers" from joining with an arbitrary username (possibly impersonating the server admins)
// For more info, see http://wiki.vg/Session#Server_operation
// In MCS, authentication is implemented as a single thread that receives queued auth requests and dispatches them one by one.





#pragma once

#include "../OSSupport/IsThread.h"

class cSettingsRepositoryInterface;



// fwd: "cRoot.h"
class cRoot;

namespace Json
{
	class Value;
}





class cAuthenticator :
	public cIsThread
{
	typedef cIsThread super;

public:
	cAuthenticator(void);
	~cAuthenticator();

	/** (Re-)read server and address from INI: */
	void ReadSettings(cSettingsRepositoryInterface & a_Settings);

	/** Queues a request for authenticating a user. If the auth fails, the user will be kicked */
	void Authenticate(int a_ClientID, const AString & a_UserName, const AString & a_ServerHash);

	/** Starts the authenticator thread. The thread may be started and stopped repeatedly */
	void Start(cSettingsRepositoryInterface & a_Settings);

	/** Stops the authenticator thread. The thread may be started and stopped repeatedly */
	void Stop(void);
	
private:

	class cUser
	{
	public:
		int     m_ClientID;
		AString m_Username;
		AString m_Password;

		cUser(int a_ClientID, const AString & a_Username, const AString & a_Password) :
			m_ClientID(a_ClientID),
			m_Username(a_Username),
			m_Password(a_Password)
		{
		}
	};

	typedef std::deque<cUser> cUserList;

	cCriticalSection m_CS;
	cUserList        m_Queue;
	cEvent           m_QueueNonempty;


	AString m_PropertiesAddress;

	AString m_Table;
	AString m_Username;
	AString m_Password;
	AString m_Salt;


	/** cIsThread override: */
	virtual void Execute(void) override;

	/** Returns true if the user authenticated okay, false on error
	Returns the case-corrected username, UUID, and properties (eg. skin). */
	int AuthFromDBPoll(AString & a_UserName, const AString & a_PassWord, AString & a_UUID, Json::Value & a_Properties);
};




