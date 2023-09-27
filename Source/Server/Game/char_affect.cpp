#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "packet.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "battle.h"
#include "guild.h"
#include "utils.h"
#include "locale_service.h"
#include "lua_incl.h"
#include "arena.h"
#include "horsename_manager.h"
#include "item.h"
#include "DragonSoul.h"

#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END))
#define IS_NO_CLEAR_ON_DEATH_AFFECT(type) ((type) == AFFECT_BLOCK_CHAT || ((type) >= 500 && (type) < 600))

void SendAffectRemovePacket(LPDESC d, uint32_t pid, uint32_t type, uint8_t point)
{
	TPacketGCAffectRemove ptoc;
	ptoc.bHeader	= HEADER_GC_AFFECT_REMOVE;
	ptoc.dwType		= type;
	ptoc.bApplyOn	= point;
	d->Packet(&ptoc, sizeof(TPacketGCAffectRemove));

	TPacketGDRemoveAffect ptod;
	ptod.dwPID		= pid;
	ptod.dwType		= type;
	ptod.bApplyOn	= point;
	db_clientdesc->DBPacket(HEADER_GD_REMOVE_AFFECT, 0, &ptod, sizeof(ptod));
}

void SendAffectAddPacket(LPDESC d, CAffect* pAff)
{
	TPacketGCAffectAdd ptoc;
	ptoc.bHeader		= HEADER_GC_AFFECT_ADD;
	ptoc.elem.dwType		= pAff->dwType;
	ptoc.elem.bApplyOn		= pAff->bApplyOn;
	ptoc.elem.lApplyValue	= pAff->lApplyValue;
	ptoc.elem.dwFlag		= pAff->dwFlag;
	ptoc.elem.lDuration		= pAff->lDuration;
	ptoc.elem.lSPCost		= pAff->lSPCost;
	d->Packet(&ptoc, sizeof(TPacketGCAffectAdd));
}
////////////////////////////////////////////////////////////////////
// Affect
CAffect * CHARACTER::FindAffect(uint32_t dwType, uint8_t bApply) const
{
	auto it = m_list_pAffect.begin();

	while (it != m_list_pAffect.end())
	{
		CAffect* pAffect = *it++;

		if (pAffect->dwType == dwType && (bApply == ITEM::APPLY_NONE || bApply == pAffect->bApplyOn))
			return pAffect;
	}

	return NULL;
}

EVENTFUNC(affect_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("affect_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == nullptr) { // <Factor>
		return 0;
	}

	if (!ch->UpdateAffect())
		return 0;
	else
		return passes_per_sec;
}

bool CHARACTER::UpdateAffect()
{
	// It's not something to handle in affect_event, but it's better to handle it in a one-second event.
	// This is all, so we do the potion treatment here.
	if (GetPoint(POINT_HP_RECOVERY) > 0)
	{
		if (GetMaxHP() <= GetHP())
		{
			PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
		}
		else
		{
			int32_t iVal = MIN(GetPoint(POINT_HP_RECOVERY), GetMaxHP() * 7 / 100);

			PointChange(POINT_HP, iVal);
			PointChange(POINT_HP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_SP_RECOVERY) > 0)
	{
		if (GetMaxSP() <= GetSP())
			PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));
		else 
		{
			int32_t iVal = MIN(GetPoint(POINT_SP_RECOVERY), GetMaxSP() * 7 / 100);

			PointChange(POINT_SP, iVal);
			PointChange(POINT_SP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_HP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_HP, GetPoint(POINT_HP_RECOVER_CONTINUE));
	}

	if (GetPoint(POINT_SP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_SP, GetPoint(POINT_SP_RECOVER_CONTINUE));
	}

	AutoRecoveryItemProcess(AFFECT_AUTO_HP_RECOVERY);
	AutoRecoveryItemProcess(AFFECT_AUTO_SP_RECOVERY);

	// restore stamina
	if (GetMaxStamina() > GetStamina())
	{
		int32_t iSec = (get_dword_time() - GetStopTime()) / 3000;
		if (iSec)
			PointChange(POINT_STAMINA, GetMaxStamina()/1);    
	}


	// ProcessAffect returns true if there is no affect.
	if (ProcessAffect())
		if (GetPoint(POINT_HP_RECOVERY) == 0 && GetPoint(POINT_SP_RECOVERY) == 0 && GetStamina() == GetMaxStamina())
		{
			m_pAffectEvent = nullptr;
			return false;
		}

	return true;
}

void CHARACTER::StartAffectEvent()
{
	if (m_pAffectEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pAffectEvent = event_create(affect_event, info, passes_per_sec);
}

void CHARACTER::ClearAffect(bool bSave)
{
	TAffectFlag afOld = m_afAffectFlag;
	uint16_t	wMovSpd = GetPoint(POINT_MOV_SPEED);
	uint16_t	wAttSpd = GetPoint(POINT_ATT_SPEED);

	auto it = m_list_pAffect.begin();

	while (it != m_list_pAffect.end())
	{
		CAffect* pAff = *it;

		if (bSave)
		{
			if (IS_NO_CLEAR_ON_DEATH_AFFECT(pAff->dwType) || IS_NO_SAVE_AFFECT(pAff->dwType))
			{
				++it;
				continue;
			}

			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pAff->dwType, pAff->bApplyOn);
			}
		}

		ComputeAffect(pAff, false);

		it = m_list_pAffect.erase(it);
		CAffect::Release(pAff);
	}

	if (afOld != m_afAffectFlag ||
			wMovSpd != GetPoint(POINT_MOV_SPEED) ||
			wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	CheckMaximumPoints();

	if (m_list_pAffect.empty())
		event_cancel(&m_pAffectEvent);
}

int32_t CHARACTER::ProcessAffect()
{
	bool	bDiff	= false;
	CAffect	*pAff	= nullptr;

	//
	// premium handling
	//
	for (int32_t i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int32_t aff_idx = i + AFFECT_PREMIUM_START;

		pAff = FindAffect(aff_idx);

		if (!pAff)
			continue;

		int32_t remain = GetPremiumRemainSeconds(i);

		if (remain < 0)
		{
			RemoveAffect(aff_idx);
			bDiff = true;
		}
		else
			pAff->lDuration = remain + 1;
	}

	////////// HAIR_AFFECT
	pAff = FindAffect(AFFECT_HAIR);
	if (pAff)
	{
		// IF HAIR_LIMIT_TIME() < CURRENT_TIME()
		if (this->GetQuestFlag("hair.limit_time") < get_global_time())
		{
			// SET HAIR NORMAL
			this->SetPart(PART_HAIR, 0);
			// REMOVE HAIR AFFECT
			RemoveAffect(AFFECT_HAIR);
		}
		else
		{
			// INCREASE AFFECT DURATION
			++(pAff->lDuration);
		}
	}
	////////// HAIR_AFFECT
	//

	CHorseNameManager::GetInstance()->Validate(this);

	TAffectFlag afOld = m_afAffectFlag;
	int32_t lMovSpd = GetPoint(POINT_MOV_SPEED);
	int32_t lAttSpd = GetPoint(POINT_ATT_SPEED);

	auto it = m_list_pAffect.begin();

	while (it != m_list_pAffect.end())
	{
		pAff = *it;

		bool bEnd = false;

		if (pAff->dwType >= GUILD_SKILL_START && pAff->dwType <= GUILD_SKILL_END)
		{
			if (!GetGuild() || !GetGuild()->UnderAnyWar())
				bEnd = true;
		}

		if (pAff->lSPCost > 0)
		{
			if (GetSP() < pAff->lSPCost)
				bEnd = true;
			else
				PointChange(POINT_SP, -pAff->lSPCost);
		}

		// AFFECT_DURATION_BUG_FIX
		// Infinite effect items also reduce time.
		// I don't think it matters because it takes a very large amount of time.
		if (--pAff->lDuration <= 0)
		{
			bEnd = true;
		}
		// END_AFFECT_DURATION_BUG_FIX

		if (bEnd)
		{
			it = m_list_pAffect.erase(it);
			ComputeAffect(pAff, false);
			bDiff = true;
			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pAff->dwType, pAff->bApplyOn);
			}

			CAffect::Release(pAff);

			continue;
		}

		++it;
	}

	if (bDiff)
	{
		if (afOld != m_afAffectFlag ||
				lMovSpd != GetPoint(POINT_MOV_SPEED) ||
				lAttSpd != GetPoint(POINT_ATT_SPEED))
		{
			UpdatePacket();
		}

		CheckMaximumPoints();
	}

	if (m_list_pAffect.empty())
		return true;

	return false;
}

void CHARACTER::SaveAffect()
{
	TPacketGDAddAffect p;

	auto it = m_list_pAffect.begin();

	while (it != m_list_pAffect.end())
	{
		CAffect* pAff = *it++;

		if (IS_NO_SAVE_AFFECT(pAff->dwType))
			continue;

		TraceLog("AFFECT_SAVE: {} {} {} {}", pAff->dwType, pAff->bApplyOn, pAff->lApplyValue, pAff->lDuration);

		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pAff->dwType;
		p.elem.bApplyOn		= pAff->bApplyOn;
		p.elem.lApplyValue	= pAff->lApplyValue;
		p.elem.dwFlag		= pAff->dwFlag;
		p.elem.lDuration	= pAff->lDuration;
		p.elem.lSPCost		= pAff->lSPCost;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}
}

EVENTINFO(load_affect_login_event_info)
{
	uint32_t pid;
	uint32_t count;
	char* data;

	load_affect_login_event_info()
	: pid(0)
	, count(0)
	, data(0)
	{
	}
};

EVENTFUNC(load_affect_login_event)
{
	load_affect_login_event_info* info = dynamic_cast<load_affect_login_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("load_affect_login_event_info> <Factor> Null pointer");
		return 0;
	}

	uint32_t dwPID = info->pid;
	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(dwPID);

	if (!ch)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	LPDESC d = ch->GetDesc();

	if (!d)
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}

	if (d->IsPhase(PHASE_HANDSHAKE) ||
			d->IsPhase(PHASE_LOGIN) ||
			d->IsPhase(PHASE_SELECT) ||
			d->IsPhase(PHASE_DEAD) ||
			d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		TraceLog("Affect Load by Event");
		ch->LoadAffect(info->count, (TPacketAffectElement*)info->data);
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
	else
	{
		SysLog("input_db.cpp:quest_login_event INVALID PHASE pid {}", ch->GetPlayerID());
		M2_DELETE_ARRAY(info->data);
		return 0;
	}
}

void CHARACTER::LoadAffect(uint32_t dwCount, TPacketAffectElement* pElements)
{
	m_bIsLoadedAffect = false;

	if (!GetDesc()->IsPhase(PHASE_GAME))
	{
		if (test_server)
			PyLog("LOAD_AFFECT: Creating Event", GetName(), dwCount);

		load_affect_login_event_info* info = AllocEventInfo<load_affect_login_event_info>();

		info->pid = GetPlayerID();
		info->count = dwCount;
		info->data = M2_NEW char[sizeof(TPacketAffectElement) * dwCount];
		memcpy(info->data, pElements, sizeof(TPacketAffectElement) * dwCount);

		event_create(load_affect_login_event, info, PASSES_PER_SEC(1));

		return;
	}

	ClearAffect(true);

	if (test_server)
		PyLog("LOAD_AFFECT: {} count {}", GetName(), dwCount);

	TAffectFlag afOld = m_afAffectFlag;

	int32_t lMovSpd = GetPoint(POINT_MOV_SPEED);
	int32_t lAttSpd = GetPoint(POINT_ATT_SPEED);

	for (uint32_t i = 0; i < dwCount; ++i, ++pElements)
	{
		// Do not load Youngjin Moo.
		if (pElements->dwType == SKILL_MUYEONG)
			continue;

		if (AFFECT_AUTO_HP_RECOVERY == pElements->dwType || AFFECT_AUTO_SP_RECOVERY == pElements->dwType)
		{
			LPITEM item = FindItemByID(pElements->dwFlag);

			if (!item)
				continue;

			item->Lock(true);
		}

		if (pElements->bApplyOn >= POINT_MAX_NUM)
		{
			SysLog("invalid affect data {} ApplyOn {} ApplyValue {}",
					GetName(), pElements->bApplyOn, pElements->lApplyValue);
			continue;
		}

		if (test_server)
		{
			PyLog("Load Affect : Affect {} {} {}", GetName(), pElements->dwType, pElements->bApplyOn);
		}

		CAffect* pAff = CAffect::Acquire();
		m_list_pAffect.push_back(pAff);

		pAff->dwType		= pElements->dwType;
		pAff->bApplyOn		= pElements->bApplyOn;
		pAff->lApplyValue	= pElements->lApplyValue;
		pAff->dwFlag		= pElements->dwFlag;
		pAff->lDuration	= pElements->lDuration;
		pAff->lSPCost		= pElements->lSPCost;

		SendAffectAddPacket(GetDesc(), pAff);

		ComputeAffect(pAff, true);
	
	
	}

	if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
	{
		RemoveGoodAffect();
	}
	
	if (afOld != m_afAffectFlag || lMovSpd != GetPoint(POINT_MOV_SPEED) || lAttSpd != GetPoint(POINT_ATT_SPEED))
	{
		UpdatePacket();
	}

	StartAffectEvent();

	m_bIsLoadedAffect = true;

	// Load and initialize the Dragon Soul Stone settings
	DragonSoul_Initialize();
}

bool CHARACTER::AddAffect(uint32_t dwType, uint8_t bApplyOn, int32_t lApplyValue, uint32_t dwFlag, int32_t lDuration, int32_t lSPCost, bool bOverride, bool IsCube)
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT && lDuration > 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat has been blocked by a GM."));
	}
	// END_OF_CHAT_BLOCK

	if (lDuration == 0)
	{
		SysLog("Character::AddAffect lDuration == 0 type {}", lDuration, dwType);
		lDuration = 1;
	}

	CAffect* pAff = nullptr;

	if (IsCube)
		pAff = FindAffect(dwType,bApplyOn);
	else
		pAff = FindAffect(dwType);

	if (dwFlag == AFF_STUN)
	{
		if (m_posDest.x != GetX() || m_posDest.y != GetY())
		{
			m_posDest.x = m_posStart.x = GetX();
			m_posDest.y = m_posStart.y = GetY();
			battle_end(this);

			SyncPacket();
		}
	}

	// Overwrite existing effects
	if (pAff && bOverride)
	{
		ComputeAffect(pAff, false); // delete the effect first

		if (GetDesc())
			SendAffectRemovePacket(GetDesc(), GetPlayerID(), pAff->dwType, pAff->bApplyOn);
	}
	else
	{
		//
		// add new effect
		//
		// NOTE: Therefore, multiple effects can be attached to the same type.
		//
		pAff = CAffect::Acquire();
		m_list_pAffect.push_back(pAff);

	}

	TraceLog("AddAffect {} type {} apply {} {} flag {} duration {}", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);
	PyLog("AddAffect {} type {} apply {} {} flag {} duration {}", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);

	pAff->dwType	= dwType;
	pAff->bApplyOn	= bApplyOn;
	pAff->lApplyValue	= lApplyValue;
	pAff->dwFlag	= dwFlag;
	pAff->lDuration	= lDuration;
	pAff->lSPCost	= lSPCost;

	uint16_t wMovSpd = GetPoint(POINT_MOV_SPEED);
	uint16_t wAttSpd = GetPoint(POINT_ATT_SPEED);

	ComputeAffect(pAff, true);

	if (pAff->dwFlag || wMovSpd != GetPoint(POINT_MOV_SPEED) || wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	StartAffectEvent();

	if (IsPC())
	{
		SendAffectAddPacket(GetDesc(), pAff);

		if (IS_NO_SAVE_AFFECT(pAff->dwType))
			return true;

		TPacketGDAddAffect p;
		p.dwPID			= GetPlayerID();
		p.elem.dwType		= pAff->dwType;
		p.elem.bApplyOn		= pAff->bApplyOn;
		p.elem.lApplyValue	= pAff->lApplyValue;
		p.elem.dwFlag		= pAff->dwFlag;
		p.elem.lDuration	= pAff->lDuration;
		p.elem.lSPCost		= pAff->lSPCost;
		db_clientdesc->DBPacket(HEADER_GD_ADD_AFFECT, 0, &p, sizeof(p));
	}

	return true;
}

void CHARACTER::RefreshAffect()
{
	auto it = m_list_pAffect.begin();

	while (it != m_list_pAffect.end())
	{
		CAffect* pAff = *it++;
		ComputeAffect(pAff, true);
	}
}

void CHARACTER::ComputeAffect(CAffect* pAff, bool bAdd)
{
	if (bAdd && pAff->dwType >= GUILD_SKILL_START && pAff->dwType <= GUILD_SKILL_END)	
	{
		if (!GetGuild())
			return;

		if (!GetGuild()->UnderAnyWar())
			return;
	}

	if (pAff->dwFlag)
	{
		if (!bAdd)
			m_afAffectFlag.Reset(pAff->dwFlag);
		else
			m_afAffectFlag.Set(pAff->dwFlag);
	}

	if (bAdd)
		PointChange(pAff->bApplyOn, pAff->lApplyValue);
	else
		PointChange(pAff->bApplyOn, -pAff->lApplyValue);

	if (pAff->dwType == SKILL_MUYEONG)
	{
		if (bAdd)
			StartMuyeongEvent();
		else
			StopMuyeongEvent();
	}
}

bool CHARACTER::RemoveAffect(CAffect* pAff)
{
	if (!pAff)
		return false;

	// AFFECT_BUF_FIX
	m_list_pAffect.remove(pAff);
	// END_OF_AFFECT_BUF_FIX

	ComputeAffect(pAff, false);

	// Fix white flag bug.
	// The white flag bug occurs when attacking immediately after casting buff skill -> transforming -> using white flag (AFFECT_REVIVE_INVISIBLE).
	// The reason is that at the time of casting the disguise, only the disguise effect is applied, ignoring the buff skill effect,
	// If you attack immediately after using the white flag, RemoveAffect is called, and it becomes a disguise effect + buff skill effect while doing ComputePoints.
	// If you are disguised in ComputePoints, you can make the buff skill effect not work,
	// ComputePoints are widely used, so I'm reluctant to make big changes (it's hard to know what side effects will happen).
	// Therefore, it is modified only when AFFECT_REVIVE_INVISIBLE is deleted with RemoveAffect.
	// If the time runs out and the white flag is released, no bug occurs, so do the same.
	// (If you look at ProcessAffect, if the time runs out and the Affect is deleted, ComputePoints is not called.)
	if (AFFECT_REVIVE_INVISIBLE != pAff->dwType)
	{
		ComputePoints();
	}
	CheckMaximumPoints();

	if (test_server)
		PyLog("AFFECT_REMOVE: {} (flag {} apply: {})", GetName(), pAff->dwFlag, pAff->bApplyOn);

	if (IsPC())
	{
		SendAffectRemovePacket(GetDesc(), GetPlayerID(), pAff->dwType, pAff->bApplyOn);
	}

	CAffect::Release(pAff);
	return true;
}

bool CHARACTER::RemoveAffect(uint32_t dwType)
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your chat block has been lifted."));
	}
	// END_OF_CHAT_BLOCK

	bool flag = false;

	CAffect* pAff;

	while ((pAff = FindAffect(dwType)))
	{
		RemoveAffect(pAff);
		flag = true;
	}

	return flag;
}

bool CHARACTER::IsAffectFlag(uint32_t dwAff) const
{
	return m_afAffectFlag.IsSet(dwAff);
}

void CHARACTER::RemoveGoodAffect()
{
	RemoveAffect(AFFECT_MOV_SPEED);
	RemoveAffect(AFFECT_ATT_SPEED);
	RemoveAffect(AFFECT_STR);
	RemoveAffect(AFFECT_DEX);
	RemoveAffect(AFFECT_INT);
	RemoveAffect(AFFECT_CON);
	RemoveAffect(AFFECT_CHINA_FIREWORK);

	RemoveAffect(SKILL_JEONGWI);
	RemoveAffect(SKILL_GEOMKYUNG);
	RemoveAffect(SKILL_CHUNKEON);
	RemoveAffect(SKILL_EUNHYUNG);
	RemoveAffect(SKILL_GYEONGGONG);
	RemoveAffect(SKILL_GWIGEOM);
	RemoveAffect(SKILL_TERROR);
	RemoveAffect(SKILL_JUMAGAP);
	RemoveAffect(SKILL_MANASHILED);
	RemoveAffect(SKILL_HOSIN);
	RemoveAffect(SKILL_REFLECT);
	RemoveAffect(SKILL_KWAESOK);
	RemoveAffect(SKILL_JEUNGRYEOK);
	RemoveAffect(SKILL_GICHEON);
}

bool CHARACTER::IsGoodAffect(uint8_t bAffectType) const
{
	switch (bAffectType)
	{
		case (AFFECT_MOV_SPEED):
		case (AFFECT_ATT_SPEED):
		case (AFFECT_STR):
		case (AFFECT_DEX):
		case (AFFECT_INT):
		case (AFFECT_CON):
		case (AFFECT_CHINA_FIREWORK):

		case (SKILL_JEONGWI):
		case (SKILL_GEOMKYUNG):
		case (SKILL_CHUNKEON):
		case (SKILL_EUNHYUNG):
		case (SKILL_GYEONGGONG):
		case (SKILL_GWIGEOM):
		case (SKILL_TERROR):
		case (SKILL_JUMAGAP):
		case (SKILL_MANASHILED):
		case (SKILL_HOSIN):
		case (SKILL_REFLECT):
		case (SKILL_KWAESOK):
		case (SKILL_JEUNGRYEOK):
		case (SKILL_GICHEON):
			return true;
	}
	return false;
}

void CHARACTER::RemoveBadAffect()
{
	PyLog("RemoveBadAffect {}", GetName());

	RemovePoison();
	RemoveFire();

	RemoveAffect(AFFECT_STUN);

	RemoveAffect(AFFECT_SLOW);

	RemoveAffect(SKILL_TUSOK);

	//RemoveAffect(SKILL_CURSE);

	//RemoveAffect(SKILL_PABUP);

	//RemoveAffect(AFFECT_FAINT);

	//RemoveAffect(AFFECT_WEB);

	//RemoveAffect(AFFECT_SLEEP);

	//RemoveAffect(AFFECT_CURSE);

	//RemoveAffect(AFFECT_PARALYZE);

	//RemoveAffect(SKILL_BUDONG);
}

