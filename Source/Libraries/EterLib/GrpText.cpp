#include "StdAfx.h"
#include "GrpText.h"
#include <EterBase/Utils.h>
#include <EterLib/Engine.h>

CGraphicFontTexture& CGraphicFontType::GetTexturePointer()
{
	return m_fontTexture;
}

bool CGraphicFontType::Create(FT_Face face, uint8_t size)
{
    m_face = face;
    m_familyName = m_face->family_name;
    m_styleName = m_face->style_name;
    m_fontSize = size;

    return m_fontTexture.Create(m_face, size);
}

FontInfo CGraphicFontType::getFontInfo() const
{
    assert(m_face != NULL && "TrueTypeFont not initialized");
    assert(FT_IS_SCALABLE(m_face) && "Font is unscalable");

    FT_Size_Metrics metrics = m_face->size->metrics;

    FontInfo outFontInfo;
    outFontInfo.scale = 1.0f;
    outFontInfo.ascender = metrics.ascender / 64.0f;
    outFontInfo.descender = metrics.descender / 64.0f;
    outFontInfo.lineGap = (metrics.height - metrics.ascender + metrics.descender) / 64.0f;
    outFontInfo.maxAdvanceWidth = metrics.max_advance / 64.0f;

    outFontInfo.underlinePosition = FT_MulFix(m_face->underline_position, metrics.y_scale) / 64.0f;
    outFontInfo.underlineThickness = FT_MulFix(m_face->underline_thickness, metrics.y_scale) / 64.0f;
    return outFontInfo;
}