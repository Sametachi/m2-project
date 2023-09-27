#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "party.h"
#include "char.h"
#include "questlua.h"
#include "questmanager.h"
#include "packet.h"

namespace quest
{
	//
	// "party" Lua functions
	//
	int32_t party_clear_ready(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		
		if (ch->GetParty())
		{
			FPartyClearReady f;
			ch->GetParty()->ForEachNearMember(f);
		}
		return 0;
	}

	int32_t party_get_max_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty())
			lua_pushnumber(L,ch->GetParty()->GetMemberMaxLevel());
		else
			lua_pushnumber(L, 1);

		return 1;
	}

    struct FRunCinematicSender
    {
        std::string data;
        struct packet_script pack;

        FRunCinematicSender(const char* str)
        {
            data = "[RUN_CINEMA value;";
            data += str;
            data += "]";

            pack.header = HEADER_GC_SCRIPT;
            pack.skin = CQuestManager::QUEST_SKIN_CINEMATIC;
            //pack.skin = CQuestManager::QUEST_SKIN_NOWINDOW;
            pack.src_size = data.size();
            pack.size = pack.src_size + sizeof(struct packet_script);
        }

        void operator()(LPCHARACTER ch)
        {
            PyLog("CINEMASEND_TRY {}", ch->GetName());

            if (ch->GetDesc())
            {
                PyLog("CINEMASEND {}", ch->GetName());
                ch->GetDesc()->BufferedPacket(&pack, sizeof(struct packet_script));
                ch->GetDesc()->Packet(data.c_str(),data.size());
            }
        }
    };

	int32_t party_run_cinematic(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;
		
		PyLog("RUN_CINEMA {}", lua_tostring(L, 1));
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty())
		{
			FRunCinematicSender f(lua_tostring(L, 1));

			ch->GetParty()->Update();
			ch->GetParty()->ForEachNearMember(f);
		}

		return 0;
	}

	struct FCinematicSender
	{
		const char* str;
		struct ::packet_script packet_script;
		int32_t len;

		FCinematicSender(const char* str)
			: str(str)
		{
			len = strlen(str);

			packet_script.header = HEADER_GC_SCRIPT;
			packet_script.skin = CQuestManager::QUEST_SKIN_CINEMATIC;
			packet_script.src_size = len;
			packet_script.size = packet_script.src_size + sizeof(struct packet_script);
		}

		void operator()(LPCHARACTER ch)
		{
			PyLog("CINEMASEND_TRY {}", ch->GetName());

			if (ch->GetDesc())
			{
				PyLog("CINEMASEND {}", ch->GetName());
				ch->GetDesc()->BufferedPacket(&packet_script, sizeof(struct packet_script));
				ch->GetDesc()->Packet(str,len);
			}
		}
	};

	int32_t party_show_cinematic(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		PyLog("CINEMA {}", lua_tostring(L, 1));
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty())
		{
			FCinematicSender f(lua_tostring(L, 1));

			ch->GetParty()->Update();
			ch->GetParty()->ForEachNearMember(f);
		}
		return 0;
	}

	int32_t party_get_near_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty())
			lua_pushnumber(L, ch->GetParty()->GetNearMemberCount());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int32_t party_syschat(lua_State* L)
	{
		LPPARTY pParty = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetParty();

		if (pParty)
		{
			std::ostringstream s;
			combine_lua_string(L, s);

			FPartyChat f(CHAT_TYPE_INFO, s.str().c_str());

			pParty->ForEachOnlineMember(f);
		}

		return 0;
	}

	int32_t party_is_leader(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
			lua_pushboolean(L, 1);
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int32_t party_is_party(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetParty() ? 1 : 0);
		return 1;
	}

	int32_t party_get_leader_pid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();
		if (ch->GetParty())
		{
			lua_pushnumber(L, ch->GetParty()->GetLeaderPID());
		}
		else
		{
			lua_pushnumber(L, -1);
		}
		return 1;
	}


	int32_t party_chat(lua_State* L)
	{
		LPPARTY pParty = CQuestManager::GetInstance()->GetCurrentCharacterPtr()->GetParty();

		if (pParty)
		{
			std::ostringstream s;
			combine_lua_string(L, s);

			FPartyChat f(CHAT_TYPE_TALKING, s.str().c_str());

			pParty->ForEachOnlineMember(f);
		}

		return 0;
	}


	int32_t party_is_map_member_flag_lt(lua_State* L)
	{

		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		auto q = CQuestManager::GetInstance();
		LPPARTY pParty = q->GetCurrentCharacterPtr()->GetParty();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		PC* pPC = q->GetCurrentPC();

		const char* sz = lua_tostring(L,1);

		if (pParty)
		{
			FPartyCheckFlagLt f;
			f.flagname = pPC->GetCurrentQuestName() + "."+sz;
			f.value = (int32_t) rint(lua_tonumber(L, 2));

			bool returnBool = pParty->ForEachOnMapMemberBool(f, ch->GetMapIndex());
			lua_pushboolean(L, returnBool);
		}

		return 1;
	}

	int32_t party_set_flag(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (ch->GetParty() && lua_isstring(L, 1) && lua_isnumber(L, 2))
			ch->GetParty()->SetFlag(lua_tostring(L, 1), (int32_t)lua_tonumber(L, 2));

		return 0;
	}

	int32_t party_get_flag(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::GetInstance()->GetCurrentCharacterPtr();

		if (!ch->GetParty() || !lua_isstring(L, 1))
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, ch->GetParty()->GetFlag(lua_tostring(L, 1)));

		return 1;
	}

	int32_t party_set_quest_flag(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		FSetQuestFlag f;

		f.flagname = q->GetCurrentPC()->GetCurrentQuestName() + "." + lua_tostring(L, 1);
		f.value = (int32_t) rint(lua_tonumber(L, 2));

		LPCHARACTER ch = q->GetCurrentCharacterPtr();

		if (ch->GetParty())
			ch->GetParty()->ForEachOnlineMember(f);
		else
			f(ch);

		return 0;
	}

	int32_t party_is_in_dungeon (lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		LPPARTY pParty = ch->GetParty();
		if (pParty != nullptr){
			lua_pushboolean (L, pParty->GetDungeon() ? true : false);
			return 1;
		}
		lua_pushboolean (L, false);
		return 1;
	}

	struct FGiveBuff
	{
		uint32_t dwType;
		uint8_t bApplyOn;
		int32_t lApplyValue;
		uint32_t dwFlag;
		int32_t lDuration;
		int32_t lSPCost;
		bool bOverride;
		bool IsCube;

		FGiveBuff (uint32_t _dwType, uint8_t _bApplyOn, int32_t _lApplyValue, uint32_t _dwFlag, int32_t _lDuration, 
					int32_t _lSPCost, bool _bOverride, bool _IsCube = false)
			: dwType (_dwType), bApplyOn (_bApplyOn), lApplyValue (_lApplyValue), dwFlag(_dwFlag), lDuration(_lDuration),
				lSPCost(_lSPCost), bOverride(_bOverride), IsCube(_IsCube)
		{}
		void operator () (LPCHARACTER ch)
		{
			ch->AddAffect(dwType, bApplyOn, lApplyValue, dwFlag, lDuration, lSPCost, bOverride, IsCube);
		}
	};
	
	int32_t party_give_buff (lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || 
			!lua_isnumber(L, 5) || !lua_isnumber(L, 6) || !lua_isboolean(L, 7) || !lua_isboolean(L, 8))
		{
			lua_pushboolean (L, false);
			return 1;
		}
		uint32_t dwType = lua_tonumber(L, 1);
		uint8_t bApplyOn = lua_tonumber(L, 2);
		int32_t lApplyValue = lua_tonumber(L, 3);
		uint32_t dwFlag = lua_tonumber(L, 4);
		int32_t lDuration = lua_tonumber(L, 5);
		int32_t lSPCost = lua_tonumber(L, 6);
		bool bOverride = lua_toboolean(L, 7);
		bool IsCube = lua_toboolean(L, 8);

		FGiveBuff f (dwType, bApplyOn, lApplyValue, dwFlag, lDuration, lSPCost, bOverride, IsCube);
		if (ch->GetParty())
			ch->GetParty()->ForEachOnMapMember(f, ch->GetMapIndex());
		else
			f(ch);

		lua_pushboolean (L, true);
		return 1;
	}

	struct FPartyPIDCollector
	{
		std::vector <uint32_t> vecPIDs;
		FPartyPIDCollector()
		{
		}
		void operator () (LPCHARACTER ch)
		{
			vecPIDs.push_back(ch->GetPlayerID());
		}
	};
	int32_t party_get_member_pids(lua_State *L)
	{
		auto q = CQuestManager::GetInstance();
		LPCHARACTER ch = q->GetCurrentCharacterPtr();
		LPPARTY pParty = ch->GetParty();
		if (!pParty)
		{
			return 0;
		}
		FPartyPIDCollector f;
		pParty->ForEachOnMapMember(f, ch->GetMapIndex());
		
		for (std::vector <uint32_t>::iterator it = f.vecPIDs.begin(); it != f.vecPIDs.end(); it++)
		{
			lua_pushnumber(L, *it);
		}
		return f.vecPIDs.size();
	}

	void RegisterPartyFunctionTable()
	{
		luaL_reg party_functions[] = 
		{
			{ "is_leader",		party_is_leader		},
			{ "is_party",		party_is_party		},
			{ "get_leader_pid",	party_get_leader_pid},
			{ "setf",			party_set_flag		},
			{ "getf",			party_get_flag		},
			{ "setqf",			party_set_quest_flag},
			{ "chat",			party_chat			},
			{ "syschat",		party_syschat		},
			{ "get_near_count",	party_get_near_count},
			{ "show_cinematic",	party_show_cinematic},
			{ "run_cinematic",	party_run_cinematic	},
			{ "get_max_level",	party_get_max_level	},
			{ "clear_ready",	party_clear_ready	},
			{ "is_in_dungeon",	party_is_in_dungeon	},
			{ "give_buff",		party_give_buff		},
			{ "is_map_member_flag_lt",	party_is_map_member_flag_lt	},
			{ "get_member_pids",		party_get_member_pids	},
			{ NULL,				NULL				}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("party", party_functions);
	}
}




