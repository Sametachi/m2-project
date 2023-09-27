#pragma once
#include "PythonWindow.h"

class CWindow;
class CTextLine;

class CWindowManager : public Singleton<CWindowManager>
{
public:
	typedef std::list<CWindow*> WindowList;

	CWindowManager();
	virtual ~CWindowManager();
	void Destroy();

	int32_t GetScreenWidth() const { return m_lWidth; }
	int32_t GetScreenHeight() const { return m_lHeight; }
	void SetScreenSize(int32_t lWidth, int32_t lHeight);
	void SetResolution(int32_t hres, int32_t vres);
	float GetAspect();

	// Mouse Handler/Position and Drag
	void SetMouseHandler(pybind11::handle poMouseHandler);
	std::tuple<int32_t, int32_t> GetMousePosition();
	bool IsDragging();

	// Register and Destroy Classes
	template <typename pyRegister>
	pyRegister* RegisterWindow(pybind11::handle m_po, const std::string& layer);
	void DestroyWindow(CWindow* pWin);

	// Attaching Icon
	bool IsAttaching();
	uint32_t GetAttachingType();
	uint32_t GetAttachingIndex();
	uint32_t GetAttachingSlotNumber();
	uint32_t GetAttachingRealSlotNumber();
	void GetAttachingIconSize(uint8_t* pbyWidth, uint8_t* pbyHeight);
	void AttachIcon(uint32_t dwType, uint32_t dwIndex, uint32_t dwSlotNumber, uint8_t byWidth, uint8_t byHeight);
	void DeattachIcon();
	void SetAttachingFlag(bool bFlag);
	void SetAttachingRealSlotNumber(uint32_t dwRealslotNumber);
	// Attaching Icon end

	// Application Window
	HWND GetAppWindow() const { return m_pAppWindow; }
	void SetAppWindow(HWND wnd) { m_pAppWindow = wnd; }

	// PointWindow
	CWindow* GetPointWindow() const { return m_pPointWindow; }

	// Lock/Unlock Window
	void LockWindow(CWindow* pWin);
	void UnlockWindow();
	CWindow* GetLockWindow() const { return m_pLockWindow; }

	// Focus Window
	void SetFocus(CWindow* pWin);
	void KillFocus();
	CWindow* GetFocus() const { return m_pFocusWindow; }

	// Mouse Capture
	void CaptureMouse(CWindow* pWin);
	void ReleaseMouse(CWindow* pWin);
	void ReleaseMouse();
	CWindow* GetCapture() const { return m_pCaptureWindow; }

	// Clipping
	void PopClipRegion();
	bool CommitClipRegion();
	bool PushClipRegion(const D3DXVECTOR4& clip_rect);
	bool PushClipRegion(CWindow* pWin);

	// Parent and Top
	void SetParent(CWindow* pWin, CWindow* pParentWindow);
	void SetTop(CWindow* pWin);

	void Update(); // OnUpdate
	void Render(); // OnRender

	// Mouse events
	void RunMouseMove(int32_t x, int32_t y);
	void RunMouseLeftButtonDown();
	void RunMouseLeftButtonUp();
	void RunMouseLeftButtonDoubleClick();
	void RunMouseRightButtonDown();
	void RunMouseRightButtonUp();
	void RunMouseRightButtonDoubleClick();
	void RunMouseMiddleButtonDown();
	void RunMouseMiddleButtonUp();
	bool RunMouseWheelEvent(int32_t nLen); // OnMouseWheelEvent

	// Keyboard Events
	void RunChar(uint32_t ch);		//OnChar event (For input)
	void RunKeyDown(KeyCode code);	//OnKeyDown event
	void RunKeyUp(KeyCode code);	//OnKeyUp event
	void RunTab();					//OnTab event
	void RunPressEscapeKey();		//OnPressEscKey event
	void RunPressExitKey();			//OnPressExitKey event

	void NotifyHideWindow(CWindow* win);

	D3DXVECTOR2 PixelToClip() const
	{
		D3DXVECTOR2 res(GetScreenWidth(), GetScreenHeight());

		res.x = 2.f / res.x;
		res.y = 2.f / res.y;
		return res;
	};

private:
	// Point on a Window
	CWindow* PickWindow(int32_t x, int32_t y);

	// Point on a Window by Position
	void SetPointedWindowByPos(int32_t x, int32_t y);

	// Remove the captured events in que
	void EventDeletionQueue();

	// Get the Layer name
	CWindow* GetLayerByName(const std::string& name);

	// Set window to TOP
	void SetTopRecursive(CWindow* pWin);

private:
	// Positions mostly, also horizontal/vertical pos
	int32_t m_lWidth;
	int32_t m_lHeight;
	int32_t m_iVres;
	int32_t m_iHres;
	int32_t m_lMouseX, m_lMouseY;
	int32_t m_lDragX, m_lDragY;
	int32_t m_lPickedX, m_lPickedY;

	// Attaching Icon
	pybind11::handle m_poMouseHandler;
	bool		m_bAttachingFlag;
	uint32_t	m_dwAttachingType;
	uint32_t	m_dwAttachingIndex;
	uint32_t	m_dwAttachingSlotNumber;
	uint32_t	m_dwAttachingRealSlotNumber;
	uint8_t		m_byAttachingIconWidth;
	uint8_t		m_byAttachingIconHeight;

	// Main Focus Window and it's list
	CWindow* m_pFocusWindow;
	WindowList	m_FocusWindowList;

	// Locked windows (Other windows are unresponsive until it exist!)
	CWindow* m_pLockWindow;
	WindowList	m_LockWindowList;

	// Mouse-focus capture
	CWindow* m_pCaptureWindow;
	WindowList	m_CaptureWindowList;

	// OnMouse*ButtonUp needs to be called on the same window which received the OnMouse*ButtonDown event (except when some other window captures mouse-events).
	CWindow* m_pLeftCaptureWindow;
	CWindow* m_pRightCaptureWindow;
	CWindow* m_pMiddleCaptureWindow;

	// OnKeyUp events need to be routed to the same window which received the OnKeyDown event. This map remembers these windows.
	std::unordered_map<KeyCode, CWindow*> m_keyHandlers;

	HWND	m_pAppWindow;

	// Currently pointed window
	CWindow* m_pPointWindow;

	// Our root window which has every Children inside
	std::unique_ptr<CWindow> m_pRootWindow;

	// Captured events will be removed over time
	WindowList m_lazyDeleteQueue;

	// Clipping Vector4 because of texts!
	using ClipStack = std::stack<D3DXVECTOR4, std::vector<D3DXVECTOR4>>;
	ClipStack clipStack_;
};

template <typename pyRegister>
pyRegister* CWindowManager::RegisterWindow(pybind11::handle m_po, const std::string& layer)
{
	CWindow* layerWnd = GetLayerByName(layer);
	pyRegister* pWin = new pyRegister();
	pWin->SetHandler(m_po);
	layerWnd->AddChild(pWin);
	return pWin;
}
