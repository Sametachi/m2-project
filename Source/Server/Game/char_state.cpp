#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "config.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "battle.h"
#include "char_manager.h"
#include "packet.h"
#include "motion.h"
#include "party.h"
#include "affect.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "exchange.h"
#include "sectree_manager.h"
#include "xmas_event.h"
#include "guild_manager.h"
#include "war_map.h"
#include "locale_service.h"
#include "BlueDragon.h"

#include <Common/VnumHelper.h>

BOOL g_test_server;
extern LPCHARACTER FindVictim(LPCHARACTER pChr, int32_t iMaxDistance);

namespace
{
	class FuncFindChrForFlag
	{
		public:
			FuncFindChrForFlag(LPCHARACTER pChr) :
				m_pChr(pChr), m_pChrFind(nullptr), m_iMinDistance(INT_MAX)
				{
				}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pChr = (LPCHARACTER) ent;

				if (!pChr->IsPC())
					return;

				if (!pChr->GetGuild())
					return;

				if (pChr->IsDead())
					return;

				int32_t iDist = DISTANCE_APPROX(pChr->GetX()-m_pChr->GetX(), pChr->GetY()-m_pChr->GetY());

				if (iDist <= 500 && m_iMinDistance > iDist &&
						!pChr->IsAffectFlag(AFF_WAR_FLAG1) &&
						!pChr->IsAffectFlag(AFF_WAR_FLAG2) &&
						!pChr->IsAffectFlag(AFF_WAR_FLAG3))
				{
					if ((uint32_t) m_pChr->GetPoint(POINT_STAT) == pChr->GetGuild()->GetID())
					{
						CWarMap* pMap = pChr->GetWarMap();
						uint8_t idx;

						if (!pMap || !pMap->GetTeamIndex(pChr->GetGuild()->GetID(), idx))
							return;

						if (!pMap->IsFlagOnBase(idx))
						{
							m_pChrFind = pChr;
							m_iMinDistance = iDist;
						}
					}
					else
					{
						m_pChrFind = pChr;
						m_iMinDistance = iDist;
					}
				}
			}

			LPCHARACTER	m_pChr;
			LPCHARACTER m_pChrFind;
			int32_t		m_iMinDistance;
	};

	class FuncFindChrForFlagBase
	{
		public:
			FuncFindChrForFlagBase(LPCHARACTER pChr) : m_pChr(pChr)
			{
			}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pChr = (LPCHARACTER) ent;

				if (!pChr->IsPC())
					return;

				CGuild* pGuild = pChr->GetGuild();

				if (!pGuild)
					return;

				int32_t iDist = DISTANCE_APPROX(pChr->GetX()-m_pChr->GetX(), pChr->GetY()-m_pChr->GetY());

				if (iDist <= 500 &&
						(pChr->IsAffectFlag(AFF_WAR_FLAG1) || 
						 pChr->IsAffectFlag(AFF_WAR_FLAG2) ||
						 pChr->IsAffectFlag(AFF_WAR_FLAG3)))
				{
					CAffect* pAff = pChr->FindAffect(AFFECT_WAR_FLAG);

					TraceLog("FlagBase {} dist {} flag gid {} chr gid {}",
							pChr->GetName(), iDist, m_pChr->GetPoint(POINT_STAT),
							pChr->GetGuild()->GetID());

					if (pAff)
					{
						if ((uint32_t) m_pChr->GetPoint(POINT_STAT) == pGuild->GetID() &&
								m_pChr->GetPoint(POINT_STAT) != pAff->lApplyValue)
						{
							CWarMap* pMap = pChr->GetWarMap();
							uint8_t idx;

							if (!pMap || !pMap->GetTeamIndex(pGuild->GetID(), idx))
								return;

							//if (pMap->IsFlagOnBase(idx))
							{
								uint8_t idx_opp = idx == 0 ? 1 : 0;

								SendGuildWarScore(m_pChr->GetPoint(POINT_STAT), pAff->lApplyValue, 1);
								//SendGuildWarScore(pAff->lApplyValue, m_pChr->GetPoint(POINT_STAT), -1);

								pMap->ResetFlag();
								//pMap->AddFlag(idx_opp);
								//pChr->RemoveAffect(AFFECT_WAR_FLAG);

								char buf[256];
								snprintf(buf, sizeof(buf), LC_TEXT("The guild %s's flag has been stolen by player %s."), pMap->GetGuild(idx)->GetName(), pMap->GetGuild(idx_opp)->GetName());
								pMap->Notice(buf);
							}
						}
					}
				}
			}

			LPCHARACTER m_pChr;
	};

	class FuncFindGuardVictim
	{
		public:
			FuncFindGuardVictim(LPCHARACTER pChr, int32_t iMaxDistance) :
				m_pChr(pChr),
			m_iMinDistance(INT_MAX),
			m_iMaxDistance(iMaxDistance),
			m_lx(pChr->GetX()),
			m_ly(pChr->GetY()),
			m_pChrVictim(nullptr)
			{
			};

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pChr = (LPCHARACTER) ent;

				if (pChr->IsPC())
					return;


				if (pChr->IsNPC() && !pChr->IsMonster())
					return;

				if (pChr->IsDead())
					return;

				if (pChr->IsAffectFlag(AFF_EUNHYUNG) || 
						pChr->IsAffectFlag(AFF_INVISIBILITY) ||
						pChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
					return;

				if (pChr->GetRaceNum() == 5001)
					return;

				int32_t iDistance = DISTANCE_APPROX(m_lx - pChr->GetX(), m_ly - pChr->GetY());

				if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
				{
					m_pChrVictim = pChr;
					m_iMinDistance = iDistance;
				}
			}

			LPCHARACTER GetVictim()
			{
				return (m_pChrVictim);
			}

		private:
			LPCHARACTER	m_pChr;

			int32_t		m_iMinDistance;
			int32_t		m_iMaxDistance;
			int32_t	m_lx;
			int32_t	m_ly;

			LPCHARACTER	m_pChrVictim;
	};

}

bool CHARACTER::IsAggressive() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_AGGRESSIVE);
}

void CHARACTER::SetAggressive()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_AGGRESSIVE);
}

bool CHARACTER::IsCoward() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_COWARD);
}

void CHARACTER::SetCoward()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_COWARD);
}

bool CHARACTER::IsBerserker() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_BERSERK);
}

bool CHARACTER::IsStoneSkinner() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_STONESKIN);
}

bool CHARACTER::IsGodSpeeder() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_GODSPEED);
}

bool CHARACTER::IsDeathBlower() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_DEATHBLOW);
}

bool CHARACTER::IsReviver() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_REVIVE);
}

void CHARACTER::CowardEscape()
{
	int32_t iDist[4] = {500, 1000, 3000, 5000};

	for (int32_t iDistIdx = 2; iDistIdx >= 0; --iDistIdx)
		for (int32_t iTryCount = 0; iTryCount < 8; ++iTryCount)
		{
			SetRotation(number(0, 359));

			float fx, fy;
			float fDist = number(iDist[iDistIdx], iDist[iDistIdx+1]);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			bool bIsWayBlocked = false;
			for (int32_t j = 1; j <= 100; ++j)
			{
				if (!SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx*j/100, GetY() + (int32_t) fy*j/100))
				{
					bIsWayBlocked = true;
					break;
				}
			}

			if (bIsWayBlocked)
				continue;

			m_dwStateDuration = PASSES_PER_SEC(1);

			int32_t iDestX = GetX() + (int32_t) fx;
			int32_t iDestY = GetY() + (int32_t) fy;

			if (Goto(iDestX, iDestY))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			PyLog("WAEGU move to {} {} (far)", iDestX, iDestY);
			return;
		}
}

void  CHARACTER::SetNoAttackShinsu()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKSHINSU);
}
bool CHARACTER::IsNoAttackShinsu() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKSHINSU);
}

void CHARACTER::SetNoAttackChunjo()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKCHUNJO);
}

bool CHARACTER::IsNoAttackChunjo() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKCHUNJO);
}

void CHARACTER::SetNoAttackJinno()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKJINNO);
}

bool CHARACTER::IsNoAttackJinno() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOATTACKJINNO);
}

void CHARACTER::SetAttackMob()
{
	SET_BIT(m_pointsInstant.dwAIFlag, MOB::AIFLAG_ATTACKMOB);
}

bool CHARACTER::IsAttackMob() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_ATTACKMOB);
}

// STATE_IDLE_REFACTORING
void CHARACTER::StateIdle()
{
	if (IsStone())
	{
		__StateIdle_Stone();
		return;
	}
	else if (IsWarp() || IsGoto())
	{
		m_dwStateDuration = 60* passes_per_sec;
		return;
	}

	if (IsPC())
		return;

	if (!IsMonster())
	{
		__StateIdle_NPC();
		return;
	}

	__StateIdle_Monster();
}

void CHARACTER::__StateIdle_Stone()
{
	m_dwStateDuration = PASSES_PER_SEC(1);

	int32_t iPercent = (GetHP() * 100) / GetMaxHP();
	uint32_t dwVnum = number(MIN(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed), MAX(GetMobTable().sAttackSpeed, GetMobTable().sMovingSpeed));

	if (iPercent <= 10 && GetMaxSP() < 10)
	{
		SetMaxSP(10);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 20 && GetMaxSP() < 9)
	{
		SetMaxSP(9);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 30 && GetMaxSP() < 8)
	{
		SetMaxSP(8);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 40 && GetMaxSP() < 7)
	{
		SetMaxSP(7);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 50 && GetMaxSP() < 6)
	{
		SetMaxSP(6);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 60 && GetMaxSP() < 5)
	{
		SetMaxSP(5);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 70 && GetMaxSP() < 4)
	{
		SetMaxSP(4);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 80 && GetMaxSP() < 3)
	{
		SetMaxSP(3);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 90 && GetMaxSP() < 2)
	{
		SetMaxSP(2);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else if (iPercent <= 99 && GetMaxSP() < 1)
	{
		SetMaxSP(1);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::GetInstance()->SelectStone(this);
		CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::GetInstance()->SelectStone(nullptr);
	}
	else
		return;

	UpdatePacket();
	return;
}

void CHARACTER::__StateIdle_NPC()
{
	MonsterChat(MONSTER_CHAT_WAIT);
	m_dwStateDuration = PASSES_PER_SEC(5);

	if (IsPet())
		return;
	else if (IsGuardNPC())
	{
		if (!quest::CQuestManager::GetInstance()->GetEventFlag("noguard"))
		{
			FuncFindGuardVictim f(this, 50000);

			if (GetSectree())
				GetSectree()->ForEachAround(f);

			LPCHARACTER victim = f.GetVictim();

			if (victim)
			{
				m_dwStateDuration = passes_per_sec/2;

				if (CanBeginFight())
					BeginFight(victim);
			}
		}
	}
	else
	{
		if (GetRaceNum() == xmas::MOB_SANTA_VNUM)
		{
			if (get_dword_time() > m_dwPlayStartTime)
			{
				int32_t	next_warp_time = 2 * 1000;

				m_dwPlayStartTime = get_dword_time() + next_warp_time;

				const int32_t WARP_MAP_INDEX_NUM = 7;
				static const int32_t c_lWarpMapIndexs[WARP_MAP_INDEX_NUM] = { 61, 62, 63, 64, 3, 23, 43 };
				int32_t lNextMapIndex;
				lNextMapIndex = c_lWarpMapIndexs[number(1, WARP_MAP_INDEX_NUM) - 1];

				if (map_allow_find(lNextMapIndex))
				{
					M2_DESTROY_CHARACTER(this);
					int32_t iNextSpawnDelay = 50 * 60;

					xmas::SpawnSanta(lNextMapIndex, iNextSpawnDelay);
				}
				else
				{
					TPacketGGXmasWarpSanta p;
					p.bHeader   = HEADER_GG_XMAS_WARP_SANTA;
					p.bChannel  = g_bChannel;
					p.lMapIndex = lNextMapIndex;
					P2P_MANAGER::GetInstance()->Send(&p, sizeof(TPacketGGXmasWarpSanta));
				}
				return;
			}
		}

		if (!IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOMOVE))
		{ 
			LPCHARACTER pChrProtege = GetProtege();

			if (pChrProtege)
			{
				if (DISTANCE_APPROX(GetX() - pChrProtege->GetX(), GetY() - pChrProtege->GetY()) > 500)
				{
					if (Follow(pChrProtege, number(100, 300)))
						return;
				}
			}

			if (!number(0, 6))
			{
				SetRotation(number(0, 359));

				float fx, fy;
				float fDist = number(200, 400);

				GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

				if (!(SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx, GetY() + (int32_t) fy) 
					&& SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx / 2, GetY() + (int32_t) fy / 2)))
					return;

				SetNowWalking(true);

				if (Goto(GetX() + (int32_t) fx, GetY() + (int32_t) fy))
					SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

				return;
			}
		}
	}
}

void CHARACTER::__StateIdle_Monster()
{
	if (IsStun())
		return;

	if (!CanMove())
		return;

	if (IsCoward())
	{
		if (!IsDead())
			CowardEscape();

		return;
	}

	if (IsBerserker())
		if (IsBerserk())
			SetBerserk(false);

	if (IsGodSpeeder())
		if (IsGodSpeed())
			SetGodSpeed(false);

	LPCHARACTER victim = GetVictim();

	if (!victim || victim->IsDead())
	{
		SetVictim(nullptr);
		victim = nullptr;
		m_dwStateDuration = PASSES_PER_SEC(1);
	}

	if (!victim || victim->IsBuilding())
	{
		if (m_pChrStone)
		{
			victim = m_pChrStone->GetNearestVictim(m_pChrStone);
		}
		else if (!no_wander && IsAggressive())
		{
			if (GetMapIndex() == 61 && quest::CQuestManager::GetInstance()->GetEventFlag("xmas_tree"));
			else
				victim = FindVictim(this, m_pMobData->m_table.wAggressiveSight);
		}
	}

	if (victim && !victim->IsDead())
	{
		if (CanBeginFight())
			BeginFight(victim);

		return;
	}

	if (IsAggressive() && !victim)
		m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
	else
		m_dwStateDuration = PASSES_PER_SEC(number(3, 5));

	LPCHARACTER pChrProtege = GetProtege();

	if (pChrProtege)
	{
		if (DISTANCE_APPROX(GetX() - pChrProtege->GetX(), GetY() - pChrProtege->GetY()) > 1000)
		{
			if (Follow(pChrProtege, number(150, 400)))
			{
				MonsterLog("[IDLE] 리더로부터 너무 멀리 떨어졌다! 복귀한다.");
				return;
			}
		}
	}

	if (!no_wander && !IS_SET(m_pointsInstant.dwAIFlag, MOB::AIFLAG_NOMOVE))
	{
		if (!number(0, 6))
		{
			SetRotation(number(0, 359));

			float fx, fy;
			float fDist = number(300, 700);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			if (!(SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx, GetY() + (int32_t) fy) 
						&& SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx/2, GetY() + (int32_t) fy/2)))
				return;

			// NOTE: When a monster wanders around in IDLE state, it is currently run unconditionally. (Never walk)
			// The graphics team wants to see the monsters walking, so they temporarily walk or run with a certain probability. (It only works once in test mode, because the overall feel of the game is wrong)
			if (g_test_server)
			{
				if (number(0, 100) < 60)
					SetNowWalking(false);
				else
					SetNowWalking(true);
			}

			if (Goto(GetX() + (int32_t) fx, GetY() + (int32_t) fy))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			return;
		}
	}

	MonsterChat(MONSTER_CHAT_WAIT);
}
// END_OF_STATE_IDLE_REFACTORING

bool __CHARACTER_GotoNearTarget(LPCHARACTER self, LPCHARACTER victim)
{
	if (IS_SET(self->GetAIFlag(), MOB::AIFLAG_NOMOVE))
		return false;

	switch (self->GetMobBattleType())
	{
		case MOB::BATTLE_TYPE_RANGE:
		case MOB::BATTLE_TYPE_MAGIC:
			if (self->Follow(victim, self->GetMobAttackRange() * 8 / 10))
				return true;
			break;

		default:
			if (self->Follow(victim, self->GetMobAttackRange() * 9 / 10))
				return true;
	}

	return false;
}

void CHARACTER::StateMove()
{
	uint32_t dwElapsedTime = get_dword_time() - m_dwMoveStartTime;
	float fRate = (float) dwElapsedTime / (float) m_dwMoveDuration;

	if (fRate > 1.0f)
		fRate = 1.0f;

	int32_t x = (int32_t) ((float) (m_posDest.x - m_posStart.x) * fRate + m_posStart.x);
	int32_t y = (int32_t) ((float) (m_posDest.y - m_posStart.y) * fRate + m_posStart.y);

	Move(x, y);

	if (IsPC() && (thecore_pulse() & 15) == 0)
	{
		UpdateSectree();

		if (GetExchange())
		{
			LPCHARACTER victim = GetExchange()->GetCompany()->GetOwner();
			int32_t iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

			if (iDist >= EXCHANGE_MAX_DISTANCE)
			{
				GetExchange()->Cancel();
			}
		}
	}

	if (IsPC())
	{
		if (IsWalking() && GetStamina() < GetMaxStamina())
		{
			if (get_dword_time() - GetWalkStartTime() > 5000)
				PointChange(POINT_STAMINA, GetMaxStamina() / 1);
		}

		if (!IsWalking() && !IsRiding())
			if ((get_dword_time() - GetLastAttackTime()) < 20000)
			{
				StartAffectEvent();

				if (IsStaminaHalfConsume())
				{
					if (thecore_pulse()&1)
						PointChange(POINT_STAMINA, -STAMINA_PER_STEP);
				}
				else
					PointChange(POINT_STAMINA, -STAMINA_PER_STEP);

				StartStaminaConsume();

				if (GetStamina() <= 0)
				{
					SetStamina(0);
					SetNowWalking(true);
					StopStaminaConsume();
				}
			}
			else if (IsStaminaConsume())
			{
				StopStaminaConsume();
			}
	}
	else
	{
		if (IsMonster() && GetVictim())
		{
			LPCHARACTER victim = GetVictim();
			UpdateAggrPoint(victim, DAMAGE_TYPE_NORMAL, -(victim->GetLevel() / 3 + 1));

			if (g_test_server)
			{
				SetNowWalking(false);
			}
		}

		if (IsMonster() && GetMobRank() >= MOB::RANK_BOSS && GetVictim())
		{
			LPCHARACTER victim = GetVictim();

			if (GetRaceNum() == 2191 && number(1, 20) == 1 && get_dword_time() - m_pMobInst->m_dwLastWarpTime > 1000)
			{
				float fx, fy;
				GetDeltaByDegree(victim->GetRotation(), 400, &fx, &fy);
				int32_t new_x = victim->GetX() + (int32_t)fx;
				int32_t new_y = victim->GetY() + (int32_t)fy;
				SetRotation(GetDegreeFromPositionXY(new_x, new_y, victim->GetX(), victim->GetY()));
				Show(victim->GetMapIndex(), new_x, new_y, 0, true);
				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
				ResetMobSkillCooltime();
				m_pMobInst->m_dwLastWarpTime = get_dword_time();
				return;
			}

			// Make a TODO turn and be less stupid!
			if (number(0, 3) == 0)
			{
				if (__CHARACTER_GotoNearTarget(this, victim))
					return;
			}
		}
	}

	if (1.0f == fRate)
	{
		if (IsPC())
		{
			TraceLog("도착 {} {} {}", GetName(), x, y);
			GotoState(m_stateIdle);
			StopStaminaConsume();
		}
		else
		{
			if (GetVictim() && !IsCoward())
			{
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] 근처에 왔으니 공격시작 %s", GetVictim()->GetName());

				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
			}
			else
			{
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] 대상이 없으니 쉬자");

				GotoState(m_stateIdle);

				LPCHARACTER rider = GetRider();

				m_dwStateDuration = PASSES_PER_SEC(number(1, 3));
			}
		}
	}
}

void CHARACTER::StateBattle()
{
	if (IsStone())
	{
		SysLog("Stone must not use battle state (name {})", GetName());
		return;
	}

	if (IsPC())
		return; 

	if (!CanMove())
		return;

	if (IsStun())
		return;

	LPCHARACTER victim = GetVictim();

	if (IsCoward())
	{
		if (IsDead())
			return;

		SetVictim(nullptr);

		if (number(1, 50) != 1)
		{
			GotoState(m_stateIdle);
			m_dwStateDuration = 1;
		}
		else
			CowardEscape();

		return;
	}

	if (!victim || (victim->IsStun() && IsGuardNPC()) || victim->IsDead())
	{
		if (victim && victim->IsDead() &&
				!no_wander && IsAggressive() && (!GetParty() || GetParty()->GetLeader() == this))
		{
			LPCHARACTER new_victim = FindVictim(this, m_pMobData->m_table.wAggressiveSight);

			SetVictim(new_victim);
			m_dwStateDuration = PASSES_PER_SEC(1);

			if (!new_victim)
			{
				switch (GetMobBattleType())
				{
					case MOB::BATTLE_TYPE_MELEE:
					case MOB::BATTLE_TYPE_SUPER_POWER:
					case MOB::BATTLE_TYPE_SUPER_TANKER:
					case MOB::BATTLE_TYPE_POWER:
					case MOB::BATTLE_TYPE_TANKER:
						{
							float fx, fy;
							float fDist = number(400, 1500);

							GetDeltaByDegree(number(0, 359), fDist, &fx, &fy);

							if (SECTREE_MANAGER::GetInstance()->IsMovablePosition(victim->GetMapIndex(),
										victim->GetX() + (int32_t) fx, 
										victim->GetY() + (int32_t) fy) && 
									SECTREE_MANAGER::GetInstance()->IsMovablePosition(victim->GetMapIndex(),
										victim->GetX() + (int32_t) fx/2,
										victim->GetY() + (int32_t) fy/2))
							{
								float dx = victim->GetX() + fx;
								float dy = victim->GetY() + fy;

								SetRotation(GetDegreeFromPosition(dx, dy));

								if (Goto((int32_t) dx, (int32_t) dy))
								{
									PyLog("KILL_AND_GO: {} distance %.1f", GetName(), fDist);
									SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
								}
							}
						}
				}
			}
			return;
		}

		SetVictim(nullptr);

		if (IsGuardNPC())
			Return();

		m_dwStateDuration = PASSES_PER_SEC(1);
		return;
	}

	if (IsSummonMonster() && !IsDead() && !IsStun())
	{
		if (!GetParty())
		{
			CPartyManager::GetInstance()->CreateParty(this);
		}

		LPPARTY pParty = GetParty();
		bool bPct = !number(0, 3);

		if (bPct && pParty->CountMemberByVnum(GetSummonVnum()) < SUMMON_MONSTER_COUNT)
		{
			MonsterLog("Summon subordinate monsters!");
			int32_t sx = GetX() - 300;
			int32_t sy = GetY() - 300;
			int32_t ex = GetX() + 300;
			int32_t ey = GetY() + 300;

			LPCHARACTER tch = CHARACTER_MANAGER::GetInstance()->SpawnMobRange(GetSummonVnum(), GetMapIndex(), sx, sy, ex, ey, true, true);

			if (tch)
			{
				pParty->Join(tch->GetVID());
				pParty->Link(tch);
			}
		}
	}

	LPCHARACTER pChrProtege = GetProtege();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= 4000.0f)
	{
		MonsterLog("Giving up because the target is too far");
		SetVictim(nullptr);

		if (pChrProtege)
			if (DISTANCE_APPROX(GetX() - pChrProtege->GetX(), GetY() - pChrProtege->GetY()) > 1000)
				Follow(pChrProtege, number(150, 400));

		return;
	}

	if (fDist >= GetMobAttackRange() * 1.15)
	{
		__CHARACTER_GotoNearTarget(this, victim);
		return;
	}

	if (m_pParty)
		m_pParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);

	if (2493 == m_pMobData->m_table.dwVnum)
	{
		m_dwStateDuration = BlueDragon_StateBattle(this);
		return;
	}

	uint32_t dwCurTime = get_dword_time();
	uint32_t dwDuration = CalculateDuration(GetLimitPoint(POINT_ATT_SPEED), 2000);

	if ((dwCurTime - m_dwLastAttackTime) < dwDuration)
	{
		m_dwStateDuration = MAX(1, (passes_per_sec * (dwDuration - (dwCurTime - m_dwLastAttackTime)) / 1000));
		return;
	}

	if (IsBerserker())
		if (GetHPPct() < m_pMobData->m_table.bBerserkPoint)
			if (IsBerserk() != true)
				SetBerserk(true);

	if (IsGodSpeeder())
		if (GetHPPct() < m_pMobData->m_table.bGodSpeedPoint)
			if (IsGodSpeed() != true)
				SetGodSpeed(true);

	if (HasMobSkill())
	{
		for (uint32_t iSkillIdx = 0; iSkillIdx < MOB::SKILL_MAX_NUM; ++iSkillIdx)
		{
			if (CanUseMobSkill(iSkillIdx))
			{
				SetRotationToXY(victim->GetX(), victim->GetY());

				if (UseMobSkill(iSkillIdx))
				{
					SendMovePacket(FUNC_MOB_SKILL, iSkillIdx, GetX(), GetY(), 0, dwCurTime);

					float fDuration = CMotionManager::GetInstance()->GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_SPECIAL_1 + iSkillIdx));
					m_dwStateDuration = (uint32_t) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));

					if (test_server)
						PyLog("USE_MOB_SKILL: {} idx {} motion {} duration %.0f", GetName(), iSkillIdx, MOTION_SPECIAL_1 + iSkillIdx, fDuration);

					return;
				}
			}
		}
	}

	if (!Attack(victim))    // What if the attack failed? Why did it fail? TODO
		m_dwStateDuration = passes_per_sec / 2;
	else
	{
		SetRotationToXY(victim->GetX(), victim->GetY());

		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0, dwCurTime);

		float fDuration = CMotionManager::GetInstance()->GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));
		m_dwStateDuration = (uint32_t) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));
	}
}

void CHARACTER::StateFlag()
{
	m_dwStateDuration = (uint32_t) PASSES_PER_SEC(0.5);

	CWarMap* pMap = GetWarMap();

	if (!pMap)
		return;

	FuncFindChrForFlag f(this);
	GetSectree()->ForEachAround(f);

	if (!f.m_pChrFind)
		return;

	if (!f.m_pChrFind->GetGuild())
		return;

	char buf[256];
	uint8_t idx;

	if (!pMap->GetTeamIndex(GetPoint(POINT_STAT), idx))
		return;

	f.m_pChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_NONE, GetPoint(POINT_STAT), idx == 0 ? AFF_WAR_FLAG1 : AFF_WAR_FLAG2, INFINITE_AFFECT_DURATION, 0, false);
	f.m_pChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_MOV_SPEED, 50 - f.m_pChrFind->GetPoint(POINT_MOV_SPEED), 0, INFINITE_AFFECT_DURATION, 0, false);

	pMap->RemoveFlag(idx);

	snprintf(buf, sizeof(buf), LC_TEXT("%s has captured the flag of %s!"), pMap->GetGuild(idx)->GetName(), f.m_pChrFind->GetName());
	pMap->Notice(buf);
}

void CHARACTER::StateFlagBase()
{
	m_dwStateDuration = (uint32_t) PASSES_PER_SEC(0.5);

	FuncFindChrForFlagBase f(this);
	GetSectree()->ForEachAround(f);
}

void CHARACTER::StateHorse()
{
	float	START_FOLLOW_DISTANCE = 400.0f;
	float	START_RUN_DISTANCE = 700.0f;
	int32_t		MIN_APPROACH = 150;
	int32_t		MAX_APPROACH = 300;	

	uint32_t	STATE_DURATION = (uint32_t)PASSES_PER_SEC(0.5);

	bool bDoMoveAlone = true;
	bool bRun = true;

	if (IsDead())
		return;

	m_dwStateDuration = STATE_DURATION;

	LPCHARACTER victim = GetRider();

	if (!victim)
	{
		M2_DESTROY_CHARACTER(this);
		return;
	}

	m_pMobInst->m_posLastAttacked = GetXYZ();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if (fDist > START_RUN_DISTANCE)
			SetNowWalking(!bRun);

		Follow(victim, number(MIN_APPROACH, MAX_APPROACH));

		m_dwStateDuration = STATE_DURATION;
	}
	else if (bDoMoveAlone && (get_dword_time() > m_dwLastAttackTime))
	{
		m_dwLastAttackTime = get_dword_time() + number(5000, 12000);

		SetRotation(number(0, 359));

		float fx, fy;
		float fDist = number(200, 400);

		GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

		if (!(SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx, GetY() + (int32_t) fy) 
					&& SECTREE_MANAGER::GetInstance()->IsMovablePosition(GetMapIndex(), GetX() + (int32_t) fx/2, GetY() + (int32_t) fy/2)))
			return;

		SetNowWalking(true);

		if (Goto(GetX() + (int32_t) fx, GetY() + (int32_t) fy))
			SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}
}

