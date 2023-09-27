#include "stdafx.h"
#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "cmd.h"
#include "packet.h"


extern ACMD(do_in_game_mall);

namespace quest
{
	int32_t game_set_event_flag(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (lua_isstring(L,1) && lua_isnumber(L, 2))
			q->RequestSetEventFlag(lua_tostring(L,1), (int32_t)lua_tonumber(L,2));

		return 0;
	}

	int32_t game_get_event_flag(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (lua_isstring(L,1))
			lua_pushnumber(L, q->GetEventFlag(lua_tostring(L,1)));
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t game_request_make_guild(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDESC d = q->GetCurrentCharacterPtr()->GetDesc();
		if (d)
		{
			uint8_t header = HEADER_GC_REQUEST_MAKE_GUILD;
			d->Packet(&header, 1);
		}
		return 0;
	}

	int32_t game_get_safebox_level(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		lua_pushnumber(L, q->GetCurrentCharacterPtr()->GetSafeboxSize()/SAFEBOX_PAGE_SIZE);
		return 1;
	}

	int32_t game_set_safebox_level(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetCurrentCharacterPtr()->ChangeSafeboxSize(3*(int32_t)lua_tonumber(L,-1));
		TSafeboxChangeSizePacket p;
		p.dwID = q->GetCurrentCharacterPtr()->GetDesc()->GetAccountTable().id;
		p.bSize = (int32_t)lua_tonumber(L,-1);
		db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_CHANGE_SIZE,  q->GetCurrentCharacterPtr()->GetDesc()->GetHandle(), &p, sizeof(p));

		q->GetCurrentCharacterPtr()->SetSafeboxSize(SAFEBOX_PAGE_SIZE * (int32_t)lua_tonumber(L,-1));
		return 0;
	}

	int32_t game_open_safebox(lua_State* /*L*/)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		ch->SetSafeboxOpenPosition();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
		return 0;
	}

	int32_t game_open_mall(lua_State* /*L*/)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		ch->SetSafeboxOpenPosition();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
		return 0;
	}

	int32_t game_drop_item(lua_State* L)
	{
		//
		// Syntax: game.drop_item(50050, 1)
		//
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		uint32_t item_vnum = (uint32_t) lua_tonumber(L, 1);
		int32_t count = (int32_t) lua_tonumber(L, 2);
		int32_t x = ch->GetX();
		int32_t y = ch->GetY();

		LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(item_vnum, count);

		if (!item)
		{
			SysLog("cannot create item vnum {} count {}", item_vnum, count);
			return 0;
		}

		PIXEL_POSITION pos;
		pos.x = x + number(-200, 200);
		pos.y = y + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();

		return 0;
	}

	int32_t game_drop_item_with_ownership(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		LPITEM item = nullptr;
		switch (lua_gettop(L))
		{
		case 1:
			item = ITEM_MANAGER::GetInstance()->CreateItem((uint32_t) lua_tonumber(L, 1));
			break;
		case 2:
		case 3:
			item = ITEM_MANAGER::GetInstance()->CreateItem((uint32_t) lua_tonumber(L, 1), (int32_t) lua_tonumber(L, 2));
			break;
		default:
			return 0;
		}

		if (item == nullptr)
		{
			return 0;
		}

		if (lua_isnumber(L, 3))
		{
			int32_t sec = (int32_t) lua_tonumber(L, 3);
			if (sec <= 0)
			{
				item->SetOwnership(ch);
			}
			else
			{
				item->SetOwnership(ch, sec);
			}
		}
		else
			item->SetOwnership(ch);

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();

		return 0;
	}
	
	void RegisterGameFunctionTable()
	{
		luaL_reg game_functions[] = 
		{
			{ "get_safebox_level",			game_get_safebox_level			},
			{ "request_make_guild",			game_request_make_guild			},
			{ "set_safebox_level",			game_set_safebox_level			},
			{ "open_safebox",				game_open_safebox				},
			{ "open_mall",					game_open_mall					},
			{ "get_event_flag",				game_get_event_flag				},
			{ "set_event_flag",				game_set_event_flag				},
			{ "drop_item",					game_drop_item					},
			{ "drop_item_with_ownership",	game_drop_item_with_ownership	},

			{ NULL,					NULL				}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("game", game_functions);
	}
}

