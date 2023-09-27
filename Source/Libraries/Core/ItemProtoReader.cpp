#include "ProtoReader.hpp"
#include "Constants/Item.hpp"
#include <Basic/Logging.hpp>

uint8_t ItemTypeStringToInteger(const std::string& c_stItemType)
{
	const static EnumDictionary TypeDict = {
		{ "ITEM_NONE", ITEM::TYPE_NONE },
		{ "ITEM_WEAPON", ITEM::TYPE_WEAPON },
		{ "ITEM_ARMOR", ITEM::TYPE_ARMOR },
		{ "ITEM_USE", ITEM::TYPE_USE },
		{ "ITEM_AUTOUSE", ITEM::TYPE_AUTOUSE },
		{ "ITEM_MATERIAL", ITEM::TYPE_MATERIAL },
		{ "ITEM_SPECIAL", ITEM::TYPE_SPECIAL },
		{ "ITEM_TOOL", ITEM::TYPE_TOOL },
		{ "ITEM_LOTTERY", ITEM::TYPE_LOTTERY },
		{ "ITEM_ELK", ITEM::TYPE_ELK },
		{ "ITEM_METIN", ITEM::TYPE_METIN },
		{ "ITEM_CONTAINER", ITEM::TYPE_CONTAINER },
		{ "ITEM_FISH", ITEM::TYPE_FISH },
		{ "ITEM_ROD", ITEM::TYPE_ROD },
		{ "ITEM_RESOURCE", ITEM::TYPE_RESOURCE },
		{ "ITEM_CAMPFIRE", ITEM::TYPE_CAMPFIRE },
		{ "ITEM_UNIQUE", ITEM::TYPE_UNIQUE },
		{ "ITEM_SKILLBOOK", ITEM::TYPE_SKILLBOOK },
		{ "ITEM_QUEST", ITEM::TYPE_QUEST },
		{ "ITEM_POLYMORPH", ITEM::TYPE_POLYMORPH },
		{ "ITEM_TREASURE_BOX", ITEM::TYPE_TREASURE_BOX },
		{ "ITEM_TREASURE_KEY", ITEM::TYPE_TREASURE_KEY },
		{ "ITEM_SKILLFORGET", ITEM::TYPE_SKILLFORGET },
		{ "ITEM_GIFTBOX", ITEM::TYPE_GIFTBOX },
		{ "ITEM_PICK", ITEM::TYPE_PICK },
		{ "ITEM_HAIR", ITEM::TYPE_HAIR },
		{ "ITEM_TOTEM", ITEM::TYPE_TOTEM },
		{ "ITEM_BLEND", ITEM::TYPE_BLEND },
		{ "ITEM_COSTUME", ITEM::TYPE_COSTUME },
		{ "ITEM_SECONDARY_COIN", ITEM::TYPE_SECONDARY_COIN },
	};

	auto it = TypeDict.find(c_stItemType);
	if (it != TypeDict.end())
		return it->second;

	return ITEM::TYPE_NONE;
}

uint8_t ItemSubTypeStringToInteger(uint8_t bType, const std::string& c_stItemSubType)
{
	const static NestedEnumDictionary SubTypeDict = {
		{
			ITEM::TYPE_WEAPON,
			{
				{ "WEAPON_SWORD",		ITEM::WEAPON_SWORD },
				{ "WEAPON_DAGGER",		ITEM::WEAPON_DAGGER },
				{ "WEAPON_BOW",			ITEM::WEAPON_BOW },
				{ "WEAPON_TWO_HANDED",	ITEM::WEAPON_TWO_HANDED },
				{ "WEAPON_BELL",		ITEM::WEAPON_BELL },
				{ "WEAPON_FAN",			ITEM::WEAPON_FAN },
				{ "WEAPON_ARROW",		ITEM::WEAPON_ARROW },
			}
		},

		{
			ITEM::TYPE_ARMOR,
			{
				{ "ARMOR_BODY",		ITEM::ARMOR_BODY },
				{ "ARMOR_HEAD",		ITEM::ARMOR_HEAD },
				{ "ARMOR_SHIELD",	ITEM::ARMOR_SHIELD },
				{ "ARMOR_WRIST",	ITEM::ARMOR_WRIST },
				{ "ARMOR_FOOTS",	ITEM::ARMOR_FOOTS },
				{ "ARMOR_NECK",		ITEM::ARMOR_NECK },
				{ "ARMOR_EAR",		ITEM::ARMOR_EAR },
			}
		},

		{
			ITEM::TYPE_USE,
			{
				{ "USE_POTION",							ITEM::USE_POTION },
				{ "USE_TALISMAN",						ITEM::USE_TALISMAN },
				{ "USE_TUNING",							ITEM::USE_TUNING },
				{ "USE_MOVE",							ITEM::USE_MOVE },
				{ "USE_TREASURE_BOX",					ITEM::USE_TREASURE_BOX },
				{ "USE_MONEYBAG",						ITEM::USE_MONEYBAG },
				{ "USE_BAIT",							ITEM::USE_BAIT },
				{ "USE_ABILITY_UP",						ITEM::USE_ABILITY_UP },
				{ "USE_AFFECT",							ITEM::USE_AFFECT },
				{ "USE_CREATE_STONE",					ITEM::USE_CREATE_STONE },
				{ "USE_SPECIAL",						ITEM::USE_SPECIAL },
				{ "USE_POTION_NODELAY",					ITEM::USE_POTION_NODELAY },
				{ "USE_CLEAR",							ITEM::USE_CLEAR },
				{ "USE_INVISIBILITY",					ITEM::USE_INVISIBILITY },
				{ "USE_DETACHMENT",						ITEM::USE_DETACHMENT },
				{ "USE_BUCKET",							ITEM::USE_BUCKET },
				{ "USE_POTION_CONTINUE",				ITEM::USE_POTION_CONTINUE },
				{ "USE_CLEAN_SOCKET",					ITEM::USE_CLEAN_SOCKET },
				{ "USE_CHANGE_ATTRIBUTE",				ITEM::USE_CHANGE_ATTRIBUTE },
				{ "USE_ADD_ATTRIBUTE",					ITEM::USE_ADD_ATTRIBUTE },
				{ "USE_ADD_ACCESSORY_SOCKET",			ITEM::USE_ADD_ACCESSORY_SOCKET },
				{ "USE_PUT_INTO_ACCESSORY_SOCKET",		ITEM::USE_PUT_INTO_ACCESSORY_SOCKET },
				{ "USE_ADD_ATTRIBUTE2",					ITEM::USE_ADD_ATTRIBUTE2 },
				{ "USE_RECIPE",							ITEM::USE_RECIPE },
				{ "USE_CHANGE_ATTRIBUTE2",				ITEM::USE_CHANGE_ATTRIBUTE2 },
				{ "USE_BIND",							ITEM::USE_BIND },
				{ "USE_UNBIND",							ITEM::USE_UNBIND },
			}
		},

		{
			ITEM::TYPE_AUTOUSE,
			{
				{ "AUTOUSE_POTION",			ITEM::AUTOUSE_POTION },
				{ "AUTOUSE_ABILITY_UP",		ITEM::AUTOUSE_ABILITY_UP },
				{ "AUTOUSE_BOMB",			ITEM::AUTOUSE_BOMB },
				{ "AUTOUSE_GOLD",			ITEM::AUTOUSE_GOLD },
				{ "AUTOUSE_MONEYBAG",		ITEM::AUTOUSE_MONEYBAG },
				{ "AUTOUSE_TREASURE_BOX",	ITEM::AUTOUSE_TREASURE_BOX },
			}
		},

		{
			ITEM::TYPE_MATERIAL,
			{
				{ "MATERIAL_LEATHER",	ITEM::MATERIAL_LEATHER },
				{ "MATERIAL_BLOOD",		ITEM::MATERIAL_BLOOD },
				{ "MATERIAL_ROOT",		ITEM::MATERIAL_ROOT },
				{ "MATERIAL_NEEDLE",	ITEM::MATERIAL_NEEDLE },
				{ "MATERIAL_JEWEL",		ITEM::MATERIAL_JEWEL },
			}
		},

		{
			ITEM::TYPE_SPECIAL,
			{
				{ "SPECIAL_MAP",		ITEM::SPECIAL_MAP },
				{ "SPECIAL_KEY",		ITEM::SPECIAL_KEY },
				{ "SPECIAL_DOC",		ITEM::SPECIAL_DOC },
				{ "SPECIAL_SPIRIT",		ITEM::SPECIAL_SPIRIT },
			}
		},

		{
			ITEM::TYPE_TOOL,
			{
				{ "TOOL_FISHING_ROD",	ITEM::TOOL_FISHING_ROD },
			}
		},

		{
			ITEM::TYPE_LOTTERY,
			{
				{ "LOTTERY_TICKET",		ITEM::LOTTERY_TICKET },
				{ "LOTTERY_INSTANT",	ITEM::LOTTERY_INSTANT },
			}
		},

		{
			ITEM::TYPE_METIN,
			{
				{ "METIN_NORMAL",		ITEM::METIN_NORMAL },
				{ "METIN_GOLD",			ITEM::METIN_GOLD },
			}
		},

		{
			ITEM::TYPE_FISH,
			{
				{ "FISH_ALIVE",			ITEM::FISH_ALIVE },
				{ "FISH_DEAD",			ITEM::FISH_DEAD },
			}
		},

		{
			ITEM::TYPE_RESOURCE,
			{
				{ "RESOURCE_FISHBONE",			ITEM::RESOURCE_FISHBONE },
				{ "RESOURCE_WATERSTONEPIECE",	ITEM::RESOURCE_WATERSTONEPIECE },
				{ "RESOURCE_WATERSTONE",		ITEM::RESOURCE_WATERSTONE },
				{ "RESOURCE_BLOOD_PEARL",		ITEM::RESOURCE_BLOOD_PEARL },
				{ "RESOURCE_BLUE_PEARL",		ITEM::RESOURCE_BLUE_PEARL },
				{ "RESOURCE_WHITE_PEARL",		ITEM::RESOURCE_WHITE_PEARL },
				{ "RESOURCE_BUCKET",			ITEM::RESOURCE_BUCKET },
				{ "RESOURCE_CRYSTAL",			ITEM::RESOURCE_CRYSTAL },
				{ "RESOURCE_GEM",				ITEM::RESOURCE_GEM },
				{ "RESOURCE_STONE",				ITEM::RESOURCE_STONE },
				{ "RESOURCE_METIN",				ITEM::RESOURCE_METIN },
				{ "RESOURCE_ORE",				ITEM::RESOURCE_ORE },
			}
		},

		{
			ITEM::TYPE_UNIQUE,
			{
				{ "UNIQUE_NONE",				ITEM::UNIQUE_NONE },
				{ "UNIQUE_BOOK",				ITEM::UNIQUE_BOOK },
				{ "UNIQUE_SPECIAL_RIDE",		ITEM::UNIQUE_SPECIAL_RIDE },
				{ "UNIQUE_SPECIAL_MOUNT_RIDE",	ITEM::UNIQUE_SPECIAL_MOUNT_RIDE },
			}
		},

		{
			ITEM::TYPE_COSTUME,
			{
				{ "COSTUME_BODY",		ITEM::COSTUME_BODY },
				{ "COSTUME_HAIR",		ITEM::COSTUME_HAIR },
			}
		},
	};

	auto it = SubTypeDict.find(bType);
	if (it != SubTypeDict.end())
	{
		auto it2 = it->second.find(c_stItemSubType);
		if (it2 != it->second.end())
			return it2->second;
	}

	return 0;
}

uint32_t ItemAntiFlagStringToInteger(const std::string& c_stItemAntiFlag)
{
	uint32_t dwAntiFlag = 0;
	const static EnumDictionary AntiFlagDict = {
		{ "ANTI_FEMALE", ITEM::ANTIFLAG_FEMALE },
		{ "ANTI_MALE", ITEM::ANTIFLAG_MALE },
		{ "ANTI_MUSA", ITEM::ANTIFLAG_WARRIOR },
		{ "ANTI_ASSASSIN", ITEM::ANTIFLAG_ASSASSIN },
		{ "ANTI_SURA", ITEM::ANTIFLAG_SURA },
		{ "ANTI_MUDANG", ITEM::ANTIFLAG_SHAMAN },
		{ "ANTI_GET", ITEM::ANTIFLAG_GET },
		{ "ANTI_DROP", ITEM::ANTIFLAG_DROP },
		{ "ANTI_SELL", ITEM::ANTIFLAG_SELL },
		{ "ANTI_EMPIRE_A", ITEM::ANTIFLAG_EMPIRE_A },
		{ "ANTI_EMPIRE_B", ITEM::ANTIFLAG_EMPIRE_B },
		{ "ANTI_EMPIRE_C", ITEM::ANTIFLAG_EMPIRE_R },
		{ "ANTI_SAVE", ITEM::ANTIFLAG_SAVE },
		{ "ANTI_GIVE", ITEM::ANTIFLAG_GIVE },
		{ "ANTI_PKDROP", ITEM::ANTIFLAG_PKDROP },
		{ "ANTI_STACK", ITEM::ANTIFLAG_STACK },
		{ "ANTI_MYSHOP", ITEM::ANTIFLAG_MYSHOP },
		{ "ANTI_SAFEBOX", ITEM::ANTIFLAG_SAFEBOX },
	};

	for (const auto& [stAntiFlag, dwValue] : AntiFlagDict)
	{
		if (c_stItemAntiFlag.find(stAntiFlag) != std::string::npos)
			dwAntiFlag += dwValue;
	}

	return dwAntiFlag;
}

uint32_t ItemFlagStringToInteger(const std::string& c_stItemFlag)
{
	uint32_t dwFlag = 0;
	const static EnumDictionary FlagDict = {
		{ "ITEM_TUNABLE", ITEM::FLAG_REFINEABLE },
		{ "ITEM_SAVE", ITEM::FLAG_SAVE },
		{ "ITEM_STACKABLE", ITEM::FLAG_STACKABLE },
		{ "COUNT_PER_1GOLD", ITEM::FLAG_COUNT_PER_1GOLD },
		{ "ITEM_SLOW_QUERY", ITEM::FLAG_SLOW_QUERY },
		{ "ITEM_UNIQUE", ITEM::FLAG_UNIQUE },
		{ "ITEM_MAKECOUNT", ITEM::FLAG_MAKECOUNT },
		{ "CONFIRM_WHEN_USE", ITEM::FLAG_CONFIRM_WHEN_USE },
		{ "QUEST_USE", ITEM::FLAG_QUEST_USE },
		{ "QUEST_USE_MULTIPLE", ITEM::FLAG_QUEST_USE_MULTIPLE },
		{ "QUEST_GIVE", ITEM::FLAG_QUEST_GIVE },
		{ "LOG", ITEM::FLAG_LOG },
		{ "ITEM_IRREMOVABLE", ITEM::FLAG_IRREMOVABLE },
		{ "ITEM_APPLICABLE", ITEM::FLAG_APPLICABLE },
	};

	for (const auto& [stAntiFlag, dwValue] : FlagDict)
	{
		if (c_stItemFlag.find(stAntiFlag) != std::string::npos)
			dwFlag += dwValue;
	}

	return dwFlag;
}

uint32_t ItemWearFlagStringToInteger(const std::string& c_stItemWearFlag)
{
	const static EnumDictionary ItemWearDict = {
		{ "WEAR_BODY",		ITEM::WEARABLE_BODY },
		{ "WEAR_HEAD", ITEM::WEARABLE_HEAD },
		{ "WEAR_FOOTS", ITEM::WEARABLE_FOOTS },
		{ "WEAR_WRIST", ITEM::WEARABLE_WRIST },
		{ "WEAR_WEAPON", ITEM::WEARABLE_WEAPON },
		{ "WEAR_NECK", ITEM::WEARABLE_NECK },
		{ "WEAR_EAR", ITEM::WEARABLE_EAR },
		{ "WEAR_SHIELD", ITEM::WEARABLE_UNIQUE },
		{ "WEAR_UNIQUE", ITEM::WEARABLE_SHIELD },
		{ "WEAR_ARROW", ITEM::WEARABLE_ARROW },
		{ "WEAR_HAIR", ITEM::WEARABLE_HAIR },
		{ "WEAR_ABILITY", ITEM::WEARABLE_ABILITY },
	};

	auto it = ItemWearDict.find(c_stItemWearFlag);
	if (it != ItemWearDict.end())
		return it->second;

	return 0;
}

uint32_t ItemImmuneStringToInteger(const std::string& c_stItemImmune)
{
	uint32_t dwImmuneFlag = 0;
	const static EnumDictionary ImmuneDict = {
		{ "STUN",		ITEM::IMMUNE_STUN },
		{ "SLOW",		ITEM::IMMUNE_SLOW },
		{ "FALL",		ITEM::IMMUNE_FALL },
		{ "CURSE",		ITEM::IMMUNE_CURSE },
		{ "POISON",		ITEM::IMMUNE_POISON },
		{ "TERROR",		ITEM::IMMUNE_TERROR },
		{ "REFLECT",	ITEM::IMMUNE_REFLECT },
	};

	for (const auto& [stAntiFlag, dwValue] : ImmuneDict)
	{
		if (c_stItemImmune.find(stAntiFlag) != std::string::npos)
			dwImmuneFlag += dwValue;
	}

	return dwImmuneFlag;
}

uint8_t ItemLimitTypeStringToInteger(const std::string& c_stItemLimitType)
{
	const static EnumDictionary LimitTypeDict = {
		{ "LIMIT_NONE", ITEM::LIMIT_NONE },
		{ "LEVEL", ITEM::LIMIT_LEVEL },
		{ "STR", ITEM::LIMIT_STR },
		{ "DEX", ITEM::LIMIT_DEX },
		{ "INT", ITEM::LIMIT_INT },
		{ "CON", ITEM::LIMIT_CON },
		{ "REAL_TIME", ITEM::LIMIT_REAL_TIME },
		{ "REAL_TIME_FIRST_USE", ITEM::LIMIT_REAL_TIME_START_FIRST_USE },
		{ "TIMER_BASED_ON_WEAR", ITEM::LIMIT_TIMER_BASED_ON_WEAR },
		{ "LIMIT_MAX_LEVEL", ITEM::LIMIT_MAX_LEVEL },
	};

	auto it = LimitTypeDict.find(c_stItemLimitType);
	if (it != LimitTypeDict.end())
		return it->second;

	return ITEM::LIMIT_NONE;
}

uint8_t ItemApplyTypeStringToInteger(const std::string& c_stItemApplyType)
{
	const static EnumDictionary ApplyTypeDict = {
		{ "ITEM::APPLY_NONE", ITEM::APPLY_NONE },
		{ "ITEM::APPLY_MAX_HP", ITEM::APPLY_MAX_HP },
		{ "ITEM::APPLY_MAX_SP", ITEM::APPLY_MAX_SP },
		{ "ITEM::APPLY_CON", ITEM::APPLY_CON },
		{ "ITEM::APPLY_INT", ITEM::APPLY_INT },
		{ "ITEM::APPLY_STR", ITEM::APPLY_STR },
		{ "ITEM::APPLY_DEX", ITEM::APPLY_DEX },
		{ "ITEM::APPLY_ATT_SPEED", ITEM::APPLY_ATT_SPEED },
		{ "ITEM::APPLY_MOV_SPEED", ITEM::APPLY_MOV_SPEED },
		{ "ITEM::APPLY_CAST_SPEED", ITEM::APPLY_CAST_SPEED },
		{ "ITEM::APPLY_HP_REGEN", ITEM::APPLY_HP_REGEN },
		{ "ITEM::APPLY_SP_REGEN", ITEM::APPLY_SP_REGEN },
		{ "ITEM::APPLY_POISON_PCT", ITEM::APPLY_POISON_PCT },
		{ "ITEM::APPLY_STUN_PCT", ITEM::APPLY_STUN_PCT },
		{ "ITEM::APPLY_SLOW_PCT", ITEM::APPLY_SLOW_PCT },
		{ "ITEM::APPLY_CRITICAL_PCT", ITEM::APPLY_CRITICAL_PCT },
		{ "ITEM::APPLY_PENETRATE_PCT", ITEM::APPLY_PENETRATE_PCT },
		{ "ITEM::APPLY_ATTBONUS_HUMAN", ITEM::APPLY_ATTBONUS_HUMAN },
		{ "ITEM::APPLY_ATTBONUS_ANIMAL", ITEM::APPLY_ATTBONUS_ANIMAL },
		{ "ITEM::APPLY_ATTBONUS_ORC", ITEM::APPLY_ATTBONUS_ORC },
		{ "ITEM::APPLY_ATTBONUS_MILGYO", ITEM::APPLY_ATTBONUS_MILGYO },
		{ "ITEM::APPLY_ATTBONUS_UNDEAD", ITEM::APPLY_ATTBONUS_UNDEAD },
		{ "ITEM::APPLY_ATTBONUS_DEVIL", ITEM::APPLY_ATTBONUS_DEVIL },
		{ "ITEM::APPLY_STEAL_HP", ITEM::APPLY_STEAL_HP },
		{ "ITEM::APPLY_STEAL_SP", ITEM::APPLY_STEAL_SP },
		{ "ITEM::APPLY_MANA_BURN_PCT", ITEM::APPLY_MANA_BURN_PCT },
		{ "ITEM::APPLY_DAMAGE_SP_RECOVER", ITEM::APPLY_DAMAGE_SP_RECOVER },
		{ "ITEM::APPLY_BLOCK", ITEM::APPLY_BLOCK },
		{ "ITEM::APPLY_DODGE", ITEM::APPLY_DODGE },
		{ "ITEM::APPLY_RESIST_SWORD", ITEM::APPLY_RESIST_SWORD },
		{ "ITEM::APPLY_RESIST_TWOHAND", ITEM::APPLY_RESIST_TWOHAND },
		{ "ITEM::APPLY_RESIST_DAGGER", ITEM::APPLY_RESIST_DAGGER },
		{ "ITEM::APPLY_RESIST_BELL", ITEM::APPLY_RESIST_BELL },
		{ "ITEM::APPLY_RESIST_FAN", ITEM::APPLY_RESIST_FAN },
		{ "ITEM::APPLY_RESIST_BOW", ITEM::APPLY_RESIST_BOW },
		{ "ITEM::APPLY_RESIST_FIRE", ITEM::APPLY_RESIST_FIRE },
		{ "ITEM::APPLY_RESIST_ELEC", ITEM::APPLY_RESIST_ELEC },
		{ "ITEM::APPLY_RESIST_MAGIC", ITEM::APPLY_RESIST_MAGIC },
		{ "ITEM::APPLY_RESIST_WIND", ITEM::APPLY_RESIST_WIND },
		{ "ITEM::APPLY_REFLECT_MELEE", ITEM::APPLY_REFLECT_MELEE },
		{ "ITEM::APPLY_REFLECT_CURSE", ITEM::APPLY_REFLECT_CURSE },
		{ "ITEM::APPLY_POISON_REDUCE", ITEM::APPLY_POISON_REDUCE },
		{ "ITEM::APPLY_KILL_SP_RECOVER", ITEM::APPLY_KILL_SP_RECOVER },
		{ "ITEM::APPLY_EXP_DOUBLE_BONUS", ITEM::APPLY_EXP_DOUBLE_BONUS },
		{ "ITEM::APPLY_GOLD_DOUBLE_BONUS", ITEM::APPLY_GOLD_DOUBLE_BONUS },
		{ "ITEM::APPLY_ITEM_DROP_BONUS", ITEM::APPLY_ITEM_DROP_BONUS },
		{ "ITEM::APPLY_POTION_BONUS", ITEM::APPLY_POTION_BONUS },
		{ "ITEM::APPLY_KILL_HP_RECOVER", ITEM::APPLY_KILL_HP_RECOVER },
		{ "ITEM::APPLY_IMMUNE_STUN", ITEM::APPLY_IMMUNE_STUN },
		{ "ITEM::APPLY_IMMUNE_SLOW", ITEM::APPLY_IMMUNE_SLOW },
		{ "ITEM::APPLY_IMMUNE_FALL", ITEM::APPLY_IMMUNE_FALL },
		{ "ITEM::APPLY_SKILL", ITEM::APPLY_SKILL },
		{ "ITEM::APPLY_BOW_DISTANCE", ITEM::APPLY_BOW_DISTANCE },
		{ "ITEM::APPLY_ATT_GRADE_BONUS", ITEM::APPLY_ATT_GRADE_BONUS },
		{ "ITEM::APPLY_DEF_GRADE_BONUS", ITEM::APPLY_DEF_GRADE_BONUS },
		{ "ITEM::APPLY_MAGIC_ATT_GRADE", ITEM::APPLY_MAGIC_ATT_GRADE },
		{ "ITEM::APPLY_MAGIC_DEF_GRADE", ITEM::APPLY_MAGIC_DEF_GRADE },
		{ "ITEM::APPLY_CURSE_PCT", ITEM::APPLY_CURSE_PCT },
		{ "ITEM::APPLY_MAX_STAMINA", ITEM::APPLY_MAX_STAMINA },
		{ "ITEM::APPLY_ATTBONUS_WARRIOR", ITEM::APPLY_ATT_BONUS_TO_WARRIOR },
		{ "ITEM::APPLY_ATTBONUS_ASSASSIN", ITEM::APPLY_ATT_BONUS_TO_ASSASSIN },
		{ "ITEM::APPLY_ATTBONUS_SURA", ITEM::APPLY_ATT_BONUS_TO_SURA },
		{ "ITEM::APPLY_ATTBONUS_SHAMAN", ITEM::APPLY_ATT_BONUS_TO_SHAMAN },
		{ "ITEM::APPLY_ATTBONUS_MONSTER", ITEM::APPLY_ATT_BONUS_TO_MONSTER },
		{ "ITEM::APPLY_MALL_ATTBONUS", ITEM::APPLY_MALL_ATTBONUS },
		{ "ITEM::APPLY_MALL_DEFBONUS", ITEM::APPLY_MALL_DEFBONUS },
		{ "ITEM::APPLY_MALL_EXPBONUS", ITEM::APPLY_MALL_EXPBONUS },
		{ "ITEM::APPLY_MALL_ITEMBONUS", ITEM::APPLY_MALL_ITEMBONUS },
		{ "ITEM::APPLY_MALL_GOLDBONUS", ITEM::APPLY_MALL_GOLDBONUS },
		{ "ITEM::APPLY_MAX_HP_PCT", ITEM::APPLY_MAX_HP_PCT },
		{ "ITEM::APPLY_MAX_SP_PCT", ITEM::APPLY_MAX_SP_PCT },
		{ "ITEM::APPLY_SKILL_DAMAGE_BONUS", ITEM::APPLY_SKILL_DAMAGE_BONUS },
		{ "ITEM::APPLY_NORMAL_HIT_DAMAGE_BONUS", ITEM::APPLY_NORMAL_HIT_DAMAGE_BONUS },
		{ "ITEM::APPLY_SKILL_DEFEND_BONUS", ITEM::APPLY_SKILL_DEFEND_BONUS },
		{ "ITEM::APPLY_NORMAL_HIT_DEFEND_BONUS", ITEM::APPLY_NORMAL_HIT_DEFEND_BONUS },
		{ "ITEM::APPLY_EXTRACT_HP_PCT", ITEM::APPLY_EXTRACT_HP_PCT },
		{ "ITEM::APPLY_RESIST_WARRIOR", ITEM::APPLY_PC_BANG_EXP_BONUS },
		{ "ITEM::APPLY_RESIST_ASSASSIN", ITEM::APPLY_PC_BANG_DROP_BONUS },
		{ "ITEM::APPLY_RESIST_SURA", ITEM::APPLY_RESIST_WARRIOR },
		{ "ITEM::APPLY_RESIST_SHAMAN", ITEM::APPLY_RESIST_ASSASSIN },
		{ "ITEM::APPLY_DEF_GRADE", ITEM::APPLY_RESIST_SURA },
		{ "ITEM::APPLY_COSTUME_ATTR_BONUS", ITEM::APPLY_RESIST_SHAMAN },
		{ "ITEM::APPLY_MAGIC_ATTBONUS_PER", ITEM::APPLY_ENERGY },
		{ "ITEM::APPLY_MELEE_MAGIC_ATTBONUS_PER", ITEM::APPLY_DEF_GRADE },
		{ "ITEM::APPLY_RESIST_ICE", ITEM::APPLY_COSTUME_ATTR_BONUS },
		{ "ITEM::APPLY_RESIST_EARTH", ITEM::APPLY_MAGIC_ATTBONUS_PER },
		{ "ITEM::APPLY_RESIST_DARK", ITEM::APPLY_MELEE_MAGIC_ATTBONUS_PER },
		{ "ITEM::APPLY_ANTI_CRITICAL_PCT", ITEM::APPLY_RESIST_ICE },
		{ "ITEM::APPLY_ANTI_PENETRATE_PCT", ITEM::APPLY_RESIST_EARTH },
		{ "ITEM::APPLY_RESIST_DARK", ITEM::APPLY_RESIST_DARK },
		{ "ITEM::APPLY_ANTI_CRITICAL_PCT", ITEM::APPLY_ANTI_CRITICAL_PCT },
		{ "ITEM::APPLY_ANTI_PENETRATE_PCT", ITEM::APPLY_ANTI_PENETRATE_PCT },
	};

	auto it = ApplyTypeDict.find(c_stItemApplyType);
	if (it != ApplyTypeDict.end())
		return it->second;

	return ITEM::APPLY_NONE;
}

bool ReadItemProto(std::vector<std::string>& tokens, TItemTable& table)
{
	if (tokens.size() < ITEM_PROTO_MAX_TOKEN)
		return false;

	try
	{
		const std::string stItemVnum = tokens[EItemProtoStructure::ITEM_VNUM];
		const size_t separator_pos = stItemVnum.find('~');
		if (separator_pos == std::string::npos)
		{
			table.dwVnum = std::stoi(stItemVnum);
		}
		else
		{
			table.dwVnum = std::stoi(stItemVnum.substr(0, separator_pos));
			table.dwVnumRange = std::stoi(stItemVnum.substr(separator_pos + 1));
		}

		memcpy(&table.szName, &tokens[EItemProtoStructure::ITEM_NAME], ITEM::NAME_MAX_LEN + 1);
		memcpy(&table.szLocaleName, &tokens[EItemProtoStructure::ITEM_NAME], ITEM::NAME_MAX_LEN + 1);

		table.bType = ItemTypeStringToInteger(tokens[EItemProtoStructure::ITEM_TYPE]);
		table.bSubType = ItemSubTypeStringToInteger(table.bType, tokens[EItemProtoStructure::ITEM_SUBTYPE]);

		table.bSize = std::stoi(tokens[EItemProtoStructure::SLOT_HEIGHT]);

		table.dwAntiFlags = ItemAntiFlagStringToInteger(tokens[EItemProtoStructure::ANTIFLAGS]);
		table.dwFlags = ItemFlagStringToInteger(tokens[EItemProtoStructure::FLAGS]);
		table.dwWearFlags = ItemWearFlagStringToInteger(tokens[EItemProtoStructure::WEARFLAGS]);
		table.dwImmuneFlag = ItemImmuneStringToInteger(tokens[EItemProtoStructure::IMMUNEFLAGS]);

		table.dwIBuyItemPrice = std::stoi(tokens[EItemProtoStructure::GOLD]);
		table.dwISellItemPrice = std::stoi(tokens[EItemProtoStructure::SHOP_BUY_PRICE]);

		for (uint8_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
		{
			table.aLimits[i].bType = ItemLimitTypeStringToInteger(tokens[EItemProtoStructure::LIMIT_TYPE0 + i * 2]);
			table.aLimits[i].lValue = std::stoi(tokens[EItemProtoStructure::LIMIT_VALUE0 + i * 2]);

			if (ITEM::LIMIT_REAL_TIME_START_FIRST_USE == table.aLimits[i].bType)
				table.cLimitRealTimeFirstUseIndex = (int8_t)i;
			else
				table.cLimitRealTimeFirstUseIndex = -1;

			if (ITEM::LIMIT_TIMER_BASED_ON_WEAR == table.aLimits[i].bType)
				table.cLimitTimerBasedOnWearIndex = (int8_t)i;
			else
				table.cLimitTimerBasedOnWearIndex = -1;
		}

		for (uint8_t i = 0; i <ITEM::APPLY_MAX_NUM; ++i)
		{
			table.aApplies[i].bType = ItemApplyTypeStringToInteger(tokens[EItemProtoStructure::APPLY_TYPE0 + i * 2]);
			table.aApplies[i].lValue = std::stoi(tokens[EItemProtoStructure::APPLY_VALUE0 + i * 2]);
		}

		for (uint8_t i = 0; i < ITEM::VALUES_MAX_NUM; ++i)
			table.alValues[i] = std::stoi(tokens[EItemProtoStructure::VALUE0 + i]);

		memset(&table.alSockets, 0, sizeof(table.alSockets));

		table.dwRefinedVnum = std::stoi(tokens[EItemProtoStructure::REFINED_VNUM]);
		table.wRefineSet = std::stoi(tokens[EItemProtoStructure::REFINESET]);
		table.bAlterToMagicItemPct = std::stoi(tokens[EItemProtoStructure::ALTER_TO_MAGIC_ITEM_PERCENT]);
		table.bSpecular = std::stoi(tokens[EItemProtoStructure::SPECULAR]);
		table.bGainSocketPct = std::stoi(tokens[EItemProtoStructure::GAIN_SOCKET_PERCENT]);
		table.sAddonType = std::stoi(tokens[EItemProtoStructure::ADDON_TYPE]);
	}
	catch (const std::exception& e)
	{
		SysLog("Could not tokenize item proto: {0}", e.what());
		return false;
	}

	return true;
}