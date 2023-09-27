#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonNonPlayer.h"
#include "PythonApplication.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonChat.h"
#include "InstanceBase.h"

#define ishan(ch)		(((ch) & 0xE0) > 0x90)
#define ishanasc(ch)	(isascii(ch) || ishan(ch))
#define ishanalp(ch)	(isalpha(ch) || ishan(ch))
#define isnhdigit(ch)	(!ishan(ch) && isdigit(ch))
#define isnhspace(ch)	(!ishan(ch) && isspace(ch))

#define LOWER(c)		(((c) >= 'A' && (c) <= 'Z') ? ((c) + ('a' - 'A')) : (c))
#define UPPER(c)		(((c) >= 'a' && (c) <= 'z') ? ((c) + ('A' - 'a')) : (c))

void SkipSpaces(char** string)
{
	for (; **string != '\0' && isnhspace((unsigned char)**string); ++(*string));
}

char* OneArgument(char* argument, char* first_arg)
{
	bool mark = false;

	if (!argument)
	{
		*first_arg = '\0';
		return nullptr;
	}

	SkipSpaces(&argument);

	while (*argument)
	{
		if (*argument == '\"')
		{
			mark = !mark;
			++argument;
			continue;
		}

		if (!mark && isnhspace((uint8_t)*argument))
			break;

		*(first_arg++) = LOWER(*argument);
		++argument;
	}

	*first_arg = '\0';

	SkipSpaces(&argument);
	return (argument);
}

bool CPythonNetworkStream::ClientCommand(const char* c_szCommand)
{
	return false;
}

bool SplitToken(const char* c_szLine, CTokenVector* pstTokenVector, const char* c_szDelimeter = " ")
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();
	std::string strLine = c_szLine;
	uint32_t basePos = 0;

	do
	{
		int32_t beginPos = strLine.find_first_not_of(c_szDelimeter, basePos);
		if (beginPos < 0)
			return false;

		int32_t endPos;

		if (strLine[beginPos] == '"')
		{
			++beginPos;
			endPos = strLine.find_first_of('\"', beginPos);

			if (endPos < 0)
				return false;

			basePos = endPos + 1;
		}
		else
		{
			endPos = strLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->emplace_back(strLine.substr(beginPos, static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(endPos) - beginPos));

		if (int32_t(strLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} while (basePos < strLine.length());

	return true;
}

void CPythonNetworkStream::ServerCommand(char * c_szCommand)
{
	if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
	{
		bool isTrue = PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_ServerCommand_Run", c_szCommand);

		if (isTrue)
			return;
	}
	else if (m_poSerCommandParserWnd)
	{
		bool isTrue = PyCallClassMemberFunc(m_poSerCommandParserWnd, "BINARY_ServerCommand_Run");

		if (isTrue)
			return;
	}

	CTokenVector TokenVector;
	if (!SplitToken(c_szCommand, &TokenVector))
		return;
	if (TokenVector.empty())
		return;

	const char * szCmd = TokenVector[0].c_str();
	
	if (!strcmpi(szCmd, "quit"))
	{
		PostQuitMessage(0);
	}
	// GIFT NOTIFY
	else if (!strcmpi(szCmd, "gift"))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "Gift_Show"); 	
	}
	// CUBE
	else if (!strcmpi(szCmd, "cube"))
	{
		if (TokenVector.size() < 2)
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		if ("open" == TokenVector[1])
		{
			if (3 > TokenVector.size())
			{
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand=%s) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
				return;
			}

			uint32_t npcVNUM = (uint32_t)atoi(TokenVector[2].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Open", npcVNUM);
		}
		else if ("close" == TokenVector[1])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Close");
		}
		else if ("info" == TokenVector[1])
		{
			if (5 != TokenVector.size())
			{
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
				return;
			}

			UINT gold = atoi(TokenVector[2].c_str());
			UINT itemVnum = atoi(TokenVector[3].c_str());
			UINT count = atoi(TokenVector[4].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_UpdateInfo", gold, itemVnum, count);
		}
		else if ("success" == TokenVector[1])
		{
			if (4 != TokenVector.size())
			{
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
				return;
			}

			UINT itemVnum = atoi(TokenVector[2].c_str());
			UINT count = atoi(TokenVector[3].c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Succeed", itemVnum, count);
		}
		else if ("fail" == TokenVector[1])
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_Failed");
		}
		else if ("r_list" == TokenVector[1])
		{
			// result list (/cube r_list npcVNUM resultCount resultText)
			// 20383 4 72723,1/72725,1/72730.1/50001,5 <- 이런식으로 "/" 문자로 구분된 리스트를 줌
			if (5 != TokenVector.size())
			{
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
				return;
			}

			uint32_t npcVNUM = (uint32_t)atoi(TokenVector[2].c_str());

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_ResultList", npcVNUM, TokenVector[4]);
		}
		else if ("m_info" == TokenVector[1])
		{
			// material list (/cube m_info requestStartIndex resultCount MaterialText)
			// ex) requestStartIndex: 0, resultCount : 5 - 해당 NPC가 만들수 있는 아이템 중 0~4번째에 해당하는 아이템을 만드는 데 필요한 모든 재료들이 MaterialText에 들어있음
			// 위 예시처럼 아이템이 다수인 경우 구분자 "@" 문자를 사용
			// 0 5 125,1|126,2|127,2|123,5&555,5&555,4/120000 <- 이런식으로 서버에서 클라로 리스트를 줌

			if (5 != TokenVector.size())
			{
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
				return;
			}

			UINT requestStartIndex = (UINT)atoi(TokenVector[2].c_str());
			UINT resultCount = (UINT)atoi(TokenVector[3].c_str());

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Cube_MaterialInfo", requestStartIndex, resultCount, TokenVector[4]);
		}
	}
	// CUEBE_END
	else if (!strcmpi(szCmd, "ObserverCount"))
	{
		if (2 != TokenVector.size())
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		UINT uObserverCount= atoi(TokenVector[1].c_str());				

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_BettingGuildWar_UpdateObserverCount", uObserverCount);
	}
	else if (!strcmpi(szCmd, "ObserverMode"))
	{		
		if (2 != TokenVector.size())
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		UINT uMode= atoi(TokenVector[1].c_str());
		
		auto rkPlayer=CPythonPlayer::GetInstance();
		rkPlayer->SetObserverMode(uMode ? true : false);

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_BettingGuildWar_SetObserverMode", uMode);
	}
	else if (!strcmpi(szCmd, "ObserverTeamInfo"))
	{		
	}
	else if (!strcmpi(szCmd, "StoneDetect"))
	{
		if (4 != TokenVector.size())
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		// vid distance(1-3) angle(0-360)
		uint32_t dwVID = atoi(TokenVector[1].c_str());
		uint8_t byDistance = atoi(TokenVector[2].c_str());
		float fAngle = atof(TokenVector[3].c_str());
		fAngle = fmod(540.0f - fAngle, 360.0f);
		TraceLog("StoneDetect [VID:{}] [Distance:{}] [Angle:{}->{}]\n", dwVID, byDistance, TokenVector[3], fAngle);

		auto rkChrMgr=CPythonCharacterManager::GetInstance();

		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(dwVID);
		if (!pInstance)
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Not Exist Instance", c_szCommand);
			return;
		}

		TPixelPosition PixelPosition;
		D3DXVECTOR3 v3Rotation(0.0f, 0.0f, fAngle);
		pInstance->NEW_GetPixelPosition(&PixelPosition);

		PixelPosition.y *= -1.0f;

		switch (byDistance)
		{
			case 0:
				CEffectManager::GetInstance()->RegisterEffect("d:/ymir work/effect/etc/firecracker/find_out.mse");
				CEffectManager::GetInstance()->CreateEffect("d:/ymir work/effect/etc/firecracker/find_out.mse", PixelPosition, v3Rotation);
				break;
			case 1:
				CEffectManager::GetInstance()->RegisterEffect("d:/ymir work/effect/etc/compass/appear_small.mse");
				CEffectManager::GetInstance()->CreateEffect("d:/ymir work/effect/etc/compass/appear_small.mse", PixelPosition, v3Rotation);
				break;
			case 2:
				CEffectManager::GetInstance()->RegisterEffect("d:/ymir work/effect/etc/compass/appear_middle.mse");
				CEffectManager::GetInstance()->CreateEffect("d:/ymir work/effect/etc/compass/appear_middle.mse", PixelPosition, v3Rotation);
				break;
			case 3:
				CEffectManager::GetInstance()->RegisterEffect("d:/ymir work/effect/etc/compass/appear_large.mse");
				CEffectManager::GetInstance()->CreateEffect("d:/ymir work/effect/etc/compass/appear_large.mse", PixelPosition, v3Rotation);
				break;
			default:
				TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Distance", c_szCommand);
				break;
		}

#ifdef _DEBUG
		auto rkChat=CPythonChat::GetInstance();
		rkChat->AppendChat(CHAT_TYPE_INFO, c_szCommand);
#endif
	}
	else if (!strcmpi(szCmd, "StartStaminaConsume"))
	{
		if (3 != TokenVector.size())
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		uint32_t dwConsumePerSec = atoi(TokenVector[1].c_str());
		uint32_t dwCurrentStamina = atoi(TokenVector[2].c_str());

		auto rPlayer = CPythonPlayer::GetInstance();
		rPlayer->StartStaminaConsume(dwConsumePerSec, dwCurrentStamina);
		return;
	}
	
	else if (!strcmpi(szCmd, "StopStaminaConsume"))
	{
		if (2 != TokenVector.size())
		{
			TraceLog("CPythonNetworkStream::ServerCommand(c_szCommand={}) - Strange Parameter Count : {}", c_szCommand, TokenVector.size());
			return;
		}

		uint32_t dwCurrentStamina = atoi(TokenVector[1].c_str());

		auto rPlayer = CPythonPlayer::GetInstance();
		rPlayer->StopStaminaConsume(dwCurrentStamina);
		return;
	}
	else if (!strcmpi(szCmd, "messenger_auth"))
	{
		const std::string & c_rstrName = TokenVector[1].c_str();
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnMessengerAddFriendQuestion", c_rstrName);
	}
	else if (!strcmpi(szCmd, "combo"))
	{
		int32_t iFlag = atoi(TokenVector[1].c_str());
		auto rPlayer = CPythonPlayer::GetInstance();
		rPlayer->SetComboSkillFlag(iFlag);
		m_bComboSkillFlag = iFlag ? true : false;
	}
	else if (!strcmpi(szCmd, "setblockmode"))
	{
		int32_t iFlag = atoi(TokenVector[1].c_str());
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnBlockMode", iFlag);
	}
	// Emotion Start
	else if (!strcmpi(szCmd, "french_kiss"))
	{
		int32_t iVID1 = atoi(TokenVector[1].c_str());
		int32_t iVID2 = atoi(TokenVector[2].c_str());

		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance1 = rkChrMgr->GetInstancePtr(iVID1);
		CInstanceBase * pInstance2 = rkChrMgr->GetInstancePtr(iVID2);
		if (pInstance1 && pInstance2)
			pInstance1->ActDualEmotion(*pInstance2, MOTION_FRENCH_KISS_START, MOTION_FRENCH_KISS_START);
	}
	else if (!strcmpi(szCmd, "kiss"))
	{
		int32_t iVID1 = atoi(TokenVector[1].c_str());
		int32_t iVID2 = atoi(TokenVector[2].c_str());

		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance1 = rkChrMgr->GetInstancePtr(iVID1);
		CInstanceBase * pInstance2 = rkChrMgr->GetInstancePtr(iVID2);
		if (pInstance1 && pInstance2)
			pInstance1->ActDualEmotion(*pInstance2, MOTION_KISS_START, MOTION_KISS_START);
	}
	else if (!strcmpi(szCmd, "slap"))
	{
		int32_t iVID1 = atoi(TokenVector[1].c_str());
		int32_t iVID2 = atoi(TokenVector[2].c_str());

		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance1 = rkChrMgr->GetInstancePtr(iVID1);
		CInstanceBase * pInstance2 = rkChrMgr->GetInstancePtr(iVID2);
		if (pInstance1 && pInstance2)
			pInstance1->ActDualEmotion(*pInstance2, MOTION_SLAP_HURT_START, MOTION_SLAP_HIT_START);
	}
	else if (!strcmpi(szCmd, "clap"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_CLAP);
	}
	else if (!strcmpi(szCmd, "cheer1"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_CHEERS_1);
	}
	else if (!strcmpi(szCmd, "cheer2"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_CHEERS_2);
	}
	else if (!strcmpi(szCmd, "dance1"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_DANCE_1);
	}
	else if (!strcmpi(szCmd, "dance2"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_DANCE_2);
	}
	else if (!strcmpi(szCmd, "dig_motion"))
	{
		int32_t iVID = atoi(TokenVector[1].c_str());
		auto rkChrMgr = CPythonCharacterManager::GetInstance();
		CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);
		if (pInstance)
			pInstance->ActEmotion(MOTION_DIG);
	}
	// Emotion End
	else
	{
		static std::map<std::string, int32_t> s_emotionDict;

		static bool s_isFirst = true;
		if (s_isFirst)
		{
			s_isFirst = false;

			s_emotionDict["dance3"] = MOTION_DANCE_3;
			s_emotionDict["dance4"] = MOTION_DANCE_4;
			s_emotionDict["dance5"] = MOTION_DANCE_5;
			s_emotionDict["dance6"] = MOTION_DANCE_6;
			s_emotionDict["congratulation"] = MOTION_CONGRATULATION;
			s_emotionDict["forgive"] = MOTION_FORGIVE;
			s_emotionDict["angry"] = MOTION_ANGRY;
			s_emotionDict["attractive"] = MOTION_ATTRACTIVE;
			s_emotionDict["sad"] = MOTION_SAD;
			s_emotionDict["shy"] = MOTION_SHY;
			s_emotionDict["cheerup"] = MOTION_CHEERUP;
			s_emotionDict["banter"] = MOTION_BANTER;
			s_emotionDict["joy"] = MOTION_JOY;
		}

		std::map<std::string, int32_t>::iterator f = s_emotionDict.find(szCmd);
		if (f == s_emotionDict.end())
		{
			TraceLog("Unknown Server Command {} | {}", c_szCommand, szCmd);
		}
		else
		{
			int32_t emotionIndex = f->second;

			int32_t iVID = atoi(TokenVector[1].c_str());
			auto rkChrMgr = CPythonCharacterManager::GetInstance();
			CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(iVID);

			if (pInstance)
				pInstance->ActEmotion(emotionIndex);
		}		
	}
}
