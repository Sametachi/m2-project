
#include "stdafx.h"

#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "item.h"
#include "item_manager.h"
#include "guild_manager.h"
#include "war_map.h"
#include "start_position.h"
#include "marriage.h"
#include "mining.h"
#include "p2p.h"
#include "polymorph.h"
#include "desc_client.h"
#include "messenger_manager.h"
#include "log.h"
#include "utils.h"
#include "unique_item.h"
#include "mob_manager.h"

#include <cctype>

extern int32_t g_nPortalLimitTime;
extern LPCLIENT_DESC db_clientdesc;
const int32_t ITEM_BROKEN_METIN_VNUM = 28960;

namespace quest 
{
	//
	// "pc" Lua functions
	//
	int32_t pc_has_master_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		bool bHasMasterSkill = false;
		for (int32_t i=0; i< SKILL_MAX_NUM; i++)
			if (ch->GetSkillMasterType(i) >= SKILL_MASTER && ch->GetSkillLevel(i) >= 21)
			{
				bHasMasterSkill = true;
				break;
			}

		lua_pushboolean(L, bHasMasterSkill);
		return 1;
	}

	int32_t pc_remove_skill_book_no_delay(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
		return 0;
	}

	int32_t pc_is_skill_book_no_delay(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_pushboolean(L, ch->FindAffect(AFFECT_SKILL_NO_BOOK_DELAY) ? true : false);
		return 1;
	}

	int32_t pc_learn_grand_master_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			SysLog("wrong skill index");
			return 0;
		}

		lua_pushboolean(L, ch->LearnGrandMasterSkill((int32_t)lua_tonumber(L, 1)));
		return 1;
	}

	int32_t pc_set_warp_location(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			SysLog("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("wrong coodinate");
			return 0;
		}

		ch->SetWarpLocation((int32_t)lua_tonumber(L,1), (int32_t)lua_tonumber(L,2), (int32_t)lua_tonumber(L,3));
		return 0;
	}

	int32_t pc_set_warp_location_local(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			SysLog("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("wrong coodinate");
			return 0;
		}

		int32_t lMapIndex = (int32_t) lua_tonumber(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::GetInstance()->GetMapRegion(lMapIndex);

		if (!region)
		{
			SysLog("invalid map index {}", lMapIndex);
			return 0;
		}

		int32_t x = (int32_t) lua_tonumber(L, 2);
		int32_t y = (int32_t) lua_tonumber(L, 3);

		if (x > region->ex - region->sx)
		{
			SysLog("x coordinate overflow max: {} input: {}", region->ex - region->sx, x);
			return 0;
		}

		if (y > region->ey - region->sy)
		{
			SysLog("y coordinate overflow max: {} input: {}", region->ey - region->sy, y);
			return 0;
		}

		ch->SetWarpLocation(lMapIndex, x, y);
		return 0;
	}

	int32_t pc_get_start_location(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_pushnumber(L, g_start_map[ch->GetEmpire()]);
		lua_pushnumber(L, g_start_position[ch->GetEmpire()][0] / 100);
		lua_pushnumber(L, g_start_position[ch->GetEmpire()][1] / 100);
		return 3;
	}

	int32_t pc_warp(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, false);
			return 1;
		}

		int32_t map_index = 0;

		if (lua_isnumber(L, 3))
			map_index = (int32_t) lua_tonumber(L,3);

		//PREVENT_HACK
		if (ch->IsHack())
		{
			lua_pushboolean(L, false);
			return 1;
		}
		//END_PREVENT_HACK
	
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "pc_warp %d %d %d",(int32_t)lua_tonumber(L,1),
					(int32_t)lua_tonumber(L,2),map_index);
		ch->WarpSet((int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2), map_index);
		
		lua_pushboolean(L, true);

		return 1;
	}

	int32_t pc_warp_local(lua_State * L)
	{
		if (!lua_isnumber(L, 1)) 
		{
			SysLog("no map index argument");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("no coodinate argument");
			return 0;
		}

		int32_t lMapIndex = (int32_t) lua_tonumber(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::GetInstance()->GetMapRegion(lMapIndex);

		if (!region)
		{
			SysLog("invalid map index {}", lMapIndex);
			return 0;
		}

		int32_t x = (int32_t) lua_tonumber(L, 2);
		int32_t y = (int32_t) lua_tonumber(L, 3);

		if (x > region->ex - region->sx)
		{
			SysLog("x coordinate overflow max: {} input: {}", region->ex - region->sx, x);
			return 0;
		}

		if (y > region->ey - region->sy)
		{
			SysLog("y coordinate overflow max: {} input: {}", region->ey - region->sy, y);
			return 0;
		}

		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->WarpSet(region->sx + x, region->sy + y);
		return 0;
	}

	int32_t pc_warp_exit(lua_State * L)
	{
		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ExitToSavedLocation();
		return 0;
	}

	int32_t pc_in_dungeon(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetDungeon()?1:0);
		return 1;
	}

	int32_t pc_hasguild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetGuild() ? 1 : 0);
		return 1;
	}

	int32_t pc_getguild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetGuild() ? ch->GetGuild()->GetID() : 0);
		return 1;
	}

	int32_t pc_isguildmaster(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		if (g)
			lua_pushboolean(L, (ch->GetPlayerID() == g->GetMasterPID()));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int32_t pc_destroy_guild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		if (g)
			g->RequestDisband(ch->GetPlayerID());

		return 0;
	}

	int32_t pc_remove_from_guild(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		if (g)
			g->RequestRemoveMember(ch->GetPlayerID());

		return 0;
	}

	int32_t pc_give_gold(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			SysLog("QUEST : wrong argument");
			return 0;
		}

		int32_t iAmount = (int32_t) lua_tonumber(L, 1);

		if (iAmount <= 0)
		{
			SysLog("QUEST : gold amount less then zero");
			return 0;
		}

		DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), iAmount);
		ch->PointChange(POINT_GOLD, iAmount, true);
		return 0;
	}

	int32_t pc_warp_to_guild_war_observer_position(lua_State* L)
	{
		luaL_checknumber(L, 1);
		luaL_checknumber(L, 2);

		uint32_t gid1 = (uint32_t)lua_tonumber(L, 1);
		uint32_t gid2 = (uint32_t)lua_tonumber(L, 2);

		CGuild* g1 = CGuildManager::GetInstance()->FindGuild(gid1);
		CGuild* g2 = CGuildManager::GetInstance()->FindGuild(gid2);

		if (!g1 || !g2)
		{
			luaL_error(L, "no such guild with id %d %d", gid1, gid2);
		}

		PIXEL_POSITION pos;

		uint32_t dwMapIndex = g1->GetGuildWarMapIndex(gid2);

		if (!CWarMapManager::GetInstance()->GetStartPosition(dwMapIndex, 2, pos))
		{
			luaL_error(L, "not under warp guild war between guild %d %d", gid1, gid2);
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		//PREVENT_HACK
		if (ch->IsHack())
			return 0;
		//END_PREVENT_HACK

		ch->SetQuestFlag("war.is_war_member", 0);
		ch->SaveExitLocation();
		ch->WarpSet(pos.x, pos.y, dwMapIndex);
		return 0;
	}

	int32_t pc_give_item_from_special_item_group(lua_State* L)
	{
		luaL_checknumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		uint32_t dwGroupVnum = (uint32_t) lua_tonumber(L,1);

		std::vector <uint32_t> dwVnums;
		std::vector <uint32_t> dwCounts;
		std::vector <LPITEM> item_gets(0);
		int32_t count = 0;

		ch->GiveItemFromSpecialItemGroup(dwGroupVnum, dwVnums, dwCounts, item_gets, count);
		
		for (int32_t i = 0; i < count; i++)
		{
			if (!item_gets[i])
			{
				if (dwVnums[i] == 1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), dwCounts[i]);
				}
				else if (dwVnums[i] == 2)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A mysterious light appears from the tree."));
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d experience points."), dwCounts[i]);
				}
			}
		}
		return 0;
	}

	int32_t pc_enough_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		uint32_t item_vnum = (uint32_t)lua_tonumber(L, 1);
		TItemTable* pTable = ITEM_MANAGER::GetInstance()->GetTable(item_vnum);
		if (!pTable)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		bool bEnoughInventoryForItem = ch->GetEmptyInventory(pTable->bSize) != -1;
		lua_pushboolean(L, bEnoughInventoryForItem);
		return 1;
	}

	int32_t pc_give_item(lua_State* L)
	{
		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) || !(lua_isstring(L, 2)||lua_isnumber(L, 2)))
		{
			SysLog("QUEST : wrong argument");
			return 0;
		}

		uint32_t dwVnum;

		if (lua_isnumber(L,2))
			dwVnum = (int32_t) lua_tonumber(L, 2);
		else if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L, 2), dwVnum))
		{
			SysLog("QUEST Make item call error : wrong item name : {}", lua_tostring(L,1));
			return 0;
		}

		int32_t icount = 1;

		if (lua_isnumber(L, 3) && lua_tonumber(L, 3) > 0)
		{
			icount = (int32_t)rint(lua_tonumber(L, 3));

			if (icount <= 0) 
			{
				SysLog("QUEST Make item call error : wrong item count : %g", lua_tonumber(L, 2));
				return 0;
			}
		}

		pPC->GiveItem(lua_tostring(L, 1), dwVnum, icount);

		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);
		return 0;
	}

	int32_t pc_give_or_drop_item(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) && !lua_isnumber(L, 1))
		{
			SysLog("QUEST Make item call error : wrong argument");
			lua_pushnumber (L, 0);
			return 1;
		}

		uint32_t dwVnum;

		if (lua_isnumber(L, 1))
		{
			dwVnum = (int32_t) lua_tonumber(L, 1);
		}
		else if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L, 1), dwVnum))
		{
			SysLog("QUEST Make item call error : wrong item name : {}", lua_tostring(L,1));
			lua_pushnumber (L, 0);

			return 1;
		}

		int32_t icount = 1;
		if (lua_isnumber(L,2) && lua_tonumber(L,2)>0)
		{
			icount = (int32_t)rint(lua_tonumber(L,2));
			if (icount<=0) 
			{
				SysLog("QUEST Make item call error : wrong item count : %g", lua_tonumber(L,2));
				lua_pushnumber (L, 0);
				return 1;
			}
		}

		PyLog("QUEST [REWARD] item {} to {}", lua_tostring(L, 1), ch->GetName());

		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();

		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);

		LPITEM item = ch->AutoGiveItem(dwVnum, icount);

		if (dwVnum >= 80003 && dwVnum <= 80007)
		{
			LogManager::GetInstance()->GoldBarLog(ch->GetPlayerID(), item->GetID(), QUEST, "quest: give_item2");
		}
		
		if (NULL != item)
			lua_pushnumber (L, item->GetID());
		else
			lua_pushnumber (L, 0);
		return 1;
	}

	int32_t pc_give_or_drop_item_and_select(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) && !lua_isnumber(L, 1))
		{
			SysLog("QUEST Make item call error : wrong argument");
			return 0;
		}

		uint32_t dwVnum;

		if (lua_isnumber(L, 1))
		{
			dwVnum = (int32_t) lua_tonumber(L, 1);
		}
		else if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L, 1), dwVnum))
		{
			SysLog("QUEST Make item call error : wrong item name : {}", lua_tostring(L,1));
			return 0;
		}

		int32_t icount = 1;
		if (lua_isnumber(L,2) && lua_tonumber(L,2)>0)
		{
			icount = (int32_t)rint(lua_tonumber(L,2));
			if (icount<=0) 
			{
				SysLog("QUEST Make item call error : wrong item count : %g", lua_tonumber(L,2));
				return 0;
			}
		}

		PyLog("QUEST [REWARD] item {} to {}", lua_tostring(L, 1), ch->GetName());

		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();

		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);

		LPITEM item = ch->AutoGiveItem(dwVnum, icount);

		if (NULL != item)
			CQuestManager::GetInstance()->SetCurrentItem(item);

		if (dwVnum >= 80003 && dwVnum <= 80007)
		{
			LogManager::GetInstance()->GoldBarLog(ch->GetPlayerID(), item->GetID(), QUEST, "quest: give_item2");
		}
		
		return 0;
	}

	int32_t pc_get_current_map_index(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetMapIndex());
		return 1;
	}

	int32_t pc_get_x(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetX()/100);
		return 1;
	}

	int32_t pc_get_y(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetY()/100);
		return 1;
	}

	int32_t pc_get_local_x(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(ch->GetMapIndex());

		if (pMap)
			lua_pushnumber(L, (ch->GetX() - pMap->m_setting.iBaseX) / 100);
		else
			lua_pushnumber(L, ch->GetX() / 100);

		return 1;
	}

	int32_t pc_get_local_y(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(ch->GetMapIndex());

		if (pMap)
			lua_pushnumber(L, (ch->GetY() - pMap->m_setting.iBaseY) / 100);
		else
			lua_pushnumber(L, ch->GetY() / 100);

		return 1;
	}

	int32_t pc_count_item(lua_State* L)
	{
		if (lua_isnumber(L, -1))
			lua_pushnumber(L,CQuestManager::GetInstance()->GetCurrentCharacterPtr()->CountSpecifyItem((uint32_t)lua_tonumber(L, -1)));
		else if (lua_isstring(L, -1))
		{
			uint32_t item_vnum;

			if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L,1), item_vnum))
			{
				SysLog("QUEST count_item call error : wrong item name : {}", lua_tostring(L,1));
				lua_pushnumber(L, 0);
			}
			else
			{
				lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->CountSpecifyItem(item_vnum));
			}
		}
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t pc_remove_item(lua_State* L)
	{
		if (lua_gettop(L) == 1)
		{
			uint32_t item_vnum;

			if (lua_isnumber(L,1))
			{
				item_vnum = (uint32_t)lua_tonumber(L, 1);
			}
			else if (lua_isstring(L,1))
			{
				if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L,1), item_vnum))
				{
					SysLog("QUEST remove_item call error : wrong item name : {}", lua_tostring(L,1));
					return 0;
				}
			}
			else
			{
				SysLog("QUEST remove_item wrong argument");
				return 0;
			}

			PyLog("QUEST remove a item vnum {} of {}[{}]", item_vnum, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetName(), CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetPlayerID());
			CQuestManager::GetInstance()->GetCurrentCharacterPtr()->RemoveSpecifyItem(item_vnum);
		}
		else if (lua_gettop(L) == 2)
		{
			uint32_t item_vnum;

			if (lua_isnumber(L, 1))
			{
				item_vnum = (uint32_t)lua_tonumber(L, 1);
			}
			else if (lua_isstring(L, 1))
			{
				if (!ITEM_MANAGER::GetInstance()->GetVnum(lua_tostring(L,1), item_vnum))
				{
					SysLog("QUEST remove_item call error : wrong item name : {}", lua_tostring(L,1));
					return 0;
				}
			}
			else
			{
				SysLog("QUEST remove_item wrong argument");
				return 0;
			}

			uint32_t item_count = (uint32_t) lua_tonumber(L, 2);
			PyLog("QUEST remove items(vnum {}) count {} of {}[{}]",
					item_vnum,
					item_count,
					CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetName(),
					CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetPlayerID());

			CQuestManager::GetInstance()->GetCurrentCharacterPtr()->RemoveSpecifyItem(item_vnum, item_count);
		}
		return 0;
	}

	int32_t pc_get_leadership(lua_State * L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetLeadershipSkillLevel());
		return 1;
	}

	int32_t pc_reset_point(lua_State * L)
	{
		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ResetPoint(CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetLevel());
		return 0;
	}

	int32_t pc_get_playtime(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetRealPoint(POINT_PLAYTIME));
		return 1;
	}

	int32_t pc_get_vid(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetVID());
		return 1;
	}
	int32_t pc_get_name(lua_State* L)
	{
		lua_pushstring(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetName());
		return 1;
	}

	int32_t pc_get_next_exp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetNextExp());
		return 1;
	}

	int32_t pc_get_exp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetExp());
		return 1;
	}

	int32_t pc_get_race(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetRaceNum());
		return 1;
	}

	int32_t pc_change_sex(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ChangeSex());
		return 1;
	}

	int32_t pc_get_job(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetJob());
		return 1;
	}

	int32_t pc_get_max_sp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetMaxSP());
		return 1;
	}

	int32_t pc_get_sp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetSP());
		return 1;
	}

	int32_t pc_change_sp(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t val = (int32_t) lua_tonumber(L, 1);

		if (val == 0)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (val > 0)
			ch->PointChange(POINT_SP, val);
		else if (val < 0)
		{
			if (ch->GetSP() < -val)
			{
				lua_pushboolean(L, 0);
				return 1;
			}

			ch->PointChange(POINT_SP, val);
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int32_t pc_get_max_hp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetMaxHP());
		return 1;
	}

	int32_t pc_get_hp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetHP());
		return 1;
	}

	int32_t pc_get_level(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetLevel());
		return 1;
	}

	int32_t pc_set_level(lua_State * L)  
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}
		else
		{
			int32_t newLevel = lua_tonumber(L, 1);
			LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();


			PyLog("QUEST [LEVEL] {} jumpint to level {}", ch->GetName(), (int32_t)rint(lua_tonumber(L,1)));

			PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
			LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), newLevel, 0);
			
			
			ch->PointChange(POINT_SKILL, newLevel - ch->GetLevel());
			ch->PointChange(POINT_SUB_SKILL, newLevel < 10 ? 0 : newLevel - MAX(ch->GetLevel(), 9));
			ch->PointChange(POINT_STAT, ((MINMAX(1, newLevel, 90) - ch->GetLevel()) * 3) + ch->GetPoint(POINT_LEVEL_STEP));
			ch->PointChange(POINT_LEVEL, newLevel - ch->GetLevel());
			ch->SetRandomHP((newLevel - 1) * number(JobInitialPoints[ch->GetJob()].hp_per_lv_begin, JobInitialPoints[ch->GetJob()].hp_per_lv_end));
			ch->SetRandomSP((newLevel - 1) * number(JobInitialPoints[ch->GetJob()].sp_per_lv_begin, JobInitialPoints[ch->GetJob()].sp_per_lv_end));


			ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
			
			ch->ComputePoints();
			ch->PointsPacket();
			ch->SkillLevelPacket();

			return 0;
		}
	}

	int32_t pc_get_weapon(lua_State * L)
	{
		LPITEM item = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetWear(WEAR_WEAPON);

		if (!item)
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int32_t pc_get_armor(lua_State * L)
	{
		LPITEM item = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetWear(WEAR_BODY);

		if (!item)
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int32_t pc_get_wear(lua_State * L)
	{		
		if (!lua_isnumber(L, 1))
		{
			SysLog("QUEST wrong set flag");
			return 0;
		}

		uint8_t bCell = (uint8_t)lua_tonumber(L, 1);

		LPITEM item = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetWear(bCell);


		if (!item)
			lua_pushnil(L);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int32_t pc_get_money(lua_State * L)
	{ 
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetGold());
		return 1;
	}

	int32_t pc_get_real_alignment(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetRealAlignment()/10);
		return 1;
	}

	int32_t pc_get_alignment(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetAlignment()/10);
		return 1;
	}

	int32_t pc_change_alignment(lua_State * L)
	{
		int32_t alignment = (int32_t)(lua_tonumber(L, 1)*10);
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->UpdateAlignment(alignment);
		return 0;
	}

	int32_t pc_change_money(lua_State * L)
	{
		int32_t gold = (int32_t)lua_tonumber(L, -1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (gold + ch->GetGold() < 0)
		{
			SysLog("QUEST wrong ChangeGold {} (now {})", gold, ch->GetGold());
		}
		else
		{
			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), gold);
			ch->PointChange(POINT_GOLD, gold, true);
		}

		return 0;
	}

	int32_t pc_set_another_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("QUEST wrong set flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L, 1);
			const char* sz2 = lua_tostring(L, 2);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			pPC->SetFlag(std::string(sz)+"."+sz2, int32_t(rint(lua_tonumber(L,3))));
			return 0;
		}
	}

	int32_t pc_get_another_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
		{
			SysLog("QUEST wrong get flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L,1);
			const char* sz2 = lua_tostring(L,2);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			if (!pPC)
			{
				return 0;
			}
			lua_pushnumber(L,pPC->GetFlag(std::string(sz)+"."+sz2));
			return 1;
		}
	}

	int32_t pc_get_flag(lua_State* L)
	{
		if (!lua_isstring(L,-1))
		{
			SysLog("QUEST wrong get flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L,-1);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			lua_pushnumber(L,pPC->GetFlag(sz));
			return 1;
		}
	}

	int32_t pc_get_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,-1))
		{
			SysLog("QUEST wrong get flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L,-1);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			lua_pushnumber(L,pPC->GetFlag(pPC->GetCurrentQuestName() + "."+sz));
			if (test_server)
				PyLog("GetQF ({} . {})", pPC->GetCurrentQuestName().c_str(), sz);
		}
		return 1;
	}

	int32_t pc_set_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			SysLog("QUEST wrong set flag");
		}
		else
		{
			const char* sz = lua_tostring(L,1);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			pPC->SetFlag(sz, int32_t(rint(lua_tonumber(L,2))));
		}
		return 0;
	}

	int32_t pc_set_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			SysLog("QUEST wrong set flag");
		}
		else
		{
			const char* sz = lua_tostring(L,1);
			auto q = CQuestManager::GetInstance();
			PC* pPC = q->GetCurrentPC();
			pPC->SetFlag(pPC->GetCurrentQuestName()+"."+sz, int32_t(rint(lua_tonumber(L,2))));
		}
		return 0;
	}

	int32_t pc_del_quest_flag(lua_State *L)
	{
		if (!lua_isstring(L, 1))
		{
			SysLog("argument error");
			return 0;
		}

		const char* sz = lua_tostring(L, 1);
		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
		pPC->DeleteFlag(pPC->GetCurrentQuestName()+"."+sz);
		return 0;
	}

	int32_t pc_give_exp2(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1))
			return 0;

		PyLog("QUEST [REWARD] {} give exp2 {}", ch->GetName(), (int32_t)rint(lua_tonumber(L,1)));

		uint32_t exp = (uint32_t)rint(lua_tonumber(L,1));

		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);
		ch->PointChange(POINT_EXP, exp);
		return 0;
	}

	int32_t pc_give_exp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		PyLog("QUEST [REWARD] {} give exp {} {}", ch->GetName(), lua_tostring(L,1), (int32_t)rint(lua_tonumber(L,2)));

		uint32_t exp = (uint32_t)rint(lua_tonumber(L,2));

		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();

		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);

		pPC->GiveExp(lua_tostring(L,1), exp);
		return 0;
	}

	int32_t pc_give_exp_perc(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (!ch || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
			return 0;

		int32_t lev = (int32_t)rint(lua_tonumber(L,2));
		double proc = (lua_tonumber(L,3));

		PyLog("QUEST [REWARD] {} give exp {} lev {} percent %g%", ch->GetName(), lua_tostring(L, 1), lev, proc);

		uint32_t exp = (uint32_t)((exp_table[MINMAX(0, lev, PLAYER_EXP_TABLE_MAX)]* proc) / 100);
		PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
		
		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);

		pPC->GiveExp(lua_tostring(L, 1), exp);
		return 0;
	}

	int32_t pc_get_empire(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetEmpire());
		return 1;
	}

	int32_t pc_get_part(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1))
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		int32_t part_idx = (int32_t)lua_tonumber(L, 1);
		lua_pushnumber(L, ch->GetPart(part_idx));
		return 1;
	}

	int32_t pc_set_part(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			return 0;
		}
		int32_t part_idx = (int32_t)lua_tonumber(L, 1);
		int32_t part_value = (int32_t)lua_tonumber(L, 2);
		ch->SetPart(part_idx, part_value);
		ch->UpdatePacket();
		return 0;
	}

	int32_t pc_get_skillgroup(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetSkillGroup());
		return 1;
	}

	int32_t pc_set_skillgroup(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("QUEST wrong skillgroup number");
		}
		else
		{
			auto q = CQuestManager::GetInstance();
			LPCHARACTER ch = q->GetCurrentCharacterPtr();

			ch->SetSkillGroup((uint8_t) rint(lua_tonumber(L, 1)));
		}
		return 0;
	}

	int32_t pc_is_polymorphed(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->IsPolymorphed());
		return 1;
	}

	int32_t pc_remove_polymorph(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_POLYMORPH);
		ch->SetPolymorph(0);
		return 0;
	}

	int32_t pc_polymorph(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
		int32_t iDuration = (int32_t) lua_tonumber(L, 2);
		ch->AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
		return 0;
	}

	int32_t pc_is_mount(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetMountVnum());
		return 1;
	}

	int32_t pc_mount(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		int32_t length = 60;

		if (lua_isnumber(L, 2))
			length = (int32_t)lua_tonumber(L, 2);

		uint32_t mount_vnum = (uint32_t)lua_tonumber(L, 1);

		if (length < 0)
			length = 60;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);

		if (ch->GetHorse())
			ch->HorseSummon(false);

		if (mount_vnum)
		{
			ch->AddAffect(AFFECT_MOUNT, POINT_MOUNT, mount_vnum, AFF_NONE, length, 0, true);
			switch (mount_vnum)
			{
			case 20201:
			case 20202:
			case 20203:
			case 20204:
			case 20213:
			case 20216:
			ch->AddAffect(AFFECT_MOUNT, POINT_MOV_SPEED, 30, AFF_NONE, length, 0, true, true);
			break;

			case 20205:
			case 20206:
			case 20207:
			case 20208:
			case 20214:
			case 20217:
			ch->AddAffect(AFFECT_MOUNT, POINT_MOV_SPEED, 40, AFF_NONE, length, 0, true, true);
			break;

			case 20209:
			case 20210:
			case 20211:
			case 20212:
			case 20215:
			case 20218:
			ch->AddAffect(AFFECT_MOUNT, POINT_MOV_SPEED, 50, AFF_NONE, length, 0, true, true);
			break;

			}
		}
		
		return 0;
	}

	int32_t pc_mount_bonus(lua_State* L)
	{
		uint8_t applyOn = static_cast<uint8_t>(lua_tonumber(L, 1));
		int32_t value = static_cast<int32_t>(lua_tonumber(L, 2));
		int32_t duration = static_cast<int32_t>(lua_tonumber(L, 3));

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if(NULL != ch)
		{
			ch->RemoveAffect(AFFECT_MOUNT_BONUS);
			ch->AddAffect(AFFECT_MOUNT_BONUS, aApplyInfo[applyOn].bPointType, value, AFF_NONE, duration, 0, false);
		}

		return 0;
	}

	int32_t pc_unmount(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);
		if (ch->IsHorseRiding())
			ch->StopRiding();
		return 0;
	}

	int32_t pc_get_horse_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetHorseLevel());
		return 1;
	}

	int32_t pc_get_horse_hp(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch->GetHorseLevel())
			lua_pushnumber(L, ch->GetHorseHealth());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t pc_get_horse_stamina(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch->GetHorseLevel())
			lua_pushnumber(L, ch->GetHorseStamina());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t pc_is_horse_alive(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetHorseLevel() > 0 && ch->GetHorseHealth()>0);
		return 1;
	}

	int32_t pc_revive_horse(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->ReviveHorse();
		return 0;
	}

	int32_t pc_have_map_scroll(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		const char* szMapName = lua_tostring(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::GetInstance()->FindRegionByPartialName(szMapName);

		if (!region)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		bool bFind = false;
		for (int32_t iCell = 0; iCell < INVENTORY_MAX_NUM; iCell++)
		{
			LPITEM item = ch->GetInventoryItem(iCell);
			if (!item)
				continue;

			if (item->GetType() == ITEM::TYPE_USE && 
					item->GetSubType() == ITEM::USE_TALISMAN && 
					(item->GetValue(0) == 1 || item->GetValue(0) == 2))
			{
				int32_t x = item->GetSocket(0);
				int32_t y = item->GetSocket(1);
				if (region->sx <=x && region->sy <= y && x <= region->ex && y <= region->ey)
				{
					bFind = true;
					break;
				}
			}
		}

		lua_pushboolean(L, bFind);
		return 1;
	}

	int32_t pc_get_war_map(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetWarMap() ? ch->GetWarMap()->GetMapIndex() : 0);
		return 1;
	}

	int32_t pc_have_pos_scroll(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			SysLog("invalid x y position");
			lua_pushboolean(L, 0);
			return 1;
		}

		if (!lua_isnumber(L,2))
		{
			SysLog("invalid radius");
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t x = (int32_t)lua_tonumber(L, 1);
		int32_t y = (int32_t)lua_tonumber(L, 2);
		float r = (float)lua_tonumber(L, 3);

		bool bFind = false;
		for (int32_t iCell = 0; iCell < INVENTORY_MAX_NUM; iCell++)
		{
			LPITEM item = ch->GetInventoryItem(iCell);
			if (!item)
				continue;

			if (item->GetType() == ITEM::TYPE_USE && 
					item->GetSubType() == ITEM::USE_TALISMAN && 
					(item->GetValue(0) == 1 || item->GetValue(0) == 2))
			{
				int32_t item_x = item->GetSocket(0);
				int32_t item_y = item->GetSocket(1);
				if ((x-item_x)*(x-item_x)+(y-item_y)*(y-item_y)<r*r)
				{
					bFind = true;
					break;
				}
			}
		}

		lua_pushboolean(L, bFind);
		return 1;
	}

	int32_t pc_get_equip_refine_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t cell = (int32_t) lua_tonumber(L, 1);
		if (cell < 0 || cell >= WEAR_MAX_NUM)
		{
			SysLog("invalid wear position {}", cell);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPITEM item = ch->GetWear(cell);
		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, item->GetRefineLevel());
		return 1;
	}

	int32_t pc_refine_equip(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			SysLog("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t cell = (int32_t) lua_tonumber(L, 1);
		int32_t level_limit = (int32_t) lua_tonumber(L, 2);
		int32_t pct = lua_isnumber(L, 3) ? (int32_t)lua_tonumber(L, 3) : 100;

		LPITEM item = ch->GetWear(cell);
		if (!item)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (item->GetRefinedVnum() == 0)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (item->GetRefineLevel()>level_limit)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (pct == 100 || number(1, 100) <= pct)
		{
			lua_pushboolean(L, 1);

			LPITEM pNewItem = ITEM_MANAGER::GetInstance()->CreateItem(item->GetRefinedVnum(), 1, 0, false);

			if (pNewItem)
			{
				for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
					if (!item->GetSocket(i))
						break;
					else
						pNewItem->SetSocket(i, 1);

				int32_t set = 0;
				for (int32_t i=0; i<ITEM::SOCKET_MAX_NUM; i++)
				{
					int32_t socket = item->GetSocket(i);
					if (socket > 2 && socket != 28960)
					{
						pNewItem->SetSocket(set++, socket);
					}
				}

				item->CopyAttributeTo(pNewItem);

				ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (REFINE SUCCESS)");

				pNewItem->EquipTo(ch, cell);

				ITEM_MANAGER::GetInstance()->FlushDelayedSave(pNewItem);

				LogManager::GetInstance()->ItemLog(ch, pNewItem, "REFINE SUCCESS (QUEST)", pNewItem->GetName());
			}
		}
		else
		{
			lua_pushboolean(L, 0);
		}

		return 1;
	}

	int32_t pc_get_skill_level(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
		lua_pushnumber(L, ch->GetSkillLevel(dwVnum));

		return 1;
	}

	int32_t pc_give_lotto(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		PyLog("TRY GIVE LOTTO TO pid {}", ch->GetPlayerID());

		uint32_t* pdw = M2_NEW uint32_t[3];

		pdw[0] = 50001;
		pdw[1] = 1;
		pdw[2] = q->GetEventFlag("lotto_round");

		DBManager::GetInstance()->ReturnQuery(QID_LOTTO, ch->GetPlayerID(), pdw,
				"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())",
				get_table_postfix(), ch->GetPlayerID());

		return 0;
	}

	int32_t pc_aggregate_monster(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->AggregateMonster();
		return 0;
	}

	int32_t pc_forget_my_attacker(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->ForgetMyAttacker();
		return 0;
	}

	int32_t pc_attract_ranger(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->AttractRanger();
		return 0;
	}

	int32_t pc_select_pid(lua_State* L)
	{
		uint32_t pid = (uint32_t) lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER new_ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pid);

		if (new_ch)
		{
			CQuestManager::GetInstance()->GetPC(new_ch->GetPlayerID());

			lua_pushnumber(L, ch->GetPlayerID());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_select_vid(lua_State* L)
	{
		uint32_t vid = (uint32_t) lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER new_ch = CHARACTER_MANAGER::GetInstance()->Find(vid);

		if (new_ch)
		{
			CQuestManager::GetInstance()->GetPC(new_ch->GetPlayerID());

			lua_pushnumber(L, (uint32_t)ch->GetVID());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_get_sex(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, GET_SEX(ch)); /* 0==MALE, 1==FEMALE */
		return 1;
	}

	int32_t pc_is_engaged(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::GetInstance()->IsEngaged(ch->GetPlayerID()));
		return 1;
	}

	int32_t pc_is_married(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::GetInstance()->IsMarried(ch->GetPlayerID()));
		return 1;
	}

	int32_t pc_is_engaged_or_married(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::GetInstance()->IsEngagedOrMarried(ch->GetPlayerID()));
		return 1;
	}

	int32_t pc_is_gm(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetGMLevel() >= GM_HIGH_WIZARD);
		return 1;
	}

	int32_t pc_get_gm_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetGMLevel());
		return 1;
	}

	int32_t pc_mining(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::GetInstance()->GetCurrentNPCCharacterPtr();
		ch->mining(npc);
		return 0;
	}

	int32_t pc_diamond_refine(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t cost = (int32_t) lua_tonumber(L, 1);
		int32_t pct = (int32_t)lua_tonumber(L, 2);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::GetInstance()->GetCurrentNPCCharacterPtr();
		LPITEM item = CQuestManager::GetInstance()->GetCurrentItem();

		if (item)
			lua_pushboolean(L, mining::OreRefine(ch, npc, item, cost, pct, NULL));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int32_t pc_ore_refine(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t cost = (int32_t) lua_tonumber(L, 1);
		int32_t pct = (int32_t)lua_tonumber(L, 2);
		int32_t metinstone_cell = (int32_t)lua_tonumber(L, 3);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::GetInstance()->GetCurrentNPCCharacterPtr();
		LPITEM item = CQuestManager::GetInstance()->GetCurrentItem();

		LPITEM metinstone_item = ch->GetInventoryItem(metinstone_cell);

		if (item && metinstone_item)
			lua_pushboolean(L, mining::OreRefine(ch, npc, item, cost, pct, metinstone_item));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int32_t pc_clear_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch == nullptr) return 0;

		ch->ClearSkill();

		return 0;
	}

	int32_t pc_clear_sub_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch == nullptr) return 0;

		ch->ClearSubSkill();

		return 0;
	}

	int32_t pc_set_skill_point(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t newPoint = (int32_t) lua_tonumber(L, 1);

		ch->SetRealPoint(POINT_SKILL, newPoint);
		ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
		ch->PointChange(POINT_SKILL, 0);
		ch->ComputePoints();
		ch->PointsPacket();

		return 0;
	}

	int32_t pc_clear_one_skill(lua_State* L)
	{
		int32_t vnum = (int32_t)lua_tonumber(L, 1);
		PyLog("{} skill clear", vnum);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch == nullptr)
		{
			PyLog("skill clear fail");
			lua_pushnumber(L, 0);
			return 1;
		}

		PyLog("{} skill clear", vnum);

		ch->ResetOneSkill(vnum);

		return 0;
	}

	int32_t pc_is_clear_skill_group(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_pushboolean(L, ch->GetQuestFlag("skill_group_clear.clear") == 1);
	
		return 1;
	}

	int32_t pc_save_exit_location(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->SaveExitLocation();

		return 0;
	}

	int32_t pc_teleport (lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t x=0,y=0;	
		if (lua_isnumber(L, 1))
		{
			const int32_t TOWN_NUM = 10;
			struct warp_by_town_name
			{
				const char* name;
				uint32_t x;
				uint32_t y;
			} ws[TOWN_NUM] = 
			{
				{"1",		4743,	9548},
				{"2",		3235,	9086},
				{"3",		3531,	8829},
				{"4",		638,	1664},
				{"5",		1745,	1909},
				{"6",		1455,	2400},
				{"7",		9599,	2692},
				{"8",		8036,	2984},
				{"9",		8639,	2460},
				{"10",		4350,	2143},
			};
			int32_t idx  = (int32_t)lua_tonumber(L, 1);

			x = ws[idx].x;
			y = ws[idx].y;
			goto teleport_area;
		}

		else
		{
			const char* arg1 = lua_tostring(L, 1);

			LPCHARACTER tch = CHARACTER_MANAGER::GetInstance()->FindPC(arg1);

			if (!tch)
			{
				const CCI* pCCI = P2P_MANAGER::GetInstance()->Find(arg1);

				if (pCCI)
				{
					if (pCCI->bChannel != g_bChannel)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pCCI->bChannel, g_bChannel);
					}
					else
					{

						PIXEL_POSITION pos;

						if (!SECTREE_MANAGER::GetInstance()->GetCenterPositionOfMap(pCCI->lMapIndex, pos))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pCCI->lMapIndex);
						}
						else
						{
							ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", pos.x, pos.y);
							ch->WarpSet(pos.x, pos.y);
							lua_pushnumber(L, 1);
						}
					}
				}
				else if (!CHARACTER_MANAGER::GetInstance()->FindPC(arg1))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
				}

				lua_pushnumber(L, 0);

				return 1;
			}
			else
			{
				x = tch->GetX() / 100;
				y = tch->GetY() / 100;
			}
		}

teleport_area:

		x *= 100;
		y *= 100;

		ch->ChatPacket(CHAT_TYPE_INFO, "You warp to (%d, %d)", x, y);
		ch->WarpSet(x,y);
		ch->Stop();
		lua_pushnumber(L, 1);
		return 1;
	}

	int32_t pc_set_skill_level(lua_State* L)
	{
		uint32_t dwVnum = (uint32_t)lua_tonumber(L, 1);
		uint8_t byLev = (uint8_t)lua_tonumber(L, 2);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		ch->SetSkillLevel(dwVnum, byLev);

		ch->SkillLevelPacket();

		return 0;
	}

	int32_t pc_give_polymorph_book(lua_State* L)
	{
		if (lua_isnumber(L, 1) != true && lua_isnumber(L, 2) != true && lua_isnumber(L, 3) != true && lua_isnumber(L, 4) != true)
		{
			SysLog("Wrong Quest Function Arguments: pc_give_polymorph_book");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CPolymorphUtils::GetInstance()->GiveBook(ch, (uint32_t)lua_tonumber(L, 1), (uint32_t)lua_tonumber(L, 2), (uint8_t)lua_tonumber(L, 3), (uint8_t)lua_tonumber(L, 4));

		return 0;
	}

	int32_t pc_upgrade_polymorph_book(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPITEM pItem = CQuestManager::GetInstance()->GetCurrentItem();

		bool ret = CPolymorphUtils::GetInstance()->BookUpgrade(ch, pItem);

		lua_pushboolean(L, ret);

		return 1;
	}

	int32_t pc_get_premium_remain_sec(lua_State* L)
	{
		int32_t	remain_seconds	= 0;
		int32_t	premium_type	= 0;
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			SysLog("wrong premium index (is not number)");
			return 0;
		}

		premium_type = (int32_t)lua_tonumber(L,1);
		switch (premium_type)
		{
			case PREMIUM_EXP:
			case PREMIUM_ITEM:
			case PREMIUM_SAFEBOX:
			case PREMIUM_AUTOLOOT:
			case PREMIUM_FISH_MIND:
			case PREMIUM_MARRIAGE_FAST:
			case PREMIUM_GOLD:
				break;

			default:
				SysLog("wrong premium index {}", premium_type);
				return 0;
		}

		remain_seconds = ch->GetPremiumRemainSeconds(premium_type);

		lua_pushnumber(L, remain_seconds);
		return 1;
	}	

	int32_t pc_send_block_mode(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->SetBlockModeForce((uint8_t)lua_tonumber(L, 1));

		return 0;
	}

	int32_t pc_change_empire(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		
		lua_pushnumber(L, ch->ChangeEmpire((uint8_t)lua_tonumber(L, 1)));

		return 1;
	}

	int32_t pc_get_change_empire_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_pushnumber(L, ch->GetChangeEmpireCount());

		return 1;
	}

	int32_t pc_set_change_empire_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->SetChangeEmpireCount();

		return 0;
	}
	
	int32_t pc_change_name(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetNewName().size() != 0)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (lua_isstring(L, 1) != true)
		{
			lua_pushnumber(L, 1);
			return 1;
		}

		const char* szName = lua_tostring(L, 1);

		if (check_name(szName) == false)
		{
			lua_pushnumber(L, 2);
			return 1;
		}

		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM player%s WHERE name='%s'", get_table_postfix(), szName);
		std::unique_ptr<SQLMsg> pmsg(DBManager::GetInstance()->DirectQuery(szQuery));

		if (pmsg->Get()->uiNumRows > 0)
		{
			MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

			int32_t	count = 0;
			str_to_number(count, row[0]);

			if (count != 0)
			{
				lua_pushnumber(L, 3);
				return 1;
			}
		}

		uint32_t pid = ch->GetPlayerID();
		db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(uint32_t));
		db_clientdesc->Packet(&pid, sizeof(uint32_t));

		/* delete messenger list */
		MessengerManager::GetInstance()->RemoveAllList(ch->GetName());

		/* change_name_log */
		LogManager::GetInstance()->ChangeNameLog(pid, ch->GetName(), szName, ch->GetDesc()->GetHostName());

		snprintf(szQuery, sizeof(szQuery), "UPDATE player%s SET name='%s' WHERE id=%u", get_table_postfix(), szName, pid);
		SQLMsg * msg = DBManager::GetInstance()->DirectQuery(szQuery);
		M2_DELETE(msg);

		ch->SetNewName(szName);
		lua_pushnumber(L, 4);
		return 1;
	}

	int32_t pc_is_dead(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch != nullptr)
		{
			lua_pushboolean(L, ch->IsDead());
			return 1;
		}

		lua_pushboolean(L, true);

		return 1;
	}

	int32_t pc_reset_status(lua_State* L)
	{
		if (lua_isnumber(L, 1))
		{
			int32_t idx = (int32_t)lua_tonumber(L, 1);

			if (idx >= 0 && idx < 4)
			{
				LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
				int32_t point = POINT_NONE;
				char buf[128];

				switch (idx)
				{
					case 0 : point = POINT_HT; break;
					case 1 : point = POINT_IQ; break;
					case 2 : point = POINT_ST; break;
					case 3 : point = POINT_DX; break;
					default : lua_pushboolean(L, false); return 1;
				}

				int32_t old_val = ch->GetRealPoint(point);
				int32_t old_stat = ch->GetRealPoint(POINT_STAT);

				ch->SetRealPoint(point, 1);
				ch->SetPoint(point, ch->GetRealPoint(point));

				ch->PointChange(POINT_STAT, old_val-1);

				if (point == POINT_HT)
				{
					uint8_t job = ch->GetJob();
					ch->SetRandomHP((ch->GetLevel()-1) * number(JobInitialPoints[job].hp_per_lv_begin, JobInitialPoints[job].hp_per_lv_end));
				}
				else if (point == POINT_IQ)
				{
					uint8_t job = ch->GetJob();
					ch->SetRandomSP((ch->GetLevel()-1) * number(JobInitialPoints[job].sp_per_lv_begin, JobInitialPoints[job].sp_per_lv_end));
				}

				ch->ComputePoints();
				ch->PointsPacket();

				if (point == POINT_HT)
				{
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				}
				else if (point == POINT_IQ)
				{
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				}

				switch (idx)
				{
					case 0 :
						snprintf(buf, sizeof(buf), "reset ht(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 1 :
						snprintf(buf, sizeof(buf), "reset iq(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 2 :
						snprintf(buf, sizeof(buf), "reset st(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 3 :
						snprintf(buf, sizeof(buf), "reset dx(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
				}

				LogManager::GetInstance()->CharLog(ch, 0, "RESET_ONE_STATUS", buf);

				lua_pushboolean(L, true);
				return 1;
			}
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int32_t pc_get_ht(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_HT));
		return 1;
	}

	int32_t pc_set_ht(lua_State* L)
	{
		if (lua_isnumber(L, 1) == false)
			return 1;

		int32_t newPoint = (int32_t)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t usedPoint = newPoint - ch->GetRealPoint(POINT_HT);
		ch->SetRealPoint(POINT_HT, newPoint);
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}

	int32_t pc_get_iq(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_IQ));
		return 1;
	}

	int32_t pc_set_iq(lua_State* L)
	{
		if (lua_isnumber(L, 1) == false)
			return 1;

		int32_t newPoint = (int32_t)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t usedPoint = newPoint - ch->GetRealPoint(POINT_IQ);
		ch->SetRealPoint(POINT_IQ, newPoint);
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}
	
	int32_t pc_get_st(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_ST));
		return 1;
	}

	int32_t pc_set_st(lua_State* L)
	{
		if (lua_isnumber(L, 1) == false)
			return 1;

		int32_t newPoint = (int32_t)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t usedPoint = newPoint - ch->GetRealPoint(POINT_ST);
		ch->SetRealPoint(POINT_ST, newPoint);
		ch->PointChange(POINT_ST, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}
	
	int32_t pc_get_dx(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_DX));
		return 1;
	}

	int32_t pc_set_dx(lua_State* L)
	{
		if (lua_isnumber(L, 1) == false)
			return 1;

		int32_t newPoint = (int32_t)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t usedPoint = newPoint - ch->GetRealPoint(POINT_DX);
		ch->SetRealPoint(POINT_DX, newPoint);
		ch->PointChange(POINT_DX, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}

	int32_t pc_is_near_vid(lua_State* L)
	{
		if (lua_isnumber(L, 1) != true || lua_isnumber(L, 2) != true)
		{
			lua_pushboolean(L, false);
		}
		else
		{
			LPCHARACTER pMe = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
			LPCHARACTER pOther = CHARACTER_MANAGER::GetInstance()->Find((uint32_t)lua_tonumber(L, 1));

			if (pMe != nullptr && pOther != nullptr)
			{
				lua_pushboolean(L, (DISTANCE_APPROX(pMe->GetX() - pOther->GetX(), pMe->GetY() - pOther->GetY()) < (int32_t)lua_tonumber(L, 2)*100));
			}
			else
			{
				lua_pushboolean(L, false);
			}
		}

		return 1;
	}

	int32_t pc_get_socket_items(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_newtable(L);

		if (pChar == nullptr) return 1;

		int32_t idx = 1;

		for (int32_t i=0; i < INVENTORY_MAX_NUM + WEAR_MAX_NUM; i++)
		{
			LPITEM pItem = pChar->GetInventoryItem(i);

			if (pItem != nullptr)
			{
				if (pItem->IsEquipped() == false)
				{
					int32_t j = 0;
					for (; j < ITEM::SOCKET_MAX_NUM; j++)
					{
						int32_t socket = pItem->GetSocket(j);

						if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
						{
							TItemTable* pItemInfo = ITEM_MANAGER::GetInstance()->GetTable(socket);
							if (pItemInfo != nullptr)
							{
								if (pItemInfo->bType == ITEM::TYPE_METIN) break;
							}
						}
					}

					if (j >= ITEM::SOCKET_MAX_NUM) continue;

					lua_newtable(L);

					{
						lua_pushstring(L, pItem->GetName());
						lua_rawseti(L, -2, 1);

						lua_pushnumber(L, i);
						lua_rawseti(L, -2, 2);
					}

					lua_rawseti(L, -2, idx++);
				}
			}
		}

		return 1;
	}

	int32_t pc_get_empty_inventory_count(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (pChar != nullptr)
		{
			lua_pushnumber(L, pChar->CountEmptyInventory());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_get_logoff_interval(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (pChar != nullptr)
		{
			lua_pushnumber(L, pChar->GetLogOffInterval());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_get_player_id(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (pChar != nullptr)
		{
			lua_pushnumber(L, pChar->GetPlayerID());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_get_account_id(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (pChar != nullptr)
		{
			if (pChar->GetDesc() != nullptr)
			{
				lua_pushnumber(L, pChar->GetDesc()->GetAccountTable().id);
				return 1;
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int32_t pc_get_account(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if(NULL != pChar)
		{
			if(NULL != pChar->GetDesc())
			{
				lua_pushstring(L, pChar->GetDesc()->GetAccountTable().login);
				return 1;
			}
		}

		lua_pushstring(L, "");
		return 1;
	}

	int32_t pc_is_riding(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if(NULL != pChar)
		{
			bool is_riding = pChar->IsRiding();

			lua_pushboolean(L, is_riding);

			return 1;
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int32_t pc_get_special_ride_vnum(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			LPITEM Unique1 = pChar->GetWear(WEAR_UNIQUE1);
			LPITEM Unique2 = pChar->GetWear(WEAR_UNIQUE2);

			if (NULL != Unique1)
			{
				if (UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup())
				{
					lua_pushnumber(L, Unique1->GetVnum());
					lua_pushnumber(L, Unique1->GetSocket(2));
					return 2;
				}
			}

			if (NULL != Unique2)
			{
				if (UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup())
				{
					lua_pushnumber(L, Unique2->GetVnum());
					lua_pushnumber(L, Unique2->GetSocket(2));
					return 2;
				}
			}
		}

		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);

		return 2;
	}

	int32_t pc_can_warp(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			lua_pushboolean(L, pChar->CanWarp());
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int32_t pc_dec_skill_point(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			pChar->PointChange(POINT_SKILL, -1);
		}

		return 0;
	}

	int32_t pc_get_skill_point(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			lua_pushnumber(L, pChar->GetPoint(POINT_SKILL));
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t pc_get_channel_id(lua_State* L)
	{
		lua_pushnumber(L, g_bChannel);

		return 1;
	}

	int32_t pc_give_poly_marble(lua_State* L)
	{
		const int32_t dwVnum = lua_tonumber(L, 1);

		const CMob* MobInfo = CMobManager::GetInstance()->Get(dwVnum);

		if (!MobInfo)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		if (0 == MobInfo->m_table.dwPolymorphItemVnum)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(MobInfo->m_table.dwPolymorphItemVnum);

		if (!item)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		item->SetSocket(0, dwVnum);

		const LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		int32_t iEmptyCell = ch->GetEmptyInventory(item->GetSize());

		if (-1 == iEmptyCell)
		{
			M2_DESTROY_ITEM(item);
			lua_pushboolean(L, false);
			return 1;
		}

		item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyCell));

		const PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();

		LogManager::GetInstance()->QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), MobInfo->m_table.dwPolymorphItemVnum, dwVnum);

		lua_pushboolean(L, true);

		return 1;
	}

	int32_t pc_get_sig_items (lua_State* L)
	{
		uint32_t group_vnum = (uint32_t)lua_tonumber (L, 1);
		const LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t count = 0;
		for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			if (ch->GetInventoryItem(i) != nullptr && ch->GetInventoryItem(i)->GetSIGVnum() == group_vnum)
			{
				lua_pushnumber(L, ch->GetInventoryItem(i)->GetID());
				count++;
			}
		}

		return count;
	}		

	int32_t pc_charge_cash(lua_State * L)
	{
		TRequestChargeCash packet;

		int32_t amount = lua_isnumber(L, 1) ? (int32_t)lua_tonumber(L, 1) : 0;
		std::string strChargeType = lua_isstring(L, 2) ? lua_tostring(L, 2) : "";

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		
		if (!ch || !ch->GetDesc() || 1 > amount || 50000 < amount)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		packet.dwAID = ch->GetDesc()->GetAccountTable().id;
		packet.dwAmount = (uint32_t)amount;
		packet.eChargeType = ERequestCharge_Cash;

		if (0 < strChargeType.length())
			std::transform(strChargeType.begin(), strChargeType.end(), strChargeType.begin(), (int32_t(*)(int32_t))std::tolower);

		if ("mileage" == strChargeType)
			packet.eChargeType = ERequestCharge_Mileage;

		db_clientdesc->DBPacketHeader(HEADER_GD_REQUEST_CHARGE_CASH, 0, sizeof(TRequestChargeCash));
		db_clientdesc->Packet(&packet, sizeof(packet));

		lua_pushboolean(L, 1);
		return 1;
	}

	int32_t pc_get_killee_drop_pct(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER pillee = pChar->GetQuestNPC();

		int32_t iDeltaPercent, iRandRange;
		if (!pillee || !ITEM_MANAGER::GetInstance()->GetDropPct(pillee, pChar, iDeltaPercent, iRandRange))
		{
			SysLog("killee is null");
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);

			return 2;
		}

		lua_pushnumber(L, iDeltaPercent);
		lua_pushnumber(L, iRandRange);
		
		return 2;
	}

	void RegisterPCFunctionTable()
	{
		luaL_reg pc_functions[] = 
		{
			{ "get_wear",		pc_get_wear			},
			{ "get_player_id",	pc_get_player_id	},
			{ "get_account_id", pc_get_account_id	},
			{ "get_account",	pc_get_account		},
			{ "get_level",		pc_get_level		},
			{ "set_level",		pc_set_level		},
			{ "get_next_exp",		pc_get_next_exp		},
			{ "get_exp",		pc_get_exp		},
			{ "get_job",		pc_get_job		},
			{ "get_race",		pc_get_race		},
			{ "change_sex",		pc_change_sex	},
			{ "gethp",			pc_get_hp		},
			{ "get_hp",			pc_get_hp		},
			{ "getmaxhp",		pc_get_max_hp		},
			{ "get_max_hp",		pc_get_max_hp		},
			{ "getsp",			pc_get_sp		},
			{ "get_sp",			pc_get_sp		},
			{ "getmaxsp",		pc_get_max_sp		},
			{ "get_max_sp",		pc_get_max_sp		},
			{ "change_sp",		pc_change_sp		},
			{ "getmoney",		pc_get_money		},
			{ "get_money",		pc_get_money		},
			{ "get_real_alignment",	pc_get_real_alignment	},
			{ "get_alignment",		pc_get_alignment	},
			{ "getweapon",		pc_get_weapon		},
			{ "get_weapon",		pc_get_weapon		},
			{ "getarmor",		pc_get_armor		},
			{ "get_armor",		pc_get_armor		},
			{ "getgold",		pc_get_money		},
			{ "get_gold",		pc_get_money		},
			{ "changegold",		pc_change_money		},
			{ "changemoney",		pc_change_money		},
			{ "changealignment",	pc_change_alignment	},
			{ "change_gold",		pc_change_money		},
			{ "change_money",		pc_change_money		},
			{ "change_alignment",	pc_change_alignment	},
			{ "getname",		pc_get_name		},
			{ "get_name",		pc_get_name		},
			{ "get_vid",		pc_get_vid		},
			{ "getplaytime",		pc_get_playtime		},
			{ "get_playtime",		pc_get_playtime		},
			{ "getleadership",		pc_get_leadership	},
			{ "get_leadership",		pc_get_leadership	},
			{ "getqf",			pc_get_quest_flag	},
			{ "setqf",			pc_set_quest_flag	},
			{ "delqf",			pc_del_quest_flag	},
			{ "getf",			pc_get_another_quest_flag},
			{ "setf",			pc_set_another_quest_flag},
			{ "get_x",			pc_get_x		},
			{ "get_y",			pc_get_y		},
			{ "getx",			pc_get_x		},
			{ "gety",			pc_get_y		},
			{ "get_local_x",		pc_get_local_x		},
			{ "get_local_y",		pc_get_local_y		},
			{ "getcurrentmapindex",	pc_get_current_map_index},
			{ "get_map_index",		pc_get_current_map_index},
			{ "give_exp",		pc_give_exp		},
			{ "give_exp_perc",		pc_give_exp_perc	},
			{ "give_exp2",		pc_give_exp2		},
			{ "give_item",		pc_give_item		},
			{ "give_item2",		pc_give_or_drop_item	},
			{ "give_item2_select",		pc_give_or_drop_item_and_select	},
			{ "give_gold",		pc_give_gold		},
			{ "count_item",		pc_count_item		},
			{ "remove_item",		pc_remove_item		},
			{ "countitem",		pc_count_item		},
			{ "removeitem",		pc_remove_item		},
			{ "reset_point",		pc_reset_point		},
			{ "has_guild",		pc_hasguild		},
			{ "hasguild",		pc_hasguild		},
			{ "get_guild",		pc_getguild		},
			{ "getguild",		pc_getguild		},
			{ "isguildmaster",		pc_isguildmaster	},
			{ "is_guild_master",	pc_isguildmaster	},
			{ "destroy_guild",		pc_destroy_guild	},
			{ "remove_from_guild",	pc_remove_from_guild	},
			{ "in_dungeon",		pc_in_dungeon		},
			{ "getempire",		pc_get_empire		},
			{ "get_empire",		pc_get_empire		},
			{ "get_skill_group",	pc_get_skillgroup	},
			{ "set_skill_group",	pc_set_skillgroup	},
			{ "warp",			pc_warp			},
			{ "warp_local",		pc_warp_local		},
			{ "warp_exit",		pc_warp_exit		},
			{ "set_warp_location",	pc_set_warp_location	},
			{ "set_warp_location_local",pc_set_warp_location_local },
			{ "get_start_location",	pc_get_start_location	},
			{ "has_master_skill",	pc_has_master_skill	},
			{ "set_part",		pc_set_part		},
			{ "get_part",		pc_get_part		},
			{ "is_polymorphed",		pc_is_polymorphed	},
			{ "remove_polymorph",	pc_remove_polymorph	},
			{ "is_mount",		pc_is_mount		},
			{ "polymorph",		pc_polymorph		},
			{ "mount",			pc_mount		},
			{ "mount_bonus",	pc_mount_bonus	},
			{ "unmount",		pc_unmount		},
			{ "warp_to_guild_war_observer_position", pc_warp_to_guild_war_observer_position	},
			{ "give_item_from_special_item_group", pc_give_item_from_special_item_group	},
			{ "learn_grand_master_skill", pc_learn_grand_master_skill	},
			{ "is_skill_book_no_delay",	pc_is_skill_book_no_delay}, 
			{ "remove_skill_book_no_delay",	pc_remove_skill_book_no_delay}, 

			{ "enough_inventory",	pc_enough_inventory	},
			{ "get_horse_level",	pc_get_horse_level	}, // TO BE DELETED XXX
			{ "is_horse_alive",		pc_is_horse_alive	}, // TO BE DELETED XXX
			{ "revive_horse",		pc_revive_horse		}, // TO BE DELETED XXX
			{ "have_pos_scroll",	pc_have_pos_scroll	},
			{ "have_map_scroll",	pc_have_map_scroll	},
			{ "get_war_map",		pc_get_war_map		},
			{ "get_equip_refine_level",	pc_get_equip_refine_level },
			{ "refine_equip",		pc_refine_equip		},
			{ "get_skill_level",	pc_get_skill_level	},
			{ "give_lotto",		pc_give_lotto		},
			{ "aggregate_monster",	pc_aggregate_monster	},
			{ "forget_my_attacker",	pc_forget_my_attacker	},
			{ "pc_attract_ranger",	pc_attract_ranger	},
			{ "select",			pc_select_vid		},
			{ "get_sex",		pc_get_sex		},
			{ "is_married",		pc_is_married		},
			{ "is_engaged",		pc_is_engaged		},
			{ "is_engaged_or_married",	pc_is_engaged_or_married},
			{ "is_gm",			pc_is_gm		},
			{ "get_gm_level",		pc_get_gm_level		},
			{ "mining",			pc_mining		},
			{ "ore_refine",		pc_ore_refine		},
			{ "diamond_refine",		pc_diamond_refine	},

			// RESET_ONE_SKILL
			{ "clear_one_skill",        pc_clear_one_skill      },
			// END_RESET_ONE_SKILL

			{ "clear_skill",                pc_clear_skill          },
			{ "clear_sub_skill",    pc_clear_sub_skill      },
			{ "set_skill_point",    pc_set_skill_point      },

			{ "is_clear_skill_group",	pc_is_clear_skill_group		},

			{ "save_exit_location",		pc_save_exit_location		},
			{ "teleport",				pc_teleport },

			{ "set_skill_level",        pc_set_skill_level      },

            { "give_polymorph_book",    pc_give_polymorph_book  },
            { "upgrade_polymorph_book", pc_upgrade_polymorph_book },
            { "get_premium_remain_sec", pc_get_premium_remain_sec },
   
			{ "send_block_mode",		pc_send_block_mode	},
			
			{ "change_empire",			pc_change_empire	},
			{ "get_change_empire_count",	pc_get_change_empire_count	},
			{ "set_change_empire_count",	pc_set_change_empire_count	},

			{ "change_name",			pc_change_name },

			{ "is_dead",				pc_is_dead	},

			{ "reset_status",		pc_reset_status	},
			{ "get_ht",				pc_get_ht	},
			{ "set_ht",				pc_set_ht	},
			{ "get_iq",				pc_get_iq	},
			{ "set_iq",				pc_set_iq	},
			{ "get_st",				pc_get_st	},
			{ "set_st",				pc_set_st	},
			{ "get_dx",				pc_get_dx	},
			{ "set_dx",				pc_set_dx	},

			{ "is_near_vid",		pc_is_near_vid	},

			{ "get_socket_items",	pc_get_socket_items	},
			{ "get_empty_inventory_count",	pc_get_empty_inventory_count	},

			{ "get_logoff_interval",	pc_get_logoff_interval	},

			{ "is_riding",			pc_is_riding	},
			{ "get_special_ride_vnum",	pc_get_special_ride_vnum	},

			{ "can_warp",			pc_can_warp		},

			{ "dec_skill_point",	pc_dec_skill_point	},
			{ "get_skill_point",	pc_get_skill_point	},

			{ "get_channel_id",		pc_get_channel_id	},

			{ "give_poly_marble",	pc_give_poly_marble	},
			{ "get_sig_items",		pc_get_sig_items	},

			{ "charge_cash",		pc_charge_cash		},

			{ "get_killee_drop_pct",	pc_get_killee_drop_pct	},

			{ NULL,			NULL			}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("pc", pc_functions);
	}
};
