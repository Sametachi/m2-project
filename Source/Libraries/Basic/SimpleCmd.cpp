#include "SimpleCmd.hpp"
#include <cassert>
#include <storm/WindowsPlatform.hpp>
#include <iostream>
#include <cstdio>

//#ifdef _DEBUG
namespace
{
	ScopedConsoleCloseHandler* globalInstance = nullptr;
	
	BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType)
	{
		switch (ctrlType) 
		{
			case CTRL_C_EVENT:
			case CTRL_BREAK_EVENT:
			case CTRL_CLOSE_EVENT:
			case CTRL_SHUTDOWN_EVENT:
				(*globalInstance)();
				return TRUE;
	
			default:
				return FALSE;
		}
	}
}

bool CreateConsoleWindow()
{
	if (!AllocConsole())
		return false;

	HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);

	CONSOLE_SCREEN_BUFFER_INFO screenInfo;
	if (!GetConsoleScreenBufferInfo(outputHandle, &screenInfo))
		return false;

	if (screenInfo.dwSize.Y < 600)
		screenInfo.dwSize.Y = 600;

	if (screenInfo.dwSize.X < 600)
		screenInfo.dwSize.X = 600;

	if (!SetConsoleScreenBufferSize(outputHandle, screenInfo.dwSize))
		return false;

	std::freopen("CONOUT$", "w", stdout);
	std::freopen("CONOUT$", "w", stderr);

	std::setvbuf(stderr, NULL, _IONBF, 0);
	std::setvbuf(stdout, NULL, _IONBF, 0);

	std::ios::sync_with_stdio();

	HWND hwnd = ::GetConsoleWindow();
	if (hwnd != NULL)
	{
		HMENU hMenu = ::GetSystemMenu(hwnd, FALSE);
		if (hMenu != NULL) DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
	}
	return true;
}

ScopedConsoleCloseHandler::ScopedConsoleCloseHandler(std::function<void()> h) : m_handler(std::move(h))
{
	assert(globalInstance == nullptr && "Another handler is already registered.");
	globalInstance = this;
	SetConsoleCtrlHandler(&ConsoleCtrlHandler, TRUE);
}

ScopedConsoleCloseHandler::~ScopedConsoleCloseHandler()
{
	SetConsoleCtrlHandler(&ConsoleCtrlHandler, FALSE);
	globalInstance = nullptr;
}

void ScopedConsoleCloseHandler::operator()()
{
	m_handler();
}
//#endif