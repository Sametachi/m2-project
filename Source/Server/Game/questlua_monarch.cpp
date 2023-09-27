#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"
#include "monarch.h"
#include "desc_client.h"
#include "start_position.h"
#include "config.h"
#include "mob_manager.h"
#include "castle.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "p2p.h"
#include "guild.h"
#include "sectree_manager.h"

ACMD(do_monarch_mob);

namespace quest
{
	EVENTINFO(monarch_powerup_event_info)
	{
		int32_t EmpireIndex;

		monarch_powerup_event_info() 
		: EmpireIndex(0)
		{
		}
	};

	// NOTE: Copied from SPacketGGMonarchTransfer for event data 
	EVENTINFO(monarch_transfer2_event_info)
	{
		uint8_t	bHeader;
		uint32_t	dwTargetPID;
		int32_t	x;
		int32_t	y;

		monarch_transfer2_event_info() 
		: bHeader(0)
		, dwTargetPID(0)
		, x(0)
		, y(0)
		{
		}
	};

	EVENTFUNC(monarch_powerup_event)
	{
		monarch_powerup_event_info * info =  dynamic_cast<monarch_powerup_event_info*>(event->info);

		if (info == nullptr)
		{
			SysLog("monarch_powerup_event> <Factor> Null pointer");
			return 0;
		}

		CMonarch::GetInstance()->PowerUp(info->EmpireIndex, false);
		return 0;
	}

	EVENTINFO(monarch_defenseup_event_info)
	{
		int32_t EmpireIndex;

		monarch_defenseup_event_info() 
		: EmpireIndex(0)
		{
		}
	};

	EVENTFUNC(monarch_defenseup_event)
	{
		monarch_powerup_event_info * info =  dynamic_cast<monarch_powerup_event_info*>(event->info);

		if (info == nullptr)
		{
			SysLog("monarch_defenseup_event> <Factor> Null pointer");
			return 0;
		}

		CMonarch::GetInstance()->DefenseUp(info->EmpireIndex, false);
		return 0;
	}
	
	//
	// "monarch_" lua functions
	//
	int32_t takemonarchmoney(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		
		int32_t nMoney = (int32_t)lua_tonumber(L,1);
		int32_t nPID = ch->GetPlayerID();
		int32_t nEmpire = ch->GetEmpire();
		nMoney = nMoney * 10000;
		
		PyLog("[MONARCH] Take Money Empire({}) pid({}) Money({})", ch->GetEmpire(), ch->GetPlayerID(), nMoney);


		db_clientdesc->DBPacketHeader(HEADER_GD_TAKE_MONARCH_MONEY, ch->GetDesc()->GetHandle(), sizeof(int32_t) * 3);
		db_clientdesc->Packet(&nEmpire, sizeof(int32_t)); 
		db_clientdesc->Packet(&nPID, sizeof(int32_t));
		db_clientdesc->Packet(&nMoney, sizeof(int32_t));
		return 1;
	}

	int32_t	 is_guild_master(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
	
		if (ch->GetGuild()	)
		{
			TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());

			if (pMember)
			{
				if (pMember->grade <= 4)
				{
					lua_pushnumber(L ,1);
					return 1;
				}
			}

		}
		lua_pushnumber(L ,0);

		return 1;
	}

	int32_t monarch_bless(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		
		if (false==ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}
	
		int32_t HealPrice = quest::CQuestManager::GetInstance()->GetEventFlag("MonarchHealGold");
		if (HealPrice == 0)
			HealPrice = 2000000;	// 200

		if (CMonarch::GetInstance()->HealMyEmpire(ch, HealPrice))
		{
			char szNotice[256];
			snprintf(szNotice, sizeof(szNotice),
					LC_TEXT("When the Blessing of the Emperor is used %s the HP and SP are restored again."), EMPIRE_NAME(ch->GetEmpire()));
			SendNoticeMap(szNotice, ch->GetMapIndex(), false);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Emperor Blessing is activated."));
		}

		return 1;
	}

	int32_t monarch_powerup(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 

		if (!ch)
			return 0;

		if (false==ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO ,LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}

		int32_t	money_need = 5000000;	// 500
		if (!CMonarch::GetInstance()->IsMoneyOk(money_need, ch->GetEmpire()))
		{
			int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, money_need);
			return 0;
		}

		if (!CMonarch::GetInstance()->CheckPowerUpCT(ch->GetEmpire()))
		{
			int32_t	next_sec = CMonarch::GetInstance()->GetPowerUpCT(ch->GetEmpire()) / passes_per_sec;
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After %d seconds you can use the Emperors Blessing."), next_sec);
			return 0;
		}

		CMonarch::GetInstance()->PowerUp(ch->GetEmpire(), true); 
	
		int32_t g_nMonarchPowerUpCT = 60 * 3;
		
		monarch_powerup_event_info* info = AllocEventInfo<monarch_powerup_event_info>();

		info->EmpireIndex = ch->GetEmpire();
		
		event_create(quest::monarch_powerup_event, info, PASSES_PER_SEC(g_nMonarchPowerUpCT));
		
		CMonarch::GetInstance()->SendtoDBDecMoney(5000000, ch->GetEmpire(), ch);
		
		char szNotice[256];
		snprintf(szNotice, sizeof(szNotice), LC_TEXT("Due to Emperor Sa-Za-Hu, the player %s will receive an attack power increase of 10 %% for 3 minutes in this area."), EMPIRE_NAME(ch->GetEmpire()));
		
		SendNoticeMap(szNotice, ch->GetMapIndex(), false);

		return 1;		
	}
	int32_t monarch_defenseup(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		
		if (!ch)
			return 0;
	
		
		
		if (false==ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO ,LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}
	
		int32_t	money_need = 5000000;
		if (!CMonarch::GetInstance()->IsMoneyOk(money_need, ch->GetEmpire()))
		{
			int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, money_need);
			return 0;
		}
	
		if (!CMonarch::GetInstance()->CheckDefenseUpCT(ch->GetEmpire()))
		{
			int32_t	next_sec = CMonarch::GetInstance()->GetDefenseUpCT(ch->GetEmpire()) / passes_per_sec;
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After %d seconds you can use the Emperors Blessing."), next_sec);
			return 0;
		}	
		
		CMonarch::GetInstance()->DefenseUp(ch->GetEmpire(), true); 

		int32_t g_nMonarchDefenseUpCT = 60 * 3;
		
		monarch_defenseup_event_info* info = AllocEventInfo<monarch_defenseup_event_info>();
		
		info->EmpireIndex = ch->GetEmpire();
		
		event_create(quest::monarch_defenseup_event, info, PASSES_PER_SEC(g_nMonarchDefenseUpCT));
		
		CMonarch::GetInstance()->SendtoDBDecMoney(5000000, ch->GetEmpire(), ch);
		
		char szNotice[256];
		snprintf(szNotice, sizeof(szNotice), LC_TEXT("By the Emperor Geum-Gang-Gwon the player %s gets 10 %% more armour for 3 minutes."), EMPIRE_NAME(ch->GetEmpire()));

		SendNoticeMap(szNotice, ch->GetMapIndex(), false);

		return 1;
	}

	int32_t is_monarch(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		if (!ch)
			return 0;
		lua_pushnumber(L, ch->IsMonarch());
		return 1;
	}

	int32_t spawn_mob(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		uint32_t mob_vnum = (uint32_t)lua_tonumber(L, 1);
	
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 

		if (!ch)
			return 0;

		const CMob* pMob = nullptr;
		
		if (!(pMob = CMobManager::GetInstance()->Get(mob_vnum)))
			if (pMob == nullptr)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
				return 0;
			}

		if (!ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO ,LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}

		uint32_t dwQuestIdx = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		bool ret = false;
		LPCHARACTER mob = nullptr;

		{
			int32_t x = ch->GetX();
			int32_t y = ch->GetY();
#if 0
			if (11505 == mob_vnum)
			{
				if (!CMonarch::GetInstance()->IsMoneyOk(CASTLE_FROG_PRICE, ch->GetEmpire()))
				{
					int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, CASTLE_FROG_PRICE);
					return 0;
				}

				mob = castle_spawn_frog(ch->GetEmpire());

				if (mob)
				{
					// decrease the treasury
					CMonarch::GetInstance()->SendtoDBDecMoney(CASTLE_FROG_PRICE, ch->GetEmpire(), ch);
					castle_save();
				}
			}
			else
#endif
			{
				mob = CHARACTER_MANAGER::GetInstance()->SpawnMob(mob_vnum, ch->GetMapIndex(), x, y, 0, pMob->m_table.bType == CHAR_TYPE_STONE, -1);
			}

			if (mob)
			{
				mob->SetQuestBy(dwQuestIdx);

				if (!ret)
				{
					ret = true;
					lua_pushnumber(L, (uint32_t) mob->GetVID());
				}
			}
		}

		return 1;
	}

	int32_t spawn_guard(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			SysLog("invalid argument");
			return 0;
		}

		uint32_t	group_vnum		= (uint32_t)lua_tonumber(L,1);
		int32_t		region_index	= (int32_t)lua_tonumber(L, 2);
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		if (!ch)
			return 0;

		if (false==ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO ,LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}

		if (false==castle_is_my_castle(ch->GetEmpire(), ch->GetMapIndex()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are only able to use this function while you are at the castle."));
			return 0;
		}

		//uint32_t 			dwQuestIdx 	= CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		LPCHARACTER		guard_leader = nullptr;
		{
			int32_t	money_need = castle_cost_of_hiring_guard(group_vnum);

			if (!CMonarch::GetInstance()->IsMoneyOk(money_need, ch->GetEmpire()))
			{
				int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, money_need);
				return 0;
			}
			guard_leader = castle_spawn_guard(ch->GetEmpire(), group_vnum, region_index);

			if (guard_leader)
			{
				CMonarch::GetInstance()->SendtoDBDecMoney(money_need, ch->GetEmpire(), ch);
				castle_save();
			}
		}

		return 1;
	}

	int32_t frog_to_empire_money(lua_State * L)
	{
		LPCHARACTER ch	= CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 

		if (NULL==ch)
			return false;

		if (!ch->IsMonarch())
		{
			if (!ch->IsGM())
			{
				ch->ChatPacket(CHAT_TYPE_INFO ,LC_TEXT("You do not have the emperor qualification."));
				SysLog("No Monarch pid {} ", ch->GetPlayerID());
				return 0;
			}
		}

		if (castle_frog_to_empire_money(ch))
		{
			int32_t empire_money = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TEST: The Gold Bar has been paid back to your kingdom's safe."));
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TEST: In your kingdom's safe there are: %d"), empire_money);
			castle_save();
			return 1;
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("TEST: You cannot pay the Gold Bar back into your kingdom's safe."));
			return 0;
		}
	}

	int32_t monarch_warp(lua_State * L)
	{
		if (!lua_isstring(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 
		if (!ch)
			return 0;


		
		if (!CMonarch::GetInstance()->IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function can only be used by the emperor."));
			return 0;
		}

		if (!ch->IsMCOK(CHARACTER::MI_WARP))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cooldown time for approximately %d seconds"), ch->GetMCLTime(CHARACTER::MI_WARP));	
			return 0;
		}


		const int32_t WarpPrice = 10000;

		if (!CMonarch::GetInstance()->IsMoneyOk(WarpPrice, ch->GetEmpire()))
		{
			int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, WarpPrice);
			return 0;	
		}

		int32_t x, y;

		LPCHARACTER tch = CHARACTER_MANAGER::GetInstance()->FindPC(name.c_str());

		if (!tch)
		{
			CCI* pCCI = P2P_MANAGER::GetInstance()->Find(name.c_str());

			if (pCCI)
			{
				if (pCCI->bEmpire != ch->GetEmpire())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot be warped to an unknown player."));
					return 0;
				}
				if (pCCI->bChannel != g_bChannel)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Adding player %d into the channel. (Present channel %d)"), pCCI->bChannel, g_bChannel);
					return 0;
				}
	
				if (!IsMonarchWarpZone(pCCI->lMapIndex))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));	
					return 0;
				}

				PIXEL_POSITION pos;

				if (!SECTREE_MANAGER::GetInstance()->GetCenterPositionOfMap(pCCI->lMapIndex, pos))
					ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pCCI->lMapIndex);
				else
				{
					//ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Warp to player %s."), name.c_str());
					ch->WarpSet(pos.x, pos.y);

					CMonarch::GetInstance()->SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

					ch->SetMC(CHARACTER::MI_WARP);
				}

			}
			else if (!CHARACTER_MANAGER::GetInstance()->FindPC(name.c_str()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
			}

			return 0;
		}
		else
		{
			if (tch->GetEmpire() != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot be warped to an unknown player."));
				return 0;
			}

			if (!IsMonarchWarpZone(tch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));
				return 0;
			}

			x = tch->GetX();
			y = tch->GetY();
		}

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Warp to player %s."), name.c_str());
		ch->WarpSet(x,y);
		ch->Stop();

		CMonarch::GetInstance()->SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

		ch->SetMC(CHARACTER::MI_WARP); 

		return 0;
	}

	int32_t empire_info(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 

		if (!ch)
			return false;

		TMonarchInfo* p = CMonarch::GetInstance()->GetMonarch();

		if (CMonarch::GetInstance()->IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))	
		{
			ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("My information about the emperor"));
		
			for (int32_t n = 1; n < 4; ++n)
			{
				if (n == ch->GetEmpire())
					ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("[%sMonarch] : %s Yang owned %lld"), EMPIRE_NAME(n), p->name[n], p->money[n]);
				else
					ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("[%sMonarch] : %s"), EMPIRE_NAME(n), p->name[n]);
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("Information about the emperor"));

			for (int32_t n = 1; n < 4; ++n)
				ch->ChatPacket(CHAT_TYPE_INFO,LC_TEXT("[%sMonarch] : %s"), EMPIRE_NAME(n), p->name[n]);
		}

		return 0;
	}

	int32_t monarch_transfer(lua_State * L)
	{
		if (!lua_isstring(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		std::string name = lua_tostring(L, 1);
		
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr(); 

		if (!ch)
			return 0;

		if (!CMonarch::GetInstance()->IsMonarch(ch->GetPlayerID(), ch->GetEmpire()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function can only be used by the emperor."));
			return 0;
		}

		if (!ch->IsMCOK(CHARACTER::MI_TRANSFER))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cooldown time for approximately %d seconds"), ch->GetMCLTime(CHARACTER::MI_TRANSFER));	
			return 0;
		}

		const int32_t WarpPrice = 10000;

		if (!CMonarch::GetInstance()->IsMoneyOk(WarpPrice, ch->GetEmpire()))
		{
			int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, WarpPrice);
			return 0;	
		}

		LPCHARACTER tch = CHARACTER_MANAGER::GetInstance()->FindPC(name.c_str());

		if (!tch)
		{
			CCI* pCCI = P2P_MANAGER::GetInstance()->Find(name.c_str());

			if (pCCI)
			{
				if (pCCI->bEmpire != ch->GetEmpire())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit players from another kingdom."));
					return 0;
				}

				if (pCCI->bChannel != g_bChannel)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The player %s is on channel %d at the moment. (Your channel: %d)"), name.c_str(), pCCI->bChannel, g_bChannel);
					return 0;
				}

				if (!IsMonarchWarpZone(pCCI->lMapIndex))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));	
					return 0;
				}
				if (!IsMonarchWarpZone(ch->GetMapIndex()))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("It cannot be summoned to that area."));
					return 0;
				}

				TPacketGGTransfer pgg;

				pgg.bHeader = HEADER_GG_TRANSFER;
				strlcpy(pgg.szName, name.c_str(), sizeof(pgg.szName));
				pgg.lX = ch->GetX();
				pgg.lY = ch->GetY();

				P2P_MANAGER::GetInstance()->Send(&pgg, sizeof(TPacketGGTransfer));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have recruited %s players."), name.c_str());

				CMonarch::GetInstance()->SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);

				ch->SetMC(CHARACTER::MI_TRANSFER);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no user with this name."));
			}

			return 0;
		}

		if (ch == tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit yourself."));
			return 0;
		}

		if (tch->GetEmpire() != ch->GetEmpire())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit players from another kingdom."));
			return 0;
		}

		if (!IsMonarchWarpZone(tch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));
			return 0;
		}
		if (!IsMonarchWarpZone(ch->GetMapIndex()))
		{
			ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("It cannot be summoned to that area."));
			return 0;
		}
		tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());

		CMonarch::GetInstance()->SendtoDBDecMoney(WarpPrice, ch->GetEmpire(), ch);
		ch->SetMC(CHARACTER::MI_TRANSFER);
		return 0;
	}

	int32_t monarch_notice(lua_State * L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch == nullptr)
			return 0;
		
		if (ch->IsMonarch() == false)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function can only be used by the emperor."));
			return 0;
		}

		std::string strNotice = lua_tostring(L, 1);

		if (strNotice.length() > 0)
			SendMonarchNotice(ch->GetEmpire(), strNotice.c_str());

		return 0;
	}

	int32_t monarch_mob(lua_State * L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch == nullptr)
			return 0;

		char vnum[256];
		strlcpy(vnum, lua_tostring(L, 1), sizeof(vnum));
		do_monarch_mob(ch, vnum, 0, 0);
		return 0;
	}

	EVENTFUNC(monarch_transfer2_event)
	{
		monarch_transfer2_event_info* info = dynamic_cast<monarch_transfer2_event_info*>(event->info);

		if (info == nullptr)
		{
			SysLog("monarch_transfer2_event> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER pTargetChar = CHARACTER_MANAGER::GetInstance()->FindByPID(info->dwTargetPID);

		if (pTargetChar != nullptr)
		{
			uint32_t qIndex = quest::CQuestManager::GetInstance()->GetQuestIndexByName("monarch_transfer");

			if (qIndex != 0)
			{
				pTargetChar->SetQuestFlag("monarch_transfer.x", info->x);
				pTargetChar->SetQuestFlag("monarch_transfer.y", info->y);
				quest::CQuestManager::GetInstance()->Letter(pTargetChar->GetPlayerID(), qIndex, 0);
			}
		}

		return 0;
	}

	int32_t monarch_transfer2(lua_State* L)
	{
		if (lua_isstring(L, 1) == false) return 0;
		
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch == nullptr) return false;

		if (CMonarch::GetInstance()->IsMonarch(ch->GetPlayerID(), ch->GetEmpire()) == false)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function can only be used by the emperor."));
			return 0;
		}

		if (ch->IsMCOK(CHARACTER::MI_TRANSFER) == false)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cooldown time for approximately %d seconds"), ch->GetMCLTime(CHARACTER::MI_TRANSFER));
			return 0;
		}
		
		const int32_t ciTransferCost = 10000;

		if (CMonarch::GetInstance()->IsMoneyOk(ciTransferCost, ch->GetEmpire()) == false)
		{
			int32_t NationMoney = CMonarch::GetInstance()->GetMoney(ch->GetEmpire());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is not enough money in the treasury. Current: %u Required amount: %u"), NationMoney, ciTransferCost);
			return 0;
		}

		std::string strTargetName = lua_tostring(L, 1);

		LPCHARACTER pTargetChar = CHARACTER_MANAGER::GetInstance()->FindPC(strTargetName.c_str());

		if (pTargetChar == nullptr)
		{
			CCI* pCCI = P2P_MANAGER::GetInstance()->Find(strTargetName.c_str());

			if (pCCI != nullptr)
			{
				if (pCCI->bEmpire != ch->GetEmpire())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit players from another kingdom."));
					return 0;
				}

				if (pCCI->bChannel != g_bChannel)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is connected to channel %d. (Current Channel: %d)"),
						   strTargetName.c_str(), pCCI->bChannel, g_bChannel);
					return 0;
				}

				if (!IsMonarchWarpZone(pCCI->lMapIndex))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));
					return 0;
				}
				if (!IsMonarchWarpZone(ch->GetMapIndex()))
				{
					ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("It cannot be summoned to that area."));
					return 0;
				}

				TPacketMonarchGGTransfer packet;
				packet.bHeader = HEADER_GG_MONARCH_TRANSFER;
				packet.dwTargetPID = pCCI->dwPID;
				packet.x = ch->GetX();
				packet.y = ch->GetY();

				P2P_MANAGER::GetInstance()->Send(&packet, sizeof(TPacketMonarchGGTransfer));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A summon request has been sent."));

				CMonarch::GetInstance()->SendtoDBDecMoney(ciTransferCost, ch->GetEmpire(), ch);
				ch->SetMC(CHARACTER::MI_TRANSFER);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no user with this name."));
				return 0;
			}
		}
		else
		{
			if (pTargetChar == ch)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit yourself."));
				return 0;
			}

			if (pTargetChar->GetEmpire() != ch->GetEmpire())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recruit players from another kingdom."));
				return 0;
			}

			if (DISTANCE_APPROX(pTargetChar->GetX() - ch->GetX(), pTargetChar->GetY() - ch->GetY()) <= 5000)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s is nearby."), pTargetChar->GetName());
				return 0;
			}

			if (!IsMonarchWarpZone(pTargetChar->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("You cannot move to that area."));
				return 0;
			}
			if (!IsMonarchWarpZone(ch->GetMapIndex()))
			{
				ch->ChatPacket (CHAT_TYPE_INFO, LC_TEXT("It cannot be summoned to that area."));
				return 0;
			}

			monarch_transfer2_event_info* info = AllocEventInfo<monarch_transfer2_event_info>();

			info->bHeader = HEADER_GG_MONARCH_TRANSFER;
			info->dwTargetPID = pTargetChar->GetPlayerID();
			info->x = ch->GetX();
			info->y = ch->GetY();

			event_create(monarch_transfer2_event, info, 1);

			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A summon request has been sent."));

			CMonarch::GetInstance()->SendtoDBDecMoney(ciTransferCost, ch->GetEmpire(), ch);
			ch->SetMC(CHARACTER::MI_TRANSFER);
			return 0;
		}

		return 0;
	}

	void RegisterMonarchFunctionTable()
	{
		luaL_reg Monarch_functions[] = 
		{
			{ "takemonarchmoney",		takemonarchmoney	},
			{ "isguildmaster",			is_guild_master		},
			{ "ismonarch",				is_monarch 			},
			{ "monarchbless",			monarch_bless		},
			{ "monarchpowerup",			monarch_powerup		},
			{ "monarchdefenseup",		monarch_defenseup	},
			{ "spawnmob",				spawn_mob			},
			{ "spawnguard",				spawn_guard			},
			{ "warp",					monarch_warp 		},
			{ "transfer",				monarch_transfer	},
			{ "transfer2",				monarch_transfer2	},
			{ "info",					empire_info 		},	
			{ "notice",					monarch_notice		},
			{ "monarch_mob",			monarch_mob			},

			{ NULL,						NULL				}
		};
		
		CQuestManager::GetInstance()->AddLuaFunctionTable("oh", Monarch_functions);
	}

}

