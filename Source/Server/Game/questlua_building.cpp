#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "guild.h"
#include "db.h"
#include "building.h"

namespace quest
{
	//
	// "building" Lua functions
	//
	int32_t building_get_land_id(lua_State * L)
	{
		using namespace building;

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		CLand* pLand = CManager::GetInstance()->FindLand((int32_t) lua_tonumber(L, 1), (int32_t) lua_tonumber(L, 2), (int32_t) lua_tonumber(L, 3));
		lua_pushnumber(L, pLand ? pLand->GetID() : 0);
		return 1;
	}

	int32_t building_get_land_info(lua_State * L)
	{
		int32_t price = 1000000000;
		int32_t owner = 1000000000;
		int32_t level_limit = 1000000000;

		if (lua_isnumber(L, 1))
		{
			using namespace building;

			CLand* pLand = CManager::GetInstance()->FindLand((uint32_t) lua_tonumber(L, 1));

			if (pLand)
			{
				const TLand & t = pLand->GetData();

				price = t.dwPrice;
				owner = t.dwGuildID;
				level_limit = t.bGuildLevelLimit;
			}
		}
		else
			SysLog("invalid argument");

		lua_pushnumber(L, price);
		lua_pushnumber(L, owner);
		lua_pushnumber(L, level_limit);
		return 3;
	}

	int32_t building_set_land_owner(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			SysLog("invalid argument");
			return 0;
		}

		using namespace building;

		CLand* pLand = CManager::GetInstance()->FindLand((uint32_t) lua_tonumber(L, 1));

		if (pLand)
		{
			if (pLand->GetData().dwGuildID == 0)
				pLand->SetOwner((uint32_t) lua_tonumber(L, 2));
		}

		return 0;
	}

	int32_t building_has_land(lua_State * L)
	{
		using namespace building;

		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			lua_pushboolean(L, true);
			return 1;
		}

		/*
		if (CManager::GetInstance()->FindLandByGuild((uint32_t) lua_tonumber(L, 1)))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
		*/

		std::unique_ptr<SQLMsg> pmsg(DBManager::GetInstance()->DirectQuery("SELECT COUNT(*) FROM land%s WHERE guild_id = %d", get_table_postfix(), (uint32_t)lua_tonumber(L,1)));

		if (pmsg->Get()->uiNumRows > 0)
		{
			MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

			int32_t	count = 0;
			str_to_number(count, row[0]);

			if (count == 0)
			{
				lua_pushboolean(L, false);
			}
			else
			{
				lua_pushboolean(L, true);
			}
		}
		else
		{
			lua_pushboolean(L, true);
		}

		return 1;
	}

	int32_t building_reconstruct(lua_State* L)
	{
		using namespace building;

		uint32_t dwNewBuilding = (uint32_t)lua_tonumber(L, 1);

		auto q = CQuestManager::GetInstance();

		LPCHARACTER npc = q->GetCurrentNPCCharacterPtr();
		if (!npc)
			return 0;

		CGuild* pGuild = npc->GetGuild();
		if (!pGuild)
			return 0;

		CLand* pLand = CManager::GetInstance()->FindLandByGuild(pGuild->GetID());
		if (!pLand)
			return 0;

		LPOBJECT pObject = pLand->FindObjectByNPC(npc);
		if (!pObject)
			return 0;

		pObject->Reconstruct(dwNewBuilding);

		return 0;
	}

	void RegisterBuildingFunctionTable()
	{
		luaL_reg functions[] =
		{
			{ "get_land_id",	building_get_land_id	},
			{ "get_land_info",	building_get_land_info	},
			{ "set_land_owner",	building_set_land_owner	},
			{ "has_land",	building_has_land	},
			{ "reconstruct",	building_reconstruct	},
			{ NULL,		NULL			}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("building", functions);
	}
};
