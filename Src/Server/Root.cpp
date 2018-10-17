
#include "stdafx.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "Root.h"
#include "Server.h"
#include "CommandOutput.h"
#include "LoggerListeners.h"
#include "IniFile.h"
#include "SettingsRepositoryInterface.h"
#include "OverridesSettingsRepository.h"
//#include "Database/DBPool.h"
#include <iostream>

#ifdef _WIN32
	#include <conio.h>
	#include <psapi.h>
#elif defined(__linux__)
	#include <fstream>
#elif defined(__APPLE__)
	#include <mach/mach.h>
#endif





cRoot * cRoot::s_Root = nullptr;
bool cRoot::m_ShouldStop = false;





cRoot::cRoot(void) :
	m_Server(nullptr),
	m_bRestart(false)
{
	s_Root = this;
}





cRoot::~cRoot()
{
	if (m_Server)
	{
		delete m_Server;
		m_Server = nullptr;
	}
	s_Root = 0;
}





void cRoot::InputThread(cRoot & a_Params)
{
	cLogCommandOutputCallback Output;

	while (!cRoot::m_ShouldStop && !a_Params.m_bRestart && !m_TerminateEventRaised && std::cin.good())
	{
		AString Command;
		std::getline(std::cin, Command);
		if (!Command.empty())
		{
			a_Params.ExecuteConsoleCommand(TrimString(Command), Output);
		}
	}

	if (m_TerminateEventRaised || !std::cin.good())
	{
		// We have come here because the std::cin has received an EOF / a terminate signal has been sent, and the server is still running
		// Stop the server:
		if (!m_RunAsService)  // Dont kill if running as a service
		{
			a_Params.m_ShouldStop = true;
		}
	}
}





void cRoot::Start(std::unique_ptr<cSettingsRepositoryInterface> overridesRepo)
{
	#ifdef _WIN32
	HWND hwnd = GetConsoleWindow();
	HMENU hmenu = GetSystemMenu(hwnd, FALSE);
	EnableMenuItem(hmenu, SC_CLOSE, MF_GRAYED);  // Disable close button when starting up; it causes problems with our CTRL-CLOSE handling
	#endif

	auto consoleLogListener = MakeConsoleListener(m_RunAsService);
	auto consoleAttachment = cLogger::GetInstance().AttachListener(std::move(consoleLogListener));
	auto fileLogListenerRet = MakeFileListener();
	if (!fileLogListenerRet.first)
	{
		LOGERROR("Failed to open log file, aborting");
		return;
	}
	auto fileAttachment = cLogger::GetInstance().AttachListener(std::move(fileLogListenerRet.second));

	LOG("--- Started Log ---");

	LOG("Reading server config...");
	auto IniFile = cpp14::make_unique<cIniFile>();
	if (!IniFile->ReadFile("settings.ini"))
	{
		LOGWARN("Regenerating settings.ini, all settings will be reset");
		IniFile->AddHeaderComment(" This is the main server configuration");
}
	auto settingsRepo = cpp14::make_unique<cOverridesSettingsRepository>(std::move(IniFile), std::move(overridesRepo));

	m_ShouldStop = false;
	while (!m_ShouldStop)
	{
		auto BeginTime = std::chrono::steady_clock::now();
		m_bRestart = false;

		LoadGlobalSettings();		

		LOG("Creating new server instance...");
		m_Server = new cServer();

		LOG("Starting server...");
		bool ShouldAuthenticate = settingsRepo->GetValueSetB("Authentication", "Authenticate", true);
		if (!m_Server->InitServer(*settingsRepo, ShouldAuthenticate))
		{			
			LOGERROR("Failure starting server, aborting...");
			break;
		}

		// This sets stuff in motion
		LOGD("Starting Authenticator...");
		m_Authenticator.Start(*settingsRepo);

		LOGD("Starting ChannelManager...");
		m_ChannelManager.Start();


		settingsRepo->Flush();

		LOGD("Finalising startup...");
		if (m_Server->Start())
		{
			LOGD("Starting InputThread...");
			try
			{
				m_InputThread = std::thread(InputThread, std::ref(*this));
				m_InputThread.detach();
			}
			catch (std::system_error & a_Exception)
			{
				LOGERROR("cRoot::Start (std::thread) error %i: could not construct input thread; %s", a_Exception.code().value(), a_Exception.what());
			}

			LOG("Startup complete, took %ldms!", static_cast<long int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - BeginTime).count()));

			// Save the current time
			m_StartTime = std::chrono::steady_clock::now();

			#ifdef _WIN32
			EnableMenuItem(hmenu, SC_CLOSE, MF_ENABLED);  // Re-enable close button
			#endif

			while (!m_ShouldStop && !m_bRestart && !m_TerminateEventRaised)  // These are modified by external threads
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}

			if (m_TerminateEventRaised)
			{
				m_ShouldStop = true;
			}

			// Stop the server:

			LOG("Shutting down server...");
			m_Server->Shutdown();
		}  // if (m_Server->Start())
		else
		{
			m_ShouldStop = true;
		}


		LOGD("Shutting down deadlock detector...");

		LOGD("Stopping authenticator...");
		m_Authenticator.Stop();

		LOGD("Stopping ChannelManager...");
		m_ChannelManager.Stop();

		LOG("Cleaning up...");
		delete m_Server; m_Server = nullptr;		

		LOG("Shutdown successful!");
	}

	settingsRepo->Flush();

	LOG("--- Stopped Log ---");
}


void cRoot::LoadGlobalSettings()
{
	// Nothing needed yet
}



void cRoot::TickCommands(void)
{
	// Execute any pending commands:
	cCommandQueue PendingCommands;
	{
		cCSLock Lock(m_CSPendingCommands);
		std::swap(PendingCommands, m_PendingCommands);
	}
	for (cCommandQueue::iterator itr = PendingCommands.begin(), end = PendingCommands.end(); itr != end; ++itr)
	{
		ExecuteConsoleCommand(itr->m_Command, *(itr->m_Output));
	}
}





void cRoot::QueueExecuteConsoleCommand(const AString & a_Cmd, cCommandOutputCallback & a_Output)
{
	// Some commands are built-in:
	if (a_Cmd == "stop")
	{
		m_ShouldStop = true;
	}
	else if (a_Cmd == "restart")
	{
		m_bRestart = true;
	}

	// Put the command into a queue (Alleviates FS #363):
	cCSLock Lock(m_CSPendingCommands);
	m_PendingCommands.push_back(cCommand(a_Cmd, &a_Output));
}





void cRoot::QueueExecuteConsoleCommand(const AString & a_Cmd)
{
	// Put the command into a queue (Alleviates FS #363):
	cCSLock Lock(m_CSPendingCommands);
	m_PendingCommands.push_back(cCommand(a_Cmd, new cLogCommandDeleteSelfOutputCallback));
}





void cRoot::ExecuteConsoleCommand(const AString & a_Cmd, cCommandOutputCallback & a_Output)
{
	// cRoot handles stopping and restarting due to our access to controlling variables
	if (a_Cmd == "stop")
	{
		m_ShouldStop = true;
		return;
	}
	else if (a_Cmd == "restart")
	{
		m_bRestart = true;
		return;
	}

	LOG("Executing console command: \"%s\"", a_Cmd.c_str());
	m_Server->ExecuteConsoleCommand(a_Cmd, a_Output);
}





void cRoot::KickUser(int a_ClientID, const int & a_Reason)
{
	m_Server->KickUser(a_ClientID, a_Reason);
}





void cRoot::AuthenticateUser(int a_ClientID, const AString & a_Name, const AString & a_PassWord, const Json::Value & a_Properties)
{
	m_Server->AuthenticateUser(a_ClientID, a_Name, a_PassWord, a_Properties);
}


int cRoot::GetVirtualRAMUsage(void)
{
	#ifdef _WIN32
		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pmc, sizeof(pmc)))
		{
			return (int)(pmc.PrivateUsage / 1024);
		}
		return -1;
	#elif defined(__linux__)
		// Code adapted from http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		std::ifstream StatFile("/proc/self/status");
		if (!StatFile.good())
		{
			return -1;
		}
		while (StatFile.good())
		{
			AString Line;
			std::getline(StatFile, Line);
			if (strncmp(Line.c_str(), "VmSize:", 7) == 0)
			{
				int res = atoi(Line.c_str() + 8);
				return (res == 0) ? -1 : res;  // If parsing failed, return -1
			}
		}
		return -1;
	#elif defined (__APPLE__)
		// Code adapted from http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		struct task_basic_info t_info;
		mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

		if (KERN_SUCCESS == task_info(
			mach_task_self(),
			TASK_BASIC_INFO,
			(task_info_t)&t_info,
			&t_info_count
		))
		{
			return static_cast<int>(t_info.virtual_size / 1024);
		}
		return -1;
	#else
		LOGINFO("%s: Unknown platform, cannot query memory usage", __FUNCTION__);
		return -1;
	#endif
}





int cRoot::GetPhysicalRAMUsage(void)
{
	#ifdef _WIN32
		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		{
			return (int)(pmc.WorkingSetSize / 1024);
		}
		return -1;
	#elif defined(__linux__)
		// Code adapted from http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		std::ifstream StatFile("/proc/self/status");
		if (!StatFile.good())
		{
			return -1;
		}
		while (StatFile.good())
		{
			AString Line;
			std::getline(StatFile, Line);
			if (strncmp(Line.c_str(), "VmRSS:", 6) == 0)
			{
				int res = atoi(Line.c_str() + 7);
				return (res == 0) ? -1 : res;  // If parsing failed, return -1
			}
		}
		return -1;
	#elif defined (__APPLE__)
		// Code adapted from http://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
		struct task_basic_info t_info;
		mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

		if (KERN_SUCCESS == task_info(
			mach_task_self(),
			TASK_BASIC_INFO,
			(task_info_t)&t_info,
			&t_info_count
		))
		{
			return static_cast<int>(t_info.resident_size / 1024);
		}
		return -1;
	#else
		LOGINFO("%s: Unknown platform, cannot query memory usage", __FUNCTION__);
		return -1;
	#endif
}


