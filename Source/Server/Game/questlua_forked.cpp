#include "stdafx.h"

#include "threeway_war.h"

#include <Common/stl.h>

#include "questlua.h"
#include "questmanager.h"
#include "char.h"
#include "dungeon.h"
#include "p2p.h"
#include "locale_service.h"
#include "threeway_war.h"

extern int32_t passes_per_sec;

namespace quest
{
	//
	// "forked_" lua functions
	//

	int32_t forked_set_dead_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		auto q = CQuestManager::GetInstance();

		if (NULL != ch)
		{
			CThreeWayWar::GetInstance()->SetReviveTokenForPlayer(ch->GetPlayerID(), q->GetEventFlag("threeway_war_dead_count"));
		}

		return 0; 
	}

	int32_t forked_get_dead_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != ch)
		{
			lua_pushnumber(L, CThreeWayWar::GetInstance()->GetReviveTokenForPlayer(ch->GetPlayerID()));
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1; 
	}

	int32_t forked_init_kill_count_per_empire(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CThreeWayWar::GetInstance()->SetKillScore(ch->GetEmpire(), 0);

		return 0;
	}

	int32_t forked_init(lua_State* L)
	{
		CThreeWayWar::GetInstance()->Initialize();
		CThreeWayWar::GetInstance()->RandomEventMapSet();

		return 0;
	}

	int32_t forked_sungzi_start_pos(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		const ForkedSungziMapInfo& info = CThreeWayWar::GetInstance()->GetEventSungZiMapInfo();

		lua_pushnumber(L, info.m_iForkedSungziStartPosition[ch->GetEmpire()-1][0]);
		lua_pushnumber(L, info.m_iForkedSungziStartPosition[ch->GetEmpire()-1][1]);

		return 2;
	}

	int32_t forked_pass_start_pos(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		const ForkedPassMapInfo& info = CThreeWayWar::GetInstance()->GetEventPassMapInfo();

		lua_pushnumber(L, info.m_iForkedPassStartPosition[ch->GetEmpire()-1][0]);
		lua_pushnumber(L, info.m_iForkedPassStartPosition[ch->GetEmpire()-1][1]);

		return 2;
	}

	int32_t forked_sungzi_mapindex (lua_State *L)
	{
		lua_pushnumber(L, GetSungziMapIndex()); 

		if (test_server)
			PyLog("forked_sungzi_map_index_by_empire {}", GetSungziMapIndex());
		return 1;
	}

	int32_t forked_pass_mapindex_by_empire(lua_State *L)
	{
		lua_pushnumber(L, GetPassMapIndex(lua_tonumber(L,1))); 

		return 1;
	}

	int32_t forked_get_pass_path_my_empire(lua_State *L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		lua_pushstring(L, GetPassMapPath(ch->GetEmpire()));

		PyLog("[PASS_PATH] Empire {} Path  {}", ch->GetEmpire(), GetPassMapPath(ch->GetEmpire()));
		return 1;
	}

	int32_t forked_get_pass_path_by_empire(lua_State *L)
	{
		int32_t iEmpire 	= (int32_t)lua_tonumber(L, 1);

		lua_pushstring(L, GetPassMapPath(iEmpire));
		PyLog("[PASS_PATH] Empire {} Path  {}", iEmpire, GetPassMapPath(iEmpire));
		return 1;
	}

	int32_t forked_is_forked_mapindex(lua_State * L)
	{
		lua_pushboolean(L, CThreeWayWar::GetInstance()->IsThreeWayWarMapIndex(lua_tonumber(L,1)));

		return 1;
	}

	int32_t forked_is_sungzi_mapindex(lua_State * L)
	{
		lua_pushboolean(L, CThreeWayWar::GetInstance()->IsSungZiMapIndex(lua_tonumber(L,1)));

		return 1;
	}

	struct FWarpInMap
	{
		int32_t m_iMapIndexTo;
		int32_t m_x;
		int32_t m_y;

		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
				if (ch->IsPC())
				{
					ch->WarpSet(m_x+(number(0,5)*100), m_y+(number(0,5)*100), m_iMapIndexTo);
				}
			}
		}
	};

	EVENTINFO(warp_all_to_map_event_info)
	{
		int32_t		m_iMapIndexFrom;
		int32_t 	m_iMapIndexTo;
		int32_t 	m_x;
		int32_t		m_y;

		warp_all_to_map_event_info()
		: m_iMapIndexFrom(0)
		, m_iMapIndexTo(0)
		, m_x(0)
		, m_y(0)
		{
		}
	};

	EVENTFUNC(warp_all_to_map_event)
	{
		warp_all_to_map_event_info * info = dynamic_cast<warp_all_to_map_event_info *>(event->info);

		if (info == nullptr)
		{
			SysLog("warp_all_to_map_event> <Factor> Null pointer");
			return 0;
		}

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::GetInstance()->GetMap(info->m_iMapIndexFrom);
		if (pSecMap)
		{
			FWarpInMap f;
			f.m_iMapIndexTo = info->m_iMapIndexTo;
			f.m_x			= info->m_x;
			f.m_y			= info->m_y;
			pSecMap->for_each(f);
		}

		return 0;
	}

	int32_t forked_warp_all_in_map(lua_State * L)
	{
		int32_t iMapIndexFrom	= (int32_t)lua_tonumber(L, 1);
		int32_t iMapIndexTo		= (int32_t)lua_tonumber(L, 2);
		int32_t ix				= (int32_t)lua_tonumber(L, 3);
		int32_t iy				= (int32_t)lua_tonumber(L, 4);
		int32_t iTime			= (int32_t)lua_tonumber(L, 5);

		warp_all_to_map_event_info* info = AllocEventInfo<warp_all_to_map_event_info>();

		info->m_iMapIndexFrom	= iMapIndexFrom;
		info->m_iMapIndexTo		= iMapIndexTo;
		info->m_x				= ix;
		info->m_y				= iy;

		event_create(warp_all_to_map_event, info, PASSES_PER_SEC(iTime));

		return 0;
	}	

	int32_t forked_is_registered_user(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != ch)
		{
			lua_pushboolean(L, CThreeWayWar::GetInstance()->IsRegisteredUser(ch->GetPlayerID()));
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int32_t forked_register_user(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (NULL != ch)
		{
			CThreeWayWar::GetInstance()->RegisterUser(ch->GetPlayerID());
		}

		return 0;
	}

	int32_t forked_purge_all_monsters(lua_State* L)
	{
		CThreeWayWar::GetInstance()->RemoveAllMonstersInThreeWay();

		return 0;
	}

	void RegisterForkedFunctionTable() 
	{
		luaL_reg forked_functions[] = 
		{
			{ "setdeadcount",			forked_set_dead_count				},
			{ "getdeadcount",			forked_get_dead_count				},
			{ "initkillcount",			forked_init_kill_count_per_empire	},
			{ "initforked",				forked_init							},
			{ "get_sungzi_start_pos",	forked_sungzi_start_pos				},
			{ "get_pass_start_pos",		forked_pass_start_pos				},
			{ "getsungzimapindex",		forked_sungzi_mapindex				},
			{ "getpassmapindexbyempire", forked_pass_mapindex_by_empire		},
			{ "getpasspathbyempire",	forked_get_pass_path_by_empire		},
			{ "isforkedmapindex",		forked_is_forked_mapindex			},
			{ "issungzimapindex",		forked_is_sungzi_mapindex			},
			{ "warp_all_in_map",		forked_warp_all_in_map				},
			{ "is_registered_user",		forked_is_registered_user			},
			{ "register_user",			forked_register_user				},
			{ "purge_all_monsters",		forked_purge_all_monsters			},

			{ NULL,				NULL			}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("forked", forked_functions);
	}
}

