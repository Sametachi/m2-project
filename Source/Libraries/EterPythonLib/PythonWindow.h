#pragma once
#include <boost/container/small_vector.hpp>
#include <Tweeny/easing.h>
#include <Tweeny/tweeny.h>
#include <EterLib/KeyboardInput.h>

#ifdef LoadImage
#undef LoadImage
#endif
//#include "../EterLib/GrpRenderTargetTexture.h"
#include "../EterBase/Utils.h"

class CWindow
{
public:
	// Positioning
	enum EHorizontalAlign
	{
		HORIZONTAL_ALIGN_LEFT = 0,
		HORIZONTAL_ALIGN_CENTER = 1,
		HORIZONTAL_ALIGN_RIGHT = 2
	};

	enum EVerticalAlign
	{
		VERTICAL_ALIGN_TOP = 0,
		VERTICAL_ALIGN_CENTER = 1,
		VERTICAL_ALIGN_BOTTOM = 2
	};

	typedef boost::container::small_vector<CWindow*, 10> WindowList;
	static uint32_t Type();
	bool IsType(uint32_t dwType);

	CWindow();
	virtual ~CWindow();

	void AddChild(CWindow* pWin);
	void Clear();
	void DestroyHandle();

	void Update();
	void Render();

	void SetName(const char* name) { m_strName = name; }
	const char* GetName() { return m_strName.c_str(); }

	void SetSize(int32_t width, int32_t height);
	void SetScale(float fx, float fy);
	int32_t	GetWidth() { return m_lWidth; }
	int32_t	GetHeight() { return m_lHeight; }

	void SetHandler(pybind11::handle handler) { m_poHandler = handler; }
	pybind11::handle GetHandler() const { return m_poHandler; }

	void SetHorizontalAlign(uint32_t dwAlign);
	void SetVerticalAlign(uint32_t dwAlign);

	void SetPosition(int32_t x, int32_t y);
	int32_t GetPositionX() const { return m_x; }
	int32_t GetPositionY() const { return m_y; }

	RECT& GetRect() { return m_rect; }
	const RECT& GetClipRect() const { return m_clipRect; }
	void SetClipRect(const RECT& rect) { m_clipRect = rect; }

	void UpdateRect();
	void MakeLocalPosition(int32_t& rlx, int32_t& rly);

	void Show();
	void Hide();

	bool IsShow() const;
	bool IsRendering();

	bool HasParent() { return m_parent ? true : false; }
	bool HasChild() { return m_children.empty() ? false : true; }

	const WindowList& GetChildren() const { return m_children; }
	size_t GetChildrenCount() { return m_children.size(); }

	void IsTransparentOnPixel(int32_t* x, int32_t* y, bool* ret);
	CWindow* GetRoot();
	CWindow* GetParent();
	void SetParentForce(CWindow* parent) { m_parent = parent; }

	bool IsChild(CWindow* pWin);
	void DeleteChild(CWindow* pWin, bool clearParent = true);
	void SetTop(CWindow* pWin);

	bool IsIn(int32_t x, int32_t y);
	bool IsIn();

	CWindow* PickWindow(int32_t x, int32_t y);
	CWindow* PickTopWindow(int32_t x, int32_t y);

	bool HasClippingEnabled() const { return m_enableClipping; }
	void EnableClipping() { m_enableClipping = true; }
	void DisableClipping() { m_enableClipping = false; }

	void AddFlag(uint32_t flag) { SET_BIT(m_dwFlag, flag); }
	void RemoveFlag(uint32_t flag) { REMOVE_BIT(m_dwFlag, flag); }
	bool IsFlag(uint32_t flag) { return (m_dwFlag & flag) ? true : false; }
	/////////////////////////////////////

	void SetSingleAlpha(float fAlpha);
	virtual void SetAlpha(float fAlpha) { m_fWindowAlpha = fAlpha; }
	virtual float GetAlpha() const { return m_fWindowAlpha; }
	void SetAllAlpha(float fAlpha);

	// Window events
	virtual void SetColor(uint32_t dwColor) {}
	virtual bool OnIsType(uint32_t dwType);
	virtual bool IsWindow() { return true; }
	virtual void OnRender();
	virtual void OnUpdate();
	virtual void OnChangePosition() { RunCallback("OnChangePosition"); }

	virtual void OnSetFocus();
	virtual void OnKillFocus();

	virtual void OnMoveWindow(int32_t x, int32_t y);
	virtual void OnMouseDrag(int32_t lx, int32_t ly);
	virtual bool OnMouseOverIn();
	virtual bool OnMouseOverOut();
	virtual void OnMouseOver();
	virtual void OnDrop();
	virtual void OnTop();

	// Mouse events
	virtual bool OnMouseLeftButtonDown();
	virtual bool OnMouseLeftButtonUp();
	virtual bool OnMouseLeftButtonDoubleClick();
	virtual bool OnMouseRightButtonDown();
	virtual bool OnMouseRightButtonUp();
	virtual bool OnMouseRightButtonDoubleClick();
	virtual bool OnMouseMiddleButtonDown();
	virtual bool OnMouseMiddleButtonUp();
	virtual bool OnMouseWheelEvent(int32_t nLen);

	// Keyboard events
	virtual bool OnChar(uint32_t ch);
	virtual bool OnKeyDown(KeyCode ikey);
	virtual bool OnKeyUp(KeyCode ikey);
	virtual bool OnTab();
	virtual bool OnPressEscapeKey();
	virtual bool OnPressExitKey();
	virtual void OnDestroy();

	///////////////////////////////////////

protected:
	/*Implementation of PythonStringUtil -> Overriding empty tuple*/
	template <typename... Args>
	bool RunCallback(const std::string& name, Args&&... args)
	{
		return PyCallClassMemberFunc(m_poHandler, name, std::forward<Args>(args)...);
	}


	std::string m_strName;

	EHorizontalAlign	m_HorizontalAlign;
	EVerticalAlign		m_VerticalAlign;
	int32_t				m_x, m_y;
	int32_t				m_lWidth, m_lHeight;

	D3DXVECTOR2			m_v2Scale = { 1.0f, 1.0f };
	D3DXMATRIX			m_matScaling;
	bool				m_bSingleAlpha;
	float				m_fSingleAlpha;
	float				m_fWindowAlpha;
	bool				m_bAllAlpha;
	float				m_fAllAlpha;
	RECT				m_rect;
	RECT				m_clipRect = { 0,0,0,0 };
	bool				m_enableClipping = false;

	bool				m_bShow;
	uint32_t			m_dwFlag;
	pybind11::handle m_poHandler;
	CWindow* m_parent;
	WindowList			m_children;
	WindowList			m_childrenCopy;
	uint8_t				m_windowType;

	std::optional<tweeny::tween<float>> m_sizeAnimation;
};

// Box
class CBox : public CWindow
{
public:
	CBox();
	virtual ~CBox();
	void SetColor(uint32_t dwColor);

protected:
	void OnRender();

protected:
	uint32_t m_dwColor;
};

// Bar
class CBar : public CWindow
{
public:
	CBar();
	virtual ~CBar();
	void SetColor(uint32_t dwColor);

protected:
	void OnRender();

protected:
	uint32_t m_dwColor;
};

// Line
class CLine : public CWindow
{
public:
	CLine();
	virtual ~CLine();
	void SetColor(uint32_t dwColor);

protected:
	void OnRender();

protected:
	uint32_t m_dwColor;
};

// Bar3D
class CBar3D : public CWindow
{
public:
	static uint32_t Type();

public:
	CBar3D();
	virtual ~CBar3D();
	void SetColor(uint32_t dwLeft, uint32_t dwRight, uint32_t dwCenter);

protected:
	void OnRender() override;

protected:
	uint32_t m_dwLeftColor;
	uint32_t m_dwRightColor;
	uint32_t m_dwCenterColor;
};

// Text
class CTextLine : public CWindow
{
public:
	CTextLine();
	virtual ~CTextLine();

	uint32_t Type();
	bool OnIsType(uint32_t dwType);

	void SetSecret(bool bFlag);
	void SetOutline(bool bFlag);
	void SetMultiLine(bool bFlag);
	void SetFontName(const std::string& font);
	void SetFontColor(uint32_t dwColor);
	void SetFontGradient(uint32_t dwColor, uint32_t dwColor2);
	void SetOutLineColor(uint32_t dwColor);
	void SetOutLineColorGradient(uint32_t dwColor, uint32_t dwColor2);
	void SetLimitWidth(float fWidth);

	void SetText(std::string text);
	const std::string& GetText();

	std::string GetHyperlinkAtPos(int32_t x, int32_t y);
	uint32_t GetLineCount() const;
	void GetTextSize(int32_t* pnWidth, int32_t* pnHeight);

protected:
	virtual void OnUpdate();
	virtual void OnRender();

	virtual void OnChangeText();
	void SetAlpha(float fAlpha);
	float GetAlpha() const;

	CGraphicTextInstance m_TextInstance;
	std::string m_text;
	std::string m_fontName;
	CGraphicTextInstance m_instancePlaceholder;
	uint32_t m_placeHolderColor;
};

// NumberLine
class CNumberLine : public CWindow
{
public:
	CNumberLine();
	CNumberLine(CWindow* pParent);

	CNumberLine(const CNumberLine& m) = delete;
	CNumberLine& operator= (const CNumberLine&) = delete;

	virtual ~CNumberLine();

	void SetPath(const char* c_szPath);
	void SetHorizontalAlign(int32_t iType);
	void SetNumber(const char* c_szNumber);

protected:
	void ClearNumber();
	void OnRender();
	void OnChangePosition();

protected:
	std::string m_strPath;
	std::string m_strNumber;
	std::vector<CGraphicImageInstance*> m_ImageInstanceVector;

	int32_t m_iHorizontalAlign;
	uint32_t m_dwWidthSummary;
};

// ImageBox
class CImageBox : public CWindow
{
public:
	CImageBox();
	virtual ~CImageBox();

	bool LoadImage(const char* c_szFileName);
	void SetDiffuseColor(float fr, float fg, float fb, float fa);
	void SetAlpha(float fAlpha);
	float GetAlpha() const;
	void SetScale(float sx, float sy);

	int32_t GetWidth();
	int32_t GetHeight();
	float m_coolTime = 0.0f;
	float m_startCoolTime = 0.0f;

	CGraphicImageInstance* GetImageInstance() { return m_pImageInstance.get(); }

protected:
	virtual void OnCreateInstance();
	virtual void OnDestroyInstance();

	virtual void OnUpdate();
	virtual void OnRender();
	void OnChangePosition();

protected:
	std::unique_ptr<CGraphicExpandedImageInstance> m_pImageInstance;
	D3DXCOLOR m_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
};

// MarkBox
class CMarkBox : public CWindow
{
public:
	CMarkBox();
	virtual ~CMarkBox();

	void LoadImage(const char* c_szFilename);
	void SetDiffuseColor(float fr, float fg, float fb, float fa);
	void SetIndex(uint32_t uIndex);
	void SetScale(float fScale);

protected:
	virtual void OnCreateInstance();
	virtual void OnDestroyInstance();

	virtual void OnUpdate();
	virtual void OnRender();
	void OnChangePosition();

	std::unique_ptr<CGraphicMarkInstance> m_pMarkInstance;
};

// ExpandedImageBox
class CExpandedImageBox : public CImageBox
{
public:
	static uint32_t Type();
	CExpandedImageBox();
	virtual ~CExpandedImageBox();

	void SetScale(float fx, float fy);
	void SetOrigin(float fx, float fy);
	void SetRotation(float fRotation);
	void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
	void SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom);
	void SetImageClipRect(float fLeft, float fTop, float fRight, float fBottom, bool bIsVertical = false);
	void SetRenderingMode(int32_t iMode);
	void LeftRightReverse();
	D3DXCOLOR GetPixelColor(int32_t x, int32_t y) { if (m_pImageInstance) return m_pImageInstance->GetPixelColor(x, y); else return D3DXCOLOR(0, 0, 0, 0); }

protected:
	void OnCreateInstance();
	void OnDestroyInstance();

	virtual void OnUpdate();
	virtual void OnRender();

	bool OnIsType(uint32_t dwType);
};

//AniImageBox
class CAniImageBox : public CWindow
{
public:
	static uint32_t Type();
	CAniImageBox();
	virtual ~CAniImageBox();
	CAniImageBox(const CAniImageBox& m) = delete;
	CAniImageBox& operator= (const CAniImageBox&) = delete;

	void SetDelay(int32_t iDelay);
	void AppendImage(const char* c_szFileName);

	void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
	void SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom);
	void SetRenderingMode(int32_t iMode);
	void SetDiffuseColor(float r, float g, float b, float a);
	void SetOrigin(float x, float y);
	void SetRotation(float rot);
	void SetScale(float scale);
	void SetAlpha(float fAlpha);
	float GetAlpha() const;
	void Stop();
	void Start();

	void SetAniImgScale(float x, float y);
	void ResetFrame();

protected:
	void OnUpdate();
	void OnRender();
	void OnChangePosition();
	virtual void OnEndFrame();

	bool OnIsType(uint32_t dwType);

protected:
	uint8_t m_bycurDelay;
	uint8_t m_byDelay;
	uint8_t m_bycurIndex;
	bool m_isRunning;
	std::vector<std::unique_ptr<CGraphicExpandedImageInstance>> m_ImageVector;
	D3DXCOLOR m_diffuse;
};

// Button
class CButton : public CWindow
{
public:
	CButton();
	virtual ~CButton() = default;
	void SetButtonScale(float xScale, float yScale);
	int32_t GetButtonImageWidth() const;
	int32_t GetButtonImageHeight() const;

	bool SetUpVisual(const char* c_szFileName);
	bool SetOverVisual(const char* c_szFileName);
	bool SetDownVisual(const char* c_szFileName);
	bool SetDisableVisual(const char* c_szFileName);
	void SetDiffuseColor(float fr, float fg, float fb, float fa);

	const char* GetUpVisualFileName();
	const char* GetOverVisualFileName();
	const char* GetDownVisualFileName();

	void Flash();
	void EnableFlash();
	void DisableFlash();

	void Enable();
	void Disable();

	void SetUp();
	void Up();
	void Over();
	void Down();
	void LeftRightReverse();
	bool IsDisable() const;
	bool IsPressed() const;
	bool IsEnable() const;

	void SetAlpha(float fAlpha);
	float GetAlpha() const;
	void SetAlwaysTooltip(bool val);

protected:
	void OnUpdate();
	void OnRender();
	void OnChangePosition();

	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonDoubleClick();
	bool OnMouseLeftButtonUp();
	bool OnMouseOverIn();
	bool OnMouseOverOut();

	void SetCurrentVisual(CGraphicImageInstance* pVisual);
	void SetFlashVisual(CGraphicImageInstance* visual);

protected:
	bool                   m_bEnable;
	bool                   m_isPressed;
	bool                   m_isFlash;
	CGraphicImageInstance* m_pcurVisual;
	CGraphicImageInstance  m_upVisual;
	CGraphicImageInstance  m_overVisual;
	CGraphicImageInstance  m_downVisual;
	CGraphicImageInstance  m_disableVisual;
	CGraphicImageInstance* m_pFlashVisual;
	D3DXCOLOR m_diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
public:
	bool m_bIsAlwaysShowTooltip = false;
};

// RadioButton
class CRadioButton : public CButton
{
public:
	CRadioButton();
	virtual ~CRadioButton();

protected:
	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonUp();
	bool OnMouseOverIn();
	bool OnMouseOverOut();
};

// ToggleButton
class CToggleButton : public CButton
{
public:
	CToggleButton();
	virtual ~CToggleButton();

protected:
	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonUp();
	bool OnMouseOverIn();
	bool OnMouseOverOut();
};

// DragButton
class CDragButton : public CButton
{
public:
	CDragButton();
	virtual ~CDragButton();
	static uint32_t Type();
	bool OnIsType(uint32_t dwType);
	void SetRestrictMovementArea(int32_t ix, int32_t iy, int32_t iwidth, int32_t iheight);

protected:
	void OnChangePosition();
	bool OnMouseOverIn();
	bool OnMouseOverOut();
	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonUp();

	RECT m_restrictArea;
};

// DragBar
class CDragBar : public CBar
{
public:
	CDragBar();
	virtual ~CDragBar();

	static uint32_t Type();
	bool OnIsType(uint32_t dwType);

	void SetRestrictMovementArea(int32_t ix, int32_t iy, int32_t iwidth, int32_t iheight);
	bool IsPressed() const { return m_isPressed; }

protected:
	void OnChangePosition();
	bool OnMouseOverIn();
	bool OnMouseOverOut();
	bool OnMouseLeftButtonDown();
	bool OnMouseLeftButtonDoubleClick();
	bool OnMouseLeftButtonUp();

	RECT m_restrictArea;
	bool m_isPressed = false;
};

// RenderTarget
//class PyGrannyIllustrator : public CWindow
//{
//public:
//	static uint32_t Type();
//	bool OnIsType(uint32_t dwType)
//	{
//		return dwType == Type();
//	}
//
//	PyGrannyIllustrator();
//	virtual ~PyGrannyIllustrator();
//
//	void SetCamera(uint32_t index, const char* background_image);
//	void OnUpdate();
//
//	void Show();
//	void Hide();
//
//	void IllustrateActor(uint32_t dwRaceVnum);
//	void IllustrateWeapon(uint32_t dwWeapon);
//	void IllustrateEffect(bool bEffect);
//
//	void ChangeArmor(uint32_t dwVnum);
//	void ChangeHair(uint32_t dwHairIndex);
//	void ChangeWeapon(uint32_t dwVnum);
//	void ChangeMotion(uint32_t dwMotionIndex);
//	void ChangeRotation(float fRotation);
//	void ForceRotation(bool bRotation);
//
//protected:
//	void OnRender() override;
//
//private:
//	CGraphicRenderTargetTexture* m_renderTarget;
//	int32_t m_iRenderTargetIndex;
//};
