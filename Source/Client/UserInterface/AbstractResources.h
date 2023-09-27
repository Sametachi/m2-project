#pragma once
#include <Basic/Singleton.h>
#include "PythonApplication.h"
#include "InstanceBase.h"

static const char* NPC_LIST_FILENAME = "Globals/NonPlayerList.txt";
static const char* HUGE_RACE_LIST_FILENAME = "Globals/HugeRaceList.txt";
static const char* RACE_HEIGHT_FILENAME = "Globals/RaceHeightList.txt";
static const char* GUILD_BUILDING_LIST_FILENAME = "Globals/GuildBuildingList.txt";
constexpr auto FILE_MAX_NUM = 700;

enum SkillMotionIndex
{
	HORSE_SKILL_WILDATTACK = MOTION_SKILL + 121,
	HORSE_SKILL_CHARGE = MOTION_SKILL + 122,
	HORSE_SKILL_SPLASH = MOTION_SKILL + 123,

	GUILD_SKILL_DRAGONBLOOD = MOTION_SKILL + 101,
	GUILD_SKILL_DRAGONBLESS = MOTION_SKILL + 102,
	GUILD_SKILL_BLESSARMOR = MOTION_SKILL + 103,
	GUILD_SKILL_SPPEDUP = MOTION_SKILL + 104,
	GUILD_SKILL_DRAGONWRATH = MOTION_SKILL + 105,
	GUILD_SKILL_MAGICUP = MOTION_SKILL + 106,
};

enum EComboTypes
{
	COMBO_TYPE_1 = 0,
	COMBO_TYPE_2 = 1,
	COMBO_TYPE_3 = 2,
};

enum EComboIndexes
{
	COMBO_INDEX_1 = 0,
	COMBO_INDEX_2 = 1,
	COMBO_INDEX_3 = 2,
	COMBO_INDEX_4 = 3,
	COMBO_INDEX_5 = 4,
	COMBO_INDEX_6 = 5,
};

class AbstractResources : public Singleton<AbstractResources>
{
public:
	AbstractResources() = default;
	virtual ~AbstractResources() = default;

	bool LoadDefaultGameData();
	bool LoadRaceData();

	void SetLoadingState(bool loadingState)
	{
		bLoadingState = loadingState;
	};

	bool GetLoadingState() const
	{
		return bLoadingState;
	};

	bool RegisterGuildBuildingList();
	const static bool RegisterSkills(uint32_t idRace, uint8_t skill_group, uint8_t empire);

	static void LoadRaceMotions(uint32_t race);
	static void RegisterRace(uint8_t race, const std::string& msmPath);
	static void SetIntroMotions(uint8_t mode, const std::string& folder);
	static void SetGeneralMotions(uint8_t mode, const std::string& folder);
	static void RegisterSharedEmotionAnis(uint32_t mode, const std::string path);
	static void RegisterEmotionAnis(std::string path);

	static void LoadWarrior(uint8_t race, const std::string& path);
	static void LoadAssassin(uint8_t race, const std::string& path);
	static void LoadSura(uint8_t race, const std::string& path);
	static void LoadShaman(uint8_t race, const std::string& path);

protected:
	const static bool RegisterConstansData();
	const static bool RegisterTitleNamesAndColors();
	const static bool RegisterEmojis();

	const static bool RegisterAffects();
	const static bool RegisterEffects();
	const static bool RegisterRefinedEffects();
	const static bool RegisterFlyEffects();

	const static bool LoadActorMotions();
	
	const static bool LoadRaceHeights();
	const static bool LoadNonPlayerData();
	const static bool LoadHugeRaceData();

	bool bLoadingState;
	//std::vector<py::dict> guildbuildingMap;
	//py::dict listDict;

private:
	enum EGuildBuildingListToken
	{
		GUIILD_BUILDING_TOKEN_VNUM,
		GUIILD_BUILDING_TOKEN_TYPE,
		GUIILD_BUILDING_TOKEN_NAME,
		GUIILD_BUILDING_TOKEN_LOCAL_NAME,
		GUIILD_BUILDING_NO_USE_TOKEN_SIZE_1,
		GUIILD_BUILDING_NO_USE_TOKEN_SIZE_2,
		GUIILD_BUILDING_NO_USE_TOKEN_SIZE_3,
		GUIILD_BUILDING_NO_USE_TOKEN_SIZE_4,
		GUIILD_BUILDING_TOKEN_X_ROT_LIMIT,
		GUIILD_BUILDING_TOKEN_Y_ROT_LIMIT,
		GUIILD_BUILDING_TOKEN_Z_ROT_LIMIT,
		GUIILD_BUILDING_TOKEN_PRICE,
		GUIILD_BUILDING_TOKEN_MATERIAL,
		GUIILD_BUILDING_TOKEN_NPC,
		GUIILD_BUILDING_TOKEN_GROUP,
		GUIILD_BUILDING_TOKEN_DEPEND_GROUP,
		GUIILD_BUILDING_TOKEN_ENABLE_FLAG,
		GUIILD_BUILDING_LIMIT_TOKEN_COUNT,
	} EGuildBuildingListToken;

	robin_hood::unordered_map<std::string_view, std::string> m_buildingTypeToFolder
	{
		{"HEADQUARTER",		"headquarter"},
		{"FACILITY",		"facility"},
		{"OBJECT",			"object"},
		{"WALL",			"fence"},
	};
};

const static robin_hood::unordered_map<uint32_t, std::string> aniMap =
{
	{MOTION_CLAP, "clap.msa"},
	{MOTION_CHEERS_1, "cheers_1.msa"},
	{MOTION_CHEERS_2, "cheers_2.msa"},
	{MOTION_DANCE_1, "dance_1.msa"},
	{MOTION_DANCE_2, "dance_2.msa"},
	{MOTION_DANCE_3, "dance_3.msa"},
	{MOTION_DANCE_4, "dance_4.msa"},
	{MOTION_DANCE_5, "dance_5.msa"},
	{MOTION_DANCE_6, "dance_6.msa"},
	{MOTION_CONGRATULATION, "congratulation.msa"},
	{MOTION_FORGIVE, "forgive.msa"},
	{MOTION_ANGRY, "angry.msa"},
	{MOTION_ATTRACTIVE, "attractive.msa"},
	{MOTION_SAD, "sad.msa"},
	{MOTION_SHY, "shy.msa"},
	{MOTION_CHEERUP, "cheerup.msa"},
	{MOTION_BANTER, "banter.msa"},
	{MOTION_JOY, "joy.msa"},
	{MOTION_FRENCH_KISS_WITH_WARRIOR, "french_kiss_with_warrior.msa"},
	{MOTION_FRENCH_KISS_WITH_ASSASSIN, "french_kiss_with_assassin.msa"},
	{MOTION_FRENCH_KISS_WITH_SURA, "french_kiss_with_sura.msa"},
	{MOTION_FRENCH_KISS_WITH_SHAMAN, "french_kiss_with_shaman.msa"},
	{MOTION_KISS_WITH_WARRIOR, "kiss_with_warrior.msa"},
	{MOTION_KISS_WITH_ASSASSIN, "kiss_with_assassin.msa"},
	{MOTION_KISS_WITH_SURA, "kiss_with_sura.msa"},
	{MOTION_KISS_WITH_SHAMAN, "kiss_with_shaman.msa"},
	{MOTION_SLAP_HIT_WITH_WARRIOR, "slap_hit.msa"},
	{MOTION_SLAP_HIT_WITH_ASSASSIN, "slap_hit.msa"},
	{MOTION_SLAP_HIT_WITH_SURA, "slap_hit.msa"},
	{MOTION_SLAP_HIT_WITH_SHAMAN, "slap_hit.msa"},
	{MOTION_SLAP_HURT_WITH_WARRIOR, "slap_hurt.msa"},
	{MOTION_SLAP_HURT_WITH_ASSASSIN, "slap_hurt.msa"},
	{MOTION_SLAP_HURT_WITH_SURA, "slap_hurt.msa"},
	{MOTION_SLAP_HURT_WITH_SHAMAN, "slap_hurt.msa"},
};

/* Register Guild Building Data */
constexpr uint8_t GUILD_MATERIAL_NUM = 3;
enum EGuildMaterialItems
{
	MATERIAL_STONE_ID = 90010,
	MATERIAL_LOG_ID = 90011,
	MATERIAL_PLYWOOD_ID = 90012,
};
static const uint8_t getGuildMaterialIndex(const uint32_t& vnum)
{
	switch (vnum)
	{
		case MATERIAL_STONE_ID:
		case MATERIAL_LOG_ID:
		case MATERIAL_PLYWOOD_ID:
			return vnum - 90010;
	}

	return -1;
}

/* Dungeon Names */
static const std::array<const char*, 5> dungeonArray =
{
	"metin2_map_spiderdungeon",
	"metin2_map_monkeydungeon",
	"metin2_map_monkeydungeon_02",
	"metin2_map_monkeydungeon_03",
	"metin2_map_deviltower1"
};

/* Load Sounds */
typedef robin_hood::unordered_map<uint8_t, std::string> MSoundMap;
static const MSoundMap useSoundFiles
{
	{ CPythonItem::USESOUND_DEFAULT,		"sound/ui/drop.wav"},
	{ CPythonItem::USESOUND_ACCESSORY,		"sound/ui/equip_ring_amulet.wav"},
	{ CPythonItem::USESOUND_ARMOR,			"sound/ui/equip_metal_armor.wav"},
	{ CPythonItem::USESOUND_BOW,			"sound/ui/equip_bow.wav"},
	{ CPythonItem::USESOUND_WEAPON,			"sound/ui/equip_metal_weapon.wav"},
	{ CPythonItem::USESOUND_POTION,			"sound/ui/eat_potion.wav"},
	{ CPythonItem::USESOUND_PORTAL,			"sound/ui/potal_scroll.wav"},
};

static const MSoundMap dropSoundFiles
{
	{ CPythonItem::DROPSOUND_DEFAULT,		"sound/ui/drop.wav"},
	{ CPythonItem::DROPSOUND_ACCESSORY,		"sound/ui/equip_ring_amulet.wav"},
	{ CPythonItem::DROPSOUND_ARMOR,			"sound/ui/equip_metal_armor.wav"},
	{ CPythonItem::DROPSOUND_BOW,			"sound/ui/equip_bow.wav"},
	{ CPythonItem::DROPSOUND_WEAPON,		"sound/ui/equip_metal_weapon.wav"},
};

typedef std::array<uint8_t, 3> RGB_COLORS;
typedef robin_hood::unordered_map<uint8_t, RGB_COLORS> MColorMaps;

/* Title Color Map */
static const MColorMaps titleColorMap
{
	{ /*PVP_LEVEL*/0,		{ 0,		204,	255 }},
	{ /*PVP_LEVEL*/1,		{ 0,		144,	255 }},
	{ /*PVP_LEVEL*/2,		{ 92,		110,	255 }},
	{ /*PVP_LEVEL*/3,		{ 155,		155,	255 }},
	{ /*PVP_LEVEL*/4,		{ 255,		255,	255 }},
	{ /*PVP_LEVEL*/5,		{ 207,		117,	0 }},
	{ /*PVP_LEVEL*/6,		{ 235,		83,		0 }},
	{ /*PVP_LEVEL*/7,		{ 227,		0,		0 }},
	{ /*PVP_LEVEL*/8,		{ 255,		0,		0 }},
};

/* Name Color Map */
static const MColorMaps nameColorMap
{
			/* NameColor */								 /* RBG */
	{ CInstanceBase::NAMECOLOR_NORMAL_PC,		{ 255,		215,		76 }},
	{ CInstanceBase::NAMECOLOR_NORMAL_NPC,		{ 122,		231,		93 }},
	{ CInstanceBase::NAMECOLOR_NORMAL_MOB,		{ 235,		22,			9 }},
	{ CInstanceBase::NAMECOLOR_PVP,				{ 238,		54,			223 }},
	{ CInstanceBase::NAMECOLOR_PK,				{ 180,		100,		0 }},
	{ CInstanceBase::NAMECOLOR_PARTY,			{ 128,		192,		255 }},
	{ CInstanceBase::NAMECOLOR_WARP,			{ 136,		218,		241 }},
	{ CInstanceBase::NAMECOLOR_WAYPOINT,		{ 255,		255,		255 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_MOB,		{ 235,		22,			9 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_NPC,		{ 122,		231,		93 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 1,	{ 157,		0,			0 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 2,	{ 222,		160,		47 }},
	{ CInstanceBase::NAMECOLOR_EMPIRE_PC + 3,	{ 23,		30,			138 }},
};

/* Skill related maps/arrays */
static const robin_hood::unordered_map<uint8_t, std::map<uint8_t, std::vector<uint8_t>>> generalSkillIndexes
{
	{ NRaceData::JOB_WARRIOR,
	{
		{1, {1, 2, 3, 4, 5, 0, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {16, 17, 18, 19, 20, 0, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
	{ NRaceData::JOB_ASSASSIN,
	{
		{1, {31, 32, 33, 34, 35, 0, 0, 0, 137, 0, 138, 0, 139, 0, 140}},
		{2, {46, 47, 48, 49, 50, 0, 0, 0, 137, 0, 138, 0, 139, 0, 140}},
	}},
	{ NRaceData::JOB_SURA,
	{
		{1, {61, 62, 63, 64, 65, 66, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {76, 77, 78, 79, 80, 81, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
	{ NRaceData::JOB_SHAMAN,
	{
		{1, {91, 92, 93, 94, 95, 96, 0, 0, 137, 0, 138, 0, 139, 0}},
		{2, {106, 107, 108, 109, 110, 111, 0, 0, 137, 0, 138, 0, 139, 0}},
	}},
};


constexpr std::array<uint8_t, 12> supportSkillIndexes =
{
	//{122, 123, 121, 129, 130, 131, 143, 0, 0, 0, 0, 0},
	{122, 123, 121, 124, 125, 129, 131, 130, 0, 0, 0, 0},
};


static const robin_hood::unordered_map < std::string, std::vector<uint8_t>> guildSkillsIndexes
{
	{ "PASSIVE", { 151 }},
	{ "ACTIVE", { 152, 153, 154, 155, 156, 157 }},
};