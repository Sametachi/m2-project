// NonPlayerCharacterInfo.h: interface for the NonPlayerCharacterInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NONPLAYERCHARACTERINFO_H__356E26F9_0EDD_4A2F_9E4C_CFC032B88C3A__INCLUDED_)
#define AFX_NONPLAYERCHARACTERINFO_H__356E26F9_0EDD_4A2F_9E4C_CFC032B88C3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ENABLE_PROTOSTRUCT_AUTODETECT
#define CPythonNonPlayer CNonPlayerCharacterInfo

class CNonPlayerCharacterInfo : public CSingleton<CNonPlayerCharacterInfo>
{
public:
	enum
	{
		CHARACTER_NAME_MAX_LEN = 24,
	};

	enum EInstanceType
	{
		INSTANCE_TYPE_ENEMY,
		INSTANCE_TYPE_NPC,
		INSTANCE_TYPE_PLAYER,

		INSTANCE_TYPE_MAX_NUM,
	};

	enum EMobEnchants
	{
		MOB_ENCHANT_CURSE,
		MOB_ENCHANT_SLOW,
		MOB_ENCHANT_POISON,
		MOB_ENCHANT_STUN,
		MOB_ENCHANT_CRITICAL,
		MOB_ENCHANT_PENETRATE,
		MOB_ENCHANTS_MAX_NUM
	};
	enum EMobResists
	{
		MOB_RESIST_SWORD,
		MOB_RESIST_TWOHAND,
		MOB_RESIST_DAGGER,
		MOB_RESIST_BELL,
		MOB_RESIST_FAN,
		MOB_RESIST_BOW,
		MOB_RESIST_FIRE,
		MOB_RESIST_ELECT,
		MOB_RESIST_MAGIC,
		MOB_RESIST_WIND,
		MOB_RESIST_POISON,
		MOB_RESISTS_MAX_NUM
	};

	enum EMobMaxNum
	{
		MOB_ATTRIBUTE_MAX_NUM = 12,
		MOB_SKILL_MAX_NUM = 5,
	};

#pragma pack(push)
#pragma pack(1)
	typedef struct SMobSkillLevel
	{
		uint32_t       dwVnum;
		uint8_t        bLevel;
	} TMobSkillLevel;

	typedef struct SMobTable_r235
	{
		enum EMobMaxNum
		{
			MOB_ATTRIBUTE_MAX_NUM = 12,
			MOB_SKILL_MAX_NUM = 1,//r1
		};

		uint32_t       dwVnum;
		char        szName[CHARACTER_NAME_MAX_LEN + 1];
		char        szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

		uint8_t        bType;                  // Monster, NPC
		uint8_t        bRank;                  // PAWN, KNIGHT, KING
		uint8_t        bBattleType;            // MELEE, etc..
		uint8_t        bLevel;                 // Level
		uint8_t        bSize;

		uint32_t       dwGoldMin;
		uint32_t       dwGoldMax;
		uint32_t       dwExp;
		uint32_t       dwMaxHP;
		uint8_t        bRegenCycle;
		uint8_t        bRegenPercent;
		uint16_t        wDef;

		uint32_t       dwAIFlag;
		uint32_t       dwRaceFlag;
		uint32_t       dwImmuneFlag;

		uint8_t        bStr, bDex, bCon, bInt;
		uint32_t       dwDamageRange[2];

		int16_t       sAttackSpeed;
		int16_t       sMovingSpeed;
		uint8_t        bAggresiveHPPct;
		uint16_t        wAggressiveSight;
		uint16_t        wAttackRange;

		char        cEnchants[MOB_ENCHANTS_MAX_NUM];
		char        cResists[MOB_RESISTS_MAX_NUM];

		uint32_t       dwResurrectionVnum;
		uint32_t       dwDropItemVnum;

		uint8_t        bMountCapacity;
		uint8_t        bOnClickType;

		uint8_t        bEmpire;
		char        szFolder[64 + 1];
		float       fDamMultiply;
		uint32_t       dwSummonVnum;
		uint32_t       dwDrainSP;
		uint32_t		dwMonsterColor;
		uint32_t       dwPolymorphItemVnum;

		TMobSkillLevel	Skills[SMobTable_r235::MOB_SKILL_MAX_NUM];

		uint8_t		bBerserkPoint;
		uint8_t		bStoneSkinPoint;
		uint8_t		bGodSpeedPoint;
		uint8_t		bDeathBlowPoint;
		uint8_t		bRevivePoint;
	} TMobTable_r235;

	typedef struct SMobTable_r255
	{
		uint32_t       dwVnum;
		char        szName[CHARACTER_NAME_MAX_LEN + 1];
		char        szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

		uint8_t        bType;                  // Monster, NPC
		uint8_t        bRank;                  // PAWN, KNIGHT, KING
		uint8_t        bBattleType;            // MELEE, etc..
		uint8_t        bLevel;                 // Level
		uint8_t        bSize;

		uint32_t       dwGoldMin;
		uint32_t       dwGoldMax;
		uint32_t       dwExp;
		uint32_t       dwMaxHP;
		uint8_t        bRegenCycle;
		uint8_t        bRegenPercent;
		uint16_t        wDef;

		uint32_t       dwAIFlag;
		uint32_t       dwRaceFlag;
		uint32_t       dwImmuneFlag;

		uint8_t        bStr, bDex, bCon, bInt;
		uint32_t       dwDamageRange[2];

		int16_t       sAttackSpeed;
		int16_t       sMovingSpeed;
		uint8_t        bAggresiveHPPct;
		uint16_t        wAggressiveSight;
		uint16_t        wAttackRange;

		char        cEnchants[MOB_ENCHANTS_MAX_NUM];
		char        cResists[MOB_RESISTS_MAX_NUM];

		uint32_t       dwResurrectionVnum;
		uint32_t       dwDropItemVnum;

		uint8_t        bMountCapacity;
		uint8_t        bOnClickType;

		uint8_t        bEmpire;
		char        szFolder[64 + 1];
		float       fDamMultiply;
		uint32_t       dwSummonVnum;
		uint32_t       dwDrainSP;
		uint32_t		dwMonsterColor;
		uint32_t       dwPolymorphItemVnum;

		TMobSkillLevel	Skills[MOB_SKILL_MAX_NUM];

		uint8_t		bBerserkPoint;
		uint8_t		bStoneSkinPoint;
		uint8_t		bGodSpeedPoint;
		uint8_t		bDeathBlowPoint;
		uint8_t		bRevivePoint;
	} TMobTable_r255;

	typedef struct SMobTable_r256
	{
		enum EMobResists_r3
		{
			MOB_RESIST_SWORD,
			MOB_RESIST_TWOHAND,
			MOB_RESIST_DAGGER,
			MOB_RESIST_BELL,
			MOB_RESIST_FAN,
			MOB_RESIST_BOW,
			MOB_RESIST_FIRE,
			MOB_RESIST_ELECT,
			MOB_RESIST_MAGIC,
			MOB_RESIST_WIND,
			MOB_RESIST_POISON,
			MOB_RESIST_BLEEDING,//r3
			MOB_RESISTS_MAX_NUM
		};

		uint32_t       dwVnum;
		char        szName[CHARACTER_NAME_MAX_LEN + 1];
		char        szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

		uint8_t        bType;                  // Monster, NPC
		uint8_t        bRank;                  // PAWN, KNIGHT, KING
		uint8_t        bBattleType;            // MELEE, etc..
		uint8_t        bLevel;                 // Level
		uint8_t        bSize;

		uint32_t       dwGoldMin;
		uint32_t       dwGoldMax;
		uint32_t       dwExp;
		uint32_t       dwMaxHP;
		uint8_t        bRegenCycle;
		uint8_t        bRegenPercent;
		uint16_t        wDef;

		uint32_t       dwAIFlag;
		uint32_t       dwRaceFlag;
		uint32_t       dwImmuneFlag;

		uint8_t        bStr, bDex, bCon, bInt;
		uint32_t       dwDamageRange[2];

		int16_t       sAttackSpeed;
		int16_t       sMovingSpeed;
		uint8_t        bAggresiveHPPct;
		uint16_t        wAggressiveSight;
		uint16_t        wAttackRange;

		char        cEnchants[MOB_ENCHANTS_MAX_NUM];
		char        cResists[SMobTable_r256::MOB_RESISTS_MAX_NUM];

		uint32_t       dwResurrectionVnum;
		uint32_t       dwDropItemVnum;

		uint8_t        bMountCapacity;
		uint8_t        bOnClickType;

		uint8_t        bEmpire;
		char        szFolder[64 + 1];
		float       fDamMultiply;
		uint32_t       dwSummonVnum;
		uint32_t       dwDrainSP;
		uint32_t		dwMonsterColor;
		uint32_t       dwPolymorphItemVnum;

		TMobSkillLevel	Skills[MOB_SKILL_MAX_NUM];

		uint8_t		bBerserkPoint;
		uint8_t		bStoneSkinPoint;
		uint8_t		bGodSpeedPoint;
		uint8_t		bDeathBlowPoint;
		uint8_t		bRevivePoint;
	} TMobTable_r256;

	typedef struct SMobTable_r262
	{
		enum EMobResists_r4
		{
			MOB_RESIST_SWORD,
			MOB_RESIST_TWOHAND,
			MOB_RESIST_DAGGER,
			MOB_RESIST_BELL,
			MOB_RESIST_FAN,
			MOB_RESIST_BOW,
			MOB_RESIST_CLAW,//r4
			MOB_RESIST_FIRE,
			MOB_RESIST_ELECT,
			MOB_RESIST_MAGIC,
			MOB_RESIST_WIND,
			MOB_RESIST_POISON,
			MOB_RESIST_BLEEDING,//r3
			MOB_RESISTS_MAX_NUM
		};

		uint32_t       dwVnum;
		char        szName[CHARACTER_NAME_MAX_LEN + 1];
		char        szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

		uint8_t        bType;                  // Monster, NPC
		uint8_t        bRank;                  // PAWN, KNIGHT, KING
		uint8_t        bBattleType;            // MELEE, etc..
		uint8_t        bLevel;                 // Level
		uint8_t		bLvlPct;
		uint8_t        bSize;//r4

		uint32_t       dwGoldMin;
		uint32_t       dwGoldMax;
		uint32_t       dwExp;
		uint32_t       dwMaxHP;
		uint8_t        bRegenCycle;
		uint8_t        bRegenPercent;
		uint16_t        wDef;

		uint32_t       dwAIFlag;
		uint32_t       dwRaceFlag;
		uint32_t       dwImmuneFlag;

		uint8_t        bStr, bDex, bCon, bInt;
		uint32_t       dwDamageRange[2];

		int16_t       sAttackSpeed;
		int16_t       sMovingSpeed;
		uint8_t        bAggresiveHPPct;
		uint16_t        wAggressiveSight;
		uint16_t        wAttackRange;

		char        cEnchants[MOB_ENCHANTS_MAX_NUM];
		char        cResists[SMobTable_r262::MOB_RESISTS_MAX_NUM];

		uint32_t       dwResurrectionVnum;
		uint32_t       dwDropItemVnum;

		uint8_t        bMountCapacity;
		uint8_t        bOnClickType;

		uint8_t        bEmpire;
		char        szFolder[64 + 1];
		float       fDamMultiply;
		uint32_t       dwSummonVnum;
		uint32_t       dwDrainSP;
		uint32_t		dwMonsterColor;
		uint32_t       dwPolymorphItemVnum;

		TMobSkillLevel	Skills[MOB_SKILL_MAX_NUM];

		uint8_t		bBerserkPoint;
		uint8_t		bStoneSkinPoint;
		uint8_t		bGodSpeedPoint;
		uint8_t		bDeathBlowPoint;
		uint8_t		bRevivePoint;

		uint32_t		dwHealerPoint;//r4
	} TMobTable_r262;

	typedef struct SMobTable_r263
	{
		enum EMobResists_r5
		{
			MOB_RESIST_SWORD,
			MOB_RESIST_TWOHAND,
			MOB_RESIST_DAGGER,
			MOB_RESIST_BELL,
			MOB_RESIST_FAN,
			MOB_RESIST_BOW,
			MOB_RESIST_CLAW,//r4
			MOB_RESIST_FIRE,
			MOB_RESIST_ELECT,
			MOB_RESIST_MAGIC,
			MOB_RESIST_WIND,
			MOB_RESIST_POISON,
			MOB_RESIST_BLEEDING,//r3
			MOB_RESISTS_MAX_NUM
		};

		uint32_t       dwVnum;
		char        szName[CHARACTER_NAME_MAX_LEN + 1];
		char        szLocaleName[CHARACTER_NAME_MAX_LEN + 1];

		uint8_t        bType;                  // Monster, NPC
		uint8_t        bRank;                  // PAWN, KNIGHT, KING
		uint8_t        bBattleType;            // MELEE, etc..
		uint8_t        bLevel;                 // Level
		uint8_t		bLvlPct;
		uint8_t        bSize;//r4

		uint32_t       dwGoldMin;
		uint32_t       dwGoldMax;
		uint32_t       dwExp;
		uint32_t       dwMaxHP;
		uint8_t        bRegenCycle;
		uint8_t        bRegenPercent;
		uint16_t        wDef;

		uint32_t       dwAIFlag;
		uint32_t       dwRaceFlag;
		uint32_t       dwImmuneFlag;

		uint8_t        bStr, bDex, bCon, bInt;
		uint32_t       dwDamageRange[2];

		int16_t       sAttackSpeed;
		int16_t       sMovingSpeed;
		uint8_t        bAggresiveHPPct;
		uint16_t        wAggressiveSight;
		uint16_t        wAttackRange;

		char        cEnchants[MOB_ENCHANTS_MAX_NUM];
		char        cResists[SMobTable_r263::MOB_RESISTS_MAX_NUM];

		uint32_t       dwResurrectionVnum;
		uint32_t       dwDropItemVnum;

		uint8_t        bMountCapacity;
		uint8_t        bOnClickType;

		uint8_t        bEmpire;
		char        szFolder[64 + 1];
		float       fDamMultiply;
		uint32_t       dwSummonVnum;
		uint32_t       dwDrainSP;
		uint32_t		dwMonsterColor;
		uint32_t       dwPolymorphItemVnum;

		TMobSkillLevel	Skills[MOB_SKILL_MAX_NUM];

		uint8_t		bBerserkPoint;
		uint8_t		bStoneSkinPoint;
		uint8_t		bGodSpeedPoint;
		uint8_t		bDeathBlowPoint;
		uint8_t		bRevivePoint;

		uint32_t		dwHealerPoint;//r5
		uint8_t		bUnk263;//r5
	} TMobTable_r263; //brazilian only 2016/08

	typedef TMobTable_r255 SMobTable, TMobTable;

#ifdef ENABLE_PROTOSTRUCT_AUTODETECT
	typedef struct SMobTableAll
	{
		static bool IsValidStruct(uint32_t structSize)
		{
			switch (structSize)
			{
				case sizeof(TMobTable_r235) :
					case sizeof(TMobTable_r255) :
					case sizeof(TMobTable_r256) :
					case sizeof(TMobTable_r262) :
					case sizeof(TMobTable_r263) :
					return true;
					break;
			}
			return false;
		}

		static void Process(void* obj, uint32_t structSize, uint32_t i, CPythonNonPlayer::TMobTable& t)
		{
#define MTABLE_COPY_STR(x) strncpy_s(t.##x##, sizeof(t.##x##), r.##x##, _TRUNCATE)
#define MTABLE_COPY_INT(x) t.##x## = r.##x
#define MTABLE_COPY_FLT(x) t.##x## = r.##x
#define MTABLE_COUNT(x) _countof(t.##x##)
#define MTABLE_PROCESS(len)\
					CPythonNonPlayer::TMobTable_r##len## & r = *((CPythonNonPlayer::TMobTable_r##len## *) obj + i);\
					MTABLE_COPY_INT(dwVnum);\
					MTABLE_COPY_STR(szName);\
					MTABLE_COPY_STR(szLocaleName);\
					MTABLE_COPY_INT(bType);\
					MTABLE_COPY_INT(bRank);\
					MTABLE_COPY_INT(bBattleType);\
					MTABLE_COPY_INT(bLevel);\
					MTABLE_COPY_INT(bSize);\
					MTABLE_COPY_INT(dwGoldMin);\
					MTABLE_COPY_INT(dwGoldMax);\
					MTABLE_COPY_INT(dwExp);\
					MTABLE_COPY_INT(dwMaxHP);\
					MTABLE_COPY_INT(bRegenCycle);\
					MTABLE_COPY_INT(bRegenPercent);\
					MTABLE_COPY_INT(wDef);\
					MTABLE_COPY_INT(dwAIFlag);\
					MTABLE_COPY_INT(dwRaceFlag);\
					MTABLE_COPY_INT(dwImmuneFlag);\
					MTABLE_COPY_INT(bStr);\
					MTABLE_COPY_INT(bDex);\
					MTABLE_COPY_INT(bCon);\
					MTABLE_COPY_INT(bInt);\
					for (size_t i=0; i<MTABLE_COUNT(dwDamageRange); ++i)\
					{\
						MTABLE_COPY_INT(dwDamageRange[i]);\
					}\
					MTABLE_COPY_INT(sAttackSpeed);\
					MTABLE_COPY_INT(sMovingSpeed);\
					MTABLE_COPY_INT(bAggresiveHPPct);\
					MTABLE_COPY_INT(wAggressiveSight);\
					MTABLE_COPY_INT(wAttackRange);\
					for (size_t i=0; i<MTABLE_COUNT(cEnchants); ++i)\
					{\
						MTABLE_COPY_INT(cEnchants[i]);\
					}\
					for (size_t i=0; i<MTABLE_COUNT(cResists); ++i)\
					{\
						MTABLE_COPY_INT(cResists[i]);\
					}\
					MTABLE_COPY_INT(dwResurrectionVnum);\
					MTABLE_COPY_INT(dwDropItemVnum);\
					MTABLE_COPY_INT(bMountCapacity);\
					MTABLE_COPY_INT(bOnClickType);\
					MTABLE_COPY_INT(bEmpire);\
					MTABLE_COPY_STR(szFolder);\
					MTABLE_COPY_FLT(fDamMultiply);\
					MTABLE_COPY_INT(dwSummonVnum);\
					MTABLE_COPY_INT(dwDrainSP);\
					MTABLE_COPY_INT(dwMonsterColor);\
					MTABLE_COPY_INT(dwPolymorphItemVnum);\
					for (size_t i=0; i<MTABLE_COUNT(Skills); ++i)\
					{\
						MTABLE_COPY_INT(Skills[i].dwVnum);\
						MTABLE_COPY_INT(Skills[i].bLevel);\
					}\
					MTABLE_COPY_INT(bBerserkPoint);\
					MTABLE_COPY_INT(bStoneSkinPoint);\
					MTABLE_COPY_INT(bGodSpeedPoint);\
					MTABLE_COPY_INT(bDeathBlowPoint);\
					MTABLE_COPY_INT(bRevivePoint);

			switch (structSize)
			{
				case sizeof(TMobTable_r235) :
				{
					MTABLE_PROCESS(235);
				}
				break;
				case sizeof(TMobTable_r255) :
				{
					MTABLE_PROCESS(255);
				}
				break;
				case sizeof(TMobTable_r256) :
				{
					MTABLE_PROCESS(256);
				}
				break;
				case sizeof(TMobTable_r262) :
				{
					MTABLE_PROCESS(262);
				}
				break;
				case sizeof(TMobTable_r263) :
				{
					MTABLE_PROCESS(263);
				}
				break;
			}
		}
	} TMobTableAll;
#endif //ENABLE_PROTOSTRUCT_AUTODETECT

#pragma pack(pop)


	typedef struct SNPCGroup
	{
		char				m_szName[CHARACTER_NAME_MAX_LEN + 1];
		uint32_t				m_dwLeaderID;
		std::vector<uint32_t>	m_FollowerIDVector;
	} TNPCGroup;


public:
	CNonPlayerCharacterInfo();
	~CNonPlayerCharacterInfo();

	void Clear();
	void Destroy();

	bool				LoadNonPlayerData(const char * c_szFileName);
	bool				LoadNPCGroupData(const char * c_szFileName);

	const char *		GetNameByVID(uint32_t dwVID);
	uint8_t				GetRankByVID(uint32_t dwVID);
	int32_t					GetLevelByVID(uint32_t dwVID);

	uint32_t				GetVIDByName(std::string strName);
	const TMobTable *	GetTable(uint32_t dwVnum);
	uint8_t				GetInstanceType(uint32_t dwVnum);

	uint32_t				GetNonPlayerCount();
	const char *		GetNameByMapIndex(uint32_t dwMapIndex);
	uint8_t				GetRankByMapIndex(uint32_t dwMapIndex);
	uint32_t				GetVIDByMapIndex(uint32_t dwMapIndex);
	int32_t					GetLevelByMapIndex(uint32_t dwMapIndex);

	// Group
	const TNPCGroup *	GetGroup(uint32_t dwGroupID);
	uint32_t				GetNPCGroupCount();
	const char *		GetNPCGroupNameByMapIndex(uint32_t dwMapIndex);
	uint32_t				GetNPCGroupIDByMapIndex(uint32_t dwMapIndex);
	uint32_t				GetNPCGroupLeaderVIDByMapIndex(uint32_t dwMapIndex);
	const char *		GetNPCGroupLeaderNameByMapIndex(uint32_t dwMapIndex);
	std::vector<uint32_t>	GetNPCGroupFollowerVIDsByMapIndex(uint32_t dwMapIndex);
	uint32_t				GetNPCGroupFollowerCountByMapIndex(uint32_t dwMapIndex);

	uint32_t				GetGroupIDByGroupName(std::string strGroupName);

	const char *		GetNPCGroupNameByGroupID(uint32_t dwGroupID);
	uint32_t				GetNPCGroupLeaderVIDByGroupID(uint32_t dwGroupID);
	const char *		GetNPCGroupLeaderNameByGroupID(uint32_t dwGroupID);
	std::vector<uint32_t>	GetNPCGroupFollowerVIDsByGroupID(uint32_t dwGroupID);
	uint32_t				GetNPCGroupFollowerCountByGroupID(uint32_t dwGroupID);

private:

	typedef std::map<uint32_t, TMobTable *> TNonPlayerDataMap;
	TNonPlayerDataMap					m_NonPlayerDataMap;
	TNonPlayerDataMap::iterator			m_NonPlayerDataMapIterator;

	typedef std::map<uint32_t, TNPCGroup *> TNPCGroupDataMap;
	TNPCGroupDataMap					m_NPCGroupDataMap;
	TNPCGroupDataMap::iterator			m_NPCGroupDataMapIterator;

};

#endif // !defined(AFX_NONPLAYERCHARACTERINFO_H__356E26F9_0EDD_4A2F_9E4C_CFC032B88C3A__INCLUDED_)
