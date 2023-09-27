#include "StdAfx.h"
#include "../EterBase/Timer.h"
#include "PythonEditLine.h"
#include "PythonWindowManager.h"
#include <boost/locale/utf.hpp>
#include <boost/algorithm/string.hpp>

namespace /* Graphical Shits */
{
	struct ApplyRenderState
	{
		ApplyRenderState()
		{
			STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	
			STATEMANAGER->SaveRenderState(D3DRS_FOGENABLE, FALSE);
			STATEMANAGER->SaveRenderState(D3DRS_LIGHTING, FALSE);
	
			STATEMANAGER->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
			STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	
			STATEMANAGER->SetTexture(0, NULL);
		}
	
		~ApplyRenderState()
		{
			STATEMANAGER->RestoreRenderState(D3DRS_SRCBLEND);
			STATEMANAGER->RestoreRenderState(D3DRS_DESTBLEND);
	
			STATEMANAGER->RestoreRenderState(D3DRS_FOGENABLE);
			STATEMANAGER->RestoreRenderState(D3DRS_LIGHTING);
		}
	};
	
	void RenderSelectionRect(float sx, float sy, float ex, float ey)
	{
		const TDiffuse diffuse = D3DXCOLOR(0.00f, 0.00f, 1.00f, 0.35f);
	
		TPDTVertex vertices[4];
		vertices[0].diffuse = diffuse;
		vertices[1].diffuse = diffuse;
		vertices[2].diffuse = diffuse;
		vertices[3].diffuse = diffuse;
		vertices[0].position = TPosition(sx, sy, 0.0f);
		vertices[1].position = TPosition(ex, sy, 0.0f);
		vertices[2].position = TPosition(sx, ey, 0.0f);
		vertices[3].position = TPosition(ex, ey, 0.0f);
	
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);
		STATEMANAGER->SetFVF(TPDTVertex::kFVF);
		if (CGraphicBase::SetPDTStream(vertices, 4))
			STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
	}
}

CPythonEditLine::CPythonEditLine(): CTextLine(), m_focused(false), m_renderCursor(false), m_cursorVisibilityTime(0), m_cursorPosition(0), m_selectionStart(0), m_selectionEnd(0)
, m_maxLength(0), m_maxVisibleLength(0)
{
	AddFlag(FLAG_DRAGABLE | FLAG_FOCUSABLE);
}

void CPythonEditLine::Enable()
{
	m_enabled = true;
}

void CPythonEditLine::Disable()
{
	m_enabled = false;
}

void CPythonEditLine::SetMax(uint32_t count)
{
	m_maxLength = count;
}

void CPythonEditLine::SetMaxVisible(uint32_t count)
{
	m_maxVisibleLength = count;
}

uint32_t CPythonEditLine::GetCursorPosition() const
{
	return m_cursorPosition;
}

void CPythonEditLine::SetCursorPosition(uint32_t position)
{
	m_cursorPosition = std::max<uint32_t>(position, m_text.length());
}

void CPythonEditLine::SetPlaceholderText(std::string text)
{
	m_placeholderText = text;
	m_placeholderInstance.SetTextPointer(Engine::GetFontManager().LoadFont(m_fontName));
	m_placeholderInstance.SetSecret(m_TextInstance.IsSecret());
	m_placeholderInstance.SetValue(text);
	m_placeholderInstance.Update();
}

void CPythonEditLine::SetPlaceholderColor(D3DXCOLOR color)
{
	m_placeHolderColor = color;
	m_placeholderInstance.SetColor(color);
	m_placeholderInstance.Update();
}

void CPythonEditLine::MoveCursor(int32_t offset, bool updateSelection)
{
	const auto glyphCount = m_TextInstance.GetGlyphCount();
	const auto oldP = m_TextInstance.GetGlyphIndexFromStringIndex(m_cursorPosition);
	const auto newP = std::clamp<int32_t>(oldP + offset, 0, glyphCount);

	m_cursorPosition = m_TextInstance.GetStringIndexFromGlyphIndex(newP);

	if (updateSelection)
		m_selectionEnd = m_cursorPosition;
	else
		m_selectionStart = m_selectionEnd = m_cursorPosition;
}

void CPythonEditLine::MoveToBeginning()
{
	m_cursorPosition = m_selectionStart = m_selectionEnd = 0;
}

void CPythonEditLine::MoveToEnd()
{
	m_cursorPosition = m_selectionStart = m_selectionEnd = m_text.length();
}

bool CPythonEditLine::Insert(uint32_t offset, const std::string& text)
{
	if (0 != m_maxLength && m_text.length() + text.length() > m_maxLength)
		return false;

	if (0 != m_maxVisibleLength) 
	{
		uint32_t count = m_TextInstance.GetGlyphCount();
		if (count >= m_maxVisibleLength)
			return false;

		TextTag tag;
		for (auto first = text.begin(), last = text.end(); first != last; ) 
		{
			if (GetTextTag(std::string(first, last), tag)) 
			{
				first += tag.length;
				continue;
			}

			boost::locale::utf::utf_traits<char>::decode(first, last);
			if (++count > m_maxVisibleLength)
				return false;
		}
	}

	// We don't want people editing hyperlinks XDD
	offset = GetTextTagBoundary(m_text, offset).first;

	m_text.insert(offset, text.data(), text.length());

	m_TextInstance.SetValue(m_text);
	m_TextInstance.Update();

	if (m_cursorPosition >= offset)
		m_cursorPosition += text.length();

	if (m_selectionEnd >= offset)
		m_selectionEnd += text.length();

	if (m_selectionStart >= offset)
		m_selectionStart += text.length();

	RunCallback("OnChange");
	return true;
}

void CPythonEditLine::Erase(uint32_t offset, uint32_t count)
{
	// We don't want people editing hyperlinks XDD
	const auto begin = GetTextTagBoundary(m_text, offset).first;
	const auto end = GetTextTagBoundary(m_text, offset + count).second;

	m_text.erase(m_text.begin() + begin, m_text.begin() + end);

	m_TextInstance.SetValue(m_text);
	m_TextInstance.Update();

	if (m_cursorPosition > begin)
		m_cursorPosition -= std::min(end - begin, m_cursorPosition - begin);

	if (m_selectionStart > begin)
		m_selectionStart -= std::min(end - begin, m_selectionStart - begin);

	if (m_selectionEnd > begin)
		m_selectionEnd -= std::min(end - begin, m_selectionEnd - begin);

	RunCallback("OnChange");
}

void CPythonEditLine::EraseGlyphs(uint32_t offset, uint32_t count)
{
	const auto glyphCount = m_TextInstance.GetGlyphCount();

	const auto glyphStart = m_TextInstance.GetGlyphIndexFromStringIndex(offset);
	const auto glyphEnd = std::min(glyphCount, glyphStart + count);

	const auto strStart = m_TextInstance.GetStringIndexFromGlyphIndex(glyphStart);
	const auto strEnd = m_TextInstance.GetStringIndexFromGlyphIndex(glyphEnd);
	Erase(strStart, strEnd - strStart);
}

void CPythonEditLine::OnUpdate()
{
	if(!m_text.empty() || m_renderCursor || m_focused) 
	{
		CTextLine::OnUpdate();
	} 
	else 
	{
		m_placeholderInstance.Update();
	}

	if (m_focused && m_selectionStart == m_selectionEnd) 
	{
		const auto now = ELTimer_GetMSec();
		if (now > m_cursorVisibilityTime) 
		{
			m_renderCursor = !m_renderCursor;
			m_cursorVisibilityTime = now + 500;
		}
	}
}

void CPythonEditLine::OnRender()
{
	if(!m_text.empty() || m_renderCursor || m_focused)
	{
		CTextLine::OnRender();
	} 
	else 
	{
		m_placeholderInstance.Render(m_rect.left, m_rect.top, 0);
	}

	if (m_selectionStart != m_selectionEnd)
		RenderSelection();

	if (m_focused && m_renderCursor)
		RenderCursor();
}

void CPythonEditLine::OnSetFocus()
{
	if (!m_enabled)
		return;

	assert(!m_focused && "Sanity check");

	m_focused = true;
	ForceShowCursor();

	RunCallback("OnSetFocus");
}

void CPythonEditLine::OnKillFocus()
{
	if (!m_enabled)
		return;

	assert(m_focused && "Sanity check");

	m_focused = false;
	m_renderCursor = false;

	RunCallback("OnKillFocus");
}

bool CPythonEditLine::OnMouseLeftButtonDown()
{
	if (!m_enabled)
		return false;

	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	MakeLocalPosition(lx, ly);

	const uint32_t pos = GetIndexFromPosition(lx, ly);

	m_cursorPosition = pos;
	m_selectionStart = pos;
	m_selectionEnd = pos;

	ForceShowCursor();
	return CWindow::OnMouseLeftButtonDown();
}

void CPythonEditLine::OnMouseDrag(int32_t x, int32_t y)
{
	if (!m_enabled)
		return;

	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	MakeLocalPosition(lx, ly);

	m_selectionEnd = m_cursorPosition = GetIndexFromPosition(lx, ly);
	ForceShowCursor();
}

bool CPythonEditLine::OnChar(uint32_t ch)
{
	if (!m_enabled)
		return false;

	// We don't handle character messages if we're not focused.
	if (!m_focused)
		return false;

	// Docs: http://www.asciitable.com/
	if (ch < 32)
		return true;

	// It should usually return true, except for number mode
	if (!CWindow::OnChar(ch))
		return true;

	char buffer[512];
	char* end = boost::locale::utf::utf_traits<char>::encode(ch, buffer);
	
	// Escape | as ||
	if (ch == '|')
		end = boost::locale::utf::utf_traits<char>::encode(ch, end);

	if (m_selectionStart != m_selectionEnd)
		EraseSelection();

	if (!Insert(m_cursorPosition, std::string(buffer, end - buffer))) 
	{
		return false;
	}

	ForceShowCursor();
	return true;
}

bool CPythonEditLine::OnKeyDown(KeyCode code)
{
	if (!m_enabled)
		return false;

	if (!m_focused)
		return false;

	ForceShowCursor();

	switch (code) 
	{
		case kVirtualKeyLeft:
			MoveCursor(-1, Engine::GetKeyboardInput().IsKeyPressed(kVirtualKeyShift));
			return true;

		case kVirtualKeyRight:
			MoveCursor(1, Engine::GetKeyboardInput().IsKeyPressed(kVirtualKeyShift));
			return true;

		case kVirtualKeyHome:
			MoveToBeginning();
			return true;

		case kVirtualKeyEnd:
			MoveToEnd();
			return true;

		case kVirtualKeyDelete:
		{
			if (m_selectionStart != m_selectionEnd)
			{
				EraseSelection();
				return true;
			}

			EraseGlyphs(m_cursorPosition, 1);
			return true;
		}

		case kVirtualKeyBack:
		{
			if (m_selectionStart != m_selectionEnd) 
			{
				EraseSelection();
				return true;
			}

			if (m_cursorPosition == 0)
				return true;

			// We want to remove the glyph preceding the current one.
			auto index = m_TextInstance.GetGlyphIndexFromStringIndex(m_cursorPosition);
			--index;
			m_cursorPosition = m_TextInstance.GetStringIndexFromGlyphIndex(index);

			EraseGlyphs(m_cursorPosition, 1);
			return true;
		}

		case kVirtualKeyV:
		case kVirtualKeyInsert:
		{
			// STRG+V only
			if (code == kVirtualKeyV && !Engine::GetKeyboardInput().IsKeyPressed(kVirtualKeyControl))
				break;

			std::string text;
			auto cc = GetClipboardContent(YITSORA_CF);
			if (cc.first) 
			{
				text.assign(reinterpret_cast<char*>(cc.first.get()));
			} 
			else 
			{
				text = GetClipboardText();
				if (text.empty())
					return true;

				boost::replace_all(text, "|", "||");
			}

			if (!RunCallback("OnPaste", text))
				return true;

			if (m_selectionStart != m_selectionEnd)
				EraseSelection();

			if (!Insert(m_cursorPosition, text))
				return true;

			return true;
		}

		case kVirtualKeyX:
		case kVirtualKeyC:
		{
			if (!Engine::GetKeyboardInput().IsKeyPressed(kVirtualKeyControl))
				break;

			if (m_selectionStart == m_selectionEnd)
				return true;

			std::pair<uint32_t, uint32_t> pos = std::minmax(m_selectionStart, m_selectionEnd);

			// Don't corrupt text-tags by cutting them
			m_selectionStart = GetTextTagBoundary(m_text, pos.first).first;
			m_selectionEnd = GetTextTagBoundary(m_text, pos.second).second;

			if (!ClearClipboard())
				return true;

			std::string text(m_text.begin() + m_selectionStart, m_text.begin() + m_selectionEnd);

			if (!SetClipboardContent(YITSORA_CF, text.c_str(), text.length() + 1))
				return true;

			std::string stripped = StripTextTags(text);

			if (!SetClipboardText(stripped)) 
			{
				ClearClipboard();
				return true;
			}

			if (code == kVirtualKeyX)
				EraseSelection();

			return true;
		}

		case kVirtualKeyA:
		{
			if (!Engine::GetKeyboardInput().IsKeyPressed(kVirtualKeyControl))
				break;

			m_selectionStart = 0;
			m_selectionEnd = m_cursorPosition = m_text.length();
			return true;
		}

		default:
			break;
	}

	// Pass it up the chain - This will invoke OnKeyDown() on
	// the bound python object, allowing client code to handle
	// special keys (e.g. ENTER)
	return CWindow::OnKeyDown(code);
}

bool CPythonEditLine::OnKeyUp(KeyCode code)
{
	// We leave handling key up events to the phase window
	// returning "handled" here leads to undesired behavior
	// like key ups getting stuck
	return false;
}

void CPythonEditLine::OnChangeText()
{
	m_cursorPosition = std::min<uint32_t>(m_cursorPosition, m_text.length());
	m_selectionStart = std::min<uint32_t>(m_selectionStart, m_text.length());
	m_selectionEnd = std::min<uint32_t>(m_selectionEnd, m_text.length());

	m_TextInstance.Update();
}

void CPythonEditLine::EraseSelection()
{
	if (m_selectionStart > m_selectionEnd)
		std::swap(m_selectionStart, m_selectionEnd);

	// We don't want people editing hyperlinks
	m_selectionStart = GetTextTagBoundary(m_text, m_selectionStart).first;
	m_selectionEnd = GetTextTagBoundary(m_text, m_selectionEnd).second;

	Erase(m_selectionStart, m_selectionEnd - m_selectionStart);
	m_cursorPosition = m_selectionEnd = m_selectionStart;
}

void CPythonEditLine::RenderSelection()
{
	auto first = m_TextInstance.GetGlyphIndexFromStringIndex(m_selectionStart);
	auto last = m_TextInstance.GetGlyphIndexFromStringIndex(m_selectionEnd);

	// Ensure proper ordering
	// (m_selectionStart isn't guranteed to be < m_selectionEnd)
	if (first > last)
		std::swap(first, last);

	ApplyRenderState state;

	auto firstLine = m_TextInstance.GetGlyphLine(first);
	const auto lastLine = m_TextInstance.GetGlyphLine(last);

	do 
	{
		uint32_t lineFirst, lineLast;
		m_TextInstance.GetLineGlyphs(firstLine, lineFirst, lineLast);

		if (first > lineFirst && first < lineLast)
			lineFirst = first;
		if (last > lineFirst && last < lineLast)
			lineLast = last;

		if (lineFirst == lineLast)
			continue;

		// Dummy coordinates we don't need
		float _1, _2;

		float sx, sy, ex, ey;
		m_TextInstance.GetGlyphPosition(lineFirst, sx, sy, _1, _2);
		m_TextInstance.GetGlyphPosition(lineLast - 1, _1, _2, ex, ey);

		sx += m_rect.left;
		sy += m_rect.top;
		ex += m_rect.left;
		ey += m_rect.top;

		RenderSelectionRect(sx, sy, ex, ey);
	} 
	while (firstLine++ != lastLine);
}

void CPythonEditLine::RenderCursor()
{
	const auto glyph = m_TextInstance.GetGlyphIndexFromStringIndex(m_cursorPosition);

	float sx, sy, ex, ey;
	m_TextInstance.GetGlyphPosition(glyph, sx, sy, ex, ey);
	ex = sx + 1.0f; // Cursor width is hardcoded

	sx += m_rect.left;
	sy += m_rect.top;
	ex += m_rect.left;
	ey += m_rect.top;

	TDiffuse diffuse = 0xffffffff;

	TPDTVertex vertices[4];
	vertices[0].diffuse = diffuse;
	vertices[1].diffuse = diffuse;
	vertices[2].diffuse = diffuse;
	vertices[3].diffuse = diffuse;
	vertices[0].position = TPosition(sx, sy, 0.0f);
	vertices[1].position = TPosition(ex, sy, 0.0f);
	vertices[2].position = TPosition(sx, ey, 0.0f);
	vertices[3].position = TPosition(ex, ey, 0.0f);

	ApplyRenderState state;

	CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);
	STATEMANAGER->SetFVF(TPDTVertex::kFVF);
	if (CGraphicBase::SetPDTStream(vertices, 4))
		STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
}

uint32_t CPythonEditLine::GetIndexFromPosition(int32_t x, int32_t y) const
{
	x = std::clamp<int32_t>(x, 0, m_TextInstance.GetWidth());
	y = std::clamp<int32_t>(y, 0, m_TextInstance.GetHeight());

	uint32_t glyphIndex;
	if (m_TextInstance.GetGlyphIndex(glyphIndex, x, y))
		return m_TextInstance.GetStringIndexFromGlyphIndex(glyphIndex);

	return m_text.length();
}

void CPythonEditLine::ForceShowCursor()
{
	m_cursorVisibilityTime = ELTimer_GetMSec() + 500;
	m_renderCursor = true;
}
