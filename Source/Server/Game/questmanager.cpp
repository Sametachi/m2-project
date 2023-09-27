#include "stdafx.h"
#include <fstream>
#include <filesystem>
#include "constants.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "lzo_manager.h"
#include "item.h"
#include "config.h"
#include "xmas_event.h"
#include "target.h"
#include "party.h"
#include "locale_service.h"
#include "dungeon.h"

uint32_t g_GoldDropTimeLimitValue = 0;
extern bool DropEvent_CharStone_SetValue(const std::string& name, int32_t value);
extern bool DropEvent_RefineBox_SetValue (const std::string& name, int32_t value);

namespace quest
{
	CQuestManager::CQuestManager()
		: m_pSelectedDungeon(nullptr), m_dwServerTimerArg(0), m_iRunningEventIndex(0), L(nullptr), m_bNoSend (false),
		m_CurrentRunningState(nullptr), m_pCurrentCharacter(nullptr), m_pCurrentNPCCharacter(nullptr), m_pCurrentPartyMember(nullptr),
		m_pCurrentPC(nullptr),  m_iCurrentSkin(0), m_bError(false), m_pOtherPCBlockRootPC(nullptr)
	{
	}

	CQuestManager::~CQuestManager()
	{
		Destroy();
	}

	void CQuestManager::Destroy()
	{
		if (L)
		{
			lua_close(L);
			L = nullptr;
		}
	}	

	bool CQuestManager::Initialize()
	{
		if (g_bAuthServer)
			return true;

		if (!InitializeLua())
			return false;

		m_pSelectedDungeon = nullptr;

		m_mapEventName.insert(TEventNameMap::value_type("click", QUEST_CLICK_EVENT));		
		m_mapEventName.insert(TEventNameMap::value_type("kill", QUEST_KILL_EVENT));		
		m_mapEventName.insert(TEventNameMap::value_type("timer", QUEST_TIMER_EVENT));	
		m_mapEventName.insert(TEventNameMap::value_type("levelup", QUEST_LEVELUP_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("login", QUEST_LOGIN_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("logout", QUEST_LOGOUT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("button", QUEST_BUTTON_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("info", QUEST_INFO_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("chat", QUEST_CHAT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("in", QUEST_ATTR_IN_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("out", QUEST_ATTR_OUT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("use", QUEST_ITEM_USE_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("server_timer", QUEST_SERVER_TIMER_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("enter", QUEST_ENTER_STATE_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("leave", QUEST_LEAVE_STATE_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("letter", QUEST_LETTER_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("take", QUEST_ITEM_TAKE_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("target", QUEST_TARGET_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("party_kill", QUEST_PARTY_KILL_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("unmount", QUEST_UNMOUNT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("pick", QUEST_ITEM_PICK_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("sig_use", QUEST_SIG_USE_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("item_informer", QUEST_ITEM_INFORMER_EVENT));

		m_bNoSend = false;

		m_iCurrentSkin = QUEST_SKIN_NORMAL;

		{
			std::ifstream inf((g_stQuestDir + "/questnpc.txt").c_str());
			int32_t line = 0;

			if (!inf.is_open())
			{
				SysLog("QUEST Cannot open 'questnpc.txt'");
			}
			else
				PyLog("QUEST can open 'questnpc.txt' ({})", g_stQuestDir.c_str());

			while (1)
			{
				uint32_t vnum;

				inf >> vnum;

				line++;

				if (inf.fail())
					break;

				std::string s;
				getline(inf, s);
				uint32_t li = 0, ri = s.size()-1;
				while (li < s.size() && isspace(s[li])) li++;
				while (ri > 0 && isspace(s[ri])) ri--;

				if (ri < li) 
				{
					SysLog("QUEST questnpc.txt:{}:npc name error",line);
					continue;
				}

				s = s.substr(li, ri-li+1);

				int32_t	n = 0;
				str_to_number(n, s.c_str());
				if (n)
					continue;

				//cout << '-' << s << '-' << endl;
				if (test_server)
					PyLog("QUEST reading script of {}({})", s.c_str(), vnum);
				m_mapNPC[vnum].Set(vnum, s);
				m_mapNPCNameID[s] = vnum;
			}

			// notarget quest
			m_mapNPC[0].Set(0, "notarget");
		}

		SetEventFlag("guild_withdraw_delay", 1);
		SetEventFlag("guild_disband_delay", 1);
		return true;
	}

	uint32_t CQuestManager::FindNPCIDByName(const std::string& name)
	{
		std::map<std::string, uint32_t>::iterator it = m_mapNPCNameID.find(name);
		return it != m_mapNPCNameID.end() ? it->second : 0;
	}

	void CQuestManager::SelectItem(uint32_t pc, uint32_t selection)
	{
		PC* pPC = GetPC(pc);
		if (pPC && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_SELECT_ITEM)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args=1;
			lua_pushnumber(pPC->GetRunningQuestState()->co,selection);

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
	}

	void CQuestManager::Confirm(uint32_t pc, EQuestConfirmType confirm, uint32_t pc2)
	{
		PC* pPC = GetPC(pc);

		if (!pPC->IsRunning())
		{
			SysLog("no quest running for pc, cannot process input : {}", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_CONFIRM)
		{
			SysLog("not wait for a confirm : {} {}", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		if (pc2 && !pPC->IsConfirmWait(pc2))
		{
			SysLog("not wait for a confirm : {} {}", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->ClearConfirmWait();

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushnumber(pPC->GetRunningQuestState()->co, confirm);

		AddScript("[END_CONFIRM_WAIT]");
		SetSkinStyle(QUEST_SKIN_NOWINDOW);
		SendScript();

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}

	}

	void CQuestManager::Input(uint32_t pc, const char* msg)
	{
		PC* pPC = GetPC(pc);
		if (!pPC)
		{
			SysLog("no pc! : {}",pc);
			return;
		}

		if (!pPC->IsRunning())
		{
			SysLog("no quest running for pc, cannot process input : {}", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_INPUT)
		{
			SysLog("not wait for a input : {} {}", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushstring(pPC->GetRunningQuestState()->co,msg);

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}
	}

	void CQuestManager::Select(uint32_t pc, uint32_t selection)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state==SUSPEND_STATE_SELECT)
		{
			pPC->SetSendDoneFlag();

			if (!pPC->GetRunningQuestState()->chat_scripts.empty())
			{
				// In case of chat event
				// Since the current quest is a quest to select which quest to execute
				// Finish and execute the selected quest.
				QuestState& old_qs =* pPC->GetRunningQuestState();
				CloseState(old_qs);

				if (selection >= pPC->GetRunningQuestState()->chat_scripts.size())
				{
					pPC->SetSendDoneFlag();
					GotoEndState(old_qs);
					pPC->EndRunning();
				}
				else
				{
					AArgScript* pas = pPC->GetRunningQuestState()->chat_scripts[selection];
					ExecuteQuestScript(*pPC, pas->quest_index, pas->state_index, pas->script.GetCode(), pas->script.GetSize());
				}
			}
			else
			{
				// on default 
				pPC->GetRunningQuestState()->args=1;
				lua_pushnumber(pPC->GetRunningQuestState()->co,selection+1);

				if (!RunState(*pPC->GetRunningQuestState()))
				{
					CloseState(*pPC->GetRunningQuestState());
					pPC->EndRunning();
				}
			}
		}
		else
		{
			SysLog("wrong QUEST_SELECT request! : {}",pc);
		}
	}

	void CQuestManager::Resume(uint32_t pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_PAUSE)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args = 0;

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
		else
		{
			//cerr << pPC << endl;
			//cerr << pPC->IsRunning() << endl;
			//cerr << pPC->GetRunningQuestState()->suspend_state;
			//cerr << SUSPEND_STATE_WAIT << endl;
			//cerr << "wrong QUEST_WAIT request! : " << pc << endl;
			SysLog("wrong QUEST_WAIT request! : {}",pc);
		}
	}

	void CQuestManager::EnterState(uint32_t pc, uint32_t quest_index, int32_t state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnEnterState(*pPC, quest_index, state);
		}
		else
			SysLog("QUEST no such pc id : {}", pc);
	}

	void CQuestManager::LeaveState(uint32_t pc, uint32_t quest_index, int32_t state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLeaveState(*pPC, quest_index, state);
		}
		else
			SysLog("QUEST no such pc id : {}", pc);
	}

	void CQuestManager::Letter(uint32_t pc, uint32_t quest_index, int32_t state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLetter(*pPC, quest_index, state);
		}
		else
			SysLog("QUEST no such pc id : {}", pc);
	}

	void CQuestManager::LogoutPC(LPCHARACTER ch)
	{
		PC* pPC = GetPC(ch->GetPlayerID());

		if (pPC && pPC->IsRunning())
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->CancelRunning();
		}

		Logout(ch->GetPlayerID());

		if (ch == m_pCurrentCharacter)
		{
			m_pCurrentCharacter = nullptr;
			m_pCurrentPC = nullptr;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//
	// Quest Event
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::Login(uint32_t pc, const char* c_pszQuest)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogin(*pPC, c_pszQuest);
		}
		else
		{
			SysLog("QUEST no such pc id : {}", pc);
		}
	}

	void CQuestManager::Logout(uint32_t pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLogout(*pPC);
		}
		else
			SysLog("QUEST no such pc id : {}", pc);
	}

	void CQuestManager::Kill(uint32_t pc, uint32_t npc)
	{
		//m_CurrentNPCRace = npc;
		PC* pPC;

		PyLog("CQuestManager::Kill QUEST_KILL_EVENT (pc={}, npc={})", pc, npc);

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			/* [hyo] Fixed an issue with duplicate counting when killing a mob
			Even if the script is processed due to the code such as when 171.kill begin ... in the quest script
			Changed to perform another check instead of returning immediately. (2011/07/21)
			*/
			// call script
			m_mapNPC[npc].OnKill(*pPC);

			LPCHARACTER ch = GetCurrentCharacterPtr();
			LPPARTY pParty = ch->GetParty();
			LPCHARACTER leader = pParty ? pParty->GetLeaderCharacter() : ch;

			if (leader)
			{
				m_pCurrentPartyMember = ch;

				if (m_mapNPC[npc].OnPartyKill(*GetPC(leader->GetPlayerID())))
					return;

				pPC = GetPC(pc);
			}

			if (m_mapNPC[QUEST_NO_NPC].OnKill(*pPC))
				return;

			if (leader)
			{
				m_pCurrentPartyMember = ch;
				m_mapNPC[QUEST_NO_NPC].OnPartyKill(*GetPC(leader->GetPlayerID()));
			}
		}
		else
			SysLog("QUEST: no such pc id : {}", pc);
	}

	bool CQuestManager::ServerTimer(uint32_t npc, uint32_t arg)
	{
		SetServerTimerArg(arg);
		m_pCurrentPC = GetPCForce(0);
		m_pCurrentCharacter = nullptr;
		m_pSelectedDungeon = nullptr;
		return m_mapNPC[npc].OnServerTimer(*m_pCurrentPC);
	}

	bool CQuestManager::Timer(uint32_t pc, uint32_t npc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				return false;
			}
			// call script
			return m_mapNPC[npc].OnTimer(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST TIMER_EVENT no such pc id : {}", pc);
			return false;
		}
		//cerr << "QUEST TIMER" << endl;
	}

	void CQuestManager::LevelUp(uint32_t pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLevelUp(*pPC);
		}
		else
		{
			SysLog("QUEST LEVELUP_EVENT no such pc id : {}", pc);
		}
	}

	void CQuestManager::AttrIn(uint32_t pc, LPCHARACTER ch, int32_t attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrIn(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST no such pc id : {}", pc);
		}
	}

	void CQuestManager::AttrOut(uint32_t pc, LPCHARACTER ch, int32_t attr)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			//m_pCurrentCharacter = ch;
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
				return;

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrOut(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST no such pc id : {}", pc);
		}
	}

	bool CQuestManager::Target(uint32_t pc, uint32_t dwQuestIndex, const char* c_pszTargetName, const char* c_pszVerb)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			bool bRet;
			return m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, dwQuestIndex, c_pszTargetName, c_pszVerb, bRet);
		}

		return false;
	}

	void CQuestManager::QuestInfo(uint32_t pc, uint32_t quest_index)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));

				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnInfo(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST INFO_EVENT no such pc id : {}", pc);
		}
	}

	void CQuestManager::QuestButton(uint32_t pc, uint32_t quest_index)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));
				}
				return;
			}
			m_mapNPC[QUEST_NO_NPC].OnButton(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST CLICK_EVENT no such pc id : {}", pc);
		}
	}

	bool CQuestManager::TakeItem(uint32_t pc, uint32_t npc, LPITEM item)
	{
		//m_CurrentNPCRace = npc;
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);
			return m_mapNPC[npc].OnTakeItem(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST USE_ITEM_EVENT no such pc id : {}", pc);
			return false;
		}
	}

	bool CQuestManager::UseItem(uint32_t pc, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			PyLog("questmanager::UseItem Start : itemVnum : {} PC : {}", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);
			/*
			if (test_server)
			{
				PyLog("Quest UseItem Start : itemVnum : {} PC : {}", item->GetOriginalVnum(), pc);
				auto it = m_mapNPC.begin();
				auto end = m_mapNPC.end();
				for(; it != end ; ++it)
				{
					PyLog("Quest UseItem : vnum : {} item Vnum : {}", it->first, item->GetOriginalVnum());
				}
			}
			if(test_server)
			PyLog("questmanager:useItem: mapNPCVnum : {}\n", m_mapNPC[item->GetVnum()].GetVnum());
			*/

			return m_mapNPC[item->GetVnum()].OnUseItem(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST USE_ITEM_EVENT no such pc id : {}", pc);
			return false;
		}
	}

	// Speical Item Group defined in Group Use
	bool CQuestManager::SIGUse(uint32_t pc, uint32_t sig_vnum, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			PyLog("questmanager::SIGUse Start : itemVnum : {} PC : {}", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[sig_vnum].OnSIGUse(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST USE_ITEM_EVENT no such pc id : {}", pc);
			return false;
		}
	}

	bool CQuestManager::GiveItemToPC(uint32_t pc, LPCHARACTER pChr)
	{
		if (!pChr->IsPC())
			return false;

		PC* pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			TargetInfo* pInfo = CTargetManager::GetInstance()->GetTargetInfo(pc, TARGET_TYPE_VID, pChr->GetVID());

			if (pInfo)
			{
				bool bRet;

				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return true;
			}
		}

		return false;
	}

	bool CQuestManager::Click(uint32_t pc, LPCHARACTER pChrTarget)
	{
		PC* pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));

				return false;
			}

			TargetInfo* pInfo = CTargetManager::GetInstance()->GetTargetInfo(pc, TARGET_TYPE_VID, pChrTarget->GetVID());
			if (test_server)
			{
				PyLog("CQuestManager::Click(pid={}, npc_name={}) - target_info", pc, pChrTarget->GetName());
			}

			if (pInfo)
			{
				bool bRet;
				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return bRet;
			}

			uint32_t dwCurrentNPCRace = pChrTarget->GetRaceNum();

			if (pChrTarget->IsNPC())
			{
				std::map<uint32_t, NPC>::iterator it = m_mapNPC.find(dwCurrentNPCRace);

				if (it == m_mapNPC.end())
				{
					SysLog("CQuestManager::Click(pid={}, target_npc_name={}) - NOT EXIST NPC RACE VNUM[{}]",
							pc, 
							pChrTarget->GetName(), 
							dwCurrentNPCRace);
					return false;
				}

				// call script
				if (it->second.HasChat())
				{
					// if have chat, give chat
					if (test_server)
						PyLog("CQuestManager::Click->OnChat");

					if (!it->second.OnChat(*pPC))
					{
						if (test_server)
							PyLog("CQuestManager::Click->OnChat Failed");

						return it->second.OnClick(*pPC);
					}

					return true;
				}
				else
				{
					// else click
					return it->second.OnClick(*pPC);
				}
			}
			return false;
		}
		else
		{
			//cout << "no such pc id : " << pc;
			SysLog("QUEST CLICK_EVENT no such pc id : {}", pc);
			return false;
		}
		//cerr << "QUEST CLICk" << endl;
	}

	void CQuestManager::Unmount(uint32_t pc)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnUnmount(*pPC);
		}
		else
			SysLog("QUEST no such pc id : {}", pc);
	}
	//German Futures Functional Test
	void CQuestManager::ItemInformer(uint32_t pc,uint32_t vnum)
	{
		
		PC* pPC;
		pPC = GetPC(pc);
		
		m_mapNPC[QUEST_NO_NPC].OnItemInformer(*pPC,vnum);
	}
	///////////////////////////////////////////////////////////////////////////////////////////
	// END OF Quest event handling
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::LoadStartQuest(const std::string& quest_name, uint32_t idx)
	{
		for (auto it = g_setQuestObjectDir.begin(); it != g_setQuestObjectDir.end(); ++it)
		{
			const std::string& stQuestObjectDir = *it;
			std::string full_name = stQuestObjectDir + "/begin_condition/" + quest_name;
			std::ifstream inf(full_name.c_str());

			if (inf.is_open())
			{
				PyLog("QUEST loading begin condition for {}", quest_name.c_str());

				std::istreambuf_iterator<char> ib(inf), ie;
				copy(ib, ie, back_inserter(m_hmQuestStartScript[idx]));
			}
		}
	}

	bool CQuestManager::CanStartQuest(uint32_t quest_index, const PC& pc)
	{
		return CanStartQuest(quest_index);
	}

	bool CQuestManager::CanStartQuest(uint32_t quest_index)
	{
		THashMapQuestStartScript::iterator it;

		if ((it = m_hmQuestStartScript.find(quest_index)) == m_hmQuestStartScript.end())
			return true;
		else
		{
			int32_t x = lua_gettop(L);
			lua_dobuffer(L, &(it->second[0]), it->second.size(), "StartScript");
			int32_t bStart = lua_toboolean(L, -1);
			lua_settop(L, x);
			return bStart != 0;
		}
	}

	bool CQuestManager::CanEndQuestAtState(const std::string& quest_name, const std::string& state_name)
	{
		return false;
	}

	void CQuestManager::DisconnectPC(LPCHARACTER ch)
	{
		m_mapPC.erase(ch->GetPlayerID());
	}

	PC * CQuestManager::GetPCForce(uint32_t pc)
	{
		PCMap::iterator it;

		if ((it = m_mapPC.find(pc)) == m_mapPC.end())
		{
			PC* pPC = &m_mapPC[pc];
			pPC->SetID(pc);
			return pPC;
		}

		return &it->second;
	}

	PC * CQuestManager::GetPC(uint32_t pc)
	{
		PCMap::iterator it;

		LPCHARACTER pChr = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);

		if (!pChr)
			return NULL;

		m_pCurrentPC = GetPCForce(pc);
		m_pCurrentCharacter = pChr;
		m_pSelectedDungeon = nullptr;
		return (m_pCurrentPC);
	}

	void CQuestManager::ClearScript()
	{
		m_strScript.clear();
		m_iCurrentSkin = QUEST_SKIN_NORMAL;
	}

	void CQuestManager::AddScript(const std::string& str)
	{
		m_strScript+=str;
	}

	void CQuestManager::SendScript()
	{
		if (m_bNoSend)
		{
			m_bNoSend = false;
			ClearScript();
			return;
		}

		if (m_strScript=="[DONE]" || m_strScript == "[NEXT]")
		{
			if (m_pCurrentPC && !m_pCurrentPC->GetAndResetDoneFlag() && m_strScript=="[DONE]" && m_iCurrentSkin == QUEST_SKIN_NORMAL && !IsError())
			{
				ClearScript();
				return;
			}
			m_iCurrentSkin = QUEST_SKIN_NOWINDOW;
		}

		//PyLog("Send Quest Script to {}", GetCurrentCharacterPtr()->GetName());
		//send -_-!
		struct ::packet_script packet_script;

		packet_script.header = HEADER_GC_SCRIPT;
		packet_script.skin = m_iCurrentSkin;
		packet_script.src_size = m_strScript.size();
		packet_script.size = packet_script.src_size + sizeof(struct packet_script);

		TEMP_BUFFER buf;
		buf.write(&packet_script, sizeof(struct packet_script));
		buf.write(&m_strScript[0], m_strScript.size());

		GetCurrentCharacterPtr()->GetDesc()->Packet(buf.read_peek(), buf.size());

		if (test_server)
			PyLog("m_strScript {} size {}", m_strScript.c_str(), buf.size());

		ClearScript();
	}

	const char* CQuestManager::GetQuestStateName(const std::string& quest_name, const int32_t state_index)
	{
		int32_t x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			SysLog("QUEST wrong quest state file {}.{}", quest_name.c_str(), state_index);
			lua_settop(L,x);
			return "";
		}
		lua_pushnumber(L, state_index);
		lua_gettable(L, -2);

		const char* str = lua_tostring(L, -1);
		lua_settop(L, x);
		return str;
	}

	int32_t CQuestManager::GetQuestStateIndex(const std::string& quest_name, const std::string& state_name)
	{
		int32_t x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			SysLog("QUEST wrong quest state file {}.{}",quest_name.c_str(),state_name.c_str());
			lua_settop(L,x);
			return 0;
		}
		lua_pushstring(L, state_name.c_str());
		lua_gettable(L, -2);

		int32_t v = (int32_t)rint(lua_tonumber(L,-1));
		lua_settop(L, x);
		if (test_server)
			PyLog("[QUESTMANAGER] GetQuestStateIndex x({}) v({}) {} {}", v,x, quest_name.c_str(), state_name.c_str());
		return v;
	}

	void CQuestManager::SetSkinStyle(int32_t iStyle)
	{
		if (iStyle<0 || iStyle >= QUEST_SKIN_COUNT)
		{
			m_iCurrentSkin = QUEST_SKIN_NORMAL;
		}
		else
			m_iCurrentSkin = iStyle;
	}

	uint32_t CQuestManager::LoadTimerScript(const std::string& name)
	{
		std::map<std::string, uint32_t>::iterator it;
		if ((it = m_mapTimerID.find(name)) != m_mapTimerID.end())
		{
			return it->second;
		}
		else
		{
			uint32_t new_id = UINT_MAX - m_mapTimerID.size();

			m_mapNPC[new_id].Set(new_id, name);
			m_mapTimerID.insert(make_pair(name, new_id));

			return new_id;
		}
	}

	uint32_t CQuestManager::GetCurrentNPCRace()
	{
		return GetCurrentNPCCharacterPtr() ? GetCurrentNPCCharacterPtr()->GetRaceNum() : 0;
	}

	LPITEM CQuestManager::GetCurrentItem()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestItemPtr() : NULL; 
	}

	void CQuestManager::ClearCurrentItem()
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->ClearQuestItemPtr();
	}

	void CQuestManager::SetCurrentItem(LPITEM item)
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->SetQuestItemPtr(item);
	}

	LPCHARACTER CQuestManager::GetCurrentNPCCharacterPtr()
	{ 
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestNPC() : NULL; 
	}

	const std::string & CQuestManager::GetCurrentQuestName()
	{
		return GetCurrentPC()->GetCurrentQuestName();
	}

	LPDUNGEON CQuestManager::GetCurrentDungeon()
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();

		if (!ch)
		{
			if (m_pSelectedDungeon)
				return m_pSelectedDungeon;
			return NULL;
		}

		return ch->GetDungeonForce();
	}

	void CQuestManager::RegisterQuest(const std::string & stQuestName, uint32_t idx)
	{
		assert(idx > 0);
		auto it = m_hmQuestName.find(stQuestName);

		if (it != m_hmQuestName.end())
			return;

		m_hmQuestName.insert(std::make_pair(stQuestName, idx));
		LoadStartQuest(stQuestName, idx);
		m_mapQuestNameByIndex.insert(make_pair(idx, stQuestName));

		PyLog("QUEST: Register %4u {}", idx, stQuestName.c_str());
	}

	uint32_t CQuestManager::GetQuestIndexByName(const std::string& name)
	{
		THashMapQuestName::iterator it = m_hmQuestName.find(name);

		if (it == m_hmQuestName.end())
			return 0; // RESERVED

		return it->second;
	}

	const std::string & CQuestManager::GetQuestNameByIndex(uint32_t idx)
	{
		auto it = m_mapQuestNameByIndex.find(idx);
		
		if (it == m_mapQuestNameByIndex.end())
		{
			SysLog("cannot find quest name by index {}", idx);
			assert(!"cannot find quest name by index");

			static std::string st = "";
			return st;
		}

		return it->second;
	}

	void CQuestManager::SendEventFlagList(LPCHARACTER ch)
	{
		for (auto it = m_mapEventFlag.begin(); it != m_mapEventFlag.end(); ++it)
		{
			const std::string& flagname = it->first;
			int32_t value = it->second;

			if (!test_server && value == 1 && flagname == "valentine_drop")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 800", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_wonso")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 500", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_fire")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 1000", flagname.c_str(), value);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", flagname.c_str(), value);
		}
	}

	void CQuestManager::RequestSetEventFlag(const std::string& name, int32_t value)
	{
		TPacketSetEventFlag p;
		strlcpy(p.szFlagName, name.c_str(), sizeof(p.szFlagName));
		p.lValue = value;
		db_clientdesc->DBPacket(HEADER_GD_SET_EVENT_FLAG, 0, &p, sizeof(TPacketSetEventFlag));
	}

	void CQuestManager::SetEventFlag(const std::string& name, int32_t value)
	{
		static const char*	DROPEVENT_CHARTONE_NAME		= "drop_char_stone";
		static const int32_t	DROPEVENT_CHARTONE_NAME_LEN = strlen(DROPEVENT_CHARTONE_NAME);

		int32_t prev_value = m_mapEventFlag[name];

		PyLog("QUEST eventflag {} {} prev_value {}", name.c_str(), value, m_mapEventFlag[name]);
		m_mapEventFlag[name] = value;

		if (name == "mob_item")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobItemRate(value);
		}
		else if (name == "mob_dam")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobDamageRate(value);
		}
		else if (name == "mob_gold")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobGoldAmountRate(value);
		}
		else if (name == "mob_gold_pct")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobGoldDropRate(value);
		}
		else if (name == "user_dam")
		{
			CHARACTER_MANAGER::GetInstance()->SetUserDamageRate(value);
		}
		else if (name == "user_dam_buyer")
		{
			CHARACTER_MANAGER::GetInstance()->SetUserDamageRatePremium(value);
		}
		else if (name == "mob_exp")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobExpRate(value);
		}
		else if (name == "mob_item_buyer")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobItemRatePremium(value);
		}
		else if (name == "mob_exp_buyer")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobExpRatePremium(value);
		}
		else if (name == "mob_gold_buyer")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobGoldAmountRatePremium(value);
		}
		else if (name == "mob_gold_pct_buyer")
		{
			CHARACTER_MANAGER::GetInstance()->SetMobGoldDropRatePremium(value);
		}
		else if (name == "crcdisconnect")
		{
			DESC_MANAGER::GetInstance()->SetDisconnectInvalidCRCMode(value != 0);
		}
		else if (!name.compare(0,5,"xmas_"))
		{
			xmas::ProcessEventFlag(name, prev_value, value);
		}
		else if (name == "newyear_boom")
		{
			const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::GetInstance()->GetClientSet();

			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();

				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", value);
			}
		}
		else if (name == "eclipse")
		{
			std::string mode("");

			if (value == 1)
			{
				mode = "dark";
			}
			else
			{
				mode = "light";
			}
			
			const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::GetInstance()->GetClientSet();

			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
			}
		}
		else if (name == "day")
		{
			const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::GetInstance()->GetClientSet();

			for (auto it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;
				if (value)
				{
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode light");
				}
			}

			if (value && !prev_value)
			{
				struct SNPCSellFireworkPosition
				{
					int32_t lMapIndex;
					int32_t x;
					int32_t y;
				} positions[] = {
					{	1,	615,	618 },
					{	3,	500,	625 },
					{	21,	598,	665 },
					{	23,	476,	360 },
					{	41,	318,	629 },
					{	43,	478,	375 },
					{	0,	0,	0   },
				};

				SNPCSellFireworkPosition* p = positions;
				while (p->lMapIndex)
				{
					if (map_allow_find(p->lMapIndex))
					{
						PIXEL_POSITION posBase;
						if (!SECTREE_MANAGER::GetInstance()->GetMapBasePositionByMapIndex(p->lMapIndex, posBase))
						{
							SysLog("cannot get map base position {}", p->lMapIndex);
							++p;
							continue;
						}

						CHARACTER_MANAGER::GetInstance()->SpawnMob(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, p->lMapIndex, posBase.x + p->x * 100, posBase.y + p->y * 100, 0, false, -1);
					}
					p++;
				}
			}
			else if (!value && prev_value)
			{
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end()) {
						M2_DESTROY_CHARACTER(*it++);
					}
				}
			}
		}
		else if (name == "pre_event_hc")
		{
			const uint32_t EventNPC = 20090;

			struct SEventNPCPosition
			{
				int32_t lMapIndex;
				int32_t x;
				int32_t y;
			} positions[] = {
				{ 3, 588, 617 },
				{ 23, 397, 250 },
				{ 43, 567, 426 },
				{ 0, 0, 0 },
			};

			if (value && !prev_value)
			{
				SEventNPCPosition* pPosition = positions;

				while (pPosition->lMapIndex)
				{
					if (map_allow_find(pPosition->lMapIndex))
					{
						PIXEL_POSITION pos;

						if (!SECTREE_MANAGER::GetInstance()->GetMapBasePositionByMapIndex(pPosition->lMapIndex, pos))
						{
							SysLog("cannot get map base position {}", pPosition->lMapIndex);
							++pPosition;
							continue;
						}

						CHARACTER_MANAGER::GetInstance()->SpawnMob(EventNPC, pPosition->lMapIndex, pos.x+pPosition->x*100, pos.y+pPosition->y*100, 0, false, -1);
					}
					pPosition++;
				}
			}
			else if (!value && prev_value)
			{
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(EventNPC, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER ch = *it++;

						switch (ch->GetMapIndex())
						{
							case 3:
							case 23:
							case 43:
								M2_DESTROY_CHARACTER(ch);
								break;
						}
					}
				}
			}
		}
		else if (name.compare(0, DROPEVENT_CHARTONE_NAME_LEN, DROPEVENT_CHARTONE_NAME)== 0)
		{
			DropEvent_CharStone_SetValue(name, value);
		}
		else if (name.compare(0, strlen("refine_box"), "refine_box")== 0)
		{
			DropEvent_RefineBox_SetValue(name, value);
		}
		else if (name == "gold_drop_limit_time")
		{
			g_GoldDropTimeLimitValue = value * 1000;
		}
		else if (name == "new_xmas_event")
		{
			static uint32_t new_santa = 20126;
			if (value != 0)
			{
				CharacterVectorInteractor i;
				bool map1_santa_exist = false;
				bool map21_santa_exist = false;
				bool map41_santa_exist = false;
				
				if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(new_santa, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER tch = *(it++);

						if (tch->GetMapIndex() == 1)
						{
							map1_santa_exist = true;
						}
						else if (tch->GetMapIndex() == 21)
						{
							map21_santa_exist = true;
						}
						else if (tch->GetMapIndex() == 41)
						{
							map41_santa_exist = true;
						}
					}
				}

				if (map_allow_find(1) && !map1_santa_exist)
				{
					LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(1);
					CHARACTER_MANAGER::GetInstance()->SpawnMob(new_santa, 1, pSectreeMap->m_setting.iBaseX + 60800, pSectreeMap->m_setting.iBaseY + 61700, 0, false, 90, true);
				}
				if (map_allow_find(21) && !map21_santa_exist)
				{
					LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(21);
					CHARACTER_MANAGER::GetInstance()->SpawnMob(new_santa, 21, pSectreeMap->m_setting.iBaseX + 59600, pSectreeMap->m_setting.iBaseY + 61000, 0, false, 110, true);
				}
				if (map_allow_find(41) && !map41_santa_exist)
				{
					LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(41);
					CHARACTER_MANAGER::GetInstance()->SpawnMob(new_santa, 41, pSectreeMap->m_setting.iBaseX + 35700, pSectreeMap->m_setting.iBaseY + 74300, 0, false, 140, true);
				}
			}
			else
			{
				CharacterVectorInteractor i;
				CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(new_santa, i);
				
				for (CharacterVectorInteractor::iterator it = i.begin(); it != i.end(); it++)
				{
					M2_DESTROY_CHARACTER(*it);
				}
			}
		}
	}

	int32_t	CQuestManager::GetEventFlag(const std::string& name)
	{
		std::map<std::string,int32_t>::iterator it = m_mapEventFlag.find(name);

		if (it == m_mapEventFlag.end())
			return 0;

		return it->second;
	}

	void CQuestManager::BroadcastEventFlagOnLogin(LPCHARACTER ch)
	{
		int32_t iEventFlagValue;

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("xmas_snow")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_snow %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("xmas_boom")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_boom %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("xmas_tree")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_tree %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("day")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		}

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("newyear_boom")))
		{
			ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::GetInstance()->GetEventFlag("eclipse")))
		{
			std::string mode;

			if (iEventFlagValue == 1) mode = "dark";
			else mode = "light";

			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
		}
	}

	void CQuestManager::Reload()
	{
		lua_close(L);
		m_mapNPC.clear();
		m_mapNPCNameID.clear();
		m_hmQuestName.clear();
		m_mapTimerID.clear();
		m_hmQuestStartScript.clear();
		m_mapEventName.clear();
		L = nullptr;
		Initialize();

		for (auto it = m_registeredNPCVnum.begin(); it != m_registeredNPCVnum.end(); ++it)
		{
			char buf[256];
			uint32_t dwVnum = *it;
			snprintf(buf, sizeof(buf), "%u", dwVnum);
			m_mapNPC[dwVnum].Set(dwVnum, buf);
		}
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, uint32_t quest_index, const int32_t state, const char* code, const int32_t code_size, std::vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		return ExecuteQuestScript(pc, CQuestManager::GetInstance()->GetQuestNameByIndex(quest_index), state, code, code_size, pChatScripts, bUseCache);
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, const std::string& quest_name, const int32_t state, const char* code, const int32_t code_size, std::vector<AArgScript*>* pChatScripts, bool bUseCache)
	{

		QuestState qs = CQuestManager::GetInstance()->OpenState(quest_name, state);
		if (pChatScripts)
			qs.chat_scripts.swap(*pChatScripts);
		
		if (bUseCache)
		{
			lua_getglobal(qs.co, "__codecache");
			// stack : __codecache
			lua_pushnumber(qs.co, (int32_t)code);
			// stack : __codecache (codeptr)
			lua_rawget(qs.co, -2);
			// stack : __codecache (compiled-code)
			if (lua_isnil(qs.co, -1))
			{
				// cache miss

				// load code to lua,
				// save it to cache
				// and only function remain in stack
				lua_pop(qs.co, 1);
				// stack : __codecache
				luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());
				// stack : __codecache (compiled-code)
				lua_pushnumber(qs.co, (int32_t)code);
				// stack : __codecache (compiled-code) (codeptr)
				lua_pushvalue(qs.co, -2);
				// stack : __codecache (compiled-code) (codeptr) (compiled_code)
				lua_rawset(qs.co, -4);
				// stack : __codecache (compiled-code)
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
			else
			{
				// cache hit
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
		}
		else
			luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());

		pc.SetQuest(quest_name, qs);

		QuestState& rqs =* pc.GetRunningQuestState();
		if (!CQuestManager::GetInstance()->RunState(rqs))
		{
			CQuestManager::GetInstance()->CloseState(rqs);
			pc.EndRunning();
			return false;
		}
		return true;
	}

	void CQuestManager::RegisterNPCVnum(uint32_t dwVnum)
	{
		if (m_registeredNPCVnum.find(dwVnum) != m_registeredNPCVnum.end())
			return;

		m_registeredNPCVnum.insert(dwVnum);

		char buf[256];

		for (const auto& stQuestObjectDir : g_setQuestObjectDir)
		{
			snprintf(buf, sizeof(buf), "%s/%u", stQuestObjectDir.c_str(), dwVnum);
			PyLog("{}", buf);

			if (std::filesystem::exists(buf))
			{
				snprintf(buf, sizeof(buf), "%u", dwVnum);
				PyLog("{}", buf);

				m_mapNPC[dwVnum].Set(dwVnum, buf);
			}
		}
	}

	void CQuestManager::WriteRunningStateToSyserr()
	{
		const char* state_name = GetQuestStateName(GetCurrentQuestName(), GetCurrentState()->st);

		std::string event_index_name = "";
		for (auto it = m_mapEventName.begin(); it != m_mapEventName.end(); ++it)
		{
			if (it->second == m_iRunningEventIndex)
			{
				event_index_name = it->first;
				break;
			}
		}

		SysLog("LUA_ERROR: quest {}.{} {}", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str());
		if (GetCurrentCharacterPtr() && test_server)
			GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_PARTY, "LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str());
	}

#ifndef __WIN32__
	void CQuestManager::QuestError(const char* func, int32_t line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		SysLog("{}", szMsg);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d]", func,line);
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#else
	void CQuestManager::QuestError(const char* func, int32_t line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		SysLog("{}", szMsg);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d]", func,line);
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#endif

	void CQuestManager::AddServerTimer(const std::string& name, uint32_t arg, LPEVENT event)
	{
		TraceLog("XXX AddServerTimer {} {}", name.c_str(), arg);
		if (m_mapServerTimer.find(make_pair(name, arg)) != m_mapServerTimer.end())
		{
			SysLog("already registered server timer name:{} arg:{}", name.c_str(), arg);
			return;
		}
		m_mapServerTimer.insert(make_pair(make_pair(name, arg), event));
	}

	void CQuestManager::ClearServerTimerNotCancel(const std::string& name, uint32_t arg)
	{
		m_mapServerTimer.erase(make_pair(name, arg));
	}

	void CQuestManager::ClearServerTimer(const std::string& name, uint32_t arg)
	{
		auto it = m_mapServerTimer.find(make_pair(name, arg));
		if (it != m_mapServerTimer.end())
		{
			LPEVENT event = it->second;
			event_cancel(&event);
			m_mapServerTimer.erase(it);
		}
	}

	void CQuestManager::CancelServerTimers(uint32_t arg)
	{
		auto it = m_mapServerTimer.begin();
		for (; it != m_mapServerTimer.end(); ++it) {
			if (it->first.second == arg) {
				LPEVENT event = it->second;
				event_cancel(&event);
				m_mapServerTimer.erase(it);
			}
		}
	}

	void CQuestManager::SetServerTimerArg(uint32_t dwArg)
	{
		m_dwServerTimerArg = dwArg;
	}

	uint32_t CQuestManager::GetServerTimerArg()
	{
		return m_dwServerTimerArg;
	}

	void CQuestManager::SelectDungeon(LPDUNGEON pDungeon)
	{
		m_pSelectedDungeon = pDungeon;
	}
	
	bool CQuestManager::PickupItem(uint32_t pc, LPITEM item)
	{
		if (test_server)
			PyLog("questmanager::PickupItem Start : itemVnum : {} PC : {}", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your request is loading. Please wait."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[item->GetVnum()].OnPickupItem(*pPC);
		}
		else
		{
			SysLog("QUEST PICK_ITEM_EVENT no such pc id : {}", pc);
			return false;
		}
	}
	void CQuestManager::BeginOtherPCBlock(uint32_t pid)
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();
		if (!ch)
		{
			SysLog("NULL?");
			return;
		}
		/*
		# 1. current pid = pid0 <- It will be m_pOtherPCBlockRootPC.
		begin_other_pc_block(pid1)
			# 2. current pid = pid1
			begin_other_pc_block(pid2)
				# 3. current_pid = pid2
			end_other_pc_block()
		end_other_pc_block()
		*/
		// when begin_other_pc_block(pid1)
		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = GetCurrentPC();
		}
		m_vecPCStack.push_back(GetCurrentCharacterPtr()->GetPlayerID());
		GetPC(pid);
	}

	void CQuestManager::EndOtherPCBlock()
	{
		if (m_vecPCStack.size() == 0)
		{
			SysLog("m_vecPCStack is alread empty. CurrentQuest{Name({}), State({})}", GetCurrentQuestName().c_str(), GetCurrentState()->_title.c_str());
			return;
		}
		uint32_t pc = m_vecPCStack.back();
		m_vecPCStack.pop_back();
		GetPC(pc);

		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = nullptr;
		}
	}

	bool CQuestManager::IsInOtherPCBlock()
	{
		return !m_vecPCStack.empty();
	}

	PC*	CQuestManager::GetOtherPCBlockRootPC()
	{
		return m_pOtherPCBlockRootPC;
	}
}

