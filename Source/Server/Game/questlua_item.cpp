#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "over9refine.h"
#include "log.h"

namespace quest
{
	//
	// "item" Lua functions
	//

	int32_t item_get_cell(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (q->GetCurrentItem())
		{
			lua_pushnumber(L, q->GetCurrentItem()->GetCell());
		}
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int32_t item_select_cell(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
		{
			return 1;
		}
		uint32_t cell = (uint32_t) lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPITEM item = ch ? ch->GetInventoryItem(cell) : NULL;

		if (!item)
		{
			return 1;
		}

		CQuestManager::GetInstance()->SetCurrentItem(item);
		lua_pushboolean(L, 1);

		return 1;
	}

	int32_t item_select(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
		{
			return 1;
		}
		uint32_t id = (uint32_t) lua_tonumber(L, 1);
		LPITEM item = ITEM_MANAGER::GetInstance()->Find(id);

		if (!item)
		{
			return 1;
		}

		CQuestManager::GetInstance()->SetCurrentItem(item);
		lua_pushboolean(L, 1);

		return 1;
	}

	int32_t item_get_id(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (q->GetCurrentItem())
		{
			lua_pushnumber(L, q->GetCurrentItem()->GetID());
		}
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int32_t item_remove(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();
		if (item != nullptr) {
			if (q->GetCurrentCharacterPtr() == item->GetOwner()) {
				ITEM_MANAGER::GetInstance()->RemoveItem(item);
			} else {
				SysLog("Tried to remove invalid item");
			}
			q->ClearCurrentItem();
		}

		return 0;
	}

	int32_t item_get_socket(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		if (q->GetCurrentItem() && lua_isnumber(L, 1))
		{
			int32_t idx = (int32_t) lua_tonumber(L, 1);
			if (idx < 0 || idx >= ITEM::SOCKET_MAX_NUM)
				lua_pushnumber(L,0);
			else
				lua_pushnumber(L, q->GetCurrentItem()->GetSocket(idx));
		}
		else
		{
			lua_pushnumber(L,0);
		}
		return 1;
	}

	int32_t item_set_socket(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		if (q->GetCurrentItem() && lua_isnumber(L,1) && lua_isnumber(L,2))
		{
			int32_t idx = (int32_t) lua_tonumber(L, 1);
			int32_t value = (int32_t) lua_tonumber(L, 2);
			if (idx >=0 && idx < ITEM::SOCKET_MAX_NUM)
				q->GetCurrentItem()->SetSocket(idx, value);
		}
		return 0;
	}

	int32_t item_get_vnum(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item) 
			lua_pushnumber(L, item->GetVnum());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_has_flag(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (!lua_isnumber(L, 1))
		{
			SysLog("flag is not a number.");
			lua_pushboolean(L, 0);
			return 1;
		}

		if (!item)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int32_t lCheckFlag = (int32_t) lua_tonumber(L, 1);	
		lua_pushboolean(L, IS_SET(item->GetFlag(), lCheckFlag));

		return 1;
	}

	int32_t item_get_value(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (!lua_isnumber(L, 1))
		{
			SysLog("index is not a number");
			lua_pushnumber(L, 0);
			return 1;
		}

		int32_t index = (int32_t) lua_tonumber(L, 1);

		if (index < 0 || index >= ITEM::VALUES_MAX_NUM)
		{
			SysLog("index({}) is out of range (0..{})", index, ITEM::VALUES_MAX_NUM);
			lua_pushnumber(L, 0);
		}
		else
			lua_pushnumber(L, item->GetValue(index));

		return 1;
	}

	int32_t item_set_value(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (!(lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)))
		{
			SysLog("index is not a number");
			lua_pushnumber(L, 0);
			return 1;
		}

		item->SetForceAttribute(
			lua_tonumber(L, 1),		// index
			lua_tonumber(L, 2),		// apply type
			lua_tonumber(L, 3)		// apply value
		);

		return 0;
	}

	int32_t item_get_name(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushstring(L, item->GetName());
		else
			lua_pushstring(L, "");

		return 1;
	}

	int32_t item_get_size(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetSize());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_get_count(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetCount());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_get_type(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetType());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_get_sub_type(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetSubType());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_get_refine_vnum(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetRefinedVnum());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_next_refine_vnum(lua_State* L)
	{
		uint32_t vnum = 0;
		if (lua_isnumber(L, 1))
			vnum = (uint32_t) lua_tonumber(L, 1);

		TItemTable* pTable = ITEM_MANAGER::GetInstance()->GetTable(vnum);
		if (pTable)
		{
			lua_pushnumber(L, pTable->dwRefinedVnum);
		}
		else
		{
			SysLog("Cannot find item table of vnum {}", vnum);
			lua_pushnumber(L, 0);
		}
		return 1;
	}

	int32_t item_get_level(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM item = q->GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetRefineLevel());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t item_can_over9refine(lua_State* L)
	{
		LPITEM item = CQuestManager::GetInstance()->GetCurrentItem();

		if (item == nullptr) return 0;

		lua_pushnumber(L, COver9RefineManager::GetInstance()->canOver9Refine(item->GetVnum()));

		return 1;
	}

	int32_t item_change_to_over9(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPITEM item = CQuestManager::GetInstance()->GetCurrentItem();

		if (ch == nullptr || item == nullptr) return 0;

		lua_pushboolean(L, COver9RefineManager::GetInstance()->Change9ToOver9(ch, item));
		
		return 1;
	}
	
	int32_t item_over9refine(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		LPITEM item = CQuestManager::GetInstance()->GetCurrentItem();

		if (ch == nullptr || item == nullptr) return 0;

		lua_pushboolean(L, COver9RefineManager::GetInstance()->Over9Refine(ch, item));
		
		return 1;
	}

	int32_t item_get_over9_material_vnum(lua_State* L)
	{
		if (lua_isnumber(L, 1))
		{
			lua_pushnumber(L, COver9RefineManager::GetInstance()->GetMaterialVnum((uint32_t)lua_tonumber(L, 1)));
			return 1;
		}

		return 0;
	}

	int32_t item_get_level_limit (lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (q->GetCurrentItem())
		{
			if (q->GetCurrentItem()->GetType() != ITEM::TYPE_WEAPON && q->GetCurrentItem()->GetType() != ITEM::TYPE_ARMOR)
			{
				return 0;
			}
			lua_pushnumber(L, q->GetCurrentItem() -> GetLevelLimit());
			return 1;
		}
		return 0;
	}

	int32_t item_start_realtime_expire(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPITEM pItem = q->GetCurrentItem();

		if (pItem)
		{
			pItem->StartRealTimeExpireEvent();
			return 1;
		}

		return 0;
	}
	
	int32_t item_copy_and_give_before_remove(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
			return 1;

		uint32_t vnum = (uint32_t)lua_tonumber(L, 1);

		auto q = CQuestManager::GetInstance();
		LPITEM pItem = q->GetCurrentItem();
		LPCHARACTER pChar = q->GetCurrentCharacterPtr();

		LPITEM pNewItem = ITEM_MANAGER::GetInstance()->CreateItem(vnum, 1, 0, false);

		if (pNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(pItem, pNewItem);
			LogManager::GetInstance()->ItemLog(pChar, pNewItem, "COPY SUCCESS", pNewItem->GetName());

			uint8_t bCell = pItem->GetCell();

			ITEM_MANAGER::GetInstance()->RemoveItem(pItem, "REMOVE (COPY SUCCESS)");

			pNewItem->AddToCharacter(pChar, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(pNewItem);
			pNewItem->AttrLog();

			lua_pushboolean(L, 1);			
		}

		return 1;
	}

	void RegisterITEMFunctionTable()
	{

		luaL_reg item_functions[] =
		{
			{ "get_id",		item_get_id		},
			{ "get_cell",		item_get_cell		},
			{ "select",		item_select		},
			{ "select_cell",	item_select_cell	},
			{ "remove",		item_remove		},
			{ "get_socket",		item_get_socket		},
			{ "set_socket",		item_set_socket		},
			{ "get_vnum",		item_get_vnum		},
			{ "has_flag",		item_has_flag		},
			{ "get_value",		item_get_value		},
			{ "set_value",		item_set_value		},
			{ "get_name",		item_get_name		},
			{ "get_size",		item_get_size		},
			{ "get_count",		item_get_count		},
			{ "get_type",		item_get_type		},
			{ "get_sub_type",	item_get_sub_type	},
			{ "get_refine_vnum",	item_get_refine_vnum	},
			{ "get_level",		item_get_level		},
			{ "next_refine_vnum",	item_next_refine_vnum	},
			{ "can_over9refine",	item_can_over9refine	},
			{ "change_to_over9",		item_change_to_over9	},
			{ "over9refine",		item_over9refine	},
			{ "get_over9_material_vnum",		item_get_over9_material_vnum	},
			{ "get_level_limit", 				item_get_level_limit },
			{ "start_realtime_expire", 			item_start_realtime_expire },
			{ "copy_and_give_before_remove",	item_copy_and_give_before_remove},

			{ NULL,			NULL			}
		};
		CQuestManager::GetInstance()->AddLuaFunctionTable("item", item_functions);
	}
}
