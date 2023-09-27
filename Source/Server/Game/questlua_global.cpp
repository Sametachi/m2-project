#include "stdafx.h"
#include <sstream>
#include "constants.h"
#include "char.h"
#include "char_manager.h"
#include "log.h"
#include "questmanager.h"
#include "questlua.h"
#include "questevent.h"
#include "config.h"
#include "mining.h"
#include "fishing.h"
#include "priv_manager.h"
#include "utils.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "over9refine.h"
#include "OXEvent.h"
#include "regen.h"
#include "cmd.h"
#include "guild.h"
#include "guild_manager.h" 
#include "sectree_manager.h"

extern ACMD(do_block_chat);

namespace quest
{
	int32_t _get_locale(lua_State* L)
	{
		lua_pushstring(L, g_stLocale.c_str());
		return 1;
	}

	int32_t _number(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, number((int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2)));
		return 1;
	}

	int32_t _time_to_str(lua_State* L)
	{
		time_t curTime = (time_t)lua_tonumber(L, -1);
		lua_pushstring(L, asctime(gmtime(&curTime)));
		return 1;
	}

	int32_t _say(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::GetInstance()->AddScript(s.str() + "[ENTER]");
		return 0;
	}

	int32_t _chat(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);

		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_TALKING, "%s", s.str().c_str());
		return 0;
	}

	int32_t _cmdchat(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_COMMAND, "%s", s.str().c_str());
		return 0;
	}

	int32_t _syschat(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_INFO, "%s", s.str().c_str());
		return 0;
	}

	int32_t _notice(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::GetInstance()->GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_NOTICE, "%s", s.str().c_str());
		return 0;
	}

	int32_t _left_image(lua_State* L)
	{
		if (lua_isstring(L, -1))
		{
			std::string s = lua_tostring(L,-1);
			CQuestManager::GetInstance()->AddScript("[LEFTIMAGE src;"+s+"]");
		}
		return 0;
	}

	int32_t _top_image(lua_State* L)
	{
		if (lua_isstring(L, -1))
		{
			std::string s = lua_tostring(L,-1);
			CQuestManager::GetInstance()->AddScript("[TOPIMAGE src;"+s+"]");
		}
		return 0;
	}

	int32_t _set_skin(lua_State* L) // Quest UI style
	{
		if (lua_isnumber(L, -1))
		{
			CQuestManager::GetInstance()->SetSkinStyle((int32_t)rint(lua_tonumber(L,-1)));
		}
		else
		{
			SysLog("QUEST wrong skin index");
		}

		return 0;
	}

	int32_t _set_server_timer(lua_State* L)
	{
		int32_t n = lua_gettop(L);
		if ((n != 2 || !lua_isnumber(L, 2) || !lua_isstring(L, 1)) &&
				(n != 3 || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)))
		{
			SysLog("QUEST set_server_timer argument count wrong.");
			return 0;
		}

		const char* name = lua_tostring(L, 1);
		double t = lua_tonumber(L, 2);
		uint32_t arg = 0;

		auto q = CQuestManager::GetInstance();

		if (lua_isnumber(L, 3))
			arg = (uint32_t) lua_tonumber(L, 3);

		int32_t timernpc = q->LoadTimerScript(name);

		LPEVENT event = quest_create_server_timer_event(name, t, timernpc, false, arg);
		q->AddServerTimer(name, arg, event);
		return 0;
	}

	int32_t _set_server_loop_timer(lua_State* L)
	{
		int32_t n = lua_gettop(L);
		if ((n != 2 || !lua_isnumber(L, 2) || !lua_isstring(L, 1)) &&
				(n != 3 || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)))
		{
			SysLog("QUEST set_server_timer argument count wrong.");
			return 0;
		}
		const char* name = lua_tostring(L, 1);
		double t = lua_tonumber(L, 2);
		uint32_t arg = 0;
		auto q = CQuestManager::GetInstance();

		if (lua_isnumber(L, 3))
			arg = (uint32_t) lua_tonumber(L, 3);

		int32_t timernpc = q->LoadTimerScript(name);

		LPEVENT event = quest_create_server_timer_event(name, t, timernpc, true, arg);
		q->AddServerTimer(name, arg, event);
		return 0;
	}

	int32_t _clear_server_timer(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		const char* name = lua_tostring(L, 1);
		uint32_t arg = (uint32_t) lua_tonumber(L, 2);
		q->ClearServerTimer(name, arg);
		return 0;
	}

	int32_t _set_named_loop_timer(lua_State* L)
	{
		int32_t n = lua_gettop(L);

		if (n != 2 || !lua_isnumber(L, -1) || !lua_isstring(L, -2))
		{
			SysLog("QUEST set_timer argument count wrong.");
		}
		else
		{
			const char* name = lua_tostring(L, -2);
			double t = lua_tonumber(L, -1);

			auto q = CQuestManager::GetInstance();
			int32_t timernpc = q->LoadTimerScript(name);
			q->GetCurrentPC()->AddTimer(name, quest_create_timer_event(name, q->GetCurrentCharacterPtr()->GetPlayerID(), t, timernpc, true));
		}

		return 0;
	}

	int32_t _get_server_timer_arg(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::GetInstance()->GetServerTimerArg());
		return 1;
	}

	int32_t _set_timer(lua_State* L)
	{
		if (lua_gettop(L) != 1 || !lua_isnumber(L, -1))
		{
			SysLog("QUEST invalid argument.");
		}
		else
		{
			double t = lua_tonumber(L, -1);

			auto q = CQuestManager::GetInstance();
			quest_create_timer_event("", q->GetCurrentCharacterPtr()->GetPlayerID(), t);
		}

		return 0;
	}

	int32_t _set_named_timer(lua_State* L)
	{
		int32_t n = lua_gettop(L);

		if (n != 2 || !lua_isnumber(L, -1) || !lua_isstring(L, -2))
		{
			SysLog("QUEST set_timer argument count wrong.");
		}
		else
		{
			const char* name = lua_tostring(L,-2);
			double t = lua_tonumber(L, -1);

			auto q = CQuestManager::GetInstance();
			int32_t timernpc = q->LoadTimerScript(name);
			q->GetCurrentPC()->AddTimer(name, quest_create_timer_event(name, q->GetCurrentCharacterPtr()->GetPlayerID(), t, timernpc));
		}

		return 0;
	}

	int32_t _timer(lua_State * L)
	{
		if (lua_gettop(L) == 1)
			return _set_timer(L);
		else
			return _set_named_timer(L);
	}

	int32_t _clear_named_timer(lua_State* L)
	{
		int32_t n = lua_gettop(L);

		if (n != 1 || !lua_isstring(L, -1))
		{
			SysLog("QUEST set_timer argument count wrong.");
		}
		else
		{
			auto q = CQuestManager::GetInstance();
			q->GetCurrentPC()->RemoveTimer(lua_tostring(L, -1));
		}

		return 0;
	}

	int32_t _getnpcid(lua_State * L)
	{
		const char* name = lua_tostring(L, -1);
		auto q = CQuestManager::GetInstance();
		lua_pushnumber(L, q->FindNPCIDByName(name));
		return 1;
	}

	int32_t _is_test_server(lua_State * L)
	{
		lua_pushboolean(L, test_server);
		return 1;
	}

	int32_t _is_speed_server(lua_State * L)
	{
		lua_pushboolean(L, speed_server);
		return 1;
	}

	int32_t _raw_script(lua_State* L)
	{
		if (test_server)
			PyLog("_raw_script : {} ", lua_tostring(L,-1));
		if (lua_isstring(L, -1))
			CQuestManager::GetInstance()->AddScript(lua_tostring(L,-1));
		else
			SysLog("QUEST wrong argument: questname: {}", CQuestManager::GetInstance()->GetCurrentQuestName().c_str());

		return 0;
	}

	int32_t _char_log(lua_State * L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		uint32_t what = 0;
		const char* how = "";
		const char* hint = "";

		if (lua_isnumber(L, 1)) what = (uint32_t)lua_tonumber(L, 1);
		if (lua_isstring(L, 2)) how = lua_tostring(L, 2);
		if (lua_tostring(L, 3)) hint = lua_tostring(L, 3);

		LogManager::GetInstance()->CharLog(ch, what, how, hint);
		return 0;
	}
	
	int32_t _item_log(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		uint32_t dwItemID = 0;
		const char* how = "";
		const char* hint = "";

		if (lua_isnumber(L, 1)) dwItemID = (uint32_t)lua_tonumber(L, 1);
		if (lua_isstring(L, 2)) how = lua_tostring(L, 2);
		if (lua_tostring(L, 3)) hint = lua_tostring(L, 3);

		LPITEM item = ITEM_MANAGER::GetInstance()->Find(dwItemID);

		if (item)
			LogManager::GetInstance()->ItemLog(ch, item, how, hint);

		return 0;
	}

	int32_t _syslog(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isstring(L, 2))
			return 0;

		if (lua_tonumber(L, 1) >= 1)
		{
			if (!test_server)
				return 0;
		}

		PC* pc = CQuestManager::GetInstance()->GetCurrentPC();

		if (!pc)
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!ch)
			return 0;

		PyLog("QUEST: quest: {} player: {} : {}", pc->GetCurrentQuestName().c_str(), ch->GetName(), lua_tostring(L, 2));

		if (test_server)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "QUEST_SYSLOG %s", lua_tostring(L, 2));
		}

		return 0;
	}

	int32_t _syserr(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		PC* pc = CQuestManager::GetInstance()->GetCurrentPC();

		if (!pc)
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!ch)
			return 0;

		SysLog("QUEST: quest: {} player: {} : {}", pc->GetCurrentQuestName().c_str(), ch->GetName(), lua_tostring(L, 1));
		ch->ChatPacket(CHAT_TYPE_INFO, "QUEST_SYSERR %s", lua_tostring(L, 1));
		return 0;
	}
	
	// LUA_ADD_BGM_INFO
	int32_t _set_bgm_volume_enable(lua_State* L)
	{
		CHARACTER_SetBGMVolumeEnable();

		return 0;
	}

	int32_t _add_bgm_info(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isstring(L, 2))
			return 0;

		int32_t mapIndex		= (int32_t)lua_tonumber(L, 1);

		const char*	bgmName	= lua_tostring(L, 2);
		if (!bgmName)
			return 0;

		float bgmVol = lua_isnumber(L, 3) ? lua_tonumber(L, 3) : (1.0f/5.0f)*0.1f;

		CHARACTER_AddBGMInfo(mapIndex, bgmName, bgmVol);

		return 0;
	}
	// END_OF_LUA_ADD_BGM_INFO	

	// LUA_ADD_GOTO_INFO
	int32_t _add_goto_info(lua_State* L)
	{
		const char* name = lua_tostring(L, 1);

		int32_t empire 	= (int32_t)lua_tonumber(L, 2);
		int32_t mapIndex 	= (int32_t)lua_tonumber(L, 3);
		int32_t x 		= (int32_t)lua_tonumber(L, 4);
		int32_t y 		= (int32_t)lua_tonumber(L, 5);

		if (!name)
			return 0;

		CHARACTER_AddGotoInfo(name, empire, mapIndex, x, y);
		return 0;
	}
	// END_OF_LUA_ADD_GOTO_INFO

	// REFINE_PICK
	int32_t _refine_pick(lua_State* L)
	{
		uint8_t bCell = (uint8_t) lua_tonumber(L,-1);

		auto q = CQuestManager::GetInstance();

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		LPITEM item = ch->GetInventoryItem(bCell);

		int32_t ret = mining::RealRefinePick(ch, item);
		lua_pushnumber(L, ret);
		return 1;
	}
	// END_OF_REFINE_PICK

	int32_t _fish_real_refine_rod(lua_State* L)
	{
		uint8_t bCell = (uint8_t) lua_tonumber(L,-1);

		auto q = CQuestManager::GetInstance();

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		LPITEM item = ch->GetInventoryItem(bCell);

		int32_t ret = fishing::RealRefineRod(ch, item);
		lua_pushnumber(L, ret);
		return 1;
	}

	int32_t _give_char_privilege(lua_State* L)
	{
		int32_t pid = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetPlayerID();
		int32_t type = (int32_t)lua_tonumber(L, 1);
		int32_t value = (int32_t)lua_tonumber(L, 2);

		if (MAX_PRIV_NUM <= type)
		{
			SysLog("PRIV_MANAGER: _give_char_privilege: wrong empire priv type({})", type);
			return 0;
		}

		CPrivManager::GetInstance()->RequestGiveCharacterPriv(pid, type, value);

		return 0;
	}

	int32_t _give_empire_privilege(lua_State* L)
	{
		int32_t empire = (int32_t)lua_tonumber(L,1);
		int32_t type = (int32_t)lua_tonumber(L, 2);
		int32_t value = (int32_t)lua_tonumber(L, 3);
		int32_t time = (int32_t) lua_tonumber(L,4);
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (MAX_PRIV_NUM <= type)
		{
			SysLog("PRIV_MANAGER: _give_empire_privilege: wrong empire priv type({})", type);
			return 0;
		}

		if (ch)
		{
			PyLog("_give_empire_privileage(empire={}, type={}, value={}, time={}), by quest, {}",
				empire, type, value, time, ch->GetName());
		}
		else
			PyLog("_give_empire_privileage(empire={}, type={}, value={}, time={}), by quest, NULL",
					empire, type, value, time);

		CPrivManager::GetInstance()->RequestGiveEmpirePriv(empire, type, value, time);
		return 0;
	}

	int32_t _give_guild_privilege(lua_State* L)
	{
		int32_t guild_id = (int32_t)lua_tonumber(L,1);
		int32_t type = (int32_t)lua_tonumber(L, 2);
		int32_t value = (int32_t)lua_tonumber(L, 3);
		int32_t time = (int32_t)lua_tonumber(L, 4);

		if (MAX_PRIV_NUM <= type)
		{
			SysLog("PRIV_MANAGER: _give_guild_privilege: wrong empire priv type({})", type);
			return 0;
		}

		PyLog("_give_guild_privileage(empire={}, type={}, value={}, time={})", 
				guild_id, type, value, time);

		CPrivManager::GetInstance()->RequestGiveGuildPriv(guild_id,type,value,time);

		return 0;
	}

	int32_t _get_empire_privilege_string(lua_State* L)
	{
		int32_t empire = (int32_t) lua_tonumber(L, 1);
		std::ostringstream os;
		bool found = false;

		for (int32_t type = PRIV_NONE + 1; type < MAX_PRIV_NUM; ++type)
		{
			CPrivManager::SPrivEmpireData* pPrivEmpireData = CPrivManager::GetInstance()->GetPrivByEmpireEx(empire, type);

			if (pPrivEmpireData && pPrivEmpireData->m_value)
			{
				if (found)
					os << ", ";

				os << LC_TEXT(c_apszPrivNames[type]) << " : " << 
					pPrivEmpireData->m_value << "%" << " (" <<
					((pPrivEmpireData->m_end_time_sec-get_global_time())/3600.0f) << " hours)" << std::endl;
				found = true;
			}
		}

		if (!found)
			os << "None!" << std::endl;

		lua_pushstring(L, os.str().c_str());
		return 1;
	}

	int32_t _get_empire_privilege(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L,0);
			return 1;
		}
		int32_t empire = (int32_t)lua_tonumber(L,1);
		int32_t type = (int32_t)lua_tonumber(L,2);
		int32_t value = CPrivManager::GetInstance()->GetPrivByEmpire(empire,type);
		lua_pushnumber(L, value);
		return 1;
	}

	int32_t _get_guild_privilege_string(lua_State* L)
	{
		int32_t guild = (int32_t) lua_tonumber(L,1);
		std::ostringstream os;
		bool found = false;

		for (int32_t type = PRIV_NONE+1; type < MAX_PRIV_NUM; ++type)
		{
			const CPrivManager::SPrivGuildData* pPrivGuildData = CPrivManager::GetInstance()->GetPrivByGuildEx(guild, type);

			if (pPrivGuildData && pPrivGuildData->value)
			{
				if (found)
					os << ", ";

				os << LC_TEXT(c_apszPrivNames[type]) << " : " << pPrivGuildData->value << "%"
					<< " (" << ((pPrivGuildData->end_time_sec - get_global_time()) / 3600.0f) << " hours)" << std::endl;
				found = true;
			}
		}

		if (!found)
			os << "None!" << std::endl;

		lua_pushstring(L, os.str().c_str());
		return 1;
	}

	int32_t _get_guildid_byname(lua_State* L)
	{
		if (!lua_isstring(L, 1)) {
			SysLog("_get_guildid_byname() - invalud argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		const char* pszGuildName = lua_tostring(L, 1);
		CGuild* pFindGuild = CGuildManager::GetInstance()->FindGuildByName(pszGuildName);
		if (pFindGuild)
			lua_pushnumber(L, pFindGuild->GetID());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t _get_guild_privilege(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L,0);
			return 1;
		}
		int32_t guild = (int32_t)lua_tonumber(L,1);
		int32_t type = (int32_t)lua_tonumber(L,2);
		int32_t value = CPrivManager::GetInstance()->GetPrivByGuild(guild,type);
		lua_pushnumber(L, value);
		return 1;
	}

	int32_t _item_name(lua_State* L)
	{
		if (lua_isnumber(L,1))
		{
			uint32_t dwVnum = (uint32_t)lua_tonumber(L,1);
			TItemTable* pTable = ITEM_MANAGER::GetInstance()->GetTable(dwVnum);
			if (pTable)
				lua_pushstring(L,pTable->szLocaleName);
			else
				lua_pushstring(L,"");
		}
		else
			lua_pushstring(L,"");
		return 1;
	}

	int32_t _mob_name(lua_State* L)
	{
		if (lua_isnumber(L, 1))
		{
			uint32_t dwVnum = (uint32_t) lua_tonumber(L,1);
			const CMob* pMob = CMobManager::GetInstance()->Get(dwVnum);

			if (pMob)
				lua_pushstring(L, pMob->m_table.szLocaleName);
			else
				lua_pushstring(L, "");
		}
		else
			lua_pushstring(L,"");

		return 1;
	}

	int32_t _mob_vnum(lua_State* L)
	{
		if (lua_isstring(L,1))
		{
			const char* str = lua_tostring(L, 1);
			const CMob* pMob = CMobManager::GetInstance()->Get(str, false);
			if (pMob)
				lua_pushnumber(L, pMob->m_table.dwVnum);
			else
				lua_pushnumber(L,0);
		}
		else
			lua_pushnumber(L,0);

		return 1;
	}

	int32_t _get_global_time(lua_State* L)
	{
		lua_pushnumber(L, get_global_time());
		return 1;
	}
	

	int32_t _get_channel_id(lua_State* L)
	{
		lua_pushnumber(L, g_bChannel);

		return 1;
	}

	int32_t _do_command(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		const char* str = lua_tostring(L, 1);
		size_t len = strlen(str);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		::interpret_command(ch, str, len);
		return 0;
	}

	int32_t _find_pc(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		const char* name = lua_tostring(L, 1);
		LPCHARACTER tch = CHARACTER_MANAGER::GetInstance()->FindPC(name);
		lua_pushnumber(L, tch ? tch->GetVID() : 0);
		return 1;
	}

	int32_t _find_pc_cond(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		int32_t iMinLev = (int32_t) lua_tonumber(L, 1);
		int32_t iMaxLev = (int32_t) lua_tonumber(L, 2);
		uint32_t uiJobFlag = (uint32_t) lua_tonumber(L, 3);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER tch;

		if (test_server)
		{
			PyLog("find_pc_cond map={}, job={}, level={}~{}",
					ch->GetMapIndex(),
					uiJobFlag,
					iMinLev, iMaxLev);
		}

		tch = CHARACTER_MANAGER::GetInstance()->FindSpecifyPC(uiJobFlag,
				ch->GetMapIndex(),
				ch,
				iMinLev,
				iMaxLev);

		lua_pushnumber(L, tch ? tch->GetVID() : 0);
		return 1;
	}

	int32_t _find_npc_by_vnum(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		uint32_t race = (uint32_t) lua_tonumber(L, 1);

		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(race, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end())
			{
				LPCHARACTER tch = *(it++);

				if (tch->GetMapIndex() == CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetMapIndex())
				{
					lua_pushnumber(L, tch->GetVID());
					return 1;
				}
			}
		}

		//SysLog("not find(race={})", race);

		lua_pushnumber(L, 0);
		return 1;
	}

	// Create a new state.
	int32_t _set_quest_state(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
			return 0;

		auto q = CQuestManager::GetInstance();
		QuestState* pqs = q->GetCurrentState();
		PC* pPC = q->GetCurrentPC();
		//assert(L == pqs->co);
		if (L!=pqs->co) 
		{
			luaL_error(L, "running thread != current thread???");
			PyLog("running thread != current thread???");
			return -1;
		}
		if (pPC)
		{
			//const char* szQuestName = lua_tostring(L, 1);
			//const char* szStateName = lua_tostring(L, 2);
			const std::string stQuestName(lua_tostring(L, 1));
			const std::string stStateName(lua_tostring(L, 2));
			if (test_server)
				PyLog("set_state {} {} ", stQuestName.c_str(), stStateName.c_str());
			if (pPC->GetCurrentQuestName() == stQuestName)
			{
				pqs->st = q->GetQuestStateIndex(pPC->GetCurrentQuestName(), lua_tostring(L, -1));
				pPC->SetCurrentQuestStateName(lua_tostring(L,-1));
			}
			else
			{
				pPC->SetQuestState(stQuestName, stStateName);
			}
		}
		return 0;
	}

	int32_t _get_quest_state(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		auto q = CQuestManager::GetInstance();
		PC* pPC = q->GetCurrentPC();
		
		if (pPC)
		{
			std::string stQuestName	= lua_tostring(L, 1);
			stQuestName += ".__status";

			int32_t nRet = pPC->GetFlag(stQuestName.c_str()); 

			lua_pushnumber(L, nRet);

			if (test_server)
				PyLog("Get_quest_state name {} value {}", stQuestName.c_str(), nRet);
		}
		else
		{
			if (test_server)
				PyLog("PC == 0 ");

			lua_pushnumber(L, 0);
		}
		return 1;
	}

	int32_t _under_han(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			lua_pushboolean(L, 0);
		else
			lua_pushboolean(L, under_han(lua_tostring(L, 1)));
		return 1;
	}

	int32_t _notice_all(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);

		TPacketGGNotice p;
		p.bHeader = HEADER_GG_NOTICE;
		p.lSize = strlen(s.str().c_str()) + 1;

		TEMP_BUFFER buf;
		buf.write(&p, sizeof(p));
		buf.write(s.str().c_str(), p.lSize);

		P2P_MANAGER::GetInstance()->Send(buf.read_peek(), buf.size()); // HEADER_GG_NOTICE

		SendNotice(s.str().c_str());
		return 1;	
	}

	EVENTINFO(warp_all_to_village_event_info)
	{
		uint32_t dwWarpMapIndex;

		warp_all_to_village_event_info() 
		: dwWarpMapIndex(0)
		{
		}
	};

	struct FWarpAllToVillage
	{
		FWarpAllToVillage() {};
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
				if (ch->IsPC())
				{
					uint8_t bEmpire =  ch->GetEmpire();
					if (bEmpire == 0)
					{
						SysLog("Unkonwn Empire {} {} ", ch->GetName(), ch->GetPlayerID());
						return;
					}

					ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
				}
			}
		}
	};

	EVENTFUNC(warp_all_to_village_event)
	{
		warp_all_to_village_event_info * info = dynamic_cast<warp_all_to_village_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("warp_all_to_village_event> <Factor> Null pointer");
			return 0;
		}

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(info->dwWarpMapIndex);

		if (NULL != pSecMap)
		{
			FWarpAllToVillage f;
			pSecMap->for_each(f);
		}

		return 0;
	}

	int32_t _warp_all_to_village(lua_State * L)
	{
		int32_t iMapIndex 	= static_cast<int32_t>(lua_tonumber(L, 1));
		int32_t iSec		= static_cast<int32_t>(lua_tonumber(L, 2));
		
		warp_all_to_village_event_info* info = AllocEventInfo<warp_all_to_village_event_info>();

		info->dwWarpMapIndex = iMapIndex;

		event_create(warp_all_to_village_event, info, PASSES_PER_SEC(iSec));

		SendNoticeMap(LC_TEXT("Everyone will be teleported into the city shortly."), iMapIndex, false);

		return 0;
	}

	int32_t _warp_to_village(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
	
		if (NULL != ch)
		{
			uint8_t bEmpire = ch->GetEmpire();	
			ch->WarpSet(g_start_position[bEmpire][0], g_start_position[bEmpire][1]);
		}

		return 0;
	}

	int32_t _say_in_map(lua_State * L)
	{
		int32_t iMapIndex 		= static_cast<int32_t>(lua_tonumber(L, 1));
		std::string Script(lua_tostring(L, 2));

		Script += "[ENTER]";
		Script += "[DONE]";
		
		struct ::packet_script packet_script;

		packet_script.header = HEADER_GC_SCRIPT;
		packet_script.skin = CQuestManager::QUEST_SKIN_NORMAL;
		packet_script.src_size = Script.size();
		packet_script.size = packet_script.src_size + sizeof(struct packet_script);

		FSendPacket f;
		f.buf.write(&packet_script, sizeof(struct packet_script));
		f.buf.write(&Script[0], Script.size());

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(iMapIndex);

		if (pSecMap)
		{
			pSecMap->for_each(f);
		}

		return 0;
	}

	struct FKillSectree2
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC() && !ch->IsPet())
					ch->Dead();
			}
		}
	};

	int32_t _kill_all_in_map (lua_State * L)
	{
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(lua_tonumber(L,1));

		if (NULL != pSecMap)
		{
			FKillSectree2 f;
			pSecMap->for_each(f);
		}

		return 0;
	}

	//Note: Use only on maps that do not regenerate mobs
	int32_t _regen_in_map(lua_State * L)
	{
		int32_t iMapIndex = static_cast<int32_t>(lua_tonumber(L, 1));
		std::string szFilename(lua_tostring(L, 2));

		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(iMapIndex);

		if (pMap != nullptr)
		{
			regen_load_in_file(szFilename.c_str(), iMapIndex, pMap->m_setting.iBaseX , pMap->m_setting.iBaseY);
		}

		return 0;
	}

	int32_t _enable_over9refine(lua_State* L)
	{
		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			uint32_t dwVnumFrom = (uint32_t)lua_tonumber(L, 1);
			uint32_t dwVnumTo = (uint32_t)lua_tonumber(L, 2);

			COver9RefineManager::GetInstance()->enableOver9Refine(dwVnumFrom, dwVnumTo);
		}

		return 0;
	}

	int32_t _add_ox_quiz(lua_State* L)
	{
		int32_t level = (int32_t)lua_tonumber(L, 1);
		const char* quiz = lua_tostring(L, 2);
		bool answer = lua_toboolean(L, 3);

		if (COXEventManager::GetInstance()->AddQuiz(level, quiz, answer) == false)
		{
			PyLog("OXEVENT : Cannot add quiz. {} {} {}", level, quiz, answer);
		}

		return 1;
	}

	EVENTFUNC(warp_all_to_map_my_empire_event)
	{
		warp_all_to_map_my_empire_event_info * info = dynamic_cast<warp_all_to_map_my_empire_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("warp_all_to_map_my_empire_event> <Factor> Null pointer");
			return 0;
		}

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(info->m_lMapIndexFrom);

		if (pSecMap)
		{
			FWarpEmpire f;

			f.m_lMapIndexTo = info->m_lMapIndexTo;
			f.m_x			= info->m_x;
			f.m_y			= info->m_y;
			f.m_bEmpire		= info->m_bEmpire;

			pSecMap->for_each(f);
		}

		return 0;
	}

	int32_t _block_chat(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (pChar != nullptr)
		{
			if (lua_isstring(L, 1) != true && lua_isstring(L, 2) != true)
			{
				lua_pushboolean(L, false);
				return 1;
			}

			std::string strName(lua_tostring(L, 1));
			std::string strTime(lua_tostring(L, 2));

			std::string strArg = strName + " " + strTime;

			do_block_chat(pChar, const_cast<char*>(strArg.c_str()), 0, 0);

			lua_pushboolean(L, true);
			return 1;
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int32_t _spawn_mob(lua_State* L)
	{
		if(!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isboolean(L, 3))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		const uint32_t dwVnum = static_cast<uint32_t>(lua_tonumber(L, 1));
		const size_t count = MINMAX(1, static_cast<size_t>(lua_tonumber(L, 2)), 10);
		const bool isAggresive = static_cast<bool>(lua_toboolean(L, 3));
		size_t SpawnCount = 0;

		const CMob* pMonster = CMobManager::GetInstance()->Get(dwVnum);

		if(NULL != pMonster)
		{
			const LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

			for(size_t i=0 ; i < count ; ++i)
			{
				const LPCHARACTER pSpawnMonster = CHARACTER_MANAGER::GetInstance()->SpawnMobRange(dwVnum,
						pChar->GetMapIndex(),
						pChar->GetX() - number(200, 750),
						pChar->GetY() - number(200, 750),
						pChar->GetX() + number(200, 750),
						pChar->GetY() + number(200, 750),
						true,
						pMonster->m_table.bType == CHAR_TYPE_STONE,
						isAggresive);

				if(NULL != pSpawnMonster)
				{
					++SpawnCount;
				}
			}

			PyLog("QUEST Spawn Monstster: VNUM({}) COUNT({}) isAggresive(%b)", dwVnum, SpawnCount, isAggresive);
		}

		lua_pushnumber(L, SpawnCount);

		return 1;
	}

	int32_t _notice_in_map(lua_State* L)
	{
		const LPCHARACTER pChar = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			SendNoticeMap(lua_tostring(L,1), pChar->GetMapIndex(), lua_toboolean(L,2));
		}

		return 0;
	}

	int32_t _get_locale_base_path(lua_State* L)
	{
		lua_pushstring(L, LocaleService_GetBasePath().c_str());

		return 1;
	}

	struct FPurgeArea
	{
		int32_t x1, y1, x2, y2;
		LPCHARACTER ExceptChar;

		FPurgeArea(int32_t a, int32_t b, int32_t c, int32_t d, LPCHARACTER p)
			: x1(a), y1(b), x2(c), y2(d),
			ExceptChar(p)
		{}

		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

				if (pChar == ExceptChar)
					return;
					
				if (!pChar->IsPet() && (pChar->IsMonster() || pChar->IsStone()))
				{
					if (x1 <= pChar->GetX() && pChar->GetX() <= x2 && y1 <= pChar->GetY() && pChar->GetY() <= y2)
					{
						M2_DESTROY_CHARACTER(pChar);
					}
				}
			}
		}
	};

	int32_t _purge_area(lua_State* L)
	{
		int32_t x1 = lua_tonumber(L, 1);
		int32_t y1 = lua_tonumber(L, 2);
		int32_t x2 = lua_tonumber(L, 3);
		int32_t y2 = lua_tonumber(L, 4);

		const int32_t mapIndex = SECTREE_MANAGER::GetInstance()->GetMapIndex(x1, y1);

		if (0 == mapIndex)
		{
			SysLog("_purge_area: cannot get a map index with ({}, {})", x1, y1);
			return 0;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::GetInstance()->GetMap(mapIndex);

		if (NULL != pSectree)
		{
			FPurgeArea func(x1, y1, x2, y2, CQuestManager::GetInstance()->GetCurrentNPCCharacterPtr());

			pSectree->for_each(func);
		}

		return 0;
	}

	struct FWarpAllInAreaToArea
	{
		int32_t from_x1, from_y1, from_x2, from_y2;
		int32_t to_x1, to_y1, to_x2, to_y2;
		size_t warpCount;

		FWarpAllInAreaToArea(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int32_t g, int32_t h)
			: from_x1(a), from_y1(b), from_x2(c), from_y2(d),
			to_x1(e), to_y1(f), to_x2(g), to_y2(h),
			warpCount(0)
		{}

		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

				if (pChar->IsPC())
				{
					if (from_x1 <= pChar->GetX() && pChar->GetX() <= from_x2 && from_y1 <= pChar->GetY() && pChar->GetY() <= from_y2)
					{
						++warpCount;

						pChar->WarpSet(number(to_x1, to_x2), number(to_y1, to_y2));
					}
				}
			}
		}
	};

	int32_t _warp_all_in_area_to_area(lua_State* L)
	{
		int32_t from_x1 = lua_tonumber(L, 1);
		int32_t from_y1 = lua_tonumber(L, 2);
		int32_t from_x2 = lua_tonumber(L, 3);
		int32_t from_y2 = lua_tonumber(L, 4);

		int32_t to_x1 = lua_tonumber(L, 5);
		int32_t to_y1 = lua_tonumber(L, 6);
		int32_t to_x2 = lua_tonumber(L, 7);
		int32_t to_y2 = lua_tonumber(L, 8);

		const int32_t mapIndex = SECTREE_MANAGER::GetInstance()->GetMapIndex(from_x1, from_y1);

		if (0 == mapIndex)
		{
			SysLog("_warp_all_in_area_to_area: cannot get a map index with ({}, {})", from_x1, from_y1);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::GetInstance()->GetMap(mapIndex);

		if (NULL != pSectree)
		{
			FWarpAllInAreaToArea func(from_x1, from_y1, from_x2, from_y2, to_x1, to_y1, to_x2, to_y2);

			pSectree->for_each(func);

			lua_pushnumber(L, func.warpCount);
			PyLog("_warp_all_in_area_to_area: {} character warp", func.warpCount);
			return 1;
		}
		else
		{
			lua_pushnumber(L, 0);
			SysLog("_warp_all_in_area_to_area: no sectree");
			return 1;
		}
	}

	int32_t _get_special_item_group(lua_State* L)
	{
		if (!lua_isnumber (L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::GetInstance()->GetSpecialItemGroup((uint32_t)lua_tonumber(L, 1));

		if (!pItemGroup)
		{
			SysLog("cannot find special item group {}", (uint32_t)lua_tonumber(L, 1));
			return 0;
		}

		int32_t count = pItemGroup->GetGroupSize();
		
		for (int32_t i = 0; i < count; i++)
		{
			lua_pushnumber(L, (int32_t)pItemGroup->GetVnum(i));
			lua_pushnumber(L, (int32_t)pItemGroup->GetCount(i));
		}

		return count*2;
	}

	void RegisterGlobalFunctionTable(lua_State* L)
	{
		extern int32_t quest_setstate(lua_State* L);

		luaL_reg global_functions[] =
		{
			{	"sys_err",					_syserr					},
			{	"sys_log",					_syslog					},
			{	"char_log",					_char_log				},
			{	"item_log",					_item_log				},
			{	"set_state",				quest_setstate			},
			{	"set_skin",					_set_skin				},
			{	"setskin",					_set_skin				},
			{	"time_to_str",				_time_to_str			},
			{	"say",						_say					},
			{	"chat",						_chat					},
			{	"cmdchat",					_cmdchat				},
			{	"syschat",					_syschat				},
			{	"get_locale",				_get_locale				},
			{	"setleftimage",				_left_image				},
			{	"settopimage",				_top_image				},
			{	"server_timer",				_set_server_timer		},
			{	"clear_server_timer",		_clear_server_timer		},
			{	"server_loop_timer",		_set_server_loop_timer	},
			{	"get_server_timer_arg",		_get_server_timer_arg	},
			{	"timer",					_timer					},
			{	"loop_timer",				_set_named_loop_timer	},
			{	"cleartimer",				_clear_named_timer		},
			{	"getnpcid",					_getnpcid				},
			{	"is_test_server",			_is_test_server			},
			{	"is_speed_server",			_is_speed_server		},
			{	"raw_script",				_raw_script				},
			{	"number",					_number	   				},

			// LUA_ADD_BGM_INFO
			{	"set_bgm_volume_enable",	_set_bgm_volume_enable	},
			{	"add_bgm_info",				_add_bgm_info			},
			// END_OF_LUA_ADD_BGM_INFO

			// LUA_ADD_GOTO_INFO
			{	"add_goto_info",			_add_goto_info			},
			// END_OF_LUA_ADD_GOTO_INFO

			// REFINE_PICK
			{	"__refine_pick",			_refine_pick			},
			// END_OF_REFINE_PICK

			{	"add_ox_quiz",					_add_ox_quiz					},
			{	"__fish_real_refine_rod",		_fish_real_refine_rod			}, // XXX
			{	"__give_char_priv",				_give_char_privilege			},
			{	"__give_empire_priv",			_give_empire_privilege			},
			{	"__give_guild_priv",			_give_guild_privilege			},
			{	"__get_empire_priv_string",		_get_empire_privilege_string	},
			{	"__get_empire_priv",			_get_empire_privilege			},
			{	"__get_guild_priv_string",		_get_guild_privilege_string		},
			{	"__get_guildid_byname",			_get_guildid_byname				},
			{	"__get_guild_priv",				_get_guild_privilege			},
			{	"item_name",					_item_name						},
			{	"mob_name",						_mob_name						},
			{	"mob_vnum",						_mob_vnum						},
			{	"get_time",						_get_global_time				},
			{	"get_global_time",				_get_global_time				},
			{	"get_channel_id",				_get_channel_id					},
			{	"command",						_do_command						},
			{	"find_pc_cond",					_find_pc_cond					},
			{	"find_pc_by_name",				_find_pc						},
			{	"find_npc_by_vnum",				_find_npc_by_vnum				},
			{	"set_quest_state",				_set_quest_state				},
			{	"get_quest_state",				_get_quest_state				},
			{	"under_han",					_under_han						},
			{	"notice",						_notice							},
			{	"notice_all",					_notice_all						},
			{	"notice_in_map",				_notice_in_map					},
			{	"warp_all_to_village",			_warp_all_to_village			},
			{	"warp_to_village",				_warp_to_village				},	
			{	"say_in_map",					_say_in_map						},	
			{	"kill_all_in_map",				_kill_all_in_map				},
			{	"regen_in_map",					_regen_in_map					},
			{	"enable_over9refine",			_enable_over9refine				},
			{	"block_chat",					_block_chat						},
			{	"spawn_mob",					_spawn_mob						},
			{	"get_locale_base_path",			_get_locale_base_path			},
			{	"purge_area",					_purge_area						},
			{	"warp_all_in_area_to_area",		_warp_all_in_area_to_area		},
			{	"get_special_item_group",		_get_special_item_group			},

			{	NULL,	NULL	}
		};
	
		int32_t i = 0;

		while (global_functions[i].name != nullptr)
		{
			lua_register(L, global_functions[i].name, global_functions[i].func);
			++i;
		}
	}
}

