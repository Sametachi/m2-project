#include "stdafx.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include "questmanager.h"
#include "config.h"
#include "char.h"

// questpc.h: PC::typedef Quest
// questpc.h: PC::typedef map<uint32_t, QuestState> QuestInfo;
// typedef 


namespace quest
{
	NPC::NPC()
	{
		m_vnum = 0;
	}

	NPC::~NPC()
	{
	}

	void NPC::Set(uint32_t vnum, const std::string& script_name)
	{
		m_vnum = vnum;

		char buf[PATH_MAX];

		auto itEventName = CQuestManager::GetInstance()->m_mapEventName.begin();

		while (itEventName != CQuestManager::GetInstance()->m_mapEventName.end())
		{
			auto it = itEventName;
			++itEventName;

			for (const auto& itObjectDir : g_setQuestObjectDir)
			{
				int32_t is = snprintf(buf, sizeof(buf), "%s/%s/%s/", itObjectDir.c_str(), script_name.c_str(), it->first.c_str());

				if (is < 0 || is >= (int32_t)sizeof(buf))
					is = sizeof(buf) - 1;

				int32_t event_index = it->second;

				if (!std::filesystem::exists(buf))
					continue;

				for (const auto& directory_entry : std::filesystem::directory_iterator(buf))
				{
					if (!std::filesystem::is_regular_file(directory_entry))
						continue;

					const auto& stStateFileName = directory_entry.path().filename().generic_string();

					TraceLog("QUEST reading %s", stStateFileName.c_str());
					strlcpy(buf + is, stStateFileName.c_str(), sizeof(buf) - is);
					LoadStateScript(event_index, buf, stStateFileName.c_str());
				}
			}
		}
	}

	void NPC::LoadStateScript(int32_t event_index, const char* filename, const char* script_name)
	{
		std::ifstream inf(filename);
		const std::string s(script_name);

		size_t i = s.find('.');

		auto q = CQuestManager::GetInstance();

		//
		// script_name examples:
		//   christmas_tree.start -> argument not exist
		//
		//   guild_manage.start.0.script -> argument exist
		//   guild_manage.start.0.when
		//   guild_manage.start.0.arg

		///////////////////////////////////////////////////////////////////////////
		// Quest name
		const std::string stQuestName = s.substr(0, i);

		int32_t quest_index = q->GetQuestIndexByName(stQuestName);

		if (quest_index == 0)
		{
			SysLog("cannot find quest index for {}\n", stQuestName.c_str());
			assert(!"cannot find quest index");
			return;
		}

		///////////////////////////////////////////////////////////////////////////
		// State name
		std::string stStateName;

		size_t j = i;
		i = s.find('.', i + 1);

		if (i == s.npos)
			stStateName = s.substr(j + 1, s.npos);
		else
			stStateName = s.substr(j + 1, i - j - 1);

		int32_t state_index = q->GetQuestStateIndex(stQuestName, stStateName);
		///////////////////////////////////////////////////////////////////////////

		PyLog("QUEST loading {} : {} [STATE] {}", 
				filename, stQuestName.c_str(), stStateName.c_str());

		if (i == s.npos)
		{
			// like in example: christmas_tree.start
			std::istreambuf_iterator<char> ib(inf), ie;
			copy(ib, ie, back_inserter(m_mapOwnQuest[event_index][quest_index][q->GetQuestStateIndex(stQuestName, stStateName)].m_code));
		}
		else
		{
			//
			// like in example: guild_manage.start.0.blah
			// NOTE : currently, only CHAT script uses argument
			//

			///////////////////////////////////////////////////////////////////////////
			// ¼ø¼­ Index (There may be several, so the actual index value is not used)
			j = i;
			i = s.find('.', i + 1);

			if (i == s.npos)
			{
				SysLog("invalid QUEST STATE index [{}] [{}]",filename, script_name);
				return;
			}

			const int32_t index = strtol(s.substr(j + 1, i - j - 1).c_str(), NULL, 10); 
			///////////////////////////////////////////////////////////////////////////
			// Type name
			j = i;
			i = s.find('.', i + 1);

			if (i != s.npos)
			{
				SysLog("invalid QUEST STATE name [{}] [{}]",filename, script_name);
				return;
			}

			const std::string type_name = s.substr(j + 1, i - j - 1);
			///////////////////////////////////////////////////////////////////////////

			std::istreambuf_iterator<char> ib(inf), ie;

			m_mapOwnArgQuest[event_index][quest_index][state_index].resize(MAX(index + 1, m_mapOwnArgQuest[event_index][quest_index][state_index].size()));

			if (type_name == "when")
			{
				copy(ib, ie, back_inserter(m_mapOwnArgQuest[event_index][quest_index][state_index][index].when_condition));
			}
			else if (type_name == "arg")
			{
				std::string s;
				getline(inf, s);
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].arg.clear();

				for (std::string::iterator it = s.begin(); it != s.end(); ++it)
				{
					m_mapOwnArgQuest[event_index][quest_index][state_index][index].arg+=*it;
				}
			}
			else if (type_name == "script")
			{
				copy(ib, ie, back_inserter(m_mapOwnArgQuest[event_index][quest_index][state_index][index].script.m_code));
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].quest_index = quest_index;
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].state_index = state_index;
			}
		}
	}

	bool NPC::OnEnterState(PC& pc, uint32_t quest_index, int32_t state)
	{
		return ExecuteEventScript(pc, QUEST_ENTER_STATE_EVENT, quest_index, state);
	}

	bool NPC::OnLeaveState(PC& pc, uint32_t quest_index, int32_t state)
	{
		return ExecuteEventScript(pc, QUEST_LEAVE_STATE_EVENT, quest_index, state);
	}

	bool NPC::OnLetter(PC& pc, uint32_t quest_index, int32_t state)
	{
		return ExecuteEventScript(pc, QUEST_LETTER_EVENT, quest_index, state);
	}

	bool NPC::OnTarget(PC & pc, uint32_t dwQuestIndex, const char* c_pszTargetName, const char* c_pszVerb, bool & bRet)
	{
		TraceLog("OnTarget begin {} verb {} qi {}", c_pszTargetName, c_pszVerb, dwQuestIndex);

		bRet = false;

		PC::QuestInfoIterator itPCQuest = pc.quest_find(dwQuestIndex);

		if (itPCQuest == pc.quest_end())
		{
			TraceLog("no quest");
			return false;
		}

		int32_t iState = itPCQuest->second.st;

		AArgQuestScriptType& r = m_mapOwnArgQuest[QUEST_TARGET_EVENT][dwQuestIndex];
		AArgQuestScriptType::iterator it = r.find(iState);

		if (it == r.end())
		{
			TraceLog("no target event, state {}", iState);
			return false;
		}

		std::vector<AArgScript>::iterator it_vec = it->second.begin();

		int32_t iTargetLen = strlen(c_pszTargetName);

		while (it_vec != it->second.end())
		{
			AArgScript & argScript = *(it_vec++);
			const char* c_pszArg = argScript.arg.c_str();

			TraceLog("OnTarget compare {} {}", c_pszArg, argScript.arg.length());

			if (strncmp(c_pszArg, c_pszTargetName, iTargetLen))
				continue;

			const char* c_pszArgVerb = strchr(c_pszArg, '.');

			if (!c_pszArgVerb)
				continue;

			if (strcmp(++c_pszArgVerb, c_pszVerb))
				continue;

			if (argScript.when_condition.size() > 0)
				TraceLog("OnTarget when {} size {}", &argScript.when_condition[0], argScript.when_condition.size());
	
			if (argScript.when_condition.size() != 0 && !IsScriptTrue(&argScript.when_condition[0], argScript.when_condition.size()))
				continue;

			TraceLog("OnTarget execute qi {} st {} code {}", dwQuestIndex, iState, (const char* ) argScript.script.GetCode());
			bRet = CQuestManager::ExecuteQuestScript(pc, dwQuestIndex, iState, argScript.script.GetCode(), argScript.script.GetSize());
			bRet = true;
			return true;
		}

		return false;
	}

	bool NPC::OnAttrIn(PC& pc)
	{
		return HandleEvent(pc, QUEST_ATTR_IN_EVENT);
	}

	bool NPC::OnAttrOut(PC& pc)
	{
		return HandleEvent(pc, QUEST_ATTR_OUT_EVENT);
	}

	bool NPC::OnTakeItem(PC& pc)
	{
		return HandleEvent(pc, QUEST_ITEM_TAKE_EVENT);
	}

	bool NPC::OnUseItem(PC& pc, bool bReceiveAll)
	{
		if (bReceiveAll)
			return HandleReceiveAllEvent(pc, QUEST_ITEM_USE_EVENT);
		else
			return HandleEvent(pc, QUEST_ITEM_USE_EVENT);
	}

	bool NPC::OnSIGUse(PC& pc, bool bReceiveAll)
	{
		if (bReceiveAll)
			return HandleReceiveAllEvent(pc, QUEST_SIG_USE_EVENT);
		else
			return HandleEvent(pc, QUEST_SIG_USE_EVENT);
	}

	bool NPC::OnClick(PC& pc)
	{
		return HandleEvent(pc, QUEST_CLICK_EVENT);
	}

	bool NPC::OnServerTimer(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_SERVER_TIMER_EVENT);
	}

	bool NPC::OnTimer(PC& pc)
	{
		return HandleEvent(pc, QUEST_TIMER_EVENT);
	}

	bool NPC::OnKill(PC & pc)
	{
		//PROF_UNIT puOnKill("quest::NPC::OnKill");
		if (m_vnum)
		{
			//PROF_UNIT puOnKill1("onk1");
			return HandleEvent(pc, QUEST_KILL_EVENT);
		}
		else
		{
			//PROF_UNIT puOnKill2("onk2");
			return HandleReceiveAllEvent(pc, QUEST_KILL_EVENT);
		}
	}

	bool NPC::OnPartyKill(PC & pc)
	{
		if (m_vnum)
		{
			return HandleEvent(pc, QUEST_PARTY_KILL_EVENT);
		}
		else
		{
			return HandleReceiveAllEvent(pc, QUEST_PARTY_KILL_EVENT);
		}
	}

	bool NPC::OnLevelUp(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_LEVELUP_EVENT);
	}

	bool NPC::OnLogin(PC& pc, const char* c_pszQuestName)
	{
		/*
		   if (c_pszQuestName)
		   {
		   uint32_t dwQI = CQuestManager::GetInstance()->GetQuestIndexByName(c_pszQuestName);

		   if (dwQI)
		   {
		   std::string stQuestName(c_pszQuestName);

		   CQuestManager & q = CQuestManager::instance();

		   QuestMapType::iterator qmit = m_mapOwnQuest[QUEST_LOGIN_EVENT].begin();

		   while (qmit != m_mapOwnQuest[QUEST_LOGIN_EVENT].end())
		   {
		   if (qmit->first != dwQI)
		   {
		   ++qmit;
		   continue;
		   }

		   int32_t iState = pc.GetFlag(stQuestName + "__status");

		   AQuestScriptType::iterator qsit;

		   if ((qsit = qmit->second.find(iState)) != qmit->second.end())
		   {
		   return q.ExecuteQuestScript(pc, stQuestName, iState, qsit->second.GetCode(), qsit->second.GetSize(), NULL, true);
		   }

		   ++qmit;
		   }

		   SysLog("Cannot find any code for {}", c_pszQuestName);
		   }
		   else
		   SysLog("Cannot find quest index by {}", c_pszQuestName);
		   }
		 */
		bool bRet = HandleReceiveAllNoWaitEvent(pc, QUEST_LOGIN_EVENT);
		HandleReceiveAllEvent(pc, QUEST_LETTER_EVENT);
		return bRet;
	}

	bool NPC::OnLogout(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_LOGOUT_EVENT);
	}

	bool NPC::OnUnmount(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_UNMOUNT_EVENT);
	}

	struct FuncMissHandleEvent
	{
		std::vector <uint32_t> vdwNewStartQuestIndices;
		int32_t size;

		FuncMissHandleEvent() : vdwNewStartQuestIndices(0), size(0)
		{}

		bool Matched()
		{
			return vdwNewStartQuestIndices.size() != 0;
		}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			uint32_t dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::GetInstance()->CanStartQuest(dwQuestIndex))
			{
				size++;
				vdwNewStartQuestIndices.push_back(dwQuestIndex);
			}
		}
	};

	struct FuncMatchHandleEvent
	{
		bool bMatched;

		std::vector <uint32_t> vdwQuesIndices;
		std::vector <int32_t> viPCStates;
		std::vector <const char*> vcodes;
		std::vector <int32_t> vcode_sizes;
		int32_t size;

		//uint32_t dwQuestIndex;
		//int32_t iPCState;
		//const char* code;
		//int32_t code_size;

		FuncMatchHandleEvent()
			: bMatched(false), vdwQuesIndices(0), viPCStates(0), vcodes(0), vcode_sizes(0), size(0)
		{}

		bool Matched()
		{
			return bMatched;
		}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			NPC::AQuestScriptType::iterator itQuestScript;

			int32_t iState = itPCQuest->second.st;
			if ((itQuestScript = itQuestMap->second.find(iState)) != itQuestMap->second.end())
			{
				bMatched = true;
				size++;
				vdwQuesIndices.push_back(itQuestMap->first);
				viPCStates.push_back(iState);
				vcodes.push_back(itQuestScript->second.GetCode());
				vcode_sizes.push_back(itQuestScript->second.GetSize());
			}
		}
	};

	bool NPC::HandleEvent(PC& pc, int32_t EventIndex)
	{
		if (EventIndex < 0 || EventIndex >= QUEST_EVENT_COUNT)
		{
			SysLog("QUEST invalid EventIndex : {}", EventIndex);
			return false;
		}

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		FuncMissHandleEvent fMiss;
		FuncMatchHandleEvent fMatch;
		MatchingQuest(pc, m_mapOwnQuest[EventIndex], fMatch, fMiss);

		bool r = false;
		if (fMatch.Matched())
		{
			for (int32_t i = 0; i < fMatch.size; i++)
			{
				if (i != 0) {
					//2012.05.14 <Yongwook Kim>: The m_pCurrentPC of the quest manager is changed.
					// When more than one script is executed, from the second time on, the PC value of the quest manager is newly set.
					PC* pPC = CQuestManager::GetInstance()->GetPC(pc.GetID());		
				}
				
				CQuestManager::ExecuteQuestScript(pc, fMatch.vdwQuesIndices[i], fMatch.viPCStates[i],
					fMatch.vcodes[i], fMatch.vcode_sizes[i]);
			}
			r = true;
		}
		if (fMiss.Matched())
		{
			QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
			
			for (int32_t i = 0; i < fMiss.size; i++)
			{
				AStateScriptType& script = rmapEventOwnQuest[fMiss.vdwNewStartQuestIndices[i]][0];
				CQuestManager::ExecuteQuestScript(pc, fMiss.vdwNewStartQuestIndices[i], 0, script.GetCode(), script.GetSize());
			}
			r = true;
		}
		else
		{
			return r;
		}
		return true;
	}

	struct FuncMissHandleReceiveAllEvent
	{
		bool bHandled;

		FuncMissHandleReceiveAllEvent()
		{
			bHandled = false;
		}

		void operator() (PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			uint32_t dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::GetInstance()->CanStartQuest(dwQuestIndex))
			{
				const NPC::AQuestScriptType & QuestScript = itQuestMap->second;
				auto it = QuestScript.find(QUEST_START_STATE_INDEX);

				if (it != QuestScript.end())
				{
					bHandled = true;
					CQuestManager::ExecuteQuestScript(
							*CQuestManager::GetInstance()->GetCurrentPC(), 
							dwQuestIndex,
							QUEST_START_STATE_INDEX, 
							it->second.GetCode(), 
							it->second.GetSize());
				}
			}
		}
	};

	struct FuncMatchHandleReceiveAllEvent
	{
		bool bHandled;

		FuncMatchHandleReceiveAllEvent()
		{
			bHandled = false;
		}

		void operator() (PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			const NPC::AQuestScriptType& QuestScript = itQuestMap->second;
			int32_t iPCState = itPCQuest->second.st;
			auto itQuestScript = QuestScript.find(iPCState);

			if (itQuestScript != QuestScript.end())
			{
				bHandled = true;

				CQuestManager::ExecuteQuestScript(
						*CQuestManager::GetInstance()->GetCurrentPC(), 
						itQuestMap->first, 
						iPCState, 
						itQuestScript->second.GetCode(), 
						itQuestScript->second.GetSize());
			}
		}
	};

	bool NPC::HandleReceiveAllEvent(PC& pc, int32_t EventIndex)
	{
		if (EventIndex < 0 || EventIndex >= QUEST_EVENT_COUNT)
		{
			SysLog("QUEST invalid EventIndex : {}", EventIndex);
			return false;
		}

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		FuncMissHandleReceiveAllEvent fMiss;
		FuncMatchHandleReceiveAllEvent fMatch;

		MatchingQuest(pc, m_mapOwnQuest[EventIndex], fMatch, fMiss);
		return fMiss.bHandled || fMatch.bHandled;
	}

	struct FuncDoNothing
	{
		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
		}
	};

	struct FuncMissHandleReceiveAllNoWaitEvent
	{
		bool bHandled;

		FuncMissHandleReceiveAllNoWaitEvent()
		{
			bHandled = false;
		}


		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			uint32_t dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::GetInstance()->CanStartQuest(dwQuestIndex))
			{
				const NPC::AQuestScriptType& QuestScript = itQuestMap->second;
				auto it = QuestScript.find(QUEST_START_STATE_INDEX);
				if (it != QuestScript.end())
				{
					bHandled = true;
					PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();
					if (CQuestManager::ExecuteQuestScript(
								*pPC,
								dwQuestIndex,
								QUEST_START_STATE_INDEX, 
								it->second.GetCode(), 
								it->second.GetSize()))
					{
						SysLog("QUEST NOT END RUNNING on Login/Logout - {}", 
								CQuestManager::GetInstance()->GetQuestNameByIndex(itQuestMap->first).c_str());

						QuestState& rqs =* pPC->GetRunningQuestState();
						CQuestManager::GetInstance()->CloseState(rqs);
						pPC->EndRunning();
					}
				}
			}
		}
	};

	struct FuncMatchHandleReceiveAllNoWaitEvent
	{
		bool bHandled;

		FuncMatchHandleReceiveAllNoWaitEvent()
		{
			bHandled = false;
		}

		void operator()(PC::QuestInfoIterator & itPCQuest, NPC::QuestMapType::iterator & itQuestMap)
		{
			const NPC::AQuestScriptType & QuestScript = itQuestMap->second;
			int32_t iPCState = itPCQuest->second.st;
			auto itQuestScript = QuestScript.find(iPCState);

			if (itQuestScript != QuestScript.end())
			{
				PC* pPC = CQuestManager::GetInstance()->GetCurrentPC();

				if (CQuestManager::ExecuteQuestScript(
							*pPC,
							itQuestMap->first, 
							iPCState, 
							itQuestScript->second.GetCode(), 
							itQuestScript->second.GetSize()))
				{
					SysLog("QUEST NOT END RUNNING on Login/Logout - {}", 
							CQuestManager::GetInstance()->GetQuestNameByIndex(itQuestMap->first).c_str());

					QuestState& rqs =* pPC->GetRunningQuestState();
					CQuestManager::GetInstance()->CloseState(rqs);
					pPC->EndRunning();
				}
				bHandled = true;
			}
		}
	};

	bool NPC::HandleReceiveAllNoWaitEvent(PC& pc, int32_t EventIndex)
	{
		//cerr << EventIndex << endl;
		if (EventIndex<0 || EventIndex>=QUEST_EVENT_COUNT)
		{
			SysLog("QUEST invalid EventIndex : {}", EventIndex);
			return false;
		}

		/*
		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}
		*/

		//FuncDoNothing fMiss;
		FuncMissHandleReceiveAllNoWaitEvent fMiss;
		FuncMatchHandleReceiveAllNoWaitEvent fMatch;

		QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		MatchingQuest(pc, rmapEventOwnQuest, fMatch, fMiss);

		return fMatch.bHandled || fMiss.bHandled;
	}

	bool NPC::OnInfo(PC & pc, uint32_t quest_index)
	{
		const int32_t EventIndex = QUEST_INFO_EVENT;

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		PC::QuestInfoIterator itPCQuest = pc.quest_find(quest_index);

		if (pc.quest_end() == itPCQuest)
		{
			SysLog("QUEST no quest by (quest {})", quest_index);
			return false;
		}

		QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		QuestMapType::iterator itQuestMap = rmapEventOwnQuest.find(quest_index);

		const char* questName = CQuestManager::GetInstance()->GetQuestNameByIndex(quest_index).c_str();

		if (itQuestMap == rmapEventOwnQuest.end())
		{
			SysLog("QUEST no info event (quest {})", questName);
			return false;
		}

		AQuestScriptType::iterator itQuestScript = itQuestMap->second.find(itPCQuest->second.st);

		if (itQuestScript == itQuestMap->second.end())
		{
			SysLog("QUEST no info script by state {} (quest {})", itPCQuest->second.st, questName);
			return false;
		}

		CQuestManager::ExecuteQuestScript(pc, quest_index, itPCQuest->second.st, itQuestScript->second.GetCode(), itQuestScript->second.GetSize());
		return true;
	}

	bool NPC::OnButton(PC & pc, uint32_t quest_index)
	{
		const int32_t EventIndex = QUEST_BUTTON_EVENT;

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		PC::QuestInfoIterator itPCQuest = pc.quest_find(quest_index);

		QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		QuestMapType::iterator itQuestMap = rmapEventOwnQuest.find(quest_index);

		if (itQuestMap == rmapEventOwnQuest.end())
			return false;

		int32_t iState = 0;

		if (itPCQuest != pc.quest_end())
		{
			iState = itPCQuest->second.st;
		}
		else
		{
			if (CQuestManager::GetInstance()->CanStartQuest(itQuestMap->first, pc) && HasStartState(itQuestMap->second))
				iState = 0;
			else
				return false;
		}

		AQuestScriptType::iterator itQuestScript=itQuestMap->second.find(iState);

		if (itQuestScript==itQuestMap->second.end())
			return false;

		CQuestManager::ExecuteQuestScript(pc, quest_index, iState, itQuestScript->second.GetCode(), itQuestScript->second.GetSize());
		return true;
	}

	struct FuncMissChatEvent
	{
		FuncMissChatEvent(std::vector<AArgScript*>& rAvailScript)
			: rAvailScript(rAvailScript)
			{}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::ArgQuestMapType::iterator& itQuestMap)
		{
			if (CQuestManager::GetInstance()->CanStartQuest(itQuestMap->first) && NPC::HasStartState(itQuestMap->second))
			{
				size_t i;
				for (i = 0; i < itQuestMap->second[QUEST_START_STATE_INDEX].size(); ++i)
				{
					if (itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition.size() == 0 || 
							IsScriptTrue(&itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition[0], itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition.size()))
						rAvailScript.push_back(&itQuestMap->second[QUEST_START_STATE_INDEX][i]);
				}
			}
		}

		std::vector<AArgScript*>& rAvailScript;
	};

	struct FuncMatchChatEvent
	{
		FuncMatchChatEvent(std::vector<AArgScript*>& rAvailScript)
			: rAvailScript(rAvailScript)
			{}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::ArgQuestMapType::iterator& itQuestMap)
		{
			int32_t iState = itPCQuest->second.st;
			std::map<int32_t, std::vector<AArgScript> >::iterator itQuestScript = itQuestMap->second.find(iState);
			if (itQuestScript != itQuestMap->second.end())
			{
				size_t i;
				for (i = 0; i < itQuestMap->second[iState].size(); i++)
				{
					if (itQuestMap->second[iState][i].when_condition.size() == 0 ||
							IsScriptTrue(&itQuestMap->second[iState][i].when_condition[0], itQuestMap->second[iState][i].when_condition.size()))
						rAvailScript.push_back(&itQuestMap->second[iState][i]);
				}
			}
		}

		std::vector<AArgScript*>& rAvailScript;
	};

	bool NPC::OnChat(PC& pc)
	{
		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				auto mgr = CQuestManager::GetInstance();

				SysLog("QUEST There's suspended quest state, can't run new quest state (quest: {} pc: {})",
						pc.GetCurrentQuestName().c_str(),
						mgr->GetCurrentCharacterPtr() ? mgr->GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		const int32_t EventIndex = QUEST_CHAT_EVENT;
		std::vector<AArgScript*> AvailScript;

		FuncMatchChatEvent fMatch(AvailScript);
		FuncMissChatEvent fMiss(AvailScript);
		MatchingQuest(pc, m_mapOwnArgQuest[EventIndex], fMatch, fMiss);


		if (AvailScript.empty())
			return false;

		{

			std::ostringstream os;
			os << "select(";
			os << '"' << ScriptToString(AvailScript[0]->arg.c_str()) << '"';
			for (size_t i = 1; i < AvailScript.size(); i++)
			{
				os << ",\"" << ScriptToString(AvailScript[i]->arg.c_str()) << '"';
			}
			os << ", '"<<LC_TEXT("Close")<<"'";
			os << ")";

			CQuestManager::ExecuteQuestScript(pc, "QUEST_CHAT_TEMP_QUEST", 0, os.str().c_str(), os.str().size(), &AvailScript, false);
		}

		return true;
	}

	bool NPC::HasChat()
	{
		return !m_mapOwnArgQuest[QUEST_CHAT_EVENT].empty();
	}

	bool NPC::ExecuteEventScript(PC& pc, int32_t EventIndex, uint32_t dwQuestIndex, int32_t iState)
	{
		QuestMapType& rQuest = m_mapOwnQuest[EventIndex];

		auto itQuest = rQuest.find(dwQuestIndex);
		if (itQuest == rQuest.end())
		{
			PyLog("ExecuteEventScript ei {} qi {} is {} - NO QUEST", EventIndex, dwQuestIndex, iState);
			return false;
		}

		AQuestScriptType& rScript = itQuest->second;
		auto itState = rScript.find(iState);
		if (itState == rScript.end())
		{
			PyLog("ExecuteEventScript ei {} qi {} is {} - NO STATE", EventIndex, dwQuestIndex, iState);
			return false;
		}

		PyLog("ExecuteEventScript ei {} qi {} is {}", EventIndex, dwQuestIndex, iState);
		CQuestManager::GetInstance()->SetCurrentEventIndex(EventIndex);
		return CQuestManager::ExecuteQuestScript(pc, dwQuestIndex, iState, itState->second.GetCode(), itState->second.GetSize());
	}

	bool NPC::OnPickupItem(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_ITEM_PICK_EVENT);
		else
			return HandleEvent(pc, QUEST_ITEM_PICK_EVENT);
	}
	bool NPC::OnItemInformer(PC& pc, uint32_t vnum)
	{
		return HandleEvent(pc, QUEST_ITEM_INFORMER_EVENT);
	}
}
