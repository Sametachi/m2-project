#include "StdAfx.h"
#include "MSApplication.h"

#pragma optimize("", off)
CMSApplication::CMSApplication() = default;

CMSApplication::~CMSApplication()
{
}

void CMSApplication::Initialize(HINSTANCE hInstance)
{
	ms_hInstance = hInstance;
}

void CMSApplication::MessageLoop()
{
	while (MessageProcess());
}

bool CMSApplication::IsMessage()
{
	MSG msg;

	if (!PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
		return false;

	return true;
}

bool CMSApplication::MessageProcess()
{
	MSG msg;

	if (!GetMessage(&msg, nullptr, 0, 0))
		return false;

	TranslateMessage(&msg);
	DispatchMessage(&msg);
	return true;
}

LRESULT CMSApplication::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uiMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	}

	return CMSWindow::WindowProcedure(hWnd, uiMsg, wParam, lParam);
}
#pragma optimize("", on)
