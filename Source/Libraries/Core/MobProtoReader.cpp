#include "ProtoReader.hpp"
#include "Constants/NonPlayer.hpp"
#include <Basic/Logging.hpp>

uint8_t MobRankStringToInteger(const std::string& c_stMobRank)
{
	const static EnumDictionary RankDict = {
		{ "PAWN", MOB::RANK_PAWN },
		{ "S_PAWN", MOB::RANK_S_PAWN },
		{ "KNIGHT", MOB::RANK_KNIGHT },
		{ "S_KNIGHT", MOB::RANK_S_KNIGHT },
		{ "BOSS", MOB::RANK_BOSS },
		{ "KING", MOB::RANK_KING },
	};

	auto it = RankDict.find(c_stMobRank);
	if (it != RankDict.end())
		return it->second;

	return MOB::RANK_PAWN;
}

uint8_t MobTypeStringToInteger(const std::string& c_stMobType)
{
	const static EnumDictionary TypeDict = {
		{ "MONSTER", CHAR_TYPE_MONSTER },
		{ "NPC", CHAR_TYPE_NPC },
		{ "STONE", CHAR_TYPE_STONE },
		{ "WARP", CHAR_TYPE_WARP },
		{ "DOOR", CHAR_TYPE_DOOR },
		{ "BUILDING", CHAR_TYPE_BUILDING },
		{ "PC", CHAR_TYPE_PC },
		{ "POLYMORPH_PC", CHAR_TYPE_POLYMORPH_PC },
		{ "HORSE", CHAR_TYPE_HORSE },
		{ "GOTO", CHAR_TYPE_GOTO },
	};

	auto it = TypeDict.find(c_stMobType);
	if (it != TypeDict.end())
		return it->second;

	return CHAR_TYPE_NPC;
}

uint8_t MobBattleTypeStringToInteger(const std::string& c_stMobBattleType)
{
	const static EnumDictionary BattleTypeDict = {
		{ "MELEE", MOB::BATTLE_TYPE_MELEE },
		{ "RANGE", MOB::BATTLE_TYPE_RANGE },
		{ "MAGIC", MOB::BATTLE_TYPE_MAGIC },
		{ "SPECIAL", MOB::BATTLE_TYPE_SPECIAL },
		{ "POWER", MOB::BATTLE_TYPE_POWER },
		{ "TANKER", MOB::BATTLE_TYPE_TANKER },
		{ "SUPER_POWER", MOB::BATTLE_TYPE_SUPER_POWER },
		{ "SUPER_TANKER", MOB::BATTLE_TYPE_SUPER_TANKER },
	};

	auto it = BattleTypeDict.find(c_stMobBattleType);
	if (it != BattleTypeDict.end())
		return it->second;

	return MOB::BATTLE_TYPE_MELEE;
}

uint8_t MobSizeStringToInteger(const std::string& c_stMobSize)
{
	const static EnumDictionary SizeDict = {
		{ "SMALL", MOB::SIZE_SMALL },
		{ "MEDIUM", MOB::SIZE_MEDIUM },
		{ "BIG", MOB::SIZE_BIG },
	};

	auto it = SizeDict.find(c_stMobSize);
	if (it != SizeDict.end())
		return it->second;

	return MOB::SIZE_RESERVED;
}

uint32_t MobAIFlagStringToInteger(const std::string& c_stMobAIFlag)
{
	uint32_t dwAIFlag = 0;
	const static EnumDictionary AIFlagDict = {
		{ "AGGR", MOB::AIFLAG_AGGRESSIVE },
		{ "NOMOVE", MOB::AIFLAG_NOMOVE },
		{ "COWARD", MOB::AIFLAG_COWARD },
		{ "NOATTSHINSU", MOB::AIFLAG_NOATTACKSHINSU },
		{ "NOATTCHUNJO", MOB::AIFLAG_NOATTACKJINNO },
		{ "NOATTJINNO", MOB::AIFLAG_NOATTACKCHUNJO },
		{ "ATTMOB", MOB::AIFLAG_ATTACKMOB },
		{ "BERSERK", MOB::AIFLAG_BERSERK },
		{ "STONESKIN", MOB::AIFLAG_STONESKIN },
		{ "GODSPEED", MOB::AIFLAG_GODSPEED },
		{ "DEATHBLOW", MOB::AIFLAG_DEATHBLOW },
		{ "REVIVE", MOB::AIFLAG_REVIVE },
	};

	for (const auto& [stAIFlagFlag, dwValue] : AIFlagDict)
	{
		if (c_stMobAIFlag.find(stAIFlagFlag) != std::string::npos)
			dwAIFlag += dwValue;
	}

	return dwAIFlag;
}

uint32_t MobRaceFlagStringToInteger(const std::string& c_stMobRaceFlag)
{
	uint32_t dwRaceFlag = 0;
	const static EnumDictionary RaceFlagDict = {
		{ "ANIMAL", MOB::RACE_FLAG_ANIMAL },
		{ "UNDEAD", MOB::RACE_FLAG_UNDEAD },
		{ "DEVIL", MOB::RACE_FLAG_DEVIL },
		{ "HUMAN", MOB::RACE_FLAG_HUMAN },
		{ "ORC", MOB::RACE_FLAG_ORC },
		{ "MILGYO", MOB::RACE_FLAG_MILGYO },
		{ "INSECT", MOB::RACE_FLAG_INSECT },
		{ "FIRE", MOB::RACE_FLAG_FIRE },
		{ "ICE", MOB::RACE_FLAG_ICE },
		{ "DESERT", MOB::RACE_FLAG_DESERT },
		{ "TREE", MOB::RACE_FLAG_TREE },
		{ "ATT_ELEC", MOB::RACE_FLAG_ATT_ELEC },
		{ "ATT_FIRE", MOB::RACE_FLAG_ATT_FIRE },
		{ "ATT_ICE", MOB::RACE_FLAG_ATT_ICE },
		{ "ATT_WIND", MOB::RACE_FLAG_ATT_WIND },
		{ "ATT_EARTH", MOB::RACE_FLAG_ATT_EARTH },
		{ "ATT_DARK", MOB::RACE_FLAG_ATT_DARK },
	};

	for (const auto& [stRaceFlag, dwValue] : RaceFlagDict)
	{
		if (c_stMobRaceFlag.find(stRaceFlag) != std::string::npos)
			dwRaceFlag += dwValue;
	}

	return dwRaceFlag;
}

uint32_t MobImmuneFlagStringToInteger(const std::string& c_stMobImmuneFlag)
{
	uint32_t dwImmuneFlag = 0;
	const static EnumDictionary RaceFlagDict = {
		{ "STUN", MOB::IMMUNE_STUN },
		{ "SLOW", MOB::IMMUNE_SLOW },
		{ "FALL", MOB::IMMUNE_FALL },
		{ "CURSE", MOB::IMMUNE_CURSE },
		{ "POISON", MOB::IMMUNE_POISON },
		{ "TERROR", MOB::IMMUNE_TERROR },
		{ "REFLECT", MOB::IMMUNE_REFLECT },
	};

	for (const auto& [stImmuneFlag, dwValue] : RaceFlagDict)
	{
		if (c_stMobImmuneFlag.find(stImmuneFlag) != std::string::npos)
			dwImmuneFlag += dwValue;
	}

	return dwImmuneFlag;
}

bool ReadMobProto(std::vector<std::string>& tokens, TMobTable& table)
{
	if (tokens.size() < MOB_PROTO_MAX_TOKEN)
		return false;

	try
	{
		table.dwVnum = std::stoi(tokens[EMobProtoStructure::MOB_VNUM]);
		memcpy(&table.szName, &tokens[EMobProtoStructure::MOB_NAME], CHARACTER_NAME_MAX_LEN + 1);
		memcpy(&table.szLocaleName, &tokens[EMobProtoStructure::MOB_NAME], CHARACTER_NAME_MAX_LEN + 1);

		table.bType = MobTypeStringToInteger(tokens[EMobProtoStructure::MOB_TYPE]);
		table.bRank = MobRankStringToInteger(tokens[EMobProtoStructure:: MOB_RANK]);

		table.bBattleType = MobBattleTypeStringToInteger(tokens[EMobProtoStructure::BATTLE_TYPE]);
		table.bLevel = std::stoi(tokens[EMobProtoStructure::LEVEL]);

		table.bSize = MobSizeStringToInteger(tokens[EMobProtoStructure::MOB_SIZE]);

		table.dwGoldMin = std::stoi(tokens[EMobProtoStructure::MOB_GOLD_MIN]);
		table.dwGoldMax = std::stoi(tokens[EMobProtoStructure::MOB_GOLD_MAX]);
		table.dwExp = std::stoi(tokens[EMobProtoStructure::EXP]);

		table.dwMaxHP = std::stoi(tokens[EMobProtoStructure::MAX_HP]);
		table.bRegenCycle = std::stoi(tokens[EMobProtoStructure::REGEN_CYCLE]);
		table.bRegenPercent = std::stoi(tokens[EMobProtoStructure::REGEN_PERCENT]);
		table.wDef = std::stoi(tokens[EMobProtoStructure::DEF]);

		table.dwAIFlag = MobAIFlagStringToInteger(tokens[EMobProtoStructure::AI_FLAG]);
		table.dwRaceFlag = MobRaceFlagStringToInteger(tokens[EMobProtoStructure::RACE_FLAG]);
		table.dwImmuneFlag = MobImmuneFlagStringToInteger(tokens[EMobProtoStructure::IMMUNE_FLAG]);

		table.bStr = std::stoi(tokens[EMobProtoStructure::ST]);
		table.bDex = std::stoi(tokens[EMobProtoStructure::DX]);
		table.bCon = std::stoi(tokens[EMobProtoStructure::HT]);
		table.bInt = std::stoi(tokens[EMobProtoStructure::IQ]);

		table.dwDamageRange[0] = std::stoi(tokens[EMobProtoStructure::DAMAGE_MIN]);
		table.dwDamageRange[1] = std::stoi(tokens[EMobProtoStructure::DAMAGE_MAX]);

		table.sAttackSpeed = std::stoi(tokens[EMobProtoStructure::ATTACK_SPEED]);
		table.sMovingSpeed = std::stoi(tokens[EMobProtoStructure::MOVE_SPEED]);
		table.bAggresiveHPPct = std::stoi(tokens[EMobProtoStructure::AGGRESSIVE_HP_PCT]);
		table.wAggressiveSight = std::stoi(tokens[EMobProtoStructure::AGGRESSIVE_SIGHT]);
		table.wAttackRange = std::stoi(tokens[EMobProtoStructure::ATTACK_RANGE]);

		for (uint8_t i = 0; i < MOB::ENCHANTS_MAX_NUM; ++i)
			table.cEnchants[i] = std::stoi(tokens[EMobProtoStructure::ENCHANT_CURSE + i]);

		for (uint8_t i = 0; i < MOB::RESISTS_MAX_NUM; ++i)
			table.cResists[i] = std::stoi(tokens[EMobProtoStructure::RESIST_SWORD + i]);

		table.dwResurrectionVnum = std::stoi(tokens[EMobProtoStructure::ATTACK_RANGE]);
		table.dwDropItemVnum = std::stoi(tokens[EMobProtoStructure::ATTACK_RANGE]);
		table.bMountCapacity = std::stoi(tokens[EMobProtoStructure::MOUNT_CAPACITY]);
		table.bOnClickType = std::stoi(tokens[EMobProtoStructure::ON_CLICK]);
		table.bEmpire = std::stoi(tokens[EMobProtoStructure::EMPIRE]);

		memcpy(&table.szFolder, tokens[EMobProtoStructure::FOLDER].c_str(), sizeof(table.szFolder));

		table.fDamMultiply = std::stof(tokens[EMobProtoStructure::DAM_MULTIPLY]);
		table.dwSummonVnum = std::stoi(tokens[EMobProtoStructure::SUMMON]);
		table.dwDrainSP = std::stoi(tokens[EMobProtoStructure::DRAIN_SP]);
		table.dwMonsterColor = std::stoi(tokens[EMobProtoStructure::MOB_COLOR]);
		table.dwPolymorphItemVnum = std::stoi(tokens[EMobProtoStructure::POLYMORPH_ITEM]);

		for (uint8_t i = 0; i < MOB::SKILL_MAX_NUM; ++i)
		{
			table.Skills[i].bLevel = std::stoi(tokens[EMobProtoStructure::SKILL_LEVEL0 + i * 2]);
			table.Skills[i].dwVnum = std::stoi(tokens[EMobProtoStructure::SKILL_VNUM0 + i * 2]);
		}

		table.bBerserkPoint = std::stoi(tokens[EMobProtoStructure::SP_BERSERK]);
		table.bStoneSkinPoint = std::stoi(tokens[EMobProtoStructure::SP_STONESKIN]);
		table.bGodSpeedPoint = std::stoi(tokens[EMobProtoStructure::SP_GODSPEED]);
		table.bDeathBlowPoint = std::stoi(tokens[EMobProtoStructure::SP_DEATHBLOW]);
		table.bRevivePoint = std::stoi(tokens[EMobProtoStructure::SP_REVIVE]);
	}
	catch (const std::exception& e)
	{
		SysLog("Could not tokenize mob proto: {0}", e.what());
		return false;
	}

	return true;
}