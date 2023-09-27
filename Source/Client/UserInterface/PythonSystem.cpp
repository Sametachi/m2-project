#include "StdAfx.h"
#include "PythonSystem.h"
#include "PythonApplication.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <filesystem>

constexpr auto FILENAME_CONFIG = "Data/Config/Configuration.ini";
constexpr auto DEBUG_FILENAME_CONFIG = "Data/Config/Debug.ini";

// Display:
std::optional<std::tuple<uint32_t, uint32_t, uint32_t>> ClientConfig::GetResolution(int32_t index)
{
	uint32_t width;
	uint32_t height;
	uint32_t bpp;

	if (!m_videoModes)
		return std::nullopt;

	if (index >= m_videoModes->count())
		return std::nullopt;

	width = m_videoModes->item(index)->getWidth();
	height = m_videoModes->item(index)->getHeight();
	bpp = m_videoModes->item(index)->getColourDepth();

	return std::make_tuple(width, height, bpp);
}

bool ClientConfig::SetResolution(int32_t index)
{
	if (!m_videoModes)
		return false;

	if (index >= m_videoModes->count())
		return false;

	m_Config.width = m_videoModes->item(index)->getWidth();
	m_Config.height = m_videoModes->item(index)->getHeight();
	m_Config.bpp = m_videoModes->item(index)->getColourDepth();
	m_Config.frequency = m_videoModes->item(index)->getRefreshRate();

	SaveConfig();
	SetForceResolution(true);
	AdjustPlayArea();
	CPythonApplication::GetInstance()->OnSizeChange(m_Config.width, m_Config.height);
	CPythonApplication::GetInstance()->AdjustSize(m_Config.width, m_Config.height);
	m_Config.width_l = m_videoModes->item(index)->getWidth();
	m_Config.height_l = m_videoModes->item(index)->getHeight();
	CPythonApplication::GetInstance()->AdjustClientPosition();
	return true;
}

bool ClientConfig::IsResolutionByDescription(const std::string& desc)
{
	if (!m_videoModes)
		return false;

	if (m_videoModes->item(desc))
		return true;

	return false;
}

void ClientConfig::CreateVideoModeList()
{
	m_videoModes = std::make_unique<VideoModeList>();
	m_videoModes->enumerate(CPythonGraphic::GetInstance()->GetD3D());
}

void ClientConfig::SetShowFPS(bool iFlag)
{
	m_Config.bShowFPS = iFlag;
}

void ClientConfig::SetForceResolution(bool iFlag)
{
	m_Config.forceResolution = iFlag;
}

// Mouse:
void ClientConfig::SetMouseLeft(uint8_t LeftMouse)
{
	m_Config.left_mouse = LeftMouse;
}

void ClientConfig::SetMouseRight(uint8_t RightMouse)
{
	m_Config.right_mouse = RightMouse;
}

void ClientConfig::SetMouseSize(uint8_t m_size)
{
	m_Config.mouse_size = m_size;
	CPythonApplication::GetInstance()->DestroyCursors();
	CPythonApplication::GetInstance()->CreateCursors();
}

// Sound:
void ClientConfig::SetMusicVolume(float fVolume)
{
	m_Config.music_volume = fVolume;
}

void ClientConfig::SetSoundVolumef(float fVolume)
{
	m_Config.soundEffect_volume = fVolume;
}

// Chat:
void ClientConfig::SetViewChatFlag(bool iFlag)
{

}

void ClientConfig::SetAllowEmojis(bool iFlag)
{
	m_Config.bAllowEmojis = iFlag;
}

// Damage:
bool ClientConfig::IsShowDamage()
{
	return m_Config.bShowDamage;
}

void ClientConfig::SetShowDamageFlag(int32_t iFlag)
{
	m_Config.bShowDamage = iFlag == 1 ? true : false;
}

// Shop title
bool ClientConfig::IsShowSalesText()
{
	return m_Config.bShowSalesText;
}

void ClientConfig::SetShowSalesTextFlag(int32_t iFlag)
{
	m_Config.bShowSalesText = iFlag == 1 ? true : false;
}

// Fog:
void ClientConfig::SetFogMode(bool iFlag)
{
	m_Config.bFogMode = iFlag;
}

// TextTail:
void ClientConfig::SetAlwaysShowPlayerNameFlag(bool iFlag)
{
	m_Config.bAlwaysShowNamePlayer = iFlag;
}

void ClientConfig::SetAlwaysShowNPCNameFlag(bool iFlag)
{
	m_Config.bAlwaysShowNameNpc = iFlag;
}

void ClientConfig::SetAlwaysShowMonsterNameFlag(bool iFlag)
{
	m_Config.bAlwaysShowNameMonster = iFlag;
}

void ClientConfig::SetAlwaysShowItemNameFlag(bool iFlag)
{
	m_Config.bAlwaysShowNameItem = iFlag;
}

#if 0
void ClientConfig::SetAlwaysShowPrivateShopNameFlag(bool iFlag)
{
	m_Config.bAlwaysShowNamePrivateShop = iFlag;
}
#endif

// Shadows:
void ClientConfig::SetShadowTargetLevel(uint32_t level)
{
	m_Config.iShadowTargetLevel = MINMAX(CPythonBackground::SHADOW_NONE, level, CPythonBackground::SHADOW_ALL);
	CPythonBackground::GetInstance()->RefreshShadowTargetLevel();
}

void ClientConfig::SetShadowQualityLevel(uint32_t level)
{
	m_Config.iShadowQualityLevel = MINMAX(CPythonBackground::SHADOW_BAD, level, CPythonBackground::SHADOW_EPIC);
	CPythonBackground::GetInstance()->RefreshShadowQualityLevel();
}

void ClientConfig::SetShadowMode(bool iFlag)
{
	m_Config.bShadowMode = iFlag;
}

void ClientConfig::SetDarkMode(bool iFlag)
{
	m_Config.darkMode = iFlag;
}

// Not revisited!
int32_t ClientConfig::GetDistance()
{
	return m_Config.iDistance;
}

#if 0
// Monster Info
void ClientConfig::SetShowMobLevel(bool iFlag)
{
	m_Config.bShowMobLevel = iFlag;
}

void ClientConfig::SetShowMobAIFlag(bool iFlag)
{
	m_Config.bShowMobAIFlag = iFlag;
}
#endif

// Config Related:
ClientConfig::TConfig* ClientConfig::GetConfig()
{
	return &m_Config;
}

void ClientConfig::SetConfig(TConfig* pNewConfig)
{
	m_Config = *pNewConfig;
}

void ClientConfig::SetDefaultConfig()
{
	std::memset(&m_Config, 0, sizeof(m_Config));

	m_Config.m_szLang = "en";

	// Display:
	m_Config.width = 1024;
	m_Config.height = 768;
	m_Config.width_l = m_Config.width;
	m_Config.height_l = m_Config.height;
	m_Config.bpp = 32;
	m_Config.WindowMode = 1;
	m_Config.frequency = 60;
	m_Config.gamma = 1;
	m_Config.bShowFPS = true;
	m_Config.forceResolution = false;
	m_Config.darkMode = false;

	// Sound:
	m_Config.music_volume = 0.2f;
	m_Config.soundEffect_volume = 0.2f;

	// Chat:
	m_Config.bViewChat = true;
	m_Config.bAllowEmojis = true;

	// Damage:
	m_Config.bShowDamage = true;

	// Shop title
	m_Config.bShowSalesText = true;

	// Fog:
	m_Config.bFogMode = false;

	// Mouse:
	m_Config.is_software_cursor = false;
	m_Config.left_mouse = 2;
	m_Config.right_mouse = 3;
	m_Config.mouse_size = 32;

	// Shadows:
	m_Config.iShadowTargetLevel = CPythonBackground::SHADOW_ALL;
	m_Config.iShadowQualityLevel = CPythonBackground::SHADOW_GOOD;
	m_Config.bShadowMode = true;

	// TextTail:
	m_Config.bAlwaysShowNamePlayer = true;
	m_Config.bAlwaysShowNameNpc = true;
	m_Config.bAlwaysShowNameMonster = true;
	m_Config.bAlwaysShowNameItem = true;
#if 0
	m_Config.bAlwaysShowNamePrivateShop = true;

	// Monster Info
	m_Config.bShowMobLevel = true;
	m_Config.bShowMobAIFlag = true;
#endif

	// Language Icons
	m_Config.bShowPlayerLanguage = true;
	m_Config.bShowChatLanguage = true;
	m_Config.bShowWhisperLanguage = true;

	// Not revisited!
	m_Config.iDistance = 3;
	m_Config.bNoAudio = false;
}

bool ClientConfig::LoadConfig()
{
	boost::property_tree::ptree r_pt;
	try
	{
		boost::property_tree::ini_parser::read_ini(FILENAME_CONFIG, r_pt);

		m_Config.m_szLang = r_pt.get<std::string>("GAME_CONFIG.LANGUAGE");

		// Display:
		m_Config.width = r_pt.get<uint32_t>("DISPLAY.SCREEN_WIDTH");
		m_Config.height = r_pt.get<uint32_t>("DISPLAY.SCREEN_HEIGHT");
		m_Config.width_l = r_pt.get<uint32_t>("DISPLAY.RESOLUTION_X");
		m_Config.height_l = r_pt.get<uint32_t>("DISPLAY.RESOLUTION_Y");
		m_Config.frequency = r_pt.get<uint32_t>("DISPLAY.FREQUENCY");
		m_Config.gamma = r_pt.get<int32_t>("DISPLAY.GAMMA");
		m_Config.WindowMode = r_pt.get<uint8_t>("DISPLAY.WINDOW_MODE");
		m_Config.bShowFPS = r_pt.get<bool>("DISPLAY.SHOW_FPS");
		m_Config.darkMode = r_pt.get<bool>("DISPLAY.DARK_MODE");

		// Sound:
		m_Config.music_volume = r_pt.get<float>("SOUND.BACKGROUND_MUSIC_VOLUME");
		m_Config.soundEffect_volume = r_pt.get<float>("SOUND.SOUND_EFFECT_VOLUME");
		m_Config.bNoAudio = r_pt.get<bool>("SOUND.DISABLE", false);

		// Mouse:
		m_Config.is_software_cursor = r_pt.get<bool>("MOUSE.SOFTWARE_CURSOR");
		m_Config.left_mouse = r_pt.get<uint8_t>("MOUSE.LEFT_QUICK_CAST");
		m_Config.right_mouse = r_pt.get<uint8_t>("MOUSE.RIGHT_QUICK_CAST");
		m_Config.mouse_size = r_pt.get<uint8_t>("MOUSE.MOUSE_SIZE");

		// Game Config:
		m_Config.bViewChat = r_pt.get<bool>("GAME_CONFIG.VIEW_CHAT");
		m_Config.bAllowEmojis = r_pt.get<bool>("GAME_CONFIG.ALLOW_EMOJIS");
		m_Config.bShowDamage = r_pt.get<bool>("GAME_CONFIG.SHOW_DAMAGE");
		m_Config.bShowSalesText = r_pt.get<bool>("GAME_CONFIG.SHOW_SALESTEXT");
		m_Config.bFogMode = r_pt.get<bool>("GAME_CONFIG.FOG_ENABLED");

		m_Config.bAlwaysShowNamePlayer = r_pt.get<bool>("GAME_CONFIG.SHOW_NAME_PLAYER");
		m_Config.bAlwaysShowNameNpc = r_pt.get<bool>("GAME_CONFIG.SHOW_NAME_NPC");
		m_Config.bAlwaysShowNameMonster = r_pt.get<bool>("GAME_CONFIG.SHOW_NAME_MOB");
		m_Config.bAlwaysShowNameItem = r_pt.get<bool>("GAME_CONFIG.SHOW_NAME_ITEM");
#if 0
		m_Config.bAlwaysShowNamePrivateShop = r_pt.get<bool>("GAME_CONFIG.SHOW_NAME_PRIVATE_SHOP");

		m_Config.bShowMobLevel = r_pt.get<bool>("GAME_CONFIG.MONSTER_LEVEL");
		m_Config.bShowMobAIFlag = r_pt.get<bool>("GAME_CONFIG.MONSTER_AIFLAG");
#endif

		// Language Icon
		m_Config.bShowPlayerLanguage = r_pt.get<bool>("GAME_CONFIG.SHOW_PLAYER_LANGUAGE");
		m_Config.bShowChatLanguage = r_pt.get<bool>("GAME_CONFIG.SHOW_CHAT_LANGUAGE");
		m_Config.bShowWhisperLanguage = r_pt.get<bool>("GAME_CONFIG.SHOW_WHISPER_LANGUAGE");

		// Shadow:
		m_Config.iShadowTargetLevel = r_pt.get<int32_t>("SHADOW.LEVEL");
		m_Config.iShadowQualityLevel = r_pt.get<int32_t>("SHADOW.QUALITY");
		m_Config.bShadowMode = r_pt.get<bool>("SHADOW.DYNAMIC");

		// Not revisited!
		m_Config.iDistance = r_pt.get<int32_t>("DISPLAY.FIELD_OF_VIEW");
		//m_Config.iCameraDistanceMode = r_pt.get<uint32_t>("DISPLAY.CAMERA_DISTANCE");

		AdjustPlayArea();
	}

	catch (boost::property_tree::ini_parser_error& e)
	{
		SysLog("Game Config Error: {0}", e.message())
	}
	catch (const boost::property_tree::ptree_error& e)
	{
		SysLog("Game Config Error: {0}", e.what())
	}

#ifdef _DEBUG
	try
	{
		boost::property_tree::ini_parser::read_ini(DEBUG_FILENAME_CONFIG, r_pt);

		m_Dbg.bInterpreter = r_pt.get<bool>("PYTHON.INTERPRETER", false);
		m_Dbg.bRunMain = r_pt.get<bool>("PYTHON.RUNMAIN", true);
		m_Dbg.szSuperPath = r_pt.get<std::string>("PYTHON.SUPERPATH", "PythonSystem/");
	}
	catch (boost::property_tree::ini_parser_error& e)
	{
		WarnLog("Debug Config Error: {0}", e.message())
	}
	catch (const boost::property_tree::ptree_error& e)
	{
		WarnLog("Debug Config Error: {0}", e.what())
	}
#endif

	return true;
}

void ClientConfig::AdjustPlayArea()
{
	if (m_Config.WindowMode == WINDOWED || m_Config.WindowMode == BOARDLESS)
	{
		uint32_t screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
		uint32_t screen_height = GetSystemMetrics(SM_CYFULLSCREEN);
		uint32_t difference = (m_Config.height - screen_height) + 7;

		if (m_Config.width >= screen_width)
		{
			m_Config.width = screen_width;
		}
		if (m_Config.height >= screen_height)
		{
			m_Config.height = m_Config.height - difference;
		}
	}
	else if (m_Config.WindowMode == BOARDLESS_FSCREEN || m_Config.WindowMode == FULLSCREEN)
	{
		uint32_t screen_width = GetSystemMetrics(SM_CXSCREEN);
		uint32_t screen_height = GetSystemMetrics(SM_CYSCREEN);

		m_Config.width = screen_width;
		m_Config.height = screen_height;
	}
}

bool ClientConfig::SaveConfig()
{
	auto p = std::filesystem::path(FILENAME_CONFIG);
	auto x = p.parent_path();
	std::filesystem::create_directory(x);

	boost::property_tree::ptree pt;

	pt.put("GAME_CONFIG.LANGUAGE", boost::lexical_cast<std::string>(m_Config.m_szLang));

	// Display:
	pt.put("DISPLAY.SCREEN_WIDTH", boost::lexical_cast<uint32_t>(m_Config.width));
	pt.put("DISPLAY.SCREEN_HEIGHT", boost::lexical_cast<uint32_t>(m_Config.height));
	pt.put("DISPLAY.RESOLUTION_X", boost::lexical_cast<uint32_t>(m_Config.width_l));
	pt.put("DISPLAY.RESOLUTION_Y", boost::lexical_cast<uint32_t>(m_Config.height_l));
	pt.put("DISPLAY.FREQUENCY", boost::lexical_cast<uint32_t>(m_Config.frequency));
	pt.put("DISPLAY.GAMMA", boost::lexical_cast<int32_t>(m_Config.gamma));
	pt.put("DISPLAY.WINDOW_MODE", boost::lexical_cast<uint8_t>(uint8_t(m_Config.WindowMode)));
	pt.put("DISPLAY.SHOW_FPS", boost::lexical_cast<bool>(m_Config.bShowFPS));
	pt.put("DISPLAY.DARK_MODE", boost::lexical_cast<bool>(m_Config.darkMode));

	// Sound:
	pt.put("SOUND.BACKGROUND_MUSIC_VOLUME", boost::lexical_cast<float>(m_Config.music_volume));
	pt.put("SOUND.SOUND_EFFECT_VOLUME", boost::lexical_cast<float>(m_Config.soundEffect_volume));
	pt.put("SOUND.DISABLE", boost::lexical_cast<bool>(m_Config.bNoAudio));

	// Mouse:
	pt.put("MOUSE.SOFTWARE_CURSOR", boost::lexical_cast<bool>(m_Config.is_software_cursor));
	pt.put("MOUSE.LEFT_QUICK_CAST", boost::lexical_cast<uint8_t>(m_Config.left_mouse));
	pt.put("MOUSE.RIGHT_QUICK_CAST", boost::lexical_cast<uint8_t>(m_Config.right_mouse));
	pt.put("MOUSE.MOUSE_SIZE", boost::lexical_cast<uint8_t>(m_Config.mouse_size));

	// Game Config:
	pt.put("GAME_CONFIG.VIEW_CHAT", boost::lexical_cast<bool>(m_Config.bViewChat));
	pt.put("GAME_CONFIG.ALLOW_EMOJIS", boost::lexical_cast<bool>(m_Config.bAllowEmojis));
	pt.put("GAME_CONFIG.SHOW_DAMAGE", boost::lexical_cast<bool>(m_Config.bShowDamage));
	pt.put("GAME_CONFIG.SHOW_SALESTEXT", boost::lexical_cast<bool>(m_Config.bShowSalesText));
	pt.put("GAME_CONFIG.FOG_ENABLED", boost::lexical_cast<bool>(m_Config.bFogMode));

	pt.put("GAME_CONFIG.SHOW_NAME_PLAYER", boost::lexical_cast<bool>(m_Config.bAlwaysShowNamePlayer));
	pt.put("GAME_CONFIG.SHOW_NAME_NPC", boost::lexical_cast<bool>(m_Config.bAlwaysShowNameNpc));
	pt.put("GAME_CONFIG.SHOW_NAME_MOB", boost::lexical_cast<bool>(m_Config.bAlwaysShowNameMonster));
	pt.put("GAME_CONFIG.SHOW_NAME_ITEM", boost::lexical_cast<bool>(m_Config.bAlwaysShowNameItem));
#if 0
	pt.put("GAME_CONFIG.SHOW_NAME_PRIVATE_SHOP", boost::lexical_cast<bool>(m_Config.bAlwaysShowNamePrivateShop));

	pt.put("GAME_CONFIG.MONSTER_LEVEL", boost::lexical_cast<bool>(m_Config.bShowMobLevel));
	pt.put("GAME_CONFIG.MONSTER_AIFLAG", boost::lexical_cast<bool>(m_Config.bShowMobAIFlag));
#endif

	pt.put("GAME_CONFIG.SHOW_PLAYER_LANGUAGE", boost::lexical_cast<bool>(m_Config.bShowPlayerLanguage));
	pt.put("GAME_CONFIG.SHOW_CHAT_LANGUAGE", boost::lexical_cast<bool>(m_Config.bShowChatLanguage));
	pt.put("GAME_CONFIG.SHOW_WHISPER_LANGUAGE", boost::lexical_cast<bool>(m_Config.bShowWhisperLanguage));

	// Shadows:
	pt.put("SHADOW.LEVEL", boost::lexical_cast<int32_t>(m_Config.iShadowTargetLevel));
	pt.put("SHADOW.QUALITY", boost::lexical_cast<int32_t>(m_Config.iShadowQualityLevel));
	pt.put("SHADOW.DYNAMIC", boost::lexical_cast<bool>(m_Config.bShadowMode));

	// Not revisited!
	pt.put("DISPLAY.FIELD_OF_VIEW", m_Config.iDistance);
	//pt.put("DISPLAY.CAMERA_DISTANCE",			m_Config.iCameraDistanceMode);
	//Language
	//pt.put("LOCALIZATION.LANGUAGE", m_Config.Language);
	//GameConfig


//Interface
	//pt.put("INTERFACE.ATLAS_SIZE", m_Config.atlas_size);

	try
	{
		boost::property_tree::ini_parser::write_ini(FILENAME_CONFIG, pt);

	}
	catch (boost::property_tree::ini_parser_error& e)
	{
		SysLog("Game Config Error: {0}", e.message())
	}
	catch (const boost::property_tree::ptree_error& e)
	{
		SysLog("Game Config Error: {0}", e.what())
	}

	return true;
}

ClientConfig::ClientConfig()
{
	std::memset(&m_Config, 0, sizeof(TConfig));
	SetDefaultConfig();
}