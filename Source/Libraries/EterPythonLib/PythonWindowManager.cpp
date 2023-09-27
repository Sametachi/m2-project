#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonGridSlotWindow.h"
#include "PythonWindowManager.h"
#include "../EterLib/StateManager.h"
#include <Storm/StringFlags.hpp>
#include <boost/range/adaptor/reversed.hpp>

namespace
{
	class CLayer : public CWindow
	{
	public:
		CLayer() : CWindow() {}
		virtual ~CLayer() {}
		bool IsWindow() { return false; }
	};

	storm::StringValueTable<uint32_t> kLayers[] =
	{
		{"GAME", kWindowLayerGame},
		{"UI_BOTTOM", kWindowLayerUiBottom},
		{"UI", kWindowLayerUi},
		{"TOP_MOST", kWindowLayerTopMost},
		{"CURTAIN", kWindowLayerCurtain},
	};

	template <typename Function>
	CWindow* TraverseWindowChainUp(CWindow* start, Function& f)
	{
		while (start && start->IsWindow())
		{
			if (f(start))
				return start;

			start = start->GetParent();
		}

		return nullptr;
	}

	template <typename Function>
	CWindow* TraverseWindowChainDown(CWindow* start, Function& f)
	{
		if (f(start))
			return start;

		for (CWindow* child : boost::adaptors::reverse(start->GetChildren()))
		{
			if (!child->IsShow())
				continue;

			child = TraverseWindowChainDown(child, f);
			if (child)
				return child;
		}

		return nullptr;
	}

	// Check whether |parent| is a direct or indirect parent of |start| or equivalent to |start|.
	bool IsParentOf(CWindow* start, CWindow* parent)
	{
		auto focus = [parent](CWindow* pWin) -> bool
		{
			return pWin == parent;
		};

		return !!TraverseWindowChainUp(start, focus);
	}

	CWindow* FindWindowUpwardsByFlag(CWindow* start, uint32_t flags)
	{
		auto focus = [flags](CWindow* pWin) -> bool
		{
			return pWin->IsFlag(flags);
		};

		return TraverseWindowChainUp(start, focus);
	}
	CWindow* FindClippingWindowUpwards(CWindow* start)
	{
		auto focus = [](CWindow* pWin) -> bool
		{
			return pWin->HasClippingEnabled();
		};

		return TraverseWindowChainUp(start, focus);
	}

	bool LiesEntirelyOutsideRect(const RECT& r, const RECT& r2)
	{
		return (r2.left > r.right || r2.right < r.left || r2.top > r.bottom || r2.bottom < r.top);
	}
}

/////////////////////////////////////////
CWindowManager::CWindowManager()
	: m_lWidth(0), m_lHeight(0), m_iVres(0), m_iHres(0),
	m_lMouseX(0), m_lMouseY(0), m_lDragX(0), m_lDragY(0),
	m_lPickedX(0), m_lPickedY(0),
	m_bAttachingFlag(false), m_dwAttachingType(0), m_dwAttachingIndex(0), m_dwAttachingSlotNumber(0), m_dwAttachingRealSlotNumber(0), m_byAttachingIconWidth(0), m_byAttachingIconHeight(0),
	m_pFocusWindow(nullptr),
	m_pLockWindow(nullptr),
	m_pPointWindow(nullptr),
	m_pLeftCaptureWindow(nullptr),
	m_pRightCaptureWindow(nullptr),
	m_pMiddleCaptureWindow(nullptr),
	m_pAppWindow(nullptr),
	m_pCaptureWindow(nullptr),
	m_pRootWindow(new CWindow())
{
	m_pRootWindow->SetName("root");
	m_pRootWindow->Show();

	for (uint32_t layer = 0; layer < kWindowLayerMax; layer++)
	{
		storm::StringRef name;
		storm::FormatValueWithTable(name, layer, kLayers);

		CWindow* pLayer = new CLayer();
		pLayer->SetName(name.data());
		pLayer->Show();

		m_pRootWindow->AddChild(pLayer);
	}

	m_FocusWindowList.clear();
	m_LockWindowList.clear();
	m_CaptureWindowList.clear();
}

CWindowManager::~CWindowManager()
{
	for (auto pLayer : m_pRootWindow->GetChildren())
		delete pLayer;
}

void CWindowManager::Destroy()
{
	EventDeletionQueue();
}

void CWindowManager::SetMouseHandler(pybind11::handle poMouseHandler)
{
	m_poMouseHandler = poMouseHandler;
}

void CWindowManager::DestroyWindow(CWindow* pWin)
{
	//PyLog("Destruction of {0}", pWin->GetName());
	if (m_lazyDeleteQueue.end() != std::find(m_lazyDeleteQueue.begin(), m_lazyDeleteQueue.end(), pWin))
	{
		PyLog("Double-destruction of {0}", pWin->GetName());
		return;
	}
	//pWin->OnDestroy();
	pWin->Hide();
	NotifyHideWindow(pWin);

	if (pWin->HasParent())
	{
		CWindow* pParent = pWin->GetParent();
		pParent->DeleteChild(pWin);
	}

	pWin->Clear();
	m_lazyDeleteQueue.emplace_back(pWin);
}

bool CWindowManager::IsDragging()
{
	return abs(m_lMouseX - m_lPickedX) + abs(m_lMouseY - m_lPickedY) > 10;
}

bool CWindowManager::IsAttaching()
{
	return m_bAttachingFlag;
}

uint32_t CWindowManager::GetAttachingType()
{
	return m_dwAttachingType;
}

uint32_t CWindowManager::GetAttachingIndex()
{
	return m_dwAttachingIndex;
}

uint32_t CWindowManager::GetAttachingSlotNumber()
{
	return m_dwAttachingSlotNumber;
}

uint32_t CWindowManager::GetAttachingRealSlotNumber()
{
	return m_dwAttachingRealSlotNumber;
}

void CWindowManager::GetAttachingIconSize(uint8_t* pbyWidth, uint8_t* pbyHeight)
{
	*pbyWidth = m_byAttachingIconWidth;
	*pbyHeight = m_byAttachingIconHeight;
}

void CWindowManager::AttachIcon(uint32_t dwType, uint32_t dwIndex, uint32_t dwSlotNumber, uint8_t byWidth, uint8_t byHeight)
{
	m_bAttachingFlag = true;
	m_dwAttachingType = dwType;
	m_dwAttachingIndex = dwIndex;
	m_dwAttachingSlotNumber = dwSlotNumber;

	m_byAttachingIconWidth = byWidth;
	m_byAttachingIconHeight = byHeight;
}

void CWindowManager::SetAttachingFlag(bool bFlag)
{
	m_bAttachingFlag = bFlag;
}

void CWindowManager::SetAttachingRealSlotNumber(uint32_t dwRealSlotNumber)
{
	m_dwAttachingRealSlotNumber = dwRealSlotNumber;
}

void CWindowManager::DeattachIcon()
{
	SetAttachingFlag(false);
	PyCallClassMemberFunc(m_poMouseHandler, "DeattachObject");
}

void CWindowManager::SetParent(CWindow* pWindow, CWindow* pParentWindow)
{
	if (!pWindow)
	{
		assert(!"CWindowManager::SetParent - There is no self window!");
		return;
	}
	if (!pParentWindow)
	{
		assert(!"There is no parent window");
		return;
	}

	if (pWindow->HasParent())
	{
		CWindow* pOldParentWindow = pWindow->GetParent();

		if (pParentWindow == pOldParentWindow)
			return;

		pOldParentWindow->DeleteChild(pWindow);
	}

	pParentWindow->AddChild(pWindow);
	pWindow->UpdateRect();
}

void CWindowManager::LockWindow(CWindow* pWin)
{
	if (m_pLockWindow == pWin)
		return;

	if (m_pLockWindow)
	{
		// Make sure we don't end up with multiple entries in the locking history.
		m_LockWindowList.remove(m_pLockWindow);
		m_LockWindowList.emplace_back(m_pLockWindow);
	}

	// Only child windows of pWin may be active at this time.
	if (pWin && m_pLockWindow && !IsParentOf(m_pLockWindow, pWin))
		SetFocus(nullptr);

	m_pLockWindow = pWin;
	SetPointedWindowByPos(m_lMouseX, m_lMouseY);
}

void CWindowManager::UnlockWindow()
{
	if (!m_pLockWindow)
		return;

	if (m_LockWindowList.empty())
	{
		m_pLockWindow = nullptr;
		KillFocus();
	}
	else
	{
		m_pLockWindow = m_LockWindowList.back();
		m_LockWindowList.pop_back();
	}

	SetPointedWindowByPos(m_lMouseX, m_lMouseY);
}

void CWindowManager::SetFocus(CWindow* pWin)
{
	if (pWin == m_pFocusWindow)
		return;

	if (m_pFocusWindow)
	{
		m_FocusWindowList.remove(m_pFocusWindow);
		m_FocusWindowList.emplace_back(m_pFocusWindow);
		m_pFocusWindow->OnKillFocus();
	}

	m_pFocusWindow = pWin;

	if (m_pFocusWindow)
	{
		/*
			Activating a window will automatically move it to the top.This has to happen recursively, because its parent might
			be partially occluded by another window (and so on...)
		*/
		SetTopRecursive(m_pFocusWindow);
		m_pFocusWindow->OnSetFocus();
	}
}

void CWindowManager::KillFocus()
{
	if (!m_pFocusWindow)
		return;

	if (m_FocusWindowList.empty())
	{
		m_pFocusWindow->OnKillFocus();
		m_pFocusWindow = nullptr;
	}
	else
	{
		m_pFocusWindow->OnKillFocus();

		m_pFocusWindow = m_FocusWindowList.back();
		m_FocusWindowList.pop_back();

		m_pFocusWindow->OnSetFocus();
	}
}

void CWindowManager::CaptureMouse(CWindow* pWin)
{
	if (pWin == m_pCaptureWindow)
		return;

	if (m_pCaptureWindow)
	{
		m_CaptureWindowList.remove(m_pCaptureWindow);
		m_CaptureWindowList.emplace_back(m_pCaptureWindow);
	}

	m_pCaptureWindow = pWin;
}

void CWindowManager::ReleaseMouse(CWindow* win)
{
	if (!win || m_pCaptureWindow == win)
	{
		ReleaseMouse();
		return;
	}

	m_CaptureWindowList.remove(win);
}

void CWindowManager::ReleaseMouse()
{
	if (!m_pCaptureWindow)
		return;

	if (m_CaptureWindowList.empty())
	{
		m_pCaptureWindow = nullptr;
	}
	else
	{
		m_pCaptureWindow = m_CaptureWindowList.back();
		m_CaptureWindowList.pop_back();
	}
}

void CWindowManager::SetTop(CWindow* pWin)
{
	CWindow* pParent = pWin->GetParent();
	if (!pParent)
		return;

	pParent->SetTop(pWin);
}

void CWindowManager::SetResolution(int32_t hres, int32_t vres)
{
	if (hres <= 0 || vres <= 0)
		return;

	m_iHres = hres;
	m_iVres = vres;
}

float CWindowManager::GetAspect()
{
	return m_iHres / float(m_iVres);
}

void CWindowManager::SetScreenSize(int32_t lWidth, int32_t lHeight)
{
	m_lWidth = lWidth;
	m_lHeight = lHeight;

	m_pRootWindow->SetSize(lWidth, lHeight);

	for (CWindow* pLayer : m_pRootWindow->GetChildren())
		pLayer->SetSize(lWidth, lHeight);
}

void CWindowManager::EventDeletionQueue()
{
	for (const CWindow* pWin : m_lazyDeleteQueue)
		delete pWin;

	m_lazyDeleteQueue.clear();
}

void CWindowManager::Update()
{
	EventDeletionQueue();
	m_pRootWindow->Update();
}

void CWindowManager::Render()
{
	m_pRootWindow->Render();
}

CWindow* CWindowManager::PickWindow(int32_t x, int32_t y)
{
	if (m_pLockWindow)
		return m_pLockWindow->PickWindow(x, y);

	CWindow* pWin = nullptr;

	if (m_pCaptureWindow && m_pCaptureWindow->IsIn(x, y))
	{
		pWin = m_pCaptureWindow->PickWindow(x, y);
		if (pWin)
			return pWin;
	}

	for (CWindow* pLayer : boost::adaptors::reverse(m_pRootWindow->GetChildren()))
	{
		pWin = pLayer->PickWindow(x, y);
		if (pWin != pLayer)
			return pWin;
	}

	return nullptr;
}

void CWindowManager::SetPointedWindowByPos(int32_t x, int32_t y)
{
	auto mouseOut = [](CWindow* pWinO) -> bool
	{
		pWinO->OnMouseOverOut();
		return false;
	};

	auto mouseIn = [](CWindow* pWinI) -> bool
	{
		pWinI->OnMouseOverIn();
		return false;
	};

	CWindow* pWin = PickWindow(x, y);
	if (m_pPointWindow != pWin)
	{
		if (m_pPointWindow)
			TraverseWindowChainUp(m_pPointWindow, mouseOut);

		m_pPointWindow = pWin;

		if (m_pPointWindow)
			TraverseWindowChainUp(m_pPointWindow, mouseIn);
	}
}

std::tuple<int32_t, int32_t> CWindowManager::GetMousePosition()
{
	return std::make_tuple(m_lMouseX, m_lMouseY);
}

void CWindowManager::RunMouseMove(int32_t x, int32_t y)
{
	if (m_iHres == 0 || m_iVres == 0)
		return;

	if (IsAttaching())
	{
		if (x > m_lWidth)
			x = m_lWidth;
		if (y > m_lHeight)
			y = m_lHeight;
	}

	x = m_lWidth * x / m_iHres;
	y = m_lHeight * y / m_iVres;

	SetPointedWindowByPos(x, y);

	if (m_lMouseX == x && m_lMouseY == y)
		return;

	m_lMouseX = x;
	m_lMouseY = y;

	// Callbacks from SetPosition() could change the capture-window, we need to grab a reference here to avoid

	CWindow* captureWnd = m_pCaptureWindow;

	if (captureWnd)
	{
		if (captureWnd->IsFlag(FLAG_MOVABLE))
		{
			int32_t x = m_lMouseX - m_lDragX;
			int32_t y = m_lMouseY - m_lDragY;

			if (captureWnd->HasParent())
			{
				const auto& r = captureWnd->GetParent()->GetRect();
				x -= r.left;
				y -= r.top;
			}

			int32_t lx = captureWnd->GetPositionX();
			int32_t ly = captureWnd->GetPositionY();

			if (captureWnd->IsFlag(FLAG_RESTRICT_X))
				x = lx;

			if (captureWnd->IsFlag(FLAG_RESTRICT_Y))
				y = ly;

			captureWnd->SetPosition(x, y);
			captureWnd->OnMoveWindow(x, y);
		}
		else if (captureWnd->IsFlag(FLAG_DRAGABLE))
		{
			int32_t x = m_lMouseX - m_lDragX;
			int32_t y = m_lMouseY - m_lDragY;

			if (captureWnd->HasParent())
			{
				const auto& r = captureWnd->GetParent()->GetRect();
				x -= r.left;
				y -= r.top;
			}

			int32_t lx = captureWnd->GetPositionX();
			int32_t ly = captureWnd->GetPositionY();

			captureWnd->OnMouseDrag(x, y);
		}
	}

	if (m_pPointWindow)
		m_pPointWindow->OnMouseOver();
}

void CWindowManager::RunMouseLeftButtonDown()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseLeftButtonDown())
		return;

	CWindow* pWin = GetPointWindow();
	if (!pWin)
		return;

	// Attach
	if (pWin->IsFlag(FLAG_ATTACH))
		pWin = pWin->GetRoot();

	m_lDragX = m_lMouseX - pWin->GetRect().left;
	m_lDragY = m_lMouseY - pWin->GetRect().top;
	m_lPickedX = m_lMouseX;
	m_lPickedY = m_lMouseY;

	if (m_lazyDeleteQueue.end() == std::find(m_lazyDeleteQueue.begin(), m_lazyDeleteQueue.end(), pWin))
		m_pLeftCaptureWindow = pWin;

	if (pWin->OnMouseLeftButtonDown())
		return;

	// Focus for EditLine
	CWindow* focus = FindWindowUpwardsByFlag(pWin, FLAG_FOCUSABLE);
	if (focus)
		SetFocus(focus);

	if (nullptr == m_pCaptureWindow && pWin->IsFlag(FLAG_MOVABLE || FLAG_DRAGABLE))
		CaptureMouse(pWin);
}

void CWindowManager::RunMouseLeftButtonUp()
{
	if (m_pCaptureWindow)
	{
		CWindow* pWin = m_pCaptureWindow;
		if (pWin->IsFlag(FLAG_MOVABLE || FLAG_DRAGABLE))
			ReleaseMouse();

		if (pWin->OnMouseLeftButtonUp())
			return;
	}

	bool processed = false;
	if (m_pLeftCaptureWindow)
	{
		processed = m_pLeftCaptureWindow->OnMouseLeftButtonUp();
		m_pLeftCaptureWindow = nullptr;
	}

	// Answer API call in C
	if (!processed)
	{
		CWindow* pWin = GetPointWindow();
		if (pWin)
			pWin->OnMouseLeftButtonUp();
	}
}

void CWindowManager::RunMouseLeftButtonDoubleClick()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseLeftButtonDoubleClick())
		return;

	CWindow* pWin = GetPointWindow();
	if (!pWin)
		return;

	pWin->OnMouseLeftButtonDoubleClick();
}

void CWindowManager::RunMouseRightButtonDown()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseRightButtonDown())
		return;

	CWindow* pWin = GetPointWindow();
	if (!pWin)
		return;

	// Attach
	if (pWin->IsFlag(FLAG_ATTACH))
		pWin = pWin->GetRoot();

	pWin->OnMouseRightButtonDown();

	if (m_lazyDeleteQueue.end() == std::find(m_lazyDeleteQueue.begin(), m_lazyDeleteQueue.end(), pWin))
		m_pRightCaptureWindow = pWin;

	CWindow* focus = FindWindowUpwardsByFlag(pWin, FLAG_FOCUSABLE);
	if (focus)
		SetFocus(focus);
}

void CWindowManager::RunMouseRightButtonUp()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseRightButtonUp())
		return;

	bool processed = false;

	if (m_pRightCaptureWindow)
	{
		processed = m_pRightCaptureWindow->OnMouseRightButtonUp();
		m_pRightCaptureWindow = nullptr;
		return;
	}

	if (!processed)
	{
		CWindow* pWin = GetPointWindow();
		if (pWin)
			pWin->OnMouseRightButtonUp();
	}

	DeattachIcon();
}

void CWindowManager::RunMouseRightButtonDoubleClick()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseRightButtonDoubleClick())
		return;

	CWindow* pWin = GetPointWindow();
	if (pWin)
		pWin->OnMouseRightButtonDoubleClick();
}

void CWindowManager::RunMouseMiddleButtonDown()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseMiddleButtonDown())
		return;

	CWindow* pWin = GetPointWindow();
	if (!pWin)
		return;

	pWin->OnMouseMiddleButtonDown();

	if (m_lazyDeleteQueue.end() == std::find(m_lazyDeleteQueue.begin(), m_lazyDeleteQueue.end(), pWin))
		m_pMiddleCaptureWindow = pWin;
}

void CWindowManager::RunMouseMiddleButtonUp()
{
	if (m_pCaptureWindow && m_pCaptureWindow->OnMouseMiddleButtonUp())
		return;

	if (m_pMiddleCaptureWindow)
	{
		m_pMiddleCaptureWindow->OnMouseMiddleButtonUp();
		m_pMiddleCaptureWindow = nullptr;
		return;
	}

	CWindow* win = GetPointWindow();
	if (!win)
		return;

	win->OnMouseMiddleButtonUp();
}

bool CWindowManager::RunMouseWheelEvent(int32_t nLen)
{
	if (m_pCaptureWindow)
		return m_pCaptureWindow->OnMouseWheelEvent(nLen);

	if (m_pLockWindow)
		return m_pLockWindow->OnMouseWheelEvent(nLen);

	auto focus = [nLen](CWindow* pWin) -> bool
	{
		return pWin->OnMouseWheelEvent(nLen);
	};

	if (m_pPointWindow && m_pPointWindow->IsRendering())
		if (TraverseWindowChainDown(m_pPointWindow, focus))
			return true;

	if (m_pFocusWindow && m_pFocusWindow->IsRendering())
		if (TraverseWindowChainDown(m_pFocusWindow, focus))
			return true;

	if (TraverseWindowChainDown(m_pRootWindow.get(), focus))
		return true;

	return false;
}

void CWindowManager::RunTab()
{
	if (m_pLockWindow)
	{
		m_pLockWindow->OnTab();
		return;
	}

	if (m_pFocusWindow && m_pFocusWindow->IsRendering())
		if (m_pFocusWindow->OnTab())
			return;

	auto focus = [](CWindow* pWin) -> bool
	{
		return pWin->OnTab();
	};

	TraverseWindowChainDown(m_pRootWindow.get(), focus);
}

void CWindowManager::RunChar(uint32_t ch)
{
	auto focus = [ch](CWindow* pWin) -> bool
	{
		return pWin->OnChar(ch);
	};

	if (m_pFocusWindow && TraverseWindowChainUp(m_pFocusWindow, focus))
		return;

	if (TraverseWindowChainDown(m_pRootWindow.get(), focus))
		return;
}

void CWindowManager::RunKeyDown(KeyCode code)
{
	auto focus = [code](CWindow* pWin) -> bool
	{
		return pWin->OnKeyDown(code);
	};

	CWindow* pWin = nullptr;

	if (m_pFocusWindow)
		pWin = TraverseWindowChainUp(m_pFocusWindow, focus);

	if (!pWin)
		pWin = TraverseWindowChainDown(m_pRootWindow.get(), focus);

	if (pWin && m_lazyDeleteQueue.end() == std::find(m_lazyDeleteQueue.begin(), m_lazyDeleteQueue.end(), pWin))
		m_keyHandlers[code] = pWin;
}

void CWindowManager::RunKeyUp(KeyCode code)
{
	auto focus = [code](CWindow* pWin) -> bool
	{
		return pWin->OnKeyUp(code);
	};

	auto it = m_keyHandlers.find(code);
	if (it != m_keyHandlers.end())
	{
		TraverseWindowChainUp(it->second, focus);
		TraverseWindowChainDown(m_pRootWindow.get(), focus);
		m_keyHandlers.erase(it);
		return;
	}

	if (m_pFocusWindow && TraverseWindowChainUp(m_pFocusWindow, focus))
		return;

	if (TraverseWindowChainDown(m_pRootWindow.get(), focus))
		return;
}

void CWindowManager::RunPressEscapeKey()
{
	if (m_pLockWindow)
	{
		m_pLockWindow->OnPressEscapeKey();
		return;
	}

	auto focus = [](CWindow* pWin) -> bool
	{
		return pWin->OnPressEscapeKey();
	};

	TraverseWindowChainDown(m_pRootWindow.get(), focus);
}

void CWindowManager::RunPressExitKey()
{
	if (m_pLockWindow)
	{
		m_pLockWindow->OnPressExitKey();
		return;
	}

	if (m_pFocusWindow)
	{
		m_pFocusWindow->OnPressExitKey();
		return;
	}

	auto focus = [](CWindow* pWin) -> bool
	{
		return pWin->OnPressExitKey();
	};

	TraverseWindowChainDown(m_pRootWindow.get(), focus);
}

void CWindowManager::NotifyHideWindow(CWindow* pWin)
{
	while (IsParentOf(m_pFocusWindow, pWin))
		KillFocus();

	while (IsParentOf(m_pLockWindow, pWin))
		UnlockWindow();

	while (IsParentOf(m_pCaptureWindow, pWin))
		ReleaseMouse();

	if (IsParentOf(m_pLeftCaptureWindow, pWin))
		m_pLeftCaptureWindow = nullptr;

	if (IsParentOf(m_pRightCaptureWindow, pWin))
		m_pRightCaptureWindow = nullptr;

	if (IsParentOf(m_pMiddleCaptureWindow, pWin))
		m_pMiddleCaptureWindow = nullptr;

	if (IsParentOf(m_pPointWindow, pWin))
		SetPointedWindowByPos(m_lMouseX, m_lMouseY);

	auto focus = [pWin](CWindow* pWinParent) -> bool
	{
		return IsParentOf(pWinParent, pWin);
	};

	m_LockWindowList.remove_if(focus);
	m_FocusWindowList.remove_if(focus);
	m_CaptureWindowList.remove_if(focus);

	for (auto first = m_keyHandlers.begin(), last = m_keyHandlers.end(); first != last; )
	{
		if (IsParentOf(first->second, pWin))
			first = m_keyHandlers.erase(first);
		else
			++first;
	}
}

CWindow* CWindowManager::GetLayerByName(const std::string& name)
{
	uint32_t index;
	if (!storm::ParseStringWithTable(name, index, kLayers))
		return nullptr;

	const CWindow::WindowList& layers = m_pRootWindow->GetChildren();
	if (index >= layers.size())
		return nullptr;

	return layers[index];
}

void CWindowManager::SetTopRecursive(CWindow* pWin)
{
	auto focus = [this](CWindow* pWinF) -> bool
	{
		SetTop(pWinF);
		return false; // Continue traversal
	};

	TraverseWindowChainUp(pWin, focus);
}

void CWindowManager::PopClipRegion()
{
	if (!clipStack_.empty())
		clipStack_.pop();

	CommitClipRegion();
}

bool CWindowManager::CommitClipRegion()
{
	if (clipStack_.empty())
	{
		STATEMANAGER->GetDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		return true;
	}

	auto top = clipStack_.top();
	D3DRECT region;
	region.x1 = (LONG)(top.x);
	region.y1 = (LONG)(top.y);
	region.x2 = (LONG)(top.z);
	region.y2 = (LONG)(top.w);

	if ((region.x2 - region.x1 > 0.f) && (region.y2 - region.y1 > 0.f))
	{
		RECT rect;
		rect.left = region.x1;
		rect.right = region.x2;
		rect.top = region.y1;
		rect.bottom = region.y2;

		STATEMANAGER->GetDevice()->SetScissorRect(&rect);
		return (STATEMANAGER->GetDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE) == S_OK);
	}
	else
		return false;
}

bool CWindowManager::PushClipRegion(const D3DXVECTOR4& clip_rect)
{
	D3DXVECTOR4 a(clip_rect);

	if (!clipStack_.empty())
	{
		D3DXVECTOR4 b = clipStack_.top();
		a.x = std::max(a.x, b.x);
		a.y = std::min(a.y, b.y);
		a.z = std::min(a.z, b.z);
		a.w = std::max(a.w, b.w);
	}

	clipStack_.push(a);

	bool success = CommitClipRegion();

	if (!success)
		clipStack_.pop();

	return success;
}

bool CWindowManager::PushClipRegion(CWindow* pWin)
{
	D3DXVECTOR2 pixelToClip(this->PixelToClip());
	D3DXVECTOR4 vector(pWin->GetRect().left + pWin->GetPositionX(), pWin->GetRect().top + pWin->GetPositionY(), pWin->GetRect().left + pWin->GetWidth(), pWin->GetRect().top + pWin->GetHeight());
	return PushClipRegion(vector);
}