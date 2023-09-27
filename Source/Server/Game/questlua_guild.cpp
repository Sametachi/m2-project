#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "guild.h"
#include "guild_manager.h"

namespace quest
{
	//
	// "guild" Lua functions
	//
	int32_t guild_around_ranking_string(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		if (!ch->GetGuild())
			lua_pushstring(L,"");
		else
		{
			char szBuf[4096+1];
			CGuildManager::GetInstance()->GetAroundRankString(ch->GetGuild()->GetID(), szBuf, sizeof(szBuf));
			lua_pushstring(L, szBuf);
		}
		return 1;
	}

	int32_t guild_high_ranking_string(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		uint32_t dwMyGuild = 0;
		if (ch->GetGuild())
			dwMyGuild = ch->GetGuild()->GetID();

		char szBuf[4096+1];
		CGuildManager::GetInstance()->GetHighRankString(dwMyGuild, szBuf, sizeof(szBuf));
		lua_pushstring(L, szBuf);
		return 1;
	}

	int32_t guild_get_ladder_point(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		if (!ch->GetGuild())
		{
			lua_pushnumber(L, -1);
		}
		else
		{
			lua_pushnumber(L, ch->GetGuild()->GetLadderPoint());
		}
		return 1;
	}

	int32_t guild_get_rank(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (!ch->GetGuild())
		{
			lua_pushnumber(L, -1);
		}
		else
		{
			lua_pushnumber(L, CGuildManager::GetInstance()->GetRank(ch->GetGuild()));
		}
		return 1;
	}

	int32_t guild_is_war(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetGuild() && ch->GetGuild()->UnderWar((uint32_t) lua_tonumber(L, 1)))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);

		return 1;
	}

	int32_t guild_name(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		CGuild* pGuild = CGuildManager::GetInstance()->FindGuild((uint32_t) lua_tonumber(L, 1));

		if (pGuild)
			lua_pushstring(L, pGuild->GetName());
		else
			lua_pushstring(L, "");

		return 1;
	}

	int32_t guild_level(lua_State* L)
	{
		luaL_checknumber(L, 1);

		CGuild* pGuild = CGuildManager::GetInstance()->FindGuild((uint32_t) lua_tonumber(L, 1));

		if (pGuild)
			lua_pushnumber(L, pGuild->GetLevel());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t guild_war_enter(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (ch->GetGuild())
			ch->GetGuild()->GuildWarEntryAccept((uint32_t) lua_tonumber(L, 1), ch);

		return 0;
	}

	int32_t guild_get_any_war(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetGuild())
			lua_pushnumber(L, ch->GetGuild()->UnderAnyWar());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t guild_get_name(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushstring(L,  "");
			return 1;
		}

		CGuild* pGuild = CGuildManager::GetInstance()->FindGuild((uint32_t) lua_tonumber(L, 1));

		if (pGuild)
			lua_pushstring(L, pGuild->GetName());
		else
			lua_pushstring(L, "");

		return 1;
	}

	int32_t guild_war_bet(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			SysLog("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		TPacketGDGuildWarBet p;

		p.dwWarID = (uint32_t) lua_tonumber(L, 1);
		strlcpy(p.szLogin, ch->GetDesc()->GetAccountTable().login, sizeof(p.szLogin));
		p.dwGuild = (uint32_t) lua_tonumber(L, 2);
		p.dwGold = (uint32_t) lua_tonumber(L, 3);

		PyLog("GUILD_WAR_BET: {} login {} war_id {} guild {} gold {}", 
				ch->GetName(), p.szLogin, p.dwWarID, p.dwGuild, p.dwGold);

		db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR_BET, 0, &p, sizeof(p));
		return 0;
	}

	int32_t guild_is_bet(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			SysLog("invalid argument");
			lua_pushboolean(L, true);
			return 1;
		}

		bool bBet = CGuildManager::GetInstance()->IsBet((uint32_t) lua_tonumber(L, 1),
				CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetDesc()->GetAccountTable().login);

		lua_pushboolean(L, bBet);
		return 1;
	}

	int32_t guild_get_warp_war_list(lua_State* L)
	{
		FBuildLuaGuildWarList f(L);
		CGuildManager::GetInstance()->for_each_war(f);
		return 1;
	}

	int32_t guild_get_reserve_war_table(lua_State * L)
	{
		std::vector<CGuildWarReserveForGame *> & con = CGuildManager::GetInstance()->GetReserveWarRef();

		int32_t i = 0;
		std::vector<CGuildWarReserveForGame *>::iterator it = con.begin();

		PyLog("con.size(): {}", con.size());

		// stack : table1
		lua_newtable(L);

		while (it != con.end())
		{
			TGuildWarReserve* p = &(*(it++))->data;

			if (p->bType != GUILD_WAR_TYPE_BATTLE)
				continue;

			lua_newtable(L);

			PyLog("con.size(): {} {} {} handi {}", p->dwID, p->dwGuildFrom, p->dwGuildTo, p->lHandicap);

			// stack : table1 table2
			lua_pushnumber(L, p->dwID);
			// stack : table1 table2 dwID
			lua_rawseti(L, -2, 1);

			// stack : table1 table2
			if (p->lPowerFrom > p->lPowerTo)
				lua_pushnumber(L, p->dwGuildFrom);
			else
				lua_pushnumber(L, p->dwGuildTo);
			// stack : table1 table2 guildfrom
			lua_rawseti(L, -2, 2);

			// stack : table1 table2
			if (p->lPowerFrom > p->lPowerTo)
				lua_pushnumber(L, p->dwGuildTo);
			else
				lua_pushnumber(L, p->dwGuildFrom);
			// stack : table1 table2 guildto
			lua_rawseti(L, -2, 3);

			lua_pushnumber(L, p->lHandicap);
			lua_rawseti(L, -2, 4);

			// stack : table1 table2
			lua_rawseti(L, -2, ++i);
		}

		return 1;
	}

	int32_t guild_get_member_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		
		if (ch == nullptr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		CGuild* pGuild = ch->GetGuild();

		if (pGuild == nullptr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, pGuild->GetMemberCount());

		return 1;
	}

	int32_t guild_change_master(lua_State* L)
	{
		// return value
		// 0 : Invalid name entered (not a string)
		// 1 : Not the guild leader
		// 2 : There is no guild member with the specified name
		// 3 : successful request
		// 4: no guild

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CGuild* pGuild = ch->GetGuild();

		if (pGuild != nullptr)
		{
			if (pGuild->GetMasterPID() == ch->GetPlayerID())
			{
				if (lua_isstring(L, 1) == false)
				{
					lua_pushnumber(L, 0);
				}
				else
				{
					bool ret = pGuild->ChangeMasterTo(pGuild->GetMemberPID(lua_tostring(L, 1)));

					lua_pushnumber(L, ret == false ? 2 : 3);
				}
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}
		else
		{
			lua_pushnumber(L, 4);
		}

		return 1;
	}

	int32_t guild_change_master_with_limit(lua_State* L)
	{
		// factor
		// arg0 : new guild leader name
		// arg1: new guild leader level limit
		// arg2: resign_limit time limit
		// arg3: be_other_leader timeout
		// arg4: be_other_member timeout
		// arg5 : Is it a cash item or not?
		//
		// return value
		// 0 : Invalid name entered (not a string)
		// 1 : Not the guild leader
		// 2 : There is no guild member with the specified name
		// 3 : successful request
		// 4: no guild
		// 5: The specified name is not online
		// 6 : The specified character level is lower than the standard level
		// 7: The new guild leader is limited to be_other_leader

		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		CGuild* pGuild = ch->GetGuild();

		if (pGuild != nullptr)
		{
			if (pGuild->GetMasterPID() == ch->GetPlayerID())
			{
				if (lua_isstring(L, 1) == false)
				{
					lua_pushnumber(L, 0);
				}
				else
				{
					LPCHARACTER pNewMaster = CHARACTER_MANAGER::GetInstance()->FindPC(lua_tostring(L,1));

					if (pNewMaster != nullptr)
					{
						if (pNewMaster->GetLevel() < lua_tonumber(L, 2))
						{
							lua_pushnumber(L, 6);
						}
						else
						{
							int32_t nBeOtherLeader = pNewMaster->GetQuestFlag("change_guild_master.be_other_leader");
							CQuestManager::GetInstance()->GetPC(ch->GetPlayerID());

							if (lua_toboolean(L, 6)) nBeOtherLeader = 0;

							if (nBeOtherLeader > get_global_time())
							{
								lua_pushnumber(L, 7);
							}
							else
							{
								bool ret = pGuild->ChangeMasterTo(pGuild->GetMemberPID(lua_tostring(L, 1)));

								if (ret == false)
								{
									lua_pushnumber(L, 2);
								}
								else
								{
									lua_pushnumber(L, 3);

									pNewMaster->SetQuestFlag("change_guild_master.be_other_leader", 0);
									pNewMaster->SetQuestFlag("change_guild_master.be_other_member", 0);
									pNewMaster->SetQuestFlag("change_guild_master.resign_limit", (int32_t)lua_tonumber(L, 3));

									ch->SetQuestFlag("change_guild_master.be_other_leader", (int32_t)lua_tonumber(L, 4));
									ch->SetQuestFlag("change_guild_master.be_other_member", (int32_t)lua_tonumber(L, 5));
									ch->SetQuestFlag("change_guild_master.resign_limit", 0);
								}
							}
						}
					}
					else
					{
						lua_pushnumber(L, 5);
					}
				}
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}
		else
		{
			lua_pushnumber(L, 4);
		}

		return 1;
	}

	void RegisterGuildFunctionTable()
	{
		luaL_reg guild_functions[] =
		{
			{ "get_rank",				guild_get_rank				},
			{ "get_ladder_point",		guild_get_ladder_point		},
			{ "high_ranking_string",	guild_high_ranking_string	},
			{ "around_ranking_string",	guild_around_ranking_string	},
			{ "name",					guild_name					},
			{ "level",					guild_level					},
			{ "is_war",					guild_is_war				},
			{ "war_enter",				guild_war_enter				},
			{ "get_any_war",			guild_get_any_war			},
			{ "get_reserve_war_table",	guild_get_reserve_war_table	},
			{ "get_name",				guild_get_name				},
			{ "war_bet",				guild_war_bet				},
			{ "is_bet",					guild_is_bet				},
			{ "get_warp_war_list",		guild_get_warp_war_list		},
			{ "get_member_count",		guild_get_member_count		},
			{ "change_master",			guild_change_master			},
			{ "change_master_with_limit",			guild_change_master_with_limit			},

			{ NULL,						NULL						}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("guild", guild_functions);
	}
}

