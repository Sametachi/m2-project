#include "StdAfx.h"
#include "GrpTextInstance.h"
#include "StateManager.h"
#include "TextTag.h"
#include "GrpImage.h"
#include "ResourceManager.h"
#include <EterBase/Utils.h>
#include <EterLib/Engine.h>
#include <GameLib/GameType.h>
#include <GameLib/ItemManager.h>

//#include "../EterPythonLib/PythonEmojiManager.h"
//#include "../UserInterface/PythonNonPlayer.h"

#include "../EterPythonLib/PythonGraphic.h"

#include <stdio.h>
#include <cstdint>
#include <Storm/UnicodeUtil.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/locale/utf.hpp>
#include "UTF8/utf8.h"

const float c_fFontFeather = 0.5f;

CGraphicTextInstance::CGraphicTextInstance()
	: m_textHasGradient{ false }
	, m_outlineHasGradient{ false }
	, m_dwTextColor(0xFFFFFFFF)
	, m_dwTextColor2(0xFFFFFFFF)
	, m_dwOutLineColor(0xFF000000)
	, m_dwOutLineColor2(0xFF000000)
	, m_textWidth(0)
	, m_textHeight(0)
	, m_maxLineHeight(0.0f)
	, m_fLimitWidth(1600.0f)
	, m_isSecret(false)
	, m_isMultiLine(false)
	, m_isOutline(false)
	, m_dirty(true)
	, m_font(nullptr)
	, m_sourceLength(0)
	, m_lineCount(0)
{
}

CGraphicTextInstance::~CGraphicTextInstance()
{
	for (auto& characters : m_chars)
		delete characters.emoji;

	m_chars.clear();
}

void CGraphicTextInstance::Update()
{
	if (!m_dirty)
		return;

	if (!m_font)
	{
		return;
	}

	CGraphicFontTexture& texture = m_font->GetTexturePointer();

	m_maxLineHeight = m_font->getFontInfo().ascender;
	m_textWidth = 0;
	m_lineCount = 0;

	CGraphicFontTexture::GlyphData* replace = nullptr;
	if (m_isSecret) 
	{
		replace = texture.GetCharacterInfomation('*');
		assert(replace && "No * character");
	}

	float curX = 0.0f;
	float curY = 0.0f;

	for (auto& characters : m_chars) 
	{
		characters.info = replace ? replace : texture.GetCharacterInfomation(characters.ch);

		if ((characters.ch == '\n') && m_isMultiLine)
		{
			curX = 0.0f;
			curY += m_maxLineHeight;
			++m_lineCount;

			// Ignore this character
			characters.info = nullptr;
			continue;
		}

		if ((characters.ch == ' '))
		{
			characters.x = curX;
			characters.y = curY;
			characters.line = m_lineCount;

			curX += texture.GetSpaceWidth();
			m_textWidth = std::max<uint16_t>(m_textWidth, static_cast<uint16_t>(curX));

			// Ignore this character
			characters.info = nullptr;
			continue;
		}

		if (characters.ch == '\t')
		{
			characters.x = curX;
			characters.y = curY;
			characters.line = m_lineCount;

			curX += (texture.GetSpaceWidth()) * 4;
			m_textWidth = std::max<uint16_t>(m_textWidth, static_cast<uint16_t>(curX));

			// Ignore this character
			characters.info = nullptr;
			continue;
		}

		if (!characters.info)
			continue;

		if (curX + characters.info->_width > m_fLimitWidth)
		{
			if (m_isMultiLine)
			{
				curX = 0.0f;
				curY += m_maxLineHeight;
				++m_lineCount;
			}
			else 
			{
				// Ignore this character
				characters.info = nullptr;
				break;
			}
		}

		m_maxLineHeight = std::max<float>(m_maxLineHeight, characters.info->_height);
		m_textWidth = std::max<uint16_t>(m_textWidth, static_cast<uint16_t>(curX) + characters.info->_advance);

		characters.x = curX;
		characters.y = curY;
		characters.line = m_lineCount;

		curX += characters.info->_advance;
	}

	++m_lineCount;
	m_textHeight = static_cast<uint16_t>(curY) + static_cast<uint16_t>(m_maxLineHeight);
	m_dirty = false;
}

bool LiesEntirelyOutsideRect(const RECT& r, const D3DXVECTOR2& start, const D3DXVECTOR2& end)
{
	return (start.x >= r.right || end.x < r.left) && (start.y >= r.bottom || end.y < r.top);
}

bool LiesEntirelyInsideRect(const RECT& r, const D3DXVECTOR2& start, const D3DXVECTOR2& end)
{
	return start.x >= r.left && end.x < r.right && start.y >= r.top && end.y < r.bottom;
}

struct TextRenderState 
{
	TextRenderState()
	{
		STATEMANAGER->SaveFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);

		STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, true);
		STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		STATEMANAGER->SaveRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		STATEMANAGER->SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		STATEMANAGER->SaveRenderState(D3DRS_FOGENABLE, FALSE);

		CPythonGraphic::GetInstance()->SetBlendOperation();

		STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);

		STATEMANAGER->SaveSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
		STATEMANAGER->SaveSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC);
		STATEMANAGER->SaveSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
	}

	~TextRenderState()
	{
		STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
		STATEMANAGER->RestoreRenderState(D3DRS_SRCBLEND);
		STATEMANAGER->RestoreRenderState(D3DRS_DESTBLEND);
		STATEMANAGER->RestoreRenderState(D3DRS_FILLMODE);
		STATEMANAGER->RestoreRenderState(D3DRS_CULLMODE);

		STATEMANAGER->RestoreRenderState(D3DRS_FOGENABLE);

		STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXCOORDINDEX);
		STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS);

		STATEMANAGER->RestoreSamplerState(0, D3DSAMP_MINFILTER);
		STATEMANAGER->RestoreSamplerState(0, D3DSAMP_MAGFILTER);
		STATEMANAGER->RestoreSamplerState(0, D3DSAMP_MIPFILTER);

		STATEMANAGER->RestoreFVF();
	}
};

void CGraphicTextInstance::Render(int32_t x, int32_t y, int32_t z, const RECT* clip)
{
	if (m_dirty)
		return;

	if (!m_font)
	{
		SysLog("TextInstance can't render Nothing?")
		return;
	}

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CGraphicTextInstance::Render **");

	const D3DXVECTOR2 pos(x, y);

	auto fontInfo = m_font->getFontInfo();

	std::map<CGraphicImageTexture*, std::vector<TPDTVertex>> verticesMap;

	TPDTVertex vertices[4];
	vertices[0].position.z = z;
	vertices[1].position.z = z;
	vertices[2].position.z = z;
	vertices[3].position.z = z;

	const float fFontHalfWeight = 1.0f;
	constexpr float texelOfset = -0.5f;

	if (m_isOutline) 
	{
		for (const auto& characters : m_chars)
		{
			auto pCurCharInfo = characters.info;
			if (!characters.info)
				continue;

			if (characters.emoji) // Not rendering X for emojis anymore (template position)
				continue;

			const D3DXVECTOR2 size(pCurCharInfo->_width, pCurCharInfo->_height);
			const D3DXVECTOR2 start(characters.x + pCurCharInfo->_bearingX, characters.y + fontInfo.ascender + pCurCharInfo->_bearingY);
			const D3DXVECTOR2 end = start + size;

			// Don't even bother sending those to the driver...
			if (clip && LiesEntirelyOutsideRect(*clip, start, end))
				continue;

			const auto tex = characters.info->_tex;
			auto& batchVertices = verticesMap[tex];

			vertices[0].texCoord.x = pCurCharInfo->_left;
			vertices[0].texCoord.y = pCurCharInfo->_top;

			vertices[1].texCoord.x = pCurCharInfo->_left;
			vertices[1].texCoord.y = pCurCharInfo->_bottom;

			vertices[2].texCoord.x = pCurCharInfo->_right;
			vertices[2].texCoord.y = pCurCharInfo->_top;

			vertices[3].texCoord.x = pCurCharInfo->_right;
			vertices[3].texCoord.y = pCurCharInfo->_bottom;

			if (m_outlineHasGradient)
			{
				vertices[3].diffuse = m_dwOutLineColor2;
				vertices[2].diffuse = m_dwOutLineColor;
				vertices[1].diffuse = m_dwOutLineColor2;
				vertices[0].diffuse = m_dwOutLineColor;
			}
			else
			{
				vertices[3].diffuse = m_dwOutLineColor;
				vertices[2].diffuse = m_dwOutLineColor;
				vertices[1].diffuse = m_dwOutLineColor;
				vertices[0].diffuse = m_dwOutLineColor;
			}

			vertices[0].position.y = pos.y + start.y - texelOfset;
			vertices[1].position.y = pos.y + end.y - texelOfset;
			vertices[2].position.y = pos.y + start.y - texelOfset;
			vertices[3].position.y = pos.y + end.y - texelOfset;

			vertices[0].position.x = pos.x + start.x - texelOfset;
			vertices[1].position.x = pos.x + start.x - texelOfset;
			vertices[2].position.x = pos.x + end.x - texelOfset;
			vertices[3].position.x = pos.x + end.x - texelOfset;

			batchVertices.emplace_back(vertices[0]);
			batchVertices.emplace_back(vertices[1]);
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[1]); //[1]
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[3]);

			vertices[0].position.y = pos.y + start.y - texelOfset;
			vertices[1].position.y = pos.y + end.y - texelOfset;
			vertices[2].position.y = pos.y + start.y - texelOfset;
			vertices[3].position.y = pos.y + end.y - texelOfset;

			vertices[0].position.x = pos.x + start.x - fFontHalfWeight - texelOfset;
			vertices[1].position.x = pos.x + start.x - fFontHalfWeight - texelOfset;
			vertices[2].position.x = pos.x + end.x - fFontHalfWeight - texelOfset;
			vertices[3].position.x = pos.x + end.x - fFontHalfWeight - texelOfset;

			batchVertices.emplace_back(vertices[0]);
			batchVertices.emplace_back(vertices[1]);
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[1]); //[1]
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[3]);

			vertices[0].position.x = pos.x + start.x + fFontHalfWeight - texelOfset;
			vertices[1].position.x = pos.x + start.x + fFontHalfWeight - texelOfset;
			vertices[2].position.x = pos.x + end.x + fFontHalfWeight - texelOfset;
			vertices[3].position.x = pos.x + end.x + fFontHalfWeight - texelOfset;

			batchVertices.emplace_back(vertices[0]);
			batchVertices.emplace_back(vertices[1]);
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[1]); //[1]
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[3]);

			vertices[0].position.x = pos.x + start.x - texelOfset;
			vertices[1].position.x = pos.x + start.x - texelOfset;
			vertices[2].position.x = pos.x + end.x - texelOfset;
			vertices[3].position.x = pos.x + end.x - texelOfset;

			vertices[0].position.y = pos.y + start.y - fFontHalfWeight - texelOfset;
			vertices[1].position.y = pos.y + end.y - fFontHalfWeight - texelOfset;
			vertices[2].position.y = pos.y + start.y - fFontHalfWeight - texelOfset;
			vertices[3].position.y = pos.y + end.y - fFontHalfWeight - texelOfset;

			batchVertices.emplace_back(vertices[0]);
			batchVertices.emplace_back(vertices[1]);
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[1]); //[1]
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[3]);

			vertices[0].position.y = pos.y + start.y + fFontHalfWeight - texelOfset;
			vertices[1].position.y = pos.y + end.y + fFontHalfWeight - texelOfset;
			vertices[2].position.y = pos.y + start.y + fFontHalfWeight - texelOfset;
			vertices[3].position.y = pos.y + end.y + fFontHalfWeight - texelOfset;

			batchVertices.emplace_back(vertices[0]);
			batchVertices.emplace_back(vertices[1]);
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[1]); //[1]
			batchVertices.emplace_back(vertices[2]);
			batchVertices.emplace_back(vertices[3]);
		}
	}

	uint32_t clippingWasEnabled = STATEMANAGER->GetRenderState(D3DRS_SCISSORTESTENABLE);;
	RECT oldRect;
	RECT newRect{};

	if (clip) 
	{
		const RECT r = 
		{
			pos.x + clip->left,
			pos.y + clip->top,
			pos.x + clip->right,
			pos.y + clip->bottom,
		};

		STATEMANAGER->GetDevice()->GetScissorRect(&oldRect);
		STATEMANAGER->GetDevice()->SetScissorRect(&r);
		STATEMANAGER->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	}

	for (const auto& characters : m_chars)
	{
		auto const pCurCharInfo = characters.info;
		if (!characters.info)
		{
			continue;
		}

		const D3DXVECTOR2 size(pCurCharInfo->_width, pCurCharInfo->_height);
		const D3DXVECTOR2 start(characters.x + pCurCharInfo->_bearingX, characters.y + fontInfo.ascender + pCurCharInfo->_bearingY);
		const D3DXVECTOR2 end = start + size;

		// Don't even bother sending those to the driver...
		if (clip && LiesEntirelyOutsideRect(*clip, start, end))
			continue;

		//if (characters.emoji && Engine::GetSettings().IsAllowEmojis())
		//{
		//    characters.emoji->SetPosition(pos.x + characters.x, pos.y + characters.y);
		//    characters.emoji->Render();
		//    continue;
		//}

		const auto tex = characters.info->_tex;
		auto& batchVertices = verticesMap[tex];

		vertices[0].position.x = pos.x + start.x - texelOfset;
		vertices[0].position.y = pos.y + start.y - texelOfset;
		vertices[0].texCoord.x = pCurCharInfo->_left;
		vertices[0].texCoord.y = pCurCharInfo->_top;

		vertices[1].position.x = pos.x + start.x - texelOfset;
		vertices[1].position.y = pos.y + end.y - texelOfset;
		vertices[1].texCoord.x = pCurCharInfo->_left;
		vertices[1].texCoord.y = pCurCharInfo->_bottom;

		vertices[2].position.x = pos.x + end.x - texelOfset;
		vertices[2].position.y = pos.y + start.y - texelOfset;
		vertices[2].texCoord.x = pCurCharInfo->_right;
		vertices[2].texCoord.y = pCurCharInfo->_top;

		vertices[3].position.x = pos.x + end.x - texelOfset;
		vertices[3].position.y = pos.y + end.y - texelOfset;
		vertices[3].texCoord.x = pCurCharInfo->_right;
		vertices[3].texCoord.y = pCurCharInfo->_bottom;

		if (m_textHasGradient) 
		{
			vertices[0].diffuse = characters.color;
			vertices[1].diffuse = characters.color2;
			vertices[2].diffuse = characters.color;
			vertices[3].diffuse = characters.color2;
		}
		else 
		{
			vertices[0].diffuse = characters.color;
			vertices[1].diffuse = characters.color;
			vertices[2].diffuse = characters.color;
			vertices[3].diffuse = characters.color;
		}

		batchVertices.emplace_back(vertices[0]);
		batchVertices.emplace_back(vertices[1]);
		batchVertices.emplace_back(vertices[2]);
		batchVertices.emplace_back(vertices[1]); //[1]
		batchVertices.emplace_back(vertices[2]);
		batchVertices.emplace_back(vertices[3]);
	}

	{
		auto state = TextRenderState();

		for (auto& d3dmap : verticesMap)
		{
			if (!d3dmap.first)
				continue;

			STATEMANAGER->SetTexture(0, d3dmap.first->GetD3DTexture());

			for (auto vertices = d3dmap.second.begin(), largeBuffer = d3dmap.second.end(); vertices != largeBuffer;)
			{
				const auto batchCount = std::min<std::size_t>(kLargePdtVertexBufferSize, largeBuffer - vertices);

				if (SetPDTStream(&*vertices, batchCount))
				{
					STATEMANAGER->DrawPrimitive(D3DPT_TRIANGLELIST, 0, batchCount / 3);
				}

				vertices += batchCount;
			}
		}

		if (clip) 
		{
			STATEMANAGER->GetDevice()->SetScissorRect(&oldRect);
			STATEMANAGER->GetDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, clippingWasEnabled);
		}
	}
	D3DPERF_EndEvent();
}

void CGraphicTextInstance::SetColor(uint32_t color)
{
	if (m_dwTextColor == color)
		return;

	for (auto& characters : m_chars) 
	{
		if (characters.color == m_dwTextColor)
			characters.color = color;
	}

	m_dwTextColor = color;
}

std::pair<uint32_t, uint32_t> CGraphicTextInstance::GetColorGradient() const
{
	return std::make_pair(m_dwTextColor, m_dwTextColor2);
}

void CGraphicTextInstance::SetColorGradient(uint32_t color, uint32_t color2)
{
	if (m_dwTextColor == color && m_dwTextColor2 == color2)
		return;

	for (auto& characters : m_chars)
	{
		characters.color = color;
		characters.color2 = color2;
	}

	m_textHasGradient = color != color2;
	m_dwTextColor = color;
	m_dwTextColor2 = color2;
}

void CGraphicTextInstance::SetColor(float r, float g, float b, float a)
{
	SetColor(D3DXCOLOR(r, g, b, a));
}

void CGraphicTextInstance::SetOutLineColor(uint32_t color)
{
	m_dwOutLineColor = color;
}

void CGraphicTextInstance::SetOutLineColorGradient(uint32_t color, uint32_t color2)
{
	if (m_dwOutLineColor == color && m_dwOutLineColor2 == color2)
		return;

	if (m_isOutline)
	{
		for (auto& characters : m_chars)
		{
			characters.color = color;
			characters.color2 = color2;
		}
	}

	m_outlineHasGradient = color != color2;
	m_dwOutLineColor = color;
	m_dwOutLineColor2 = color2;
}

void CGraphicTextInstance::SetOutLineColor(float r, float g, float b, float a)
{
	m_dwOutLineColor = D3DXCOLOR(r, g, b, a);
}

void CGraphicTextInstance::SetOutline(bool value)
{
	m_isOutline = value;
}

void CGraphicTextInstance::SetSecret(bool Value)
{
	m_isSecret = Value;
	m_dirty = true;
}

void CGraphicTextInstance::SetMultiLine(bool Value)
{
	m_isMultiLine = Value;
	m_dirty = true;
}

void CGraphicTextInstance::SetLimitWidth(float fWidth)
{
	m_fLimitWidth = fWidth;
	m_dirty = true;
}

void CGraphicTextInstance::SetTextPointer(CGraphicFontType* pText)
{
	m_font = pText;
	m_dirty = true;
}

uint32_t CGraphicTextInstance::GetWidth() const
{
	return m_textWidth;
}

uint32_t CGraphicTextInstance::GetHeight() const
{
	return m_textHeight;
}

uint32_t CGraphicTextInstance::GetLineCount() const
{
	return m_lineCount;
}

void CGraphicTextInstance::HandleHyperLink(uint32_t color, TextTag tag, const std::string::difference_type sourcePos)
{
	std::vector<std::string> strs;
	boost::split(strs, tag.content, boost::is_any_of(":"));
	if (!strs.empty()) 
	{
/*
		// TODO(05): Enable TextInstance render extension for growth pet items ("itempet")
		if (strs[0] == "item")
		{
			int itemVnum = std::stoi(strs[1], 0, 16);
			auto pItemData = CItemManager::GetInstance()->GetProto(itemVnum);
			if (pItemData) 
			{

				auto socket0 = std::stoll(strs[3]);
				auto socket1 = std::stoll(strs[4]);
				auto socket2 = std::stoll(strs[5]);

				std::string itemName = "[";
				if (itemVnum >= 70103 && itemVnum <= 70106) 
				{
					if (socket0 != 0) 
					{
						auto mobName = CPythonNonPlayer::GetInstance()->GetName(socket0);
						if (mobName) 
						{
							itemName.append(mobName.value());
							itemName.append(" ");
						}
					}
				}

				itemName.append(pItemData->GetName());
				itemName.append("]");
				auto end_it = utf8::find_invalid(itemName.begin(), itemName.end());

				std::u32string utf32line;
				utf8::utf8to32(itemName.begin(), end_it, back_inserter(utf32line));

				auto basePos = sourcePos;
				for (const auto& character : utf32line) 
				{
					AppendCharacter(character, color, color, basePos);
					++basePos;
				}
			}
		}
		else if (strs[0] == "itemname") 
		{
			int itemVnum = std::stoi(strs[1]);
			auto pItemData = CItemManager::GetInstance()->GetProto(itemVnum);
			if (pItemData) 
			{
				std::string itemName = pItemData->GetName();
				auto end_it = utf8::find_invalid(itemName.begin(), itemName.end());

				std::u32string utf32line;
				utf8::utf8to32(itemName.begin(), end_it, back_inserter(utf32line));

				auto basePos = sourcePos;
				for (const auto& character : utf32line) 
				{
					AppendCharacter(character, color, color, basePos);
					++basePos;
				}
			}
		}

		else if (strs[0] == "mobname") 
		{
			int mobVnum = std::stoi(strs[1]);
			auto mobData = CPythonNonPlayer::GetInstance()->GetName(mobVnum);
			if (mobData) 
			{
				std::string mobName = mobData.value();
				auto end_it = utf8::find_invalid(mobName.begin(), mobName.end());

				std::u32string utf32line;
				utf8::utf8to32(mobName.begin(), end_it, back_inserter(utf32line));

				auto basePos = sourcePos;
				for (const auto& character : utf32line) 
				{
					AppendCharacter(character, color, color, basePos);
					++basePos;
				}
			}
		}
	*/
	}
}

void CGraphicTextInstance::HandleEmoticon(uint32_t color, TextTag tag, const std::string::difference_type sourcePos)
{
	CGraphicImageInstance* inst = nullptr;
	CGraphicImage* r = nullptr;
	/*
	if (tag.content.find(':') == std::string::npos)
	{
		r = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(PythonEmojiManager::Instance().GetFilePath(tag.content)->c_str());
		if (r)
		{
			inst = new CGraphicImageInstance;
		}
	}
	else
	{
		std::vector<std::string> strs;
		boost::split(strs, tag.content, boost::is_any_of(":"));
		auto& type = strs[0];
		if (type == "item") 
		{
			auto itemVnum = std::stoi(strs[1]);
			auto pItemData = CItemManager::GetInstance()->GetProto(itemVnum);
			if (pItemData) 
			{
				inst = new CGraphicImageInstance;
				r = pItemData->GetIconImage();
				inst->SetScale(0.7, 0.7);
			}
		}
	}

	auto spaceWidth = m_font->GetTexturePointer().GetSpaceWidth();
	if (inst) 
	{
		inst->SetImagePointer(r);
		AppendCharacter('x', color, color, sourcePos, inst);

		for (int i = 0; i < inst->GetWidth() / (spaceWidth); ++i)
			AppendCharacter(' ', color, color, sourcePos);

		if (inst->GetWidth() % ((uint32_t)spaceWidth) > 1)
			AppendCharacter(' ', color, color, sourcePos);
	}
	*/
}

void CGraphicTextInstance::SetValue(std::string value)
{
	for (const auto& characters : m_chars)
		delete characters.emoji;

	if (m_isMultiLine)
	{
		boost::replace_all(value, "\\n", "\n");
	}

	m_chars.clear();
	m_chars.reserve(value.length());

	uint32_t color = m_dwTextColor;
	uint32_t color2 = m_dwTextColor2;
	TextTag tag;

	for (auto first = value.begin(), last = value.end(); first != last;) 
	{
		const auto sourcePos = first - value.begin();

		if (GetTextTag(value.substr(sourcePos), tag)) 
		{
			if (tag.type == TEXT_TAG_COLOR)
			{
				color = strtoul(std::string(tag.content).c_str(), nullptr, 16);
				color2 = strtoul(std::string(tag.content).c_str(), nullptr, 16);
			}
			else if (tag.type == TEXT_TAG_EMOTICON_START) 
			{
				HandleEmoticon(color, tag, sourcePos);
			}
			else if (tag.type == TEXT_TAG_HYPERLINK_START) 
			{
				HandleHyperLink(color, tag, sourcePos);
			}
			else if (tag.type == TEXT_TAG_RESTORE_COLOR)
			{
				color = m_dwTextColor;
				color2 = m_dwTextColor2;
			}
			else if (tag.type == TEXT_TAG_TAG)
				AppendCharacter('|', color, color, sourcePos);

			first += tag.length;
			continue;
		}

		// Note: decode() advances first to one-past-last-read.
		AppendCharacter(boost::locale::utf::utf_traits<char>::decode(first, last), color, color2, sourcePos);
	}
	m_sourceLength = value.length();
	m_dirty = true;
}

void CGraphicTextInstance::GetTextSize(int32_t* pRetWidth, int32_t* pRetHeight)
{
	*pRetWidth = m_textWidth;
	*pRetHeight = m_textHeight;
}

/*
	Glyph implementation
*/

uint32_t CGraphicTextInstance::GetGlyphLine(uint32_t index)
{
	if (index == m_chars.size())
		return m_lineCount;

	return m_chars[index].line;
}

void CGraphicTextInstance::GetLineGlyphs(uint32_t line, uint32_t& first, uint32_t& last)
{
	struct CharacterLineCmp 
	{
		bool operator()(const Character& c, uint32_t line)
		{
			return c.line < line;
		}

		bool operator()(uint32_t line, const Character& c)
		{
			return line < c.line;
		}
	};

	const auto r = std::equal_range(m_chars.begin(), m_chars.end(), line, CharacterLineCmp());

	first = r.first - m_chars.begin();
	last = r.second - m_chars.begin();
}

void CGraphicTextInstance::GetGlyphPosition(uint32_t index, float& sx, float& sy, float& ex, float& ey) const
{
	assert(index <= m_chars.size() && "Range not Exist!");

	if (m_chars.empty()) 
	{
		sx = 0.0f; ex = 1.0f;
		sy = 0.0f; ey = m_maxLineHeight;
		return;
	}

	if (index != m_chars.size()) 
	{
		const auto& ch = m_chars[index];

		sx = ch.x;
		sy = ch.y;
		ex = ch.x + (ch.info ? ch.info->_advance : m_font->GetTexturePointer().GetSpaceWidth());
		ey = ch.y + m_maxLineHeight;
	}
	else 
	{
		// Use one-before-last as starting point.
		const auto& ch = m_chars[static_cast<std::vector<CGraphicTextInstance::Character, std::allocator<CGraphicTextInstance::Character>>::size_type>(index) - 1];

		sx = ch.x + (ch.info ? ch.info->_advance : m_font->GetTexturePointer().GetSpaceWidth());
		sy = ch.y;
		ex = sx + 1.0f;
		ey = sy + m_maxLineHeight;
	}
}

bool CGraphicTextInstance::GetGlyphIndex(uint32_t& index, float x, float y) const
{
	uint32_t first, last;
	if (!FindLineForY(first, last, y))
		return false;

	for (uint32_t i = first; i != last; ++i) 
	{
		const auto& ch = m_chars[i];
		if (ch.ch == ' ' || ch.ch == '\t') 
		{
			if (x >= ch.x && x < ch.x + m_font->GetTexturePointer().GetSpaceWidth() * (ch.ch == ' ') ? 1 : 4)
			{
				index = i;
				return true;
			}
		}

		if (!ch.info)
			continue;

		if (x >= ch.x && x < ch.x + ch.info->_advance) 
		{
			index = i;
			return true;
		}
	}

	return false;
}

uint32_t CGraphicTextInstance::GetStringIndexFromGlyphIndex(uint32_t index) const
{
	// If we have no glyphs or not existing range (which is valid!), we simply return the source-string length.
	if (m_chars.empty() || index >= m_chars.size())
		return m_sourceLength;

	return m_chars[index].sourcePos;
}

uint32_t CGraphicTextInstance::GetGlyphIndexFromStringIndex(uint32_t index) const
{
	auto cmpSourcePos = [](const Character& a, uint32_t p) -> bool
	{
		return a.sourcePos < p;
	};

	const auto it = std::lower_bound(m_chars.begin(), m_chars.end(), index, cmpSourcePos);
	return it - m_chars.begin();
}

uint32_t CGraphicTextInstance::GetGlyphCount() const
{
	return m_chars.size();
}

void CGraphicTextInstance::AppendCharacter(uint32_t code, uint32_t color, uint32_t color2, uint32_t sourcePos, CGraphicImageInstance* emoji)
{
	// Left incase?
	//if (code == -1 || code >= 1000)
	//	code = 32;

	if (m_font)
		m_font->GetTexturePointer().GetCharacterInfomation(code);

	Character ch = {};
	ch.sourcePos = sourcePos;
	ch.ch = code;
	ch.info = nullptr;
	ch.color = color;
	ch.color2 = color2;
	ch.emoji = emoji;
	m_chars.emplace_back(ch);
}

#pragma optimize( "", off )
bool CGraphicTextInstance::FindLineForY(uint32_t& first, uint32_t& last, float y) const
{
	first = 0;
	float curY = 0.0f;

	for (uint32_t i = 0, size = m_chars.size(); i != size; ++i) 
	{
		const auto& ch = m_chars[i];
		if (!ch.info && ch.ch != ' ')
			continue;

		if (ch.y != curY) 
		{
			last = i;
			curY = ch.y;

			// If our new Y is greater than the target line's Y, we just finished the target line.
			// We return true here, so |first| still contains the last line's start-index.
			if (y <= curY)
				return true;

			first = i;
		}
	}

	// Special handling for last incomplete line
	if (y <= curY + m_maxLineHeight) 
	{
		last = m_chars.size();
		return true;
	}

	return false;
}
#pragma optimize( "", on )