#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "char_manager.h"
#include "arena.h"

namespace quest
{
	int32_t arena_start_duel(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPCHARACTER ch2 = CHARACTER_MANAGER::GetInstance()->FindPC(lua_tostring(L,1));
		int32_t nSetPoint = (int32_t)lua_tonumber(L, 2);

		if (ch == nullptr || ch2 == nullptr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (ch->IsHorseRiding())
		{
			ch->StopRiding();
			ch->HorseSummon(false);
		}

		if (ch2->IsHorseRiding())
		{
			ch2->StopRiding();
			ch2->HorseSummon(false);
		}

		if (CArenaManager::GetInstance()->IsMember(ch->GetMapIndex(), ch->GetPlayerID()) != MEMBER_NO || 
				CArenaManager::GetInstance()->IsMember(ch2->GetMapIndex(), ch2->GetPlayerID()) != MEMBER_NO	)
		{
			lua_pushnumber(L, 2);
			return 1;
		}

		if (CArenaManager::GetInstance()->StartDuel(ch, ch2, nSetPoint) == false)
		{
			lua_pushnumber(L, 3);
			return 1;
		}

		lua_pushnumber(L, 1);

		return 1;
	}

	int32_t arena_add_map(lua_State* L)
	{
		int32_t mapIdx		= (int32_t)lua_tonumber(L, 1);
		int32_t startposAX	= (int32_t)lua_tonumber(L, 2);
		int32_t startposAY	= (int32_t)lua_tonumber(L, 3);
		int32_t startposBX	= (int32_t)lua_tonumber(L, 4);
		int32_t startposBY	= (int32_t)lua_tonumber(L, 5);

		if (CArenaManager::GetInstance()->AddArena(mapIdx, startposAX, startposAY, startposBX, startposBY) == false)
		{
			PyLog("Failed to load arena map info(map:{} AX:{} AY:{} BX:{} BY:{}", mapIdx, startposAX, startposAY, startposBX, startposBY);
		}
		else
		{
			PyLog("Add Arena Map:{} startA({},{}) startB({},{})", mapIdx, startposAX, startposAY, startposBX, startposBY);
		}

		return 1;
	}

	int32_t arena_get_duel_list(lua_State* L)
	{
		CArenaManager::GetInstance()->GetDuelList(L);

		return 1;
	}

	int32_t arena_add_observer(lua_State* L)
	{
		int32_t mapIdx = (int32_t)lua_tonumber(L, 1);
		int32_t ObPointX = (int32_t)lua_tonumber(L, 2);
		int32_t ObPointY = (int32_t)lua_tonumber(L, 3);
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CArenaManager::GetInstance()->AddObserver(ch, mapIdx, ObPointX, ObPointY);

		return 1;
	}

	int32_t arena_is_in_arena(lua_State* L)
	{
		uint32_t pid = (uint32_t)lua_tonumber(L, 1);

		LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pid);

		if (ch == nullptr)
		{
			lua_pushnumber(L, 1);
		}
		else
		{
			if (ch->GetArena() == nullptr || ch->GetArenaObserverMode())
			{
				if (CArenaManager::GetInstance()->IsMember(ch->GetMapIndex(), ch->GetPlayerID()) == MEMBER_DUELIST)
					lua_pushnumber(L, 1);
				else
					lua_pushnumber(L, 0);
			}
			else
			{
				lua_pushnumber(L, 0);
			}
		}
		return 1;
	}

	void RegisterArenaFunctionTable()
	{
		luaL_reg arena_functions[] =
		{
			{"start_duel",		arena_start_duel		},
			{"add_map",			arena_add_map			},
			{"get_duel_list",	arena_get_duel_list		},
			{"add_observer",	arena_add_observer		},
			{"is_in_arena",		arena_is_in_arena		},

			{NULL,	NULL}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("arena", arena_functions);
	}
}

