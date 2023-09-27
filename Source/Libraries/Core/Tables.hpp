#pragma once
#include <cstdint>
#include "Constants/Item.hpp"
#include "Constants/NonPlayer.hpp"
#include "Constants/Controls.hpp"

using TItemLimit = struct
{
	uint8_t        bType;
	int32_t        lValue;
};

using TItemApply = struct
{
	uint8_t        bType;
	int32_t        lValue;
};

using TItemTable = struct
{
	uint32_t		dwVnum;
	uint32_t		dwVnumRange;
	char			szName[ITEM::NAME_MAX_LEN + 1];
	char			szLocaleName[ITEM::NAME_MAX_LEN + 1];
	uint8_t			bType;
	uint8_t			bSubType;

	uint8_t			bWeight;
	uint8_t			bSize;

	uint32_t		dwAntiFlags;
	uint32_t		dwFlags;
	uint32_t		dwWearFlags;
	uint32_t		dwImmuneFlag;

	uint32_t		dwIBuyItemPrice;
	uint32_t		dwISellItemPrice;

	TItemLimit		aLimits[ITEM::LIMIT_SLOT_MAX_NUM];
	TItemApply		aApplies[ITEM::APPLY_MAX_NUM];
	int32_t			alValues[ITEM::VALUES_MAX_NUM];
	int32_t			alSockets[ITEM::SOCKET_MAX_NUM];
	uint32_t		dwRefinedVnum;
	uint16_t		wRefineSet;
	uint8_t			bAlterToMagicItemPct;
	uint8_t			bSpecular;
	uint8_t			bGainSocketPct;

	// Server side variables
	int16_t			sAddonType;
	int8_t			cLimitRealTimeFirstUseIndex;
	int8_t			cLimitTimerBasedOnWearIndex;

};

using TMobSkillLevel = struct
{
	uint32_t       dwVnum;
	uint8_t        bLevel;
};

using TMobTable = struct
{
	uint32_t		dwVnum;
	char			szName[CHARACTER_NAME_MAX_LEN + 1];
	char			szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

	uint8_t			bType;
	uint8_t			bRank;
	uint8_t			bBattleType;
	uint8_t			bLevel;
	uint8_t			bSize;

	uint32_t		dwGoldMin;
	uint32_t		dwGoldMax;
	uint32_t		dwExp;
	uint32_t		dwMaxHP;
	uint8_t			bRegenCycle;
	uint8_t			bRegenPercent;
	uint16_t		wDef;

	uint32_t		dwAIFlag;
	uint32_t		dwRaceFlag;
	uint32_t		dwImmuneFlag;

	uint8_t			bStr, bDex, bCon, bInt;
	uint32_t		dwDamageRange[2];

	int16_t			sAttackSpeed;
	int16_t			sMovingSpeed;
	uint8_t			bAggresiveHPPct;
	uint16_t		wAggressiveSight;
	uint16_t		wAttackRange;

	uint8_t			cEnchants[MOB::ENCHANTS_MAX_NUM];
	uint8_t			cResists[MOB::RESISTS_MAX_NUM];

	uint32_t		dwResurrectionVnum;
	uint32_t		dwDropItemVnum;

	uint8_t			bMountCapacity;
	uint8_t			bOnClickType;

	uint8_t			bEmpire;
	char			szFolder[64 + 1];
	float			fDamMultiply;
	uint32_t		dwSummonVnum;
	uint32_t		dwDrainSP;
	uint32_t		dwMonsterColor;
	uint32_t		dwPolymorphItemVnum;

	TMobSkillLevel	Skills[MOB::SKILL_MAX_NUM];

	uint8_t			bBerserkPoint;
	uint8_t			bStoneSkinPoint;
	uint8_t			bGodSpeedPoint;
	uint8_t			bDeathBlowPoint;
	uint8_t			bRevivePoint;
};