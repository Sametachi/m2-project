
#include "stdafx.h"
#include "questmanager.h"
#include "BattleArena.h"

namespace quest
{
	int32_t ba_start(lua_State* L)
	{
		if (lua_isnumber(L, 1))
		{
			CBattleArena::GetInstance()->Start((int32_t)lua_tonumber(L, 1));
		}

		return 0;
	}

	void RegisterBattleArenaFunctionTable()
	{
		luaL_reg ba_functions[] =
		{
			{	"start",	ba_start	},

			{	NULL,		NULL		}
		};

		CQuestManager::GetInstance()-> AddLuaFunctionTable("ba", ba_functions);
	}
}

