#pragma once
#include "PythonWindow.h"

enum
{
	ITEM_WIDTH = 32,
	ITEM_HEIGHT = 32,

	SLOT_NUMBER_NONE = 0xffffffff
};

enum EHighlightType
{
	HILIGHTSLOT_ACCE,
};

enum ESlotStyle
{
	SLOT_STYLE_NONE,
	SLOT_STYLE_PICK_UP,
	SLOT_STYLE_SELECT
};

enum ESlotState
{
	SLOT_STATE_LOCK = (1 << 0),
	SLOT_STATE_CANT_USE = (1 << 1),
	SLOT_STATE_DISABLE = (1 << 2),
	SLOT_STATE_ALWAYS_RENDER_COVER = (1 << 3),
	SLOT_STATE_CANT_MOUSE_EVENT = (1 << 4),
	SLOT_STATE_UNUSABLE = (1 << 5),
};

enum EWindowType
{
	SLOT_WND_DEFAULT,
	SLOT_WND_INVENTORY
};

class CSlotWindow : public CWindow
{
public:
	static uint32_t Type();

public:
	class CSlotButton;
	class CCoverButton;
	class CCoolTimeFinishEffect;

	friend class CSlotButton;
	friend class CCoverButton;

	typedef struct SSlot
	{
		uint32_t dwState;
		uint32_t dwSlotNumber;
		uint32_t dwCenterSlotNumber;
		uint32_t dwRealSlotNumber;
		uint32_t dwRealCenterSlotNumber;
		uint32_t dwItemIndex;
		bool isItem;
		uint32_t dwItemID;

		// CoolTime
		float fCoolTime;
		float fStartCoolTime;

		// Toggle
		bool bActive;

		int32_t ixPosition;
		int32_t iyPosition;

		int32_t ixCellSize;
		int32_t iyCellSize;

		uint8_t	byxPlacedItemSize;
		uint8_t	byyPlacedItemSize;

		CGraphicImageInstance* pInstance;
		CGraphicImageInstance* pBackgroundInstance;
		std::unique_ptr<CNumberLine> pNumberLine;

		bool bRenderBaseSlotImage;
		CCoverButton* pCoverButton;
		CSlotButton* pSlotButton;
		CImageBox* pSignImage;
		CAniImageBox* pFinishCoolTimeEffect;
	} TSlot;
	using TSlotList = std::list<TSlot>;
	using TSlotListIterator = TSlotList::iterator;

public:
	CSlotWindow();
	virtual ~CSlotWindow();

	void SetSlotID(uint32_t dwIndex, uint32_t dwID);

	void Destroy();
	void SetWindowType(int32_t iType);

	// Manage Slot
	void SetSlotType(uint32_t dwType);
	void SetSlotStyle(uint32_t dwStyle);

	void AppendSlot(uint32_t dwIndex, int32_t ixPosition, int32_t iyPosition, int32_t ixCellSize, int32_t iyCellSize);
	void SetCoverButton(uint32_t dwIndex, const char* c_szUpImageName, const char* c_szOverImageName, const char* c_szDownImageName, const char* c_szDisableImageName, bool bLeftButtonEnable, bool bRightButtonEnable);

	void SetSlotBaseImage(const char* c_szFileName, float fr, float fg, float fb, float fa);

	void AppendSlotButton(const char* c_szUpImageName, const char* c_szOverImageName, const char* c_szDownImageName);
	void AppendRequirementSignImage(const char* c_szImageName);

	void DeleteCoverButton(uint32_t dwIndex);
	bool HasCoverButton(uint32_t dwIndex);

	void EnableCoverButton(uint32_t dwIndex);
	void DisableCoverButton(uint32_t dwIndex);
	void SetAlwaysRenderCoverButton(uint32_t dwIndex, bool bAlwaysRender = false);

	void ShowSlotBaseImage(uint32_t dwIndex);
	void HideSlotBaseImage(uint32_t dwIndex);
	bool IsDisableCoverButton(uint32_t dwIndex);
	bool HasSlot(uint32_t dwIndex);

	void ClearAllSlot();
	void ClearSlot(uint32_t dwIndex);
	void SetSlot(uint32_t dwIndex, uint32_t dwVirtualNumber, uint8_t byWidth, uint8_t byHeight, CGraphicImage* pImage, D3DXCOLOR& diffuseColor);
	void SetSlotCount(uint32_t dwIndex, uint32_t dwCount);
	void SetSlotCountNew(uint32_t dwIndex, uint32_t dwGrade, uint32_t dwCount);
	void SetRealSlotNumber(uint32_t dwIndex, uint32_t dwID);
	void SetSlotCoolTime(uint32_t dwIndex, float fCoolTime, float fElapsedTime = 0.0f);

	void ActivateSlot(uint32_t dwIndex);
	void DeactivateSlot(uint32_t dwIndex);
	void RefreshSlot();

	uint32_t GetSlotCount();

	void LockSlot(uint32_t dwIndex);
	void UnlockSlot(uint32_t dwIndex);
	void SetCantUseSlot(uint32_t dwIndex);
	void SetCanMouseEventSlot(DWORD dwIndex);
	void SetCantMouseEventSlot(DWORD dwIndex);
	void SetUsableSlotOnTopWnd(DWORD dwIndex);
	void SetUnusableSlotOnTopWnd(DWORD dwIndex);
	void SetUseSlot(uint32_t dwIndex);
	void EnableSlot(uint32_t dwIndex);
	void DisableSlot(uint32_t dwIndex);

	// Select
	void ClearSelected();
	void SelectSlot(uint32_t dwSelectingIndex);
	bool isSelectedSlot(uint32_t dwIndex);
	uint32_t GetSelectedSlotCount();
	uint32_t GetSelectedSlotNumber(uint32_t dwIndex);

	// Slot Button
	void ShowSlotButton(uint32_t dwSlotNumber);
	void HideAllSlotButton();
	void OnPressedSlotButton(uint32_t dwType, uint32_t dwSlotNumber, bool isLeft = true);

	// Slot background			
	void SetSlotBackground(uint32_t dwIndex, const char* c_szFileName);

	// Requirement Sign
	void ShowRequirementSign(uint32_t dwSlotNumber);
	void HideRequirementSign(uint32_t dwSlotNumber);

	// ToolTip
	bool OnOverInItem(uint32_t dwSlotNumber);
	void OnOverOutItem();

	// For Usable Item
	void SetUseMode(bool bFlag);
	void SetUsableItem(bool bFlag);

	// For Scale
	void SetSlotBaseImageScale(float fx, float fy);
	void SetScale(float fx, float fy);
	void SetCoverButtonScale(uint32_t dwIndex, float fx, float fy);

	// CallBack
	void ReserveDestroyCoolTimeFinishEffect(uint32_t dwSlotIndex);

protected:
	void __Initialize();
	void __CreateToggleSlotImage();
	void __CreateSlotEnableEffect();
	void __CreateFinishCoolTimeEffect(TSlot* pSlot);
	void __CreateBaseImage(const char* c_szFileName, float fr, float fg, float fb, float fa);
	void __CreateBaseImageScale(const char* c_szFileName, float fr, float fg, float fb, float fa, float sx, float sy);

	void __DestroyToggleSlotImage();
	void __DestroySlotEnableEffect();
	void __DestroyFinishCoolTimeEffect(TSlot* pSlot);
	void __DestroyBaseImage();

	// Event
	void OnUpdate();
	void OnRender();
	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonUp();
	bool OnMouseRightButtonDown();
	bool OnMouseLeftButtonDoubleClick();
	bool OnMouseOverOut();
	void OnMouseOver();
	void RenderSlotBaseImage();
	void RenderLockedSlot();
	virtual void OnRenderPickingSlot();
	virtual void OnRenderSelectedSlot();

	// Select
	void OnSelectEmptySlot(int32_t iSlotNumber);
	void OnSelectItemSlot(int32_t iSlotNumber);
	void OnUnselectEmptySlot(int32_t iSlotNumber);
	void OnUnselectItemSlot(int32_t iSlotNumber);
	void OnUseSlot();

	// Manage Slot
	bool GetSlotPointer(uint32_t dwIndex, TSlot** ppSlot);
	bool GetSelectedSlotPointer(TSlot** ppSlot);
	virtual bool GetPickedSlotPointer(TSlot** ppSlot);
	void ClearSlot(TSlot* pSlot);
	virtual void OnRefreshSlot();

	// ETC
	bool OnIsType(uint32_t dwType);

protected:
	int32_t m_iWindowType;
	uint32_t m_dwSlotType;
	uint32_t m_dwSlotStyle;
	std::list<uint32_t> m_dwSelectedSlotIndexList;
	TSlotList m_SlotList;
	uint32_t m_dwToolTipSlotNumber;

	bool m_isUseMode;
	bool m_isUsableItem;

	CGraphicImageInstance* m_pBaseImageInstance;
	CImageBox* m_pToggleSlotImage;
	CAniImageBox* m_pSlotActiveEffect;
	std::deque<uint32_t> m_ReserveDestroyEffectDeque;
	D3DXVECTOR2 m_v2Scale;
};