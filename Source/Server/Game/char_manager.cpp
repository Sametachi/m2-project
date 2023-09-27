#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "config.h"
#include "xmas_event.h"
#include "questmanager.h"
#include "questlua.h"
#include "locale_service.h"

#ifndef __GNUC__
#include <boost/bind.hpp>
#endif

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pChrSelectedStone(nullptr),
	m_bUsePendingDestroy(false)
{
	RegisterRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM);
	RegisterRaceNum(xmas::MOB_SANTA_VNUM);
	RegisterRaceNum(xmas::MOB_XMAS_TREE_VNUM);

	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 100;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 100;
	
	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
	auto it = m_map_pChrByVID.begin();
	while (it != m_map_pChrByVID.end()) {
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pChrByVID is changed here
		it = m_map_pChrByVID.begin();
	}
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pPCChr.begin();

	while (it != m_map_pPCChr.end())
		(it++)->second->Disconnect("GracefulShutdown");
}

uint32_t CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char* name, uint32_t dwPID)
{
	uint32_t dwVID = AllocVID();
	LPCHARACTER ch = M2_NEW CHARACTER;

	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pChrByVID.insert(std::make_pair(dwVID, ch));

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pPCChr.insert(NAME_MAP::value_type(szName, ch));
		m_map_pChrByPID.insert(std::make_pair(dwPID, ch));
	}

	return (ch);
}

void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	auto it = m_map_pChrByVID.find(ch->GetVID());
	if (it == m_map_pChrByVID.end()) {
		SysLog("[CHARACTER_MANAGER::DestroyCharacter] <Factor> {} not found", (int32_t)(ch->GetVID()));
		return; // prevent duplicated destrunction
	}

	if (ch->IsNPC() && !ch->IsPet() && ch->GetRider() == nullptr)
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}
	}

	if (m_bUsePendingDestroy)
	{
		m_set_pChrPendingDestroy.insert(ch);
		return;
	}

	m_map_pChrByVID.erase(it);

	if (ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pPCChr.find(szName);

		if (m_map_pPCChr.end() != it)
			m_map_pPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID())
	{
		auto it = m_map_pChrByPID.find(ch->GetPlayerID());

		if (m_map_pChrByPID.end() != it)
		{
			m_map_pChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);

	M2_DELETE(ch);
}

LPCHARACTER CHARACTER_MANAGER::Find(uint32_t dwVID)
{
	auto it = m_map_pChrByVID.find(dwVID);

	if (m_map_pChrByVID.end() == it)
		return NULL;
	
	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != nullptr && dwVID != (uint32_t)found->GetVID()) {
		SysLog("[CHARACTER_MANAGER::Find] <Factor> {} != {}", dwVID, (uint32_t)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID & vid)
{
	LPCHARACTER tch = Find((uint32_t) vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(uint32_t dwPID)
{
	auto it = m_map_pChrByPID.find(dwPID);

	if (m_map_pChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != nullptr && dwPID != found->GetPlayerID()) {
		SysLog("[CHARACTER_MANAGER::FindByPID] <Factor> {} != {}", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char* name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pPCChr.find(szName);

	if (it == m_map_pPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != nullptr && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		SysLog("[CHARACTER_MANAGER::FindPC] <Factor> {} != {}", name, found->GetName());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(uint32_t dwVnum, int32_t lMapIndex)
{
	{
		if (dwVnum == 5001 && !quest::CQuestManager::GetInstance()->GetEventFlag("japan_regen"))
		{
			TraceLog("WAEGU[5001] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5002 && !quest::CQuestManager::GetInstance()->GetEventFlag("newyear_mob"))
		{
			TraceLog("HAETAE (new-year-mob) [5002] regen disabled.");
			return NULL;
		}
	}

	{
		if (dwVnum == 5004 && !quest::CQuestManager::GetInstance()->GetEventFlag("independence_day"))
		{
			TraceLog("INDEPENDECE DAY [5004] regen disabled.");
			return NULL;
		}
	}

	const CMob* pMob = CMobManager::GetInstance()->Get(dwVnum);

	if (!pMob)
	{
		SysLog("no mob data for vnum {}", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		SysLog("not allowed map {}", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);
	if (pSectreeMap == nullptr) {
		return NULL;
	}

	int32_t i;
	int32_t x, y;
	for (i=0; i<2000; i++)
	{
		x = number(1, (pSectreeMap->m_setting.iWidth / 100)  - 1) * 100 + pSectreeMap->m_setting.iBaseX;
		y = number(1, (pSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pSectreeMap->m_setting.iBaseY;
		//LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);
		LPSECTREE tree = pSectreeMap->Find(x, y);

		if (!tree)
			continue;

		uint32_t dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		SysLog("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);

	if (!sectree)
	{
		PyLog("SpawnMobRandomPosition: cannot create monster at non-exist sectree {} x {} (map {})", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->CreateCharacter(pMob->m_table.szLocaleName);

	if (!ch)
	{
		PyLog("SpawnMobRandomPosition: cannot create new character");
		return NULL;
	}

	ch->SetProto(pMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::GetInstance()->GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(number(0, 360));

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		SysLog("SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}

	char buf[512+1];
	int32_t local_x = x - pSectreeMap->m_setting.iBaseX;
	int32_t local_y = y - pSectreeMap->m_setting.iBaseY;
	snprintf(buf, sizeof(buf), "spawn %s[%d] random position at %ld %ld %ld %ld (time: %d)", ch->GetName(), dwVnum, x, y, local_x, local_y, get_global_time());
	
	if (test_server)
		SendNotice(buf);

	PyLog("{}", buf);
	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMob(uint32_t dwVnum, int32_t lMapIndex, int32_t x, int32_t y, int32_t z, bool bSpawnMotion, int32_t iRot, bool bShow)
{
	const CMob* pMob = CMobManager::GetInstance()->Get(dwVnum);
	if (!pMob)
	{
		SysLog("SpawnMob: no mob data for vnum {}", dwVnum);
		return NULL;
	}

	if (!(pMob->m_table.bType == CHAR_TYPE_NPC || pMob->m_table.bType == CHAR_TYPE_WARP || pMob->m_table.bType == CHAR_TYPE_GOTO) || mining::IsVeinOfOre (dwVnum))
	{
		LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);

		if (!tree)
		{
			PyLog("no sectree for spawn at {} {} mobvnum {} mapindex {}", x, y, dwVnum, lMapIndex);
			return NULL;
		}

		uint32_t dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if (mining::IsVeinOfOre (dwVnum)) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if (is_set)
		{
			// SPAWN_BLOCK_LOG
			static bool s_isLog=quest::CQuestManager::GetInstance()->GetEventFlag("spawn_block_log");
			static uint32_t s_nextTime=get_global_time()+10000;

			uint32_t curTime=get_global_time();

			if (curTime>s_nextTime)
			{
				s_nextTime=curTime;
				s_isLog=quest::CQuestManager::GetInstance()->GetEventFlag("spawn_block_log");

			}

			if (s_isLog)
				PyLog("SpawnMob: BLOCKED position for spawn {} {} at {} {} (attr {})", pMob->m_table.szName, dwVnum, x, y, dwAttr);
			// END_OF_SPAWN_BLOCK_LOG
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			PyLog("SpawnMob: BAN_p position for mob spawn {} {} at {} {}", pMob->m_table.szName, dwVnum, x, y);
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);

	if (!sectree)
	{
		PyLog("SpawnMob: cannot create monster at non-exist sectree {} x {} (map {})", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->CreateCharacter(pMob->m_table.szLocaleName);

	if (!ch)
	{
		PyLog("SpawnMob: cannot create new character");
		return NULL;
	}

	if (iRot == -1)
		iRot = number(0, 360);

	ch->SetProto(pMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pMob->m_table.bType == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::GetInstance()->GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);

	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		PyLog("SpawnMob: cannot show monster");
		return NULL;
	}

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, bool bIsException, bool bSpawnMotion, bool bAggressive)
{
	const CMob* pMob = CMobManager::GetInstance()->Get(dwVnum);

	if (!pMob)
		return NULL;

	if (pMob->m_table.bType == CHAR_TYPE_STONE)
		bSpawnMotion = true;

	int32_t i = 16;

	while (i--)
	{
		int32_t x = number(sx, ex);
		int32_t y = number(sy, ey);
		/*
		   if (bIsException)
		   if (is_regen_exception(x, y))
		   continue;
		 */
		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);

		if (ch)
		{
			TraceLog("MOB_SPAWN: {}({}) {}x{}", ch->GetName(), (uint32_t) ch->GetVID(), ch->GetX(), ch->GetY());
			if (bAggressive)
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pChr)
{
	m_pChrSelectedStone = pChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, int32_t tx, int32_t ty, LPREGEN pRegen, bool bAggressive_)
{
	CMobGroup* pGroup = CMobManager::GetInstance()->GetGroup(dwVnum);

	if (!pGroup)
	{
		SysLog("NOT_EXIST_GROUP_VNUM({}) Map({}) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pChrMaster = nullptr;
	LPPARTY pParty = nullptr;

	const std::vector<uint32_t>& c_rdwMembers = pGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (uint32_t i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)	
				return false;

			continue;
		}

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pChrSelectedStone)
			tch->SetStone(m_pChrSelectedStone);
		else if (pParty)
		{
			pParty->Join(tch->GetVID());
			pParty->Link(tch);
		}
		else if (!pChrMaster)
		{
			pChrMaster = tch;
			pChrMaster->SetRegen(pRegen);

			pParty = CPartyManager::GetInstance()->CreateParty(pChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CHARACTER_MANAGER::SpawnGroupGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, LPREGEN pRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const uint32_t dwGroupID = CMobManager::GetInstance()->GetGroupFromGroupGroup(dwVnum);

	if(dwGroupID != 0)
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pRegen, bAggressive_, pDungeon);
	}
	else
	{
		SysLog("NOT_EXIST_GROUP_GROUP_VNUM({}) MAP({})", dwVnum, lMapIndex);
		return false;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(uint32_t dwVnum, int32_t lMapIndex, int32_t sx, int32_t sy, int32_t ex, int32_t ey, LPREGEN pRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	CMobGroup* pGroup = CMobManager::GetInstance()->GetGroup(dwVnum);

	if (!pGroup)
	{
		SysLog("NOT_EXIST_GROUP_VNUM({}) Map({}) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pChrMaster = nullptr;
	LPPARTY pParty = nullptr;

	const std::vector<uint32_t>& c_rdwMembers = pGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = nullptr;

	for (uint32_t i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - number(300, 500);
		sy = tch->GetY() - number(300, 500);
		ex = tch->GetX() + number(300, 500);
		ey = tch->GetY() + number(300, 500);

		if (m_pChrSelectedStone)
			tch->SetStone(m_pChrSelectedStone);
		else if (pParty)
		{
			pParty->Join(tch->GetVID());
			pParty->Link(tch);
		}
		else if (!pChrMaster)
		{
			pChrMaster = tch;
			pChrMaster->SetRegen(pRegen);

			pParty = CPartyManager::GetInstance()->CreateParty(pChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
	}
};

void CHARACTER_MANAGER::Update(int32_t iPulse)
{
	BeginPendingDestroy();

	bool bResetChatCounter = !(iPulse % PASSES_PER_SEC(5));

	// Update PC character
	std::for_each(m_map_pPCChr.begin(), m_map_pPCChr.end(),
		[&bResetChatCounter, &iPulse](const NAME_MAP::value_type& v)
		{
			LPCHARACTER ch = v.second;

			if (bResetChatCounter)
			{
				ch->ResetChatCounter();
				ch->CFSM::Update();
			}

			ch->UpdateCharacter(iPulse);
		}
	);

	// Update Monster
	std::for_each(m_set_pChrState.begin(), m_set_pChrState.end(),
		[iPulse](LPCHARACTER ch)
		{
			ch->UpdateStateMachine(iPulse);
		}
	);

	// Update to Santa
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			std::for_each(i.begin(), i.end(), [iPulse](LPCHARACTER ch)
				{
					ch->UpdateStateMachine(iPulse);
				}
			);
		}
	}

	// Record mob hunting counts once every hour
	if (0 == (iPulse % PASSES_PER_SEC(3600)))
	{
		for (auto it = m_map_dwMobKillCount.begin(); it != m_map_dwMobKillCount.end(); ++it)
			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);

		m_map_dwMobKillCount.clear();
	}

	// The test server counts the number of characters every 60 seconds
	if (test_server && 0 == (iPulse % PASSES_PER_SEC(60)))
		PyLog("CHARACTER COUNT vid %zu pid %zu", m_map_pChrByVID.size(), m_map_pChrByPID.size());

	// Delayed DestroyCharacter
	FlushPendingDestroy();
}

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pChrForDelayedSave.begin();

	while (it != m_set_pChrForDelayedSave.end())
	{
		LPCHARACTER pChr = *it++;
		pChr->SaveReal();
	}

	m_set_pChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != nullptr);

	CHARACTER_SET::iterator it = m_set_pChrState.find(ch);

	if (it == m_set_pChrState.end())
	{
		m_set_pChrState.insert(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pChrState.find(ch);

	if (it != m_set_pChrState.end())
	{
		m_set_pChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pChrForDelayedSave.insert(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pChrForDelayedSave.find(ch);

	if (it == m_set_pChrForDelayedSave.end())
		return false;

	m_set_pChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pChrMonsterLog.insert(ch);
}

void CHARACTER_MANAGER::UnregisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pChrMonsterLog.erase(ch);
}

void CHARACTER_MANAGER::PacketMonsterLog(LPCHARACTER ch, const void* buf, int32_t size)
{
	for (auto it = m_set_pChrMonsterLog.begin(); it!=m_set_pChrMonsterLog.end();++it)
	{
		LPCHARACTER c = *it;

		if (ch && DISTANCE_APPROX(c->GetX()-ch->GetX(), c->GetY()-ch->GetY())>6000)
			continue;

		LPDESC d = c->GetDesc();

		if (d)
			d->Packet(buf, size);
	}
}

void CHARACTER_MANAGER::KillLog(uint32_t dwVnum)
{
	const uint32_t SEND_LIMIT = 10000;

	auto it = m_map_dwMobKillCount.find(dwVnum);

	if (it == m_map_dwMobKillCount.end())
		m_map_dwMobKillCount.insert(std::make_pair(dwVnum, 1));
	else
	{
		++it->second;

		if (it->second > SEND_LIMIT)
		{
			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);
			m_map_dwMobKillCount.erase(it);
		}
	}
}

void CHARACTER_MANAGER::RegisterRaceNum(uint32_t dwVnum)
{
	m_set_dwRegisteredRaceNum.insert(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	uint32_t dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end())
	{
		PyLog("RegisterRaceNumMap {} {}", ch->GetName(), dwVnum);
		m_map_pChrByRaceNum[dwVnum].insert(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	uint32_t dwVnum = ch->GetRaceNum();

	auto it = m_map_pChrByRaceNum.find(dwVnum);

	if (it != m_map_pChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(uint32_t dwRaceNum, CharacterVectorInteractor & i)
{
	std::map<uint32_t, CHARACTER_SET>::iterator it = m_map_pChrByRaceNum.find(dwRaceNum);

	if (it == m_map_pChrByRaceNum.end())
		return false;

	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(uint32_t uiJobFlag, int32_t lMapIndex, LPCHARACTER except, int32_t iMinLevel, int32_t iMaxLevel)
{
	LPCHARACTER chFind = nullptr;
	int32_t n = 0;

	for (auto it = m_map_pChrByPID.begin(); it != m_map_pChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			uint32_t uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int32_t CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)	
{ 
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;

	return m_iMobItemRate; 
}

int32_t CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)	
{ 
	return m_iMobDamageRate; 
}

int32_t CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{ 
	if (!ch)
		return m_iMobGoldAmountRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;

	return m_iMobGoldAmountRate; 
}

int32_t CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iMobGoldDropRate;
	
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int32_t CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{ 
	if (!ch)
		return m_iMobExpRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;

	return m_iMobExpRate; 
}

int32_t	CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(int32_t lMapIndex, const std::string & s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (!pSecMap)
		return;

	struct packet_script p;

	p.header = HEADER_GC_SCRIPT;
	p.skin = 1;
	p.src_size = s.size();

	quest::FSendPacket f;
	p.size = p.src_size + sizeof(struct packet_script);
	f.buf.write(&p, sizeof(struct packet_script));
	f.buf.write(&s[0], s.size());

	pSecMap->for_each(f);
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	// To support the function that does not flush when Begin is repeated after Begin
	// If already started, return false
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{
	m_bUsePendingDestroy = false; // The actual destruction is processed only when the flag is set first

	if (!m_set_pChrPendingDestroy.empty())
	{
		PyLog("FlushPendingDestroy size {}", m_set_pChrPendingDestroy.size());
		
		CHARACTER_SET::iterator it = m_set_pChrPendingDestroy.begin(),
			end = m_set_pChrPendingDestroy.end();
		for (; it != end; ++it) {
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pChrPendingDestroy.clear();
	}
}

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET& r)
{
	m_bMyBegin = false;
#ifdef __GNUC__
	using namespace __gnu_cxx;
#endif

	reserve(r.size());
#ifdef __GNUC__
	transform(r.begin(), r.end(), back_inserter(*this), identity<CHARACTER_SET::value_type>());
#else
	insert(end(), r.begin(), r.end());
#endif

	if (CHARACTER_MANAGER::GetInstance()->BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::GetInstance()->FlushPendingDestroy();
}

