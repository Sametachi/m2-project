/**
	@file UserInterface/Crashpad.h
	@date 07/09/2022
	@brief Crashpad integration for metin2
*/
#pragma once

#include <crashpad/client/crashpad_client.h>
#include <crashpad/client/crash_report_database.h>

class CCrashpad
{
public:
	CCrashpad();

	bool Start();

private:
	/// Crashpad client
	crashpad::CrashpadClient m_cClient;

	/// Crashpad database
	std::unique_ptr<crashpad::CrashReportDatabase> m_pDb;
};
