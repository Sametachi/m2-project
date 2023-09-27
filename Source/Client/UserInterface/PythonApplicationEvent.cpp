#include "StdAfx.h"
#include "PythonApplication.h"
#include <EterLib/Camera.h>
#include "../../ThirdParty/ImGUI/imgui.h"

void CPythonApplication::OnCameraUpdate()
{
	if (m_pyBackground.IsMapReady())
	{
		CCamera* pkCameraMgr = CCameraManager::GetInstance()->GetCurrentCamera();
		if (pkCameraMgr)
			pkCameraMgr->Update();
	}
}

void CPythonApplication::OnUIUpdate()
{
	m_kWndMgr.Update();
}

void CPythonApplication::OnUIRender()
{
	m_kWndMgr.Render();
}

void CPythonApplication::OnSizeChange(int32_t width, int32_t height)
{

}

void CPythonApplication::OnResuming()
{

}

void CPythonApplication::OnSuspending()
{
	//__ResetCameraWhenMinimize();
}

void CPythonApplication::OnWindowMoved()
{
}

void CPythonApplication::OnActivated()
{
}

void CPythonApplication::OnDeactivated()
{
}

void CPythonApplication::OnMouseMiddleButtonDown(int32_t x, int32_t y)
{
	CCamera* pkCmrCur = CCameraManager::GetInstance()->GetCurrentCamera();
	if (pkCmrCur)
		pkCmrCur->BeginDrag(x, y);

	if (!m_pyBackground.IsMapReady())
		return;

	SetCursorNum(CAMERA_ROTATE);
	if (CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(false, true);
}

void CPythonApplication::OnMouseMiddleButtonUp(int32_t x, int32_t y)
{
	CCamera* pkCmrCur = CCameraManager::GetInstance()->GetCurrentCamera();
	if (pkCmrCur)
		pkCmrCur->EndDrag();

	if (!m_pyBackground.IsMapReady())
		return;

	SetCursorNum(NORMAL);
	if (CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(true);
}

void CPythonApplication::OnMouseWheel(int32_t nLen)
{
	if (!(CWindowManager::GetInstance()->RunMouseWheelEvent(nLen)))
	{
		CCamera* pkCmrCur = CCameraManager::GetInstance()->GetCurrentCamera();
		if (pkCmrCur)
			pkCmrCur->Wheel(nLen);
	}
}

void CPythonApplication::OnMouseMove(int32_t x, int32_t y)
{
	CCamera* pkCmrCur = CCameraManager::GetInstance()->GetCurrentCamera();

	POINT Point;
	if (pkCmrCur)
	{
		if (CPythonBackground::GetInstance()->IsMapReady() && pkCmrCur->Drag(x, y, &Point))
		{
			x = Point.x;
			y = Point.y;
			ClientToScreen(m_hWnd, &Point);
			SetCursorPos(Point.x, Point.y);
		}
	}

	RECT rcWnd;
	GetClientRect(&rcWnd);

	m_kWndMgr.SetResolution(rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top);
	m_kWndMgr.RunMouseMove(x, y);
}

void CPythonApplication::OnMouseLeftButtonDown(int32_t x, int32_t y)
{
	m_kWndMgr.RunMouseMove(x, y);
	m_kWndMgr.RunMouseLeftButtonDown();
}

void CPythonApplication::OnMouseLeftButtonUp(int32_t x, int32_t y)
{
	m_kWndMgr.RunMouseMove(x, y);
	m_kWndMgr.RunMouseLeftButtonUp();
}

void CPythonApplication::OnMouseLeftButtonDoubleClick(int32_t x, int32_t y)
{
	m_kWndMgr.RunMouseMove(x, y);
	m_kWndMgr.RunMouseLeftButtonDoubleClick();
}

void CPythonApplication::OnMouseRightButtonDown(int32_t x, int32_t y)
{
	m_kWndMgr.RunMouseMove(x, y);
	m_kWndMgr.RunMouseRightButtonDown();
}

void CPythonApplication::OnMouseRightButtonUp(int32_t x, int32_t y)
{
	m_kWndMgr.RunMouseMove(x, y);
	m_kWndMgr.RunMouseRightButtonUp();
}

void CPythonApplication::OnChar(uint32_t ch)
{
	m_kWndMgr.RunChar(ch);
}

void CPythonApplication::OnKeyDown(KeyCode code)
{
	m_keyboard.OnKeyDown(code);

	//if (code == kVirtualKeyTab)
	//{
	//	m_kWndMgr.RunTab();
	//
	//	const auto rkPlayer = CPythonPlayer::InstancePtr();
	//	if (rkPlayer)
	//	{
	//		const auto pkInstTarget = CPythonCharacterManager::Instance().GetTabNextTargetPointer(rkPlayer->NEW_GetMainActorPtr());
	//		if (pkInstTarget)
	//			rkPlayer->SetTarget(pkInstTarget->GetVirtualID(), true);
	//	}
	//}
	if (code == kVirtualKeyEscape)
	{
		m_kWndMgr.RunPressEscapeKey();
	}

	m_kWndMgr.RunKeyDown(code);
}

void CPythonApplication::OnKeyUp(KeyCode code)
{
	m_keyboard.OnKeyUp(code);
	m_kWndMgr.RunKeyUp(code);
}

void CPythonApplication::RunPressExitKey()
{
	m_kWndMgr.RunPressExitKey();
}

void CPythonApplication::OnMouseUpdate()
{
	auto [lx, ly] = m_kWndMgr.GetMousePosition();
	PyCallClassMemberFunc(m_poMouseHandler, "Update", lx, ly);
}

void CPythonApplication::OnMouseRender()
{
	PyCallClassMemberFunc(m_poMouseHandler, "Render");
}
