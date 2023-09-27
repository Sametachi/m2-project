#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonGridSlotWindow.h"
#include "../EterLib/GrpSubImage.h"
#include "PythonEditLine.h"
#include "../EterLib/ResourceManager.h"
#include "../ScriptLib/StdAfx.h"

static BOOST_FORCEINLINE CWindow* CapsuleGetWindow(pybind11::capsule& c)
{
	auto ptr = c.get_pointer();
	if (!ptr)
		throw std::runtime_error("Invalid capsule pointer");

	return reinterpret_cast<CWindow*>(ptr);
}

static void wndMgrSetMouseHandler(pybind11::handle poHandler)
{
	CWindowManager::GetInstance()->SetMouseHandler(poHandler);
}

static void wndMgrSetScreenSize(int32_t width, int32_t height)
{
	CWindowManager::GetInstance()->SetScreenSize(width, height);
}

static int32_t wndMgrGetScreenWidth()
{
	return  CWindowManager::GetInstance()->GetScreenWidth();
}

static int32_t wndMgrGetScreenHeight()
{
	return  CWindowManager::GetInstance()->GetScreenHeight();
}

static void wndMgrAttachIcon(uint32_t iType, uint32_t iIndex, uint32_t iSlotNumber, uint8_t iWidth, uint8_t iHeight)
{
	CWindowManager::GetInstance()->AttachIcon(iType, iIndex, iSlotNumber, iWidth, iHeight);
}

static void wndMgrDeattachIcon()
{
	CWindowManager::GetInstance()->DeattachIcon();
}

static void wndMgrSetAttachingFlag(bool bFlag)
{

	CWindowManager::GetInstance()->SetAttachingFlag(bFlag);
}

static float wndMgrGetAspect()
{
	return  CWindowManager::GetInstance()->GetAspect(); 
}

static void CapsuleDestroyer(void* capsule)
{
	CWindow* winPointer = static_cast<CWindow*>(capsule);
	CWindowManager::GetInstance()->DestroyWindow(winPointer);
}

template <class pyRegister>
static pybind11::capsule wndMgrRegister(pybind11::handle po, std::string szLayer)
{
	CWindow* kWndMgr = CWindowManager::GetInstance()->RegisterWindow<pyRegister>(po, szLayer);
	if (!kWndMgr)
		throw std::runtime_error("Could not properly register window");
	auto capsule = pybind11::capsule(kWndMgr, CapsuleDestroyer);

	return capsule;
}

static void wndMgrOver(pybind11::capsule c) // https://metin2.dev/board/topic/20092-new-function/?do=findComment&comment=109247 2018 python
{
	CButton* pWin = (CButton*)CapsuleGetWindow(c);
	pWin->Over();
}
 
static void wndMgrDestroy(pybind11::capsule c)
{
	CWindow* pWin = CapsuleGetWindow(c);
	CWindowManager::GetInstance()->DestroyWindow(pWin);
}

static uint32_t GetFlag(const char* pszFlag)
{
	if (!stricmp(pszFlag, "movable"))
		return FLAG_MOVABLE;
	else if (!stricmp(pszFlag, "limit"))
		return FLAG_LIMIT;
	else if (!stricmp(pszFlag, "dragable"))
		return FLAG_DRAGABLE;
	else if (!stricmp(pszFlag, "attach"))
		return FLAG_ATTACH;
	else if (!stricmp(pszFlag, "restrict_x"))
		return FLAG_RESTRICT_X;
	else if (!stricmp(pszFlag, "restrict_y"))
		return FLAG_RESTRICT_Y;
	else if (!stricmp(pszFlag, "float"))
		return FLAG_FLOAT;
	else if (!stricmp(pszFlag, "not_pick"))
		return FLAG_NOT_PICK;
	else if (!stricmp(pszFlag, "ignore_size"))
		return FLAG_IGNORE_SIZE;
	else if (!stricmp(pszFlag, "rtl"))
		return FLAG_RTL;
	else if (!stricmp(pszFlag, "ltr"))
		return FLAG_RTL;
	else if (!stricmp(pszFlag, "alpha_sensitive"))
		return FLAG_ALPHA_SENSITIVE;
	else if (!stricmp(pszFlag, "focusable"))
		return FLAG_FOCUSABLE;
	else if (!stricmp(pszFlag, "animated_board"))
		return FLAG_ANIMATED_BOARD;
	else if (!stricmp(pszFlag, "component"))
		return FLAG_COMPONENT;
	else
		return 0;
}

static void wndMgrAddFlag(pybind11::capsule pWin_handle, std::string pszFlag)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pszFlag.empty())
	{
		uint32_t flag = GetFlag(pszFlag.c_str());
		if (!flag)
		{
			WarnLog("Unknown window flag {0}", pszFlag);
		}
		else
			pWin->AddFlag(flag);
	}

}

static bool wndMgrIsRTL(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->IsFlag(FLAG_RTL);
}

static void wndMgrSetName(pybind11::capsule pWin_handle, std::string szName)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->SetName(szName.c_str());
}

static std::string wndMgrGetName(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->GetName();
}

static void wndMgrSetTop(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindowManager::GetInstance()->SetTop(pWin);
}

static void wndMgrShow(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->Show();
}

static void wndMgrHide(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->Hide();
}

static bool wndMgrIsShow(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->IsShow();
}

static bool wndMgrIsRendering(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->IsRendering();
}

static bool wndMgrIsAttachedTo(pybind11::capsule pWin_handle, pybind11::capsule c)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindow* pWinAttached = CapsuleGetWindow(c);

	return false;
}

static bool wndMgrIsAttaching(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return false;
}

static void wndMgrSetAttachedTo(pybind11::capsule pWin_handle, pybind11::capsule c)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindow* pWinAttached = CapsuleGetWindow(c);
}

static void wndButtonLeftRightReverse(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	((CButton*)pWin)->LeftRightReverse();
}

static void wndButtonSetButtonScale(pybind11::capsule pWin_handle, float xScale, float yScale)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	((CButton*)pWin)->SetButtonScale(xScale, yScale);
}

static int32_t wndButtonGetButtonImageWidth(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return ((CButton*)pWin)->GetButtonImageWidth();
}

static int32_t wndButtonGetButtonImageHeight(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return ((CButton*)pWin)->GetButtonImageHeight();
}

static void wndMgrSetParent(pybind11::capsule pWin_handle, pybind11::capsule pParentWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindow* pParentWin = CapsuleGetWindow(pParentWin_handle);
	CWindowManager::GetInstance()->SetParent(pWin, pParentWin);
}

static pybind11::handle wndMgrGetParent(pybind11::capsule pWin_handle, pybind11::capsule pParentWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return pWin->GetParent()->GetHandler();
}

static void wndMgrSetPickAlways(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	//CWindowManager::GetInstance()->SetPickAlways(pWin);
}

static pybind11::handle wndMgrGetPickedWindow()
{
	return  CWindowManager::GetInstance()->GetPointWindow()->GetHandler();
}

static bool wndMgrIsFocus(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	return  pWin == CWindowManager::GetInstance()->GetFocus();
}

static void wndMgrSetFocus(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindowManager::GetInstance()->SetFocus(pWin);
}

static pybind11::handle wndMgrGetFocus()
{
	CWindow* wnd = CWindowManager::GetInstance()->GetFocus();
	if (!wnd)
		return pybind11::handle();

	return wnd->GetHandler();
}

static void wndMgrKillFocus(pybind11::capsule pWin_handle = pybind11::capsule())
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (pWin)
	{

		if (pWin == CWindowManager::GetInstance()->GetFocus())
		{
			CWindowManager::GetInstance()->KillFocus();
		}
	}
	else
		CWindowManager::GetInstance()->KillFocus();
}

static void wndMgrLock(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindowManager::GetInstance()->LockWindow(pWin);
}

static void wndMgrUnlock()
{
	CWindowManager::GetInstance()->UnlockWindow();
}

static void wndMgrSetWndSize(pybind11::capsule pWin_handle, float width, float height)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->SetSize(static_cast<int32_t>(width), static_cast<int32_t>(height));
}

static void wndMgrCaptureMouse(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindowManager::GetInstance()->CaptureMouse(pWin);
}

static void wndMgrReleaseMouse(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindowManager::GetInstance()->ReleaseMouse(pWin);
}

static pybind11::handle wndMgrGetCapture()
{
	return CWindowManager::GetInstance()->GetCapture()->GetHandler();
}

static void wndMgrSetWndPosition(pybind11::capsule pWin_handle, long x, long y)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->SetPosition(x, y);
}

static int32_t wndMgrGetWndWidth(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->GetWidth();
}

static int32_t wndMgrGetWndHeight(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->GetHeight();
}

static std::tuple<int32_t, int32_t> wndMgrGetWndLocalPosition(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return std::make_tuple(pWin->GetPositionX(), pWin->GetPositionY());
}

static std::tuple<LONG, LONG> wndMgrGetWndGlobalPosition(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	RECT& rRect = pWin->GetRect();
	return std::make_tuple(rRect.left, rRect.top);
}

static std::tuple<LONG, LONG, LONG, LONG> wndMgrGetWindowRect(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	RECT& rRect = pWin->GetRect();
	return std::make_tuple(rRect.left, rRect.top, rRect.right - rRect.left, rRect.bottom - rRect.top);
}

static std::tuple<LONG, LONG, LONG, LONG> wndMgrGetWindowBaseRect(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	RECT& rRect = pWin->GetRect();
	return std::make_tuple(rRect.left, rRect.top, rRect.right, rRect.bottom);
}

static void wndMgrSetWindowHorizontalAlign(pybind11::capsule pWin_handle, int iAlign)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->SetHorizontalAlign(iAlign);

}

static void wndMgrSetWindowVerticalAlign(pybind11::capsule pWin_handle, int iAlign)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->SetVerticalAlign(iAlign);

}

static size_t wndMgrGetChildCount(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin->GetChildrenCount();
}

static bool wndMgrIsPickedWindow(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	CWindow* pPickedWin = CWindowManager::GetInstance()->GetPointWindow();
	return  pWin == pPickedWin;
}

static bool wndMgrIsIn(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return  pWin == CWindowManager::GetInstance()->GetPointWindow();
}

static std::tuple<int32_t, int32_t> wndMgrGetMouseLocalPosition(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	pWin->MakeLocalPosition(lx, ly);
	return std::make_tuple(lx, ly);
}

static std::tuple<int32_t, int32_t> wndMgrGetMousePosition()
{
	return CWindowManager::GetInstance()->GetMousePosition();
}

static bool wndMgrIsDragging()
{
	return  CWindowManager::GetInstance()->IsDragging();
}

static void wndMgrUpdateRect(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	pWin->UpdateRect();
}

static bool wndMgrOnKeyDown(pybind11::capsule pWin_handle, KeyCode c)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	return pWin->OnKeyDown(c);
}

static void wndMgrAppendSlot(pybind11::capsule pWin_handle, uint32_t iIndex, int32_t ixPosition, int32_t iyPosition, int32_t ixCellSize, int32_t iyCellSize)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->AppendSlot(iIndex, ixPosition, iyPosition, ixCellSize, iyCellSize);

}

static void wndMgrArrangeSlot(pybind11::capsule pWin_handle, uint32_t iStartIndex, uint32_t ixCellCount, uint32_t iyCellCount, int32_t ixCellSize, int32_t iyCellSize, int32_t ixBlank, int32_t iyBlank)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CGridSlotWindow::Type()))
	{
		SysLog("wndMgr.ArrangeSlot : not a grid window");
		throw std::runtime_error("invalid window type");
	}

	CGridSlotWindow* pGridSlotWin = (CGridSlotWindow*)pWin;
	pGridSlotWin->ArrangeGridSlot(iStartIndex, ixCellCount, iyCellCount, ixCellSize, iyCellSize, ixBlank, iyBlank);

}

static void wndMgrClearSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->ClearSlot(iSlotIndex);

}

static void wndMgrClearAllSlot(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->ClearAllSlot();

}

static bool wndMgrHasSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;

	return  pSlotWin->HasSlot(iSlotIndex);
}

static void wndMgrSetSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex, uint32_t iItemIndex, uint8_t iWidth, uint8_t iHeight, std::string imageFileName)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
	{
		SysLog("wndMgr.ArrangeSlot : not a slot window");
		throw std::runtime_error("invalid window type");
	}

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;

	auto iImageHandle = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(imageFileName);
	auto dc = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	pSlotWin->SetSlot(iSlotIndex, iItemIndex, iWidth, iHeight, iImageHandle, dc);
}

static void wndMgrSetSlot2(pybind11::capsule pWin_handle, uint32_t iSlotIndex, uint32_t iItemIndex, uint8_t iWidth, uint8_t iHeight, std::string imageFileName, std::tuple<float, float, float, float> t)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
	{
		SysLog("wndMgr.ArrangeSlot : not a slot window");
		throw std::runtime_error("invalid window type");
	}

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;

	auto iImageHandle = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(imageFileName);
	pSlotWin->SetSlot(iSlotIndex, iItemIndex, iWidth, iHeight, iImageHandle, D3DXCOLOR(std::get<0>(t), std::get<1>(t), std::get<2>(t), std::get<3>(t)));
}

static void wndMgrSetSlotCount(pybind11::capsule pWin_handle, uint32_t iSlotIndex, uint32_t iCount)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotCount(iSlotIndex, iCount);

}

static void wndMgrSetSlotCountNew(pybind11::capsule pWin_handle, uint32_t iSlotIndex, uint32_t iGrade, uint32_t iCount)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotCountNew(iSlotIndex, iGrade, iCount);
}

static void wndMgrSetRealSlotNumber(pybind11::capsule pWin_handle, uint32_t iSlotIndex, uint32_t iRealSlotNumber)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetRealSlotNumber(iSlotIndex, iRealSlotNumber);
}

static void wndMgrSetSlotCoolTime(pybind11::capsule pWin_handle, uint32_t iSlotIndex, float fCoolTime, float fElapsedTime)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotCoolTime(iSlotIndex, fCoolTime, fElapsedTime);

}

static void wndMgrSetToggleSlot()
{
	assert(!"wndMgrSetToggleSlot - Don't use such function");
}

static void wndMgrActivateSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->ActivateSlot(iSlotIndex);
}

static void wndMgrDeactivateSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->DeactivateSlot(iSlotIndex);
}

static void wndMgrEnableSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->EnableSlot(iSlotIndex);
}

static void wndMgrDisableSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->DisableSlot(iSlotIndex);
}

static void wndMgrShowSlotBaseImage(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->ShowSlotBaseImage(iSlotIndex);
}

static void wndMgrHideSlotBaseImage(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->HideSlotBaseImage(iSlotIndex);
}

static void wndMgrSetSlotType(pybind11::capsule pWin_handle, uint32_t iType)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotType(iType);

}

static void wndMgrSetSlotStyle(pybind11::capsule pWin_handle, uint32_t iStyle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotStyle(iStyle);

}

static void wndMgrSetSlotBaseImage(pybind11::capsule pWin_handle, std::string szFileName, float fr, float fg, float fb, float fa)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
	{
		SysLog("wndMgr.ArrangeSlot : not a slot window");
		throw std::runtime_error("invalid window type");
	}

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetSlotBaseImage(szFileName.c_str(), fr, fg, fb, fa);

}

static void wndMgrSetCoverButton(pybind11::capsule pWindow_handle, uint32_t iSlotIndex, std::string szUpImageName, std::string szOverImageName, std::string szDownImageName, std::string szDisableImageName, bool iLeftButtonEnable, bool iRightButtonEnable)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->SetCoverButton(iSlotIndex, szUpImageName.c_str(), szOverImageName.c_str(), szDownImageName.c_str(), szDisableImageName.c_str(), iLeftButtonEnable, iRightButtonEnable);

}

static void wndMgrEnableCoverButton(pybind11::capsule pWindow_handle, uint32_t iSlotIndex)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->EnableCoverButton(iSlotIndex);

}

static void wndMgrDisableCoverButton(pybind11::capsule pWindow_handle, uint32_t iSlotIndex)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->DisableCoverButton(iSlotIndex);

}

static bool wndMgrIsDisableCoverButton(pybind11::capsule pWindow_handle, uint32_t iSlotIndex)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	return  pSlotWin->IsDisableCoverButton(iSlotIndex);
}

static void wndMgrSetAlwaysRenderCoverButton(pybind11::capsule pWindow_handle, uint32_t iSlotIndex, bool bAlwaysRender)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->SetAlwaysRenderCoverButton(iSlotIndex, bAlwaysRender);

}

static void wndMgrAppendSlotButton(pybind11::capsule pWindow_handle, std::string szUpImageName, std::string szOverImageName, std::string szDownImageName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->AppendSlotButton(szUpImageName.c_str(), szOverImageName.c_str(), szDownImageName.c_str());
}

static void wndMgrAppendRequirementSignImage(pybind11::capsule pWindow_handle, std::string szImageName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->AppendRequirementSignImage(szImageName.c_str());
}

static void wndMgrShowSlotButton(pybind11::capsule pWindow_handle, uint32_t iSlotNumber)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->ShowSlotButton(iSlotNumber);

}

static void wndMgrHideAllSlotButton(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->HideAllSlotButton();

}

static void wndMgrShowRequirementSign(pybind11::capsule pWindow_handle, uint32_t iSlotNumber)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->ShowRequirementSign(iSlotNumber);

}

static void wndMgrHideRequirementSign(pybind11::capsule pWindow_handle, uint32_t iSlotNumber)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->HideRequirementSign(iSlotNumber);

}

static void wndMgrRefreshSlot(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->RefreshSlot();

}

static void wndMgrSetUseMode(pybind11::capsule pWin_handle, bool iFlag)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetUseMode(iFlag);

}

static void wndMgrSetUsableItem(pybind11::capsule pWin_handle, bool iFlag)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetUsableItem(iFlag);

}

static void wndMgrSelectSlot(pybind11::capsule pWin_handle, uint32_t iIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SelectSlot(iIndex);

}

static void wndMgrClearSelected(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->ClearSelected();

}

static uint32_t wndMgrGetSelectedSlotCount(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	return  pSlotWin->GetSelectedSlotCount();
}

static uint32_t wndMgrGetSelectedSlotNumber(pybind11::capsule pWin_handle, uint32_t iSlotNumber)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	return  pSlotWin->GetSelectedSlotNumber(iSlotNumber);
}

static bool wndMgrIsSelectedSlot(pybind11::capsule pWin_handle, uint32_t iSlotNumber)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	return  pSlotWin->isSelectedSlot(iSlotNumber);
}

static uint32_t wndMgrGetSlotCount(pybind11::capsule pWin_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	return  pSlotWin->GetSlotCount();
}

static void wndMgrLockSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->LockSlot(iSlotIndex);
}

static void wndMgrUnlockSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->UnlockSlot(iSlotIndex);
}

static void wndMgrSetCanMouseEventSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetCanMouseEventSlot(iSlotIndex);
}

static void wndMgrSetCantMouseEventSlot(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetCantMouseEventSlot(iSlotIndex);
}

static void wndMgrSetUsableSlotOnTopWnd(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetUsableSlotOnTopWnd(iSlotIndex);
}

static void wndMgrSetUnusableSlotOnTopWnd(pybind11::capsule pWin_handle, uint32_t iSlotIndex)
{
	CWindow* pWin = CapsuleGetWindow(pWin_handle);
	if (!pWin->IsType(CSlotWindow::Type()))
		throw std::runtime_error("invalid window type");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWin;
	pSlotWin->SetUnusableSlotOnTopWnd(iSlotIndex);
}

static void wndBarSetColor(pybind11::capsule pWindow_handle, uint32_t iColor)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (!pWindow->IsType(CBar3D::Type()))
	{
		pWindow->SetColor(iColor);
	}
	else
		throw std::runtime_error("Bar3D required right and center color");
}

static void wndBarSetColor2(pybind11::capsule pWindow_handle, uint32_t iColor, uint32_t iRightColor, uint32_t iCenterColor)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (pWindow->IsType(CBar3D::Type()))
	{
		((CBar3D*)pWindow)->SetColor(iColor, iRightColor, iCenterColor);
	}
	else
	{
		throw std::runtime_error("Only invoke SetColor with 3 colors for Bar3D");
	}
}

static void wndTextSetMax(pybind11::capsule pWindow_handle, uint32_t iMax)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWindow)->SetMax(iMax);
}

static void wndEditInsert(pybind11::capsule pWindow_handle, std::string str, uint32_t pos)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	auto wnd = (CPythonEditLine*)pWindow;
	wnd->Insert(pos, str);
}

static void wndEditInsert2(pybind11::capsule pWindow_handle, std::string str)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	auto wnd = (CPythonEditLine*)pWindow;
	wnd->Insert(wnd->GetCursorPosition(), str);
}

static void wndEditErase(pybind11::capsule pWindow_handle, uint32_t str, uint32_t pos)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	auto wnd = (CPythonEditLine*)pWindow;
	wnd->Erase(pos, str);
}

static void wndEditErase2(pybind11::capsule pWindow_handle, uint32_t str)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	auto wnd = (CPythonEditLine*)pWindow;
	wnd->Erase(wnd->GetCursorPosition(), str);
}

static void wndEditSetMaxVisible(pybind11::capsule pWindow_handle, uint32_t iMax)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWindow)->SetMaxVisible(iMax);
}

static void wndEditSetPlaceholderText(pybind11::capsule pWindow_handle, std::string iMax)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWindow)->SetPlaceholderText(iMax);
}

static void wndEditSetPlaceholderColor(pybind11::capsule pWindow_handle, uint32_t iMax)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWindow)->SetPlaceholderColor(iMax);
}

static void wndTextSetHorizontalAlign(pybind11::capsule pWindow_handle, uint32_t iType)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetHorizontalAlign(iType);
}

static void wndTextSetVerticalAlign(pybind11::capsule pWindow_handle, uint32_t iType)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetVerticalAlign(iType);
}

static void wndTextSetSecret(pybind11::capsule pWindow_handle, bool iFlag)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetSecret(iFlag);
}

static void wndTextSetOutline(pybind11::capsule pWindow_handle, bool iFlag)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetOutline(iFlag);
}

static void wndTextSetMultiLine(pybind11::capsule pWindow_handle, bool iFlag)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetMultiLine(iFlag);
}

static void wndTextSetText(pybind11::capsule pWindow_handle, std::string szText)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);

	((CTextLine*)pWindow)->SetText(szText);
}

static void wndTextSetFontName(pybind11::capsule pWindow_handle, std::string szFontName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetFontName(szFontName);
}

static void wndTextSetFontColor(pybind11::capsule pWindow_handle, float fr, float fg, float fb)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetFontColor(D3DXCOLOR(fr, fg, fb, 1.0f));
}

static void wndTextSetFontColor3(pybind11::capsule pWindow_handle, float fr, float fg, float fb, float fa)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetFontColor(D3DXCOLOR(fr, fg, fb, fa));
}

static void wndTextSetFontColor2(pybind11::capsule pWindow_handle, uint32_t iColor)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetFontColor(iColor);
}

static void wndTextSetFontColor4(pybind11::capsule pWindow_handle, uint32_t iColor, uint32_t iColor2)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetFontGradient(iColor, iColor2);
}

static void wndTextSetOutLineColor(pybind11::capsule pWindow_handle, uint32_t dwColor)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetOutLineColor(dwColor);
}

static void wndTextSetOutLineColor2(pybind11::capsule pWindow_handle, uint32_t dwColor, uint32_t dwColor2)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetOutLineColorGradient(dwColor, dwColor2);
}

static void wndTextSetLimitWidth(pybind11::capsule pWindow_handle, float fWidth)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CTextLine*)pWindow)->SetLimitWidth(fWidth);
}

static std::string wndTextGetText(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CTextLine*)pWindow)->GetText();
}

static std::string wndTextGetHyperlinkAtPos(pybind11::capsule pWindow_handle, int32_t x, int32_t y)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	auto textLine = (CTextLine*)pWindow;
	return  textLine->GetHyperlinkAtPos(x, y);
}

static void wndTextSetTextClip(pybind11::capsule pWindow_handle, LONG left, LONG top, LONG right, LONG bottom)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	RECT r{ left, top, right, bottom };
	((CTextLine*)pWindow)->SetClipRect(r);
}


static uint32_t wndTextGetLineCount(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CTextLine*)pWindow)->GetLineCount();
}

static std::tuple<int32_t, int32_t> wndTextGetTextSize(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	int32_t nWidth;
	int32_t nHeight;

	CTextLine* pkTextLine = (CTextLine*)pWin;
	pkTextLine->GetTextSize(&nWidth, &nHeight);

	return std::make_tuple(nWidth, nHeight);
}

static uint32_t wndTextGetCursorPosition(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	return  ((CPythonEditLine*)pWin)->GetCursorPosition();
}

static void wndEditMoveToEnd(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWin)->MoveToEnd();
}

static void wndEditDisable(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWin)->Disable();
}

static void wndEditEnable(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CPythonEditLine*)pWin)->Enable();
}

static void wndNumberSetNumber(pybind11::capsule pWindow_handle, std::string szNumber)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CNumberLine*)pWin)->SetNumber(szNumber.c_str());

}

static void wndNumberSetNumberHorizontalAlignCenter(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CNumberLine*)pWin)->SetHorizontalAlign(CWindow::HORIZONTAL_ALIGN_CENTER);

}

static void wndNumberSetNumberHorizontalAlignRight(pybind11::capsule pWindow_handle)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CNumberLine*)pWin)->SetHorizontalAlign(CWindow::HORIZONTAL_ALIGN_RIGHT);

}

static void wndNumberSetPath(pybind11::capsule pWindow_handle, std::string szPath)
{
	CWindow* pWin = CapsuleGetWindow(pWindow_handle);
	((CNumberLine*)pWin)->SetPath(szPath.c_str());

}

static void wndMarkBox_SetImage(pybind11::capsule pWindow_handle, std::string szFileName)
{
}

static void wndMarkBox_SetImageFilename(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CMarkBox*)pWindow)->LoadImage(szFileName.c_str());
}

static void wndMarkBox_Load()
{
}

static void wndMarkBox_SetIndex(pybind11::capsule pWindow_handle, uint32_t nIndex)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CMarkBox*)pWindow)->SetIndex(nIndex);
}

static void wndMarkBox_SetScale(pybind11::capsule pWindow_handle, float fScale)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CMarkBox*)pWindow)->SetScale(fScale);
}

static void wndMarkBox_SetDiffuseColor(pybind11::capsule pWindow_handle, float fr, float fg, float fb, float fa)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CMarkBox*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);

}

static void wndImageLoadImage(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (!((CImageBox*)pWindow)->LoadImage(szFileName.c_str()))
		throw std::runtime_error("Failed to load image (filename: " + szFileName + ")");
}

static void wndImageSetDiffuseColor(pybind11::capsule pWindow_handle, float fr, float fg, float fb, float fa)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CImageBox*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);

}

static int32_t wndImageGetWidth(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CImageBox*)pWindow)->GetWidth();
}

static int32_t wndImageGetHeight(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CImageBox*)pWindow)->GetHeight();
}

static void wndSetClipRect(pybind11::capsule pWindow_handle, float fLeft, float fTop, float fRight, float fBottom, bool isVertical)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (pWindow->IsType(CExpandedImageBox::Type()))
		((CExpandedImageBox*)pWindow)->SetImageClipRect(fLeft, fTop, fRight, fBottom, isVertical);

}
static void wndImageSetScale(pybind11::capsule pWindow_handle, float fx, float fy)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);

	((CExpandedImageBox*)pWindow)->SetScale(fx, fy);

}

static void wndImageSetOrigin(pybind11::capsule pWindow_handle, float fx, float fy)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CExpandedImageBox*)pWindow)->SetOrigin(fx, fy);

}

static void wndImageSetRotation(pybind11::capsule pWindow_handle, float fRotation)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CExpandedImageBox*)pWindow)->SetRotation(fRotation);

}

static void wndImageSetRenderingRect(pybind11::capsule pWindow_handle, float fLeft, float fTop, float fRight, float fBottom)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (pWindow->IsType(CExpandedImageBox::Type()))
		((CExpandedImageBox*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else if (pWindow->IsType(CAniImageBox::Type()))
		((CAniImageBox*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);

}

static void wndImageSetRenderingMode(pybind11::capsule pWindow_handle, int32_t iMode)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	if (pWindow->IsType(CExpandedImageBox::Type()))
		((CExpandedImageBox*)pWindow)->SetRenderingMode(iMode);

}

static void wndImageSetDelay(pybind11::capsule pWindow_handle, int32_t fDelay)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->SetDelay(fDelay);

}

static void wndImageAppendImage(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->AppendImage(szFileName.c_str());
}

static void wndAniImageStart(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->Start();
}

static void wndAniImageStop(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->Stop();
}

static void wndImageResetFrame(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->ResetFrame();
}

static void wndImageSetAniImgScale(pybind11::capsule pWindow_handle, float fX, float fY)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->SetAniImgScale(fX, fY);
}

static void wndMgrSetAniImgDiffuseColor(pybind11::capsule pWindow_handle, float r, float g, float b, float a)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CAniImageBox*)pWindow)->SetDiffuseColor(r, g, b, a);
}

static void wndImageSetRenderingRectWithScale(pybind11::capsule pWindow_handle, float fLeft, float fTop, float fRight, float fBottom)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);

	if (pWindow->IsType(CExpandedImageBox::Type()))
	{
		((CExpandedImageBox*)pWindow)->SetRenderingRectWithScale(fLeft, fTop, fRight, fBottom);
	}
	else if (pWindow->IsType(CAniImageBox::Type()))
	{
		((CAniImageBox*)pWindow)->SetRenderingRectWithScale(fLeft, fTop, fRight, fBottom);
	}
}

static void wndButtonSetUpVisual(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->SetUpVisual(szFileName.c_str());

}

static void wndButtonSetOverVisual(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->SetOverVisual(szFileName.c_str());

}

static void wndButtonSetDownVisual(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->SetDownVisual(szFileName.c_str());

}

static void wndButtonSetDisableVisual(pybind11::capsule pWindow_handle, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->SetDisableVisual(szFileName.c_str());

}

static std::string wndButtonGetUpVisualFileName(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CButton*)pWindow)->GetUpVisualFileName();
}

static std::string wndButtonGetOverVisualFileName(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CButton*)pWindow)->GetOverVisualFileName();
}

static std::string wndButtonGetDownVisualFileName(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CButton*)pWindow)->GetDownVisualFileName();
}

static void wndButtonFlash(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->Flash();
}

static void wndButtonEnableFlash(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->EnableFlash();
}

static void wndButtonDisableFlash(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->DisableFlash();
}

static void wndButtonEnable(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->Enable();

}

static void wndButtonDisable(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->Disable();
}

static void wndButtonDown(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	((CButton*)pWindow)->Down();

}

static void wndButtonSetUp(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);

	((CButton*)pWindow)->SetUp();

}

static bool wndButtonIsDown(pybind11::capsule pWindow_handle)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);
	return  ((CButton*)pWindow)->IsPressed();
}

static void wndButtonSetRestrictMovementArea(pybind11::capsule pWindow_handle, int32_t ix, int32_t iy, int32_t iwidth, int32_t iheight)
{
	CWindow* pWindow = CapsuleGetWindow(pWindow_handle);

	if (pWindow->IsType(CDragButton::Type()))
		((CDragButton*)pWindow)->SetRestrictMovementArea(ix, iy, iwidth, iheight);
	else if (pWindow->IsType(CDragBar::Type()))
		((CDragBar*)pWindow)->SetRestrictMovementArea(ix, iy, iwidth, iheight);
}

static bool wndDragBarIsPressed(pybind11::capsule c)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	return ((CDragBar*)pWindow)->IsPressed();
}

static void wndMgrSetSlotBackground(pybind11::capsule c, uint32_t iSlotIndex, std::string szFileName)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->SetSlotBackground(iSlotIndex, szFileName.c_str());
}

static void wndMgrSetSlotID(pybind11::capsule c, uint32_t iSlotIndex, uint32_t id)
{
	CWindow* pWindow = CapsuleGetWindow(c);

	if (!pWindow->IsType(CSlotWindow::Type()))
		throw std::runtime_error("Window is not a slotwindow");

	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->SetSlotID(iSlotIndex, id);
}

static void wndMgrSetAttachingRealSlotNumber(uint32_t iRealSlotNumber)
{
	CWindowManager::GetInstance()->SetAttachingRealSlotNumber(iRealSlotNumber);
}

static void wndMgrRemoveFlag(pybind11::capsule c, std::string pszFlagstr)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	if (!pszFlagstr.empty())
	{
		uint32_t flag = GetFlag(pszFlagstr.c_str());
		if (!flag)
		{
			WarnLog("Unknown window flag {0}", pszFlagstr);
		}
		else
			pWindow->RemoveFlag(flag);
	}
}

static void wndMgrSetAlpha(pybind11::capsule c, float fAlpha)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	pWindow->SetSingleAlpha(fAlpha);
}

static float wndMgrGetAlpha(pybind11::capsule c, float fAlpha)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	return pWindow->GetAlpha();
}

static void wndMgrSetAllAlpha(pybind11::capsule c, float fAlpha)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	pWindow->SetAllAlpha(fAlpha);
}

static void wndMgrDeleteCoverButton(pybind11::capsule c, uint32_t iSlotIndex)
{
	CWindow* pWindow = CapsuleGetWindow(c);

	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->DeleteCoverButton(iSlotIndex);
}

static bool wndMgrHasCoverButton(pybind11::capsule c, uint32_t iSlotIndex)
{
	CWindow* pWindow = CapsuleGetWindow(c);

	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	return pSlotWin->HasCoverButton(iSlotIndex);
}

static void wndMgrSetSlotBaseImageScale(pybind11::capsule c, float fx, float fy)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	if (!pWindow->IsType(CSlotWindow::Type()))
	{
		SysLog("wndMgr.ArrangeSlot : not a slot window");
		return;
	}

	CSlotWindow* pSlotWin = (CSlotWindow*)pWindow;
	pSlotWin->SetSlotBaseImageScale(fx, fy);
}

static bool wndMgrIsDisable(pybind11::capsule c, float fx, float fy)
{
	CWindow* pWindow = CapsuleGetWindow(c);
	return ((CButton*)pWindow)->IsDisable();
}

static void wndMgrIllSetCamera(pybind11::capsule c, uint32_t render_index, std::string background_image)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->SetCamera(render_index, background_image);
#endif
}

static void wndMgrIllShow(pybind11::capsule c)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->Show();
#endif
}

static void wndMgrIllHide(pybind11::capsule c)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->Hide();
#endif
}

static void wndMgrIllIllustrateRace(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->IllustrateActor(race_vnum);
#endif
}

static void wndMgrIllIllustrateWeapon(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->IllustrateWeapon(race_vnum);
#endif
}

static void wndMgrIllIllustrateEffect(pybind11::capsule c, bool b)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->IllustrateEffect(b);
#endif
}

static void wndMgrIllChangeArmor(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ChangeArmor(race_vnum);
#endif
}

static void wndMgrIllChangeHair(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ChangeHair(race_vnum);
#endif
}

static void wndMgrIllChangeWeapon(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ChangeWeapon(race_vnum);
#endif
}

static void wndMgrIllChangeMotion(pybind11::capsule c, uint32_t race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ChangeMotion(race_vnum);
#endif
}

static void wndMgrIllChangeRotation(pybind11::capsule c, float race_vnum)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ChangeRotation(race_vnum);
#endif
}

static void wndMgrIllForceRotation(pybind11::capsule c, bool b)
{
	CWindow* pWindow = CapsuleGetWindow(c);
#if 0
	PyGrannyIllustrator* pGrIllustrator = (PyGrannyIllustrator*)pWindow;
	pGrIllustrator->ForceRotation(b);
#endif
}

PYBIND11_EMBEDDED_MODULE(wndMgr, m)
{
	m.def("SetSlotID", wndMgrSetSlotID);

	// Window Manager
	m.def("SetMouseHandler", wndMgrSetMouseHandler);
	m.def("SetScreenSize", wndMgrSetScreenSize);
	m.def("GetScreenWidth", wndMgrGetScreenWidth);
	m.def("GetScreenHeight", wndMgrGetScreenHeight);
	m.def("AttachIcon", wndMgrAttachIcon);
	m.def("DeattachIcon", wndMgrDeattachIcon);
	m.def("SetAttachingFlag", wndMgrSetAttachingFlag);
	m.def("SetAttachingRealSlotNumber", wndMgrSetAttachingRealSlotNumber);
	m.def("GetAspect", wndMgrGetAspect);

	// Window
	m.def("Register", wndMgrRegister<CWindow>);
	m.def("RegisterSlotWindow", wndMgrRegister<CSlotWindow>);
	m.def("RegisterGridSlotWindow", wndMgrRegister<CGridSlotWindow>);
	m.def("RegisterTextLine", wndMgrRegister<CTextLine>);
	m.def("RegisterMarkBox", wndMgrRegister<CMarkBox>);
	m.def("RegisterImageBox", wndMgrRegister<CImageBox>);
	m.def("RegisterExpandedImageBox", wndMgrRegister<CExpandedImageBox>);
	m.def("RegisterAniImageBox", wndMgrRegister<CAniImageBox>);
	m.def("RegisterButton", wndMgrRegister<CButton>);
	m.def("RegisterRadioButton", wndMgrRegister<CRadioButton>);
	m.def("RegisterToggleButton", wndMgrRegister<CToggleButton>);
	m.def("RegisterDragButton", wndMgrRegister<CDragButton>);
	m.def("RegisterBox", wndMgrRegister<CBox>);
	m.def("RegisterBar", wndMgrRegister<CBar>);
	m.def("RegisterLine", wndMgrRegister<CLine>);
	m.def("RegisterBar3D", wndMgrRegister<CBar3D>);
	m.def("RegisterNumberLine", wndMgrRegister<CNumberLine>);
	m.def("RegisterEditLine", wndMgrRegister<CPythonEditLine>);
	//m.def("RegisterGrannyIllustrator", wndMgrRegister<PyGrannyIllustrator>);

	m.def("Destroy", wndMgrDestroy);
	m.def("AddFlag", wndMgrAddFlag);
	m.def("RemoveFlag", wndMgrRemoveFlag);
	m.def("IsRTL", wndMgrIsRTL);

	// Alpha
	m.def("SetAlpha", wndMgrSetAlpha);
	m.def("GetAlpha", wndMgrGetAlpha);
	m.def("SetAllAlpha", wndMgrSetAllAlpha);

	// Base Window
	m.def("SetName", wndMgrSetName);
	m.def("GetName", wndMgrGetName);

	m.def("SetTop", wndMgrSetTop);
	m.def("Show", wndMgrShow);
	m.def("Hide", wndMgrHide);
	m.def("IsShow", wndMgrIsShow);
	m.def("IsRendering", wndMgrIsRendering);

	m.def("IsAttachedTo", wndMgrIsAttachedTo);
	m.def("IsAttaching", wndMgrIsAttaching);
	m.def("SetAttachedTo", wndMgrSetAttachedTo);

	m.def("SetParent", wndMgrSetParent);
	m.def("GetParent", wndMgrGetParent);
	m.def("SetPickAlways", wndMgrSetPickAlways);
	m.def("GetPickedWindow", wndMgrGetPickedWindow);
	m.def("IsPickedWindow", wndMgrIsPickedWindow);
	
	m.def("IsFocus", wndMgrIsFocus);
	m.def("SetFocus", wndMgrSetFocus);
	m.def("KillFocus", wndMgrKillFocus);
	m.def("GetFocus", wndMgrGetFocus);

	m.def("Lock", wndMgrLock);
	m.def("Unlock", wndMgrUnlock);

	m.def("CaptureMouse", wndMgrCaptureMouse);
	m.def("ReleaseMouse", wndMgrReleaseMouse);
	m.def("GetCapture", wndMgrGetCapture);

	m.def("SetWindowSize", wndMgrSetWndSize);
	m.def("SetWindowPosition", wndMgrSetWndPosition);
	m.def("GetWindowWidth", wndMgrGetWndWidth);
	m.def("GetWindowHeight", wndMgrGetWndHeight);
	m.def("GetWindowLocalPosition", wndMgrGetWndLocalPosition);
	m.def("GetWindowGlobalPosition", wndMgrGetWndGlobalPosition);
	m.def("GetWindowRect", wndMgrGetWindowRect);
	m.def("GetWindowBaseRect", wndMgrGetWindowBaseRect);

	m.def("SetWindowHorizontalAlign", wndMgrSetWindowHorizontalAlign);
	m.def("SetWindowVerticalAlign", wndMgrSetWindowVerticalAlign);

	m.def("GetChildCount", wndMgrGetChildCount);

	m.def("IsIn", wndMgrIsIn);
	m.def("GetMouseLocalPosition", wndMgrGetMouseLocalPosition);
	m.def("GetMousePosition", wndMgrGetMousePosition);
	m.def("IsDragging", wndMgrIsDragging);

	m.def("UpdateRect", wndMgrUpdateRect);
	m.def("OnKeyDown", wndMgrOnKeyDown);

	// Slot Window

	m.def("AppendSlot", wndMgrAppendSlot);
	m.def("ArrangeSlot", wndMgrArrangeSlot);
	m.def("ClearSlot", wndMgrClearSlot);
	m.def("ClearAllSlot", wndMgrClearAllSlot);
	m.def("HasSlot", wndMgrHasSlot);
	m.def("SetSlot", wndMgrSetSlot);
	m.def("SetSlot", wndMgrSetSlot2);
	m.def("SetSlotBaseImageScale", wndMgrSetSlotBaseImageScale);
	m.def("SetSlotCount", wndMgrSetSlotCount);
	m.def("SetSlotCountNew", wndMgrSetSlotCountNew);
	m.def("SetSlotRealNumber", wndMgrSetRealSlotNumber);
	m.def("SetSlotCoolTime", wndMgrSetSlotCoolTime);
	m.def("SetToggleSlot", wndMgrSetToggleSlot);
	m.def("ActivateSlot", wndMgrActivateSlot);
	m.def("DeactivateSlot", wndMgrDeactivateSlot);
	m.def("EnableSlot", wndMgrEnableSlot);
	m.def("DisableSlot", wndMgrDisableSlot);
	m.def("ShowSlotBaseImage", wndMgrShowSlotBaseImage);
	m.def("HideSlotBaseImage", wndMgrHideSlotBaseImage);
	m.def("SetSlotType", wndMgrSetSlotType);
	m.def("SetSlotStyle", wndMgrSetSlotStyle);
	m.def("SetSlotBaseImage", wndMgrSetSlotBaseImage);

	m.def("SetCoverButton", wndMgrSetCoverButton);
	m.def("DeleteCoverButton", wndMgrDeleteCoverButton);
	m.def("HasCoverButton", wndMgrHasCoverButton);
	m.def("EnableCoverButton", wndMgrEnableCoverButton);
	m.def("DisableCoverButton", wndMgrDisableCoverButton);
	m.def("IsDisableCoverButton", wndMgrIsDisableCoverButton);
	m.def("SetAlwaysRenderCoverButton", wndMgrSetAlwaysRenderCoverButton);

	m.def("AppendSlotButton", wndMgrAppendSlotButton);
	m.def("AppendRequirementSignImage", wndMgrAppendRequirementSignImage);
	m.def("ShowSlotButton", wndMgrShowSlotButton);
	m.def("HideAllSlotButton", wndMgrHideAllSlotButton);
	m.def("ShowRequirementSign", wndMgrShowRequirementSign);
	m.def("HideRequirementSign", wndMgrHideRequirementSign);
	m.def("RefreshSlot", wndMgrRefreshSlot);
	m.def("SetUseMode", wndMgrSetUseMode);
	m.def("SetUsableItem", wndMgrSetUsableItem);
	
	m.def("SelectSlot", wndMgrSelectSlot);
	m.def("ClearSelected", wndMgrClearSelected);
	m.def("GetSelectedSlotCount", wndMgrGetSelectedSlotCount);
	m.def("GetSelectedSlotNumber", wndMgrGetSelectedSlotNumber);
	m.def("IsSelectedSlot", wndMgrIsSelectedSlot);
	m.def("GetSlotCount", wndMgrGetSlotCount);
	m.def("LockSlot", wndMgrLockSlot);
	m.def("UnlockSlot", wndMgrUnlockSlot);
	m.def("SetCanMouseEventSlot", wndMgrSetCanMouseEventSlot);
	m.def("SetCantMouseEventSlot", wndMgrSetCantMouseEventSlot);
	m.def("SetUsableSlotOnTopWnd", wndMgrSetUsableSlotOnTopWnd);
	m.def("SetUnusableSlotOnTopWnd", wndMgrSetUnusableSlotOnTopWnd);

	// Bar
	m.def("SetColor", wndBarSetColor);
	m.def("SetColor", wndBarSetColor2);

	// TextLine
	m.def("SetSecret", wndTextSetSecret);
	m.def("SetOutline", wndTextSetOutline);
	m.def("SetMultiLine", wndTextSetMultiLine);
	m.def("SetText", wndTextSetText);
	m.def("SetFontName", wndTextSetFontName);
	m.def("SetFontColor", wndTextSetFontColor);
	m.def("SetFontColor", wndTextSetFontColor2);
	m.def("SetFontColor", wndTextSetFontColor3);
	m.def("SetFontColor", wndTextSetFontColor4);
	m.def("SetOutLineColor", wndTextSetOutLineColor);
	m.def("SetOutLineColor", wndTextSetOutLineColor2);
	m.def("SetLimitWidth", wndTextSetLimitWidth);
	m.def("GetText", wndTextGetText);
	m.def("GetHyperlinkAtPos", wndTextGetHyperlinkAtPos);
	m.def("SetClipRect", wndTextSetTextClip);
	m.def("GetTextLineCount", wndTextGetLineCount);
	m.def("GetTextSize", wndTextGetTextSize);
	//m.def("SetHorizontalAlign", wndTextSetHorizontalAlign);
	//m.def("SetVerticalAlign", wndTextSetVerticalAlign);

	// EditLine
	m.def("SetMax", wndTextSetMax);
	m.def("SetMaxVisible", wndEditSetMaxVisible);
	m.def("Insert", wndEditInsert);
	m.def("Insert", wndEditInsert2);
	m.def("Erase", wndEditErase);
	m.def("Erase", wndEditErase2);
	m.def("GetCursorPosition", wndTextGetCursorPosition);
	m.def("MoveToEnd", wndEditMoveToEnd);
	m.def("SetPlaceholderText", wndEditSetPlaceholderText);
	m.def("SetPlaceholderColor", wndEditSetPlaceholderColor);
	m.def("DisableEditLine", wndEditDisable);
	m.def("EnableEditLine", wndEditEnable);

	// NumberLine
	m.def("SetNumber", wndNumberSetNumber);
	m.def("SetNumberHorizontalAlignCenter", wndNumberSetNumberHorizontalAlignCenter);
	m.def("SetNumberHorizontalAlignRight", wndNumberSetNumberHorizontalAlignRight);
	m.def("SetPath", wndNumberSetPath);

	// MarkBox
	m.def("MarkBox_SetImage", wndMarkBox_SetImage);
	m.def("MarkBox_SetImageFilename", wndMarkBox_SetImageFilename);
	m.def("MarkBox_Load", wndMarkBox_Load);
	m.def("MarkBox_SetIndex", wndMarkBox_SetIndex);
	m.def("MarkBox_SetScale", wndMarkBox_SetScale);
	m.def("MarkBox_SetDiffuseColor", wndMarkBox_SetDiffuseColor);

	// ImageBox + ExpandedImageBox
	m.def("LoadImage", wndImageLoadImage);
	m.def("SetDiffuseColor", wndImageSetDiffuseColor);

	m.def("SetScale", wndImageSetScale);
	m.def("SetOrigin", wndImageSetOrigin);
	m.def("SetRotation", wndImageSetRotation);
	m.def("SetRenderingRect", wndImageSetRenderingRect);
	m.def("SetRenderingMode", wndImageSetRenderingMode);
	m.def("GetWidth", wndImageGetWidth);
	m.def("GetHeight", wndImageGetHeight);
	m.def("SetClipRect", wndSetClipRect);

	// AniImageBox
	m.def("SetDelay", wndImageSetDelay);
	m.def("AppendImage", wndImageAppendImage);
	m.def("StartAnimation", wndAniImageStart);
	m.def("StopAnimation", wndAniImageStop);
	m.def("SetAniImgScale", wndImageSetAniImgScale);
	m.def("SetAniImgDiffuseColor", wndMgrSetAniImgDiffuseColor);
	m.def("SetRenderingRectWithScale", wndImageSetRenderingRectWithScale);
	m.def("ResetFrame", wndImageResetFrame);

	// Button
	m.def("SetUpVisual", wndButtonSetUpVisual);
	m.def("SetOverVisual", wndButtonSetOverVisual);
	m.def("SetDownVisual", wndButtonSetDownVisual);
	m.def("SetDisableVisual", wndButtonSetDisableVisual);
	m.def("GetUpVisualFileName", wndButtonGetUpVisualFileName);
	m.def("GetOverVisualFileName", wndButtonGetOverVisualFileName);
	m.def("GetDownVisualFileName", wndButtonGetDownVisualFileName);
	m.def("Flash", wndButtonFlash);
	m.def("EnableFlash", wndButtonEnableFlash);
	m.def("DisableFlash", wndButtonDisableFlash);
	m.def("Enable", wndButtonEnable);
	m.def("Disable", wndButtonDisable);
	m.def("IsDisable", wndMgrIsDisable);
	m.def("Over", wndMgrOver);
	m.def("LeftRightReverse", wndButtonLeftRightReverse);
	m.def("SetButtonScale", wndButtonSetButtonScale);
	m.def("GetButtonImageWidth", wndButtonGetButtonImageWidth);
	m.def("GetButtonImageHeight", wndButtonGetButtonImageHeight);

	m.def("Down", wndButtonDown);
	m.def("SetUp", wndButtonSetUp);
	m.def("IsDown", wndButtonIsDown);

	// Background
	m.def("SetSlotBackground", wndMgrSetSlotBackground);

	// DragButton + DragBar
	m.def("SetRestrictMovementArea", wndButtonSetRestrictMovementArea);
	m.def("IsPressed", wndDragBarIsPressed);

	// Target Illustrator
	m.def("IllSetCamera", wndMgrIllSetCamera);
	m.def("IllForceShow", wndMgrIllShow);
	m.def("IllForceHide", wndMgrIllHide);
	m.def("IllustrateActor", wndMgrIllIllustrateRace);
	m.def("IllustrateWeapon", wndMgrIllIllustrateWeapon);
	m.def("IllustrateEffect", wndMgrIllIllustrateEffect);

	m.def("IllChangeArmor", wndMgrIllChangeArmor);
	m.def("IllChangeHair", wndMgrIllChangeHair);
	m.def("IllChangeWeapon", wndMgrIllChangeWeapon);
	m.def("IllChangeMotion", wndMgrIllChangeMotion);
	m.def("IllChangeRotation", wndMgrIllChangeRotation);
	m.def("IllForceRotation", wndMgrIllForceRotation);

	m.attr("SLOT_WND_DEFAULT") = (int32_t)SLOT_WND_DEFAULT;
	m.attr("SLOT_WND_INVENTORY") = (int32_t)SLOT_WND_INVENTORY;

	m.attr("SLOT_STYLE_NONE") = (int32_t)SLOT_STYLE_NONE;
	m.attr("SLOT_STYLE_PICK_UP") = (int32_t)SLOT_STYLE_PICK_UP;
	m.attr("SLOT_STYLE_SELECT") = (int32_t)SLOT_STYLE_SELECT;

	m.attr("HORIZONTAL_ALIGN_LEFT") = (int32_t)CWindow::HORIZONTAL_ALIGN_LEFT;
	m.attr("HORIZONTAL_ALIGN_CENTER") = (int32_t)CWindow::HORIZONTAL_ALIGN_CENTER;
	m.attr("HORIZONTAL_ALIGN_RIGHT") = (int32_t)CWindow::HORIZONTAL_ALIGN_RIGHT;
	m.attr("VERTICAL_ALIGN_TOP") = (int32_t)CWindow::VERTICAL_ALIGN_TOP;
	m.attr("VERTICAL_ALIGN_CENTER") = (int32_t)CWindow::VERTICAL_ALIGN_CENTER;
	m.attr("VERTICAL_ALIGN_BOTTOM") = (int32_t)CWindow::VERTICAL_ALIGN_BOTTOM;

	m.attr("RENDERING_MODE_MODULATE") = (int32_t)CGraphicExpandedImageInstance::RENDERING_MODE_MODULATE;
}

