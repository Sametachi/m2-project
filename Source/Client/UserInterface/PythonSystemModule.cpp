#include "StdAfx.h"
#include "PythonSystem.h"
/*
		Remove this shit
*/

// CHINA_CRYPT_KEY
uint32_t g_adwEncryptKey[4];
uint32_t g_adwDecryptKey[4];

#include "AccountConnector.h"
#include <GameLib/TerrainPatch.h>

inline const uint8_t* GetKey_20050304Myevan()
{
	volatile static uint32_t s_adwKey[1938];

	volatile uint32_t seed = 1491971513;
	for (uint32_t i = 0; i < uint8_t(seed); i++)
	{
		seed ^= 2148941891;
		seed += 3592385981;
		s_adwKey[i] = seed;
	}

	return (const uint8_t*)s_adwKey;
}

//#include <eterCrypt.h>

void CAccountConnector::__BuildClientKey_20050304Myevan()
{
	const uint8_t* c_pszKey = GetKey_20050304Myevan();
	memcpy(g_adwEncryptKey, c_pszKey + 157, 16);

	for (uint32_t i = 0; i < 4; ++i)
		g_adwEncryptKey[i] = random();

	tea_encrypt((uint32_t*)g_adwDecryptKey, (const uint32_t*)g_adwEncryptKey, (const uint32_t*)(c_pszKey + 37), 16);
}
// END_OF_CHINA_CRYPT_KEY

static uint32_t systemsettingGetWidth()
{
	return  ClientConfig::GetInstance()->GetWidth();
}

static uint32_t systemsettingGetHeight()
{
	return  ClientConfig::GetInstance()->GetHeight();
}

static void systemsettingSetInterfaceHandler(pybind11::handle poHandler)
{
	//ClientConfig::GetInstance()->SetInterfaceHandler(poHandler);
}

static void systemsettingDestroyInterfaceHandler()
{
	//ClientConfig::GetInstance()->DestroyInterfaceHandler();
}

static void systemsettingReserveResource()
{
	std::set<std::string> ResourceSet;
	//CResourceManager::GetInstance()->PushBackgroundLoadingSet(ResourceSet);
}

static int32_t systemsettingGetResolutionCount()
{
	return  ClientConfig::GetInstance()->GetResolutionCount();
}

static std::tuple<uint32_t, uint32_t, uint32_t> systemsettingGetCurrentResolution()
{
	ClientConfig::TConfig *tmp = ClientConfig::GetInstance()->GetConfig();
	return std::make_tuple( tmp->width, tmp->height, tmp->bpp);
}

static int32_t systemsettingGetResolution()
{
	return  ClientConfig::GetInstance()->GetResolutionCount();
}

static int32_t systemsettingGetFrequency()
{
	return  ClientConfig::GetInstance()->GetFrequency();
}

static bool systemsettingSaveConfig()
{
	return ClientConfig::GetInstance()->SaveConfig();
}

static std::tuple<int32_t, int32_t,bool,bool,float,uint8_t, int32_t, int32_t> systemsettingGetConfig()
{
	ClientConfig::TConfig *tmp = ClientConfig::GetInstance()->GetConfig();

	int32_t iRes = /*ClientConfig::GetInstance()->GetResolutionIndex()*/0;
	int32_t iFrequency = ClientConfig::GetInstance()->GetFrequency();

	return std::make_tuple(iRes,
		iFrequency,
		tmp->is_software_cursor,
		true,
		tmp->music_volume,
		tmp->soundEffect_volume,
		tmp->gamma,
		tmp->iDistance);
}

static float systemsettingGetMusicVolume()
{
	return  ClientConfig::GetInstance()->GetMusicVolume();
}

static int systemsettingGetSoundVolume()
{
	return  ClientConfig::GetInstance()->GetSoundVolume();
}

static void systemsettingSetMusicVolume(float fVolume)
{
	ClientConfig::GetInstance()->SetMusicVolume(fVolume);
}

static void systemsettingSetSoundVolumef(float fVolume)
{
	ClientConfig::GetInstance()->SetSoundVolumef(fVolume);
}

static bool systemsettingIsSoftwareCursor()
{
	return  ClientConfig::GetInstance()->IsSoftwareCursor();
}

static void systemsettingSetViewChatFlag(bool iFlag)
{
	ClientConfig::GetInstance()->SetViewChatFlag(iFlag);
}

static bool systemsettingIsViewChat()
{
	return  ClientConfig::GetInstance()->IsViewChat();
}

static void systemsettingSetAlwaysShowPlayerName(bool iFlag)
{
	ClientConfig::GetInstance()->SetAlwaysShowPlayerNameFlag(iFlag);
}

static bool systemsettingIsAlwaysShowPlayerName()
{
	return  ClientConfig::GetInstance()->IsAlwaysShowPlayerName();
}

static void systemsettingSetAlwaysShowNPCName(bool iFlag)
{
	ClientConfig::GetInstance()->SetAlwaysShowNPCNameFlag(iFlag);
}

static bool systemsettingIsAlwaysShowNPCName()
{
	return  ClientConfig::GetInstance()->IsAlwaysShowNPCName();
}

static void systemsettingSetAlwaysShowMonsterName(bool iFlag)
{
	ClientConfig::GetInstance()->SetAlwaysShowMonsterNameFlag(iFlag);
}

static bool systemsettingIsAlwaysShowMonsterName()
{
	return  ClientConfig::GetInstance()->IsAlwaysShowMonsterName();
}

static void systemsettingSetAlwaysShowItemName(bool iFlag)
{
	ClientConfig::GetInstance()->SetAlwaysShowItemNameFlag(iFlag);
}

static bool systemsettingIsAlwaysShowItemName()
{
	return  ClientConfig::GetInstance()->IsAlwaysShowItemName();
}

static void systemsettingSetShowDamageFlag(int32_t iFlag)
{
	ClientConfig::GetInstance()->SetShowDamageFlag(iFlag);
}

static bool systemsettingIsShowDamage()
{
	return  ClientConfig::GetInstance()->IsShowDamage();
}

static void systemsettingSetShowSalesTextFlag(int32_t iFlag)
{
	ClientConfig::GetInstance()->SetShowSalesTextFlag(iFlag);
}

static bool systemsettingIsShowSalesText()
{
	return  ClientConfig::GetInstance()->IsShowSalesText();
}

static void systemSetShadowTargetLevel(uint32_t level)
{
	ClientConfig::GetInstance()->SetShadowTargetLevel(level);
}

static int32_t systemGetShadowTargetLevel()
{
	return ClientConfig::GetInstance()->GetShadowTargetLevel();
}

static int32_t systemGetShadowQualityLevel()
{
	return ClientConfig::GetInstance()->GetShadowQualityLevel();
}

static void systemSetShadowQualityLevel(uint32_t quality)
{
	ClientConfig::GetInstance()->SetShadowQualityLevel(quality);
}

static void systemSetShadowMode(bool mode)
{
	ClientConfig::GetInstance()->SetShadowMode(mode);
	CTerrainPatch::IS_DYNAMIC_SHADOW = ClientConfig::GetInstance()->IsDynamicShadow();
}

static bool systemIsDynamicShadow()
{
	return ClientConfig::GetInstance()->IsDynamicShadow();
}

static void systemsettingSetMouseLeft(uint8_t iFlag)
{
	ClientConfig::GetInstance()->SetMouseLeft(iFlag);
}

static void systemsettingSetMouseRight(uint8_t iFlag)
{
	ClientConfig::GetInstance()->SetMouseRight(iFlag);
}

static int32_t systemsettingGetMouseLeft()
{
	return ClientConfig::GetInstance()->GetMouseLeft();
}

static int32_t systemsettingGetMouseRight()
{
	return ClientConfig::GetInstance()->GetMouseRight();
}

static void systemsettingSetMouseSize(uint8_t iFlag)
{
	ClientConfig::GetInstance()->SetMouseSize(iFlag);
}

static int32_t systemsettingGetMouseSize()
{
	return ClientConfig::GetInstance()->GetMouseSize();
}

PYBIND11_EMBEDDED_MODULE(systemSetting, m)
{
	m.def("GetWidth",	systemsettingGetWidth);
	m.def("GetHeight",	systemsettingGetHeight);
	m.def("SetInterfaceHandler",	systemsettingSetInterfaceHandler);
	m.def("DestroyInterfaceHandler",	systemsettingDestroyInterfaceHandler);
	m.def("ReserveResource",	systemsettingReserveResource);
	m.def("GetResolutionCount",	systemsettingGetResolutionCount);
	m.def("GetCurrentResolution",	systemsettingGetCurrentResolution);
	m.def("GetResolution",	systemsettingGetResolution);
	m.def("GetFrequency",	systemsettingGetFrequency);
	m.def("SaveConfig",	systemsettingSaveConfig);
	m.def("GetConfig",	systemsettingGetConfig);
	m.def("GetMusicVolume",	systemsettingGetMusicVolume);
	m.def("GetSoundVolume",	systemsettingGetSoundVolume);
	m.def("SetMusicVolume",	systemsettingSetMusicVolume);
	m.def("SetSoundVolumef",	systemsettingSetSoundVolumef);
	m.def("IsSoftwareCursor",	systemsettingIsSoftwareCursor);
	m.def("SetViewChatFlag",	systemsettingSetViewChatFlag);
	m.def("IsViewChat",	systemsettingIsViewChat);

	m.def("SetAlwaysShowPlayerName", systemsettingSetAlwaysShowPlayerName);
	m.def("IsAlwaysShowPlayerName",	systemsettingIsAlwaysShowPlayerName);

	m.def("SetAlwaysShowNPCName", systemsettingSetAlwaysShowNPCName);
	m.def("IsAlwaysShowNPCName", systemsettingIsAlwaysShowNPCName);

	m.def("SetAlwaysShowMonsterName", systemsettingSetAlwaysShowMonsterName);
	m.def("IsAlwaysShowMonsterName", systemsettingIsAlwaysShowMonsterName);

	m.def("SetAlwaysShowItemName", systemsettingSetAlwaysShowItemName);
	m.def("IsAlwaysShowItemName", systemsettingIsAlwaysShowItemName);

	m.def("SetShowDamageFlag",	systemsettingSetShowDamageFlag);
	m.def("IsShowDamage",	systemsettingIsShowDamage);
	m.def("SetShowSalesTextFlag",	systemsettingSetShowSalesTextFlag);
	m.def("IsShowSalesText",	systemsettingIsShowSalesText);

	m.def("GetShadowTargetLevel", systemGetShadowTargetLevel);
	m.def("SetShadowTargetLevel", systemSetShadowTargetLevel);
	m.def("GetShadowQualityLevel", systemGetShadowQualityLevel);
	m.def("SetShadowQualityLevel", systemSetShadowQualityLevel);
	m.def("SetShadowMode", systemSetShadowMode);
	m.def("IsDynamicShadow", systemIsDynamicShadow);

	m.def("SetMouseLeft", systemsettingSetMouseLeft);
	m.def("SetMouseRight", systemsettingSetMouseRight);
	m.def("GetMouseLeft", systemsettingGetMouseLeft);
	m.def("GetMouseRight", systemsettingGetMouseRight);
	m.def("GetMouseSize", systemsettingGetMouseSize);
	m.def("SetMouseSize", systemsettingSetMouseSize);
}
