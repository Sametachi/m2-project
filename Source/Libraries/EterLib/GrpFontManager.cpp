#include "StdAfx.h"
#include "GrpFontManager.h"
#include <stdexcept>
#include <knownfolders.h>
#include <shlobj_core.h>
#include <fmt/format.h>
#include <storm/io/View.hpp>
#include "../EterBase/Stl.h"

std::array<std::string, 12> FontTypes = {
	"Font/arial.ttf",
	"Font/Tahoma.ttf",
	"Font/Tahoma-Regular.ttf",
	"Font/NunitoSans-Bold.ttf",
	"Font/NunitoSans-Italic.ttf",
	"Font/NunitoSans-Regular.ttf",
	"Font/NunitoSans-SemiBold.ttf",
	"Font/Beaufort-Bold.ttf",
	"Font/Beaufort-Italic.ttf",
	"Font/Beaufort-Regular.ttf",
	"Font/Spiegel-Italic.ttf",
	"Font/Spiegel-Regular.ttf"
};

void FontManager::PreloadFonts()
{
	for (const std::string& fontName : FontTypes)
	{
		RegisterFont(fontName);
	}
}

inline std::string WToStr(const std::wstring& s)
{
	std::string temp(s.length(), ' ');
	std::copy(s.begin(), s.end(), temp.begin());
	return temp;
}

std::string getFontPath()
{
	wchar_t* pOut;
	SHGetKnownFolderPath(FOLDERID_Fonts, 0, 0, &pOut);
	std::string res = WToStr(pOut);
	CoTaskMemFree(pOut);
	return res;
}

struct FontData
{
	std::string name;
	std::string style;
	uint8_t size;
};

// To support all type of shit..
FontData GetFontDataByDefinition(const std::string& name)
{
	std::string nameCopy = name;
	std::string styleName = "Regular";
	uint8_t size = 12;

	std::string realName;
	const auto pos = nameCopy.find(':');
	if (pos != std::string::npos)
	{
		realName = nameCopy.substr(0, pos);

		if (nameCopy.back() == 'i')
		{
			styleName = "Italic";
			nameCopy.pop_back();
		}

		if (nameCopy.back() == 'b')
		{
			styleName = "Bold";
			nameCopy.pop_back();
		}

		if (nameCopy.back() == 's')
		{
			styleName = "SemiBold";
			nameCopy.pop_back();
		}

		if (nameCopy.back() == 'l')
		{
			styleName = "Light";
			nameCopy.pop_back();
		}

		if (nameCopy.back() == 'm')
		{
			styleName = "Medium";
			nameCopy.pop_back();
		}
		const std::string sizeStr(nameCopy.c_str() + pos + 1);
		size = atoi(sizeStr.c_str());
	}
	else
	{
		realName = name;
	}

	return { realName, styleName, size };
}

FontManager::FontManager()
{
	const FT_Error error = FT_Init_FreeType(&m_library);
	if (error)
	{
		throw new std::runtime_error("Could not initialize FreeType library!");
	}

	const char* WindowsOriginalFonts[] = { "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", "SOFTWARE\\Microsoft\\WindowsNT\\CurrentVersion\\Fonts", "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Fonts" };
	for (int32_t i = 0; i < 3; ++i)
	{
		HKEY fontsKey;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, WindowsOriginalFonts[i], 0, KEY_READ, &fontsKey) == ERROR_SUCCESS)
		{
			std::string fontPath = getFontPath();

			const int32_t BufSize = 32000;
			char value[BufSize];
			DWORD valueSize = BufSize;
			char data[BufSize]{};
			DWORD dataSize = BufSize;
			DWORD type;
			int32_t i = 0;
			while (RegEnumValueA(fontsKey, i, value, &valueSize, 0, &type, (LPBYTE)data, &dataSize) != ERROR_NO_MORE_ITEMS)
			{
				valueSize = BufSize;
				dataSize = BufSize;
				++i;

				std::string v = value;
				int32_t pos = v.find("(TrueType)");
				if (pos == std::string::npos || type != REG_SZ)
				{
					continue;
				}
				v = v.substr(0, pos - 1);
				m_fontFiles[v] = fontPath + "\\" + data;
			}
			break;
		}
	}
}

FontManager::~FontManager()
{
	// First remove the Face
	for (auto i = m_faces.begin(); i != m_faces.end(); ++i)
	{
		FT_Done_Face(i->second);
	}

	// Remove the fontmap AND AFTER the library!
	stl_wipe_second(m_fontMap);
	FT_Done_FreeType(m_library);
}

CGraphicFontType* FontManager::LoadFont(const std::string& fileName)
{
	const auto it = m_fontMap.find(fileName);
	if (it != m_fontMap.end())
		return it->second;

	FontData fontData = GetFontDataByDefinition(fileName);
	auto r = m_faces.equal_range(fontData.name);

	// If no such font is loaded...
	if (r.first == r.second)
	{
		auto it = m_fontFiles.find(fontData.name);
		if (it != m_fontFiles.end())
		{
			RegisterFont(it->second);
			r = m_faces.equal_range(fontData.name);
		}
	}

	FT_Face font_face = nullptr;

	for (auto& i = r.first; i != r.second; ++i)
	{
		if (i->second->style_name == fontData.style)
		{
			font_face = i->second;
			break;
		}
	}

	if (!font_face && r.first != r.second)
	{
		// Just default to the first of what we found
		font_face = r.first->second;
	}
	else if (!font_face)
	{
		return nullptr;
	}

	int32_t error = 0;
	error = FT_Set_Pixel_Sizes(font_face, 0, fontData.size);

	if (error)
	{
		WarnLog("Specified font size not supported by font");
		return nullptr;
	}

	CGraphicFontType* FontType = new CGraphicFontType();
	if (!FontType->Create(font_face, fontData.size))
		return nullptr;

	m_fontMap.emplace(fileName, FontType);
	return FontType;
}

// To be able to load up multiple fonts in one runtime
void FontManager::RegisterFont(const std::string& fileName)
{
	ConsoleLog("Registering Font {0}", fileName.c_str());
	const auto it = m_fontMap.find(fileName);
	if (it != m_fontMap.end())
		return;

	auto vfs = CallFS().Open(fileName, Buffered);

	if (!vfs)
	{
		SysLog("Couldn't open on: {0}", fileName)
		return;
	}

	FT_Face face;

	const uint32_t size = vfs->GetSize();

	auto data = std::make_unique<storm::View>(storm::GetDefaultAllocator());
	auto ptr = data->Initialize(size);

	if (!vfs->Read(0, ptr, size))
	{
		SysLog("Cannot read: {0}", fileName);
		return;
	}

	FT_Error error = FT_New_Memory_Face(m_library, (const FT_Byte*)data->GetData(), size, 0, &face);

	if (error)
	{
		SysLog("Could not load Face for Font {0}", fileName.c_str());
		return;
	}

	m_mappedFonts.emplace(fileName, std::move(data));

	error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
	if (error)
	{
		SysLog("Could not set encoding while processing Face for Font {0}", fileName.c_str());
		return;
	}

	std::string familyName = face->family_name;

	m_faces.emplace(familyName, face);

	int32_t numFaces = face->num_faces;
	for (int32_t i = 1; i < numFaces; ++i)
	{
		error = FT_New_Memory_Face(m_library, (const FT_Byte*)data->GetData(), size, i, &face);

		if (error)
		{
			continue;
		}

		std::string familyName = face->family_name;

		m_faces.emplace(familyName, face);
	}
}

void FontManager::SetDefaultFont(const std::string& normal, const std::string& italic, const std::string& bold)
{
	m_defaultFont = normal;
	m_defaultItalicFont = italic;
	m_defaultBoldFont = bold;
}

CGraphicFontType* FontManager::GetDefaultFont()
{
	return LoadFont(m_defaultFont);
}

CGraphicFontType* FontManager::GetDefaultItalicFont()
{
	return LoadFont(m_defaultItalicFont);
}

void FontManager::DestroyFont(CGraphicFontType* font)
{
	assert(!font->GetName().empty() && "Font name empty");
	m_fontMap.erase(font->GetName());
}
