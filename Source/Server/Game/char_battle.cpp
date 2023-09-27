#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "battle.h"
#include "pvp.h"
#include "skill.h"
#include "start_position.h"
#include "cmd.h"
#include "dungeon.h"
#include "log.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "db.h"
#include "vector.h"
#include "marriage.h"
#include "arena.h"
#include "regen.h"
#include "monarch.h"
#include "exchange.h"
#include "shop_manager.h"
#include "castle.h"
#include "ani.h"
#include "BattleArena.h"
#include "packet.h"
#include "party.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "questmanager.h"
#include "questlua.h"
#include "threeway_war.h"
#include "BlueDragon.h"
#include "DragonLair.h"
#include <random>
#include <utility>
#include <type_traits>

uint32_t AdjustExpByLevel(const LPCHARACTER ch, const uint32_t exp)
{
	if (PLAYER_EXP_TABLE_MAX < ch->GetLevel())
	{
		double ret = 0.95;
		double factor = 0.1;

		for (ssize_t i=0 ; i < ch->GetLevel()-100 ; ++i)
		{
			if ((i%10) == 0)
				factor /= 2.0;

			ret *= 1.0 - factor;
		}

		ret = ret * static_cast<double>(exp);

		if (ret < 1.0)
			return 1;

		return static_cast<uint32_t>(ret);
	}

	return exp;
}

bool CHARACTER::CanBeginFight() const
{
	if (!CanMove())
		return false;

	return m_pointsInstant.position == POS_STANDING && !IsDead() && !IsStun();
}

void CHARACTER::BeginFight(LPCHARACTER pVictim)
{
	SetVictim(pVictim);
	SetPosition(POS_FIGHTING);
	SetNextStatePulse(1);
}

bool CHARACTER::CanFight() const
{
	return m_pointsInstant.position >= POS_FIGHTING ? true : false;
}

void CHARACTER::CreateFly(uint8_t bType, LPCHARACTER pVictim)
{
	TPacketGCCreateFly packFly;

	packFly.bHeader         = HEADER_GC_CREATE_FLY;
	packFly.bType           = bType;
	packFly.dwStartVID      = GetVID();
	packFly.dwEndVID        = pVictim->GetVID();

	PacketAround(&packFly, sizeof(TPacketGCCreateFly));
}

void CHARACTER::DistributeSP(LPCHARACTER pKiller, int32_t iMethod)
{
	if (pKiller->GetSP() >= pKiller->GetMaxSP())
		return;

	bool bAttacking = (get_dword_time() - GetLastAttackTime()) < 3000;
	bool bMoving = (get_dword_time() - GetLastMoveTime()) < 3000;

	if (iMethod == 1)
	{
		int32_t num = number(0, 3);

		if (!num)
		{
			int32_t iLvDelta = GetLevel() - pKiller->GetLevel();
			int32_t iAmount = 0;

			if (iLvDelta >= 5)
				iAmount = 10;
			else if (iLvDelta >= 0)
				iAmount = 6;
			else if (iLvDelta >= -3)
				iAmount = 2;

			if (iAmount != 0)
			{
				iAmount += (iAmount* pKiller->GetPoint(POINT_SP_REGEN)) / 100;

				if (iAmount >= 11)
					CreateFly(FLY_SP_BIG, pKiller);
				else if (iAmount >= 7)
					CreateFly(FLY_SP_MEDIUM, pKiller);
				else
					CreateFly(FLY_SP_SMALL, pKiller);

				pKiller->PointChange(POINT_SP, iAmount);
			}
		}
	}
	else
	{
		if (pKiller->GetJob() == JOB_SHAMAN || (pKiller->GetJob() == JOB_SURA && pKiller->GetSkillGroup() == 2))
		{
			int32_t iAmount;

			if (bAttacking)
				iAmount = 2 + GetMaxSP() / 100;
			else if (bMoving)
				iAmount = 3 + GetMaxSP() * 2 / 100;
			else
				iAmount = 10 + GetMaxSP() * 3 / 100; // Normal

			iAmount += (iAmount* pKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pKiller->PointChange(POINT_SP, iAmount);
		}
		else
		{
			int32_t iAmount;

			if (bAttacking)
				iAmount = 2 + pKiller->GetMaxSP() / 200;
			else if (bMoving)
				iAmount = 2 + pKiller->GetMaxSP() / 100;
			else
			{
				// Normal
				if (pKiller->GetHP() < pKiller->GetMaxHP())
					iAmount = 2 + (pKiller->GetMaxSP() / 100); // Max HP
				else
					iAmount = 9 + (pKiller->GetMaxSP() / 100); // Normal
			}

			iAmount += (iAmount* pKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pKiller->PointChange(POINT_SP, iAmount);
		}
	}
}


bool CHARACTER::Attack(LPCHARACTER pVictim, uint8_t bType)
{
	if (test_server)
		PyLog("[TEST_SERVER] Attack : {} type {}, MobBattleType {}", GetName(), bType, !GetMobBattleType() ? 0 : GetMobAttackRange());
	//PROF_UNIT puAttack("Attack");
	if (!CanMove())
		return false;

	// CASTLE
	if (IS_CASTLE_MAP(GetMapIndex()) && !castle_can_attack(this, pVictim))
		return false;
	// CASTLE

	uint32_t dwCurrentTime = get_dword_time();

	if (IsPC())
	{
		if (IS_SPEED_HACK(this, pVictim, dwCurrentTime))
			return false;

		if (bType == 0 && dwCurrentTime < GetSkipComboAttackByTime())
			return false;
	}
	else
	{
		MonsterChat(MONSTER_CHAT_ATTACK);
	}

	pVictim->SetSyncOwner(this);

	if (pVictim->CanBeginFight())
		pVictim->BeginFight(this);

	int32_t iRet;

	if (bType == 0)
	{
		//
		// normal attack
		//
		switch (GetMobBattleType())
		{
			case MOB::BATTLE_TYPE_MELEE:
			case MOB::BATTLE_TYPE_POWER:
			case MOB::BATTLE_TYPE_TANKER:
			case MOB::BATTLE_TYPE_SUPER_POWER:
			case MOB::BATTLE_TYPE_SUPER_TANKER:
				iRet = battle_melee_attack(this, pVictim);
				break;

			case MOB::BATTLE_TYPE_RANGE:
				FlyTarget(pVictim->GetVID(), pVictim->GetX(), pVictim->GetY(), HEADER_CG_FLY_TARGETING);
				iRet = Shoot(0) ? BATTLE_DAMAGE : BATTLE_NONE;
				break;

			case MOB::BATTLE_TYPE_MAGIC:
				FlyTarget(pVictim->GetVID(), pVictim->GetX(), pVictim->GetY(), HEADER_CG_FLY_TARGETING);
				iRet = Shoot(1) ? BATTLE_DAMAGE : BATTLE_NONE;
				break;

			default:
				SysLog("Unhandled battle type {}", GetMobBattleType());
				iRet = BATTLE_NONE;
				break;
		}
	}
	else
	{
		if (IsPC())
		{
			if (dwCurrentTime - m_dwLastSkillTime > 1500)
			{
				TraceLog("HACK: Too int32_t skill using term. Name({}) PID({}) delta({})",
						GetName(), GetPlayerID(), (dwCurrentTime - m_dwLastSkillTime));
				return false;
			}
		}

		TraceLog("Attack call ComputeSkill {} {}", bType, pVictim?pVictim->GetName():"");
		iRet = ComputeSkill(bType, pVictim);
	}

	//if (test_server && IsPC())
	//	PyLog("{} Attack {} type {} ret {}", GetName(), pVictim->GetName(), bType, iRet);
	if (iRet == BATTLE_DAMAGE || iRet == BATTLE_DEAD)
	{
		OnMove(true);
		pVictim->OnMove();

		// only pc sets victim null. For npc, state machine will reset this.
		if (BATTLE_DEAD == iRet && IsPC())
			SetVictim(nullptr);

		return true;
	}

	return false;
}

void CHARACTER::DeathPenalty(uint8_t bTown)
{
	TraceLog("DEATH_PERNALY_CHECK({}) town({})", GetName(), bTown);

	Cube_close(this);

	if (CBattleArena::GetInstance()->IsBattleArenaMap(GetMapIndex()))
	{
		return;
	}
	
	if (GetLevel() < 10)
	{
		PyLog("NO_DEATH_PENALTY_LESS_LV10({})", GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not lose any Experience because of the Blessing of the Dragon God."));
		return;
	}

   	if (number(0, 2))
	{
		PyLog("NO_DEATH_PENALTY_LUCK({})", GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not lose any Experience because of the Blessing of the Dragon God."));
		return;
	}

	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY))
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
		{
			if (FindAffect(AFFECT_NO_DEATH_PENALTY))
			{
				PyLog("NO_DEATH_PENALTY_AFFECT({})", GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not lose any Experience because of the Blessing of the Dragon God."));
				RemoveAffect(AFFECT_NO_DEATH_PENALTY);
				return;
			}
		}

		int32_t iLoss = ((GetNextExp() * aiExpLossPercents[MINMAX(1, GetLevel(), PLAYER_EXP_TABLE_MAX)]) / 100);

		iLoss = MIN(800000, iLoss);

		if (bTown)
		{
			iLoss = 0;
		}

		if (IsEquipUniqueItem(UNIQUE_ITEM_TEARDROP_OF_GODNESS))
			iLoss /= 2;

		PyLog("DEATH_PENALTY({}) EXP_LOSS: {} percent {}%", GetName(), iLoss, aiExpLossPercents[MIN(gPlayerMaxLevel, GetLevel())]);

		PointChange(POINT_EXP, -iLoss, true);
	}
}

bool CHARACTER::IsStun() const
{
	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN))
		return true;

	return false;
}

EVENTFUNC(StunEvent)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("StunEvent> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == nullptr) { // <Factor>
		return 0;
	}
	ch->m_pStunEvent = nullptr;
	ch->Dead();
	return 0;
}

void CHARACTER::Stun()
{
	if (IsStun())
		return;

	if (IsDead())
		return;

	if (!IsPC() && m_pParty)
	{
		m_pParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);
	}

	TraceLog("{}: Stun", GetName());

	PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
	PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));

	CloseMyShop();

	event_cancel(&m_pRecoveryEvent);

	TPacketGCStun pack;
	pack.header	= HEADER_GC_STUN;
	pack.vid	= m_vid;
	PacketAround(&pack, sizeof(pack));

	SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (m_pStunEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pStunEvent = event_create(StunEvent, info, PASSES_PER_SEC(3));
}

EVENTINFO(SCharDeadEventInfo)
{
	bool isPC;
	uint32_t dwID;

	SCharDeadEventInfo()
	: isPC(0)
	, dwID(0)
	{
	}
};

EVENTFUNC(dead_event)
{
	const SCharDeadEventInfo* info = dynamic_cast<SCharDeadEventInfo*>(event->info);

	if (info == nullptr)
	{
		SysLog("dead_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = nullptr;

	if (info->isPC)
	{
		ch = CHARACTER_MANAGER::GetInstance()->FindByPID(info->dwID);
	}
	else
	{
		ch = CHARACTER_MANAGER::GetInstance()->Find(info->dwID);
	}

	if (!ch)
	{
		SysLog("DEAD_EVENT: cannot find char pointer with {} id({})", info->isPC ? "PC" : "MOB", info->dwID);
		return 0;
	}

	ch->m_pDeadEvent = nullptr;

	if (ch->GetDesc())
	{
		ch->GetDesc()->SetPhase(PHASE_GAME);

		ch->SetPosition(POS_STANDING);

		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
			ch->WarpSet(pos.x, pos.y);
		else
		{
			SysLog("cannot find spawn position (name {})", ch->GetName());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}

		ch->PointChange(POINT_HP, (ch->GetMaxHP() / 2) - ch->GetHP(), true);

		ch->DeathPenalty(0);

		ch->StartRecoveryEvent();

		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
	}
	else
	{
		if (ch->IsMonster())
		{
			if (ch->IsRevive() == false && ch->HasReviverInParty())
			{
				ch->SetPosition(POS_STANDING);
				ch->SetHP(ch->GetMaxHP());

				ch->ViewReencode();

				ch->SetAggressive();
				ch->SetRevive(true);

				return 0;
			}
		}

		M2_DESTROY_CHARACTER(ch);
	}

	return 0;
}

bool CHARACTER::IsDead() const
{
	if (m_pointsInstant.position == POS_DEAD)
		return true;

	return false;
}

#define GetGoldMultipler() (distribution_test_server ? 3 : 1)

void CHARACTER::RewardGold(LPCHARACTER pAttacker)
{
	// ADD_PREMIUM
	bool isAutoLoot = 
		(pAttacker->GetPremiumRemainSeconds(PREMIUM_AUTOLOOT) > 0 ||
		 pAttacker->IsEquipUniqueGroup(UNIQUE_GROUP_AUTOLOOT))
		? true : false;
	// END_OF_ADD_PREMIUM

	PIXEL_POSITION pos;

	if (!isAutoLoot)
		if (!SECTREE_MANAGER::GetInstance()->GetMovablePosition(GetMapIndex(), GetX(), GetY(), pos))
			return;

	int32_t iTotalGold = 0;
	
	int32_t iGoldPercent = MobRankStats[GetMobRank()].iGoldPercent;

	if (pAttacker->IsPC())
		iGoldPercent = iGoldPercent * (100 + CPrivManager::GetInstance()->GetPriv(pAttacker, PRIV_GOLD_DROP)) / 100;

	if (pAttacker->GetPoint(POINT_MALL_GOLDBONUS))
		iGoldPercent += (iGoldPercent* pAttacker->GetPoint(POINT_MALL_GOLDBONUS) / 100);

	iGoldPercent = iGoldPercent * CHARACTER_MANAGER::GetInstance()->GetMobGoldDropRate(pAttacker) / 100;

	// ADD_PREMIUM
	if (pAttacker->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0 ||
			pAttacker->IsEquipUniqueGroup(UNIQUE_GROUP_LUCKY_GOLD))
		iGoldPercent += iGoldPercent;
	// END_OF_ADD_PREMIUM

	if (iGoldPercent > 100) 
		iGoldPercent = 100;

	int32_t iPercent;

	if (GetMobRank() >= MOB::RANK_BOSS)
		iPercent = ((iGoldPercent * PERCENT_LVDELTA_BOSS(pAttacker->GetLevel(), GetLevel())) / 100);
	else
		iPercent = ((iGoldPercent * PERCENT_LVDELTA(pAttacker->GetLevel(), GetLevel())) / 100);
	//int32_t iPercent = CALCULATE_VALUE_LVDELTA(pAttacker->GetLevel(), GetLevel(), iGoldPercent);

	if (number(1, 100) > iPercent)
		return;

	int32_t iGoldMultipler = GetGoldMultipler();

	if (1 == number(1, 50000)) 
		iGoldMultipler *= 10;
	else if (1 == number(1, 10000))
		iGoldMultipler *= 5;

	if (pAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
		if (number(1, 100) <= pAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
			iGoldMultipler *= 2;

	if (test_server)
		pAttacker->ChatPacket(CHAT_TYPE_PARTY, "gold_mul %d rate %d", iGoldMultipler, CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(pAttacker));

	LPITEM item;

	int32_t iGold10DropPct = 100;
	iGold10DropPct = (iGold10DropPct * 100) / (100 + CPrivManager::GetInstance()->GetPriv(pAttacker, PRIV_GOLD10_DROP));

	if (GetMobRank() >= MOB::RANK_BOSS && !IsStone() && GetMobTable().dwGoldMax != 0)
	{
		if (1 == number(1, iGold10DropPct))
			iGoldMultipler *= 10;

		int32_t iSplitCount = number(25, 35);

		for (int32_t i = 0; i < iSplitCount; ++i)
		{
			int32_t iGold = number(GetMobTable().dwGoldMin, GetMobTable().dwGoldMax) / iSplitCount;
			if (test_server)
				PyLog("iGold {}", iGold);
			iGold = iGold * CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(pAttacker) / 100;
			iGold *= iGoldMultipler;

			if (iGold == 0)
			{
				continue ;
			}

			if (test_server)
			{
				PyLog("Drop Moeny MobGoldAmountRate {} {}", CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(pAttacker), iGoldMultipler);
				PyLog("Drop Money gold {} GoldMin {} GoldMax {}", iGold, GetMobTable().dwGoldMax, GetMobTable().dwGoldMax);
			}

			// NOTE: Money Bombs don't do third hand handling
			if ((item = ITEM_MANAGER::GetInstance()->CreateItem(1, iGold)))
			{
				pos.x = GetX() + ((number(-14, 14) + number(-14, 14)) * 23);
				pos.y = GetY() + ((number(-14, 14) + number(-14, 14)) * 23);

				item->AddToGround(GetMapIndex(), pos);
				item->StartDestroyEvent();

				iTotalGold += iGold; // Total gold
			}
		}
	}
	// 1% chance to drop 10 money. (10x drop)
	else if (1 == number(1, iGold10DropPct))
	{
		//
		// money bomb drop
		//
		for (int32_t i = 0; i < 10; ++i)
		{
			int32_t iGold = number(GetMobTable().dwGoldMin, GetMobTable().dwGoldMax);
			iGold = iGold * CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(pAttacker) / 100;
			iGold *= iGoldMultipler;

			if (iGold == 0)
			{
				continue;
			}

			// NOTE: Money Bombs don't do third hand handling
			if ((item = ITEM_MANAGER::GetInstance()->CreateItem(1, iGold)))
			{
				pos.x = GetX() + (number(-7, 7) * 20);
				pos.y = GetY() + (number(-7, 7) * 20);

				item->AddToGround(GetMapIndex(), pos);
				item->StartDestroyEvent();

				iTotalGold += iGold; // Total gold
			}
		}
	}
	else
	{
		int32_t iGold = number(GetMobTable().dwGoldMin, GetMobTable().dwGoldMax);
		iGold = iGold * CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(pAttacker) / 100;
		iGold *= iGoldMultipler;

		int32_t iSplitCount;

		if (iGold >= 3) 
			iSplitCount = number(1, 3);
		else if (GetMobRank() >= MOB::RANK_BOSS)
		{
			iSplitCount = number(3, 10);

			if ((iGold / iSplitCount) == 0)
				iSplitCount = 1;
		}
		else
			iSplitCount = 1;

		if (iGold != 0)
		{
			iTotalGold += iGold; // Total gold

			for (int32_t i = 0; i < iSplitCount; ++i)
			{
				if (isAutoLoot)
				{
					pAttacker->GiveGold(iGold / iSplitCount);
				}
				else if ((item = ITEM_MANAGER::GetInstance()->CreateItem(1, iGold / iSplitCount)))
				{
					pos.x = GetX() + (number(-7, 7) * 20);
					pos.y = GetY() + (number(-7, 7) * 20);

					item->AddToGround(GetMapIndex(), pos);
					item->StartDestroyEvent();
				}
			}
		}
	}

	DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_MONSTER, GetRaceNum(), iTotalGold);
}

void CHARACTER::Reward(bool bItemDrop)
{
	if (GetRaceNum() == 5001)
	{
		PIXEL_POSITION pos;

		if (!SECTREE_MANAGER::GetInstance()->GetMovablePosition(GetMapIndex(), GetX(), GetY(), pos))
			return;

		LPITEM item;
		int32_t iGold = number(GetMobTable().dwGoldMin, GetMobTable().dwGoldMax);
		iGold = iGold * CHARACTER_MANAGER::GetInstance()->GetMobGoldAmountRate(nullptr) / 100;
		iGold *= GetGoldMultipler();
		int32_t iSplitCount = number(25, 35);

		PyLog("WAEGU Dead gold {} split {}", iGold, iSplitCount);

		for (int32_t i = 1; i <= iSplitCount; ++i)
		{
			if ((item = ITEM_MANAGER::GetInstance()->CreateItem(1, iGold / iSplitCount)))
			{
				if (i != 0)
				{
					pos.x = number(-7, 7) * 20;
					pos.y = number(-7, 7) * 20;

					pos.x += GetX();
					pos.y += GetY();
				}

				item->AddToGround(GetMapIndex(), pos);
				item->StartDestroyEvent();
			}
		}
		return;
	}

	//PROF_UNIT puReward("Reward");
   	LPCHARACTER pAttacker = DistributeExp();

	if (!pAttacker)
		return;

	//PROF_UNIT pu1("r1");
	if (pAttacker->IsPC())
	{
		if (GetLevel() - pAttacker->GetLevel() >= -10)
			if (pAttacker->GetRealAlignment() < 0)
			{
				if (pAttacker->IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_KILL))
					pAttacker->UpdateAlignment(14);
				else
					pAttacker->UpdateAlignment(7);
			}
			else
				pAttacker->UpdateAlignment(2);

		pAttacker->SetQuestNPCID(GetVID());
		quest::CQuestManager::GetInstance()->Kill(pAttacker->GetPlayerID(), GetRaceNum());
		CHARACTER_MANAGER::GetInstance()->KillLog(GetRaceNum());

		if (!number(0, 9))
		{
			if (pAttacker->GetPoint(POINT_KILL_HP_RECOVERY))
			{
				int32_t iHP = pAttacker->GetMaxHP()* pAttacker->GetPoint(POINT_KILL_HP_RECOVERY) / 100;
				pAttacker->PointChange(POINT_HP, iHP);
				CreateFly(FLY_HP_SMALL, pAttacker);
			}

			if (pAttacker->GetPoint(POINT_KILL_SP_RECOVER))
			{
				int32_t iSP = pAttacker->GetMaxSP()* pAttacker->GetPoint(POINT_KILL_SP_RECOVER) / 100;
				pAttacker->PointChange(POINT_SP, iSP);
				CreateFly(FLY_SP_SMALL, pAttacker);
			}
		}
	}
	//pu1.Pop();

	if (!bItemDrop)
		return;

	PIXEL_POSITION pos = GetXYZ();

	if (!SECTREE_MANAGER::GetInstance()->GetMovablePosition(GetMapIndex(), pos.x, pos.y, pos))
		return;

	
	//PROF_UNIT pu2("r2");
	if (test_server)
		PyLog("Drop money : Attacker {}", pAttacker->GetName());
	RewardGold(pAttacker);
	//pu2.Pop();

	
	//PROF_UNIT pu3("r3");
	LPITEM item;

	static std::vector<LPITEM> s_vec_item;
	s_vec_item.clear();

	if (ITEM_MANAGER::GetInstance()->CreateDropItem(this, pAttacker, s_vec_item))
	{
		if (s_vec_item.size() == 0);
		else if (s_vec_item.size() == 1)
		{
			item = s_vec_item[0];
			item->AddToGround(GetMapIndex(), pos);

			if (CBattleArena::GetInstance()->IsBattleArenaMap(pAttacker->GetMapIndex()) == false)
			{
				item->SetOwnership(pAttacker);
			}

			item->StartDestroyEvent();

			pos.x = number(-7, 7) * 20;
			pos.y = number(-7, 7) * 20;
			pos.x += GetX();
			pos.y += GetY();

			PyLog("DROP_ITEM: {} {} {} from {}", item->GetName(), pos.x, pos.y, GetName());
		}
		else
		{
			int32_t iItemIdx = s_vec_item.size() - 1;

			std::priority_queue<std::pair<int32_t, LPCHARACTER> > pq;

			int32_t total_dam = 0;

			for (TDamageMap::iterator it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
			{
				int32_t iDamage = it->second.iTotalDamage;
				if (iDamage > 0)
				{
					LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(it->first);

					if (ch)
					{
						pq.push(std::make_pair(iDamage, ch));
						total_dam += iDamage;
					}
				}
			}

			std::vector<LPCHARACTER> v;

			while (!pq.empty() && pq.top().first * 10 >= total_dam)
			{
				v.push_back(pq.top().second);
				pq.pop();
			}

			if (v.empty())
			{
				while (iItemIdx >= 0)
				{
					item = s_vec_item[iItemIdx--];

					if (!item)
					{
						SysLog("item null in vector idx {}", iItemIdx + 1);
						continue;
					}

					item->AddToGround(GetMapIndex(), pos);
					item->StartDestroyEvent();

					pos.x = number(-7, 7) * 20;
					pos.y = number(-7, 7) * 20;
					pos.x += GetX();
					pos.y += GetY();

					PyLog("DROP_ITEM: {} {} {} by {}", item->GetName(), pos.x, pos.y, GetName());
				}
			}
			else
			{
				std::vector<LPCHARACTER>::iterator it = v.begin();

				while (iItemIdx >= 0)
				{
					item = s_vec_item[iItemIdx--];

					if (!item)
					{
						SysLog("item null in vector idx {}", iItemIdx + 1);
						continue;
					}

					item->AddToGround(GetMapIndex(), pos);

					LPCHARACTER ch = *it;

					if (ch->GetParty())
						ch = ch->GetParty()->GetNextOwnership(ch, GetX(), GetY());

					++it;

					if (it == v.end())
						it = v.begin();

					if (CBattleArena::GetInstance()->IsBattleArenaMap(ch->GetMapIndex()) == false)
					{
						item->SetOwnership(ch);
					}

					item->StartDestroyEvent();

					pos.x = number(-7, 7) * 20;
					pos.y = number(-7, 7) * 20;
					pos.x += GetX();
					pos.y += GetY();

					PyLog("DROP_ITEM: {} {} {} by {}", item->GetName(), pos.x, pos.y, GetName());
				}
			}
		}
	}

	m_map_kDamage.clear();
}

struct TItemDropPenalty
{
	int32_t iInventoryPct;		// Range: 1 ~ 1000
	int32_t iInventoryQty;		// Range: --
	int32_t iEquipmentPct;		// Range: 1 ~ 100
	int32_t iEquipmentQty;		// Range: --
};

TItemDropPenalty aItemDropPenalty_kor[9] =
{
	{   0,   0,  0,  0 },
	{   0,   0,  0,  0 },
	{   0,   0,  0,  0 },
	{   0,   0,  0,  0 },
	{   0,   0,  0,  0 },
	{  25,   1,  5,  1 },
	{  50,   2, 10,  1 },
	{  75,   4, 15,  1 },
	{ 100,   8, 20,  1 },
};

void CHARACTER::ItemDropPenalty(LPCHARACTER pKiller)
{
	if (GetMyShop())
		return;

	if (GetLevel() < 50)
		return;
	
	if (CBattleArena::GetInstance()->IsBattleArenaMap(GetMapIndex()))
	{
		return;
	}

	struct TItemDropPenalty * table = &aItemDropPenalty_kor[0];

	if (GetLevel() < 10)
		return;

	int32_t iAlignIndex;

	if (GetRealAlignment() >= 120000)
		iAlignIndex = 0;
	else if (GetRealAlignment() >= 80000)
		iAlignIndex = 1;
	else if (GetRealAlignment() >= 40000)
		iAlignIndex = 2;
	else if (GetRealAlignment() >= 10000)
		iAlignIndex = 3;
	else if (GetRealAlignment() >= 0)
		iAlignIndex = 4;
	else if (GetRealAlignment() > -40000)
		iAlignIndex = 5;
	else if (GetRealAlignment() > -80000)
		iAlignIndex = 6;
	else if (GetRealAlignment() > -120000)
		iAlignIndex = 7;
	else
		iAlignIndex = 8;

	std::vector<std::pair<LPITEM, int32_t> > vec_item;
	LPITEM pItem;
	int32_t	i;
	bool isDropAllEquipments = false;

	TItemDropPenalty& r = table[iAlignIndex];
	PyLog("{} align {} inven_pct {} equip_pct {}", GetName(), iAlignIndex, r.iInventoryPct, r.iEquipmentPct);

	bool bDropInventory = r.iInventoryPct >= number(1, 1000);
	bool bDropEquipment = r.iEquipmentPct >= number(1, 100);
	bool bDropAntiDropUniqueItem = false;

	if ((bDropInventory || bDropEquipment) && IsEquipUniqueItem(UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY))
	{
		bDropInventory = false;
		bDropEquipment = false;
		bDropAntiDropUniqueItem = true;
	}

	if (bDropInventory) // Drop Inventory
	{
		std::vector<uint8_t> vec_bSlots;

		for (i = 0; i < INVENTORY_MAX_NUM; ++i)
			if (GetInventoryItem(i))
				vec_bSlots.push_back(i);

		if (!vec_bSlots.empty())
		{
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(vec_bSlots.begin(), vec_bSlots.end(), g);

			int32_t iQty = MIN(vec_bSlots.size(), r.iInventoryQty);

			if (iQty)
				iQty = number(1, iQty);

			for (i = 0; i < iQty; ++i)
			{
				pItem = GetInventoryItem(vec_bSlots[i]);

				if (IS_SET(pItem->GetAntiFlag(), ITEM::ANTIFLAG_GIVE | ITEM::ANTIFLAG_PKDROP))
					continue;

				SyncQuickslot(QUICKSLOT_TYPE_ITEM, vec_bSlots[i], 255);
				vec_item.push_back(std::make_pair(pItem->RemoveFromCharacter(), INVENTORY));
			}
		}
		else if (iAlignIndex == 8)
			isDropAllEquipments = true;
	}

	if (bDropEquipment) // Drop Equipment
	{
		std::vector<uint8_t> vec_bSlots;

		for (i = 0; i < WEAR_MAX_NUM; ++i)
			if (GetWear(i))
				vec_bSlots.push_back(i);

		if (!vec_bSlots.empty())
		{
			std::random_device rd;
			std::mt19937 g(rd());
			std::shuffle(vec_bSlots.begin(), vec_bSlots.end(), g);
			int32_t iQty;

			if (isDropAllEquipments)
				iQty = vec_bSlots.size();
			else
				iQty = MIN(vec_bSlots.size(), number(1, r.iEquipmentQty));

			if (iQty)
				iQty = number(1, iQty);

			for (i = 0; i < iQty; ++i)
			{
				pItem = GetWear(vec_bSlots[i]);

				if (IS_SET(pItem->GetAntiFlag(), ITEM::ANTIFLAG_GIVE | ITEM::ANTIFLAG_PKDROP))
					continue;

				SyncQuickslot(QUICKSLOT_TYPE_ITEM, vec_bSlots[i], 255);
				vec_item.push_back(std::make_pair(pItem->RemoveFromCharacter(), EQUIPMENT));
			}
		}
	}

	if (bDropAntiDropUniqueItem)
	{
		LPITEM pItem;

		pItem = GetWear(WEAR_UNIQUE1);

		if (pItem && pItem->GetVnum() == UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY)
		{
			SyncQuickslot(QUICKSLOT_TYPE_ITEM, WEAR_UNIQUE1, 255);
			vec_item.push_back(std::make_pair(pItem->RemoveFromCharacter(), EQUIPMENT));
		}

		pItem = GetWear(WEAR_UNIQUE2);

		if (pItem && pItem->GetVnum() == UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY)
		{
			SyncQuickslot(QUICKSLOT_TYPE_ITEM, WEAR_UNIQUE2, 255);
			vec_item.push_back(std::make_pair(pItem->RemoveFromCharacter(), EQUIPMENT));
		}
	}

	{
		PIXEL_POSITION pos;
		pos.x = GetX();
		pos.y = GetY();

		uint32_t i;

		for (i = 0; i < vec_item.size(); ++i)
		{
			LPITEM item = vec_item[i].first;
			int32_t window = vec_item[i].second;

			item->AddToGround(GetMapIndex(), pos);
			item->StartDestroyEvent();

			PyLog("DROP_ITEM_p: {} {} {} from {}", item->GetName(), pos.x, pos.y, GetName());
			LogManager::GetInstance()->ItemLog(this, item, "DEAD_DROP", (window == INVENTORY) ? "INVENTORY" : ((window == EQUIPMENT) ? "EQUIPMENT" : ""));

			pos.x = GetX() + number(-7, 7) * 20;
			pos.y = GetY() + number(-7, 7) * 20;
		}
	}
}

class FPartyAlignmentCompute
{
	public:
		FPartyAlignmentCompute(int32_t iAmount, int32_t x, int32_t y)
		{
			m_iAmount = iAmount;
			m_iCount = 0;
			m_iStep = 0;
			m_iKillerX = x;
			m_iKillerY = y;
		}

		void operator () (LPCHARACTER pChr)
		{
			if (DISTANCE_APPROX(pChr->GetX() - m_iKillerX, pChr->GetY() - m_iKillerY) < PARTY_DEFAULT_RANGE)
			{
				if (m_iStep == 0)
				{
					++m_iCount;
				}
				else
				{
					pChr->UpdateAlignment(m_iAmount / m_iCount);
				}
			}
		}

		int32_t m_iAmount;
		int32_t m_iCount;
		int32_t m_iStep;

		int32_t m_iKillerX;
		int32_t m_iKillerY;
};



void CHARACTER::Dead(LPCHARACTER pKiller, bool bImmediateDead)
{
	if (IsDead())
		return;

	{
		if (IsHorseRiding())
		{
			StopRiding();
		}
		else if (GetMountVnum())
		{
			RemoveAffect(AFFECT_MOUNT_BONUS);
			m_dwMountVnum = 0;
			UnEquipSpecialRideUniqueItem();

			UpdatePacket();
		}
	}

	if (!pKiller && m_dwKillerPID)
		pKiller = CHARACTER_MANAGER::GetInstance()->FindByPID(m_dwKillerPID);

	m_dwKillerPID = 0; // Must be initialized DO NOT DELETE THIS LINE UNLESS YOU ARE 1000000% SURE

	bool isAgreedPVP = false;
	bool isUnderGuildWar = false;
	bool isDuel = false;
	bool isForked = false;

	if (pKiller && pKiller->IsPC())
	{
		if (pKiller->m_pChrTarget == this)
			pKiller->SetTarget(nullptr);

		if (!IsPC() && pKiller->GetDungeon())
			pKiller->GetDungeon()->IncKillCount(pKiller, this);

		isAgreedPVP = CPVPManager::GetInstance()->Dead(this, pKiller->GetPlayerID());
		isDuel = CArenaManager::GetInstance()->OnDead(pKiller, this);

		if (IsPC())
		{
			CGuild * g1 = GetGuild();
			CGuild * g2 = pKiller->GetGuild();

			if (g1 && g2)
				if (g1->UnderWar(g2->GetID()))
					isUnderGuildWar = true;

			pKiller->SetQuestNPCID(GetVID());
			quest::CQuestManager::GetInstance()->Kill(pKiller->GetPlayerID(), quest::QUEST_NO_NPC);
			CGuildManager::GetInstance()->Kill(pKiller, this);
		}
	}

	//CHECK_FORKEDROAD_WAR
	if (IsPC())
	{
		if (CThreeWayWar::GetInstance()->IsThreeWayWarMapIndex(GetMapIndex()))
			isForked = true;
	}
	//END_CHECK_FORKEDROAD_WAR

	if (pKiller &&
			!isAgreedPVP &&
			!isUnderGuildWar &&
			IsPC() &&
			!isDuel &&
			!isForked &&
			!IS_CASTLE_MAP(GetMapIndex()))
	{
		if (GetGMLevel() == GM_PLAYER || test_server)
		{
			ItemDropPenalty(pKiller);
		}
	}

	// CASTLE_SIEGE
	if (IS_CASTLE_MAP(GetMapIndex()))
	{
		if (CASTLE_FROG_VNUM == GetRaceNum())
			castle_frog_die(this, pKiller);
		else if (castle_is_guard_vnum(GetRaceNum()))
			castle_guard_die(this, pKiller);
		else if (castle_is_tower_vnum(GetRaceNum()))
			castle_tower_die(this, pKiller);
	}
	// CASTLE_SIEGE

	if (isForked)
	{
		CThreeWayWar::GetInstance()->onDead(this, pKiller);
	}

	SetPosition(POS_DEAD);
	ClearAffect(true);

	if (pKiller && IsPC())
	{
		if (!pKiller->IsPC())
		{
			if (!isForked)
			{
				TraceLog("DEAD: {} WITH PENALTY", GetName());
				SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
				LogManager::GetInstance()->CharLog(this, pKiller->GetRaceNum(), "DEAD_BY_NPC", pKiller->GetName());
			}
		}
		else
		{
			TraceLog("DEAD_BY_PC: {} KILLER {}", GetName(), pKiller->GetName());
			REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);

			if (GetEmpire() != pKiller->GetEmpire())
			{
				int32_t iEP = MIN(GetPoint(POINT_EMPIRE_POINT), pKiller->GetPoint(POINT_EMPIRE_POINT));

				PointChange(POINT_EMPIRE_POINT, -(iEP / 10));
				pKiller->PointChange(POINT_EMPIRE_POINT, iEP / 5);

				if (GetPoint(POINT_EMPIRE_POINT) < 10)
				{
					// TODO: You need to put the code that blows to the entrance.
				}

				char buf[256];
				snprintf(buf, sizeof(buf),
						"%d %d %d %s %d %d %d %s",
						GetEmpire(), GetAlignment(), GetPKMode(), GetName(),
						pKiller->GetEmpire(), pKiller->GetAlignment(), pKiller->GetPKMode(), pKiller->GetName());

				LogManager::GetInstance()->CharLog(this, pKiller->GetPlayerID(), "DEAD_BY_PC", buf);
			}
			else
			{
				if (!isAgreedPVP && !isUnderGuildWar && !IsKillerMode() && GetAlignment() >= 0 && !isDuel && !isForked)
				{
					int32_t iNoPenaltyProb = 0;

					if (pKiller->GetAlignment() >= 0)	// 1/3 percent down
						iNoPenaltyProb = 33;
					else				// 4/5 percent down
						iNoPenaltyProb = 20;

					if (number(1, 100) < iNoPenaltyProb)
						pKiller->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not drop any Item(s) as you are protected by the Dragon God."));
					else
					{
						if (pKiller->GetParty())
						{
							FPartyAlignmentCompute f(-20000, pKiller->GetX(), pKiller->GetY());
							pKiller->GetParty()->ForEachOnlineMember(f);

							if (f.m_iCount == 0)
								pKiller->UpdateAlignment(-20000);
							else
							{
								PyLog("ALIGNMENT PARTY count {} amount {}", f.m_iCount, f.m_iAmount);

								f.m_iStep = 1;
								pKiller->GetParty()->ForEachOnlineMember(f);
							}
						}
						else
							pKiller->UpdateAlignment(-20000);
					}
				}

				char buf[256];
				snprintf(buf, sizeof(buf),
						"%d %d %d %s %d %d %d %s",
						GetEmpire(), GetAlignment(), GetPKMode(), GetName(),
						pKiller->GetEmpire(), pKiller->GetAlignment(), pKiller->GetPKMode(), pKiller->GetName());

				LogManager::GetInstance()->CharLog(this, pKiller->GetPlayerID(), "DEAD_BY_PC", buf);
			}
		}
	}
	else
	{
		TraceLog("DEAD: %s", GetName());
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
	}

	ClearSync();

	//TraceLog("stun cancel {}[{}]", GetName(), (uint32_t)GetVID());
	event_cancel(&m_pStunEvent);

	if (IsPC())
	{
		m_dwLastDeadTime = get_dword_time();
		SetKillerMode(false);
		GetDesc()->SetPhase(PHASE_DEAD);
	}
	else
	{
		if (!IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD))
		{
			if (!(pKiller && pKiller->IsPC() && pKiller->GetGuild() && pKiller->GetGuild()->UnderAnyWar(GUILD_WAR_TYPE_FIELD)))
			{
				if (GetMobTable().dwResurrectionVnum)
				{
					// DUNGEON_MONSTER_REBIRTH_BUG_FIX
					LPCHARACTER chResurrect = CHARACTER_MANAGER::GetInstance()->SpawnMob(GetMobTable().dwResurrectionVnum, GetMapIndex(), GetX(), GetY(), GetZ(), true, (int32_t) GetRotation());
					if (GetDungeon() && chResurrect)
					{
						chResurrect->SetDungeon(GetDungeon());
					}
					// END_OF_DUNGEON_MONSTER_REBIRTH_BUG_FIX

					Reward(false);
				}
				else if (IsRevive())
				{
					Reward(false);
				}
				else
				{
					Reward(true); // Drops gold, item, etc..
				}
			}
			else
			{
				if (pKiller->m_dwUnderGuildWarInfoMessageTime < get_dword_time())
				{
					pKiller->m_dwUnderGuildWarInfoMessageTime = get_dword_time() + 60000;
					pKiller->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] There are no experience points for hunting during a guild war."));
				}
			}
		}
	}

	// BOSS_KILL_LOG
	if (GetMobRank() >= MOB::RANK_BOSS && pKiller && pKiller->IsPC())
	{
		char buf[51];
		snprintf(buf, sizeof(buf), "%d %ld", g_bChannel, pKiller->GetMapIndex());
		if (IsStone())
			LogManager::GetInstance()->CharLog(pKiller, GetRaceNum(), "STONE_KILL", buf);
		else
			LogManager::GetInstance()->CharLog(pKiller, GetRaceNum(), "BOSS_KILL", buf);
	}
	// END_OF_BOSS_KILL_LOG

	TPacketGCDead pack;
	pack.header	= HEADER_GC_DEAD;
	pack.vid	= m_vid;
	PacketAround(&pack, sizeof(pack));

	REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (GetDesc() != nullptr) {
		auto it = m_list_pAffect.begin();

		while (it != m_list_pAffect.end())
			SendAffectAddPacket(GetDesc(), *it++);
	}
	if (isDuel == false)
	{
		if (m_pDeadEvent)
		{
			TraceLog("DEAD_EVENT_CANCEL: {}", GetName());
			event_cancel(&m_pDeadEvent);
		}

		if (IsStone())
			ClearStone();

		if (GetDungeon())
		{
			GetDungeon()->DeadCharacter(this);
		}

		SCharDeadEventInfo* pEventInfo = AllocEventInfo<SCharDeadEventInfo>();

		if (IsPC())
		{
			pEventInfo->isPC = true;
			pEventInfo->dwID = this->GetPlayerID();

			m_pDeadEvent = event_create(dead_event, pEventInfo, PASSES_PER_SEC(180));
		}
		else
		{
			pEventInfo->isPC = false;
			pEventInfo->dwID = this->GetVID();

			if (IsRevive() == false && HasReviverInParty())
			{
				m_pDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(3));
			}
			else
			{
				m_pDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(10));
			}
		}

		TraceLog("DEAD_EVENT_CREATE: {}", GetName());
	}

	if (m_pExchange != nullptr)
	{
		m_pExchange->Cancel();
	}

	if (IsCubeOpen())
	{
		Cube_close(this);
	}
	
	CShopManager::GetInstance()->StopShopping(this);
	CloseMyShop();
	CloseSafebox();

	if (IsMonster() && 2493 == GetMobTable().dwVnum)
	{
		if (NULL != pKiller && NULL != pKiller->GetGuild())
		{
			CDragonLairManager::GetInstance()->OnDragonDead(this, pKiller->GetGuild()->GetID());
		}
		else
		{
			SysLog("DragonLair: Dragon killed by nobody");
		}
	}
}

struct FuncSetLastAttacked
{
	FuncSetLastAttacked(uint32_t dwTime) : m_dwTime(dwTime)
	{
	}

	void operator () (LPCHARACTER ch)
	{
		ch->SetLastAttacked(m_dwTime);
	}

	uint32_t m_dwTime;
};

void CHARACTER::SetLastAttacked(uint32_t dwTime)
{
	assert(m_pMobInst != nullptr);

	m_pMobInst->m_dwLastAttackedTime = dwTime;
	m_pMobInst->m_posLastAttacked = GetXYZ();
}

void CHARACTER::SendDamagePacket(LPCHARACTER pAttacker, int32_t Damage, uint8_t DamageFlag)
{
	if (IsPC() || (pAttacker->IsPC() && pAttacker->GetTarget() == this))
	{
		TPacketGCDamageInfo damageInfo;
		memset(&damageInfo, 0, sizeof(TPacketGCDamageInfo));

		damageInfo.header = HEADER_GC_DAMAGE_INFO;
		damageInfo.dwVID = (uint32_t)GetVID();
		damageInfo.flag = DamageFlag;
		damageInfo.damage = Damage;

		if (GetDesc() != nullptr)
		{
			GetDesc()->Packet(&damageInfo, sizeof(TPacketGCDamageInfo));
		}

		if (pAttacker->GetDesc() != nullptr)
		{
			pAttacker->GetDesc()->Packet(&damageInfo, sizeof(TPacketGCDamageInfo));
		}
		/*
		   if (GetArenaObserverMode() == false && GetArena() != nullptr)
		   {
		   GetArena()->SendPacketToObserver(&damageInfo, sizeof(TPacketGCDamageInfo));
		   }
		 */		
	}
}
 
bool CHARACTER::Damage(LPCHARACTER pAttacker, int32_t dam, EDamageType type) // returns true if dead
{
	if (DAMAGE_TYPE_MAGIC == type)
	{
		dam = (int32_t)((float)dam * (100 + (pAttacker->GetPoint(POINT_MAGIC_ATT_BONUS_PER) + pAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100.f + 0.5f);
	}
	if (GetRaceNum() == 5001)
	{
		bool bDropMoney = false;
		int32_t iPercent = (GetHP() * 100) / GetMaxHP();

		if (iPercent <= 10 && GetMaxSP() < 5)
		{
			SetMaxSP(5);
			bDropMoney = true;
		}
		else if (iPercent <= 20 && GetMaxSP() < 4)
		{
			SetMaxSP(4);
			bDropMoney = true;
		}
		else if (iPercent <= 40 && GetMaxSP() < 3)
		{
			SetMaxSP(3);
			bDropMoney = true;
		}
		else if (iPercent <= 60 && GetMaxSP() < 2)
		{
			SetMaxSP(2);
			bDropMoney = true;
		}
		else if (iPercent <= 80 && GetMaxSP() < 1)
		{
			SetMaxSP(1);
			bDropMoney = true;
		}

		if (bDropMoney)
		{
			uint32_t dwGold = 1000;
			int32_t iSplitCount = number(10, 13);

			PyLog("WAEGU DropGoldOnHit {} times", GetMaxSP());

			for (int32_t i = 1; i <= iSplitCount; ++i)
			{
				PIXEL_POSITION pos;
				LPITEM item;

				if ((item = ITEM_MANAGER::GetInstance()->CreateItem(1, dwGold / iSplitCount)))
				{
					if (i != 0)
					{
						pos.x = (number(-14, 14) + number(-14, 14)) * 20;
						pos.y = (number(-14, 14) + number(-14, 14)) * 20;

						pos.x += GetX();
						pos.y += GetY();
					}

					item->AddToGround(GetMapIndex(), pos);
					item->StartDestroyEvent();
				}
			}
		}
	}

	
	if (type != DAMAGE_TYPE_NORMAL && type != DAMAGE_TYPE_NORMAL_RANGE)
	{
		if (IsAffectFlag(AFF_TERROR))
		{
			int32_t pct = GetSkillPower(SKILL_TERROR) / 400;

			if (number(1, 100) <= pct)
				return false;
		}
	}

	int32_t iCurHP = GetHP();
	int32_t iCurSP = GetSP();

	bool IsCritical = false;
	bool IsPenetrate = false;
	bool IsDeathBlow = false;

	enum DamageFlag
	{
		DAMAGE_NORMAL	= (1 << 0),
		DAMAGE_POISON	= (1 << 1),
		DAMAGE_DODGE	= (1 << 2),
		DAMAGE_BLOCK	= (1 << 3),
		DAMAGE_PENETRATE= (1 << 4),
		DAMAGE_CRITICAL = (1 << 5),
	};

	//PROF_UNIT puAttr("Attr");

	//
	// Magical skills and range-type skills (guests) calculate critical and penetrating attacks.
	// It shouldn't have been done, but I can't do the Nerf (down balance) patch, so it's critical and
	// Do not use the original value of the penetrating attack, and apply more than /2.
	//
	// Because there are many stories of warriors, melee skill is also added
	//
	// 20091109: It is concluded that the warrior has become incredibly strong as a result, and the ratio of the warrior is close to 70% by German standards.
	//
	if (type == DAMAGE_TYPE_MELEE || type == DAMAGE_TYPE_RANGE || type == DAMAGE_TYPE_MAGIC)
	{
		if (pAttacker)
		{
			int32_t iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (!IsPC())
				iCriticalPct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_CRITICAL_BONUS);

			if (iCriticalPct)
			{
				if (iCriticalPct >= 10)
					iCriticalPct = 5 + (iCriticalPct - 10) / 4;
				else
					iCriticalPct /= 2;

				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (number(1, 100) <= iCriticalPct)
				{
					IsCritical = true;
					dam *= 2;
					EffectPacket(SE_CRITICAL);

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}
				}
			}

			int32_t iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (!IsPC())
				iPenetratePct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_PENETRATE_BONUS);


			if (iPenetratePct)
			{
				{
					CSkillProto* pSk = CSkillManager::GetInstance()->Get(SKILL_RESIST_PENETRATE);

					if (NULL != pSk)
					{
						pSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

						iPenetratePct -= static_cast<int32_t>(pSk->kPointPoly.Eval());
					}
				}

				if (iPenetratePct >= 10)
				{
					iPenetratePct = 5 + (iPenetratePct - 10) / 4;
				}
				else
				{
					iPenetratePct /= 2;
				}

				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Additional Stabbing Weapon Damage %d"), GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100);

					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}
				}
			}
		}
	}
	else if (type == DAMAGE_TYPE_NORMAL || type == DAMAGE_TYPE_NORMAL_RANGE)
	{
		if (type == DAMAGE_TYPE_NORMAL)
		{
			if (GetPoint(POINT_BLOCK) && number(1, 100) <= GetPoint(POINT_BLOCK))
			{
				if (test_server)
				{
					pAttacker->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s block! (%ld%%)"), GetName(), GetPoint(POINT_BLOCK));
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s block! (%ld%%)"), GetName(), GetPoint(POINT_BLOCK));
				}

				SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
				return false;
			}
		}
		else if (type == DAMAGE_TYPE_NORMAL_RANGE)
		{
			if (GetPoint(POINT_DODGE) && number(1, 100) <= GetPoint(POINT_DODGE))
			{
				if (test_server)
				{
					pAttacker->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s avoid! (%ld%%)"), GetName(), GetPoint(POINT_DODGE));
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s avoid! (%ld%%)"), GetName(), GetPoint(POINT_DODGE));
				}

				SendDamagePacket(pAttacker, 0, DAMAGE_DODGE);
				return false;
			}
		}

		if (IsAffectFlag(AFF_JEONGWIHON))
			dam = (int32_t) (dam * (100 + GetSkillPower(SKILL_JEONGWI) * 25 / 100) / 100);

		if (IsAffectFlag(AFF_TERROR))
			dam = (int32_t) (dam * (95 - GetSkillPower(SKILL_TERROR) / 5) / 100);

		if (IsAffectFlag(AFF_HOSIN))
			dam = dam * (100 - GetPoint(POINT_RESIST_NORMAL_DAMAGE)) / 100;

		
		if (pAttacker)
		{
			if (type == DAMAGE_TYPE_NORMAL)
			{
				if (GetPoint(POINT_REFLECT_MELEE))
				{
					int32_t reflectDamage = dam * GetPoint(POINT_REFLECT_MELEE) / 100;

					// NOTE: If the attacker has the IMMUNE_REFLECT attribute, it is not reflected
					// Instead, the plan requested that it be fixed at 1/3 damage.
					if (pAttacker->IsImmune(ITEM::IMMUNE_REFLECT))
						reflectDamage = int32_t(reflectDamage / 3.0f + 0.5f);

					pAttacker->Damage(this, reflectDamage, DAMAGE_TYPE_SPECIAL);
				}
			}

			int32_t iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (!IsPC())
				iCriticalPct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_CRITICAL_BONUS);

			if (iCriticalPct)
			{
				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (number(1, 100) <= iCriticalPct)
				{
					IsCritical = true;
					dam *= 2;
					EffectPacket(SE_CRITICAL);
				}
			}

			int32_t iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (!IsPC())
				iPenetratePct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_PENETRATE_BONUS);

			{
				CSkillProto* pSk = CSkillManager::GetInstance()->Get(SKILL_RESIST_PENETRATE);

				if (NULL != pSk)
				{
					pSk->SetPointVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

					iPenetratePct -= static_cast<int32_t>(pSk->kPointPoly.Eval());
				}
			}


			if (iPenetratePct)
			{
				
				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Additional Stabbing Weapon Damage %d"), GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100);
					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;
				}
			}

			if (pAttacker->GetPoint(POINT_STEAL_HP))
			{
				int32_t pct = 1;

				if (number(1, 10) <= pct)
				{
					int32_t iHP = MIN(dam, MAX(0, iCurHP))* pAttacker->GetPoint(POINT_STEAL_HP) / 100;

					if (iHP > 0 && GetHP() >= iHP)
					{
						CreateFly(FLY_HP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_HP, iHP);
						PointChange(POINT_HP, -iHP);
					}
				}
			}

			if (pAttacker->GetPoint(POINT_STEAL_SP))
			{
				int32_t pct = 1;

				if (number(1, 10) <= pct)
				{
					int32_t iCur;

					if (IsPC())
						iCur = iCurSP;
					else
						iCur = iCurHP;

					int32_t iSP = MIN(dam, MAX(0, iCur))* pAttacker->GetPoint(POINT_STEAL_SP) / 100;

					if (iSP > 0 && iCur >= iSP)
					{
						CreateFly(FLY_SP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_SP, iSP);

						if (IsPC())
							PointChange(POINT_SP, -iSP);
					}
				}
			}

			if (pAttacker->GetPoint(POINT_STEAL_GOLD))
			{
				if (number(1, 100) <= pAttacker->GetPoint(POINT_STEAL_GOLD))
				{
					int32_t iAmount = number(1, GetLevel());
					pAttacker->PointChange(POINT_GOLD, iAmount);
					DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_MISC, 1, iAmount);
				}
			}

			if (pAttacker->GetPoint(POINT_HIT_HP_RECOVERY) && number(0, 4) > 0)
			{
				int32_t i = MIN(dam, iCurHP)* pAttacker->GetPoint(POINT_HIT_HP_RECOVERY) / 100;

				if (i)
				{
					CreateFly(FLY_HP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_HP, i);
				}
			}

			if (pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) && number(0, 4) > 0)
			{
				int32_t i = MIN(dam, iCurHP)* pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) / 100;

				if (i)
				{
					CreateFly(FLY_SP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_SP, i);
				}
			}

			if (pAttacker->GetPoint(POINT_MANA_BURN_PCT))
			{
				if (number(1, 100) <= pAttacker->GetPoint(POINT_MANA_BURN_PCT))
					PointChange(POINT_SP, -50);
			}
		}
	}

	 
	switch (type)
	{
		case DAMAGE_TYPE_NORMAL:
		case DAMAGE_TYPE_NORMAL_RANGE:
			if (pAttacker)
				if (pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS))
					dam = dam * (100 + pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS)) / 100;

			dam = dam * (100 - MIN(99, GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS))) / 100;
			break;

		case DAMAGE_TYPE_MELEE:
		case DAMAGE_TYPE_RANGE:
		case DAMAGE_TYPE_FIRE:
		case DAMAGE_TYPE_ICE:
		case DAMAGE_TYPE_ELEC:
		case DAMAGE_TYPE_MAGIC:
			if (pAttacker)
				if (pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS))
					dam = dam * (100 + pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS)) / 100;

			dam = dam * (100 - MIN(99, GetPoint(POINT_SKILL_DEFEND_BONUS))) / 100;
			break;

		default:
			break;
	}

	
	if (IsAffectFlag(AFF_MANASHIELD))
	{
		int32_t iDamageSPPart = dam / 3;
		int32_t iDamageToSP = iDamageSPPart * GetPoint(POINT_MANASHIELD) / 100;
		int32_t iSP = GetSP();

		if (iDamageToSP <= iSP)
		{
			PointChange(POINT_SP, -iDamageToSP);
			dam -= iDamageSPPart;
		}
		else
		{
			PointChange(POINT_SP, -GetSP());
			dam -= iSP * 100 / MAX(GetPoint(POINT_MANASHIELD), 1);
		}
	}
 
	if (GetPoint(POINT_MALL_DEFBONUS) > 0)
	{
		int32_t dec_dam = MIN(200, dam * GetPoint(POINT_MALL_DEFBONUS) / 100);
		dam -= dec_dam;
	}

	if (pAttacker)
	{

		if (pAttacker->GetPoint(POINT_MALL_ATTBONUS) > 0)
		{
			int32_t add_dam = MIN(300, dam* pAttacker->GetLimitPoint(POINT_MALL_ATTBONUS) / 100);
			dam += add_dam;
		}

		int32_t iEmpire = GetEmpire();
		int32_t lMapIndex = GetMapIndex();
		int32_t iMapEmpire = SECTREE_MANAGER::GetInstance()->GetEmpireFromMapIndex(lMapIndex);

		if (pAttacker->IsPC())
		{
			iEmpire = pAttacker->GetEmpire();
			lMapIndex = pAttacker->GetMapIndex();
			iMapEmpire = SECTREE_MANAGER::GetInstance()->GetEmpireFromMapIndex(lMapIndex);

			if (iEmpire && iMapEmpire && iEmpire != iMapEmpire)
			{
				int32_t percent = 10;
				
				if (184 <= lMapIndex && lMapIndex <= 189)
				{
					percent = 9;
				}
				else
				{
					percent = 9;
				}

				dam = dam* percent / 10;
			}

			if (!IsPC() && GetMonsterDrainSPPoint())
			{
				int32_t iDrain = GetMonsterDrainSPPoint();

				if (iDrain <= pAttacker->GetSP())
					pAttacker->PointChange(POINT_SP, -iDrain);
				else
				{
					int32_t iSP = pAttacker->GetSP();
					pAttacker->PointChange(POINT_SP, -iSP);
				}
			}

		}
		else if (pAttacker->IsGuardNPC())
		{
			SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD);
			Stun();
			return true;
		}

		if (pAttacker->IsPC() && CMonarch::GetInstance()->IsPowerUp(pAttacker->GetEmpire()))
		{
			dam += dam / 10;
		}

		if (IsPC() && CMonarch::GetInstance()->IsDefenceUp(GetEmpire()))
		{
			dam -= dam / 10;
		}
	}
	//puAttr.Pop();

	if (!GetSectree() || GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
		return false;

	if (!IsPC())
	{
		if (m_pParty && m_pParty->GetLeader())
			m_pParty->GetLeader()->SetLastAttacked(get_dword_time());
		else
			SetLastAttacked(get_dword_time());

		MonsterChat(MONSTER_CHAT_ATTACKED);
	}

	if (IsStun())
	{
		Dead(pAttacker);
		return true;
	}

	if (IsDead())
		return true;

	if (type == DAMAGE_TYPE_POISON)
	{
		if (GetHP() - dam <= 0)
		{
			dam = GetHP() - 1;
		}
	}

	int32_t iDmgPct = CHARACTER_MANAGER::GetInstance()->GetUserDamageRate(pAttacker);
	dam = dam * iDmgPct / 100;

	if (IsMonster() && IsStoneSkinner())
	{
		if (GetHPPct() < GetMobTable().bStoneSkinPoint)
			dam /= 2;
	}

	//PROF_UNIT puRest1("Rest1");
	if (pAttacker)
	{
		if (pAttacker->IsMonster() && pAttacker->IsDeathBlower())
		{
			if (pAttacker->IsDeathBlow())
			{
				if (number(1, 4) == GetJob())
				{
					IsDeathBlow = true;
					dam = dam * 4;
				}
			}
		}

		dam = BlueDragon_Damage(this, pAttacker, dam);

		uint8_t damageFlag = 0;

		if (type == DAMAGE_TYPE_POISON)
			damageFlag = DAMAGE_POISON;
		else
			damageFlag = DAMAGE_NORMAL;

		if (IsCritical)
			damageFlag |= DAMAGE_CRITICAL;

		if (IsPenetrate)
			damageFlag |= DAMAGE_PENETRATE;


		float damMul = this->GetDamMul();
		float tempDam = dam;
		dam = tempDam * damMul + 0.5f;


		if (pAttacker)
			SendDamagePacket(pAttacker, dam, damageFlag);

		if (test_server)
		{
			if(pAttacker)
			{
				pAttacker->ChatPacket(CHAT_TYPE_INFO, "-> %s, DAM %d HP %d(%d%%) %s%s",
						GetName(), 
						dam, 
						GetHP(),
						(GetHP() * 100) / GetMaxHP(),
						IsCritical ? "crit " : "",
						IsPenetrate ? "pene " : "",
						IsDeathBlow ? "deathblow " : "");
			}

			ChatPacket(CHAT_TYPE_PARTY, "<- %s, DAM %d HP %d(%d%%) %s%s",
					pAttacker ? pAttacker->GetName() : 0,
					dam, 
					GetHP(),
					(GetHP() * 100) / GetMaxHP(),
					IsCritical ? "crit " : "",
					IsPenetrate ? "pene " : "",
					IsDeathBlow ? "deathblow " : "");
		}

		if (m_bDetailLog)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s[%ld]'s Attack Position: %d %d"), pAttacker->GetName(), (uint32_t) pAttacker->GetVID(), pAttacker->GetX(), pAttacker->GetY());
		}
	}

	if (!cannot_dead)
	{
		PointChange(POINT_HP, -dam, false);
	}

	//puRest1.Pop();

	//PROF_UNIT puRest2("Rest2");
	if (pAttacker && dam > 0 && IsNPC())
	{
		//PROF_UNIT puRest20("Rest20");
		TDamageMap::iterator it = m_map_kDamage.find(pAttacker->GetVID());

		if (it == m_map_kDamage.end())
		{
			m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(dam, 0)));
			it = m_map_kDamage.find(pAttacker->GetVID());
		}
		else
		{
			it->second.iTotalDamage += dam;
		}
		//puRest20.Pop();

		//PROF_UNIT puRest21("Rest21");
		StartRecoveryEvent();
		//puRest21.Pop();

		//PROF_UNIT puRest22("Rest22");
		UpdateAggrPointEx(pAttacker, type, dam, it->second);
		//puRest22.Pop();
	}
	//puRest2.Pop();

	//PROF_UNIT puRest3("Rest3");
	if (GetHP() <= 0)
	{
		Stun();

		if (pAttacker && !pAttacker->IsNPC())
			m_dwKillerPID = pAttacker->GetPlayerID();
		else
			m_dwKillerPID = 0;
	}

	return false;
}

void CHARACTER::DistributeHP(LPCHARACTER pKiller)
{
	if (pKiller->GetDungeon()) 
		return;
}

static void GiveExp(LPCHARACTER from, LPCHARACTER to, int32_t iExp)
{
	iExp = CALCULATE_VALUE_LVDELTA(to->GetLevel(), from->GetLevel(), iExp);

	if (distribution_test_server)
		iExp *= 3;

	int32_t iBaseExp = iExp;

	iExp = iExp * (100 + CPrivManager::GetInstance()->GetPriv(to, PRIV_EXP_PCT)) / 100;

	{
		if (to->IsEquipUniqueItem(UNIQUE_ITEM_LARBOR_MEDAL))
			iExp += iExp * 20 /100;

		if (to->GetMapIndex() >= 660000 && to->GetMapIndex() < 670000) 
			iExp += iExp * 20 / 100; // 1.2 (20%)

		if (to->GetPoint(POINT_EXP_DOUBLE_BONUS))
			if (number(1, 100) <= to->GetPoint(POINT_EXP_DOUBLE_BONUS))
				iExp += iExp * 30 / 100; // 1.3 (30%)

		if (to->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_EXP))
			iExp += iExp * 50 / 100;

		switch (to->GetMountVnum())
		{
			case 20110:
			case 20111:
			case 20112:
			case 20113:
				if (to->IsEquipUniqueItem(71115) || to->IsEquipUniqueItem(71117) || to->IsEquipUniqueItem(71119) ||
						to->IsEquipUniqueItem(71121))
				{
					iExp += iExp * 10 / 100;
				}
				break;

			case 20114:
			case 20120:
			case 20121:
			case 20122:
			case 20123:
			case 20124:
			case 20125:

				iExp += iExp * 30 / 100;
				break;
		}
	}

	{
		if (to->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		{
			iExp += (iExp * 50 / 100);
		}

		if (to->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_EXP))
		{
			iExp += (iExp * 50 / 100);
		}

		iExp += iExp * to->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_EXP_BONUS) / 100;
	}

	iExp += (iExp * to->GetPoint(POINT_RAMADAN_CANDY_BONUS_EXP)/100);
	iExp += (iExp * to->GetPoint(POINT_MALL_EXPBONUS)/100);
	iExp += (iExp * to->GetPoint(POINT_EXP)/100);

	if (test_server)
	{
		PyLog("Bonus Exp : Ramadan Candy: {} MallExp: {} PointExp: {}", 
				to->GetPoint(POINT_RAMADAN_CANDY_BONUS_EXP),
				to->GetPoint(POINT_MALL_EXPBONUS),
				to->GetPoint(POINT_EXP)
			);
	}

	iExp = iExp * CHARACTER_MANAGER::GetInstance()->GetMobExpRate(to) / 100;

	iExp = MIN(to->GetNextExp() / 10, iExp);

	if (test_server)
	{
		if (quest::CQuestManager::GetInstance()->GetEventFlag("exp_bonus_log") && iBaseExp>0)
			to->ChatPacket(CHAT_TYPE_INFO, "exp bonus %d%%", (iExp-iBaseExp)*100/iBaseExp);
	}

	iExp = AdjustExpByLevel(to, iExp);

	to->PointChange(POINT_EXP, iExp, true);
	from->CreateFly(FLY_EXP, to);

	{
		LPCHARACTER you = to->GetMarryPartner();
		if (you)
		{
			uint32_t dwUpdatePoint = 2000*iExp/to->GetLevel()/to->GetLevel()/3;

			if (to->GetPremiumRemainSeconds(PREMIUM_MARRIAGE_FAST) > 0 || 
					you->GetPremiumRemainSeconds(PREMIUM_MARRIAGE_FAST) > 0)
				dwUpdatePoint = (uint32_t)(dwUpdatePoint * 3);

			marriage::TMarriage* pMarriage = marriage::CManager::GetInstance()->Get(to->GetPlayerID());

			// DIVORCE_NULL_BUG_FIX
			if (pMarriage && pMarriage->IsNear())
				pMarriage->Update(dwUpdatePoint);
			// END_OF_DIVORCE_NULL_BUG_FIX
		}
	}
}

namespace NPartyExpDistribute
{
	struct FPartyTotaler
	{
		int32_t		total;
		int32_t		member_count;
		int32_t		x, y;

		FPartyTotaler(LPCHARACTER center)
			: total(0), member_count(0), x(center->GetX()), y(center->GetY())
		{};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				total += party_exp_distribute_table[ch->GetLevel()];

				++member_count;
			}
		}
	};

	struct FPartyDistributor
	{
		int32_t		total;
		LPCHARACTER	c;
		int32_t		x, y;
		uint32_t		_iExp;
		int32_t		m_iMode;
		int32_t		m_iMemberCount;

		FPartyDistributor(LPCHARACTER center, int32_t member_count, int32_t total, uint32_t iExp, int32_t iMode) 
			: total(total), c(center), x(center->GetX()), y(center->GetY()), _iExp(iExp), m_iMode(iMode), m_iMemberCount(member_count)
			{
				if (m_iMemberCount == 0)
					m_iMemberCount = 1;
			};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				uint32_t iExp2 = 0;

				switch (m_iMode)
				{
					case PARTY_EXP_DISTRIBUTION_NON_PARITY:
						iExp2 = (uint32_t) (_iExp * (float) party_exp_distribute_table[ch->GetLevel()] / total);
						break;

					case PARTY_EXP_DISTRIBUTION_PARITY:
						iExp2 = _iExp / m_iMemberCount;
						break;

					default:
						SysLog("Unknown party exp distribution mode {}", m_iMode);
						return;
				}

				GiveExp(c, ch, iExp2);
			}
		}
	};
}

typedef struct SDamageInfo
{
	int32_t iDam;
	LPCHARACTER pAttacker;
	LPPARTY pParty;

	void Clear()
	{
		pAttacker = nullptr;
		pParty = nullptr;
	}

	inline void Distribute(LPCHARACTER ch, int32_t iExp)
	{
		if (pAttacker)
			GiveExp(ch, pAttacker, iExp);
		else if (pParty)
		{
			NPartyExpDistribute::FPartyTotaler f(ch);
			pParty->ForEachOnlineMember(f);

			if (pParty->IsPositionNearLeader(ch))
				iExp = iExp * (100 + pParty->GetExpBonusPercent()) / 100;

			if (test_server)
			{
				if (quest::CQuestManager::GetInstance()->GetEventFlag("exp_bonus_log") && pParty->GetExpBonusPercent())
					pParty->ChatPacketToAllMember(CHAT_TYPE_INFO, "exp party bonus %d%%", pParty->GetExpBonusPercent());
			}

			if (pParty->GetExpCentralizeCharacter())
			{
				LPCHARACTER tch = pParty->GetExpCentralizeCharacter();

				if (DISTANCE_APPROX(ch->GetX() - tch->GetX(), ch->GetY() - tch->GetY()) <= PARTY_DEFAULT_RANGE)
				{
					int32_t iExpCenteralize = (int32_t) (iExp * 0.05f);
					iExp -= iExpCenteralize;

					GiveExp(ch, pParty->GetExpCentralizeCharacter(), iExpCenteralize);
				}
			}

			NPartyExpDistribute::FPartyDistributor fDist(ch, f.member_count, f.total, iExp, pParty->GetExpDistributionMode());
			pParty->ForEachOnlineMember(fDist);
		}
	}
} TDamageInfo;

LPCHARACTER CHARACTER::DistributeExp()
{
	int32_t iExpToDistribute = GetExp();

	if (iExpToDistribute <= 0)
		return NULL;

	int32_t	iTotalDam = 0;
	LPCHARACTER pChrMostAttacked = nullptr;
	int32_t iMostDam = 0;

	typedef std::vector<TDamageInfo> TDamageInfoTable;
	TDamageInfoTable damage_info_table;
	std::map<LPPARTY, TDamageInfo> map_party_damage;

	damage_info_table.reserve(m_map_kDamage.size());

	TDamageMap::iterator it = m_map_kDamage.begin();

	while (it != m_map_kDamage.end())
	{
		const VID& c_VID = it->first;
		int32_t iDam = it->second.iTotalDamage;

		++it;

		LPCHARACTER pAttacker = CHARACTER_MANAGER::GetInstance()->Find(c_VID);

		if (!pAttacker || pAttacker->IsNPC() || DISTANCE_APPROX(GetX()-pAttacker->GetX(), GetY()-pAttacker->GetY())>5000)
			continue;

		iTotalDam += iDam;
		if (!pChrMostAttacked || iDam > iMostDam)
		{
			pChrMostAttacked = pAttacker;
			iMostDam = iDam;
		}

		if (pAttacker->GetParty())
		{
			std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.find(pAttacker->GetParty());
			if (it == map_party_damage.end())
			{
				TDamageInfo di;
				di.iDam = iDam;
				di.pAttacker = nullptr;
				di.pParty = pAttacker->GetParty();
				map_party_damage.insert(std::make_pair(di.pParty, di));
			}
			else
			{
				it->second.iDam += iDam;
			}
		}
		else
		{
			TDamageInfo di;

			di.iDam = iDam;
			di.pAttacker = pAttacker;
			di.pParty = nullptr;

			//PyLog("__ pq_damage {} {}", pAttacker->GetName(), iDam);
			//pq_damage.push(di);
			damage_info_table.push_back(di);
		}
	}

	for (std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.begin(); it != map_party_damage.end(); ++it)
	{
		damage_info_table.push_back(it->second);
		//PyLog("__ pq_damage_party [{}] {}", it->second.pParty->GetLeaderPID(), it->second.iDam);
	}

	SetExp(0);
	//m_map_kDamage.clear();

	if (iTotalDam == 0)
		return NULL;

	if (m_pChrStone)
	{
		//PyLog("__ Give half to Stone : {}", iExpToDistribute>>1);
		int32_t iExp = iExpToDistribute >> 1;
		m_pChrStone->SetExp(m_pChrStone->GetExp() + iExp);
		iExpToDistribute -= iExp;
	}

	TraceLog("{} total exp: {}, damage_info_table.size() == {}, TotalDam {}",
			GetName(), iExpToDistribute, damage_info_table.size(), iTotalDam);
	//TraceLog("{} total exp: {}, pq_damage.size() == {}, TotalDam {}",
	//GetName(), iExpToDistribute, pq_damage.size(), iTotalDam);

	if (damage_info_table.empty())
		return NULL;

	DistributeHP(pChrMostAttacked);

	{
		TDamageInfoTable::iterator di = damage_info_table.begin();
		{
			TDamageInfoTable::iterator it;

			for (it = damage_info_table.begin(); it != damage_info_table.end();++it)
			{
				if (it->iDam > di->iDam)
					di = it;
			}
		}

		int32_t	iExp = iExpToDistribute / 5;
		iExpToDistribute -= iExp;

		float fPercent = (float) di->iDam / iTotalDam;

		if (fPercent > 1.0f)
		{
			SysLog("DistributeExp percent over 1.0 (fPercent {} name {})", fPercent, di->pAttacker->GetName());
			fPercent = 1.0f;
		}

		iExp += (int32_t) (iExpToDistribute * fPercent);

		//PyLog("{} given exp percent %.1f + 20 dam {}", GetName(), fPercent * 100.0f, di.iDam);

		di->Distribute(this, iExp);

		if (fPercent == 1.0f)
			return pChrMostAttacked;

		di->Clear();
	}

	{
		TDamageInfoTable::iterator it;

		for (it = damage_info_table.begin(); it != damage_info_table.end(); ++it)
		{
			TDamageInfo & di = *it;

			float fPercent = (float) di.iDam / iTotalDam;

			if (fPercent > 1.0f)
			{
				SysLog("DistributeExp percent over 1.0 (fPercent {} name {})", fPercent, di.pAttacker->GetName());
				fPercent = 1.0f;
			}

			di.Distribute(this, (int32_t) (iExpToDistribute * fPercent));
		}
	}

	return pChrMostAttacked;
}

int32_t CHARACTER::GetArrowAndBow(LPITEM* ppkBow, LPITEM* ppkArrow, int32_t iArrowCount/* = 1 */)
{
	LPITEM pBow;

	if (!(pBow = GetWear(WEAR_WEAPON)) || pBow->GetProto()->bSubType != ITEM::WEAPON_BOW)
	{
		return 0;
	}

	LPITEM pArrow;

	if (!(pArrow = GetWear(WEAR_ARROW)) || pArrow->GetType() != ITEM::TYPE_WEAPON ||
			pArrow->GetProto()->bSubType != ITEM::WEAPON_ARROW)
	{
		return 0;
	}

	iArrowCount = MIN(iArrowCount, pArrow->GetCount());

	*ppkBow = pBow;
	*ppkArrow = pArrow;

	return iArrowCount;
}

void CHARACTER::UseArrow(LPITEM pArrow, uint32_t dwArrowCount)
{
	int32_t iCount = pArrow->GetCount();
	uint32_t dwVnum = pArrow->GetVnum();
	iCount = iCount - MIN(iCount, dwArrowCount);
	pArrow->SetCount(iCount);

	if (iCount == 0)
	{
		LPITEM pNewArrow = FindSpecifyItem(dwVnum);

		PyLog("UseArrow : FindSpecifyItem {}", dwVnum);

		if (pNewArrow)
			EquipItem(pNewArrow);
	}
}

class CFuncShoot
{
	public:
		LPCHARACTER	m_me;
		uint8_t		m_bType;
		bool		m_bSucceed;

		CFuncShoot(LPCHARACTER ch, uint8_t bType) : m_me(ch), m_bType(bType), m_bSucceed(FALSE)
		{
		}

		void operator () (uint32_t dwTargetVID)
		{
			if (m_bType > 1)
			{
				if (g_bSkillDisable)
					return;

				m_me->m_SkillUseInfo[m_bType].SetMainTargetVID(dwTargetVID);
				/*if (m_bType == SKILL_BIPABU || m_bType == SKILL_KWANKYEOK)
				  m_me->m_SkillUseInfo[m_bType].ResetHitCount();*/
			}

			LPCHARACTER pVictim = CHARACTER_MANAGER::GetInstance()->Find(dwTargetVID);

			if (!pVictim)
				return;

			if (!battle_is_attackable(m_me, pVictim))
				return;

			if (m_me->IsNPC())
			{
				if (DISTANCE_APPROX(m_me->GetX() - pVictim->GetX(), m_me->GetY() - pVictim->GetY()) > 5000)
					return;
			}

			LPITEM pBow, pArrow;

			switch (m_bType)
			{
				case 0:
					{
						int32_t iDam = 0;

						if (m_me->IsPC())
						{
							if (m_me->GetJob() != JOB_ASSASSIN)
								return;

							if (0 == m_me->GetArrowAndBow(&pBow, &pArrow))
								return;

							if (m_me->GetSkillGroup() != 0)
								if (!m_me->IsNPC() && m_me->GetSkillGroup() != 2)
								{
									if (m_me->GetSP() < 5)
										return;

									m_me->PointChange(POINT_SP, -5);
								}

							iDam = CalcArrowDamage(m_me, pVictim, pBow, pArrow);
							m_me->UseArrow(pArrow, 1);

							// check speed hack
							uint32_t	dwCurrentTime	= get_dword_time();
							if (IS_SPEED_HACK(m_me, pVictim, dwCurrentTime))
								iDam	= 0;
						}
						else
							iDam = CalcMeleeDamage(m_me, pVictim);

						NormalAttackAffect(m_me, pVictim);

						iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_BOW)) / 100;

						//PyLog("{} arrow {} dam {}", m_me->GetName(), pVictim->GetName(), iDam);

						m_me->OnMove(true);
						pVictim->OnMove();

						if (pVictim->CanBeginFight())
							pVictim->BeginFight(m_me);

						pVictim->Damage(m_me, iDam, DAMAGE_TYPE_NORMAL_RANGE);
						
					}
					break;

				case 1:
					{
						int32_t iDam;

						if (m_me->IsPC())
							return;

						iDam = CalcMagicDamage(m_me, pVictim);

						NormalAttackAffect(m_me, pVictim);

						
						iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_MAGIC)) / 100;

						//PyLog("{} arrow {} dam {}", m_me->GetName(), pVictim->GetName(), iDam);

						m_me->OnMove(true);
						pVictim->OnMove();

						if (pVictim->CanBeginFight())
							pVictim->BeginFight(m_me); 

						pVictim->Damage(m_me, iDam, DAMAGE_TYPE_MAGIC);
						
					}
					break;

				case SKILL_YEONSA:
					{
						//int32_t iUseArrow = 2 + (m_me->GetSkillPower(SKILL_YEONSA) *6/100);
						int32_t iUseArrow = 1;

						{
							if (iUseArrow == m_me->GetArrowAndBow(&pBow, &pArrow, iUseArrow))
							{
								m_me->OnMove(true);
								pVictim->OnMove();

								if (pVictim->CanBeginFight())
									pVictim->BeginFight(m_me); 

								m_me->ComputeSkill(m_bType, pVictim);
								m_me->UseArrow(pArrow, iUseArrow);

								if (pVictim->IsDead())
									break;

							}
							else
								break;
						}
					}
					break;


				case SKILL_KWANKYEOK:
					{
						int32_t iUseArrow = 1;

						if (iUseArrow == m_me->GetArrowAndBow(&pBow, &pArrow, iUseArrow))
						{
							m_me->OnMove(true);
							pVictim->OnMove();

							if (pVictim->CanBeginFight())
								pVictim->BeginFight(m_me); 

							PyLog("{} kwankeyok {}", m_me->GetName(), pVictim->GetName());
							m_me->ComputeSkill(m_bType, pVictim);
							m_me->UseArrow(pArrow, iUseArrow);
						}
					}
					break;

				case SKILL_GIGUNG:
					{
						int32_t iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pBow, &pArrow, iUseArrow))
						{
							m_me->OnMove(true);
							pVictim->OnMove();

							if (pVictim->CanBeginFight())
								pVictim->BeginFight(m_me);

							PyLog("{} gigung {}", m_me->GetName(), pVictim->GetName());
							m_me->ComputeSkill(m_bType, pVictim);
							m_me->UseArrow(pArrow, iUseArrow);
						}
					}

					break;
				case SKILL_HWAJO:
					{
						int32_t iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pBow, &pArrow, iUseArrow))
						{
							m_me->OnMove(true);
							pVictim->OnMove();

							if (pVictim->CanBeginFight())
								pVictim->BeginFight(m_me);

							PyLog("{} hwajo {}", m_me->GetName(), pVictim->GetName());
							m_me->ComputeSkill(m_bType, pVictim);
							m_me->UseArrow(pArrow, iUseArrow);
						}
					}

					break;

				case SKILL_HORSE_WILDATTACK_RANGE:
					{
						int32_t iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pBow, &pArrow, iUseArrow))
						{
							m_me->OnMove(true);
							pVictim->OnMove();

							if (pVictim->CanBeginFight())
								pVictim->BeginFight(m_me);

							PyLog("{} horse_wildattack {}", m_me->GetName(), pVictim->GetName());
							m_me->ComputeSkill(m_bType, pVictim);
							m_me->UseArrow(pArrow, iUseArrow);
						}
					}

					break;

				case SKILL_MARYUNG:
					//case SKILL_GUMHWAN:
				case SKILL_TUSOK:
				case SKILL_BIPABU:
				case SKILL_NOEJEON:
				case SKILL_GEOMPUNG:
				case SKILL_SANGONG:
				case SKILL_MAHWAN:
				case SKILL_PABEOB:
					//case SKILL_CURSE:
					{
						m_me->OnMove(true);
						pVictim->OnMove();

						if (pVictim->CanBeginFight())
							pVictim->BeginFight(m_me);

						PyLog("{} - Skill {} -> {}", m_me->GetName(), m_bType, pVictim->GetName());
						m_me->ComputeSkill(m_bType, pVictim);
					}
					break;

				case SKILL_CHAIN:
					{
						m_me->OnMove(true);
						pVictim->OnMove();

						if (pVictim->CanBeginFight())
							pVictim->BeginFight(m_me); 

						PyLog("{} - Skill {} -> {}", m_me->GetName(), m_bType, pVictim->GetName());
						m_me->ComputeSkill(m_bType, pVictim);

						// TODO to multiple people
					}
					break;

				case SKILL_YONGBI:
					{
						m_me->OnMove(true);
					}
					break;

					/*case SKILL_BUDONG:
					  {
					  m_me->OnMove(true);
					  pVictim->OnMove();

					  uint32_t* pdw;
					  uint32_t dwEI = AllocEventInfo(sizeof(uint32_t) * 2, &pdw);
					  pdw[0] = m_me->GetVID();
					  pdw[1] = pVictim->GetVID();

					  event_create(budong_event_func, dwEI, PASSES_PER_SEC(1));
					  }
					  break;*/

				default:
					SysLog("CFuncShoot: I don't know this type [{}] of range attack.", (int32_t) m_bType);
					break;
			}

			m_bSucceed = TRUE;
		}
};

bool CHARACTER::Shoot(uint8_t bType)
{
	TraceLog("Shoot {} type {} flyTargets.size %zu", GetName(), bType, m_vec_dwFlyTargets.size());

	if (!CanMove())
	{
		return false;
	}	

	CFuncShoot f(this, bType);

	if (m_dwFlyTargetID != 0)
	{
		f(m_dwFlyTargetID);
		m_dwFlyTargetID = 0;
	}

	f = std::for_each(m_vec_dwFlyTargets.begin(), m_vec_dwFlyTargets.end(), f);
	m_vec_dwFlyTargets.clear();

	return f.m_bSucceed;
}

void CHARACTER::FlyTarget(uint32_t dwTargetVID, int32_t x, int32_t y, uint8_t bHeader)
{
	LPCHARACTER pVictim = CHARACTER_MANAGER::GetInstance()->Find(dwTargetVID);
	TPacketGCFlyTargeting pack;

	//pack.bHeader	= HEADER_GC_FLY_TARGETING;
	pack.bHeader	= (bHeader == HEADER_CG_FLY_TARGETING) ? HEADER_GC_FLY_TARGETING : HEADER_GC_ADD_FLY_TARGETING;
	pack.dwShooterVID	= GetVID();

	if (pVictim)
	{
		pack.dwTargetVID = pVictim->GetVID();
		pack.x = pVictim->GetX();
		pack.y = pVictim->GetY();

		if (bHeader == HEADER_CG_FLY_TARGETING)
			m_dwFlyTargetID = dwTargetVID;
		else
			m_vec_dwFlyTargets.push_back(dwTargetVID);
	}
	else
	{
		pack.dwTargetVID = 0;
		pack.x = x;
		pack.y = y;
	}

	TraceLog("FlyTarget {} vid {} x {} y {}", GetName(), pack.dwTargetVID, pack.x, pack.y);
	PacketAround(&pack, sizeof(pack), this);
}

LPCHARACTER CHARACTER::GetNearestVictim(LPCHARACTER pChr)
{
	if (!pChr)
		pChr = this;

	float fMinDist = 99999.0f;
	LPCHARACTER pVictim = nullptr;

	TDamageMap::iterator it = m_map_kDamage.begin();

	// First, filter out people who are not around.
	while (it != m_map_kDamage.end())
	{
		const VID& c_VID = it->first;
		++it;

		LPCHARACTER pAttacker = CHARACTER_MANAGER::GetInstance()->Find(c_VID);

		if (!pAttacker)
			continue;

		if (pAttacker->IsAffectFlag(AFF_EUNHYUNG) || 
				pAttacker->IsAffectFlag(AFF_INVISIBILITY) ||
				pAttacker->IsAffectFlag(AFF_REVIVE_INVISIBLE))
			continue;

		float fDist = DISTANCE_APPROX(pAttacker->GetX() - pChr->GetX(), pAttacker->GetY() - pChr->GetY());

		if (fDist < fMinDist)
		{
			pVictim = pAttacker;
			fMinDist = fDist;
		}
	}

	return pVictim;
}

void CHARACTER::SetVictim(LPCHARACTER pVictim)
{
	if (!pVictim)
	{
		if (0 != (uint32_t)m_kVIDVictim)
			MonsterLog("공격 대상을 해제");

		m_kVIDVictim.Reset();
		battle_end(this);
	}
	else
	{
		if (m_kVIDVictim != pVictim->GetVID())
			MonsterLog("공격 대상을 설정: %s", pVictim->GetName());

		m_kVIDVictim = pVictim->GetVID();
		m_dwLastVictimSetTime = get_dword_time();
	}
}

LPCHARACTER CHARACTER::GetVictim() const
{
	return CHARACTER_MANAGER::GetInstance()->Find(m_kVIDVictim);
}

LPCHARACTER CHARACTER::GetProtege() const // return the object to be protected
{
	if (m_pChrStone)
		return m_pChrStone;

	if (m_pParty)
		return m_pParty->GetLeader();

	return NULL;
}

int32_t CHARACTER::GetAlignment() const
{
	return m_iAlignment;
}

int32_t CHARACTER::GetRealAlignment() const
{
	return m_iRealAlignment;
}

void CHARACTER::ShowAlignment(bool bShow)
{
	if (bShow)
	{
		if (m_iAlignment != m_iRealAlignment)
		{
			m_iAlignment = m_iRealAlignment;
			UpdatePacket();
		}
	}
	else
	{
		if (m_iAlignment != 0)
		{
			m_iAlignment = 0;
			UpdatePacket();
		}
	}
}

void CHARACTER::UpdateAlignment(int32_t iAmount)
{
	bool bShow = false;

	if (m_iAlignment == m_iRealAlignment)
		bShow = true;

	int32_t i = m_iAlignment / 10;

	m_iRealAlignment = MINMAX(-200000, m_iRealAlignment + iAmount, 200000);

	if (bShow)
	{
		m_iAlignment = m_iRealAlignment;

		if (i != m_iAlignment / 10)
			UpdatePacket();
	}
}

void CHARACTER::SetKillerMode(bool isOn)
{
	if ((isOn ? ADD_CHARACTER_STATE_KILLER : 0) == IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER))
		return;

	if (isOn)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
	else
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);

	m_iKillerModePulse = thecore_pulse();
	UpdatePacket();
	PyLog("SetKillerMode Update {}[{}]", GetName(), GetPlayerID());
}

bool CHARACTER::IsKillerMode() const
{
	return IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
}

void CHARACTER::UpdateKillerMode()
{
	if (!IsKillerMode())
		return;

	int32_t iKillerSeconds = 30;

	if (thecore_pulse() - m_iKillerModePulse >= PASSES_PER_SEC(iKillerSeconds))
		SetKillerMode(false);
}

void CHARACTER::SetPKMode(uint8_t bPKMode)
{
	if (bPKMode >= PK_MODE_MAX_NUM)
		return;

	if (m_bPKMode == bPKMode)
		return;

	if (bPKMode == PK_MODE_GUILD && !GetGuild())
		bPKMode = PK_MODE_FREE;

	m_bPKMode = bPKMode;
	UpdatePacket();

	PyLog("PK_MODE: {} {}", GetName(), m_bPKMode);
}

uint8_t CHARACTER::GetPKMode() const
{
	return m_bPKMode;
}

struct FuncForgetMyAttacker
{
	LPCHARACTER m_ch;
	FuncForgetMyAttacker(LPCHARACTER ch)
	{
		m_ch = ch;
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (ch->m_kVIDVictim == m_ch->GetVID())
				ch->SetVictim(nullptr);
		}
	}
};

struct FuncAggregateMonster
{
	LPCHARACTER m_ch;
	FuncAggregateMonster(LPCHARACTER ch)
	{
		m_ch = ch;
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim())
				return;

			if (number(1, 100) <= 50) // Temporarily attracts enemies with a 50% chance
				if (DISTANCE_APPROX(ch->GetX() - m_ch->GetX(), ch->GetY() - m_ch->GetY()) < 5000)
					if (ch->CanBeginFight())
						ch->BeginFight(m_ch);
		}
	}
};

struct FuncAttractRanger
{
	LPCHARACTER m_ch;
	FuncAttractRanger(LPCHARACTER ch)
	{
		m_ch = ch;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim() && ch->GetVictim() != m_ch)
				return;
			if (ch->GetMobAttackRange() > 150)
			{
				int32_t iNewRange = 150;//(int32_t)(ch->GetMobAttackRange() * 0.2);
				if (iNewRange < 150)
					iNewRange = 150;

				ch->AddAffect(AFFECT_BOW_DISTANCE, POINT_BOW_DISTANCE, iNewRange - ch->GetMobAttackRange(), AFF_NONE, 3*60, 0, false);
			}
		}
	}
};

struct FuncPullMonster
{
	LPCHARACTER m_ch;
	int32_t m_iLength;
	FuncPullMonster(LPCHARACTER ch, int32_t iLength = 300)
	{
		m_ch = ch;
		m_iLength = iLength;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			//if (ch->GetVictim() && ch->GetVictim() != m_ch)
			//return;
			float fDist = DISTANCE_APPROX(m_ch->GetX() - ch->GetX(), m_ch->GetY() - ch->GetY());
			if (fDist > 3000 || fDist < 100)
				return;

			float fNewDist = fDist - m_iLength;
			if (fNewDist < 100) 
				fNewDist = 100;

			float degree = GetDegreeFromPositionXY(ch->GetX(), ch->GetY(), m_ch->GetX(), m_ch->GetY());
			float fx;
			float fy;

			GetDeltaByDegree(degree, fDist - fNewDist, &fx, &fy);
			int32_t tx = (int32_t)(ch->GetX() + fx);
			int32_t ty = (int32_t)(ch->GetY() + fy);

			ch->Sync(tx, ty);
			ch->Goto(tx, ty);
			ch->CalculateMoveDuration();

			ch->SyncPacket();
		}
	}
};

void CHARACTER::ForgetMyAttacker()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncForgetMyAttacker f(this);
		pSec->ForEachAround(f);
	}
	ReviveInvisible(5);
}

void CHARACTER::AggregateMonster()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncAggregateMonster f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::AttractRanger()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncAttractRanger f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::PullMonster()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncPullMonster f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::UpdateAggrPointEx(LPCHARACTER pAttacker, EDamageType type, int32_t dam, CHARACTER::TBattleInfo & info)
{
	// It goes up higher depending on the specific attack type.
	switch (type)
	{
		case DAMAGE_TYPE_NORMAL_RANGE:
			dam = (int32_t) (dam*1.2f);
			break;

		case DAMAGE_TYPE_RANGE:
			dam = (int32_t) (dam*1.5f);
			break;

		case DAMAGE_TYPE_MAGIC:
			dam = (int32_t) (dam*1.2f);
			break;

		default:
			break;
	}

	// Gives a bonus if the attacker is the current target.
	if (pAttacker == GetVictim())
		dam = (int32_t) (dam * 1.2f);

	info.iAggro += dam;

	if (info.iAggro < 0)
		info.iAggro = 0;

	//PyLog("UpdateAggrPointEx for {} by {} dam {} total {}", GetName(), pAttacker->GetName(), dam, total);
	if (GetParty() && dam > 0 && type != DAMAGE_TYPE_SPECIAL)
	{
		LPPARTY pParty = GetParty();

		// If you are a leader, you have more influence.
		int32_t iPartyAggroDist = dam;

		if (pParty->GetLeaderPID() == GetVID())
			iPartyAggroDist /= 2;
		else
			iPartyAggroDist /= 3;

		pParty->SendMessage(this, PM_AGGRO_INCREASE, iPartyAggroDist, pAttacker->GetVID());
	}

	ChangeVictimByAggro(info.iAggro, pAttacker);
}

void CHARACTER::UpdateAggrPoint(LPCHARACTER pAttacker, EDamageType type, int32_t dam)
{
	if (IsDead() || IsStun())
		return;

	TDamageMap::iterator it = m_map_kDamage.find(pAttacker->GetVID());

	if (it == m_map_kDamage.end())
	{
		m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(0, dam)));
		it = m_map_kDamage.find(pAttacker->GetVID());
	}

	UpdateAggrPointEx(pAttacker, type, dam, it->second);
}

void CHARACTER::ChangeVictimByAggro(int32_t iNewAggro, LPCHARACTER pNewVictim)
{
	if (get_dword_time() - m_dwLastVictimSetTime < 3000) // 3초는 기다려야한다
		return;

	if (pNewVictim == GetVictim())
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			return;
		}

		// Aggro if is decreased
		TDamageMap::iterator it;
		TDamageMap::iterator itFind = m_map_kDamage.end();

		for (it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
		{
			if (it->second.iAggro > iNewAggro)
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(it->first);

				if (ch && !ch->IsDead() && DISTANCE_APPROX(ch->GetX() - GetX(), ch->GetY() - GetY()) < 5000)
				{
					itFind = it;
					iNewAggro = it->second.iAggro;
				}
			}
		}

		if (itFind != m_map_kDamage.end())
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(CHARACTER_MANAGER::GetInstance()->Find(itFind->first));
			m_dwStateDuration = 1;
		}
	}
	else
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(pNewVictim);
			m_dwStateDuration = 1;
		}
	}
}

