#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include "Resource.h"
#include "Ref.h"
#include "GrpFontTexture.h"

struct FontInfo
{
	uint16_t pixelSize;
	int16_t fontType;
	float ascender;
	float descender;
	float lineGap;
	float maxAdvanceWidth;
	float underlineThickness;
	float underlinePosition;
	float scale;
};

class CGraphicFontType
{
public:
	const std::string& GetName() const { return m_fileName; };
	const std::string& GetFamilyName() const { return m_familyName; };

	bool Create(FT_Face face, uint8_t size);
	FontInfo getFontInfo() const;

	CGraphicFontTexture& GetTexturePointer();

private:
	std::string m_fileName;
	std::string m_familyName;
	std::string m_styleName;
	FontInfo m_info = {};
	mutable std::string m_formattedName;
	int32_t m_fontSize = 0;

	CGraphicFontTexture m_fontTexture;
	FT_Face m_face = nullptr;
};
