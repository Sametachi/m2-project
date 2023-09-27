#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "sectree_manager.h"
#include "target.h"

namespace quest 
{
	//
	// "target" Lua functions
	//
	int32_t target_pos(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t iQuestIndex = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument, name: {}, quest_index {}", ch->GetName(), iQuestIndex);
			return 0;
		}

		PIXEL_POSITION pos;

		if (!SECTREE_MANAGER::GetInstance()->GetMapBasePositionByMapIndex(ch->GetMapIndex(), pos))
		{
			SysLog("cannot find base position in this map {}", ch->GetMapIndex());
			return 0;
		}

		int32_t x = pos.x + (int32_t) lua_tonumber(L, 2) * 100;
		int32_t y = pos.y + (int32_t) lua_tonumber(L, 3) * 100;

		CTargetManager::GetInstance()->CreateTarget(ch->GetPlayerID(),
				iQuestIndex,
				lua_tostring(L, 1),
				TARGET_TYPE_POS,
				x,
				y,
				(int32_t) lua_tonumber(L, 4),
				lua_isstring(L, 5) ? lua_tostring(L, 5) : NULL,
				lua_isnumber(L, 6) ? (int32_t)lua_tonumber(L, 6): 1);

		return 0;
	}

	int32_t target_vid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t iQuestIndex = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
		{
			SysLog("invalid argument, name: {}, quest_index {}", ch->GetName(), iQuestIndex);
			return 0;
		}


		CTargetManager::GetInstance()->CreateTarget(ch->GetPlayerID(),
				iQuestIndex,
				lua_tostring(L, 1),
				TARGET_TYPE_VID,
				(int32_t) lua_tonumber(L, 2),
				0,
				ch->GetMapIndex(),
				lua_isstring(L, 3) ? lua_tostring(L, 3) : NULL,
				lua_isnumber(L, 4) ? (int32_t)lua_tonumber(L, 4): 1);

		return 0;
	}

	int32_t target_delete(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t iQuestIndex = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1))
		{
			SysLog("invalid argument, name: {}, quest_index {}", ch->GetName(), iQuestIndex);
			return 0;
		}

		CTargetManager::GetInstance()->DeleteTarget(ch->GetPlayerID(), iQuestIndex, lua_tostring(L, 1));

		return 0;
	}

	int32_t target_clear(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		int32_t iQuestIndex = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		CTargetManager::GetInstance()->DeleteTarget(ch->GetPlayerID(), iQuestIndex, NULL);

		return 0;
	}

	int32_t target_id(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		uint32_t dwQuestIndex = CQuestManager::GetInstance()->GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1))
		{
			SysLog("invalid argument, name: {}, quest_index {}", ch->GetName(), dwQuestIndex);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPEVENT pEvent = CTargetManager::GetInstance()->GetTargetEvent(ch->GetPlayerID(), dwQuestIndex, (const char* ) lua_tostring(L, 1));

		if (pEvent)
		{
			TargetInfo* pInfo = dynamic_cast<TargetInfo *>(pEvent->info);

			if (pInfo == nullptr)
			{
				SysLog("target_id> <Factor> Null pointer");
				lua_pushnumber(L, 0);
				return 1;
			}

			if (pInfo->iType == TARGET_TYPE_VID)
			{
				lua_pushnumber(L, pInfo->iArg1);
				return 1;
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	void RegisterTargetFunctionTable()
	{
		luaL_reg target_functions[] =
		{
			{ "pos",			target_pos		},
			{ "vid",			target_vid		},
			{ "npc",			target_vid		}, // TODO: delete this
			{ "pc",			target_vid		}, // TODO: delete this
			{ "delete",			target_delete		},
			{ "clear",			target_clear		},
			{ "id",			target_id		},
			{ NULL,			NULL			},
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("target", target_functions);
	}
};

