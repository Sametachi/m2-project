#include "StdAfx.h"
#include "PythonApplication.h"
#include "resource.h"
#ifndef LoadImage
#define LoadImage LoadImageA
#endif

bool CPythonApplication::CreateCursors()
{
	m_bCursorVisible = TRUE;
	m_bLiarCursorOn = false;

	int32_t ResourceID[CURSOR_COUNT] =
	{
		IDC_CURSOR_NORMAL,
		IDC_CURSOR_ATTACK,
		IDC_CURSOR_ATTACK,
		IDC_CURSOR_TALK,
		IDC_CURSOR_NO,
		IDC_CURSOR_PICK,

		IDC_CURSOR_DOOR,
		IDC_CURSOR_CHAIR,
		IDC_CURSOR_CHAIR,			// Magic
		IDC_CURSOR_BUY,				// Buy
		IDC_CURSOR_SELL,			// Sell

		IDC_CURSOR_CAMERA_ROTATE,	// Camera Rotate
		IDC_CURSOR_HSIZE,			// Horizontal Size
		IDC_CURSOR_VSIZE,			// Vertical Size
		IDC_CURSOR_HVSIZE,			// Horizontal & Vertical Size
	};

	m_CursorHandleMap.clear();

	for (int32_t i = 0; i < CURSOR_COUNT; ++i)
	{
		HANDLE hCursor = LoadImage(ms_hInstance, MAKEINTRESOURCE(ResourceID[i]), IMAGE_CURSOR, m_pySystem.GetMouseSize(), m_pySystem.GetMouseSize(), LR_VGACOLOR);

		if (nullptr == hCursor)
		{
			SysLog("CreateCursor fail! Image: {0} Error: {1}", ResourceID[i], GetLastError());
			return false;
		}

		m_CursorHandleMap.emplace(i, hCursor);
	}

	return true;
}

void CPythonApplication::DestroyCursors()
{
	for (auto& itor : m_CursorHandleMap)
		DestroyCursor(static_cast<HCURSOR>(itor.second));
}

void CPythonApplication::SetCursorVisible(bool bFlag, bool bLiarCursorOn)
{
	m_bCursorVisible = bFlag;
	m_bLiarCursorOn = bLiarCursorOn;

	if (CURSOR_MODE_HARDWARE == m_iCursorMode)
	{
		int32_t iShowNum{ 0 };
		if (false == m_bCursorVisible)
		{
			do
			{
				iShowNum = ShowCursor(m_bCursorVisible);
			} while (iShowNum >= 0);
		}
		else
		{
			do
			{
				iShowNum = ShowCursor(m_bCursorVisible);
			} while (iShowNum < 0);
		}
	}
}

bool CPythonApplication::GetCursorVisible()
{
	return m_bCursorVisible;
}

bool CPythonApplication::GetLiarCursorOn()
{
	return m_bLiarCursorOn;
}

int32_t CPythonApplication::GetCursorMode()
{
	return m_iCursorMode;
}

bool CPythonApplication::__IsContinuousChangeTypeCursor(int32_t iCursorNum)
{
	switch (iCursorNum)
	{
	case CURSOR_SHAPE_NORMAL:
	case CURSOR_SHAPE_ATTACK:
	case CURSOR_SHAPE_TARGET:
	case CURSOR_SHAPE_MAGIC:
	case CURSOR_SHAPE_BUY:
	case CURSOR_SHAPE_SELL:
		return true;
		break;
	}

	return false;
}

bool CPythonApplication::SetCursorNum(int32_t iCursorNum)
{
	if (CURSOR_SHAPE_NORMAL == iCursorNum)
	{
		if (!__IsContinuousChangeTypeCursor(m_iCursorNum))
			iCursorNum = m_iContinuousCursorNum;
	}
	else
	{
		if (__IsContinuousChangeTypeCursor(m_iCursorNum))
		{
			m_iContinuousCursorNum = m_iCursorNum;
		}
	}

	if (CURSOR_MODE_HARDWARE == m_iCursorMode)
	{
		auto itor = m_CursorHandleMap.find(iCursorNum);
		if (m_CursorHandleMap.end() == itor)
			return FALSE;

		auto hCursor = static_cast<HCURSOR>(itor->second);

		SetCursor(hCursor);
		m_hCurrentCursor = hCursor;
	}

	m_iCursorNum = iCursorNum;

	PyCallClassMemberFunc(m_poMouseHandler, "ChangeCursor", m_iCursorNum);
	return true;
}

void CPythonApplication::SetCursorMode(int32_t iMode)
{
	m_iCursorMode = CURSOR_MODE_HARDWARE;
	ShowCursor(true);
}
