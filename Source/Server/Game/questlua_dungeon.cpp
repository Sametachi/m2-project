#include "stdafx.h"
#include "constants.h"
#include "questmanager.h"
#include "questlua.h"
#include "dungeon.h"
#include "char.h"
#include "party.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"

template <class Func> Func CDungeon::ForEachMember(Func f)
{
	for (auto it = m_set_pCharacter.begin(); it != m_set_pCharacter.end(); ++it)
	{
		PyLog("Dungeon ForEachMember {}", (*it)->GetName());
		f(*it);
	}
	return f;
}

namespace quest
{
	//
	// "dungeon" lua functions
	//
	int32_t dungeon_notice(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Notice(lua_tostring(L, 1));
		return 0;
	}

	int32_t dungeon_set_quest_flag(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		FSetQuestFlag f;

		f.flagname = q->GetCurrentPC()->GetCurrentQuestName() + "." + lua_tostring(L, 1);
		f.value = (int32_t) rint(lua_tonumber(L, 2));

		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ForEachMember(f);

		return 0;
	}

	int32_t dungeon_set_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			SysLog("wrong set flag");
		}
		else
		{
			auto q = CQuestManager::GetInstance();
			LPDUNGEON pDungeon = q->GetCurrentDungeon();

			if (pDungeon)
			{
				const char* sz = lua_tostring(L,1);
				int32_t value = int32_t(lua_tonumber(L, 2));
				pDungeon->SetFlag(sz, value);
			}
			else
			{
				SysLog("no dungeon !!!");
			}
		}
		return 0;
	}

	int32_t dungeon_get_flag(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			SysLog("wrong get flag");
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			const char* sz = lua_tostring(L,1);
			lua_pushnumber(L, pDungeon->GetFlag(sz));
		}
		else
		{
			SysLog("no dungeon !!!");
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t dungeon_get_flag_from_map_index(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			SysLog("wrong get flag");
		}

		uint32_t dwMapIndex = (uint32_t) lua_tonumber(L, 2);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::GetInstance()->FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				const char* sz = lua_tostring(L,1);
				lua_pushnumber(L, pDungeon->GetFlag(sz));
			}
			else
			{
				SysLog("no dungeon !!!");
				lua_pushnumber(L, 0);
			}
		}
		else
		{
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int32_t dungeon_get_map_index(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			PyLog("Dungeon GetMapIndex {}",pDungeon->GetMapIndex());
			lua_pushnumber(L, pDungeon->GetMapIndex());
		}
		else
		{
			SysLog("no dungeon !!!");
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int32_t dungeon_regen_file(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			SysLog("wrong filename");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnRegen(lua_tostring(L,1));

		return 0;
	}

	int32_t dungeon_set_regen_file(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			SysLog("wrong filename");
			return 0;
		}
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnRegen(lua_tostring(L,1), false);
		return 0;
	}

	int32_t dungeon_clear_regen(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();
		if (pDungeon)
			pDungeon->ClearRegen();
		return 0;
	}

	int32_t dungeon_check_eliminated(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();
		if (pDungeon)
			pDungeon->CheckEliminated();
		return 0;
	}

	int32_t dungeon_set_exit_all_at_eliminate(lua_State* L)
	{
		if (!lua_isnumber(L,1))
		{
			SysLog("wrong time");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SetExitAllAtEliminate((int32_t)lua_tonumber(L, 1));

		return 0;
	}

	int32_t dungeon_set_warp_at_eliminate(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("wrong time");
			return 0;
		}

		if (!lua_isnumber(L, 2))
		{
			SysLog("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 3)) 
		{
			SysLog("wrong X");
			return 0;
		}

		if (!lua_isnumber(L, 4))
		{
			SysLog("wrong Y");
			return 0;
		}

		const char* c_pszRegenFile = nullptr;

		if (lua_gettop(L) >= 5)
			c_pszRegenFile = lua_tostring(L,5);

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			pDungeon->SetWarpAtEliminate((int32_t)lua_tonumber(L,1),
										 (int32_t)lua_tonumber(L,2),
										 (int32_t)lua_tonumber(L,3),
										 (int32_t)lua_tonumber(L,4),
										 c_pszRegenFile);
		}
		else
			SysLog("cannot find dungeon");

		return 0;
	}

	int32_t dungeon_new_jump(lua_State* L)
	{
		if (lua_gettop(L) < 3)
		{
			SysLog("not enough argument");
			return 0;
		}

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("wrong argument");
			return 0;
		}

		int32_t lMapIndex = (int32_t)lua_tonumber(L,1);

		LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Create(lMapIndex);

		if (!pDungeon)
		{
			SysLog("cannot create dungeon {}", lMapIndex);
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		//ch->WarpSet(pDungeon->GetMapIndex(), (int32_t) lua_tonumber(L, 2), (int32_t)lua_tonumber(L, 3));
		ch->WarpSet((int32_t) lua_tonumber(L, 2), (int32_t)lua_tonumber(L, 3), pDungeon->GetMapIndex());
		return 0;
	}

	int32_t dungeon_new_jump_all(lua_State* L)
	{
		if (lua_gettop(L)<3 || !lua_isnumber(L,1) || !lua_isnumber(L, 2) || !lua_isnumber(L,3))
		{
			SysLog("not enough argument");
			return 0;
		}

		int32_t lMapIndex = (int32_t)lua_tonumber(L,1);

		LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Create(lMapIndex);

		if (!pDungeon)
		{
			SysLog("cannot create dungeon {}", lMapIndex);
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		pDungeon->JumpAll(ch->GetMapIndex(), (int32_t)lua_tonumber(L, 2), (int32_t)lua_tonumber(L, 3));

		return 0;
	}

	int32_t dungeon_new_jump_party (lua_State* L)
	{
		if (lua_gettop(L)<3 || !lua_isnumber(L,1) || !lua_isnumber(L, 2) || !lua_isnumber(L,3))
		{
			SysLog("not enough argument");
			return 0;
		}

		int32_t lMapIndex = (int32_t)lua_tonumber(L,1);

		LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Create(lMapIndex);

		if (!pDungeon)
		{
			SysLog("cannot create dungeon {}", lMapIndex);
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty() == nullptr)
		{
			SysLog("cannot go to dungeon alone.");
			return 0;
		}
		pDungeon->JumpParty(ch->GetParty(), ch->GetMapIndex(), (int32_t)lua_tonumber(L, 2), (int32_t)lua_tonumber(L, 3));

		return 0;
	}

	int32_t dungeon_jump_all(lua_State* L)
	{
		if (lua_gettop(L)<2 || !lua_isnumber(L, 1) || !lua_isnumber(L,2))
			return 0;

		LPDUNGEON pDungeon = CQuestManager::GetInstance()->GetCurrentDungeon();

		if (!pDungeon)
			return 0;

		pDungeon->JumpAll(pDungeon->GetMapIndex(), (int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2));
		return 0;
	}

	int32_t dungeon_warp_all(lua_State* L)
	{
		if (lua_gettop(L)<2 || !lua_isnumber(L, 1) || !lua_isnumber(L,2))
			return 0;

		LPDUNGEON pDungeon = CQuestManager::GetInstance()->GetCurrentDungeon();

		if (!pDungeon)
			return 0;

		pDungeon->WarpAll(pDungeon->GetMapIndex(), (int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2));
		return 0;
	}

	int32_t dungeon_get_kill_stone_count(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}


		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetKillStoneCount());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int32_t dungeon_get_kill_mob_count(lua_State * L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetKillMobCount());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int32_t dungeon_is_use_potion(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, 1);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUsePotion());
			return 1;
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int32_t dungeon_revived(lua_State* L) 
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushboolean(L, 1);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUseRevive());
			return 1;
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int32_t dungeon_set_dest(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		LPPARTY pParty = ch->GetParty();
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon && pParty)
			pDungeon->SendDestPositionToParty(pParty, (int32_t)lua_tonumber(L,1), (int32_t)lua_tonumber(L,2));

		return 0;
	}

	int32_t dungeon_unique_set_maxhp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetMaxHP(lua_tostring(L,1), (int32_t)lua_tonumber(L,2));

		return 0;
	}

	int32_t dungeon_unique_set_hp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetHP(lua_tostring(L,1), (int32_t)lua_tonumber(L,2));

		return 0;
	}

	int32_t dungeon_unique_set_def_grade(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetDefGrade(lua_tostring(L,1), (int32_t)lua_tonumber(L,2));

		return 0;
	}

	int32_t dungeon_unique_get_hp_perc(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushnumber(L,0);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetUniqueHpPerc(lua_tostring(L,1)));
			return 1;
		}

		lua_pushnumber(L,0);
		return 1;
	}

	int32_t dungeon_is_unique_dead(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUniqueDead(lua_tostring(L,1))?1:0);
			return 1;
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int32_t dungeon_purge_unique(lua_State* L)
	{
		if (!lua_isstring(L,1))
			return 0;
		PyLog("QUEST_DUNGEON_PURGE_UNIQUE {}", lua_tostring(L,1));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->PurgeUnique(lua_tostring(L,1));

		return 0;
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
	
	int32_t dungeon_purge_area(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2) || !lua_isnumber(L,3) || !lua_isnumber(L,4))
			return 0;
		PyLog("QUEST_DUNGEON_PURGE_AREA");

		int32_t x1 = lua_tonumber(L, 1);
		int32_t y1 = lua_tonumber(L, 2);
		int32_t x2 = lua_tonumber(L, 3);
		int32_t y2 = lua_tonumber(L, 4);

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		const int32_t mapIndex = pDungeon->GetMapIndex();

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

	int32_t dungeon_kill_unique(lua_State* L)
	{
		if (!lua_isstring(L,1))
			return 0;
		PyLog("QUEST_DUNGEON_KILL_UNIQUE {}", lua_tostring(L,1));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->KillUnique(lua_tostring(L,1));

		return 0;
	}

	int32_t dungeon_spawn_stone_door(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN_STONE_DOOR {} {}", lua_tostring(L,1), lua_tostring(L,2));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnStoneDoor(lua_tostring(L,1), lua_tostring(L,2));

		return 0;
	}

	int32_t dungeon_spawn_wooden_door(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN_WOODEN_DOOR {} {}", lua_tostring(L,1), lua_tostring(L,2));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnWoodenDoor(lua_tostring(L,1), lua_tostring(L,2));

		return 0;
	}

	int32_t dungeon_spawn_move_group(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isstring(L,2) || !lua_isstring(L,3))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN_MOVE_GROUP {} {} {}", (int32_t)lua_tonumber(L,1), lua_tostring(L,2), lua_tostring(L,3));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnMoveGroup((uint32_t)lua_tonumber(L,1), lua_tostring(L,2), lua_tostring(L,3));

		return 0;
	}

	int32_t dungeon_spawn_move_unique(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2) || !lua_isstring(L,3) || !lua_isstring(L,4))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN_MOVE_UNIQUE {} {} {} {}", lua_tostring(L,1), (int32_t)lua_tonumber(L,2), lua_tostring(L,3), lua_tostring(L,4));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnMoveUnique(lua_tostring(L,1), (uint32_t)lua_tonumber(L,2), lua_tostring(L,3), lua_tostring(L,4));

		return 0;
	}

	int32_t dungeon_spawn_unique(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2) || !lua_isstring(L,3))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN_UNIQUE {} {} {}", lua_tostring(L,1), (int32_t)lua_tonumber(L,2), lua_tostring(L,3));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnUnique(lua_tostring(L,1), (uint32_t)lua_tonumber(L,2), lua_tostring(L,3));

		return 0;
	}

	int32_t dungeon_spawn(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isstring(L,2))
			return 0;
		PyLog("QUEST_DUNGEON_SPAWN {} {}", (int32_t)lua_tonumber(L,1), lua_tostring(L,2));

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Spawn((uint32_t)lua_tonumber(L,1), lua_tostring(L,2));

		return 0;
	}

	int32_t dungeon_set_unique(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		uint32_t vid = (uint32_t) lua_tonumber(L, 2);

		if (pDungeon)
			pDungeon->SetUnique(lua_tostring(L, 1), vid);
		return 0;
	}

	int32_t dungeon_get_unique_vid(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushnumber(L,0);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetUniqueVid(lua_tostring(L,1)));
			return 1;
		}

		lua_pushnumber(L,0);
		return 1;
	}

	int32_t dungeon_spawn_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		uint32_t vid = 0;

		if (pDungeon)
		{
			uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
			int32_t x = (int32_t) lua_tonumber(L, 2);
			int32_t y = (int32_t) lua_tonumber(L, 3);
			float radius = lua_isnumber(L, 4) ? (float) lua_tonumber(L, 4) : 0;
			uint32_t count = (lua_isnumber(L, 5)) ? (uint32_t) lua_tonumber(L, 5) : 1;

			PyLog("dungeon_spawn_mob {} {} {}", dwVnum, x, y);

			if (count == 0)
				count = 1;

			while (count --)
			{
				if (radius<1)
				{
					LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, x, y);
					if (ch && !vid)
						vid = ch->GetVID();
				}
				else
				{
					float angle = number(0, 999) * M_PI * 2 / 1000;
					float r = number(0, 999) * radius / 1000;

					int32_t nx = x + (int32_t)(r * cos(angle));
					int32_t ny = y + (int32_t)(r * sin(angle));

					LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, nx, ny);
					if (ch && !vid)
						vid = ch->GetVID();
				}
			}
		}

		lua_pushnumber(L, vid);
		return 1;
	}

	int32_t dungeon_spawn_mob_dir(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		uint32_t vid = 0;

		if (pDungeon)
		{
			uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
			int32_t x = (int32_t) lua_tonumber(L, 2);
			int32_t y = (int32_t) lua_tonumber(L, 3);
			uint8_t dir = (int32_t) lua_tonumber(L, 4);
			
			LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, x, y, dir);
			if (ch && !vid)
				vid = ch->GetVID();
		}
		lua_pushnumber(L, vid);
		return 1;
	}
	
	int32_t dungeon_spawn_mob_ac_dir(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		uint32_t vid = 0;

		if (pDungeon)
		{
			uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
			int32_t x = (int32_t) lua_tonumber(L, 2);
			int32_t y = (int32_t) lua_tonumber(L, 3);
			uint8_t dir = (int32_t) lua_tonumber(L, 4);
			
			LPCHARACTER ch = pDungeon->SpawnMob_ac_dir(dwVnum, x, y, dir);
			if (ch && !vid)
				vid = ch->GetVID();
		}
		lua_pushnumber(L, vid);
		return 1;
	}

	int32_t dungeon_spawn_goto_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
			return 0;

		int32_t lFromX = (int32_t)lua_tonumber(L, 1);
		int32_t lFromY = (int32_t)lua_tonumber(L, 2);
		int32_t lToX   = (int32_t)lua_tonumber(L, 3);
		int32_t lToY   = (int32_t)lua_tonumber(L, 4);

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnGotoMob(lFromX, lFromY, lToX, lToY);

		return 0;
	}

	int32_t dungeon_spawn_name_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isstring(L, 4))
			return 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			uint32_t dwVnum = (uint32_t) lua_tonumber(L, 1);
			int32_t x = (int32_t)lua_tonumber(L, 2);
			int32_t y = (int32_t)lua_tonumber(L, 3);
			pDungeon->SpawnNameMob(dwVnum, x, y, lua_tostring(L, 4));
		}
		return 0;
	}

	int32_t dungeon_spawn_group(lua_State* L)
	{
		//
		// argument: vnum,x,y,radius,aggressive,count
		//
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 6))
		{
			SysLog("invalid argument");
			return 0;
		}

		uint32_t vid = 0;

		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
		{
			uint32_t group_vnum = (uint32_t)lua_tonumber(L, 1);
			int32_t local_x = (int32_t) lua_tonumber(L, 2) * 100;
			int32_t local_y = (int32_t) lua_tonumber(L, 3) * 100;
			float radius = (float) lua_tonumber(L, 4) * 100;
			bool bAggressive = lua_toboolean(L, 5);
			uint32_t count = (uint32_t) lua_tonumber(L, 6);

			LPCHARACTER chRet = pDungeon->SpawnGroup(group_vnum, local_x, local_y, radius, bAggressive, count);
			if (chRet)
				vid = chRet->GetVID();
		}

		lua_pushnumber(L, vid);
		return 1;
	}

	int32_t dungeon_join(lua_State* L)
	{
		if (lua_gettop(L) < 1 || !lua_isnumber(L, 1))
			return 0;

		int32_t lMapIndex = (int32_t)lua_tonumber(L, 1);
		LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Create(lMapIndex);

		if (!pDungeon)
			return 0;

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
			pDungeon->JoinParty(ch->GetParty());
		else if (!ch->GetParty())
			pDungeon->Join(ch);

		return 0;
	}

	int32_t dungeon_exit(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		ch->ExitToSavedLocation();
		return 0;
	}

	int32_t dungeon_exit_all(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ExitAll();

		return 0;
	}

	struct FSayDungeonByItemGroup
	{
		const CDungeon::ItemGroup* item_group;
		std::string can_enter_ment;
		std::string cant_enter_ment;
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					struct ::packet_script packet_script;
					TEMP_BUFFER buf;
					
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							packet_script.header = HEADER_GC_SCRIPT;
							packet_script.skin = quest::CQuestManager::QUEST_SKIN_NORMAL;
							packet_script.src_size = can_enter_ment.size();
							packet_script.size = packet_script.src_size + sizeof(struct packet_script);

							buf.write(&packet_script, sizeof(struct packet_script));
							buf.write(&can_enter_ment[0], can_enter_ment.size());
							ch->GetDesc()->Packet(buf.read_peek(), buf.size());
							return;
						}
					}

					packet_script.header = HEADER_GC_SCRIPT;
					packet_script.skin = quest::CQuestManager::QUEST_SKIN_NORMAL;
					packet_script.src_size = cant_enter_ment.size();
					packet_script.size = packet_script.src_size + sizeof(struct packet_script);

					buf.write(&packet_script, sizeof(struct packet_script));
					buf.write(&cant_enter_ment[0], cant_enter_ment.size());
					ch->GetDesc()->Packet(buf.read_peek(), buf.size());
				}
			}
		}
	};


	int32_t dungeon_say_diff_by_item_group(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3))
		{
			PyLog("QUEST wrong set flag");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		CDungeon* pDungeon = q->GetCurrentDungeon();

		if(!pDungeon)
		{
			SysLog("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP* pMap = SECTREE_MANAGER::GetInstance()->GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			SysLog("cannot find map by index {}", pDungeon->GetMapIndex());
			return 0;
		}
		FSayDungeonByItemGroup f;
		PyLog("diff_by_item");
	
		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);

		if (f.item_group == nullptr)
		{
			SysLog("invalid item group");
			return 0;
		}

		f.can_enter_ment = lua_tostring(L, 2);
		f.can_enter_ment+= "[ENTER][ENTER][ENTER][ENTER][DONE]";
		f.cant_enter_ment = lua_tostring(L, 3);
		f.cant_enter_ment+= "[ENTER][ENTER][ENTER][ENTER][DONE]";

		pMap -> for_each(f);

		return 0;
	}
	
	struct FExitDungeonByItemGroup
	{
		const CDungeon::ItemGroup* item_group;
		
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							return;
						}
					}
					ch->ExitToSavedLocation();
				}
			}
		}
	};
	
	int32_t dungeon_exit_all_by_item_group (lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			PyLog("QUEST wrong set flag");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		CDungeon* pDungeon = q->GetCurrentDungeon();

		if(!pDungeon)
		{
			SysLog("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP* pMap = SECTREE_MANAGER::GetInstance()->GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			SysLog("cannot find map by index {}", pDungeon->GetMapIndex());
			return 0;
		}
		FExitDungeonByItemGroup f;

		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);
	
		if (f.item_group == nullptr)
		{
			SysLog("invalid item group");
			return 0;
		}
		
		pMap -> for_each(f);
		
		return 0;
	}

	struct FDeleteItemInItemGroup
	{
		const CDungeon::ItemGroup* item_group;

		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							ch->RemoveSpecifyItem (it->first, it->second);
							return;
						}
					}
				}
			}
		}
	};
	
	int32_t dungeon_delete_item_in_item_group_from_all(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			PyLog("QUEST wrong set flag");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		CDungeon* pDungeon = q->GetCurrentDungeon();

		if(!pDungeon)
		{
			SysLog("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP* pMap = SECTREE_MANAGER::GetInstance()->GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			SysLog("cannot find map by index {}", pDungeon->GetMapIndex());
			return 0;
		}
		FDeleteItemInItemGroup f;

		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);

		if (f.item_group == nullptr)
		{
			SysLog("invalid item group");
			return 0;
		}

		pMap -> for_each(f);
		
		return 0;
	}


	int32_t dungeon_kill_all(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->KillAll();

		return 0;
	}

	int32_t dungeon_purge(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Purge();

		return 0;
	}

	int32_t dungeon_exit_all_to_start_position(lua_State * L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ExitAllToStartPosition();

		return 0;
	}

	int32_t dungeon_count_monster(lua_State * L)
	{
		auto q = CQuestManager::GetInstance();
		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			lua_pushnumber(L, pDungeon->CountMonster());
		else
		{
			SysLog("not in a dungeon");
			lua_pushnumber(L, LONG_MAX);
		}

		return 1;
	}

	int32_t dungeon_select(lua_State* L)
	{
		uint32_t dwMapIndex = (uint32_t) lua_tonumber(L, 1);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::GetInstance()->FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				CQuestManager::GetInstance()->SelectDungeon(pDungeon);
				lua_pushboolean(L, 1);
			}
			else
			{
				CQuestManager::GetInstance()->SelectDungeon(nullptr);
				lua_pushboolean(L, 0);
			}
		}
		else
		{
			CQuestManager::GetInstance()->SelectDungeon(nullptr);
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int32_t dungeon_find(lua_State* L)
	{
		uint32_t dwMapIndex = (uint32_t) lua_tonumber(L, 1);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::GetInstance()->FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				lua_pushboolean(L, 1);
			}
			else
			{
				lua_pushboolean(L, 0);
			}
		}
		else
		{
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int32_t dungeon_all_near_to(lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::GetInstance()->GetCurrentDungeon();

		if (pDungeon != nullptr)
		{
			lua_pushboolean(L, pDungeon->IsAllPCNearTo((int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2), 30));
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int32_t dungeon_set_warp_location (lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::GetInstance()->GetCurrentDungeon();

		if (pDungeon == nullptr)
		{
			return 0;
		}
		
		if (lua_gettop(L)<3 || !lua_isnumber(L, 1) || !lua_isnumber(L,2) || !lua_isnumber(L, 3))
		{
			return 0;
		}

		FSetWarpLocation f ((int32_t)lua_tonumber(L, 1), (int32_t)lua_tonumber(L, 2), (int32_t)lua_tonumber(L, 3));
		pDungeon->ForEachMember (f);

		return 0;
	}

	int32_t dungeon_set_item_group (lua_State* L)
	{
		if (!lua_isstring (L, 1) || !lua_isnumber (L, 2))
		{
			return 0;
		}
		std::string group_name (lua_tostring (L, 1));
		int32_t size = lua_tonumber (L, 2);

		CDungeon::ItemGroup item_group;
		
		for (int32_t i = 0; i < size; i++)
		{
			if (!lua_isnumber (L, i * 2 + 3) || !lua_isnumber (L, i * 2 + 4))
			{
				return 0;
			}
			item_group.push_back (std::pair <uint32_t, int32_t> (lua_tonumber (L, i * 2 + 3), lua_tonumber (L, i * 2 + 4)));
		}
		LPDUNGEON pDungeon = CQuestManager::GetInstance()->GetCurrentDungeon();

		if (pDungeon == nullptr)
		{
			return 0;
		}
		
		pDungeon->CreateItemGroup (group_name, item_group);
		return 0;
	}

	int32_t dungeon_set_quest_flag2(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		FSetQuestFlag f;

		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("Invalid Argument");
		}

		f.flagname = std::string (lua_tostring(L, 1)) + "." + lua_tostring(L, 2);
		f.value = (int32_t) rint(lua_tonumber(L, 3));

		LPDUNGEON pDungeon = q->GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ForEachMember(f);

		return 0;
	}

	void RegisterDungeonFunctionTable() 
	{
		luaL_reg dungeon_functions[] = 
		{
			{ "join",			dungeon_join		},
			{ "exit",			dungeon_exit		},
			{ "exit_all",		dungeon_exit_all	},
			{ "set_item_group",	dungeon_set_item_group	},
			{ "exit_all_by_item_group",	dungeon_exit_all_by_item_group},
			{ "say_diff_by_item_group",	dungeon_say_diff_by_item_group},
			{ "delete_item_in_item_group_from_all", dungeon_delete_item_in_item_group_from_all},
			{ "purge",			dungeon_purge		},
			{ "kill_all",		dungeon_kill_all	},
			{ "spawn",			dungeon_spawn		},
			{ "spawn_mob",		dungeon_spawn_mob	},
			{ "spawn_mob_dir",	dungeon_spawn_mob_dir	},
			{ "spawn_mob_ac_dir",	dungeon_spawn_mob_ac_dir	},
			{ "spawn_name_mob",	dungeon_spawn_name_mob	},
			{ "spawn_goto_mob",		dungeon_spawn_goto_mob	},
			{ "spawn_group",		dungeon_spawn_group	},
			{ "spawn_unique",		dungeon_spawn_unique	},
			{ "spawn_move_unique",		dungeon_spawn_move_unique},
			{ "spawn_move_group",		dungeon_spawn_move_group},
			{ "spawn_stone_door",		dungeon_spawn_stone_door},
			{ "spawn_wooden_door",		dungeon_spawn_wooden_door},
			{ "purge_unique",		dungeon_purge_unique	},
			{ "purge_area",			dungeon_purge_area	},
			{ "kill_unique",		dungeon_kill_unique	},
			{ "is_unique_dead",		dungeon_is_unique_dead	},
			{ "unique_get_hp_perc",		dungeon_unique_get_hp_perc},
			{ "unique_set_def_grade",	dungeon_unique_set_def_grade},
			{ "unique_set_hp",		dungeon_unique_set_hp	},
			{ "unique_set_maxhp",		dungeon_unique_set_maxhp},
			{ "get_unique_vid",		dungeon_get_unique_vid},
			{ "get_kill_stone_count",	dungeon_get_kill_stone_count},
			{ "get_kill_mob_count",		dungeon_get_kill_mob_count},
			{ "is_use_potion",		dungeon_is_use_potion	},
			{ "revived",			dungeon_revived		},
			{ "set_dest",			dungeon_set_dest	},
			{ "jump_all",			dungeon_jump_all	},
			{ "warp_all",		dungeon_warp_all	},
			{ "new_jump_all",		dungeon_new_jump_all	},
			{ "new_jump_party",		dungeon_new_jump_party	},
			{ "new_jump",			dungeon_new_jump	},
			{ "regen_file",			dungeon_regen_file	},
			{ "set_regen_file",		dungeon_set_regen_file	},
			{ "clear_regen",		dungeon_clear_regen	},
			{ "set_exit_all_at_eliminate",	dungeon_set_exit_all_at_eliminate},
			{ "set_warp_at_eliminate",	dungeon_set_warp_at_eliminate},
			{ "get_map_index",		dungeon_get_map_index	},
			{ "check_eliminated",		dungeon_check_eliminated},
			{ "exit_all_to_start_position",	dungeon_exit_all_to_start_position },
			{ "count_monster",		dungeon_count_monster	},
			{ "setf",					dungeon_set_flag	},
			{ "getf",					dungeon_get_flag	},
			{ "getf_from_map_index",	dungeon_get_flag_from_map_index	},
			{ "set_unique",			dungeon_set_unique	},
			{ "select",			dungeon_select		},
			{ "find",			dungeon_find		},
			{ "notice",			dungeon_notice		},
			{ "setqf",			dungeon_set_quest_flag	},
			{ "all_near_to",	dungeon_all_near_to	},
			{ "set_warp_location",	dungeon_set_warp_location	},
			{ "setqf2",			dungeon_set_quest_flag2	},

			{ NULL,				NULL			}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("d", dungeon_functions);
	}
}
