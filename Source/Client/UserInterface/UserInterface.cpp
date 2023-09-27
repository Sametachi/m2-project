#include "StdAfx.h"
#include "PythonApplication.h"
#include "AbstractResources.h"
#include "PythonBindings.h"
#include "resource.h"

#include <EterLib/Util.h>
#include <EterLib/Clipboard.h>
#include <EterLib/MSWindow.h>
#include <EterBase/lzo.h>
#include <VFE/Include/VFE.hpp>
#include <Basic/SimpleApp.hpp>
#include <Basic/SimpleCmd.hpp>
#include <Basic/winvercorrect.hpp>
#include <cstdlib>
#include <ppl.h>
#include <filesystem>
#include <cxxopts.hpp>
#include <version.h>
#include <dwmapi.h>

#include "../../ThirdParty/PythonCAPI/CApiLibs.h"
#include "../../ThirdParty/ImGUI/imgui.h"
#include "Crashpad.h"

namespace fs = std::filesystem;
extern bool IsConsole = true; // TODO: change this to false

extern "C" {
	__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
	__declspec(dllexport) int32_t AmdPowerXpressRequestHighPerformance = 1;
};

// Start PyBind
extern "C" void inityit();

// Package management
std::array<std::string, 10> ActivePackages = {
	"System",
	"Actor",
	"Design",
	"Icon",
	"Item",
	"Map",
	"MapData",
	"MapEnv",
	"Seasonal",
	"Sound"
};

// Start Python engine
bool RunMainScript(CPythonLauncher& pyLauncher)
{
	if (!pyLauncher.Run())
	{
		FatalLog("Failed to initialize the Main Module.");
		SysLog("Please check the Log file for more information.", "Critical Error");
		return false;
	}

	return true;
}

class YitsoraClient : public SimpleApp
{
public:
	YitsoraClient(HINSTANCE hInstance);
	~YitsoraClient();
	int32_t Loop(int32_t argc, const char** argv);
	void SetVirtualPackages();

	HINSTANCE m_hInstance;
	CLZO lzo;

	FileSystem file_system;
	ArchiveFSProvider fs_pack;
	VirtualDiskProvider fs_disk;
	Logger m_log;
	cxxopts::ParseResult m_res;

	AbstractResources absResource;
	bool m_noloop;

	CCrashpad m_crashpad;
};

YitsoraClient::YitsoraClient(HINSTANCE hInstance) : m_hInstance(hInstance), m_log(ProviderType::Spdlog), m_noloop(false)
{
	cxxopts::Options options(V_SERVER_NAME, V_SERVER_NAME " command options");

	options.add_options()
#ifdef _DEBUG
	("p,pyhome", "Python Home Path (Required if Interpreter Mode Enabled)", cxxopts::value<std::string>())
		("V,version", "Shows version")
#endif
		("y,yeolsoe", "Yeolsoe Login System Token", cxxopts::value<std::string>())
		;

	options.allow_unrecognised_options();

	m_res = options.parse(__argc, __argv);

	if (m_res.count("version"))
	{
		printf("%s Python interpreter rev.%s (Python %s)\n", V_SERVER_NAME, V_REVISION_STR, PY_VERSION);
		m_noloop = true;
		return;
	}

	// flush log at normal app exit
	atexit([]() { Logger::GetInstance()->Flush(); });

	if (!m_crashpad.Start())
	{
		m_noloop = true;
		return;
	}

	// Create Console
	if (IsConsole)
	{
#ifndef _DEBUG
		CreateConsoleWindow();
#endif
	}

	m_log.SetCustomLogFileName("SysLog");
	m_log.SetCustomErrFileName("SysErr");

	if (!m_log.Initialize(false, 
#ifdef _DEBUG
		true, IsConsole
#endif
		))
	{
		MessageBoxW(nullptr, L"Unable to start game client", L"Fatal Error", MB_OK | MB_ICONERROR);
		throw std::runtime_error("Unable to initialize logging");
	}

#ifdef _DEBUG
	m_log.SetLevel(Log::LogLevel::Trace);
#else
	m_log.SetLevel(Log::LogLevel::Info);
#endif

	// Initialize Clipboard for EditLine
	ClipboardInit();

	// First setup VFS then set the Providers.
	SetVirtualStepFileSyster(&file_system);
	file_system.RegisterProvider(&fs_pack);

	// If we allow loading from Disk,
	// then we silence missing file warnings
	// and then @Register the pc's hdd/ssd as disk
	file_system.ReadDiskFiles(true);
	if (CanReadDiskFiles)
	{
		// Allow VFS to load files outside of Archives.
		file_system.LogMissingFiles(false);
		file_system.RegisterProvider(&fs_disk);
	}
	else
		file_system.LogMissingFiles(true);

	SetVirtualPackages();

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
}

YitsoraClient::~YitsoraClient()
{
	// Destroy everything
	SetVirtualStepFileSyster(nullptr);

	if (!m_noloop)
		ImGui::DestroyContext();
}

int32_t YitsoraClient::Loop(int32_t argc, const char** argv)
{
	if (m_noloop)
		return 0;

	// Initialize Devil
	ilInit();

	// Set Granny
	granny_log_callback Callback{};
	Callback.Function = 0;
	Callback.UserData = 0;
	GrannySetLogCallback(&Callback);
	GrannyFilterMessage(GrannyFileReadingLogMessage, false);

	CTextFileLoader::SetCacheMode();

	// Execute application
	auto app = std::make_unique<CPythonApplication>();
	app->Initialize(m_hInstance);

	bool ret = false;
	{
		CApiLibs_Init();

		CPythonLauncher pyLauncher(L"Eter", false);

#ifdef _DEBUG
		wchar_t** aArgv = (wchar_t**)malloc(sizeof(ptrdiff_t) * __argc);
		for (auto i = 0; i < __argc; i++)
		{
			aArgv[i] = (wchar_t*)malloc(sizeof(wchar_t) * (strlen(__argv[i]) + 1));
			MultiByteToWideChar(CP_ACP, 0, __argv[i], -1, aArgv[i], strlen(__argv[i]) + 1);
		}


		if (ClientConfig::GetInstance()->RunMain())
#endif
		{
			ret = RunMainScript(pyLauncher);
		}

#ifdef _DEBUG
		if (ClientConfig::GetInstance()->IsInterpreterMode())
		{

#ifdef _DEBUG
			if (!CApiLibs_TclInit())
			{
				SysLog("Tkinter init fail");
			}
#endif

			Py_Main(__argc, aArgv);
		}

		for (auto i = 0; i < __argc; i++)
		{
			free(aArgv[i]);
		}
		free(aArgv);
#endif

		app->Clear();
		pyLauncher.Clear();
		app->Destroy();
		m_log.Destroy();
	}
	app = nullptr;
	return !ret;
}

void YitsoraClient::SetVirtualPackages()
{
	CTextFileLoader::SetCacheMode();

	for (const std::string& ArchiveName : ActivePackages)
	{
		fs_pack.AddArchive(ArchiveName, "Data/", true);
	}
}

extern "C" int32_t APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int32_t nCmdShow)
{
	// Check Windows versions..
	if (!IsWindows7SP1OrGreater()) // Target SP1 or Greater
	{
		MessageBoxA(nullptr, "This application requires Windows 7 Service Pack 1 or Greater.", "Yitsora Online", MB_OK | MB_ICONERROR);
		return false;
	}

	YitsoraClient yitInstance(hInstance);

	int32_t loop_status = yitInstance.Loop(__argc, const_cast<const char**>(__argv));
	// Remove the Logger on application exit
	TraceLog("Application closed.");
	return loop_status;
}

#ifdef _DEBUG
int main(int argc, char** argv)
{
	std::string args = "";

	for (auto i = 0; i < argc; i++)
		args += argv[i] + std::string(" ");

	args.erase(args.size() - 1);
	return WinMain(GetModuleHandle(nullptr), nullptr, (LPSTR)args.c_str(), SW_SHOW);
}
#endif
