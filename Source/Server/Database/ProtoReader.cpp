#include "stdafx.h"
#include <sstream>
#include <cmath>
#include "ProtoReader.h"
#include "CsvReader.h"

inline std::string trim_left(const std::string& str)
{
	std::string::size_type n = str.find_first_not_of(" \t\v\n\r");
    return n == std::string::npos ? str : str.substr(n, str.length());
}

inline std::string trim_right(const std::string& str)
{
	std::string::size_type n = str.find_last_not_of(" \t\v\n\r");
    return n == std::string::npos ? str : str.substr(0, n + 1);
}

std::string trim(const std::string& str){return trim_left(trim_right(str));}

static std::string* StringSplit(std::string strOrigin, std::string strTok)
{
    int32_t cutAt;
    int32_t index = 0;
	std::string* strResult = new std::string[30];

    while ((cutAt = strOrigin.find_first_of(strTok)) != strOrigin.npos)
    {
       if (cutAt > 0)
       {
            strResult[index++] = strOrigin.substr(0, cutAt);
       }
       strOrigin = strOrigin.substr(cutAt+1);
    }

    if (strOrigin.length() > 0)
    {
        strResult[index++] = strOrigin.substr(0, cutAt);
    }

	for(int32_t i=0;i<index;i++)
	{
		strResult[i] = trim(strResult[i]);
	}

    return strResult;
}



int32_t get_Item_Type_Value(std::string inputString)
{
	std::string arType[] = {"ITEM_NONE", "ITEM_WEAPON",
		"ITEM_ARMOR", "ITEM_USE", 
		"ITEM_AUTOUSE", "ITEM_MATERIAL",
		"ITEM_SPECIAL", "ITEM_TOOL", 
		"ITEM_LOTTERY", "ITEM_ELK",					//9

		"ITEM_METIN", "ITEM_CONTAINER", 
		"ITEM_FISH", "ITEM_ROD", 
		"ITEM_RESOURCE", "ITEM_CAMPFIRE",
		"ITEM_UNIQUE", "ITEM_SKILLBOOK", 
		"ITEM_QUEST", "ITEM_POLYMORPH",				//19

		"ITEM_TREASURE_BOX", "ITEM_TREASURE_KEY",
		"ITEM_SKILLFORGET", "ITEM_GIFTBOX", 
		"ITEM_PICK", "ITEM_HAIR", 
		"ITEM_TOTEM", "ITEM_BLEND", 
		"ITEM_COSTUME", "ITEM_DS",					//29
	
		"ITEM_SPECIAL_DS",	"ITEM_EXTRACT",
		"ITEM_SECONDARY_COIN",						//32

		"ITEM_RING",
		"ITEM_BELT",								//34
	};

	
	int32_t retInt = -1;

	for (int32_t j=0;j<sizeof(arType)/sizeof(arType[0]);j++) {
		std::string tempString = arType[j];
		if	(inputString.find(tempString)!= std::string::npos && tempString.find(inputString)!= std::string::npos)
		{
			retInt =  j;
			break;
		}
	}

	return retInt;

}

int32_t get_Item_SubType_Value(int32_t type_value, std::string inputString)
{
	static std::string arSub1[] = { "WEAPON_SWORD", "WEAPON_DAGGER", "WEAPON_BOW", "WEAPON_TWO_HANDED",
				"WEAPON_BELL", "WEAPON_FAN", "WEAPON_ARROW", "WEAPON_MOUNT_SPEAR"};
	static std::string arSub2[] = { "ARMOR_BODY", "ARMOR_HEAD", "ARMOR_SHIELD", "ARMOR_WRIST", "ARMOR_FOOTS",
				"ARMOR_NECK", "ARMOR_EAR", "ARMOR_NUM_TYPES"};
	static std::string arSub3[] = { "USE_POTION", "USE_TALISMAN", "USE_TUNING", "USE_MOVE", "USE_TREASURE_BOX", "USE_MONEYBAG", "USE_BAIT",
				"USE_ABILITY_UP", "USE_AFFECT", "USE_CREATE_STONE", "USE_SPECIAL", "USE_POTION_NODELAY", "USE_CLEAR",
				"USE_INVISIBILITY", "USE_DETACHMENT", "USE_BUCKET", "USE_POTION_CONTINUE", "USE_CLEAN_SOCKET",
				"USE_CHANGE_ATTRIBUTE", "USE_ADD_ATTRIBUTE", "USE_ADD_ACCESSORY_SOCKET", "USE_PUT_INTO_ACCESSORY_SOCKET",
				"USE_ADD_ATTRIBUTE2", "USE_RECIPE", "USE_CHANGE_ATTRIBUTE2", "USE_BIND", "USE_UNBIND", "USE_TIME_CHARGE_PER", "USE_TIME_CHARGE_FIX", "USE_PUT_INTO_BELT_SOCKET", "USE_PUT_INTO_RING_SOCKET"};
	static std::string arSub4[] = { "AUTOUSE_POTION", "AUTOUSE_ABILITY_UP", "AUTOUSE_BOMB", "AUTOUSE_GOLD", "AUTOUSE_MONEYBAG", "AUTOUSE_TREASURE_BOX"};
	static std::string arSub5[] = { "MATERIAL_LEATHER", "MATERIAL_BLOOD", "MATERIAL_ROOT", "MATERIAL_NEEDLE", "MATERIAL_JEWEL",
		"MATERIAL_DS_REFINE_NORMAL", "MATERIAL_DS_REFINE_BLESSED", "MATERIAL_DS_REFINE_HOLLY"};
	static std::string arSub6[] = { "SPECIAL_MAP", "SPECIAL_KEY", "SPECIAL_DOC", "SPECIAL_SPIRIT"};
	static std::string arSub7[] = { "TOOL_FISHING_ROD" };
	static std::string arSub8[] = { "LOTTERY_TICKET", "LOTTERY_INSTANT" };
	static std::string arSub10[] = { "METIN_NORMAL", "METIN_GOLD" };
	static std::string arSub12[] = { "FISH_ALIVE", "FISH_DEAD"};
	static std::string arSub14[] = { "RESOURCE_FISHBONE", "RESOURCE_WATERSTONEPIECE", "RESOURCE_WATERSTONE", "RESOURCE_BLOOD_PEARL",
						"RESOURCE_BLUE_PEARL", "RESOURCE_WHITE_PEARL", "RESOURCE_BUCKET", "RESOURCE_CRYSTAL", "RESOURCE_GEM",
						"RESOURCE_STONE", "RESOURCE_METIN", "RESOURCE_ORE" };
	static std::string arSub16[] = { "UNIQUE_NONE", "UNIQUE_BOOK", "UNIQUE_SPECIAL_RIDE", "UNIQUE_3", "UNIQUE_4", "UNIQUE_5",
					"UNIQUE_6", "UNIQUE_7", "UNIQUE_8", "UNIQUE_9", "USE_SPECIAL"};
	static std::string arSub28[] = { "COSTUME_BODY", "COSTUME_HAIR" };
	static std::string arSub29[] = { "DS_SLOT1", "DS_SLOT2", "DS_SLOT3", "DS_SLOT4", "DS_SLOT5", "DS_SLOT6" };
	static std::string arSub31[] = { "EXTRACT_DRAGON_SOUL", "EXTRACT_DRAGON_HEART" };
	
	static std::string* arSubType[] = {
		nullptr,	//0
		arSub1,		//1
		arSub2,		//2
		arSub3,		//3
		arSub4,		//4
		arSub5,		//5
		arSub6,		//6
		arSub7,		//7
		arSub8,		//8
		nullptr,	//9
		arSub10,	//10
		nullptr,	//11
		arSub12,	//12
		nullptr,	//13
		arSub14,	//14
		nullptr,	//15
		arSub16,	//16
		nullptr,	//17
		nullptr,	//18
		nullptr,	//19
		nullptr,	//20
		nullptr,	//21
		nullptr,	//22
		nullptr,	//23
		nullptr,	//24
		nullptr,	//25
		nullptr,	//26
		nullptr,	//27
		arSub28,	//28
		arSub29,	//29
		arSub29,	//30
		arSub31,	//31
		nullptr,	//32
		nullptr,	//33
		nullptr,	//34
	};

	static int32_t arNumberOfSubtype[_countof(arSubType)] = {
		0,
		sizeof(arSub1)/sizeof(arSub1[0]),
		sizeof(arSub2)/sizeof(arSub2[0]),
		sizeof(arSub3)/sizeof(arSub3[0]),
		sizeof(arSub4)/sizeof(arSub4[0]),
		sizeof(arSub5)/sizeof(arSub5[0]),
		sizeof(arSub6)/sizeof(arSub6[0]),
		sizeof(arSub7)/sizeof(arSub7[0]),
		sizeof(arSub8)/sizeof(arSub8[0]),
		0,
		sizeof(arSub10)/sizeof(arSub10[0]),
		0,
		sizeof(arSub12)/sizeof(arSub12[0]),
		0,
		sizeof(arSub14)/sizeof(arSub14[0]),
		0,
		sizeof(arSub16)/sizeof(arSub16[0]),
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		sizeof(arSub28)/sizeof(arSub28[0]),
		sizeof(arSub29)/sizeof(arSub29[0]),
		sizeof(arSub29)/sizeof(arSub29[0]),
		sizeof(arSub31)/sizeof(arSub31[0]),
		0,
		0,
		0,
	};
	

	assert(_countof(arSubType) > type_value && "Subtype rule: Out of range!!");

	if (_countof(arSubType) <= type_value)
	{
		SysLog("SubType : Out of range!! (type_value: {}, count of registered subtype: {}", type_value, _countof(arSubType));
		return -1;
	}

	if (!arSubType[type_value])
		return 0;

	int32_t retInt = -1;

	for (int32_t j=0;j<arNumberOfSubtype[type_value];j++) {
		std::string tempString = arSubType[type_value][j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{
			retInt =  j;
			break;
		}
	}
	
	return retInt;
}





int32_t get_Item_AntiFlag_Value(std::string inputString)
{

	std::string arAntiFlag[] = {"ANTI_FEMALE", "ANTI_MALE", "ANTI_MUSA", "ANTI_ASSASSIN", "ANTI_SURA", "ANTI_MUDANG",
							"ANTI_GET", "ANTI_DROP", "ANTI_SELL", "ANTI_EMPIRE_A", "ANTI_EMPIRE_B", "ANTI_EMPIRE_C",
							"ANTI_SAVE", "ANTI_GIVE", "ANTI_pDROP", "ANTI_STACK", "ANTI_MYSHOP", "ANTI_SAFEBOX"};


	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, "|");
	for(int32_t i =0;i<sizeof(arAntiFlag)/sizeof(arAntiFlag[0]);i++) 
	{
		std::string tempString = arAntiFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) 
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Item_Flag_Value(std::string inputString)
{

	std::string arFlag[] = {"ITEM_TUNABLE", "ITEM_SAVE", "ITEM_STACKABLE", "COUNT_PER_1GOLD", "ITEM_SLOW_QUERY", "ITEM_UNIQUE",
			"ITEM_MAKECOUNT", "ITEM_IRREMOVABLE", "CONFIRM_WHEN_USE", "QUEST_USE", "QUEST_USE_MULTIPLE",
			"QUEST_GIVE", "ITEM_QUEST", "LOG", "STACKABLE", "SLOW_QUERY", "REFINEABLE", "IRREMOVABLE", "ITEM_APPLICABLE"};


	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, "|");
	for(int32_t i =0;i<sizeof(arFlag)/sizeof(arFlag[0]);i++) {
		std::string tempString = arFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0)
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Item_WearFlag_Value(std::string inputString)
{

	std::string arWearrFlag[] = {"WEAR_BODY", "WEAR_HEAD", "WEAR_FOOTS", "WEAR_WRIST", "WEAR_WEAPON", "WEAR_NECK", "WEAR_EAR", "WEAR_SHIELD", "WEAR_UNIQUE",
					"WEAR_ARROW", "WEAR_HAIR", "WEAR_ABILITY"};


	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, "|");
	for(int32_t i =0;i<sizeof(arWearrFlag)/sizeof(arWearrFlag[0]);i++) 
	{
		std::string tempString = arWearrFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0)
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Item_Immune_Value(std::string inputString)
{

	std::string arImmune[] = {"PARA","CURSE","STUN","SLEEP","SLOW","POISON","TERROR"};

	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, "|");
	for(int32_t i =0;i<sizeof(arImmune)/sizeof(arImmune[0]);i++) 
	{
		std::string tempString = arImmune[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0)
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Item_LimitType_Value(std::string inputString)
{
	std::string arLimitType[] = {"LIMIT_NONE", "LEVEL", "STR", "DEX", "INT", "CON", "REAL_TIME", "REAL_TIME_FIRST_USE", "TIMER_BASED_ON_WEAR"};
	
	int32_t retInt = -1;
	for (int32_t j=0;j<sizeof(arLimitType)/sizeof(arLimitType[0]);j++) 
	{
		std::string tempString = arLimitType[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{
			retInt =  j;
			break;
		}
	}

	return retInt;
}

int32_t get_Item_ApplyType_Value(std::string inputString)
{
	std::string arApplyType[] = {"APPLY_NONE", "APPLY_MAX_HP", "APPLY_MAX_SP", "APPLY_CON", "APPLY_INT", "APPLY_STR", "APPLY_DEX", "APPLY_ATT_SPEED",
			"APPLY_MOV_SPEED", "APPLY_CAST_SPEED", "APPLY_HP_REGEN", "APPLY_SP_REGEN", "APPLY_POISON_PCT", "APPLY_STUN_PCT",
			"APPLY_SLOW_PCT", "APPLY_CRITICAL_PCT", "APPLY_PENETRATE_PCT", "APPLY_ATTBONUS_HUMAN", "APPLY_ATTBONUS_ANIMAL",
			"APPLY_ATTBONUS_ORC", "APPLY_ATTBONUS_MILGYO", "APPLY_ATTBONUS_UNDEAD", "APPLY_ATTBONUS_DEVIL", "APPLY_STEAL_HP",
			"APPLY_STEAL_SP", "APPLY_MANA_BURN_PCT", "APPLY_DAMAGE_SP_RECOVER", "APPLY_BLOCK", "APPLY_DODGE", "APPLY_RESIST_SWORD",
			"APPLY_RESIST_TWOHAND", "APPLY_RESIST_DAGGER", "APPLY_RESIST_BELL", "APPLY_RESIST_FAN", "APPLY_RESIST_BOW", "APPLY_RESIST_FIRE",
			"APPLY_RESIST_ELEC", "APPLY_RESIST_MAGIC", "APPLY_RESIST_WIND", "APPLY_REFLECT_MELEE", "APPLY_REFLECT_CURSE", "APPLY_POISON_REDUCE",
			"APPLY_KILL_SP_RECOVER", "APPLY_EXP_DOUBLE_BONUS", "APPLY_GOLD_DOUBLE_BONUS", "APPLY_ITEM_DROP_BONUS", "APPLY_POTION_BONUS",
			"APPLY_KILL_HP_RECOVER", "APPLY_IMMUNE_STUN", "APPLY_IMMUNE_SLOW", "APPLY_IMMUNE_FALL", "APPLY_SKILL", "APPLY_BOW_DISTANCE",
			"APPLY_ATT_GRADE_BONUS", "APPLY_DEF_GRADE_BONUS", "APPLY_MAGIC_ATT_GRADE", "APPLY_MAGIC_DEF_GRADE", "APPLY_CURSE_PCT",
			"APPLY_MAX_STAMINA", "APPLY_ATTBONUS_WARRIOR", "APPLY_ATTBONUS_ASSASSIN", "APPLY_ATTBONUS_SURA", "APPLY_ATTBONUS_SHAMAN",
			"APPLY_ATTBONUS_MONSTER", "APPLY_MALL_ATTBONUS", "APPLY_MALL_DEFBONUS", "APPLY_MALL_EXPBONUS", "APPLY_MALL_ITEMBONUS",
			"APPLY_MALL_GOLDBONUS", "APPLY_MAX_HP_PCT", "APPLY_MAX_SP_PCT", "APPLY_SKILL_DAMAGE_BONUS", "APPLY_NORMAL_HIT_DAMAGE_BONUS",
			"APPLY_SKILL_DEFEND_BONUS", "APPLY_NORMAL_HIT_DEFEND_BONUS"
			"APPLY_EXTRACT_HP_PCT", "APPLY_RESIST_WARRIOR", "APPLY_RESIST_ASSASSIN", "APPLY_RESIST_SURA", "APPLY_RESIST_SHAMAN",
			"APPLY_DEF_GRADE", "APPLY_COSTUME_ATTR_BONUS", "APPLY_MAGIC_ATTBONUS_PER", "APPLY_MELEE_MAGIC_ATTBONUS_PER",
			"APPLY_RESIST_ICE", "APPLY_RESIST_EARTH", "APPLY_RESIST_DARK", "APPLY_ANTI_CRITICAL_PCT", "APPLY_ANTI_PENETRATE_PCT",
	};

	int32_t retInt = -1;
	for (int32_t j=0;j<sizeof(arApplyType)/sizeof(arApplyType[0]);j++) 
	{
		std::string tempString = arApplyType[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0)
		{ 
			retInt =  j;
			break;
		}
	}

	return retInt;

}

int32_t get_Mob_Rank_Value(std::string inputString)
{
	std::string arRank[] = {"PAWN", "S_PAWN", "KNIGHT", "S_KNIGHT", "BOSS", "KING"};

	int32_t retInt = -1;
	for (int32_t j=0;j<sizeof(arRank)/sizeof(arRank[0]);j++) {
		std::string tempString = arRank[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			retInt =  j;
			break;
		}
	}

	return retInt;
}


int32_t get_Mob_Type_Value(std::string inputString)
{
	std::string arType[] = { "MONSTER", "NPC", "STONE", "WARP", "DOOR", "BUILDING", "PC", "POLYMORPH_PC", "HORSE", "GOTO"};

	int32_t retInt = -1;

	for (int32_t j=0;j<sizeof(arType)/sizeof(arType[0]);j++) {
		std::string tempString = arType[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			retInt =  j;
			break;
		}
	}

	return retInt;
}

int32_t get_Mob_BattleType_Value(std::string inputString)
{
	std::string arBattleType[] = { "MELEE", "RANGE", "MAGIC", "SPECIAL", "POWER", "TANKER", "SUPER_POWER", "SUPER_TANKER"};

	int32_t retInt = -1;
	for (int32_t j=0;j<sizeof(arBattleType)/sizeof(arBattleType[0]);j++) 
	{
		std::string tempString = arBattleType[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{ 
			retInt =  j;
			break;
		}
	}

	return retInt;
}

int32_t get_Mob_Size_Value(std::string inputString)
{
	std::string arSize[] = { "SAMLL", "MEDIUM", "BIG"};

	int32_t retInt = 0;
	for (int32_t j=0;j<sizeof(arSize)/sizeof(arSize[0]);j++) 
	{
		std::string tempString = arSize[j];
		std::string tempInputString = trim(inputString);
		if	(tempInputString.compare(tempString)==0) 
		{
			retInt =  j + 1;
			break;
		}
	}

	return retInt;
}

int32_t get_Mob_AIFlag_Value(std::string inputString)
{
	std::string arAIFlag[] = {"AGGR","NOMOVE","COWARD","NOATTSHINSU","NOATTCHUNJO","NOATTJINNO","ATTMOB","BERSERK","STONESKIN","GODSPEED","DEATHBLOW","REVIVE"};


	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, ",");
	for(int32_t i =0;i<sizeof(arAIFlag)/sizeof(arAIFlag[0]);i++) {
		std::string tempString = arAIFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0)
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Mob_RaceFlag_Value(std::string inputString)
{
	std::string arRaceFlag[] = {"ANIMAL","UNDEAD","DEVIL","HUMAN","ORC","MILGYO","INSECT","FIRE","ICE","DESERT","TREE",
		"ATT_ELEC","ATT_FIRE","ATT_ICE","ATT_WIND","ATT_EARTH","ATT_DARK"};

	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, ",");
	for(int32_t i =0;i<sizeof(arRaceFlag)/sizeof(arRaceFlag[0]);i++) 
	{
		std::string tempString = arRaceFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) 
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}

int32_t get_Mob_ImmuneFlag_Value(std::string inputString)
{
	std::string arImmuneFlag[] = {"STUN","SLOW","FALL","CURSE","POISON","TERROR", "REFLECT"};

	int32_t retValue = 0;
	std::string* arInputString = StringSplit(inputString, ",");
	for(int32_t i =0;i<sizeof(arImmuneFlag)/sizeof(arImmuneFlag[0]);i++) {
		std::string tempString = arImmuneFlag[i];
		for (int32_t j=0; j<30 ; j++)
		{
			std::string tempString2 = arInputString[j];
			if (tempString2.compare(tempString)==0) 
				retValue = retValue + pow((float)2,(float)i);
			
			if (tempString2.compare("") == 0)
				break;
		}
	}
	delete []arInputString;

	return retValue;
}


#ifndef __DUMP_PROTO__
bool Set_Proto_Mob_Table(TMobTable *mobTable, cCsvTable &csvTable,std::map<int32_t,const char*> &nameMap)
{
	int32_t col = 0;
	str_to_number(mobTable->dwVnum, csvTable.AsStringByIndex(col++));
	strlcpy(mobTable->szName, csvTable.AsStringByIndex(col++), sizeof(mobTable->szName));

	auto it = nameMap.find(mobTable->dwVnum);
	if (it != nameMap.end()) 
	{
		const char* localeName = it->second;
		strlcpy(mobTable->szLocaleName, localeName, sizeof (mobTable->szLocaleName));
	} else 
	{
		strlcpy(mobTable->szLocaleName, mobTable->szName, sizeof (mobTable->szLocaleName));
	}

	int32_t rankValue = get_Mob_Rank_Value(csvTable.AsStringByIndex(col++));
	mobTable->bRank = rankValue;

	int32_t typeValue = get_Mob_Type_Value(csvTable.AsStringByIndex(col++));
	mobTable->bType = typeValue;

	int32_t battleTypeValue = get_Mob_BattleType_Value(csvTable.AsStringByIndex(col++));
	mobTable->bBattleType = battleTypeValue;

	str_to_number(mobTable->bLevel, csvTable.AsStringByIndex(col++));

	int32_t sizeValue = get_Mob_Size_Value(csvTable.AsStringByIndex(col++));
	mobTable->bSize = sizeValue;

	int32_t aiFlagValue = get_Mob_AIFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwAIFlag = aiFlagValue;

	col++;

	int32_t raceFlagValue = get_Mob_RaceFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwRaceFlag = raceFlagValue;

	int32_t immuneFlagValue = get_Mob_ImmuneFlag_Value(csvTable.AsStringByIndex(col++));
	mobTable->dwImmuneFlag = immuneFlagValue;

	str_to_number(mobTable->bEmpire, csvTable.AsStringByIndex(col++));

	strlcpy(mobTable->szFolder, csvTable.AsStringByIndex(col++), sizeof(mobTable->szFolder));

	str_to_number(mobTable->bOnClickType, csvTable.AsStringByIndex(col++));	

	str_to_number(mobTable->bStr, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bDex, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bCon, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bInt, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDamageRange[0], csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDamageRange[1], csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwMaxHP, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRegenCycle, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRegenPercent,	csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwGoldMin, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwGoldMax, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwExp,	csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wDef, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->sAttackSpeed, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->sMovingSpeed, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bAggresiveHPPct, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wAggressiveSight, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->wAttackRange, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->dwDropItemVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwResurrectionVnum, csvTable.AsStringByIndex(col++));

	for (auto& rEnchant : mobTable->cEnchants)
		str_to_number(rEnchant, csvTable.AsStringByIndex(col++));

	for (auto& rResist : mobTable->cResists)
		str_to_number(rResist, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->fDamMultiply, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwSummonVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->dwDrainSP, csvTable.AsStringByIndex(col++));

	++col;

	str_to_number(mobTable->dwPolymorphItemVnum, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->Skills[0].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[0].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[1].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[1].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[2].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[2].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[3].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[3].dwVnum, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[4].bLevel, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->Skills[4].dwVnum, csvTable.AsStringByIndex(col++));

	str_to_number(mobTable->bBerserkPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bStoneSkinPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bGodSpeedPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bDeathBlowPoint, csvTable.AsStringByIndex(col++));
	str_to_number(mobTable->bRevivePoint, csvTable.AsStringByIndex(col++));

	TraceLog("MOB #{} {} level: {} rank: {} empire: {}", mobTable->dwVnum, mobTable->szLocaleName, mobTable->bLevel, mobTable->bRank, mobTable->bEmpire);

	return true;
}

bool Set_Proto_Item_Table(TItemTable *itemTable, cCsvTable &csvTable,std::map<int32_t,const char*> &nameMap)
{
	int32_t col = 0;

	int32_t dataArray[33];
	for (int32_t i=0; i<sizeof(dataArray)/sizeof(dataArray[0]);i++) 
	{
		int32_t validCheck = 0;
		if (i==2) {
			dataArray[i] = get_Item_Type_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==3) {
			dataArray[i] = get_Item_SubType_Value(dataArray[i-1], csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==5) {
			dataArray[i] = get_Item_AntiFlag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==6) {
			dataArray[i] = get_Item_Flag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==7) {
			dataArray[i] = get_Item_WearFlag_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==8) {
			dataArray[i] = get_Item_Immune_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==14) {
			dataArray[i] = get_Item_LimitType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==16) {
			dataArray[i] = get_Item_LimitType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==18) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==20) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else if (i==22) {
			dataArray[i] = get_Item_ApplyType_Value(csvTable.AsStringByIndex(col));
			validCheck = dataArray[i];
		} else {
			str_to_number(dataArray[i], csvTable.AsStringByIndex(col));
		}

		if (validCheck == -1)
		{
			std::ostringstream dataStream;

			for (int32_t j = 0; j < i; ++j)
				dataStream << dataArray[j] << ",";

			SysLog("ItemProto Reading Failed : Invalid value. (index: {}, col: {}, value: {}) {} ~ {} Values: {}", i, col, csvTable.AsStringByIndex(col), 0, i, dataStream.str());

			exit(0);
		}
		
		col = col + 1;
	}

	{
		std::string s(csvTable.AsStringByIndex(0));
		int32_t pos = s.find("~");

		if (std::string::npos == pos)
		{
			itemTable->dwVnum = dataArray[0];
			itemTable->dwVnumRange = 0;
		}
		else
		{
			std::string s_start_vnum (s.substr(0, pos));
			std::string s_end_vnum (s.substr(pos +1));

			int32_t start_vnum = atoi(s_start_vnum.c_str());
			int32_t end_vnum = atoi(s_end_vnum.c_str());
			if (0 == start_vnum || (0 != end_vnum && end_vnum < start_vnum))
			{
				SysLog ("INVALID VNUM {}", s.c_str());
				return false;
			}
			itemTable->dwVnum = start_vnum;
			itemTable->dwVnumRange = end_vnum - start_vnum;
		}
	}

	strlcpy(itemTable->szName, csvTable.AsStringByIndex(1), sizeof(itemTable->szName));

	auto it = nameMap.find(itemTable->dwVnum);
	if (it != nameMap.end()) {
		const char* localeName = it->second;
		strlcpy(itemTable->szLocaleName, localeName, sizeof (itemTable->szLocaleName));
	} else {
		strlcpy(itemTable->szLocaleName, itemTable->szName, sizeof (itemTable->szLocaleName));
	}
	itemTable->bType = dataArray[2];
	itemTable->bSubType = dataArray[3];
	itemTable->bSize = dataArray[4];
	itemTable->dwAntiFlags = dataArray[5];
	itemTable->dwFlags = dataArray[6];
	itemTable->dwWearFlags = dataArray[7];
	itemTable->dwImmuneFlag = dataArray[8];
	itemTable->dwISellItemPrice = dataArray[9];
	itemTable->dwIBuyItemPrice = dataArray[10];
	itemTable->dwRefinedVnum = dataArray[11];
	itemTable->wRefineSet = dataArray[12];
	itemTable->bAlterToMagicItemPct = dataArray[13];
	itemTable->cLimitRealTimeFirstUseIndex = -1;
	itemTable->cLimitTimerBasedOnWearIndex = -1;

	int32_t i;

	for (i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
	{
		itemTable->aLimits[i].bType = dataArray[14+i*2];
		itemTable->aLimits[i].lValue = dataArray[15+i*2];

		if (ITEM::LIMIT_REAL_TIME_START_FIRST_USE == itemTable->aLimits[i].bType)
			itemTable->cLimitRealTimeFirstUseIndex = (char)i;

		if (ITEM::LIMIT_TIMER_BASED_ON_WEAR == itemTable->aLimits[i].bType)
			itemTable->cLimitTimerBasedOnWearIndex = (char)i;

	}

	for (i = 0; i < ITEM::APPLY_MAX_NUM; ++i)
	{
		itemTable->aApplies[i].bType = dataArray[18+i*2];
		itemTable->aApplies[i].lValue = dataArray[19+i*2];
	}

	for (i = 0; i < ITEM::VALUES_MAX_NUM; ++i)
		itemTable->alValues[i] = dataArray[24+i];

	itemTable->bGainSocketPct = dataArray[31];
	itemTable->sAddonType = dataArray[32];

	str_to_number(itemTable->bWeight, "0");
	
	return true;
}

#endif
