#include "StdAfx.h"
#include "GrpFontTexture.h"
#include "Util.h"

#include <boost/locale/utf.hpp>
#include <storm/StringFlags.hpp>
#include FT_GLYPH_H
#include FT_BITMAP_H
#include FT_STROKER_H
#include <VFE/Include/VFE.hpp>

CGraphicFontTexture::CGraphicFontTexture()
{
	m_activeTex = nullptr;
	m_face = nullptr;
}

CGraphicFontTexture::~CGraphicFontTexture()
{
	for (const auto ch : m_textures)
	{
		delete ch;
	}
	m_textures.clear();

	for (auto& fft : m_codePoints)
		delete fft.second;

	m_codePoints.clear();
}

CGraphicImageTexture* CGraphicFontTexture::AppendTexture()
{
	auto pNewTexture = new CGraphicImageTexture;

	if (!pNewTexture->Create(_TEXTURE_SIZE, _TEXTURE_SIZE, D3DFMT_A8R8G8B8))
	{
		delete pNewTexture;
		return nullptr;
	}

	m_textures.push_back(pNewTexture);
	return pNewTexture;
}

bool CGraphicFontTexture::UpdateTexture()
{
	/* ToDo for Animated text line! */
	return true;
}

bool CGraphicFontTexture::Create(FT_Face face, int32_t fontSize)
{
	m_face = face;
	m_fontSize = fontSize;
	m_verticalOffset = (uint32_t)(m_fontSize * 1.2) + 2;
	m_activeTex = AppendTexture();
	m_penPos.x = 0.0f;
	m_penPos.y = 0.0f;

	for (int32_t i = 33; i < 0x2320; ++i)
	{
		AddCodePoint(i);
	}

	AddCodePoint(0x000020AC); // Euro Sign
	AddCodePoint(0xf0f3);     // Font Awesome Bell Icon
	AddCodePoint(0xf3c5);

	return true;
}

CGraphicFontTexture::GlyphData* CGraphicFontTexture::GetCharacterInfomation(uint32_t code)
{
	auto it = m_codePoints.find(code);
	if (it == m_codePoints.end())
	{
		AddCodePoint(code);
	}

	if (!m_codePoints[code])
		return m_codePoints['?'];

	return m_codePoints[code];
}

void CGraphicFontTexture::AddCodePoint(uint32_t code)
{
	if (!m_face)
		return;

	auto glyphIndex = FT_Get_Char_Index(m_face, code);

	auto error = FT_Load_Char(m_face, code, FT_LOAD_NO_BITMAP); // No bitmap -> the bitmap will be rendered manually
	if (error)
	{
		return;
	}

	FT_GlyphSlot slot = m_face->glyph;

	FT_Glyph glyph;
	error = FT_Get_Glyph(slot, &glyph);
	if (error)
	{
		return;
	}

	error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
	if (error)
	{
		return;
	}

	FT_BitmapGlyph bitmap = (FT_BitmapGlyph)glyph;

	int32_t xx = bitmap->left;
	int32_t yy = -bitmap->top;
	int32_t ww = bitmap->bitmap.width;
	int32_t hh = bitmap->bitmap.rows;

	GlyphData* fft = new GlyphData;
	fft->_width = (float)ww;
	fft->_height = (float)hh;
	fft->_bearingX = xx;
	fft->_bearingY = yy;

	fft->_advance = slot->advance.x >> 6;

	auto buffer = new unsigned int[bitmap->bitmap.width * bitmap->bitmap.rows];
	memset(buffer, 0, sizeof(unsigned int) * bitmap->bitmap.width * bitmap->bitmap.rows);

	switch (bitmap->bitmap.pixel_mode)
	{
	case FT_PIXEL_MODE_MONO:
	{
		for (int i = 0; i < bitmap->bitmap.rows; ++i)
		{
			uint8_t* src = bitmap->bitmap.buffer + (i * bitmap->bitmap.pitch);
			int32_t pitch = i * bitmap->bitmap.rows;
			int32_t j;
			for (j = 0; j < bitmap->bitmap.width; ++j)
				buffer[j + pitch] = (src[j / 8] & (0x80 >> (j & 7)))
				? 0xFFFFFFFF : 0x00000000;
		}
	}
	break;
	case FT_PIXEL_MODE_GRAY:
	{
		for (int32_t i = 0; i < bitmap->bitmap.rows; ++i)
		{
			for (int32_t j = 0; j < bitmap->bitmap.width; ++j)
			{
				uint8_t c = bitmap->bitmap.buffer[i * bitmap->bitmap.pitch + j];
				if (c > 0)
					buffer[i * bitmap->bitmap.width + j] = (c << 24) | (0xFF << 16) | (0xFF << 8) | 0xFF;
			}
		}
	}
	}

	if (nullptr == m_activeTex)
	{
		m_activeTex = AppendTexture();
		if (nullptr == m_activeTex)
		{
			FT_Done_Glyph(glyph);
			return;
		}
		m_penPos.x = 0;
		m_penPos.y = 0;
	}

	if (m_penPos.x + fft->_width > _TEXTURE_SIZE)
	{
		m_penPos.x = 0;
		m_penPos.y += m_verticalOffset;
		if (m_penPos.y + m_verticalOffset > _TEXTURE_SIZE)
		{
			m_activeTex = AppendTexture();
			if (nullptr == m_activeTex)
			{
				FT_Done_Glyph(glyph);

				return;
			}
			m_penPos.y = 0;
		}
		else
		{
			// ?
		}
	}
	m_activeTex->SetSubData(0, (uint32_t)m_penPos.x, (uint32_t)m_penPos.y, fft->_width, fft->_height, 4 * fft->_width, buffer, D3DFMT_A8R8G8B8);

	if (buffer)
	{
		delete[] buffer;
		buffer = 0;
	}

	float bround = 0.001f;
	fft->_left = m_penPos.x * _INVERSE_TEXTURE_SIZE;
	fft->_top = m_penPos.y * _INVERSE_TEXTURE_SIZE;
	fft->_right = (m_penPos.x + fft->_width) * _INVERSE_TEXTURE_SIZE;
	fft->_bottom = (m_penPos.y + fft->_height) * _INVERSE_TEXTURE_SIZE;
	fft->_tex = m_activeTex;

	m_codePoints[code] = fft;
	m_penPos.x += fft->_width;
	m_penPos.x += 4;

	FT_Done_Glyph(glyph);
}

const float CGraphicFontTexture::_INVERSE_TEXTURE_SIZE(1.0f / float(_TEXTURE_SIZE));
