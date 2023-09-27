#include "StdAfx.h"
#include "PythonApplication.h"

#include <Basic/winvercorrect.hpp>
#include <Eterlib/Camera.h>
#include <winuser.h>
#include <dwmapi.h>
#include "../../ThirdParty/ImGUI/imgui.h"

static int32_t gs_nMouseCaptureRef = 0;

void CPythonApplication::SafeSetCapture()
{
	SetCapture(m_hWnd);
	gs_nMouseCaptureRef++;
}

void CPythonApplication::SafeReleaseCapture()
{
	gs_nMouseCaptureRef--;
	if (gs_nMouseCaptureRef == 0)
		ReleaseCapture();
}

void CPythonApplication::__SetFullScreenWindow(HWND hWnd, uint32_t dwWidth, uint32_t dwHeight, uint32_t dwBPP)
{
	DEVMODE DevMode{};
	DevMode.dmSize = sizeof(DevMode);
	DevMode.dmBitsPerPel = dwBPP;
	DevMode.dmPelsWidth = dwWidth;
	DevMode.dmPelsHeight = dwHeight;
	DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	int32_t Error = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
	if (Error == DISP_CHANGE_RESTART)
		ChangeDisplaySettings(nullptr, 0);
}

void CPythonApplication::__MinimizeFullScreenWindow(HWND hWnd, uint32_t dwWidth, uint32_t dwHeight)
{
	ChangeDisplaySettings(nullptr, 0);
	SetWindowPos(hWnd, nullptr, 0, 0, dwWidth, dwHeight, SWP_SHOWWINDOW);
	ShowWindow(hWnd, SW_MINIMIZE);
}

void CPythonApplication::__ResetCameraWhenMinimize()
{
	CCamera* pkCmrCur = CCameraManager::GetInstance()->GetCurrentCamera();

	if (pkCmrCur)
		pkCmrCur->EndDrag();

	SetCursorNum(NORMAL);
	if (GetCursorVisible() && CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(true, true);
}

LRESULT CPythonApplication::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if (IsWindows8Point1OrGreater())
	{
		BOOL isDarkMode = ClientConfig::GetInstance()->IsDarkMode();
		bool SetDarkMode = SUCCEEDED(DwmSetWindowAttribute(hWnd, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE, &isDarkMode, sizeof(isDarkMode)));
	}

	// Setup DearImGui
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureKeyboard || io.WantCaptureMouse)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, uiMsg, wParam, lParam))
			return true;
	}

	static bool s_in_sizemove = false;
	static bool s_in_suspend = false;
	static bool s_minimized = true;
	static bool s_fullscreen = false;

	switch (uiMsg)
	{
	case WM_PAINT:
		if (s_in_sizemove)
		{
			Process();
		}
		else
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
			EndPaint(hWnd, &ps);
		}
		break;

	case WM_MOVE:
		OnWindowMoved();
		break;

	case WM_ACTIVATEAPP:
	{
		m_isActivateWnd = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);

		if (m_isActivateWnd)
		{
			if (m_isWindowFullScreenEnable)
			{
				__SetFullScreenWindow(hWnd, m_dwWidth, m_dwHeight, m_pySystem.GetBPP());
			}

			KeyboardInput& keyboard = Engine::GetKeyboardInput();
			for (KeyCode i = 0; i < std::numeric_limits<KeyCode>::max(); i++)
			{
				if (keyboard.IsKeyPressed(i))
					OnKeyUp(i);
			}

			m_SoundManager.RestoreVolume();

		}
		else
		{
			if (m_isWindowFullScreenEnable)
			{
				__MinimizeFullScreenWindow(hWnd, m_dwWidth, m_dwHeight);
			}

			m_SoundManager.SaveVolume();
		}
		if (m_isActivateWnd)
		{
			OnActivated();
		}
		else
		{
			OnDeactivated();
		}

		break;
	}

	case WM_CHAR:
		OnChar(uint32_t(wParam));
		return 0;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		OnKeyDown(int8_t(LOWORD(wParam)));
		return 0;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		OnKeyUp(int8_t(LOWORD(wParam)));
		return 0;

	case WM_LBUTTONDOWN:
		SafeSetCapture();
		OnMouseLeftButtonDown(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		return 0;

	case WM_LBUTTONUP:
		if (hWnd == GetCapture())
		{
			SafeReleaseCapture();
			OnMouseLeftButtonUp(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		SafeSetCapture();
		OnMouseLeftButtonDoubleClick(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		return 0;

	case WM_MBUTTONDOWN:
		SafeSetCapture();
		OnMouseMiddleButtonDown(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		break;

	case WM_MBUTTONUP:
		if (GetCapture() == hWnd)
		{
			SafeReleaseCapture();
			OnMouseMiddleButtonUp(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		}
		break;

	case WM_RBUTTONDOWN:
		SafeSetCapture();
		OnMouseRightButtonDown(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		return 0;

	case WM_RBUTTONUP:
		if (hWnd == GetCapture())
		{
			SafeReleaseCapture();
			OnMouseRightButtonUp(int16_t(LOWORD(lParam)), int16_t(HIWORD(lParam)));
		}
		return 0;

		//case 0x20a:
	case WM_MOUSEWHEEL:
		OnMouseWheel(int16_t(HIWORD(wParam)));
		break;

	case WM_EXITSIZEMOVE:
	{
		RECT rcWnd;
		GetClientRect(&rcWnd);

		uint32_t uWidth = rcWnd.right - rcWnd.left;
		uint32_t uHeight = rcWnd.bottom - rcWnd.left;
		m_grpDevice.ResizeBackBuffer(uWidth, uHeight);
		//OnSizeChange(int32_t(LOWORD(lParam)), int32_t(HIWORD(lParam)));
	}
	break;

	case WM_SETCURSOR:
		if (IsActive())
		{
			if (m_bCursorVisible && CURSOR_MODE_HARDWARE == m_iCursorMode)
			{
				SetCursor(static_cast<HCURSOR>(m_hCurrentCursor));
				return 0;
			}


			SetCursor(nullptr);
			return 0;
		}
		break;

	case WM_SIZE:
		switch (wParam)
		{
		case SIZE_RESTORED:
		{
			if (!s_minimized)
			{
				RECT rcWnd;
				GetClientRect(&rcWnd);

				uint32_t uWidth = rcWnd.right - rcWnd.left;
				uint32_t uHeight = rcWnd.bottom - rcWnd.left;

				//Engine::GetSettings().SetForceResolution(true);
				s_minimized = true;
				m_grpDevice.ResizeBackBuffer(uWidth, uHeight);
				//Engine::GetSettings().AdjustPlayArea();
				//AdjustClientPosition();
			}
		}
		break;

		case SIZE_MAXIMIZED:
		{
			RECT rcWnd;
			GetClientRect(&rcWnd);

			uint32_t uWidth = rcWnd.right - rcWnd.left;
			uint32_t uHeight = rcWnd.bottom - rcWnd.left;

			//Engine::GetSettings().SetForceResolution(true);
			s_minimized = false;
			m_grpDevice.ResizeBackBuffer(uWidth, uHeight);
		}
		break;
		}

		if (wParam == SIZE_MINIMIZED)
		{
			if (!m_isMinimizedWnd)
			{
				m_isMinimizedWnd = true;
				if (!s_in_suspend)
					OnSuspending();
				s_in_suspend = true;
			}
		}
		else if (m_isMinimizedWnd)
		{
			m_isMinimizedWnd = false;
			if (s_in_suspend)
				OnResuming();
			s_in_suspend = false;
		}

		//OnSizeChange(int32_t(LOWORD(lParam)), int32_t(HIWORD(lParam)));
		break;

	case WM_CLOSE:
#ifdef _DEBUG
		PostQuitMessage(0);
#else	
		RunPressExitKey();
#endif
		return 0;

	case WM_DESTROY:
		return 0;

	case WM_MENUCHAR:
		return MAKELRESULT(0, MNC_CLOSE);

	default:
		break;
	}

	return CMSApplication::WindowProcedure(hWnd, uiMsg, wParam, lParam);
}
