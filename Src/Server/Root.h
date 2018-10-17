
#pragma once

#include "Protocol/Authenticator.h"
#include "Channel/Manager.h"
#include <thread>





// fwd:
class cThread;
class cServer;
class cPlayer;
class cCommandOutputCallback;
class cSettingsRepositoryInterface;

namespace Json
{
	class Value;
}





/** The root of the object hierarchy */
// tolua_begin
class cRoot
{
public:
	static cRoot * Get() { return s_Root; }
	// tolua_end

	static bool m_TerminateEventRaised;
	static bool m_RunAsService;
	static bool m_ShouldStop;


	cRoot(void);
	~cRoot();

	void Start(std::unique_ptr<cSettingsRepositoryInterface> overridesRepo);

	cServer * GetServer(void) { return m_Server; }

	/** Returns the up time of the server in seconds */
	int GetServerUpTime(void)
	{
		return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_StartTime).count());
	}



	/** The current time where the startup of the server has been completed */
	std::chrono::steady_clock::time_point m_StartTime;
	cAuthenticator &   GetAuthenticator  (void) { return m_Authenticator; }
	cManager &  GetChannelManager (void) { return m_ChannelManager; }

	/** Queues a console command for execution through the cServer class.
	The command will be executed in the tick thread
	The command's output will be written to the a_Output callback
	"stop" and "restart" commands have special handling.
	*/
	void QueueExecuteConsoleCommand(const AString & a_Cmd, cCommandOutputCallback & a_Output);

	/** Queues a console command for execution through the cServer class.
	The command will be executed in the tick thread
	The command's output will be sent to console
	"stop" and "restart" commands have special handling.
	*/
	void QueueExecuteConsoleCommand(const AString & a_Cmd);  // tolua_export

	/** Executes a console command through the cServer class; does special handling for "stop" and "restart". */
	void ExecuteConsoleCommand(const AString & a_Cmd, cCommandOutputCallback & a_Output);

	/** Kicks the user, no matter in what world they are. Used from cAuthenticator */
	void KickUser(int a_ClientID, const int & a_Reason);

	/** Called by cAuthenticator to auth the specified user */
	void AuthenticateUser(int a_ClientID, const AString & a_Name, const AString & a_PassWord, const Json::Value & a_Properties);

	/** Executes commands queued in the command queue */
	void TickCommands(void);

	/** Returns the amount of virtual RAM used, in KiB. Returns a negative number on error */
	static int GetVirtualRAMUsage(void);

	/** Returns the amount of virtual RAM used, in KiB. Returns a negative number on error */
	static int GetPhysicalRAMUsage(void);

	// tolua_end

private:
	class cCommand
	{
	public:
		cCommand(const AString & a_Command, cCommandOutputCallback * a_Output) :
			m_Command(a_Command),
			m_Output(a_Output)
		{
		}

		AString m_Command;
		cCommandOutputCallback * m_Output;
	} ;

	typedef std::vector<cCommand> cCommandQueue;

	cCriticalSection m_CSPendingCommands;
	cCommandQueue    m_PendingCommands;

	std::thread m_InputThread;

	cServer *        m_Server;

	cAuthenticator     m_Authenticator;
	cManager    m_ChannelManager;

	bool m_bRestart;


	void LoadGlobalSettings();

	/** Does the actual work of executing a command */
	void DoExecuteConsoleCommand(const AString & a_Cmd);

	static cRoot * s_Root;

	static void InputThread(cRoot & a_Params);
};  // tolua_export





