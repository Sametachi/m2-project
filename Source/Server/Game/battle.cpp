#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "vector.h"
#include "packet.h"
#include "pvp.h"
#include "guild.h"
#include "affect.h"
#include "unique_item.h"
#include "lua_incl.h"
#include "arena.h"
#include "castle.h"
#include "sectree.h"
#include "ani.h"
#include "locale_service.h"

int32_t battle_hit(LPCHARACTER ch, LPCHARACTER victim, int32_t & iRetDam);

bool battle_distance_valid_by_xy(int32_t x, int32_t y, int32_t tx, int32_t ty)
{
	int32_t distance = DISTANCE_APPROX(x - tx, y - ty);

	if (distance > 170)
		return false;

	return true;
}

bool battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim)
{
	return battle_distance_valid_by_xy(ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY());
}

bool timed_event_cancel(LPCHARACTER ch)
{
	if (ch->m_pTimedEvent)
	{
		event_cancel(&ch->m_pTimedEvent);
		return true;
	}

	/* RECALL_DELAY
	   차후 전투로 인해 귀환부 딜레이가 취소 되어야 할 경우 주석 해제
	   if (ch->m_p_RecallEvent)
	   {
	   event_cancel(&ch->m_pRecallEvent);
	   return true;
	   }
	   END_OF_RECALL_DELAY */

	return false;
}

bool battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim)
{
	if (victim->IsDead())
		return false;

	{
		SECTREE	*sectree = nullptr;

		sectree	= ch->GetSectree();
		if (sectree && sectree->IsAttr(ch->GetX(), ch->GetY(), ATTR_BANPK))
			return false;

		sectree = victim->GetSectree();
		if (sectree && sectree->IsAttr(victim->GetX(), victim->GetY(), ATTR_BANPK))
			return false;
	}
	

	if (ch->IsStun() || ch->IsDead())
		return false;

	if (ch->IsPC() && victim->IsPC())
	{
		CGuild* g1 = ch->GetGuild();
		CGuild* g2 = victim->GetGuild();

		if (g1 && g2)
		{
			if (g1->UnderWar(g2->GetID()))
				return true;
		}
	}

	if (IS_CASTLE_MAP(ch->GetMapIndex()) && false==castle_can_attack(ch, victim))
			return false;

	if (CArenaManager::GetInstance()->CanAttack(ch, victim))
		return true;

	return CPVPManager::GetInstance()->CanAttack(ch, victim);
}

int32_t battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (test_server&&ch->IsPC())
		PyLog("battle_melee_attack : [{}] attack to [{}]", ch->GetName(), victim->GetName());

	if (!victim || ch == victim)
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		PyLog("battle_melee_attack : [{}] attack to [{}]", ch->GetName(), victim->GetName());

	if (!battle_is_attackable(ch, victim))
		return BATTLE_NONE;
	
	if (test_server&&ch->IsPC())
		PyLog("battle_melee_attack : [{}] attack to [{}]", ch->GetName(), victim->GetName());

	int32_t distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

	if (!victim->IsBuilding())
	{
		int32_t max = 300;
	
		if (!ch->IsPC())
		{
			// Use monster attack distance
			max = (int32_t) (ch->GetMobAttackRange() * 1.15f);
		}
		else
		{
			// If the opponent is a melee mob, the mob's attack range is the maximum attack distance.
			if (!victim->IsPC() && MOB::BATTLE_TYPE_MELEE == victim->GetMobBattleType())
				max = MAX(300, (int32_t) (victim->GetMobAttackRange() * 1.15f));
		}

		if (distance > max)
		{
			if (test_server)
				PyLog("VICTIM_FAR: {} distance: {} max: {}", ch->GetName(), distance, max);

			return BATTLE_NONE;
		}
	}

	if (timed_event_cancel(ch))
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	if (timed_event_cancel(victim))
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Action cancelled. You have entered a battle."));

	ch->SetPosition(POS_FIGHTING);
	ch->SetVictim(victim);

	const PIXEL_POSITION & vpos = victim->GetXYZ();
	ch->SetRotationToXY(vpos.x, vpos.y);

	int32_t dam;
	int32_t ret = battle_hit(ch, victim, dam);
	return (ret);
}


void battle_end_ex(LPCHARACTER ch)
{
	if (ch->IsPosition(POS_FIGHTING))
		ch->SetPosition(POS_STANDING);
}

void battle_end(LPCHARACTER ch)
{
	battle_end_ex(ch);
}

// AG = Attack Grade
// AL = Attack Limit
int32_t CalcBattleDamage(int32_t iDam, int32_t iAttackerLev, int32_t iVictimLev)
{
	if (iDam < 3)
		iDam = number(1, 5); 

	//return CALCULATE_DAMAGE_LVDELTA(iAttackerLev, iVictimLev, iDam);
	return iDam;
}

int32_t CalcMagicDamageWithValue(int32_t iDam, LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	return CalcBattleDamage(iDam, pAttacker->GetLevel(), pVictim->GetLevel());
}

int32_t CalcMagicDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	int32_t iDam = 0;

	if (pAttacker->IsNPC())
	{
		iDam = CalcMeleeDamage(pAttacker, pVictim, false, false);	
	}

	iDam += pAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);

	return CalcMagicDamageWithValue(iDam, pAttacker, pVictim);
}

float CalcAttackRating(LPCHARACTER pAttacker, LPCHARACTER pVictim, bool bIgnoreTargetRating)
{
	int32_t iARSrc;
	int32_t iERSrc;

	int32_t attacker_dx = pAttacker->GetPolymorphPoint(POINT_DX);
	int32_t attacker_lv = pAttacker->GetLevel();

	int32_t victim_dx = pVictim->GetPolymorphPoint(POINT_DX);
	int32_t victim_lv = pAttacker->GetLevel();

	iARSrc = MIN(90, (attacker_dx * 4	+ attacker_lv * 2) / 6);
	iERSrc = MIN(90, (victim_dx	  * 4	+ victim_lv   * 2) / 6);

	float fAR = ((float) iARSrc + 210.0f) / 300.0f; 

	if (bIgnoreTargetRating)
		return fAR;

	// ((Edx * 2 + 20) / (Edx + 110)) * 0.3
	float fER = ((float) (iERSrc * 2 + 5) / (iERSrc + 95)) * 3.0f / 10.0f;

	return fAR - fER;
}

int32_t CalcAttBonus(LPCHARACTER pAttacker, LPCHARACTER pVictim, int32_t iAtk)
{
	if (!pVictim->IsPC())
		iAtk += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_ATTACK_BONUS);

	if (!pAttacker->IsPC())
	{
		int32_t iReduceDamagePct = pVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_TRANSFER_DAMAGE);
		iAtk = iAtk * (100 + iReduceDamagePct) / 100;
	}

	if (pAttacker->IsNPC() && pVictim->IsPC())
	{
		iAtk = (iAtk * CHARACTER_MANAGER::GetInstance()->GetMobDamageRate(pAttacker)) / 100;
	}

	if (pVictim->IsNPC())
	{
		if (pVictim->IsRaceFlag(MOB::RACE_FLAG_ANIMAL))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_ANIMAL)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_UNDEAD))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_UNDEAD)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_DEVIL))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_DEVIL)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_HUMAN))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_HUMAN)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_ORC))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_ORC)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_MILGYO))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_MILGYO)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_INSECT))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_INSECT)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_FIRE))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_FIRE)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_ICE))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_ICE)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_DESERT))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_DESERT)) / 100;
		else if (pVictim->IsRaceFlag(MOB::RACE_FLAG_TREE))
			iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_TREE)) / 100;

		iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 100;
	}
	else if (pVictim->IsPC())
	{
		iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_HUMAN)) / 100;

		switch (pVictim->GetJob())
		{
			case JOB_WARRIOR:
				iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_WARRIOR)) / 100;
				break;

			case JOB_ASSASSIN:
				iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN)) / 100;
				break;

			case JOB_SURA:
				iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk += (iAtk* pAttacker->GetPoint(POINT_ATTBONUS_SHAMAN)) / 100;
				break;
		}
	}

	if (pAttacker->IsPC())
	{
		switch (pAttacker->GetJob())
		{
			case JOB_WARRIOR:
				iAtk -= (iAtk* pVictim->GetPoint(POINT_RESIST_WARRIOR)) / 100;
				break;
				
			case JOB_ASSASSIN:
				iAtk -= (iAtk* pVictim->GetPoint(POINT_RESIST_ASSASSIN)) / 100;
				break;
				
			case JOB_SURA:
				iAtk -= (iAtk* pVictim->GetPoint(POINT_RESIST_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk -= (iAtk* pVictim->GetPoint(POINT_RESIST_SHAMAN)) / 100;
				break;
		}
	}

	//[ mob -> PC ] Apply elemental attribute defense
	//2013/01/17
	// Resistance is applied only to a value equivalent to 30% of monster attribute attack damage.
	if (pAttacker->IsNPC() && pVictim->IsPC())
	{
		if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_ELEC))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_ELEC))		/ 10000;
		else if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_FIRE))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_FIRE))		/ 10000;
		else if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_ICE))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_ICE))		/ 10000;
		else if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_WIND))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_WIND))		/ 10000;
		else if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_EARTH))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_EARTH))	/ 10000;
		else if (pAttacker->IsRaceFlag(MOB::RACE_FLAG_ATT_DARK))
			iAtk -= (iAtk * 30* pVictim->GetPoint(POINT_RESIST_DARK))		/ 10000;
	}
		
	
	return iAtk;
}

void Item_GetDamage(LPITEM pItem, int32_t* pdamMin, int32_t* pdamMax)
{
	*pdamMin = 0;
	*pdamMax = 1;

	if (!pItem)
		return;

	switch (pItem->GetType())
	{
		case ITEM::TYPE_ROD:
		case ITEM::TYPE_PICK:
			return;
	}

	if (pItem->GetType() != ITEM::TYPE_WEAPON)
		SysLog("Item_GetDamage - !ITEM_WEAPON vnum={}, type={}", pItem->GetOriginalVnum(), pItem->GetType());

	*pdamMin = pItem->GetValue(3);
	*pdamMax = pItem->GetValue(4);
}

int32_t CalcMeleeDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim, bool bIgnoreDefense, bool bIgnoreTargetRating)
{
	LPITEM pWeapon = pAttacker->GetWear(WEAR_WEAPON);
	bool bPolymorphed = pAttacker->IsPolymorphed();

	if (pWeapon && !(bPolymorphed && !pAttacker->IsPolyMaintainStat()))
	{
		if (pWeapon->GetType() != ITEM::TYPE_WEAPON)
			return 0;

		switch (pWeapon->GetSubType())
		{
			case ITEM::WEAPON_SWORD:
			case ITEM::WEAPON_DAGGER:
			case ITEM::WEAPON_TWO_HANDED:
			case ITEM::WEAPON_BELL:
			case ITEM::WEAPON_FAN:
				break;

			case ITEM::WEAPON_BOW:
				SysLog("CalcMeleeDamage should not handle bows (name: {})", pAttacker->GetName());
				return 0;

			default:
				return 0;
		}
	}

	int32_t iDam = 0;
	float fAR = CalcAttackRating(pAttacker, pVictim, bIgnoreTargetRating);
	int32_t iDamMin = 0, iDamMax = 0;

	// TESTSERVER_SHOW_ATTACKINFO
	int32_t DEBUG_iDamCur = 0;
	int32_t DEBUG_iDamBonus = 0;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	if (bPolymorphed && !pAttacker->IsPolyMaintainStat())
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX

		uint32_t dwMobVnum = pAttacker->GetPolymorphVnum();
		const CMob* pMob = CMobManager::GetInstance()->Get(dwMobVnum);

		if (pMob)
		{
			int32_t iPower = pAttacker->GetPolymorphPower();
			iDamMin += pMob->m_table.dwDamageRange[0] * iPower / 100;
			iDamMax += pMob->m_table.dwDamageRange[1] * iPower / 100;
		}
	}
	else if (pWeapon)
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX
	}
	else if (pAttacker->IsNPC())
	{
		iDamMin = pAttacker->GetMobDamageMin();
		iDamMax = pAttacker->GetMobDamageMax();
	}

	iDam = number(iDamMin, iDamMax) * 2;

	// TESTSERVER_SHOW_ATTACKINFO
	DEBUG_iDamCur = iDam;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO
	//
	int32_t iAtk = 0;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pAttacker->GetLevel() * 2);
	iAtk = (int32_t) (iAtk * fAR);
	iAtk += pAttacker->GetLevel() * 2; // and add again

	if (pWeapon)
	{
		iAtk += pWeapon->GetValue(5) * 2;

		// 2004.11.12.myevan.TESTSERVER_SHOW_ATTACKINFO
		DEBUG_iDamBonus = pWeapon->GetValue(5) * 2;
		///////////////////////////////////////////////
	}

	iAtk += pAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS); // party attacker role bonus
	iAtk = (int32_t) (iAtk * (100 + (pAttacker->GetPoint(POINT_ATT_BONUS) + pAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pAttacker, pVictim, iAtk);

	int32_t iDef = 0;

	if (!bIgnoreDefense)
	{
		iDef = (pVictim->GetPoint(POINT_DEF_GRADE) * (100 + pVictim->GetPoint(POINT_DEF_BONUS)) / 100);

		if (!pAttacker->IsPC())
			iDef += pVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

	if (pAttacker->IsNPC())
		iAtk = (int32_t) (iAtk* pAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	if (test_server)
	{
		int32_t DEBUG_iLV = pAttacker->GetLevel()*2;
		int32_t DEBUG_iST = int32_t((pAttacker->GetPoint(POINT_ATT_GRADE) - DEBUG_iLV) * fAR);
		int32_t DEBUG_iPT = pAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
		int32_t DEBUG_iWP = 0;
		int32_t DEBUG_iPureAtk = 0;
		int32_t DEBUG_iPureDam = 0;
		char szRB[32] = "";
		char szGradeAtkBonus[32] = "";

		DEBUG_iWP = int32_t(DEBUG_iDamCur * fAR);
		DEBUG_iPureAtk = DEBUG_iLV + DEBUG_iST + DEBUG_iWP+DEBUG_iDamBonus;
		DEBUG_iPureDam = iAtk - iDef;

		if (pAttacker->IsNPC())
		{
			snprintf(szGradeAtkBonus, sizeof(szGradeAtkBonus), "=%d*%.1f", DEBUG_iPureAtk, pAttacker->GetMobDamageMultiply());
			DEBUG_iPureAtk = int32_t(DEBUG_iPureAtk* pAttacker->GetMobDamageMultiply());
		}

		if (DEBUG_iDamBonus != 0)
			snprintf(szRB, sizeof(szRB), "+RB(%d)", DEBUG_iDamBonus);

		char szPT[32] = "";

		if (DEBUG_iPT != 0)
			snprintf(szPT, sizeof(szPT), ", PT=%d", DEBUG_iPT);

		char szUnknownAtk[32] = "";

		if (iAtk != DEBUG_iPureAtk)
			snprintf(szUnknownAtk, sizeof(szUnknownAtk), "+?(%d)", iAtk-DEBUG_iPureAtk);

		char szUnknownDam[32] = "";

		if (iDam != DEBUG_iPureDam)
			snprintf(szUnknownDam, sizeof(szUnknownDam), "+?(%d)", iDam-DEBUG_iPureDam);

		char szMeleeAttack[128];

		snprintf(szMeleeAttack, sizeof(szMeleeAttack), 
				"%s(%d)-%s(%d)=%d%s, ATK=LV(%d)+ST(%d)+WP(%d)%s%s%s, AR=%.3g%s", 
				pAttacker->GetName(),
				iAtk,
				pVictim->GetName(),
				iDef,
				iDam,
				szUnknownDam,
				DEBUG_iLV, 
				DEBUG_iST,
				DEBUG_iWP, 
				szRB,
				szUnknownAtk,
				szGradeAtkBonus,
				fAR,
				szPT);

		pAttacker->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
		pVictim->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
	}

	return CalcBattleDamage(iDam, pAttacker->GetLevel(), pVictim->GetLevel());
}

int32_t CalcArrowDamage(LPCHARACTER pAttacker, LPCHARACTER pVictim, LPITEM pBow, LPITEM pArrow, bool bIgnoreDefense)
{
	if (!pBow || pBow->GetType() != ITEM::TYPE_WEAPON || pBow->GetSubType() != ITEM::WEAPON_BOW)
		return 0;

	if (!pArrow)
		return 0;

	int32_t iDist = (int32_t) (DISTANCE_SQRT(pAttacker->GetX() - pVictim->GetX(), pAttacker->GetY() - pVictim->GetY()));
	//int32_t iGap = (iDist / 100) - 5 - pBow->GetValue(5) - pAttacker->GetPoint(POINT_BOW_DISTANCE);
	int32_t iGap = (iDist / 100) - 5 - pAttacker->GetPoint(POINT_BOW_DISTANCE);
	int32_t iPercent = 100 - (iGap * 5);

	if (iPercent <= 0)
		return 0;
	else if (iPercent > 100)
		iPercent = 100;

	int32_t iDam = 0;

	float fAR = CalcAttackRating(pAttacker, pVictim, false);
	iDam = number(pBow->GetValue(3), pBow->GetValue(4)) * 2 + pArrow->GetValue(3);
	int32_t iAtk;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pAttacker->GetLevel() * 2);
	iAtk = (int32_t) (iAtk * fAR);
	iAtk += pAttacker->GetLevel() * 2; // and add again

	// Refine Grade
	iAtk += pBow->GetValue(5) * 2;

	iAtk += pAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
	iAtk = (int32_t) (iAtk * (100 + (pAttacker->GetPoint(POINT_ATT_BONUS) + pAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pAttacker, pVictim, iAtk);

	int32_t iDef = 0;

	if (!bIgnoreDefense)
		iDef = (pVictim->GetPoint(POINT_DEF_GRADE) * (100 + pAttacker->GetPoint(POINT_DEF_BONUS)) / 100);

	if (pAttacker->IsNPC())
		iAtk = (int32_t) (iAtk* pAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	int32_t iPureDam = iDam;

	iPureDam = (iPureDam * iPercent) / 100;

	if (test_server)
	{
		pAttacker->ChatPacket(CHAT_TYPE_INFO, "ARROW %s -> %s, DAM %d DIST %d GAP %d %% %d",
				pAttacker->GetName(), 
				pVictim->GetName(), 
				iPureDam, 
				iDist, iGap, iPercent);
	}

	return iPureDam;
	//return iDam;
}


void NormalAttackAffect(LPCHARACTER pAttacker, LPCHARACTER pVictim)
{
	if (pAttacker->GetPoint(POINT_POISON_PCT) && !pVictim->IsAffectFlag(AFF_POISON))
	{
		if (number(1, 100) <= pAttacker->GetPoint(POINT_POISON_PCT))
			pVictim->AttackedByPoison(pAttacker);
	}

	int32_t iStunDuration = 2;
	if (pAttacker->IsPC() && !pVictim->IsPC())
		iStunDuration = 4;

	AttackAffect(pAttacker, pVictim, POINT_STUN_PCT, ITEM::IMMUNE_STUN,  AFFECT_STUN, POINT_NONE,        0, AFF_STUN, iStunDuration, "STUN");
	AttackAffect(pAttacker, pVictim, POINT_SLOW_PCT, ITEM::IMMUNE_SLOW,  AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 20,		"SLOW");
}

int32_t battle_hit(LPCHARACTER pAttacker, LPCHARACTER pVictim, int32_t & iRetDam)
{
	//PROF_UNIT puHit("Hit");
	if (test_server)
		PyLog("battle_hit : [{}] attack to [{}] : dam :{} type :{}", pAttacker->GetName(), pVictim->GetName(), iRetDam);

	int32_t iDam = CalcMeleeDamage(pAttacker, pVictim);

	if (iDam <= 0)
		return (BATTLE_DAMAGE);

	NormalAttackAffect(pAttacker, pVictim);

	//iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST)) / 100;
	LPITEM pWeapon = pAttacker->GetWear(WEAR_WEAPON);

	if (pWeapon)
		switch (pWeapon->GetSubType())
		{
			case ITEM::WEAPON_SWORD:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_SWORD)) / 100;
				break;

			case ITEM::WEAPON_TWO_HANDED:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_TWOHAND)) / 100;
				break;

			case ITEM::WEAPON_DAGGER:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_DAGGER)) / 100;
				break;

			case ITEM::WEAPON_BELL:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_BELL)) / 100;
				break;

			case ITEM::WEAPON_FAN:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_FAN)) / 100;
				break;

			case ITEM::WEAPON_BOW:
				iDam = iDam * (100 - pVictim->GetPoint(POINT_RESIST_BOW)) / 100;
				break;
		}


	//Final damage correction. (As of February 2011, only applicable to giant spiders.)
	float attMul = pAttacker->GetAttMul();
	float tempIDam = iDam;
	iDam = attMul * tempIDam + 0.5f;

	iRetDam = iDam;

	//PROF_UNIT puDam("Dam");
	if (pVictim->Damage(pAttacker, iDam, DAMAGE_TYPE_NORMAL))
		return (BATTLE_DEAD);

	return (BATTLE_DAMAGE);
}

uint32_t GET_ATTACK_SPEED(LPCHARACTER ch)
{
    if (!ch)
        return 1000;

	LPITEM item = ch->GetWear(WEAR_WEAPON);
	uint32_t default_bonus = SPEEDHACK_LIMIT_BONUS;    // Yuduri attack speed (default 80)
	uint32_t riding_bonus = 0;

	if (ch->IsRiding())
	{
		// 50 bonus attack speed when riding something
		riding_bonus = 50;
	}

	uint32_t ani_speed = ani_attack_speed(ch);
    uint32_t real_speed = (ani_speed * 100) / (default_bonus + ch->GetPoint(POINT_ATT_SPEED) + riding_bonus);

	// Double attack speed for daggers
	if (item && item->GetSubType() == ITEM::WEAPON_DAGGER)
		real_speed /= 2;

    return real_speed;

}

void SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, uint32_t current_time)
{
	if (!ch || !victim)
		return;

	if (!ch->IsPC())
		return;

	ch->m_kAttackLog.dwVID = victim->GetVID();
	ch->m_kAttackLog.dwTime = current_time;
}

void SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, uint32_t current_time)
{
	if (!ch || !victim)
		return;

	if (!ch->IsPC())
		return;

	victim->m_AttackedLog.dwPID			= ch->GetPlayerID();
	victim->m_AttackedLog.dwAttackedTime= current_time;
}

bool IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, uint32_t current_time)
{
	if (ch->m_kAttackLog.dwVID == victim->GetVID())
	{
		if (current_time - ch->m_kAttackLog.dwTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			if (test_server)
			{
				PyLog("{} attack hack! time (delta, limit)=({}, {}) hack_count {}",
						ch->GetName(),
						current_time - ch->m_kAttackLog.dwTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);

				ch->ChatPacket(CHAT_TYPE_INFO, "%s attack hack! time (delta, limit)=(%u, %u) hack_count %d",
						ch->GetName(),
						current_time - ch->m_kAttackLog.dwTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);
			}

			SET_ATTACK_TIME(ch, victim, current_time);
			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACK_TIME(ch, victim, current_time);

	if (victim->m_AttackedLog.dwPID == ch->GetPlayerID())
	{
		if (current_time - victim->m_AttackedLog.dwAttackedTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			if (test_server)
			{
				PyLog("{} Attack Speed HACK! time (delta, limit)=({}, {}), hack_count = {}",
						ch->GetName(),
						current_time - victim->m_AttackedLog.dwAttackedTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);

				ch->ChatPacket(CHAT_TYPE_INFO, "Attack Speed Hack(%s), (delta, limit)=(%u, %u)",
						ch->GetName(),
						current_time - victim->m_AttackedLog.dwAttackedTime,
						GET_ATTACK_SPEED(ch));
			}

			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACKED_TIME(ch, victim, current_time);
	return false;
}


