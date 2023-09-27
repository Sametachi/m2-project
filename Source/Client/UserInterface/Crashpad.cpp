/**
	@file UserInterface/Crashpad.cpp
	@date 07/09/2022
	@brief Crashpad integration for metin2
*/
#include "StdAfx.h"
#include "Crashpad.h"
#include "Version.h"
#include "CrashpadConfig.h"

#include <crashpad/client/settings.h>

#ifdef _WIN32
#include <ShlObj.h>

/// Crashpad exception handler
static LPTOP_LEVEL_EXCEPTION_FILTER g_hCrashPadExp;

static LONG WINAPI CorsacExp(EXCEPTION_POINTERS* exp)
{
	Logger::GetInstance()->Flush();
	return g_hCrashPadExp(exp);
}

#endif

CCrashpad::CCrashpad() = default;

bool CCrashpad::Start()
{
#ifdef _WIN32
	PWSTR wPath;
	if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &wPath)))
	{
		CoTaskMemFree(wPath);
		return false;
	}

	std::wstring dbDir = wPath, mtDir, heDir;
	dbDir += L"\\";
	CoTaskMemFree(wPath);
	dbDir += V_SERVER_NAME_WIDE;
	mtDir = dbDir;

	auto r = SHCreateDirectoryExW(nullptr, dbDir.c_str(), nullptr);
	if (r != ERROR_ALREADY_EXISTS && r != ERROR_SUCCESS && r != ERROR_FILE_EXISTS)
		return false;

	dbDir += L"\\chpdb";
	mtDir += L"\\chpmt";

	// avoid showing the crash box and pass it to our handler
	SetErrorMode(SEM_FAILCRITICALERRORS);

	WCHAR modPath[MAX_PATH + 1];
	if (!GetModuleFileNameW(GetModuleHandleW(nullptr), modPath, MAX_PATH))
		return false;

	const auto p = wcsrchr(modPath, L'\\');
	modPath[p - modPath] = L'\0';
	heDir = modPath;
	heDir += L"\\";
	heDir += CRASHPAD_HANDLER_NAME_W;
	heDir += L".exe";
#endif

	base::FilePath fp(heDir), db(dbDir), md(mtDir);
	std::vector<std::string> args;
	args.push_back("--no-rate-limit");
	std::map<std::string, std::string> anno;

	anno["server_id"] = V_SERVER_ID_STR;
	anno["version"] = V_VERSION_STRING;
	anno["cvs"] = GIT_NAME;
	anno["cvs_date"] = GIT_LAST_CHANGE;
	anno["build_date"] = __TIMESTAMP__;

#ifdef _WIN32
	anno["platform"] = "Windows";
#endif
#if defined(_WIN64) || defined(_M_AMD64)
	anno["arch"] = "x86_64";
#elif defined(_M_X86)
	anno["arch"] = "i386";
#endif

	//anno["token_user"] = szToken;

	/*
	m_pDb = std::move(crashpad::CrashReportDatabase::Initialize(db));
	if (!m_pDb)
		return false;

	const auto settings = m_pDb->GetSettings();
	if (!settings)
		return false;

	settings->SetUploadsEnabled(true);
	*/

	if (!m_cClient.StartHandler(fp, db, md, CRASHPAD_URL, anno, args, true, true))
		return false;

#ifdef _WIN32
	g_hCrashPadExp = SetUnhandledExceptionFilter(CorsacExp);
#endif

	return true;
}
