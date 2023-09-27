#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"

namespace quest
{
	//
	// "quest" Lua functions
	//
	int32_t quest_start(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestStartFlag();
		q->GetCurrentPC()->SetCurrentQuestStartFlag();
		return 0;
	}

	int32_t quest_done(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		q->GetCurrentPC()->SetCurrentQuestDoneFlag();
		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestDoneFlag();
		return 0;
	}

	int32_t quest_set_title(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestTitle(lua_tostring(L,-1));
		if (lua_isstring(L,-1))
			q->GetCurrentPC()->SetCurrentQuestTitle(lua_tostring(L,-1));

		return 0;
	}

	int32_t quest_set_another_title(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		if (lua_isstring(L,1) && lua_isstring(L,2))
			q->GetCurrentPC()->SetQuestTitle(lua_tostring(L,1),lua_tostring(L,2));

		return 0;
	}

	int32_t quest_set_clock_name(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestClockName(lua_tostring(L,-1));
		if (lua_isstring(L,-1))
			q->GetCurrentPC()->SetCurrentQuestClockName(lua_tostring(L,-1));

		return 0;
	}

	int32_t quest_set_clock_value(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestClockValue((int32_t)rint(lua_tonumber(L,-1)));
		if (lua_isnumber(L,-1))
			q->GetCurrentPC()->SetCurrentQuestClockValue((int32_t)rint(lua_tonumber(L,-1)));

		return 0;
	}

	int32_t quest_set_counter_name(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestCounterName(lua_tostring(L,-1));
		if (lua_isstring(L,-1))
			q->GetCurrentPC()->SetCurrentQuestCounterName(lua_tostring(L,-1));

		return 0;
	}

	int32_t quest_set_counter_value(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestCounterValue((int32_t)rint(lua_tonumber(L,-1)));
		if (lua_isnumber(L,-1))
			q->GetCurrentPC()->SetCurrentQuestCounterValue((int32_t)rint(lua_tonumber(L,-1)));

		return 0;
	}

	int32_t quest_set_icon_file(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();

		//q->GetPC(q->GetCurrentCharacterPtr()->GetPlayerID())->SetCurrentQuestCounterValue((int32_t)rint(lua_tonumber(L,-1)));
		if (lua_isstring(L,-1))
			q->GetCurrentPC()->SetCurrentQuestIconFile(lua_tostring(L,-1));

		return 0;
	}

	int32_t quest_setstate(lua_State* L)
	{
		if (lua_tostring(L, -1)==NULL)
		{
			SysLog("state name is empty");
			return 0;
		}

		auto q = CQuestManager::GetInstance();
		QuestState* pqs = q->GetCurrentState();
		PC* pPC = q->GetCurrentPC();
		//assert(L == pqs->co);

		if (L!=pqs->co) 
		{
			luaL_error(L, "running thread != current thread???");
			if (test_server)
				PyLog("running thread != current thread???");
			return 0;
		}

		if (pPC)
		{
			//pqs->st = lua_tostring(L, -1);
			//cerr << "QUEST new state" << pPC->GetCurrentQuestName(); << ":"
			//cerr <<  lua_tostring(L,-1);
			//cerr << endl;
			//
			std::string stCurrentState = lua_tostring(L,-1);
			if (test_server)
				PyLog("questlua->setstate({}, {})", pPC->GetCurrentQuestName().c_str(), stCurrentState.c_str());
			pqs->st = q->GetQuestStateIndex(pPC->GetCurrentQuestName(), stCurrentState);
			pPC->SetCurrentQuestStateName(stCurrentState);
		}
		return 0;
	}

	int32_t quest_coroutine_yield(lua_State * L)
	{
		auto q = CQuestManager::GetInstance();
		if (q->IsInOtherPCBlock())
		{
			SysLog("FATAL ERROR! Yield occur in other_pc_block.");
			PC* pPC = q->GetOtherPCBlockRootPC();
			if (!pPC)
			{
				SysLog("	... FFFAAATTTAAALLL Error. RootPC is NULL");
				return 0;
			}
			QuestState* pQS = pPC->GetRunningQuestState();
			if (!pQS || !q->GetQuestStateName(pPC->GetCurrentQuestName(), pQS->st))
			{
				SysLog("	... WHO AM I? WHERE AM I? I only know QuestName({})...", pPC->GetCurrentQuestName().c_str());
			}
			else
			{
				SysLog("	Current Quest({}). State({})", pPC->GetCurrentQuestName().c_str(), q->GetQuestStateName(pPC->GetCurrentQuestName(), pQS->st));
			}
			return 0;
		}
		return lua_yield(L, lua_gettop(L));
	}

	int32_t quest_no_send(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		q->SetNoSend();
		return 0;
	}

	int32_t quest_get_current_quest_index(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		PC* pPC = q->GetCurrentPC();

		int32_t idx = q->GetQuestIndexByName(pPC->GetCurrentQuestName());
		lua_pushnumber(L, idx);
		return 1;
	}

	int32_t quest_begin_other_pc_block(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		uint32_t pid = lua_tonumber(L, -1);
		q->BeginOtherPCBlock(pid);
		return 0;
	}

	int32_t quest_end_other_pc_block(lua_State* L)
	{
		auto q = CQuestManager::GetInstance();
		q->EndOtherPCBlock();
		return 0;
	}

	void RegisterQuestFunctionTable()
	{
		luaL_reg quest_functions[] = 
		{
			{ "setstate",				quest_setstate				},
			{ "set_state",				quest_setstate				},
			{ "yield",					quest_coroutine_yield		},
			{ "set_title",				quest_set_title				},
			{ "set_title2",				quest_set_another_title		},
			{ "set_clock_name",			quest_set_clock_name		},
			{ "set_clock_value",		quest_set_clock_value		},
			{ "set_counter_name",		quest_set_counter_name		},
			{ "set_counter_value",		quest_set_counter_value		},
			{ "set_icon",				quest_set_icon_file			},
			{ "start",					quest_start					},
			{ "done",					quest_done					},
			{ "getcurrentquestindex",	quest_get_current_quest_index	},
			{ "no_send",				quest_no_send				},
			// Let other_pc_block be between begin_other_pc_block(pid) and end_other_pc_block.
			// In other_pc_block, current_pc is changed to pid.
			// When finished, it goes back to the original current_pc.
			/*		Made for something like this.
					for i, pid in next, pids, nil do
						q->begin_other_pc_block(pid)
						if pc.count_item(PASS_TICKET) < 1 then
							table.insert(criminalNames, pc.get_name())
							canPass = false
						end
						q->end_other_pc_block()
					end
			*/
			// Caution: Yield must never occur inside other_pc_block (ex. wait, select, input, ...)
			{ "begin_other_pc_block",	quest_begin_other_pc_block	}, 
			{ "end_other_pc_block",		quest_end_other_pc_block	},
			{ NULL,						NULL						}
		};

		CQuestManager::GetInstance()->AddLuaFunctionTable("q", quest_functions);
	}
}




