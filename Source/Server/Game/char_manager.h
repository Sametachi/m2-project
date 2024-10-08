#pragma once

#include <Common/stl.h>
#include <Common/length.h>

#include "vid.h"

class CDungeon;
class CHARACTER;
class CharacterVectorInteractor;

class CHARACTER_MANAGER : public Singleton<CHARACTER_MANAGER>
{
	public:
		typedef TR1_NS::unordered_map<std::string, LPCHARACTER> NAME_MAP;

		CHARACTER_MANAGER();
		virtual ~CHARACTER_MANAGER();

		void                    Destroy();

		void			GracefulShutdown();

		uint32_t			AllocVID();

		LPCHARACTER             CreateCharacter(const char* name, uint32_t dwPID = 0);
		void					DestroyCharacter(LPCHARACTER ch);

		void			Update(int32_t iPulse);

		LPCHARACTER		SpawnMob(uint32_t dwVnum, int32_t lMapIndex, int32_t x, int32_t y, int32_t z, bool bSpawnMotion = false, int32_t iRot = -1, bool bShow = true);
		LPCHARACTER		SpawnMobRange(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, bool bIsException=false, bool bSpawnMotion = false , bool bAggressive = false);
		LPCHARACTER		SpawnGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, LPREGEN pRegen = nullptr, bool bAggressive_ = false, LPDUNGEON pDungeon = nullptr);
		bool			SpawnGroupGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, LPREGEN pRegen = nullptr, bool bAggressive_ = false, LPDUNGEON pDungeon = nullptr);
		bool			SpawnMoveGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, int32_t tx, int32_t ty, LPREGEN pRegen = nullptr, bool bAggressive_ = false);
		LPCHARACTER		SpawnMobRandomPosition(uint32_t dwVnum, int32_t lMapIndex);

		void			SelectStone(LPCHARACTER pChrStone);

		NAME_MAP &		GetPCMap() { return m_map_pPCChr; }

		LPCHARACTER		Find(uint32_t dwVID);
		LPCHARACTER		Find(const VID & vid);
		LPCHARACTER		FindPC(const char* name);
		LPCHARACTER		FindByPID(uint32_t dwPID);

		bool			AddToStateList(LPCHARACTER ch);
		void			RemoveFromStateList(LPCHARACTER ch);

		void                    DelayedSave(LPCHARACTER ch);
		bool                    FlushDelayedSave(LPCHARACTER ch);
		void			ProcessDelayedSave();

		template<class Func>	Func for_each_pc(Func f);

		void			RegisterForMonsterLog(LPCHARACTER ch);
		void			UnregisterForMonsterLog(LPCHARACTER ch);
		void			PacketMonsterLog(LPCHARACTER ch, const void* buf, int32_t size);

		void			KillLog(uint32_t dwVnum);

		void			RegisterRaceNum(uint32_t dwVnum);
		void			RegisterRaceNumMap(LPCHARACTER ch);
		void			UnregisterRaceNumMap(LPCHARACTER ch);
		bool			GetCharactersByRaceNum(uint32_t dwRaceNum, CharacterVectorInteractor & i);

		LPCHARACTER		FindSpecifyPC(uint32_t uiJobFlag, int32_t lMapIndex, LPCHARACTER except=NULL, int32_t iMinLevel = 1, int32_t iMaxLevel = PLAYER_MAX_LEVEL_CONST);

		void			SetMobItemRate(int32_t value)	{ m_iMobItemRate = value; }
		void			SetMobDamageRate(int32_t value)	{ m_iMobDamageRate = value; }
		void			SetMobGoldAmountRate(int32_t value)	{ m_iMobGoldAmountRate = value; }
		void			SetMobGoldDropRate(int32_t value)	{ m_iMobGoldDropRate = value; }
		void			SetMobExpRate(int32_t value)	{ m_iMobExpRate = value; }

		void			SetMobItemRatePremium(int32_t value)	{ m_iMobItemRatePremium = value; }
		void			SetMobGoldAmountRatePremium(int32_t value)	{ m_iMobGoldAmountRatePremium = value; }
		void			SetMobGoldDropRatePremium(int32_t value)	{ m_iMobGoldDropRatePremium = value; }
		void			SetMobExpRatePremium(int32_t value)		{ m_iMobExpRatePremium = value; }

		void			SetUserDamageRatePremium(int32_t value)	{ m_iUserDamageRatePremium = value; }
		void			SetUserDamageRate(int32_t value) { m_iUserDamageRate = value; }
		int32_t			GetMobItemRate(LPCHARACTER ch);
		int32_t			GetMobDamageRate(LPCHARACTER ch);
		int32_t			GetMobGoldAmountRate(LPCHARACTER ch);
		int32_t			GetMobGoldDropRate(LPCHARACTER ch);
		int32_t			GetMobExpRate(LPCHARACTER ch);

		int32_t			GetUserDamageRate(LPCHARACTER ch);
		void		SendScriptToMap(int32_t lMapIndex, const std::string & s); 

		bool			BeginPendingDestroy();
		void			FlushPendingDestroy();

	private:
		int32_t					m_iMobItemRate;
		int32_t					m_iMobDamageRate;
		int32_t					m_iMobGoldAmountRate;
		int32_t					m_iMobGoldDropRate;
		int32_t					m_iMobExpRate;

		int32_t					m_iMobItemRatePremium;
		int32_t					m_iMobGoldAmountRatePremium;
		int32_t					m_iMobGoldDropRatePremium;
		int32_t					m_iMobExpRatePremium;

		int32_t					m_iUserDamageRate;
		int32_t					m_iUserDamageRatePremium;
		int32_t					m_iVIDCount;

		TR1_NS::unordered_map<uint32_t, LPCHARACTER> m_map_pChrByVID;
		TR1_NS::unordered_map<uint32_t, LPCHARACTER> m_map_pChrByPID;
		NAME_MAP			m_map_pPCChr;

		char				dummy1[1024];
		CHARACTER_SET		m_set_pChrState;
		CHARACTER_SET		m_set_pChrForDelayedSave;
		CHARACTER_SET		m_set_pChrMonsterLog;

		LPCHARACTER			m_pChrSelectedStone;

		std::map<uint32_t, uint32_t> m_map_dwMobKillCount;

		std::set<uint32_t>		m_set_dwRegisteredRaceNum;
		std::map<uint32_t, CHARACTER_SET> m_map_pChrByRaceNum;

		bool				m_bUsePendingDestroy;
		CHARACTER_SET		m_set_pChrPendingDestroy;
};

template<class Func>	
Func CHARACTER_MANAGER::for_each_pc(Func f)
{
	for (auto it = m_map_pChrByPID.begin(); it != m_map_pChrByPID.end(); ++it)
		f(it->second);

	return f;
}

class CharacterVectorInteractor : public CHARACTER_VECTOR
{
	public:
		CharacterVectorInteractor() : m_bMyBegin(false) { }

		CharacterVectorInteractor(const CHARACTER_SET& r);
		virtual ~CharacterVectorInteractor();

	private:
		bool m_bMyBegin;
};

#define M2_DESTROY_CHARACTER(ptr) CHARACTER_MANAGER::GetInstance()->DestroyCharacter(ptr)
