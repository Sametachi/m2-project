#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonWindowManager.h"
#include "PythonScissor.h"
#include <boost/range/adaptor/reversed.hpp>
#include <list>
#include <algorithm>
#include <EterLib/Engine.h>
#include <EterLib/TextTag.h>
#include <EterLib/GrpSubImage.h>
//#include <EterBase/StepTimer.h>
//#include <EterLib/RenderTargetManager.h>
//#include <UserInterface/CGrannyIllustrator.h>

namespace Rect
{
	struct Rectangle
	{
		int32_t X;
		int32_t Y;
		int32_t Width;
		int32_t Height;
	};

	Rectangle clamp(Rectangle smaller, Rectangle larger)
	{
		Rectangle ret{};
		ret.X = std::max(smaller.X, larger.X);
		ret.Y = std::max(smaller.Y, larger.Y);
		ret.Width = std::min(smaller.X + smaller.Width, larger.X + larger.Width) - ret.X;
		ret.Height = std::min(smaller.Y + smaller.Height, std::min(smaller.Y + smaller.Height, larger.Y + larger.Height) - ret.Y);
		return ret;
	}
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

CWindow::CWindow()
	: m_HorizontalAlign(HORIZONTAL_ALIGN_LEFT), m_VerticalAlign(VERTICAL_ALIGN_TOP), m_x(0), m_y(0), m_lWidth(0), m_lHeight(0),
	m_bShow(false), m_parent(nullptr),
	m_bSingleAlpha(false), m_fSingleAlpha(1.0f), m_fWindowAlpha(1.0f), m_bAllAlpha(false), m_fAllAlpha(1.0f),
	m_clipRect({ 0, 0, 0, 0 }), m_dwFlag(0)
{
	m_rect.bottom = m_rect.left = m_rect.right = m_rect.top = 0;
	m_windowType = WINDOW_TYPE_WINDOW; // setting all window type to window first
	D3DXMatrixIdentity(&m_matScaling);
}

CWindow::~CWindow() {}

uint32_t CWindow::Type() { return kWindowNone; }
bool CWindow::IsType(uint32_t dwType) { return OnIsType(dwType); }
bool CWindow::OnIsType(uint32_t dwType) { if (Type() == dwType) return true; return false; }
struct FClear { void operator () (CWindow* pWin) const { pWin->Clear(); } };

void CWindow::Clear()
{
	std::for_each(m_children.begin(), m_children.end(), FClear());
	m_children.clear();
	m_parent = nullptr;

	DestroyHandle();
	Hide();
}

void CWindow::DestroyHandle()
{
	m_poHandler = pybind11::object();
}

void CWindow::Show()
{
	if (IsFlag(FLAG_ANIMATED_BOARD))
	{
		if (!m_bShow)
		{
			m_bShow = true;

			m_sizeAnimation = tweeny::from(0.0f).to(1.0f).during(110).via(tweeny::easing::elasticInOut).onStep
			(
				[this](tweeny::tween<float>& t, float scale)
				{
					if (t.progress() == 0.0f)
					{
						SetScale(0.0f, 0.0f);
						SetAllAlpha(0.0f);
					}

					SetScale(scale, scale);
					SetAllAlpha(scale);

					if (t.progress() == 1.0f)
					{
						SetScale(1.0f, 1.0f);
						SetAllAlpha(1.0f);
						return true;
					}
					else
					{
						return false;
					}
				}
			);
			m_sizeAnimation.value().step(0.0f);
		}
		else
		{
			m_bShow = true;
			SetScale(1.0f, 1.0f);
			SetAllAlpha(1.0f);
		}
	}
	else
	{
		m_bShow = true;
	}
}

void CWindow::Hide()
{
	if (IsFlag(FLAG_ANIMATED_BOARD))
	{
		if (m_bShow)
		{
			m_sizeAnimation = tweeny::from(1.0f).to(0.0f).during(110).via(tweeny::easing::cubicOut).onStep([this](tweeny::tween<float>& t, float scale)
				{
					if (t.progress() == 0.0f)
					{
						SetScale(1.0f, 1.0f);
						SetAllAlpha(1.0f);
					}
					SetScale(scale, scale);
					SetAllAlpha(scale);

					if (t.progress() == 1.0f)
					{
						SetScale(0.0f, 0.0f);
						SetAllAlpha(0.0f);
						m_bShow = false;
						return true;
					}
					else
					{
						return false;
					}

				});
			m_sizeAnimation.value().step(0.0f);
		}
		else
			m_bShow = false;
	}
	else
	{
		m_bShow = false;
	}
}

bool CWindow::IsShow() const
{
	if (m_parent && !m_parent->IsFlag(FLAG_COMPONENT) && !m_enableClipping)
	{
		const auto* parentClipper = Rect::FindClippingWindowUpwards(m_parent);
		if (parentClipper)
		{
			if (Rect::LiesEntirelyOutsideRect(parentClipper->m_rect, m_rect))
				return false;
		}
	}

	return m_bShow;
}

bool CWindow::IsRendering()
{
	if (!IsShow())
		return false;

	if (!m_parent)
		return true;

	return m_parent->IsRendering();
}

void CWindow::Update()
{
	if (!IsShow())
		return;

	OnUpdate();


	if (m_sizeAnimation)
		m_sizeAnimation.value().step(static_cast<uint32_t>(CTimer::GetInstance()->GetElapsedMilliecond()));

	m_childrenCopy = m_children;
	for (CWindow* child : m_childrenCopy)
		child->Update();
}

void CWindow::SetSingleAlpha(float fAlpha)
{
	m_bSingleAlpha = true;
	m_fSingleAlpha = fAlpha;

	if (m_bAllAlpha)
		SetAlpha(fAlpha * m_fAllAlpha);
	else
		SetAlpha(fAlpha);
}

void CWindow::SetAllAlpha(float fAlpha)
{
	if (m_bSingleAlpha)
		return;

	if (m_bAllAlpha && m_fAllAlpha == fAlpha)
		return;

	m_bAllAlpha = true;
	m_fAllAlpha = fAlpha;

	if (m_bSingleAlpha)
		SetAlpha(fAlpha * m_fSingleAlpha);
	else
		SetAlpha(fAlpha);

	for (CWindow* child : m_children)
	{
		child->SetAllAlpha(fAlpha);
	}
}

void CWindow::Render()
{
	if (!IsShow())
		return;

	D3DXMATRIX currentProj;
	STATEMANAGER->GetTransform(D3DTS_WORLD, &currentProj);

	D3DXMATRIX newProj;
	newProj = currentProj * m_matScaling;

	STATEMANAGER->SetTransform(D3DTS_WORLD, &newProj);

	if (m_enableClipping)
	{
		Rect::Rectangle rc{};
		rc.X = std::max<int32_t>(0, m_rect.left);
		rc.Y = std::max<int32_t>(0, m_rect.top);
		rc.Width = m_lWidth * m_v2Scale.x;
		rc.Height = m_lHeight * m_v2Scale.y;

		const CWindow* parentClipper = Rect::FindClippingWindowUpwards(GetParent());

		if (parentClipper)
		{
			Rect::Rectangle rcParent{};
			rcParent.X = parentClipper->m_rect.left;
			rcParent.Y = parentClipper->m_rect.top;
			rcParent.Width = parentClipper->m_lWidth;
			rcParent.Height = parentClipper->m_lHeight;
			rc = clamp(rc, rcParent);
		}

		if (rc.Height < 0)
			rc.Height = 0;

		if (rc.Width < 0)
			rc.Width = 0;

		ScissorsSetter setter(rc.X, rc.Y, rc.Width, rc.Height);

		OnRender();

#ifdef GAME_UI_DEBUG
		if (CWindowManager::Instance().GetPointWindow() == this)
		{
			CPythonGraphic::Instance().SetDiffuseColor(0.0f, 1.0f, 0.0f);
			CPythonGraphic::Instance().RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
		}
#endif

		m_childrenCopy = m_children;
		for (CWindow* child : m_childrenCopy)
		{
			child->Render();
		}
	}
	else
	{
		OnRender();

#ifdef GAME_UI_DEBUG
		if (CWindowManager::Instance().GetPointWindow() == this)
		{
			CPythonGraphic::Instance().SetDiffuseColor(0.0f, 1.0f, 0.0f);
			CPythonGraphic::Instance().RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
		}
#endif

		m_childrenCopy = m_children;
		for (CWindow* child : m_childrenCopy)
			child->Render();
	}
	STATEMANAGER->SetTransform(D3DTS_WORLD, &currentProj);
}

void CWindow::OnUpdate()
{
	RunCallback("OnUpdate");
}

void CWindow::OnRender()
{
	RunCallback("OnRender");
}

void CWindow::SetSize(int32_t width, int32_t height)
{
	m_lWidth = width;
	m_lHeight = height;
	UpdateRect();
}

void CWindow::SetScale(float fx, float fy)
{
	m_v2Scale.x = fx;
	m_v2Scale.y = fy;
	UpdateRect();

	D3DXVECTOR2 vCenter(m_x + (m_lWidth / 2), m_y + (m_lHeight / 2));
	D3DXMatrixTransformation2D(&m_matScaling, &vCenter, NULL, &m_v2Scale, &vCenter, NULL, NULL);
}

void CWindow::SetHorizontalAlign(uint32_t dwAlign)
{
	m_HorizontalAlign = static_cast<EHorizontalAlign>(dwAlign);
	UpdateRect();
}

void CWindow::SetVerticalAlign(uint32_t dwAlign)
{
	m_VerticalAlign = static_cast<EVerticalAlign>(dwAlign);
	UpdateRect();
}

void CWindow::SetPosition(int32_t x, int32_t y)
{
	m_x = x;
	m_y = y;

	UpdateRect();
}

void CWindow::UpdateRect()
{
	if (m_parent)
	{
		if (m_parent->IsFlag(FLAG_RTL))
		{
			switch (m_HorizontalAlign)
			{
			case HORIZONTAL_ALIGN_LEFT:
				m_rect.left = m_parent->m_lWidth - m_lWidth - m_x;
				break;

			case HORIZONTAL_ALIGN_RIGHT:
				m_rect.left = m_x;
				break;

			case HORIZONTAL_ALIGN_CENTER:
				m_rect.left = (m_parent->m_lWidth - m_x - m_lWidth) / 2 + m_x;
				break;
			}
		}
		else
		{
			switch (m_HorizontalAlign)
			{
			case HORIZONTAL_ALIGN_LEFT:
				m_rect.left = m_x;
				break;

			case HORIZONTAL_ALIGN_RIGHT:
				m_rect.left = m_parent->m_lWidth - m_lWidth - m_x;
				break;

			case HORIZONTAL_ALIGN_CENTER:
				m_rect.left = (m_parent->m_lWidth - m_x - m_lWidth) / 2 + m_x;
				break;
			}
		}

		switch (m_VerticalAlign)
		{
		case VERTICAL_ALIGN_TOP:
			m_rect.top = m_y;
			break;

		case VERTICAL_ALIGN_BOTTOM:
			m_rect.top = m_parent->m_lHeight - m_lHeight - m_y;
			break;

		case VERTICAL_ALIGN_CENTER:
			m_rect.top = (m_parent->m_lHeight - m_y - m_lHeight) / 2 + m_y;
			break;
		}

		m_rect.left += m_parent->m_rect.left;
		m_rect.top += m_parent->m_rect.top;
	}
	else
	{
		m_rect.left = m_x;
		m_rect.top = m_y;
	}

	m_rect.bottom = m_rect.top + m_lHeight;
	m_rect.right = m_rect.left + m_lWidth;

	for (CWindow* child : m_children)
		child->UpdateRect();

	OnChangePosition();
}

void CWindow::MakeLocalPosition(int32_t& x, int32_t& y)
{
	x = x - m_rect.left;
	y = y - m_rect.top;
}

void CWindow::AddChild(CWindow* pWin)
{
	assert(!IsChild(pWin) && "Duplicate");
	assert(pWin && "Nullptr in AddChild");

	pWin->m_parent = this;
	m_children.emplace_back(pWin);
}

CWindow* CWindow::GetRoot()
{
	if (m_parent && m_parent->IsWindow())
		return m_parent->GetRoot();

	return this;
}

CWindow* CWindow::GetParent()
{
	return m_parent;
}

bool CWindow::IsChild(CWindow* pWin)
{
	return m_children.end() != std::find(m_children.begin(), m_children.end(), pWin);
}

void CWindow::DeleteChild(CWindow* win, bool clearParent)
{
	if (clearParent)
		win->m_parent = nullptr;

	const auto it = std::find(m_children.begin(), m_children.end(), win);
	if (it != m_children.end())
	{
		m_children.erase(it);
	}
	else
	{
		SysLog("Failed to find child window '{0}' in '{1}'", win->m_strName.c_str(), m_strName.c_str());
	}
}

void CWindow::SetTop(CWindow* pWin)
{
	const auto it = std::find(m_children.begin(), m_children.end(), pWin);
	if (m_children.end() != it)
	{
		m_children.erase(it);
		m_children.push_back(pWin);
		pWin->OnTop();
	}
	else
	{
		SysLog("Child window search failed on: '{0}'' in '{1}'", pWin->GetName(), m_strName);
	}
}

/*
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Send API Call
*/
void CWindow::OnMouseDrag(int32_t lx, int32_t ly) { RunCallback("OnMouseDrag", lx, ly); }
void CWindow::OnMoveWindow(int32_t lx, int32_t ly) { RunCallback("OnMoveWindow", lx, ly); }
void CWindow::OnSetFocus() { RunCallback("OnSetFocus"); }
void CWindow::OnKillFocus() { RunCallback("OnKillFocus"); }
bool CWindow::OnMouseOverIn() { return  RunCallback("OnMouseOverIn"); }
bool CWindow::OnMouseOverOut() { return RunCallback("OnMouseOverOut"); }
void CWindow::OnMouseOver() { /*RunCallback("OnMouseOver");*/ }
void CWindow::OnDrop() { RunCallback("OnDrop"); }
void CWindow::OnTop() { RunCallback("OnTop"); }
bool CWindow::OnMouseLeftButtonDown() { return RunCallback("OnMouseLeftButtonDown"); }
bool CWindow::OnMouseLeftButtonUp() { return RunCallback("OnMouseLeftButtonUp"); }
bool CWindow::OnMouseLeftButtonDoubleClick() { return RunCallback("OnMouseLeftButtonDoubleClick"); }
bool CWindow::OnMouseRightButtonDown() { return RunCallback("OnMouseRightButtonDown"); }
bool CWindow::OnMouseRightButtonUp() { return RunCallback("OnMouseRightButtonUp"); }
bool CWindow::OnMouseRightButtonDoubleClick() { return RunCallback("OnMouseRightButtonDoubleClick"); }
bool CWindow::OnMouseMiddleButtonDown() { return RunCallback("OnMouseMiddleButtonDown"); }
bool CWindow::OnMouseMiddleButtonUp() { return RunCallback("OnMouseMiddleButtonUp"); }
bool CWindow::OnChar(uint32_t ch) { return RunCallback("OnChar", ch); }
bool CWindow::OnKeyDown(KeyCode code) { return RunCallback("OnKeyDown", code); };
bool CWindow::OnKeyUp(KeyCode code) { return RunCallback("OnKeyUp", code); };
bool CWindow::OnTab() { return RunCallback("OnTab"); }
bool CWindow::OnPressEscapeKey() { return RunCallback("OnPressEscapeKey"); }
bool CWindow::OnPressExitKey() { return RunCallback("OnPressExitKey"); }
void CWindow::OnDestroy() { RunCallback("OnDestroy"); }
bool CWindow::OnMouseWheelEvent(int32_t nLen)
{
	return RunCallback("OnRunMouseWheel", nLen);
}

bool CWindow::IsIn(int32_t x, int32_t y)
{
	return x >= m_rect.left && x <= m_rect.right && y >= m_rect.top && y <= m_rect.bottom;
}

bool CWindow::IsIn()
{
	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	return IsIn(lx, ly);
}

CWindow* CWindow::PickWindow(int32_t x, int32_t y)
{
	for (auto& pWin : boost::adaptors::reverse(m_children))
	{
		if (pWin->IsShow())
		{
			if (!pWin->IsFlag(FLAG_IGNORE_SIZE))
			{
				if (!pWin->IsIn(x, y))
				{
					if (0L <= pWin->GetWidth())
						continue;
				}
			}

			CWindow* pResult = pWin->PickWindow(x, y);
			if (pResult)
				return pResult;
		}
	}

	if (IsFlag(FLAG_NOT_PICK))
		return nullptr;

	if (IsFlag(FLAG_ALPHA_SENSITIVE)) // check flag
	{
		bool isFullTransparent = true;
		IsTransparentOnPixel(&x, &y, &isFullTransparent); // check transparency of the clicking position
		if (isFullTransparent)
			return nullptr; // if transparent then return nothing, else give current window
	}

	return (this);
}

CWindow* CWindow::PickTopWindow(int32_t x, int32_t y)
{
	for (auto& pWin : boost::adaptors::reverse(m_children))
	{
		if (pWin->IsShow())
		{
			if (pWin->IsIn(x, y))
			{
				if (!pWin->IsFlag(FLAG_NOT_PICK))
				{
					if (pWin->IsFlag(FLAG_ALPHA_SENSITIVE)) // if the window is alpha sensitive check the alpha
					{
						bool isFullTransparent = true;
						pWin->IsTransparentOnPixel(&x, &y, &isFullTransparent);
						if (isFullTransparent) // if the window is transparent at the coordinates then its not the top window
							continue;
					}
					return pWin;
				}
			}
		}

	}

	return nullptr;
}

void CWindow::IsTransparentOnPixel(int32_t* x, int32_t* y, bool* ret)
{
	if (IsShow() && IsIn(*x, *y)) // check if the window is active and the cursor is in the window
	{
		if (m_windowType == WINDOW_TYPE_EX_IMAGE) // check if its an expanded_image
		{
			D3DXCOLOR pixel = ((CExpandedImageBox*)this)->GetPixelColor(*x - m_rect.left, *y - m_rect.top); // get current pixel color

			if ((uint8_t)pixel.a != 0) // if the pixel is not trasparent then the whole window is not trasparent
			{
				*ret = false;
				return;
			}
		}
		else if (m_children.empty()) // if its not ex_image and has no child then its NOT transparent [default for other components]
		{
			*ret = false;
			return;
		}
	}
	if (!m_children.empty()) // check if all the childs are trasparent on the current position
	{
		auto ritor = m_children.rbegin();
		for (; ritor != m_children.rend(); ritor++)
		{
			(*ritor)->IsTransparentOnPixel(x, y, ret);
			if (!*ret)
				return;
		}
	}
}

/*
	CBox
*/
CBox::CBox() : CWindow(), m_dwColor(0xff000000) {}
CBox::~CBox() {}
void CBox::SetColor(uint32_t dwColor) { m_dwColor = dwColor; }
void CBox::OnRender()
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwColor);
	CPythonGraphic::GetInstance()->RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
}

/*
	CBar
*/
CBar::CBar() : CWindow(), m_dwColor(0xff000000) {}
CBar::~CBar() {}
void CBar::SetColor(uint32_t dwColor) { m_dwColor = dwColor; }
void CBar::OnRender()
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwColor);
	CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
}

/*
	CLine
*/
CLine::CLine() : CWindow(), m_dwColor(0xff000000) {}
CLine::~CLine() {}
void CLine::SetColor(uint32_t dwColor) { m_dwColor = dwColor; }
void CLine::OnRender()
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwColor);
	CPythonGraphic::GetInstance()->RenderLine2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
}

/*
	CBar3D
*/
uint32_t CBar3D::Type() { return kWindowBar3D; }

CBar3D::CBar3D() : CWindow()
{
	m_dwLeftColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
	m_dwRightColor = D3DXCOLOR(0.4f, 0.4f, 0.4f, 1.0f);
	m_dwCenterColor = D3DXCOLOR(0.04f, 0.04f, 0.04f, 1.0f);
}
CBar3D::~CBar3D() {}

void CBar3D::SetColor(uint32_t dwLeft, uint32_t dwRight, uint32_t dwCenter)
{
	m_dwLeftColor = dwLeft;
	m_dwRightColor = dwRight;
	m_dwCenterColor = dwCenter;
}

void CBar3D::OnRender()
{
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwCenterColor);
	CPythonGraphic::GetInstance()->RenderBar2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwLeftColor);
	CPythonGraphic::GetInstance()->RenderLine2d(m_rect.left, m_rect.top, m_rect.right, m_rect.top);
	CPythonGraphic::GetInstance()->RenderLine2d(m_rect.left, m_rect.top, m_rect.left, m_rect.bottom);
	CPythonGraphic::GetInstance()->SetDiffuseColor(m_dwRightColor);
	CPythonGraphic::GetInstance()->RenderLine2d(m_rect.left, m_rect.bottom, m_rect.right, m_rect.bottom);
	CPythonGraphic::GetInstance()->RenderLine2d(m_rect.right, m_rect.top, m_rect.right, m_rect.bottom);
}

/*
	CTextLine
*/
CTextLine::CTextLine() : CWindow(), m_placeHolderColor{ 0 } { m_TextInstance.SetColor(0.78f, 0.78f, 0.78f); }

CTextLine::~CTextLine()
{
}

uint32_t CTextLine::Type() { return kWindowTextLine; }

bool CTextLine::OnIsType(uint32_t dwType)
{
	if (CTextLine::Type() == dwType)
	{
		return true;
	}

	return CWindow::OnIsType(dwType);
}

void CTextLine::SetSecret(bool bFlag)
{
	m_TextInstance.SetSecret(bFlag);
	OnChangeText();
}

void CTextLine::SetOutline(bool bFlag)
{
	m_TextInstance.SetOutline(bFlag);
}

void CTextLine::SetMultiLine(bool bFlag)
{
	m_TextInstance.SetMultiLine(bFlag);
	OnChangeText();
}

void CTextLine::SetFontName(const std::string& font)
{
	m_fontName = font;
	m_TextInstance.SetTextPointer(Engine::GetFontManager().LoadFont(font));
	OnChangeText();
}

void CTextLine::SetFontColor(uint32_t dwColor)
{
	m_TextInstance.SetColor(dwColor);
}

void CTextLine::SetFontGradient(uint32_t dwColor, uint32_t dwColor2)
{
	m_TextInstance.SetColorGradient(dwColor, dwColor2);
}

void CTextLine::SetOutLineColor(uint32_t dwColor)
{
	m_TextInstance.SetOutLineColor(dwColor);
}

void CTextLine::SetOutLineColorGradient(uint32_t dwColor, uint32_t dwColor2)
{
	m_TextInstance.SetOutLineColorGradient(dwColor, dwColor2);
}

void CTextLine::SetLimitWidth(float fWidth)
{
	m_TextInstance.SetLimitWidth(fWidth);
	OnChangeText();
}

void CTextLine::SetText(std::string text)
{
	m_text = std::move(text);
	m_TextInstance.SetValue(m_text);
	OnChangeText();
}

const std::string& CTextLine::GetText()
{
	return m_text;
}

void CTextLine::GetTextSize(int32_t* pnWidth, int32_t* pnHeight)
{
	m_TextInstance.GetTextSize(pnWidth, pnHeight);
}

std::string CTextLine::GetHyperlinkAtPos(int32_t x, int32_t y)
{
	uint32_t glyphIndex;
	if (!m_TextInstance.GetGlyphIndex(glyphIndex, x, y))
		return std::string();

	const int pos = m_TextInstance.GetStringIndexFromGlyphIndex(glyphIndex);

	TextTag tag;
	bool inHyperlink = false;
	std::string hyperlinkText;

	for (auto first = m_text.begin(), last = m_text.end(); first != last; )
	{
		const auto sourcePos = first - m_text.begin();

		if (GetTextTag(std::string(m_text).substr(sourcePos), tag))
		{
			if (tag.type == TEXT_TAG_HYPERLINK_START)
			{
				inHyperlink = true;
				hyperlinkText = tag.content;
			}
			else if (tag.type == TEXT_TAG_HYPERLINK_END)
			{
				inHyperlink = false;
			}

			first += tag.length;
			continue;
		}

		if (inHyperlink && pos <= sourcePos)
			return hyperlinkText;

		++first;
	}

	return std::string();
}

uint32_t CTextLine::GetLineCount() const
{
	return m_TextInstance.GetLineCount();
}

void CTextLine::OnUpdate()
{
	m_TextInstance.Update();
}

void CTextLine::OnRender()
{
	const RECT* clipRect = nullptr;
	if (m_clipRect.left != m_clipRect.right || m_clipRect.top != m_clipRect.bottom)
		clipRect = &m_clipRect;

	m_TextInstance.Render(m_rect.left, m_rect.top, 0, clipRect);
}

void CTextLine::OnChangeText()
{
	m_TextInstance.Update();
	SetSize(m_TextInstance.GetWidth(), m_TextInstance.GetHeight());
	UpdateRect();
}

void CTextLine::SetAlpha(float fAlpha)
{
	auto gradient = m_TextInstance.GetColorGradient();

	D3DXCOLOR first = gradient.first;
	first.a = fAlpha;

	D3DXCOLOR second = gradient.second;
	second.a = fAlpha;

	m_TextInstance.SetColorGradient(first, second);
}

float CTextLine::GetAlpha() const
{
	D3DXCOLOR kColor = m_TextInstance.GetColorGradient().first;
	return kColor.a;
}

/*
	CNumberLine
*/
CNumberLine::CNumberLine() : CWindow()
{
	m_strPath = "d:/ymir work/ui/game/taskbar/";
	m_iHorizontalAlign = HORIZONTAL_ALIGN_LEFT;
	m_dwWidthSummary = 0;
}

CNumberLine::CNumberLine(CWindow* pParent) : CWindow()
{
	m_strPath = "d:/ymir work/ui/game/taskbar/";
	m_iHorizontalAlign = HORIZONTAL_ALIGN_LEFT;
	m_dwWidthSummary = 0;

	m_parent = pParent;
}

CNumberLine::~CNumberLine()
{
	ClearNumber();
}

void CNumberLine::SetPath(const char* c_szPath)
{
	m_strPath = c_szPath;
}
void CNumberLine::SetHorizontalAlign(int32_t iType)
{
	m_iHorizontalAlign = iType;
}
void CNumberLine::SetNumber(const char* c_szNumber)
{
	if (0 == m_strNumber.compare(c_szNumber))
		return;

	ClearNumber();

	m_strNumber = c_szNumber;

	for (auto& cChar : m_strNumber)
	{
		std::string strImageFileName;

		if (':' == cChar)
			strImageFileName = m_strPath + "colon.sub";
		else if ('?' == cChar)
			strImageFileName = m_strPath + "questionmark.sub";
		else if ('/' == cChar)
			strImageFileName = m_strPath + "slash.sub";
		else if ('%' == cChar)
			strImageFileName = m_strPath + "percent.sub";
		else if ('+' == cChar)
			strImageFileName = m_strPath + "plus.sub";
		else if ('m' == cChar)
			strImageFileName = m_strPath + "m.sub";
		else if ('g' == cChar)
			strImageFileName = m_strPath + "g.sub";
		else if ('p' == cChar)
			strImageFileName = m_strPath + "p.sub";
		else if (cChar >= '0' && cChar <= '9')
		{
			strImageFileName = m_strPath;
			strImageFileName += cChar;
			strImageFileName += ".sub";
		}
		else
			continue;

		if (!CResourceManager::GetInstance()->IsFileExist(strImageFileName.c_str()))
			continue;

		CGraphicImage* pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strImageFileName.c_str());

		CGraphicImageInstance* pInstance = CGraphicImageInstance::New();
		pInstance->SetImagePointer(pImage);
		m_ImageInstanceVector.emplace_back(pInstance);

		m_dwWidthSummary += pInstance->GetWidth();
	}
}

void CNumberLine::ClearNumber()
{
	m_ImageInstanceVector.clear();
	m_dwWidthSummary = 0;
	m_strNumber.clear();
}

void CNumberLine::OnRender()
{
	for (auto& i : m_ImageInstanceVector)
		i->Render();
}

void CNumberLine::OnChangePosition()
{
	int32_t ix = m_x;
	int32_t iy = m_y;

	if (m_parent)
	{
		ix = m_rect.left;
		iy = m_rect.top;
	}

	switch (m_iHorizontalAlign)
	{
	case HORIZONTAL_ALIGN_LEFT:
		break;
	case HORIZONTAL_ALIGN_CENTER:
		ix -= int32_t(m_dwWidthSummary) / 2;
		break;
	case HORIZONTAL_ALIGN_RIGHT:
		ix -= int32_t(m_dwWidthSummary);
		break;
	}

	for (auto& i : m_ImageInstanceVector)
	{
		i->SetPosition(ix, iy);
		ix += i->GetWidth();
	}
}

/*
	CImageBox
*/
CImageBox::CImageBox() : CWindow()
{
	m_pImageInstance = nullptr;
}

CImageBox::~CImageBox()
{
	OnDestroyInstance();
}

void CImageBox::OnCreateInstance()
{
	OnDestroyInstance();

	m_pImageInstance = std::make_unique<CGraphicExpandedImageInstance>();
}

void CImageBox::OnDestroyInstance()
{
	m_pImageInstance.reset();
}

bool CImageBox::LoadImage(const char* c_szFileName)
{
	if (c_szFileName[0] == '\0')
		return false;

	OnCreateInstance();

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(c_szFileName);
	if (!pResource)
	{
		SysLog("Resource: {0} not found!", c_szFileName);
		return false;
	}

	if (!pResource->IsType(CGraphicImage::Type()))
	{
		SysLog("Resource type not valid");
		return false;
	}

	m_pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));
	if (m_pImageInstance->IsEmpty())
	{
		SysLog("Image instance can NOT created for: {0}", c_szFileName);
		return false;
	}

	SetSize(m_pImageInstance->GetWidth(), m_pImageInstance->GetHeight());
	UpdateRect();

	return true;
}

void CImageBox::SetScale(float sx, float sy)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetScale(sx, sy);
	CWindow::SetSize(int32_t(float(GetWidth()) * sx), int32_t(float(GetHeight()) * sy));
}

void CImageBox::SetDiffuseColor(float fr, float fg, float fb, float fa)
{
	if (!m_pImageInstance)
		return;

	m_diffuse = { fr, fg, fb, fa };
	m_pImageInstance->SetDiffuseColor(fr, fg, fb, fa);
}

void CImageBox::SetAlpha(float fAlpha) { SetDiffuseColor(m_diffuse.r, m_diffuse.g, m_diffuse.b, fAlpha); }
float CImageBox::GetAlpha() const { return m_diffuse.a; }

int32_t CImageBox::GetWidth()
{
	if (!m_pImageInstance)
		return 0;

	return m_pImageInstance->GetWidth();
}

int32_t CImageBox::GetHeight()
{
	if (!m_pImageInstance)
		return 0;

	return m_pImageInstance->GetHeight();
}

void CImageBox::OnUpdate()
{
}
void CImageBox::OnRender()
{
	if (!m_pImageInstance)
		return;

	if (IsShow())
		m_pImageInstance->Render();
}
void CImageBox::OnChangePosition()
{
	if (!m_pImageInstance)
		return;

	m_pImageInstance->SetPosition(m_rect.left, m_rect.top);
}

/*
	CMarkBox
*/
CMarkBox::CMarkBox() : CWindow()
{
	m_pMarkInstance = nullptr;
}

CMarkBox::~CMarkBox()
{
	OnDestroyInstance();
}

void CMarkBox::OnCreateInstance()
{
	OnDestroyInstance();
	m_pMarkInstance = std::make_unique<CGraphicMarkInstance>();
}

void CMarkBox::OnDestroyInstance()
{
	m_pMarkInstance.reset();
}

void CMarkBox::LoadImage(const char* c_szFilename)
{
	OnCreateInstance();

	m_pMarkInstance->SetImageFileName(c_szFilename);
	m_pMarkInstance->Load();
	SetSize(m_pMarkInstance->GetWidth(), m_pMarkInstance->GetHeight());

	UpdateRect();
}

void CMarkBox::SetScale(float fScale)
{
	if (!m_pMarkInstance)
		return;

	m_pMarkInstance->SetScale(fScale);
}

void CMarkBox::SetIndex(uint32_t uIndex)
{
	if (!m_pMarkInstance)
		return;

	m_pMarkInstance->SetIndex(uIndex);
}

void CMarkBox::SetDiffuseColor(float fr, float fg, float fb, float fa)
{
	if (!m_pMarkInstance)
		return;

	m_pMarkInstance->SetDiffuseColor(fr, fg, fb, fa);
}

void CMarkBox::OnUpdate()
{
}

void CMarkBox::OnRender()
{
	if (!m_pMarkInstance)
		return;

	if (IsShow())
		m_pMarkInstance->Render();
}

void CMarkBox::OnChangePosition()
{
	if (!m_pMarkInstance)
		return;

	m_pMarkInstance->SetPosition(m_rect.left, m_rect.top);
}

/*
	CExpandedImageBox
*/
uint32_t CExpandedImageBox::Type()
{
	return kWindowExpandedImageBox;
}

bool CExpandedImageBox::OnIsType(uint32_t dwType)
{
	if (CExpandedImageBox::Type() == dwType)
		return true;

	return false;
}

CExpandedImageBox::CExpandedImageBox() : CImageBox()
{
	m_windowType = WINDOW_TYPE_EX_IMAGE;
}

CExpandedImageBox::~CExpandedImageBox()
{
	OnDestroyInstance();
}

void CExpandedImageBox::OnCreateInstance()
{
	OnDestroyInstance();

	m_pImageInstance = std::make_unique<CGraphicExpandedImageInstance>();
}

void CExpandedImageBox::OnDestroyInstance()
{
	m_pImageInstance.reset();
}

void CExpandedImageBox::SetScale(float fx, float fy)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetScale(fx, fy);
	CWindow::SetSize(int32_t(float(GetWidth()) * fx), int32_t(float(GetHeight()) * fy));
}

void CExpandedImageBox::SetOrigin(float fx, float fy)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetOrigin(fx, fy);
}

void CExpandedImageBox::SetRotation(float fRotation)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRotation(fRotation);
}

void CExpandedImageBox::SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRenderingRect(fLeft, fTop, fRight, fBottom);
}

void CExpandedImageBox::SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom)
{
	if (!m_pImageInstance)
		return;

	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRenderingRectWithScale(fLeft, fTop, fRight, fBottom);
}

void CExpandedImageBox::SetImageClipRect(float fLeft, float fTop, float fRight, float fBottom, bool bIsVertical)
{
	if (!m_pImageInstance)
		return;

	const RECT& c_rRect = GetRect();

	float fDifLeft = (c_rRect.left < fLeft) ? -(float(fLeft - c_rRect.left) / float(GetWidth())) : 0.0f;
	float fDifTop = (c_rRect.top < fTop) ? -(float(fTop - c_rRect.top) / float(GetHeight())) : 0.0f;
	float fDifRight = (c_rRect.right > fRight) ? -(float(c_rRect.right - fRight) / float(GetWidth())) : 0.0f;
	float fDifBottom = (c_rRect.bottom > fBottom) ? -(float(c_rRect.bottom - fBottom) / float(GetHeight())) : 0.0f;

	if (bIsVertical)
		static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRenderingRect(fLeft, fDifTop, fRight, fDifBottom);
	else
		static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRenderingRect(fDifLeft, fDifTop, fDifRight, fDifBottom);
}

void CExpandedImageBox::SetRenderingMode(int32_t iMode)
{
	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetRenderingMode(iMode);
}

void CExpandedImageBox::LeftRightReverse()
{
	static_cast<CGraphicExpandedImageInstance*>(m_pImageInstance.get())->SetInverse();
}

void CExpandedImageBox::OnUpdate()
{
}

void CExpandedImageBox::OnRender()
{
	if (!m_pImageInstance)
		return;

	if (IsShow())
		m_pImageInstance->Render();
}

/*
	CAniImageBox
*/
uint32_t CAniImageBox::Type()
{
	return kWindowAniImageBox;
}

bool CAniImageBox::OnIsType(uint32_t dwType)
{
	if (Type() == dwType)
		return true;

	return false;
}

CAniImageBox::CAniImageBox() : CWindow(), m_bycurDelay(0), m_byDelay(4), m_bycurIndex(0), m_isRunning(true), m_diffuse{ 1.0f, 1.0f, 1.0f, 1.0f }
{
	m_ImageVector.clear();
}

CAniImageBox::~CAniImageBox()
{
	m_ImageVector.clear();
}

void CAniImageBox::SetDelay(int32_t iDelay)
{
	m_byDelay = iDelay;
}

void CAniImageBox::AppendImage(const char* c_szFileName)
{
	CGraphicImage* res = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	auto pImageInstance = std::make_unique<CGraphicExpandedImageInstance>(/*c_szFileName*/);
	pImageInstance->SetImagePointer(res);

	if (pImageInstance->IsEmpty())
		return;

	m_ImageVector.emplace_back(std::move(pImageInstance));
	m_bycurIndex = static_cast<uint8_t>(rand() % m_ImageVector.size());
}

void CAniImageBox::SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
{
	std::for_each(m_ImageVector.begin(), m_ImageVector.end(), [&](std::unique_ptr<CGraphicExpandedImageInstance>& inst)
		{
			inst->SetRenderingRect(fLeft, fTop, fRight, fBottom);
		});
}

void CAniImageBox::SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom)
{
	std::for_each(m_ImageVector.begin(), m_ImageVector.end(), [&](std::unique_ptr<CGraphicExpandedImageInstance>& inst)
		{
			inst->SetRenderingRectWithScale(fLeft, fTop, fRight, fBottom);
		});
}

struct FSetRenderingMode
{
	int32_t iMode;
	void operator () (std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) const
	{
		pInstance->SetRenderingMode(iMode);
	}
};
void CAniImageBox::SetRenderingMode(int32_t iMode)
{
	FSetRenderingMode setRenderingMode{};
	setRenderingMode.iMode = iMode;
	for_each(m_ImageVector.begin(), m_ImageVector.end(), setRenderingMode);
}

struct FSetDiffuseColor
{
	float r;
	float g;
	float b;
	float a;

	void operator()(std::unique_ptr<CGraphicExpandedImageInstance>& pInstance)
	{
		pInstance->SetDiffuseColor(r, g, b, a);
	}
};

void CAniImageBox::SetDiffuseColor(float r, float g, float b, float a)
{
	m_diffuse = { r, g, b, a };

	FSetDiffuseColor setDiffuseColor{};
	setDiffuseColor.r = r;
	setDiffuseColor.g = g;
	setDiffuseColor.b = b;
	setDiffuseColor.a = a;

	for_each(m_ImageVector.begin(), m_ImageVector.end(), setDiffuseColor);
}

void CAniImageBox::SetOrigin(float x, float y)
{
	for_each(m_ImageVector.begin(), m_ImageVector.end(), [x, y](std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) { pInstance->SetOrigin(x, y); });
}

void CAniImageBox::SetRotation(float rot)
{
	for_each(m_ImageVector.begin(), m_ImageVector.end(), [rot](std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) { pInstance->SetRotation(rot); });
}

void CAniImageBox::SetScale(float scale)
{
	for_each(m_ImageVector.begin(), m_ImageVector.end(), [scale](std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) { pInstance->SetScale(scale, scale); });
}

void CAniImageBox::SetAlpha(float fAlpha)
{
	FSetDiffuseColor setDiffuseColor{};
	setDiffuseColor.r = m_diffuse.r;
	setDiffuseColor.g = m_diffuse.g;
	setDiffuseColor.b = m_diffuse.b;
	setDiffuseColor.a = fAlpha;

	for_each(m_ImageVector.begin(), m_ImageVector.end(), setDiffuseColor);
}

float CAniImageBox::GetAlpha() const { return m_diffuse.a; }
void CAniImageBox::Stop() { m_isRunning = false; }
void CAniImageBox::Start() { m_isRunning = true; }

struct FSetAniImgScale
{
	float x;
	float y;

	void operator()(std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) { pInstance->SetScale(x, y); }
};

void CAniImageBox::SetAniImgScale(float x, float y)
{
	FSetAniImgScale setAniImgScale{};
	setAniImgScale.x = x;
	setAniImgScale.y = y;

	for_each(m_ImageVector.begin(), m_ImageVector.end(), setAniImgScale);
}

void CAniImageBox::ResetFrame()
{
	m_bycurIndex = 0;
}

void CAniImageBox::OnUpdate()
{
	if (!m_isRunning)
		return;

	++m_bycurDelay;
	if (m_bycurDelay < m_byDelay)
	{
		return;
	}

	m_bycurDelay = 0;

	++m_bycurIndex;
	if (m_bycurIndex >= m_ImageVector.size())
	{
		m_bycurIndex = 0;

		OnEndFrame();
	}
}

void CAniImageBox::OnRender()
{
	if (m_bycurIndex < m_ImageVector.size())
	{
		m_ImageVector[m_bycurIndex]->Render();
	}
}

struct FChangePosition
{
	float fx, fy;

	void operator()(std::unique_ptr<CGraphicExpandedImageInstance>& pInstance) { pInstance->SetPosition(fx, fy); }
};

void CAniImageBox::OnChangePosition()
{
	FChangePosition changePosition{};
	changePosition.fx = m_rect.left;
	changePosition.fy = m_rect.top;
	for_each(m_ImageVector.begin(), m_ImageVector.end(), changePosition);
}

void CAniImageBox::OnEndFrame() { RunCallback("OnEndFrame"); }

/*
	CButton
*/
CButton::CButton() : CWindow(), m_bEnable(true), m_isPressed(false), m_isFlash(false), m_pcurVisual(nullptr), m_pFlashVisual(nullptr)
{
}

void CButton::SetButtonScale(float xScale, float yScale)
{
	if (m_pcurVisual)
		m_pcurVisual->SetScale(xScale, yScale);

	if (!m_upVisual.IsEmpty())
		m_upVisual.SetScale(xScale, yScale);

	if (!m_overVisual.IsEmpty())
		m_overVisual.SetScale(xScale, yScale);

	if (!m_downVisual.IsEmpty())
		m_downVisual.SetScale(xScale, yScale);

	if (!m_disableVisual.IsEmpty())
		m_disableVisual.SetScale(xScale, yScale);

	SetSize(m_upVisual.GetWidth() * xScale, m_upVisual.GetHeight() * yScale);
}

int32_t CButton::GetButtonImageWidth() const
{
	if (m_pcurVisual)
		return m_pcurVisual->GetWidth();

	return 0;
}

int32_t CButton::GetButtonImageHeight() const
{
	if (m_pcurVisual)
		return m_pcurVisual->GetHeight();

	return 0;
}

bool CButton::SetUpVisual(const char* c_szFileName)
{
	auto pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_upVisual.SetImagePointer(pResource);

	if (m_upVisual.IsEmpty())
		return false;

	SetSize(m_upVisual.GetWidth(), m_upVisual.GetHeight());
	SetCurrentVisual(&m_upVisual);

	return true;
}

bool CButton::SetOverVisual(const char* c_szFileName)
{
	auto pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_overVisual.SetImagePointer(pResource);

	if (m_overVisual.IsEmpty())
		return false;

	SetSize(m_overVisual.GetWidth(), m_overVisual.GetHeight());

	return true;
}

bool CButton::SetDownVisual(const char* c_szFileName)
{
	auto pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_downVisual.SetImagePointer(pResource);

	if (m_downVisual.IsEmpty())
		return false;

	SetSize(m_downVisual.GetWidth(), m_downVisual.GetHeight());

	return true;
}

bool CButton::SetDisableVisual(const char* c_szFileName)
{
	auto pResource = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	m_disableVisual.SetImagePointer(pResource);

	if (m_downVisual.IsEmpty())
		return false;

	SetSize(m_disableVisual.GetWidth(), m_disableVisual.GetHeight());

	return true;
}

void CButton::SetDiffuseColor(float fr, float fg, float fb, float fa)
{
	if (m_pcurVisual)
		m_pcurVisual->SetDiffuseColor(fr, fg, fb, fa);

	if (!m_upVisual.IsEmpty())
		m_upVisual.SetDiffuseColor(fr, fg, fb, fa);

	if (!m_overVisual.IsEmpty())
		m_overVisual.SetDiffuseColor(fr, fg, fb, fa);

	if (!m_downVisual.IsEmpty())
		m_downVisual.SetDiffuseColor(fr, fg, fb, fa);

	if (!m_disableVisual.IsEmpty())
		m_disableVisual.SetDiffuseColor(fr, fg, fb, fa);
}

const char* CButton::GetUpVisualFileName() { return m_upVisual.GetGraphicImagePointer()->GetFileName(); }

const char* CButton::GetOverVisualFileName() { return m_overVisual.GetGraphicImagePointer()->GetFileName(); }

const char* CButton::GetDownVisualFileName() { return m_downVisual.GetGraphicImagePointer()->GetFileName(); }

void CButton::SetFlashVisual(CGraphicImageInstance* visual)
{
	m_pFlashVisual = visual;

	if (visual)
	{
		visual->SetPosition(m_x, m_y);
	}
}

void CButton::Flash()
{
	m_isFlash = true;
}

void CButton::EnableFlash()
{
	m_isFlash = true;
	SetFlashVisual(m_pFlashVisual);
}

void CButton::DisableFlash()
{
	m_isFlash = false;
	SetFlashVisual(nullptr);
}

void CButton::Enable()
{
	SetUp();
	m_bEnable = true;
}

void CButton::Disable()
{
	m_bEnable = false;
	if (!m_disableVisual.IsEmpty())
		SetCurrentVisual(&m_disableVisual);
}

bool CButton::IsDisable() const { return !m_bEnable; }

void CButton::SetUp()
{
	SetCurrentVisual(&m_upVisual);
	m_isPressed = false;
}

void CButton::Up()
{
	if (IsIn())
		SetCurrentVisual(&m_overVisual);
	else
		SetCurrentVisual(&m_upVisual);

	RunCallback("CallEvent");
}

void CButton::Over()
{
	SetCurrentVisual(&m_overVisual);
}

void CButton::Down()
{
	m_isPressed = true;
	SetCurrentVisual(&m_downVisual);
	RunCallback("DownEvent");
}

void CButton::LeftRightReverse()
{
	m_upVisual.SetInverse();
	m_overVisual.SetInverse();
	m_downVisual.SetInverse();
	m_disableVisual.SetInverse();
}

void CButton::OnUpdate()
{
}

void CButton::OnRender()
{
	if (!IsShow())
		return;

	if (m_pcurVisual)
	{
		if (m_isFlash && !IsIn())
		{
			if (uint64_t(GetTickCount64() / 500) % 2)
			{
				if (m_pFlashVisual)
				{
					m_pFlashVisual->Render();
				}
				return;
			}
		}

		m_pcurVisual->Render();
	}

	RunCallback("OnRender");
}

void CButton::OnChangePosition()
{
	if (m_pcurVisual)
		m_pcurVisual->SetPosition(m_rect.left, m_rect.top);

	if (m_pFlashVisual)
		m_pFlashVisual->SetPosition(m_rect.left, m_rect.top);
}

bool CButton::OnMouseLeftButtonDown()
{
	if (!IsEnable())
		return true;

	m_isPressed = true;
	Down();

	return true;
}

bool CButton::OnMouseLeftButtonDoubleClick()
{
	if (!IsEnable())
		return true;

	OnMouseLeftButtonDown();

	return true;
}

bool CButton::OnMouseLeftButtonUp()
{
	if (!IsEnable())
		return true;
	if (!IsPressed())
		return true;

	m_isPressed = false;
	Up();

	return true;
}

bool CButton::OnMouseOverIn()
{
	bool enable = IsEnable();

	if (enable || m_bIsAlwaysShowTooltip)
		RunCallback("ShowToolTip");

	if (!enable)
		return false;

	Over();

	return RunCallback("OnMouseOverIn");
}

bool CButton::OnMouseOverOut()
{
	const bool enable = IsEnable();

	if (enable || m_bIsAlwaysShowTooltip)
		RunCallback("HideToolTip");

	if (!enable)
		return false;

	SetUp();
	return RunCallback("OnMouseOverOut");
}

void CButton::SetCurrentVisual(CGraphicImageInstance* pVisual)
{
	m_pcurVisual = pVisual;
	m_pcurVisual->SetPosition(m_rect.left, m_rect.top);
}

bool CButton::IsEnable() const { return m_bEnable; }
bool CButton::IsPressed() const { return m_isPressed; }

void CButton::SetAlpha(float fAlpha) { SetDiffuseColor(m_diffuse.r, m_diffuse.g, m_diffuse.b, fAlpha); }
float CButton::GetAlpha() const { return m_diffuse.a; }
void CButton::SetAlwaysTooltip(bool val) { m_bIsAlwaysShowTooltip = val; }

/*
	CRadioButton
*/
CRadioButton::CRadioButton() : CButton() {}
CRadioButton::~CRadioButton() {}

bool CRadioButton::OnMouseLeftButtonDown()
{
	if (!IsEnable())
		return false;

	if (!m_isPressed)
	{
		Down();
		RunCallback("CallEvent");
	}

	return false;
}

bool CRadioButton::OnMouseLeftButtonUp() { return true; }

bool CRadioButton::OnMouseOverIn()
{
	if (!IsEnable())
		return false;

	if (!m_isPressed)
		SetCurrentVisual(&m_overVisual);

	RunCallback("ShowToolTip");
	return true;
}

bool CRadioButton::OnMouseOverOut()
{
	if (!IsEnable())
		return false;

	if (!m_isPressed)
		SetCurrentVisual(&m_upVisual);

	RunCallback("HideToolTip");
	return true;
}

/*
	CToggleButton
*/

CToggleButton::CToggleButton() : CButton() {}

CToggleButton::~CToggleButton() {}

bool CToggleButton::OnMouseLeftButtonDown()
{
	if (!IsEnable())
		return false;

	if (m_isPressed)
	{
		SetUp();

		if (IsIn())
			SetCurrentVisual(&m_overVisual);
		else
			SetCurrentVisual(&m_upVisual);

		RunCallback("OnToggleUp");
	}
	else
	{
		Down();
		RunCallback("OnToggleDown");
	}

	return false;
}

bool CToggleButton::OnMouseLeftButtonUp() { return true; }

bool CToggleButton::OnMouseOverIn()
{
	if (!IsEnable())
		return false;

	if (!m_isPressed)
		SetCurrentVisual(&m_overVisual);

	RunCallback("ShowToolTip");
	return true;
}

bool CToggleButton::OnMouseOverOut()
{
	if (!IsEnable())
		return false;

	if (!m_isPressed)
		SetCurrentVisual(&m_upVisual);

	RunCallback("HideToolTip");
	return true;
}

/*
	CDragButton
*/
CDragButton::CDragButton() : CButton()
{
	m_restrictArea.left = 0;
	m_restrictArea.top = 0;
	m_restrictArea.right = CWindowManager::GetInstance()->GetScreenWidth();
	m_restrictArea.bottom = CWindowManager::GetInstance()->GetScreenHeight();
}

uint32_t CDragButton::Type() { return kWindowDragButton; }

bool CDragButton::OnIsType(uint32_t dwType)
{
	if (CDragButton::Type() == dwType)
	{
		return true;
	}

	return CWindow::OnIsType(dwType);
}

CDragButton::~CDragButton() {}

void CDragButton::SetRestrictMovementArea(int32_t ix, int32_t iy, int32_t iwidth, int32_t iheight)
{
	m_restrictArea.left = ix;
	m_restrictArea.top = iy;
	m_restrictArea.right = ix + iwidth;
	m_restrictArea.bottom = iy + iheight;
}

void CDragButton::OnChangePosition()
{
	m_x = std::max<int32_t>(m_x, m_restrictArea.left);
	m_y = std::max<int32_t>(m_y, m_restrictArea.top);
	m_x = std::min<int32_t>(m_x, std::max<int32_t>(0, m_restrictArea.right - m_lWidth));
	m_y = std::min<int32_t>(m_y, std::max<int32_t>(0, m_restrictArea.bottom - m_lHeight));

	m_rect.left = m_x;
	m_rect.top = m_y;

	if (m_parent)
	{
		const RECT& r = m_parent->GetRect();
		m_rect.left += r.left;
		m_rect.top += r.top;
	}

	m_rect.right = m_rect.left + m_lWidth;
	m_rect.bottom = m_rect.top + m_lHeight;

	for (auto& child : m_children)
		child->UpdateRect();

	if (m_pcurVisual)
		m_pcurVisual->SetPosition(m_rect.left, m_rect.top);

	if (IsPressed())
		RunCallback("OnMove");
}

bool CDragButton::OnMouseOverIn()
{
	if (!IsEnable())
		return false;

	CButton::OnMouseOverIn();
	RunCallback("OnMouseOverIn");
	return true;
}

bool CDragButton::OnMouseOverOut()
{
	if (!IsEnable())
		return false;

	CButton::OnMouseOverIn();
	RunCallback("OnMouseOverOut");
	return true;
}

bool CDragButton::OnMouseLeftButtonDown()
{
	if (!IsEnable())
		return false;

	m_isPressed = true;
	RunCallback("OnMouseLeftButtonDown");
	return RunCallback("OnMove");
}

bool CDragButton::OnMouseLeftButtonUp()
{
	if (!IsPressed())
		return true;

	m_isPressed = false;
	return RunCallback("OnMouseLeftButtonUp");
}

/*
	CDragBar
*/
CDragBar::CDragBar() : CBar()
{
	m_restrictArea.left = 0;
	m_restrictArea.top = 0;
	m_restrictArea.right = CWindowManager::GetInstance()->GetScreenWidth();
	m_restrictArea.bottom = CWindowManager::GetInstance()->GetScreenHeight();
}

uint32_t CDragBar::Type() { return kWindowDragBar; }

bool CDragBar::OnIsType(uint32_t dwType)
{
	if (CDragBar::Type() == dwType)
	{
		return true;
	}

	return CWindow::OnIsType(dwType);
}

CDragBar::~CDragBar() {}

void CDragBar::SetRestrictMovementArea(int32_t ix, int32_t iy, int32_t iwidth, int32_t iheight)
{
	m_restrictArea.left = ix;
	m_restrictArea.top = iy;
	m_restrictArea.right = ix + iwidth;
	m_restrictArea.bottom = iy + iheight;
}

void CDragBar::OnChangePosition()
{
	m_x = std::max<int32_t>(m_x, m_restrictArea.left);
	m_y = std::max<int32_t>(m_y, m_restrictArea.top);
	m_x = std::min<int32_t>(m_x, std::max<int32_t>(0, m_restrictArea.right - m_lWidth));
	m_y = std::min<int32_t>(m_y, std::max<int32_t>(0, m_restrictArea.bottom - m_lHeight));

	m_rect.left = m_x;
	m_rect.top = m_y;

	if (m_parent)
	{
		const RECT& r = m_parent->GetRect();
		m_rect.left += r.left;
		m_rect.top += r.top;
	}

	m_rect.right = m_rect.left + m_lWidth;
	m_rect.bottom = m_rect.top + m_lHeight;

	for (auto& child : m_children)
		child->UpdateRect();

	if (IsPressed())
		RunCallback("OnMove");
}

bool CDragBar::OnMouseOverIn()
{
	RunCallback("OnMouseOverIn");
	return true;
}

bool CDragBar::OnMouseOverOut()
{
	RunCallback("OnMouseOverOut");
	return true;
}

bool CDragBar::OnMouseLeftButtonDown()
{
	m_isPressed = true;
	return RunCallback("OnMouseLeftButtonDown");
}

bool CDragBar::OnMouseLeftButtonDoubleClick()
{
	OnMouseLeftButtonDown();
	return RunCallback("OnMouseLeftButtonDown");
}

bool CDragBar::OnMouseLeftButtonUp()
{
	if (!IsPressed())
		return true;

	m_isPressed = false;
	return RunCallback("OnMouseLeftButtonUp");
}

/*
	Render Target
*/
//uint32_t PyGrannyIllustrator::Type() { return kWindowGrannyIllustrator; }
//
//PyGrannyIllustrator::PyGrannyIllustrator() : CWindow(), m_renderTarget(nullptr), m_iRenderTargetIndex(-1)
//{
//}
//
//PyGrannyIllustrator::~PyGrannyIllustrator()
//{
//	m_renderTarget = nullptr;
//	m_iRenderTargetIndex = -1;
//}
//
//void PyGrannyIllustrator::SetCamera(uint32_t index, const char* background_image)
//{
//	m_iRenderTargetIndex = index;
//
//	const auto pRenderTarget = CRenderTargetManager::Instance().GetRenderTarget(index);
//	if (pRenderTarget)
//		m_renderTarget = pRenderTarget;
//
//	CGrannyIllustrator::Instance().InitializeRenderInstance(m_iRenderTargetIndex, background_image);
//}
//
//void PyGrannyIllustrator::Show()
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().Show(m_iRenderTargetIndex, true);
//	CWindow::Show();
//}
//
//void PyGrannyIllustrator::Hide()
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().Show(m_iRenderTargetIndex, false);
//	CWindow::Hide();
//}
//
//void PyGrannyIllustrator::IllustrateActor(uint32_t dwRaceVnum)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().IllustrateActor(m_iRenderTargetIndex, dwRaceVnum);
//}
//
//void PyGrannyIllustrator::IllustrateWeapon(uint32_t dwWeapon)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().IllustrateWeapon(m_iRenderTargetIndex, dwWeapon);
//}
//
//void PyGrannyIllustrator::IllustrateEffect(bool bEffect)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().IllustrateEffect(m_iRenderTargetIndex, bEffect);
//}
//
//void PyGrannyIllustrator::ChangeArmor(uint32_t dwVnum)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ChangeArmor(m_iRenderTargetIndex, dwVnum);
//}
//
//void PyGrannyIllustrator::ChangeHair(uint32_t dwHairIndex)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ChangeHair(m_iRenderTargetIndex, dwHairIndex);
//}
//
//void PyGrannyIllustrator::ChangeWeapon(uint32_t dwVnum)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ChangeWeapon(m_iRenderTargetIndex, dwVnum);
//}
//
//void PyGrannyIllustrator::ChangeMotion(uint32_t dwMotionIndex)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ChangeMotion(m_iRenderTargetIndex, dwMotionIndex);
//}
//
//void PyGrannyIllustrator::ChangeRotation(float fRotation)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ChangeRotation(m_iRenderTargetIndex, fRotation);
//}
//
//void PyGrannyIllustrator::ForceRotation(bool bRotation)
//{
//	if (m_iRenderTargetIndex != -1)
//		CGrannyIllustrator::Instance().ForceRotation(m_iRenderTargetIndex, bRotation);
//}
//
//void PyGrannyIllustrator::OnUpdate()
//{
//	if (m_renderTarget && m_bShow)
//		m_renderTarget->SetRenderingRect(&m_rect);
//}
//
//void PyGrannyIllustrator::OnRender()
//{
//	if (m_renderTarget && m_bShow)
//		m_renderTarget->Render();
//}
