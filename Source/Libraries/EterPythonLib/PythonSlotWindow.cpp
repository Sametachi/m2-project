#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"

class CSlotWindow::CSlotButton : public CButton
{
public:
	enum ESlotButtonType
	{
		SLOT_BUTTON_TYPE_PLUS,
		SLOT_BUTTON_TYPE_COVER
	};

	CSlotButton(ESlotButtonType dwType, uint32_t dwSlotNumber, CSlotWindow* pParent) : CButton()
	{
		m_dwSlotButtonType = dwType;
		m_dwSlotNumber = dwSlotNumber;
		m_pParent = pParent;
	}

	bool OnMouseLeftButtonDown() override
	{
		if (!IsEnable())
			return false;

		if (CWindowManager::GetInstance()->IsAttaching())
			return false;

		m_isPressed = true;
		Down();
		return false;
	}

	bool OnMouseLeftButtonUp() override
	{
		if (!IsEnable())
			return true;

		if (!IsPressed())
			return true;

		if (IsIn())
			SetCurrentVisual(&m_overVisual);
		else
			SetCurrentVisual(&m_upVisual);

		m_pParent->OnPressedSlotButton(m_dwSlotButtonType, m_dwSlotNumber);

		return true;
	}

	bool OnMouseOverIn()
	{
		if (IsEnable())
			SetCurrentVisual(&m_overVisual);

		m_pParent->OnOverInItem(m_dwSlotNumber);
		return true;
	}

	bool OnMouseOverOut() override
	{
		if (IsEnable())
		{
			SetUp();
			SetCurrentVisual(&m_upVisual);
		}
		m_pParent->OnOverOutItem();
		return true;
	}

protected:
	ESlotButtonType m_dwSlotButtonType;
	uint32_t m_dwSlotNumber;
	CSlotWindow* m_pParent;
};

class CSlotWindow::CCoverButton : public CButton
{
public:
	CCoverButton(CSlotWindow* pParent, uint32_t dwSlotNumber) : m_bLeftButtonEnable{ true }, m_bRightButtonEnable{ false }, m_pParent{ pParent }
	{
		m_dwSlotNumber = dwSlotNumber;
	}

	void SetLeftButtonEnable(bool bEnable)
	{
		m_bLeftButtonEnable = bEnable;
	}

	void SetRightButtonEnable(bool bEnable)
	{
		m_bRightButtonEnable = bEnable;
	}

	void OnRender() override
	{
	}

	void RenderButton()
	{
		CButton::OnRender();
	}

	bool OnMouseLeftButtonDown() override
	{
		if (!IsEnable())
			return false;

		if (m_bLeftButtonEnable)
			CButton::OnMouseLeftButtonDown();
		else
			m_pParent->OnMouseLeftButtonDown();

		return false;
	}

	bool OnMouseLeftButtonUp() override
	{
		if (!IsEnable())
			return true;

		if (m_bLeftButtonEnable)
		{
			CButton::OnMouseLeftButtonUp();
			m_pParent->OnMouseLeftButtonDown();
		}
		else
		{
			m_pParent->OnMouseLeftButtonUp();
		}
		return true;
	}

	bool OnMouseRightButtonDown() override
	{
		if (!IsEnable())
			return false;

		if (m_bRightButtonEnable)
		{
			CButton::OnMouseLeftButtonDown();
		}
		return false;
	}

	bool OnMouseRightButtonUp() override
	{
		if (!IsEnable())
			return true;

		m_pParent->OnMouseRightButtonDown();
		if (m_bRightButtonEnable)
		{
			CButton::OnMouseLeftButtonUp();
		}
		return true;
	}

	bool OnMouseOverIn()
	{
		if (IsEnable())
		{
			SetCurrentVisual(&m_overVisual);
		}
		m_pParent->OnOverInItem(m_dwSlotNumber);
		return true;
	}

	bool OnMouseOverOut() override
	{
		if (IsEnable())
		{
			SetUp();
			SetCurrentVisual(&m_upVisual);
		}
		m_pParent->OnOverOutItem();
		return true;
	}

	void SetScale(float fx, float fy);

protected:
	bool m_bLeftButtonEnable;
	bool m_bRightButtonEnable;
	uint32_t m_dwSlotNumber;
	CSlotWindow* m_pParent;
};

void CSlotWindow::CCoverButton::SetScale(float fx, float fy)
{
	if (m_pcurVisual)
	{
		m_pcurVisual->SetScale(fx, fy);
	}

	if (!m_upVisual.IsEmpty())
	{
		m_upVisual.SetScale(fx, fy);
	}

	if (!m_overVisual.IsEmpty())
	{
		m_overVisual.SetScale(fx, fy);
	}

	if (!m_downVisual.IsEmpty())
	{
		m_downVisual.SetScale(fx, fy);
	}

	if (!m_disableVisual.IsEmpty())
	{
		m_disableVisual.SetScale(fx, fy);
	}
}

class CSlotWindow::CCoolTimeFinishEffect : public CAniImageBox
{
public:
	CCoolTimeFinishEffect(CSlotWindow* pParent, uint32_t dwSlotIndex) : CAniImageBox()
	{
		m_parent = pParent;
		m_dwSlotIndex = dwSlotIndex;
	}
	virtual ~CCoolTimeFinishEffect()
	{
	}

	void OnEndFrame()
	{
		((CSlotWindow*)m_parent)->ReserveDestroyCoolTimeFinishEffect(m_dwSlotIndex);
	}

protected:
	uint32_t m_dwSlotIndex;
};


// Set & Append

void CSlotWindow::SetWindowType(int32_t iType)
{
	m_iWindowType = iType;
}

void CSlotWindow::SetSlotType(uint32_t dwType)
{
	m_dwSlotType = dwType;
}

void CSlotWindow::SetSlotStyle(uint32_t dwStyle)
{
	m_dwSlotStyle = dwStyle;
}

void CSlotWindow::SetSlotBaseImageScale(float fx, float fy)
{
	if (m_pBaseImageInstance)
	{
		m_pBaseImageInstance->SetScale(fx, fy);
	}
}

void CSlotWindow::SetScale(float fx, float fy)
{
	m_v2Scale.x = fx;
	m_v2Scale.y = fy;
}

void CSlotWindow::SetCoverButtonScale(uint32_t dwIndex, float fx, float fy)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
	{
		return;
	}

	if (pSlot->pCoverButton)
	{
		pSlot->pCoverButton->SetScale(fx, fy);
	}
}

void CSlotWindow::AppendSlot(uint32_t dwIndex, int32_t ixPosition, int32_t iyPosition, int32_t ixCellSize, int32_t iyCellSize)
{
	TSlot Slot{};
	Slot.pInstance = nullptr;
	Slot.pBackgroundInstance = nullptr;
	Slot.pNumberLine.reset();
	Slot.pCoverButton = nullptr;
	Slot.pSlotButton = nullptr;
	Slot.pSignImage = nullptr;
	Slot.pFinishCoolTimeEffect = nullptr;

	ClearSlot(&Slot);
	Slot.dwSlotNumber = dwIndex;
	Slot.dwCenterSlotNumber = dwIndex;
	Slot.dwRealSlotNumber = dwIndex;
	Slot.dwRealCenterSlotNumber = dwIndex;
	Slot.ixPosition = ixPosition;
	Slot.iyPosition = iyPosition;
	Slot.ixCellSize = ixCellSize;
	Slot.iyCellSize = iyCellSize;
	m_SlotList.emplace_back(std::move(Slot));
}

void CSlotWindow::SetCoverButton(uint32_t dwIndex, const char* c_szUpImageName, const char* c_szOverImageName, const char* c_szDownImageName, const char* c_szDisableImageName, bool bLeftButtonEnable, bool bRightButtonEnable)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	CCoverButton*& rpCoverButton = pSlot->pCoverButton;

	if (!rpCoverButton)
	{
		rpCoverButton = new CCoverButton(this, pSlot->dwSlotNumber);
		CWindowManager::GetInstance()->SetParent(rpCoverButton, this);
	}

	rpCoverButton->SetLeftButtonEnable(bLeftButtonEnable);
	rpCoverButton->SetRightButtonEnable(bRightButtonEnable);
	rpCoverButton->SetUpVisual(c_szUpImageName);
	rpCoverButton->SetOverVisual(c_szOverImageName);
	rpCoverButton->SetDownVisual(c_szDownImageName);
	rpCoverButton->SetDisableVisual(c_szDisableImageName);
	rpCoverButton->Enable();
	rpCoverButton->SetScale(m_v2Scale.x, m_v2Scale.y);
	rpCoverButton->Show();

	if (pSlot->pSlotButton)
		SetTop(pSlot->pSlotButton);
}

void CSlotWindow::DeleteCoverButton(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	CCoverButton*& rpCoverButton = pSlot->pCoverButton;
	if (!rpCoverButton)
		return;

	rpCoverButton->Hide();
	rpCoverButton->Disable();
	rpCoverButton->DestroyHandle();
}

bool CSlotWindow::HasCoverButton(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return false;

	CCoverButton*& rpCoverButton = pSlot->pCoverButton;
	return rpCoverButton ? true : false;
}

void CSlotWindow::EnableCoverButton(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (!pSlot->pCoverButton)
		return;

	pSlot->pCoverButton->Enable();
}

void CSlotWindow::DisableCoverButton(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (!pSlot->pCoverButton)
		return;

	pSlot->pCoverButton->Disable();
}

void CSlotWindow::SetAlwaysRenderCoverButton(uint32_t dwIndex, bool bAlwaysRender)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (bAlwaysRender)
		SET_BIT(pSlot->dwState, SLOT_STATE_ALWAYS_RENDER_COVER);
	else
		REMOVE_BIT(pSlot->dwState, SLOT_STATE_ALWAYS_RENDER_COVER);
}

void CSlotWindow::ShowSlotBaseImage(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->bRenderBaseSlotImage = true;
}

void CSlotWindow::HideSlotBaseImage(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->bRenderBaseSlotImage = false;
}

bool CSlotWindow::IsDisableCoverButton(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return false;

	if (!pSlot->pCoverButton)
		return false;

	return pSlot->pCoverButton->IsDisable();
}

void CSlotWindow::SetSlotBaseImage(const char* c_szFileName, float fr, float fg, float fb, float fa)
{
	__CreateBaseImage(c_szFileName, fr, fg, fb, fa);
}

void CSlotWindow::AppendSlotButton(const char* c_szUpImageName, const char* c_szOverImageName, const char* c_szDownImageName)
{
	for (auto& rSlot : m_SlotList)
	{
		CSlotButton*& rpSlotButton = rSlot.pSlotButton;

		if (!rpSlotButton)
		{
			rpSlotButton = new CSlotButton(CSlotButton::SLOT_BUTTON_TYPE_PLUS, rSlot.dwSlotNumber, this);
			rpSlotButton->AddFlag(FLAG_FLOAT);
			CWindowManager::GetInstance()->SetParent(rpSlotButton, this);
		}

		rpSlotButton->SetUpVisual(c_szUpImageName);
		rpSlotButton->SetOverVisual(c_szOverImageName);
		rpSlotButton->SetDownVisual(c_szDownImageName);
		rpSlotButton->SetPosition(rSlot.ixPosition + 1, rSlot.iyPosition + 19);
		rpSlotButton->Hide();
	}
}

void CSlotWindow::AppendRequirementSignImage(const char* c_szImageName)
{
	for (auto& rSlot : m_SlotList)
	{
		CImageBox*& rpSignImage = rSlot.pSignImage;

		if (!rpSignImage)
		{
			rpSignImage = new CImageBox();
			CWindowManager::GetInstance()->SetParent(rpSignImage, this);
		}

		rpSignImage->LoadImage(c_szImageName);
		rpSignImage->Hide();
	}
}

bool CSlotWindow::HasSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return false;

	return true;
}

void CSlotWindow::SetSlot(uint32_t dwIndex, uint32_t dwVirtualNumber, uint8_t byWidth, uint8_t byHeight, CGraphicImage* pImage, D3DXCOLOR& diffuseColor)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (pSlot->isItem)
	{
		if (pSlot->dwItemIndex == dwVirtualNumber)
		{
			pSlot->dwState = 0;
			pSlot->isItem = true;
			if (pImage && pSlot->pInstance)
			{
				pSlot->pInstance->SetImagePointer(pImage);
				pSlot->pInstance->SetScale(m_v2Scale.x, m_v2Scale.y);
			}
			return;
		}
	}

	ClearSlot(pSlot);
	pSlot->dwState = 0;
	pSlot->isItem = true;
	pSlot->dwItemIndex = dwVirtualNumber;

	if (pImage)
	{
		assert(nullptr == pSlot->pInstance);
		pSlot->pInstance = CGraphicImageInstance::New();
		pSlot->pInstance->SetDiffuseColor(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);
		pSlot->pInstance->SetImagePointer(pImage);
		pSlot->pInstance->SetScale(m_v2Scale.x, m_v2Scale.y);
	}

	pSlot->byxPlacedItemSize = byWidth;
	pSlot->byyPlacedItemSize = byHeight;

	if (pSlot->pCoverButton)
	{
		pSlot->pCoverButton->SetScale(m_v2Scale.x, m_v2Scale.y);
		pSlot->pCoverButton->Show();
	}
}

void CSlotWindow::SetSlotCount(uint32_t dwIndex, uint32_t dwCount)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (dwCount <= 0)
	{
		pSlot->pNumberLine.reset();
	}
	else
	{
		char szCount[16 + 1];
		_snprintf_s(szCount, sizeof(szCount), "%u", dwCount);

		if (!pSlot->pNumberLine)
		{
			auto pNumberLine = std::make_unique<CNumberLine>(this);
			pNumberLine->SetHorizontalAlign(CNumberLine::HORIZONTAL_ALIGN_RIGHT);
			pNumberLine->Show();
			pSlot->pNumberLine = std::move(pNumberLine);
		}

		pSlot->pNumberLine->SetNumber(szCount);
	}
}

void CSlotWindow::SetSlotCountNew(uint32_t dwIndex, uint32_t dwGrade, uint32_t dwCount)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (dwCount <= 0)
	{
		pSlot->pNumberLine.reset();
	}
	else
	{
		char szCount[16 + 1];

		switch (dwGrade)
		{
		case 0:
			_snprintf_s(szCount, sizeof(szCount), "%u", dwCount);
			break;
		case 1:
			_snprintf_s(szCount, sizeof(szCount), "m%u", dwCount);
			break;
		case 2:
			_snprintf_s(szCount, sizeof(szCount), "g%u", dwCount);
			break;
		case 3:
			_snprintf_s(szCount, sizeof(szCount), "p");
			break;
		}

		if (!pSlot->pNumberLine)
		{
			auto pNumberLine = std::make_unique<CNumberLine>(this);
			pNumberLine->SetHorizontalAlign(CNumberLine::HORIZONTAL_ALIGN_RIGHT);
			pNumberLine->Show();
			pSlot->pNumberLine = std::move(pNumberLine);
		}

		pSlot->pNumberLine->SetNumber(szCount);
	}
}

void CSlotWindow::SetRealSlotNumber(uint32_t dwIndex, uint32_t dwSlotRealNumber)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->dwRealSlotNumber = dwSlotRealNumber;
}

void CSlotWindow::SetSlotCoolTime(uint32_t dwIndex, float fCoolTime, float fElapsedTime)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->fCoolTime = fCoolTime;
	pSlot->fStartCoolTime = CTimer::GetInstance()->GetCurrentSecond() - fElapsedTime;
}

void CSlotWindow::ActivateSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->bActive = true;

	if (!m_pSlotActiveEffect)
		__CreateSlotEnableEffect();
}

void CSlotWindow::DeactivateSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->bActive = false;
}

void CSlotWindow::ClearSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	ClearSlot(pSlot);
}

void CSlotWindow::ClearSlot(TSlot* pSlot)
{
	pSlot->bActive = false;
	pSlot->byxPlacedItemSize = 1;
	pSlot->byyPlacedItemSize = 1;

	pSlot->isItem = false;
	pSlot->dwState = 0;
	pSlot->fCoolTime = 0.0f;
	pSlot->fStartCoolTime = 0.0f;
	pSlot->dwCenterSlotNumber = 0xffffffff;
	pSlot->dwRealCenterSlotNumber = 0xffffffff;

	pSlot->dwItemIndex = 0;
	pSlot->bRenderBaseSlotImage = true;

	if (pSlot->pInstance)
	{
		CGraphicImageInstance::Delete(pSlot->pInstance);
		pSlot->pInstance = nullptr;
	}

	if (pSlot->pBackgroundInstance)
	{
		CGraphicImageInstance::Delete(pSlot->pBackgroundInstance);
		pSlot->pBackgroundInstance = nullptr;
	}

	if (pSlot->pCoverButton)
		pSlot->pCoverButton->Hide();
	if (pSlot->pSlotButton)
		pSlot->pSlotButton->Hide();
	if (pSlot->pSignImage)
		pSlot->pSignImage->Hide();
	if (pSlot->pFinishCoolTimeEffect)
		pSlot->pFinishCoolTimeEffect->Hide();
}

void CSlotWindow::ClearAllSlot()
{
	Destroy();
}

void CSlotWindow::RefreshSlot()
{
	OnRefreshSlot();
	if (IsRendering())
	{
		TSlot* pSlot;
		if (GetPickedSlotPointer(&pSlot))
		{
			OnOverOutItem();
			OnOverInItem(pSlot->dwSlotNumber);
		}
	}
}

void CSlotWindow::OnRefreshSlot()
{
}

uint32_t CSlotWindow::GetSlotCount()
{
	return m_SlotList.size();
}

void CSlotWindow::LockSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->dwState |= SLOT_STATE_LOCK;
}

void CSlotWindow::UnlockSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->dwState ^= SLOT_STATE_LOCK;
}

void CSlotWindow::SetCantUseSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->dwState |= SLOT_STATE_CANT_USE;
}

void CSlotWindow::SetCanMouseEventSlot(DWORD dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	REMOVE_BIT(pSlot->dwState, SLOT_STATE_CANT_MOUSE_EVENT);
}

void CSlotWindow::SetCantMouseEventSlot(DWORD dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	SET_BIT(pSlot->dwState, SLOT_STATE_CANT_MOUSE_EVENT);
}

void CSlotWindow::SetUsableSlotOnTopWnd(DWORD dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	REMOVE_BIT(pSlot->dwState, SLOT_STATE_UNUSABLE);
}

void CSlotWindow::SetUnusableSlotOnTopWnd(DWORD dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	SET_BIT(pSlot->dwState, SLOT_STATE_UNUSABLE);
}

void CSlotWindow::SetUseSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	pSlot->dwState ^= SLOT_STATE_CANT_USE;
}

void CSlotWindow::EnableSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	REMOVE_BIT(pSlot->dwState, SLOT_STATE_DISABLE);
}

void CSlotWindow::DisableSlot(uint32_t dwIndex)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;
	SET_BIT(pSlot->dwState, SLOT_STATE_DISABLE);
}

// Select

void CSlotWindow::SelectSlot(uint32_t dwSelectingIndex)
{
	auto itor = m_dwSelectedSlotIndexList.begin();
	for (; itor != m_dwSelectedSlotIndexList.end();)
	{
		if (dwSelectingIndex == *itor)
		{
			m_dwSelectedSlotIndexList.erase(itor);
			return;
		}
		++itor;
	}

	TSlot* pSlot;
	if (GetSlotPointer(dwSelectingIndex, &pSlot))
	{
		if (!pSlot->isItem)
			return;

		m_dwSelectedSlotIndexList.emplace_back(dwSelectingIndex);
	}
}

bool CSlotWindow::isSelectedSlot(uint32_t dwIndex)
{
	auto itor = m_dwSelectedSlotIndexList.begin();
	for (; itor != m_dwSelectedSlotIndexList.end(); ++itor)
	{
		if (dwIndex == *itor)
			return true;
	}

	return false;
}

void CSlotWindow::ClearSelected()
{
	m_dwSelectedSlotIndexList.clear();
}

uint32_t CSlotWindow::GetSelectedSlotCount()
{
	return m_dwSelectedSlotIndexList.size();
}

uint32_t CSlotWindow::GetSelectedSlotNumber(uint32_t dwIndex)
{
	if (dwIndex >= m_dwSelectedSlotIndexList.size())
		return uint32_t(-1);

	uint32_t dwCount = 0;
	auto itor = m_dwSelectedSlotIndexList.begin();
	for (; itor != m_dwSelectedSlotIndexList.end(); ++itor)
	{
		if (dwIndex == dwCount)
			break;

		++dwCount;
	}

	return *itor;
}

void CSlotWindow::ShowSlotButton(uint32_t dwSlotNumber)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwSlotNumber, &pSlot))
		return;

	if (pSlot->pSlotButton)
		pSlot->pSlotButton->Show();
}

void CSlotWindow::HideAllSlotButton()
{
	for (auto& rSlot : m_SlotList)
	{
		if (rSlot.pSlotButton)
			rSlot.pSlotButton->Hide();
	}
}

void CSlotWindow::ShowRequirementSign(uint32_t dwSlotNumber)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwSlotNumber, &pSlot))
		return;

	if (!pSlot->pSignImage)
		return;

	pSlot->pSignImage->Show();
}

void CSlotWindow::HideRequirementSign(uint32_t dwSlotNumber)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwSlotNumber, &pSlot))
		return;

	if (!pSlot->pSignImage)
		return;

	pSlot->pSignImage->Hide();
}

// Event

bool CSlotWindow::OnMouseLeftButtonDown()
{
	TSlot* pSlot;
	if (!GetPickedSlotPointer(&pSlot))
	{
		CWindowManager::GetInstance()->DeattachIcon();
		return true;
	}

	if (!(pSlot->dwState & SLOT_STATE_CANT_MOUSE_EVENT))
		if (pSlot->isItem && !(pSlot->dwState & SLOT_STATE_LOCK))
			OnSelectItemSlot(pSlot->dwSlotNumber);
		else
			OnSelectEmptySlot(pSlot->dwSlotNumber);

	return true;
}

bool CSlotWindow::OnMouseLeftButtonUp()
{
	if (!CWindowManager::GetInstance()->IsAttaching() || !CWindowManager::GetInstance()->IsDragging())
		return false;

	if (!IsIn())
		return false;

	TSlot* pSlot;
	if (!GetPickedSlotPointer(&pSlot))
	{
		CWindowManager::GetInstance()->DeattachIcon();
		return true;
	}

	if (!(pSlot->dwState & SLOT_STATE_CANT_MOUSE_EVENT))
		if (pSlot->isItem)
			OnSelectItemSlot(pSlot->dwSlotNumber);
		else
			OnSelectEmptySlot(pSlot->dwSlotNumber);

	return true;
}

void CSlotWindow::SetSlotID(uint32_t dwIndex, uint32_t dwID)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	if (dwID <= 0)
	{
		return;
	}
	else
	{

		pSlot->dwItemID = dwID;
	}
}

bool CSlotWindow::OnMouseRightButtonDown()
{
	TSlot* pSlot;
	if (!GetPickedSlotPointer(&pSlot))
		return true;

	if (!(pSlot->dwState & SLOT_STATE_CANT_MOUSE_EVENT))
		if (pSlot->isItem)
			OnUnselectItemSlot(pSlot->dwSlotNumber);
		else
			OnUnselectEmptySlot(pSlot->dwSlotNumber);

	return true;
}

bool CSlotWindow::OnMouseLeftButtonDoubleClick()
{
	TSlot* pSlot;

	if (GetPickedSlotPointer(&pSlot))
		if (!(pSlot->dwState & SLOT_STATE_CANT_MOUSE_EVENT))
			OnUseSlot();

	return true;
}

bool CSlotWindow::OnMouseOverOut()
{
	OnOverOutItem();
	return true;
}

void CSlotWindow::OnMouseOver()
{
	CWindow* pPointWindow = CWindowManager::GetInstance()->GetPointWindow();
	if (this == pPointWindow)
	{
		TSlot* pSlot;
		if (GetPickedSlotPointer(&pSlot))
		{
			if (OnOverInItem(pSlot->dwSlotNumber))
			{
				return;
			}
		}
	}

	OnOverOutItem();
}

void CSlotWindow::OnSelectEmptySlot(int32_t iSlotNumber)
{
	RunCallback("OnSelectEmptySlot", iSlotNumber);
}

void CSlotWindow::OnSelectItemSlot(int32_t iSlotNumber)
{
	RunCallback("OnSelectItemSlot", iSlotNumber);

	if (CWindowManager::GetInstance()->IsAttaching())
		OnOverOutItem();
}

void CSlotWindow::OnUnselectEmptySlot(int32_t iSlotNumber)
{
	RunCallback("OnUnselectEmptySlot", iSlotNumber);
}

void CSlotWindow::OnUnselectItemSlot(int32_t iSlotNumber)
{
	RunCallback("OnUnselectItemSlot", iSlotNumber);
}

void CSlotWindow::OnUseSlot()
{
	TSlot* pSlot;
	if (GetPickedSlotPointer(&pSlot))
	{
		if (pSlot->isItem)
			RunCallback("OnUseSlot", pSlot->dwSlotNumber);
	}
}

bool CSlotWindow::OnOverInItem(uint32_t dwSlotNumber)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwSlotNumber, &pSlot))
		return false;

	if (!pSlot->isItem)
		return false;

	if (pSlot->dwSlotNumber == m_dwToolTipSlotNumber)
		return true;

	m_dwToolTipSlotNumber = dwSlotNumber;
	RunCallback("OnOverInItem", dwSlotNumber);

	return true;
}

void CSlotWindow::OnOverOutItem()
{
	if (SLOT_NUMBER_NONE == m_dwToolTipSlotNumber)
		return;

	m_dwToolTipSlotNumber = SLOT_NUMBER_NONE;
	RunCallback("OnOverOutItem");
}

void CSlotWindow::OnPressedSlotButton(uint32_t dwType, uint32_t dwSlotNumber, bool isLeft)
{
	if (CSlotButton::SLOT_BUTTON_TYPE_PLUS == dwType)
	{
		RunCallback("OnPressedSlotButton", dwSlotNumber);
	}
	else if (CSlotButton::SLOT_BUTTON_TYPE_COVER == dwType)
	{
		if (isLeft)
			OnMouseLeftButtonDown();
	}
}

void CSlotWindow::OnUpdate()
{
	for (auto& dwSlotIndex : m_ReserveDestroyEffectDeque)
	{
		TSlot* pSlot;
		if (!GetSlotPointer(dwSlotIndex, &pSlot))
			continue;

		__DestroyFinishCoolTimeEffect(pSlot);

	}
	m_ReserveDestroyEffectDeque.clear();

	if (m_pSlotActiveEffect)
		m_pSlotActiveEffect->Update();
}

void CSlotWindow::OnRender()
{
	RenderSlotBaseImage();

	switch (m_dwSlotStyle)
	{
	case SLOT_STYLE_PICK_UP:
		OnRenderPickingSlot();
		break;
	case SLOT_STYLE_SELECT:
		OnRenderSelectedSlot();
		break;
	}

	for (const auto& window : m_children)
		window->OnRender();

	//
	// Draw all slot boxes
	//////////////////////////////////////////////////////////////////////////
#ifdef __RENDER_SLOT_AREA__
	CPythonGraphic::Instance().SetDiffuseColor(0.5f, 0.5f, 0.5f);
	for (itor = m_SlotList.begin(); itor != m_SlotList.end(); ++itor)
	{
		TSlot& rSlot = *itor;
		CPythonGraphic::Instance().RenderBox2d(m_rect.left + rSlot.ixPosition,
			m_rect.top + rSlot.iyPosition,
			m_rect.left + rSlot.ixPosition + rSlot.ixCellSize,
			m_rect.top + rSlot.iyPosition + rSlot.iyCellSize);
	}
	CPythonGraphic::Instance().SetDiffuseColor(1.0f, 0.0f, 0.0f, 1.0f);
	CPythonGraphic::Instance().RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
#endif
	//////////////////////////////////////////////////////////////////////////

	for (auto& rSlot : m_SlotList)
	{
		if (rSlot.pSlotButton)
			rSlot.pSlotButton->SetPosition(rSlot.ixPosition + 1, rSlot.iyPosition + 19);
		if (rSlot.pSignImage)
			rSlot.pSignImage->SetPosition(rSlot.ixPosition - 7, rSlot.iyPosition + 10);

		if (rSlot.pInstance)
		{
			rSlot.pInstance->SetPosition(m_rect.left + rSlot.ixPosition, m_rect.top + rSlot.iyPosition);
			rSlot.pInstance->Render();
		}

		if (!rSlot.isItem)
		{
			if (IS_SET(rSlot.dwState, SLOT_STATE_ALWAYS_RENDER_COVER))
			{
				rSlot.pCoverButton->Show();
				rSlot.pCoverButton->SetPosition(rSlot.ixPosition, rSlot.iyPosition);
				rSlot.pCoverButton->RenderButton();
			}

			continue;
		}

		if (IS_SET(rSlot.dwState, SLOT_STATE_DISABLE))
		{
			CPythonGraphic::GetInstance()->SetDiffuseColor(1.0f, 0.0f, 0.0f, 0.3f);
			CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + rSlot.ixPosition,
				m_rect.top + rSlot.iyPosition,
				m_rect.left + rSlot.ixPosition + rSlot.ixCellSize,
				m_rect.top + rSlot.iyPosition + rSlot.iyCellSize);
		}

		if (IS_SET(rSlot.dwState, SLOT_STATE_CANT_MOUSE_EVENT))
		{
			CPythonGraphic::GetInstance()->SetDiffuseColor(1.0f, 0.0f, 0.0f, 0.3f);
			CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + rSlot.ixPosition,
				m_rect.top + rSlot.iyPosition,
				m_rect.left + rSlot.ixPosition + rSlot.byxPlacedItemSize * ITEM_WIDTH,
				m_rect.top + rSlot.iyPosition + rSlot.byyPlacedItemSize * ITEM_HEIGHT);
		}

		if (IS_SET(rSlot.dwState, SLOT_STATE_UNUSABLE))
		{
			CPythonGraphic::GetInstance()->SetDiffuseColor(1.0f, 1.0f, 1.0f, 0.3f);
			CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + rSlot.ixPosition,
				m_rect.top + rSlot.iyPosition,
				m_rect.left + rSlot.ixPosition + rSlot.byxPlacedItemSize * ITEM_WIDTH,
				m_rect.top + rSlot.iyPosition + rSlot.byyPlacedItemSize * ITEM_HEIGHT);
		}

		if (rSlot.fCoolTime != 0.0f)
		{
			const float fcurTime = CTimer::GetInstance()->GetCurrentSecond();
			float fPercentage = (fcurTime - rSlot.fStartCoolTime) / rSlot.fCoolTime;
			CPythonGraphic::GetInstance()->RenderCoolTimeBox(m_rect.left + rSlot.ixPosition + 16.0f, m_rect.top + rSlot.iyPosition + 16.0f, 16.0f, fPercentage);

			if (fcurTime - rSlot.fStartCoolTime >= rSlot.fCoolTime)
			{
				if ((fcurTime - rSlot.fStartCoolTime) - rSlot.fCoolTime < 1.0f)
					__CreateFinishCoolTimeEffect(&rSlot);

				rSlot.fCoolTime = 0.0f;
				rSlot.fStartCoolTime = 0.0f;
			}
		}

		if (rSlot.pCoverButton)
		{
			if (rSlot.pCoverButton->GetPositionX() != rSlot.ixPosition || rSlot.pCoverButton->GetPositionY() != rSlot.iyPosition)
				rSlot.pCoverButton->SetPosition(rSlot.ixPosition, rSlot.iyPosition);
			rSlot.pCoverButton->SetScale(m_v2Scale.x, m_v2Scale.y);
			rSlot.pCoverButton->RenderButton();
		}

		if (rSlot.pNumberLine)
		{
			int32_t ix = rSlot.byxPlacedItemSize * ITEM_WIDTH + rSlot.ixPosition - 4;
			int32_t iy = rSlot.iyPosition + rSlot.byyPlacedItemSize * ITEM_HEIGHT - 12 + 2;

			if (rSlot.pNumberLine->GetPositionX() != ix || rSlot.pNumberLine->GetPositionY() != iy)
				rSlot.pNumberLine->SetPosition(ix, iy);
			rSlot.pNumberLine->UpdateRect();
			rSlot.pNumberLine->Update();
			rSlot.pNumberLine->Render();
		}

		if (rSlot.pFinishCoolTimeEffect)
		{
			rSlot.pFinishCoolTimeEffect->SetPosition(rSlot.ixPosition, rSlot.iyPosition);
			rSlot.pFinishCoolTimeEffect->Update();
			rSlot.pFinishCoolTimeEffect->Render();
		}

		if (rSlot.bActive)
		{
			if (m_pSlotActiveEffect && rSlot.byyPlacedItemSize == 1)
			{
				int32_t ix = m_rect.left + rSlot.ixPosition;
				int32_t iy = m_rect.top + rSlot.iyPosition;
				m_pSlotActiveEffect->SetPosition(ix, iy);
				m_pSlotActiveEffect->Render();
			}
		}
	}

	RenderLockedSlot();
}

void CSlotWindow::RenderSlotBaseImage()
{
	if (!m_pBaseImageInstance)
		return;

	for (auto& rSlot : m_SlotList)
	{
		if (!rSlot.bRenderBaseSlotImage)
			continue;

		CGraphicImageInstance* bgImageInstance = rSlot.pBackgroundInstance ? rSlot.pBackgroundInstance : m_pBaseImageInstance;

		bgImageInstance->SetPosition(m_rect.left + rSlot.ixPosition, m_rect.top + rSlot.iyPosition);
		bgImageInstance->Render();
	}
}

void CSlotWindow::OnRenderPickingSlot()
{
	if (!CWindowManager::GetInstance()->IsAttaching())
		return;

	TSlot* pSlot;
	if (!GetSelectedSlotPointer(&pSlot))
		return;

	CPythonGraphic::GetInstance()->SetDiffuseColor(1.0f, 1.0f, 1.0f, 0.5f);
	CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + pSlot->ixPosition,
		m_rect.top + pSlot->iyPosition,
		m_rect.left + pSlot->ixPosition + pSlot->ixCellSize,
		m_rect.top + pSlot->iyPosition + pSlot->iyCellSize);
}

void CSlotWindow::OnRenderSelectedSlot()
{
	auto itor = m_dwSelectedSlotIndexList.begin();
	for (; itor != m_dwSelectedSlotIndexList.end(); ++itor)
	{
		TSlot* pSlot;
		if (!GetSlotPointer(*itor, &pSlot))
			continue;

		CPythonGraphic::GetInstance()->SetDiffuseColor(1.0f, 1.0f, 1.0f, 0.5f);
		CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + pSlot->ixPosition,
			m_rect.top + pSlot->iyPosition,
			m_rect.left + pSlot->ixPosition + pSlot->ixCellSize,
			m_rect.top + pSlot->iyPosition + pSlot->iyCellSize);
	}
}

void CSlotWindow::RenderLockedSlot()
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(0.0f, 0.0f, 0.0f, 0.5f);
	for (auto& rSlot : m_SlotList)
	{
		if (!rSlot.isItem)
			continue;

		if (rSlot.dwState & SLOT_STATE_LOCK)
		{
			CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left + rSlot.ixPosition,
				m_rect.top + rSlot.iyPosition,
				m_rect.left + rSlot.ixPosition + rSlot.ixCellSize,
				m_rect.top + rSlot.iyPosition + rSlot.iyCellSize);
		}
	}
}

// Check Slot

bool CSlotWindow::GetSlotPointer(uint32_t dwIndex, TSlot** ppSlot)
{
	for (auto& rSlot : m_SlotList)
	{
		if (dwIndex == rSlot.dwSlotNumber)
		{
			*ppSlot = &rSlot;
			return true;
		}
	}

	return false;
}

bool CSlotWindow::GetSelectedSlotPointer(TSlot** ppSlot)
{
	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	MakeLocalPosition(lx, ly);

	for (TSlot& rSlot : m_SlotList)
	{
		// Brainstorm
		if (lx >= rSlot.ixPosition && ly >= rSlot.iyPosition && lx <= rSlot.ixPosition + rSlot.ixCellSize && ly <= rSlot.iyPosition + rSlot.iyCellSize)
		{
			*ppSlot = &rSlot;
			return true;
		}
	}

	return false;
}

bool CSlotWindow::GetPickedSlotPointer(TSlot** ppSlot)
{
	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();

	int32_t ixLocal = lx - m_rect.left;
	int32_t iyLocal = ly - m_rect.top;

	for (auto& rSlot : m_SlotList)
	{
		int32_t ixCellSize = rSlot.ixCellSize;
		int32_t iyCellSize = rSlot.iyCellSize;

		if (rSlot.isItem)
		{
			ixCellSize = std::max<int32_t>(rSlot.ixCellSize, int32_t(rSlot.byxPlacedItemSize * ITEM_WIDTH));
			iyCellSize = std::max<int32_t>(rSlot.iyCellSize, int32_t(rSlot.byyPlacedItemSize * ITEM_HEIGHT));
		}

		if (ixLocal >= rSlot.ixPosition && iyLocal >= rSlot.iyPosition && ixLocal <= rSlot.ixPosition + ixCellSize && iyLocal <= rSlot.iyPosition + iyCellSize)
		{
			*ppSlot = &rSlot;
			return true;
		}
	}

	return false;
}

void CSlotWindow::SetUseMode(bool bFlag)
{
	m_isUseMode = bFlag;
}

void CSlotWindow::SetUsableItem(bool bFlag)
{
	m_isUsableItem = bFlag;
}

void CSlotWindow::ReserveDestroyCoolTimeFinishEffect(uint32_t dwSlotIndex)
{
	m_ReserveDestroyEffectDeque.emplace_back(dwSlotIndex);
}

uint32_t CSlotWindow::Type()
{
	return kWindowSlot;
}

bool CSlotWindow::OnIsType(uint32_t dwType)
{
	if (Type() == dwType)
		return true;

	return CWindow::OnIsType(dwType);
}

void CSlotWindow::SetSlotBackground(uint32_t dwIndex, const char* c_szFileName)
{
	TSlot* pSlot;
	if (!GetSlotPointer(dwIndex, &pSlot))
		return;

	//Destroy old one first
	if (pSlot->pBackgroundInstance)
	{
		CGraphicImageInstance::Delete(pSlot->pBackgroundInstance);
		pSlot->pBackgroundInstance = nullptr;
	}

	CGraphicImage* pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	pSlot->pBackgroundInstance = CGraphicImageInstance::New();
	pSlot->pBackgroundInstance->SetImagePointer(pImage);
}

void CSlotWindow::__CreateToggleSlotImage()
{
	__DestroyToggleSlotImage();

	m_pToggleSlotImage = new CImageBox();
	m_pToggleSlotImage->LoadImage("d:/ymir work/ui/public/slot_toggle.sub");
	m_pToggleSlotImage->Show();
}

void CSlotWindow::__CreateSlotEnableEffect()
{
	__DestroySlotEnableEffect();

	m_pSlotActiveEffect = new CAniImageBox();
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/00.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/01.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/02.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/03.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/04.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/05.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/06.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/07.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/08.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/09.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/10.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/11.sub");
	m_pSlotActiveEffect->AppendImage("d:/ymir work/ui/public/slotactiveeffect/12.sub");
	m_pSlotActiveEffect->SetRenderingMode(CGraphicExpandedImageInstance::RENDERING_MODE_SCREEN);
	m_pSlotActiveEffect->Show();
}

void CSlotWindow::__CreateFinishCoolTimeEffect(TSlot* pSlot)
{
	__DestroyFinishCoolTimeEffect(pSlot);

	CAniImageBox* pFinishCoolTimeEffect = new CCoolTimeFinishEffect(this, pSlot->dwSlotNumber);
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/00.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/01.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/02.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/03.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/04.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/05.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/06.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/07.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/08.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/09.sub");
	pFinishCoolTimeEffect->AppendImage("d:/ymir work/ui/public/slotfinishcooltimeeffect/10.sub");
	pFinishCoolTimeEffect->SetRenderingMode(CGraphicExpandedImageInstance::RENDERING_MODE_SCREEN);
	pFinishCoolTimeEffect->ResetFrame();
	pFinishCoolTimeEffect->SetDelay(2);
	pFinishCoolTimeEffect->Show();

	pSlot->pFinishCoolTimeEffect = pFinishCoolTimeEffect;
}

void CSlotWindow::__CreateBaseImage(const char* c_szFileName, float fr, float fg, float fb, float fa)
{
	__DestroyBaseImage();

	CGraphicImage* pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_pBaseImageInstance = CGraphicImageInstance::New();
	m_pBaseImageInstance->SetImagePointer(pImage);
	m_pBaseImageInstance->SetDiffuseColor(fr, fg, fb, fa);
}

void CSlotWindow::__CreateBaseImageScale(const char* c_szFileName, float fr, float fg, float fb, float fa, float sx, float sy)
{
	__DestroyBaseImage();

	CGraphicImage* pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_pBaseImageInstance = CGraphicImageInstance::New();
	m_pBaseImageInstance->SetImagePointer(pImage);
	m_pBaseImageInstance->SetDiffuseColor(fr, fg, fb, fa);
	m_pBaseImageInstance->SetScale(sx, sy);
}

void CSlotWindow::__DestroyToggleSlotImage()
{
	if (m_pToggleSlotImage)
	{
		delete m_pToggleSlotImage;
		m_pToggleSlotImage = nullptr;
	}
}

void CSlotWindow::__DestroySlotEnableEffect()
{
	if (m_pSlotActiveEffect)
	{
		delete m_pSlotActiveEffect;
		m_pSlotActiveEffect = nullptr;
	}
}

void CSlotWindow::__DestroyFinishCoolTimeEffect(TSlot* pSlot)
{
	if (pSlot->pFinishCoolTimeEffect)
	{
		delete pSlot->pFinishCoolTimeEffect;
		pSlot->pFinishCoolTimeEffect = nullptr;
	}
}

void CSlotWindow::__DestroyBaseImage()
{
	if (m_pBaseImageInstance)
	{
		CGraphicImageInstance::Delete(m_pBaseImageInstance);
		m_pBaseImageInstance = nullptr;
	}
}

void CSlotWindow::__Initialize()
{
	m_dwSlotType = 0;
	m_iWindowType = SLOT_WND_DEFAULT;
	m_dwSlotStyle = SLOT_STYLE_PICK_UP;
	m_dwToolTipSlotNumber = SLOT_NUMBER_NONE;

	m_isUseMode = false;
	m_isUsableItem = false;

	m_pToggleSlotImage = nullptr;
	m_pSlotActiveEffect = nullptr;
	m_pBaseImageInstance = nullptr;
	m_v2Scale.y = 1.0f;
	m_v2Scale.x = 1.0f;
}

void CSlotWindow::Destroy()
{
	for (auto& rSlot : m_SlotList)
	{
		ClearSlot(&rSlot);

		rSlot.pNumberLine.reset();

		if (rSlot.pCoverButton)
			CWindowManager::GetInstance()->DestroyWindow(rSlot.pCoverButton);
		if (rSlot.pSlotButton)
			CWindowManager::GetInstance()->DestroyWindow(rSlot.pSlotButton);
		if (rSlot.pSignImage)
			CWindowManager::GetInstance()->DestroyWindow(rSlot.pSignImage);
		if (rSlot.pFinishCoolTimeEffect)
			CWindowManager::GetInstance()->DestroyWindow(rSlot.pFinishCoolTimeEffect);
	}

	m_SlotList.clear();

	__DestroyToggleSlotImage();
	__DestroySlotEnableEffect();
	__DestroyBaseImage();

	__Initialize();
}

CSlotWindow::CSlotWindow() : CWindow()
{
	__Initialize();
}

CSlotWindow::~CSlotWindow()
{
	Destroy();
}
