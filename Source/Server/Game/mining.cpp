#include "stdafx.h"
#include "mining.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "item.h"
#include "config.h"
#include "db.h"
#include "log.h"
#include "skill.h"

namespace mining
{
	enum
	{
		MAX_ORE = 18,
		MAX_FRACTION_COUNT = 9,
		ORE_COUNT_FOR_REFINE = 100,
	};

	struct SInfo
	{
		uint32_t dwLoadVnum;
		uint32_t dwRawOreVnum;
		uint32_t dwRefineVnum;
	};

	SInfo info[MAX_ORE] =
	{
		{ 20047, 50601, 50621 },
		{ 20048, 50602, 50622 },
		{ 20049, 50603, 50623 },
		{ 20050, 50604, 50624 },
		{ 20051, 50605, 50625 },
		{ 20052, 50606, 50626 },
		{ 20053, 50607, 50627 },
		{ 20054, 50608, 50628 },
		{ 20055, 50609, 50629 },
		{ 20056, 50610, 50630 },
		{ 20057, 50611, 50631 },
		{ 20058, 50612, 50632 },
		{ 20059, 50613, 50633 },
		{ 30301, 50614, 50634 },
		{ 30302, 50615, 50635 },
		{ 30303, 50616, 50636 },
		{ 30304, 50617, 50637 },
		{ 30305, 50618, 50638 },
	};

	int32_t fraction_info[MAX_FRACTION_COUNT][3] =
	{
		{ 20,  1, 10 },
		{ 30, 11, 20 },
		{ 20, 21, 30 },
		{ 15, 31, 40 },
		{  5, 41, 50 },
		{  4, 51, 60 },
		{  3, 61, 70 },
		{  2, 71, 80 },
		{  1, 81, 90 },
	};

	int32_t PickGradeAddPct[10] =
	{
		3, 5, 8, 11, 15, 20, 26, 32, 40, 50
	};

	int32_t SkillLevelAddPct[SKILL_MAX_LEVEL + 1] =
	{
		0,
		1, 1, 1, 1,		//  1 - 4
		2, 2, 2, 2,		//  5 - 8
		3, 3, 3, 3,		//  9 - 12
		4, 4, 4, 4,		// 13 - 16
		5, 5, 5, 5,		// 17 - 20
		6, 6, 6, 6,		// 21 - 24
		7, 7, 7, 7,		// 25 - 28
		8, 8, 8, 8,		// 29 - 32
		9, 9, 9, 9,		// 33 - 36
		10, 10, 10, 	// 37 - 39
		11,				// 40
	};

	uint32_t GetRawOreFromLoad(uint32_t dwLoadVnum)
	{
		for (int32_t i = 0; i < MAX_ORE; ++i)
		{
			if (info[i].dwLoadVnum == dwLoadVnum)
				return info[i].dwRawOreVnum;
		}
		return 0;
	}

	uint32_t GetRefineFromRawOre(uint32_t dwRawOreVnum)
	{
		for (int32_t i = 0; i < MAX_ORE; ++i)
		{
			if (info[i].dwRawOreVnum == dwRawOreVnum)
				return info[i].dwRefineVnum;
		}
		return 0;
	}

	int32_t GetFractionCount()
	{
		int32_t r = number(1, 100);

		for (int32_t i = 0; i < MAX_FRACTION_COUNT; ++i)
		{
			if (r <= fraction_info[i][0])
				return number(fraction_info[i][1], fraction_info[i][2]);
			else
				r -= fraction_info[i][0];
		}

		return 0; 
	}

	void OreDrop(LPCHARACTER ch, uint32_t dwLoadVnum)
	{
		uint32_t dwRawOreVnum = GetRawOreFromLoad(dwLoadVnum);

		int32_t iFractionCount = GetFractionCount();

		if (iFractionCount == 0)
		{
			SysLog("Wrong ore fraction count");
			return;
		}

		LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(dwRawOreVnum, GetFractionCount());

		if (!item)
		{
			SysLog("cannot create item vnum {}", dwRawOreVnum);
			return;
		}

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
		item->SetOwnership(ch, 15);

		DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_DROP, item->GetVnum(), item->GetCount());
	}

	int32_t GetOrePct(LPCHARACTER ch)
	{
		int32_t defaultPct = 20;
		int32_t iSkillLevel = ch->GetSkillLevel(SKILL_MINING);

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		if (!pick || pick->GetType() != ITEM::TYPE_PICK)
			return 0;

		return defaultPct + SkillLevelAddPct[MINMAX(0, iSkillLevel, 40)] + PickGradeAddPct[MINMAX(0, pick->GetRefineLevel(), 9)];
	}

	EVENTINFO(mining_event_info)
	{
		uint32_t pid;
		uint32_t vid_load;

		mining_event_info() 
		: pid(0)
		, vid_load(0)
		{
		}
	};

	// REFINE_PICK
	bool Pick_Check(CItem& item)
	{
		if (item.GetType() != ITEM::TYPE_PICK)    
			return false;

		return true;
	}

	int32_t Pick_GetMaxExp(CItem& pick)
	{
		return pick.GetValue(2);
	}

	int32_t Pick_GetCurExp(CItem& pick)
	{
		return pick.GetSocket(0);
	}

	void Pick_IncCurExp(CItem& pick)
	{
		int32_t cur = Pick_GetCurExp(pick);
		pick.SetSocket(0, cur + 1);
	}

	void Pick_MaxCurExp(CItem& pick)
	{
		int32_t max = Pick_GetMaxExp(pick);
		pick.SetSocket(0, max);
	}

	bool Pick_Refinable(CItem& item)
	{
		if (Pick_GetCurExp(item) < Pick_GetMaxExp(item))
			return false;

		return true;
	}

	bool Pick_IsPracticeSuccess(CItem& pick)
	{
		return (number(1,pick.GetValue(1))==1);
	}
	
	bool Pick_IsRefineSuccess(CItem& pick)
	{
		return (number(1,100) <= pick.GetValue(3));
	}

	int32_t RealRefinePick(LPCHARACTER ch, LPITEM item)
	{
		if (!ch || !item)
			return 2;

		auto rkLogMgr = LogManager::GetInstance();
		auto rkItemMgr = ITEM_MANAGER::GetInstance();

		if (!Pick_Check(*item))
		{
			SysLog("REFINE_PICK_HACK pid({}) item({}:{}) type({})", ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetType());
			rkLogMgr->RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), -1, 1, "PICK_HACK");
			return 2;
		}

		CItem& rkOldPick = *item;

		if (!Pick_Refinable(rkOldPick))
			return 2;

		int32_t iAdv = rkOldPick.GetValue(0) / 10;

		if (rkOldPick.IsEquipped())
			return 2;

		if (Pick_IsRefineSuccess(rkOldPick))
		{
			rkLogMgr->RefineLog(ch->GetPlayerID(), rkOldPick.GetName(), rkOldPick.GetID(), iAdv, 1, "PICK");

			LPITEM pNewPick = ITEM_MANAGER::GetInstance()->CreateItem(rkOldPick.GetRefinedVnum(), 1);
			if (pNewPick)
			{
				uint8_t bCell = rkOldPick.GetCell();
				rkItemMgr->RemoveItem(item, "REMOVE (REFINE PICK)");
				pNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				LogManager::GetInstance()->ItemLog(ch, pNewPick, "REFINE PICK SUCCESS", pNewPick->GetName());
				return 1;
			}

			return 2;
		}
		else
		{
			rkLogMgr->RefineLog(ch->GetPlayerID(), rkOldPick.GetName(), rkOldPick.GetID(), iAdv, 0, "PICK");

			LPITEM pNewPick = ITEM_MANAGER::GetInstance()->CreateItem(rkOldPick.GetValue(4), 1);

			if (pNewPick)
			{
				uint8_t bCell = rkOldPick.GetCell();
				rkItemMgr->RemoveItem(item, "REMOVE (REFINE PICK)");
				pNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				rkLogMgr->ItemLog(ch, pNewPick, "REFINE PICK FAIL", pNewPick->GetName());
				return 0;
			}

			return 2;
		}
	}

	void CHEAT_MAX_PICK(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		Pick_MaxCurExp(pick);

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your mining points have reached their maximum. (%d)"), Pick_GetCurExp(pick));
	}

	void PracticePick(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		if (pick.GetRefinedVnum()<=0)
			return;

		if (Pick_IsPracticeSuccess(pick))
		{

			if (Pick_Refinable(pick))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your mining points have reached their maximum level."));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can get lumberjack Deokbae to upgrade your Pickaxe."));
			}
			else
			{
				Pick_IncCurExp(pick);	

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your Mining Points have increased! (%d/%d)"),
						Pick_GetCurExp(pick), Pick_GetMaxExp(pick));

				if (Pick_Refinable(pick))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your mining points have reached their maximum level."));
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can get lumberjack Deokbae to upgrade your Pickaxe."));
				}
			}
		}
	}
	// END_OF_REFINE_PICK

	EVENTFUNC(mining_event)
	{
		mining_event_info* info = dynamic_cast<mining_event_info*>(event->info);

		if (info == nullptr)
		{
			SysLog("mining_event_info> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(info->pid);
		LPCHARACTER load = CHARACTER_MANAGER::GetInstance()->Find(info->vid_load);

		if (!ch)
			return 0;

		ch->mining_take();

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		// REFINE_PICK
		if (!pick || !Pick_Check(*pick))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot mine without a Pick."));
			return 0;
		}
		// END_OF_REFINE_PICK

		if (!load)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Nothing to mine here."));
			return 0;
		}

		int32_t iPct = GetOrePct(ch);

		if (number(1, 100) <= iPct)
		{
			OreDrop(ch, load->GetRaceNum());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The mining has been successful."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The mining has failed."));
		}

		PracticePick(ch, pick);

		return 0;
	}

	LPEVENT CreateMiningEvent(LPCHARACTER ch, LPCHARACTER load, int32_t count)
	{
		mining_event_info* info = AllocEventInfo<mining_event_info>();
		info->pid = ch->GetPlayerID();
		info->vid_load = load->GetVID();

		return event_create(mining_event, info, PASSES_PER_SEC(2 * count));
	}

	bool OreRefine(LPCHARACTER ch, LPCHARACTER npc, LPITEM item, int32_t cost, int32_t pct, LPITEM metinstone_item)
	{
		if (!ch || !npc)
			return false;

		if (item->GetOwner() != ch)
		{
			SysLog("wrong owner");
			return false;
		}

		if (item->GetCount() < ORE_COUNT_FOR_REFINE)
		{
			SysLog("not enough count");
			return false;
		}

		uint32_t dwRefinedVnum = GetRefineFromRawOre(item->GetVnum());

		if (dwRefinedVnum == 0)
			return false;

		ch->SetRefineNPC(npc);
		item->SetCount(item->GetCount() - ORE_COUNT_FOR_REFINE);
		int32_t iCost = ch->ComputeRefineFee(cost, 1);

		if (ch->GetGold() < iCost)
			return false;

		ch->PayRefineFee(iCost);

		if (metinstone_item)
			ITEM_MANAGER::GetInstance()->RemoveItem(metinstone_item, "REMOVE (MELT)");

		if (number(1, 100) <= pct)
		{
			ch->AutoGiveItem(dwRefinedVnum, 1);
			return true;
		}

		return false;
	}

	bool IsVeinOfOre (uint32_t vnum)
	{
		for (int32_t i = 0; i < MAX_ORE; i++)
		{
			if (info[i].dwLoadVnum == vnum)
				return true;
		}
		return false;
	}
}

