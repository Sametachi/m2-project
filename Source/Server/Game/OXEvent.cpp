#include "stdafx.h"
#include <Core/Net/PacketsGC.hpp>
#include "constants.h"
#include "config.h"
#include "questmanager.h"
#include "start_position.h"
#include "packet.h"
#include "buffer_manager.h"
#include "log.h"
#include "char.h"
#include "char_manager.h"
#include "OXEvent.h"
#include "desc.h"

bool COXEventManager::Initialize()
{
	m_timedEvent = nullptr;
	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);

	return true;
}

void COXEventManager::Destroy()
{
	CloseEvent();

	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);
}

OXEventStatus COXEventManager::GetStatus()
{
	uint8_t ret = quest::CQuestManager::GetInstance()->GetEventFlag("oxevent_status");

	switch (ret)
	{
		case 0 :
			return OXEVENT_FINISH;
			
		case 1 :
			return OXEVENT_OPEN;
			
		case 2 :
			return OXEVENT_CLOSE;
			
		case 3 :
			return OXEVENT_QUIZ;
			
		default :
			return OXEVENT_ERR;
	}

	return OXEVENT_ERR;
}

void COXEventManager::SetStatus(OXEventStatus status)
{
	uint8_t val = 0;
	
	switch (status)
	{
		case OXEVENT_OPEN :
			val = 1;
			break;
			
		case OXEVENT_CLOSE :
			val = 2;
			break;
			
		case OXEVENT_QUIZ :
			val = 3;
			break;
		
		case OXEVENT_FINISH :
		case OXEVENT_ERR :
		default :
			val = 0;
			break;
	}
	quest::CQuestManager::GetInstance()->RequestSetEventFlag("oxevent_status", val);
}

bool COXEventManager::Enter(LPCHARACTER pChar)
{
	if (GetStatus() == OXEVENT_FINISH)
	{
		PyLog("OXEVENT : map finished. but char enter. {}", pChar->GetName());
		return false;
	}

	PIXEL_POSITION pos = pChar->GetXYZ();

	if (pos.x == 896500 && pos.y == 24600)
	{
		return EnterAttender(pChar);
	}
	else if (pos.x == 896300 && pos.y == 28900)
	{
		return EnterAudience(pChar);
	}
	else
	{
		PyLog("OXEVENT : wrong pos enter {} {}", pos.x, pos.y);
		return false;
	}

	return false;
}

bool COXEventManager::EnterAttender(LPCHARACTER pChar)
{
	uint32_t pid = pChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));
	m_map_attender.insert(std::make_pair(pid, pid));

	return true;
}

bool COXEventManager::EnterAudience(LPCHARACTER pChar)
{
	uint32_t pid = pChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));

	return true;
}

bool COXEventManager::AddQuiz(uint8_t level, const char* pszQuestion, bool answer)
{
	if (m_vec_quiz.size() < (size_t) level + 1)
		m_vec_quiz.resize(level + 1);

	struct tag_Quiz tmpQuiz;

	tmpQuiz.level = level;
	strlcpy(tmpQuiz.Quiz, pszQuestion, sizeof(tmpQuiz.Quiz));
	tmpQuiz.answer = answer;

	m_vec_quiz[level].push_back(tmpQuiz);
	return true;
}

bool COXEventManager::ShowQuizList(LPCHARACTER pChar)
{
	int32_t c = 0;
	
	for (size_t i = 0; i < m_vec_quiz.size(); ++i)
	{
		for (size_t j = 0; j < m_vec_quiz[i].size(); ++j, ++c)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, "%d %s %s", m_vec_quiz[i][j].level, m_vec_quiz[i][j].Quiz, m_vec_quiz[i][j].answer ? LC_TEXT("TRUE") : LC_TEXT("FALSE"));
		}
	}

	pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Total number of the Quiz: %d"), c);	
	return true;
}

void COXEventManager::ClearQuiz()
{
	for (uint32_t i = 0; i < m_vec_quiz.size(); ++i)
	{
		m_vec_quiz[i].clear();
	}

	m_vec_quiz.clear();
}

EVENTINFO(OXEventInfoData)
{
	bool answer;

	OXEventInfoData()
	: answer(false)
	{
	}
};

EVENTFUNC(oxevent_timer)
{
	static uint8_t flag = 0;
	OXEventInfoData* info = dynamic_cast<OXEventInfoData*>(event->info);

	if (info == nullptr)
	{
		SysLog("oxevent_timer> <Factor> Null pointer");
		return 0;
	}

	switch (flag)
	{
		case 0:
			SendNoticeMap(LC_TEXT("The result will follow in 10 seconds."), OXEVENT_MAP_INDEX, true);
			flag++;
			return PASSES_PER_SEC(10);
			
		case 1:
			SendNoticeMap(LC_TEXT("The correct answer is:"), OXEVENT_MAP_INDEX, true);

			if (info->answer)
			{
				COXEventManager::GetInstance()->CheckAnswer(true);
				SendNoticeMap(LC_TEXT("Yes (O)"), OXEVENT_MAP_INDEX, true);
			}
			else
			{
				COXEventManager::GetInstance()->CheckAnswer(false);
				SendNoticeMap(LC_TEXT("No (X)"), OXEVENT_MAP_INDEX, true);
			}

			SendNoticeMap(LC_TEXT("In 5 sec. everyone who gave an incorrect answer will be removed."), OXEVENT_MAP_INDEX, true);

			flag++;
			return PASSES_PER_SEC(5);

		case 2:
			COXEventManager::GetInstance()->WarpToAudience();
			COXEventManager::GetInstance()->SetStatus(OXEVENT_CLOSE);
			SendNoticeMap(LC_TEXT("Ready for the next question?"), OXEVENT_MAP_INDEX, true);
			flag = 0;
			break;
	}
	return 0;
}

bool COXEventManager::Quiz(uint8_t level, int32_t timelimit)
{
	if (m_vec_quiz.size() == 0) return false;
	if (level > m_vec_quiz.size()) level = m_vec_quiz.size() - 1;
	if (m_vec_quiz[level].size() <= 0) return false;

	if (timelimit < 0) timelimit = 30;

	int32_t idx = number(0, m_vec_quiz[level].size()-1);

	SendNoticeMap(LC_TEXT("Question."), OXEVENT_MAP_INDEX, true);
	SendNoticeMap(m_vec_quiz[level][idx].Quiz, OXEVENT_MAP_INDEX, true);
	SendNoticeMap(LC_TEXT("If it's correct, then go to O. If it's wrong, go to X."), OXEVENT_MAP_INDEX, true);

	if (m_timedEvent != nullptr) {
		event_cancel(&m_timedEvent);
	}

	OXEventInfoData* answer = AllocEventInfo<OXEventInfoData>();

	answer->answer = m_vec_quiz[level][idx].answer;

	timelimit -= 15;
	m_timedEvent = event_create(oxevent_timer, answer, PASSES_PER_SEC(timelimit));

	SetStatus(OXEVENT_QUIZ);

	m_vec_quiz[level].erase(m_vec_quiz[level].begin()+idx);
	return true;
}

bool COXEventManager::CheckAnswer(bool answer)
{
	if (m_map_attender.size() <= 0) return true;
	
	itertype(m_map_attender) iter = m_map_attender.begin();
	itertype(m_map_attender) iter_tmp;
	
	m_map_miss.clear();

	int32_t rect[4];
	if (answer != true)
	{
		rect[0] = 892600;
		rect[1] = 22900;
		rect[2] = 896300;
		rect[3] = 26400;
	}
	else
	{
		rect[0] = 896600;
		rect[1] = 22900;
		rect[2] = 900300;
		rect[3] = 26400;
	}
	
	LPCHARACTER pChar = nullptr;
	PIXEL_POSITION pos;
	for (; iter != m_map_attender.end();)
	{
		pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(iter->second);
		if (pChar != nullptr)
		{
			pos = pChar->GetXYZ();

			if (pos.x < rect[0] || pos.x > rect[2] || pos.y < rect[1] || pos.y > rect[3])
			{
				pChar->EffectPacket(SE_FAIL);
				iter_tmp = iter;
				iter++;
				m_map_attender.erase(iter_tmp);
				m_map_miss.insert(std::make_pair(pChar->GetPlayerID(), pChar->GetPlayerID()));
			}
			else
			{
				pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Correct!"));
				char chatbuf[256];
				int32_t len = snprintf(chatbuf, sizeof(chatbuf), 
						"%s %u %u", number(0, 1) == 1 ? "cheer1" : "cheer2", (uint32_t)pChar->GetVID(), 0);

				if (len < 0 || len >= (int32_t) sizeof(chatbuf))
					len = sizeof(chatbuf) - 1;

				++len;

				TPacketGCChat pack_chat;
				pack_chat.header = HEADER_GC_CHAT;
				pack_chat.size = sizeof(TPacketGCChat) + len;
				pack_chat.type = CHAT_TYPE_COMMAND;
				pack_chat.id = 0;

				TEMP_BUFFER buf;
				buf.write(&pack_chat, sizeof(TPacketGCChat));
				buf.write(chatbuf, len);

				pChar->PacketAround(buf.read_peek(), buf.size());
				pChar->EffectPacket(SE_SUCCESS);

				++iter;
			}
		}
		else
		{
			itertype(m_map_char) err = m_map_char.find(iter->first);
			if (err != m_map_char.end()) m_map_char.erase(err);

			itertype(m_map_miss) err2 = m_map_miss.find(iter->first);
			if (err2 != m_map_miss.end()) m_map_miss.erase(err2);

			iter_tmp = iter;
			++iter;
			m_map_attender.erase(iter_tmp);
		}
	}
	return true;
}

void COXEventManager::WarpToAudience()
{
	if (m_map_miss.size() <= 0) return;

	itertype(m_map_miss) iter = m_map_miss.begin();
	LPCHARACTER pChar = nullptr;
	
	for (; iter != m_map_miss.end(); ++iter)
	{
		pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(iter->second);

		if (pChar != nullptr)
		{
			switch (number(0, 3))
			{
				case 0 : pChar->Show(OXEVENT_MAP_INDEX, 896300, 28900); break;
				case 1 : pChar->Show(OXEVENT_MAP_INDEX, 890900, 28100); break;
				case 2 : pChar->Show(OXEVENT_MAP_INDEX, 896600, 20500); break;
				case 3 : pChar->Show(OXEVENT_MAP_INDEX, 902500, 28100); break;
				default : pChar->Show(OXEVENT_MAP_INDEX, 896300, 28900); break;
			}
		}
	}

	m_map_miss.clear();
}

bool COXEventManager::CloseEvent()
{
	if (m_timedEvent != nullptr) {
		event_cancel(&m_timedEvent);
	}

	itertype(m_map_char) iter = m_map_char.begin();

	LPCHARACTER pChar = nullptr;
	for (; iter != m_map_char.end(); ++iter)
	{
		pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(iter->second);

		if (pChar != nullptr)
			pChar->WarpSet(EMPIRE_START_X(pChar->GetEmpire()), EMPIRE_START_Y(pChar->GetEmpire()));
	}

	m_map_char.clear();

	return true;
}

bool COXEventManager::LogWinner()
{
	itertype(m_map_attender) iter = m_map_attender.begin();
	
	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(iter->second);

		if (pChar)
			LogManager::GetInstance()->CharLog(pChar, 0, "OXEVENT", "LastManStanding");
	}

	return true;
}

bool COXEventManager::GiveItemToAttender(uint32_t dwItemVnum, uint8_t count)
{
	itertype(m_map_attender) iter = m_map_attender.begin();

	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pChar = CHARACTER_MANAGER::GetInstance()->FindByPID(iter->second);

		if (pChar)
		{
			pChar->AutoGiveItem(dwItemVnum, count);
			LogManager::GetInstance()->ItemLog(pChar->GetPlayerID(), 0, count, dwItemVnum, "OXEVENT_REWARD", "", pChar->GetDesc()->GetHostName(), dwItemVnum);
		}
	}

	return true;
}

