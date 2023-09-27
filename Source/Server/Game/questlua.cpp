
#include "stdafx.h"

#include <sstream>
#include <filesystem>
#include "questmanager.h"
#include "questlua.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "buffer_manager.h"
#include "db.h"
#include "xmas_event.h"
#include "locale_service.h"
#include "regen.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "sectree_manager.h"

namespace quest
{
	std::string ScriptToString(const std::string& str)
	{
		lua_State* L = CQuestManager::GetInstance()->GetLuaState();
		int32_t x = lua_gettop(L);

		int32_t errcode = lua_dobuffer(L, ("return "+str).c_str(), str.size()+7, "ScriptToString");
		std::string retstr;
		if (!errcode)
		{
			if (lua_isstring(L,-1))
				retstr = lua_tostring(L, -1);
		}
		else
		{
			SysLog("LUA ScriptRunError (code:{} src:[{}])", errcode, str.c_str());
		}
		lua_settop(L,x);
		return retstr;
	}

	void FSetWarpLocation::operator() (LPCHARACTER ch)
	{
		if (ch->IsPC())
		{
			ch->SetWarpLocation (map_index, x, y);
		}
	}

	void FSetQuestFlag::operator() (LPCHARACTER ch)
	{
		if (!ch->IsPC())
			return;

		PC* pPC = CQuestManager::GetInstance()->GetPCForce(ch->GetPlayerID());

		if (pPC)
			pPC->SetFlag(flagname, value);
	}

	bool FPartyCheckFlagLt::operator() (LPCHARACTER ch)
	{
		if (!ch->IsPC())
			return false;

		PC* pPC = CQuestManager::GetInstance()->GetPCForce(ch->GetPlayerID());
		bool returnBool;
		if (pPC)
		{
			int32_t flagValue = pPC->GetFlag(flagname);
			if (value > flagValue)
				returnBool = true;
			else
				returnBool = false;
		}

		return returnBool;
	}

	FPartyChat::FPartyChat(int32_t ChatType, const char* str) : iChatType(ChatType), str(str)
	{
	}

	void FPartyChat::operator() (LPCHARACTER ch)
	{
		ch->ChatPacket(iChatType, "%s", str);
	}

	void FPartyClearReady::operator() (LPCHARACTER ch)
	{
		ch->RemoveAffect(AFFECT_DUNGEON_READY);
	}

	void FSendPacket::operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->GetDesc())
			{
				ch->GetDesc()->Packet(buf.read_peek(), buf.size());
			}
		}
	}

	void FSendPacketToEmpire::operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->GetDesc())
			{
				if (ch->GetEmpire() == bEmpire)
					ch->GetDesc()->Packet(buf.read_peek(), buf.size());
			}
		}
	}

	void FWarpEmpire::operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC() && ch->GetEmpire() == m_bEmpire)
			{
				ch->WarpSet(m_x, m_y, m_lMapIndexTo);
			}
		}
	}

	FBuildLuaGuildWarList::FBuildLuaGuildWarList(lua_State * lua_state) : L(lua_state), m_count(1)
	{
		lua_newtable(lua_state);
	}

	void FBuildLuaGuildWarList::operator() (uint32_t g1, uint32_t g2)
	{
		CGuild* g = CGuildManager::GetInstance()->FindGuild(g1);

		if (!g)
			return;

		if (g->GetGuildWarType(g2) == GUILD_WAR_TYPE_FIELD)
			return;

		if (g->GetGuildWarState(g2) != GUILD_WAR_ON_WAR)
			return;

		lua_newtable(L);
		lua_pushnumber(L, g1);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, g2);
		lua_rawseti(L, -2, 2);
		lua_rawseti(L, -2, m_count++);
	}

	bool IsScriptTrue(const char* code, int32_t size)
	{
		if (size==0)
			return true;

		lua_State* L = CQuestManager::GetInstance()->GetLuaState();
		int32_t x = lua_gettop(L);
		int32_t errcode = lua_dobuffer(L, code, size, "IsScriptTrue");
		int32_t bStart = lua_toboolean(L, -1);
		if (errcode)
		{
			char buf[100];
			snprintf(buf, sizeof(buf), "LUA ScriptRunError (code:%%d src:[%%%ds])", size);
			SysLog("{}", buf);
		}
		lua_settop(L,x);
		return bStart != 0;
	}

	void combine_lua_string(lua_State * L, std::ostringstream & s)
	{
		char buf[32];

		int32_t n = lua_gettop(L);
		int32_t i;

		for (i = 1; i <= n; ++i)
		{
			if (lua_isstring(L,i))
				s << lua_tostring(L, i);
			else if (lua_isnumber(L, i))
			{
				snprintf(buf, sizeof(buf), "%.14g\n", lua_tonumber(L,i));
				s << buf;
			}
		}
	}

	int32_t highscore_show(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		const char* pszBoardName = lua_tostring(L, 1);
		uint32_t mypid = q->GetCurrentCharacterPtr()->GetPlayerID();
		bool bOrder = (int32_t) lua_tonumber(L, 2) != 0 ? true : false;

		DBManager::GetInstance()->ReturnQuery(QID_HIGHSCORE_SHOW, mypid, NULL, 
				"SELECT h.pid, p.name, h.value FROM highscore%s as h, player%s as p WHERE h.board = '%s' AND h.pid = p.id ORDER BY h.value %s LIMIT 10",
				get_table_postfix(), get_table_postfix(), pszBoardName, bOrder ? "DESC" : "");
		return 0;
	}

	int32_t highscore_register(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		THighscoreRegisterQueryInfo * qi = M2_NEW THighscoreRegisterQueryInfo;

		strlcpy(qi->szBoard, lua_tostring(L, 1), sizeof(qi->szBoard));
		qi->dwPID = q->GetCurrentCharacterPtr()->GetPlayerID();
		qi->iValue = (int32_t) lua_tonumber(L, 2);
		qi->bOrder = (int32_t) lua_tonumber(L, 3);

		DBManager::GetInstance()->ReturnQuery(QID_HIGHSCORE_REGISTER, qi->dwPID, qi, 
				"SELECT value FROM highscore%s WHERE board='%s' AND pid=%u", get_table_postfix(), qi->szBoard, qi->dwPID);
		return 1;
	}

	// 
	// "member" Lua functions
	//
	int32_t member_chat(lua_State* L)
	{
		std::ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::GetInstance()->GetCurrentPartyMember()->ChatPacket(CHAT_TYPE_TALKING, "%s", s.str().c_str());
		return 0;
	}

	int32_t member_clear_ready(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentPartyMember();
		ch->RemoveAffect(AFFECT_DUNGEON_READY);
		return 0;
	}

	int32_t member_set_ready(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentPartyMember();
		ch->AddAffect(AFFECT_DUNGEON_READY, POINT_NONE, 0, AFF_DUNGEON_READY, 65535, 0, true);
		return 0;
	}

	int32_t mob_spawn(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			SysLog("invalid argument");
			return 0;
		}

		uint32_t mob_vnum = (uint32_t)lua_tonumber(L, 1);
		int32_t local_x = (int32_t) lua_tonumber(L, 2)*100;
		int32_t local_y = (int32_t) lua_tonumber(L, 3)*100;
		float radius = (float) lua_tonumber(L, 4)*100;
		bool bAggressive = lua_toboolean(L, 5);
		uint32_t count = (lua_isnumber(L, 6))?(uint32_t) lua_tonumber(L, 6):1;

		if (count == 0)
			count = 1;
		else if (count > 10)
		{
			SysLog("count bigger than 10");
			count = 10;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(ch->GetMapIndex());
		if (pMap == nullptr) {
			return 0;
		}
		uint32_t dwQuestIdx = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		bool ret = false;
		LPCHARACTER mob = nullptr;

		while (count--)
		{
			for (int32_t loop = 0; loop < 8; ++loop)
			{
				float angle = number(0, 999) * M_PI * 2 / 1000;
				float r = number(0, 999) * radius / 1000;

				int32_t x = local_x + pMap->m_setting.iBaseX + (int32_t)(r * cos(angle));
				int32_t y = local_y + pMap->m_setting.iBaseY + (int32_t)(r * sin(angle));

				mob = CHARACTER_MANAGER::GetInstance()->SpawnMob(mob_vnum, ch->GetMapIndex(), x, y, 0);

				if (mob)
					break;
			}

			if (mob)
			{
				if (bAggressive)
					mob->SetAggressive();

				mob->SetQuestBy(dwQuestIdx);

				if (!ret)
				{
					ret = true;
					lua_pushnumber(L, (uint32_t) mob->GetVID());
				}
			}
		}

		if (!ret)
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t mob_spawn_group(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 6))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		uint32_t group_vnum = (uint32_t)lua_tonumber(L, 1);
		int32_t local_x = (int32_t) lua_tonumber(L, 2) * 100;
		int32_t local_y = (int32_t) lua_tonumber(L, 3) * 100;
		float radius = (float) lua_tonumber(L, 4) * 100;
		bool bAggressive = lua_toboolean(L, 5);
		uint32_t count = (uint32_t) lua_tonumber(L, 6);

		if (count == 0)
			count = 1;
		else if (count > 10)
		{
			SysLog("count bigger than 10");
			count = 10;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(ch->GetMapIndex());
		if (pMap == nullptr) {
			lua_pushnumber(L, 0);
			return 1;
		}
		uint32_t dwQuestIdx = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		bool ret = false;
		LPCHARACTER mob = nullptr;

		while (count--)
		{
			for (int32_t loop = 0; loop < 8; ++loop)
			{
				float angle = number(0, 999) * M_PI * 2 / 1000;
				float r = number(0, 999)*radius/1000;

				int32_t x = local_x + pMap->m_setting.iBaseX + (int32_t)(r * cos(angle));
				int32_t y = local_y + pMap->m_setting.iBaseY + (int32_t)(r * sin(angle));

				mob = CHARACTER_MANAGER::GetInstance()->SpawnGroup(group_vnum, ch->GetMapIndex(), x, y, x, y, NULL, bAggressive);

				if (mob)
					break;
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

		if (!ret)
			lua_pushnumber(L, 0);

		return 1;
	}

	//
	// global Lua functions
	//
	//
	// Registers Lua function table
	//
	void CQuestManager::AddLuaFunctionTable(const char* c_pszName, luaL_reg* preg)
	{
		lua_newtable(L);

		while ((preg->name))
		{
			lua_pushstring(L, preg->name);
			lua_pushcfunction(L, preg->func);
			lua_rawset(L, -3);
			preg++;
		}

		lua_setglobal(L, c_pszName);
	}

	void CQuestManager::BuildStateIndexToName(const char* questName)
	{
		int32_t x = lua_gettop(L);
		lua_getglobal(L, questName);

		if (lua_isnil(L,-1))
		{
			SysLog("QUEST wrong quest state file for quest {}",questName);
			lua_settop(L,x);
			return;
		}

		for (lua_pushnil(L); lua_next(L, -2);)
		{
			if (lua_isstring(L, -2) && lua_isnumber(L, -1))
			{
				lua_pushvalue(L, -2);
				lua_rawset(L, -4);
			}
			else
			{
				lua_pop(L, 1);
			}
		}

		lua_settop(L, x);
	}

	/**
	 * @version 05/06/08	Bang2ni - __get_guildid_byname 스크립트 함수 등록
	 */
	bool CQuestManager::InitializeLua()
	{
		L = lua_open();

		luaopen_base(L);
		luaopen_table(L);
		luaopen_string(L);
		luaopen_math(L);
		//TEMP
		luaopen_io(L);
		luaopen_debug(L);

		RegisterAffectFunctionTable();
		RegisterBuildingFunctionTable();
		RegisterDungeonFunctionTable();
		RegisterGameFunctionTable();
		RegisterGuildFunctionTable();
		RegisterHorseFunctionTable();
#ifdef __PET_SYSTEM__
		RegisterPetFunctionTable();
#endif
		RegisterITEMFunctionTable();
		RegisterMarriageFunctionTable();
		RegisterNPCFunctionTable();
		RegisterPartyFunctionTable();
		RegisterPCFunctionTable();
		RegisterQuestFunctionTable();
		RegisterTargetFunctionTable();
		RegisterArenaFunctionTable();
		RegisterForkedFunctionTable();
		RegisterMonarchFunctionTable(); 
		RegisterOXEventFunctionTable();
		RegisterMgmtFunctionTable();
		RegisterBattleArenaFunctionTable();
		RegisterDanceEventFunctionTable();
		RegisterDragonLairFunctionTable();
		RegisterDragonSoulFunctionTable();

		{
			luaL_reg member_functions[] = 
			{
				{ "chat",			member_chat		},
				{ "set_ready",			member_set_ready	},
				{ "clear_ready",		member_clear_ready	},
				{ NULL,				NULL			}
			};

			AddLuaFunctionTable("member", member_functions);
		}

		{
			luaL_reg highscore_functions[] = 
			{
				{ "register",			highscore_register	},
				{ "show",			highscore_show		},
				{ NULL,				NULL			}
			};

			AddLuaFunctionTable("highscore", highscore_functions);
		}

		{
			luaL_reg mob_functions[] =
			{
				{ "spawn",			mob_spawn		},
				{ "spawn_group",		mob_spawn_group		},
				{ NULL,				NULL			}
			};

			AddLuaFunctionTable("mob", mob_functions);
		}

		//
		// global namespace functions
		//
		RegisterGlobalFunctionTable(L);

		// LUA_INIT_ERROR_MESSAGE
		{
			char settingsFileName[256];
			snprintf(settingsFileName, sizeof(settingsFileName), "%s/settings.lua", LocaleService_GetBasePath().c_str());

			int32_t settingsLoadingResult = lua_dofile(L, settingsFileName);
			PyLog("LoadSettings({}), returns {}", settingsFileName, settingsLoadingResult);
			if (settingsLoadingResult != 0)
			{
				SysLog("LOAD_SETTINS_FAILURE({})", settingsFileName);
				return false;
			}
		}

		{
			char questlibFileName[256];
			snprintf(questlibFileName, sizeof(questlibFileName), "%s/questlib.lua", LocaleService_GetQuestPath().c_str());

			int32_t questlibLoadingResult = lua_dofile(L, questlibFileName);
			PyLog("LoadQuestlib({}), returns {}", questlibFileName, questlibLoadingResult);
			if (questlibLoadingResult != 0)
			{
				SysLog("LOAD_QUESTLIB_FAILURE({})", questlibFileName);
				return false;
			}
		}

		{
			char translateFileName[256];
			snprintf(translateFileName, sizeof(translateFileName), "%s/translate.lua", LocaleService_GetBasePath().c_str());

			int32_t translateLoadingResult = lua_dofile(L, translateFileName);
			PyLog("LoadTranslate({}), returns {}", translateFileName, translateLoadingResult);
			if (translateLoadingResult != 0)
			{
				SysLog("LOAD_TRANSLATE_ERROR({})", translateFileName);
				return false;
			}
		}

		{
			char questLocaleFileName[256];
			snprintf(questLocaleFileName, sizeof(questLocaleFileName), "%s/locale.lua", g_stQuestDir.c_str());

			int32_t questLocaleLoadingResult = lua_dofile(L, questLocaleFileName);
			PyLog("LoadQuestLocale({}), returns {}", questLocaleFileName, questLocaleLoadingResult);
			if (questLocaleLoadingResult != 0)
			{
				SysLog("LoadQuestLocale({}) FAILURE", questLocaleFileName);
				return false;
			}
		}
		// END_OF_LUA_INIT_ERROR_MESSAGE

		for (const auto& it : g_setQuestObjectDir)
		{
			char buf[PATH_MAX];
			snprintf(buf, sizeof(buf), "%s/state/", it.c_str());
			if (!std::filesystem::exists(buf))
				continue;

			int32_t iQuestIdx = 0;

			for (const auto& directory_entry : std::filesystem::directory_iterator(buf))
			{
				if (!std::filesystem::is_regular_file(directory_entry))
					continue;

				const auto& stQuestName = directory_entry.path().filename().generic_string();

				snprintf(buf + 11, sizeof(buf) - 11, "%s", stQuestName.c_str());
				RegisterQuest(stQuestName, ++iQuestIdx);
				int32_t ret = lua_dofile(L, (it + "/state/" + stQuestName).c_str());
				PyLog("QUEST: loading %s, returns %d", (buf + stQuestName), ret);

				BuildStateIndexToName(stQuestName.c_str());
			}
		}

		lua_setgcthreshold(L, 0);

		lua_newtable(L);
		lua_setglobal(L, "__codecache");
		return true;
	}

	void CQuestManager::GotoSelectState(QuestState& qs)
	{
		lua_checkstack(qs.co, 1);

		//int32_t n = lua_gettop(L);
		int32_t n = luaL_getn(qs.co, -1);
		qs.args = n;
		//cout << "select here (1-" << qs.args << ")" << endl;
		//

		std::ostringstream os;
		os << "[QUESTION ";

		for (int32_t i=1; i<=n; i++)
		{
			lua_rawgeti(qs.co,-1,i);
			if (lua_isstring(qs.co,-1))
			{
				//printf("%d\t%s\n",i,lua_tostring(qs.co,-1));
				if (i != 1)
					os << "|";
				os << i << ";" << lua_tostring(qs.co,-1);
			}
			else
			{
				SysLog("SELECT wrong data {}", lua_typename(qs.co, -1));
				SysLog("here");
			}
			lua_pop(qs.co,1);
		}
		os << "]";


		AddScript(os.str());
		qs.suspend_state = SUSPEND_STATE_SELECT;
		if (test_server)
			PyLog("{}", m_strScript.c_str());
		SendScript();
	}

	EVENTINFO(confirm_timeout_event_info)
	{
		uint32_t dwWaitPID;
		uint32_t dwReplyPID;

		confirm_timeout_event_info()
		: dwWaitPID(0)
		, dwReplyPID(0)
		{
		}
	};

	EVENTFUNC(confirm_timeout_event)
	{
		confirm_timeout_event_info * info = dynamic_cast<confirm_timeout_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("confirm_timeout_event> <Factor> Null pointer");
			return 0;
		}

		LPCHARACTER chWait = CHARACTER_MANAGER::GetInstance()->FindByPID(info->dwWaitPID);
		LPCHARACTER chReply = nullptr; //CHARACTER_MANAGER::info().FindByPID(info->dwReplyPID);

		if (chReply)
		{
			// Automatically closes after time passes
		}

		if (chWait)
		{
			CQuestManager::GetInstance()->Confirm(info->dwWaitPID, CONFIRM_TIMEOUT);
		}

		return 0;
	}

	void CQuestManager::GotoConfirmState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_CONFIRM;
		uint32_t dwVID = (uint32_t) lua_tonumber(qs.co, -3);
		const char* szMsg = lua_tostring(qs.co, -2);
		int32_t iTimeout = (int32_t) lua_tonumber(qs.co, -1);

		PyLog("GotoConfirmState vid {} msg '{}', timeout {}", dwVID, szMsg, iTimeout);

		// 1. Display a confirmation window to the other party
		// 2. Pops up a window telling me to wait for confirmation
		// 3. Timeout setting (If timeout occurs, close the other party's window and send me to close the window as well)

		// One
		// If there is no other party, just don't send it to the other party. passed due to timeout
		LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(dwVID);
		if (ch && ch->IsPC())
		{
			ch->ConfirmWithMsg(szMsg, iTimeout, GetCurrentCharacterPtr()->GetPlayerID());
		}

		// 2
		GetCurrentPC()->SetConfirmWait((ch && ch->IsPC())?ch->GetPlayerID():0);
		std::ostringstream os;
		os << "[CONFIRM_WAIT timeout;" << iTimeout << "]";
		AddScript(os.str());
		SendScript();

		// 3
		confirm_timeout_event_info* info = AllocEventInfo<confirm_timeout_event_info>();

		info->dwWaitPID = GetCurrentCharacterPtr()->GetPlayerID();
		info->dwReplyPID = (ch && ch->IsPC()) ? ch->GetPlayerID() : 0;

		event_create(confirm_timeout_event, info, PASSES_PER_SEC(iTimeout));
	}

	void CQuestManager::GotoSelectItemState(QuestState& qs)
	{
		qs.suspend_state = SUSPEND_STATE_SELECT_ITEM;
		AddScript("[SELECT_ITEM]");
		SendScript();
	}

	void CQuestManager::GotoInputState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_INPUT;
		AddScript("[INPUT]");
		SendScript();

		// check the time limit
		//event_create(input_timeout_event, dwEI, PASSES_PER_SEC(iTimeout));
	}

	void CQuestManager::GotoPauseState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_PAUSE;
		AddScript("[NEXT]");
		SendScript();
	}

	void CQuestManager::GotoEndState(QuestState & qs)
	{
		AddScript("[DONE]");
		SendScript();
	}

	//
	// * OpenState
	//
	// The beginning of script
	// 

	QuestState CQuestManager::OpenState(const std::string& quest_name, int32_t state_index)
	{
		QuestState qs;
		qs.args=0;
		qs.st = state_index;
		qs.co = lua_newthread(L);
		qs.ico = lua_ref(L, 1/*qs.co*/);
		return qs;
	}

	//
	// * RunState
	// 
	// decides script to wait for user input, or finish
	// 
	bool CQuestManager::RunState(QuestState & qs)
	{
		ClearError();

		m_CurrentRunningState = &qs;
		int32_t ret = lua_resume(qs.co, qs.args);

		if (ret == 0)
		{
			if (lua_gettop(qs.co) == 0)
			{
				// end of quest
				GotoEndState(qs);
				return false;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "select"))
			{
				GotoSelectState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "wait"))
			{
				GotoPauseState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "input"))
			{
				GotoInputState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "confirm"))
			{
				GotoConfirmState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "select_item"))
			{
				GotoSelectItemState(qs);
				return true;
			}
		}
		else
		{
			SysLog("LUA_ERROR: {}", lua_tostring(qs.co, 1));
		}

		WriteRunningStateToSyserr();
		SetError();

		GotoEndState(qs);
		return false;
	}

	//
	// * CloseState
	//
	// makes script end
	//
	void CQuestManager::CloseState(QuestState& qs)
	{
		if (qs.co)
		{
			//cerr << "ICO "<<qs.ico <<endl;
			lua_unref(L, qs.ico);
			qs.co = 0;
		}
	}
}
