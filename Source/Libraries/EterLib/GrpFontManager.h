#pragma once
#include "GrpText.h"
#include "../EterBase/Utils.h"
#include <boost/unordered_map.hpp>
#include <unordered_map>
#include <VFE/Include/VFE.hpp>

class FontManager
{
public:
    FontManager();
    ~FontManager();

    CGraphicFontType* LoadFont(const std::string& fileName);
    void RegisterFont(const std::string& fileName);
    void SetDefaultFont(const std::string& normal, const std::string& italic, const std::string& bold);
    void PreloadFonts();

    CGraphicFontType* GetDefaultFont();
    CGraphicFontType* GetDefaultItalicFont();

private:
    using FontMap= std::map<std::string, CGraphicFontType*, ci_less>;

    void DestroyFont(CGraphicFontType* font);

    FontMap m_fontMap;
    FontMap m_fontFamilyMap;
    std::map<std::string, std::string> m_fontFiles;
    std::multimap<std::string, FT_Face> m_faces;
    std::map<std::string, std::unique_ptr<storm::View>> m_mappedFonts;

    std::string m_defaultFont;
    std::string m_defaultItalicFont;
    std::string m_defaultBoldFont;

    FT_Library m_library;
};

