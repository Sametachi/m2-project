#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "war_map.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "item.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "guild_manager.h"
#include "buffer_manager.h"
#include "db.h"
#include "packet.h"
#include "locale_service.h"

EVENTINFO(war_map_info)
{
	int32_t iStep;
	CWarMap* pWarMap;

	war_map_info() 
	: iStep(0)
	, pWarMap(0)
	{
	}
};

EVENTFUNC(war_begin_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("war_begin_event> <Factor> Null pointer");
		return 0;
	}

	CWarMap* pMap = info->pWarMap;
	pMap->CheckWarEnd();
	return PASSES_PER_SEC(10);
}

EVENTFUNC(war_end_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("war_end_event> <Factor> Null pointer");
		return 0;
	}

	CWarMap* pMap = info->pWarMap;

	if (info->iStep == 0)
	{
		++info->iStep;
		pMap->ExitAll();
		return PASSES_PER_SEC(5);
	}
	else
	{
		pMap->SetEndEvent(nullptr);
		CWarMapManager::GetInstance()->DestroyWarMap(pMap);
		return 0;
	}
}

EVENTFUNC(war_timeout_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("war_timeout_event> <Factor> Null pointer");
		return 0;
	}

	CWarMap* pMap = info->pWarMap;
	pMap->Timeout();
	return 0;
}

void CWarMap::STeamData::Initialize()
{
	dwID = 0;
	pGuild = nullptr;
	iMemberCount = 0;
	iUsePotionPrice = 0;
	iScore = 0;
	pChrFlag = nullptr;
	pChrFlagBase = nullptr;

	set_pidJoiner.clear();
}

CWarMap::CWarMap(int32_t lMapIndex, const TGuildWarInfo& r_info, TWarMapInfo* pWarMapInfo, uint32_t dwGuildID1, uint32_t dwGuildID2)
{
	m_kMapInfo =* pWarMapInfo;
	m_kMapInfo.lMapIndex = lMapIndex;

	memcpy(&m_WarInfo, &r_info, sizeof(TGuildWarInfo));

	m_TeamData[0].Initialize();
	m_TeamData[0].dwID = dwGuildID1;
	m_TeamData[0].pGuild = CGuildManager::GetInstance()->TouchGuild(dwGuildID1);

	m_TeamData[1].Initialize();
	m_TeamData[1].dwID = dwGuildID2;
	m_TeamData[1].pGuild = CGuildManager::GetInstance()->TouchGuild(dwGuildID2);
	m_iObserverCount = 0;

	war_map_info* info = AllocEventInfo<war_map_info>();
	info->pWarMap = this;

	SetBeginEvent(event_create(war_begin_event, info, PASSES_PER_SEC(60)));
	m_pEndEvent = nullptr;
	m_pTimeoutEvent = nullptr;
	m_pResetFlagEvent = nullptr;
	m_bTimeout = false;
	m_dwStartTime = get_dword_time();
	m_bEnded = false;

	if (GetType() == WAR_MAP_TYPE_FLAG)
	{
		AddFlagBase(0);
		AddFlagBase(1);
		AddFlag(0);
		AddFlag(1);
	}
}

CWarMap::~CWarMap()
{
	event_cancel(&m_pBeginEvent);
	event_cancel(&m_pEndEvent);
	event_cancel(&m_pTimeoutEvent);
	event_cancel(&m_pResetFlagEvent);

	PyLog("WarMap::~WarMap : map index {}", GetMapIndex());

	auto it = m_set_pChr.begin();

	while (it != m_set_pChr.end())
	{
		LPCHARACTER ch = *(it++);

		if (ch->GetDesc())
		{
			PyLog("WarMap::~WarMap : disconnecting {}", ch->GetName());
			DESC_MANAGER::GetInstance()->DestroyDesc(ch->GetDesc());
		}
	}

	m_set_pChr.clear();
}

void CWarMap::SetBeginEvent(LPEVENT pEv)
{
	if (m_pBeginEvent != nullptr) {
		event_cancel(&m_pBeginEvent);
	}
	if (pEv != nullptr) {
		m_pBeginEvent = pEv;
	}
}

void CWarMap::SetEndEvent(LPEVENT pEv)
{
	if (m_pEndEvent != nullptr) {
		event_cancel(&m_pEndEvent);
	}
	if (pEv != nullptr) {
		m_pEndEvent = pEv;
	}
}

void CWarMap::SetTimeoutEvent(LPEVENT pEv)
{
	if (m_pTimeoutEvent != nullptr) {
		event_cancel(&m_pTimeoutEvent);
	}
	if (pEv != nullptr) {
		m_pTimeoutEvent = pEv;
	}
}

void CWarMap::SetResetFlagEvent(LPEVENT pEv)
{
	if (m_pResetFlagEvent != nullptr) {
		event_cancel(&m_pResetFlagEvent);
	}
	if (pEv != nullptr) {
		m_pResetFlagEvent = pEv;
	}
}

bool CWarMap::GetTeamIndex(uint32_t dwGuildID, uint8_t & bIdx)
{
	if (m_TeamData[0].dwID == dwGuildID)
	{
		bIdx = 0;
		return true;
	}
	else if (m_TeamData[1].dwID == dwGuildID)
	{
		bIdx = 1;
		return true;
	}

	return false;
}

uint32_t CWarMap::GetGuildID(uint8_t bIdx)
{
	assert(bIdx < 2);
	return m_TeamData[bIdx].dwID;
}

CGuild * CWarMap::GetGuild(uint8_t bIdx)
{
	return m_TeamData[bIdx].pGuild;
}

int32_t CWarMap::GetMapIndex()
{
	return m_kMapInfo.lMapIndex;
}

uint8_t CWarMap::GetType()
{
	return m_kMapInfo.bType;
}

uint32_t CWarMap::GetGuildOpponent(LPCHARACTER ch)
{
	if (ch->GetGuild())
	{
		uint32_t gid = ch->GetGuild()->GetID();
		uint8_t idx;

		if (GetTeamIndex(gid, idx))
			return m_TeamData[!idx].dwID;
	}
	return 0;
}

uint32_t CWarMap::GetWinnerGuild()
{
	uint32_t win_gid = 0;

	if (m_TeamData[1].iScore > m_TeamData[0].iScore)
	{
		win_gid = m_TeamData[1].dwID;
	}
	else if (m_TeamData[0].iScore > m_TeamData[1].iScore)
	{
		win_gid = m_TeamData[0].dwID;
	}

	return (win_gid);
}

void CWarMap::UsePotion(LPCHARACTER ch, LPITEM item)
{
	if (m_pEndEvent)
		return;

	if (ch->IsObserverMode())
		return;

	if (!ch->GetGuild())
		return;

	if (!item->GetProto())
		return;

	int32_t iPrice = item->GetProto()->dwIBuyItemPrice;

	uint32_t gid = ch->GetGuild()->GetID();

	if (gid == m_TeamData[0].dwID)
		m_TeamData[0].iUsePotionPrice += iPrice;
	else if (gid == m_TeamData[1].dwID)
		m_TeamData[1].iUsePotionPrice += iPrice;
}

int32_t CWarMap::STeamData::GetAccumulatedJoinerCount()
{
	return set_pidJoiner.size();
}

int32_t CWarMap::STeamData::GetCurJointerCount()
{
	return iMemberCount;
}

void CWarMap::STeamData::AppendMember(LPCHARACTER ch)
{
	set_pidJoiner.insert(ch->GetPlayerID());
	++iMemberCount;
}

void CWarMap::STeamData::RemoveMember(LPCHARACTER ch)
{
	--iMemberCount;
}


struct FSendUserCount
{
	char buf1[30];
	char buf2[128];

	FSendUserCount(uint32_t g1, int32_t g1_count, uint32_t g2, int32_t g2_count, int32_t observer)
	{
		snprintf(buf1, sizeof(buf1), "ObserverCount %d", observer);
		snprintf(buf2, sizeof(buf2), "WarUC %u %d %u %d %d", g1, g1_count, g2, g2_count, observer);
	}

	void operator() (LPCHARACTER ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf1);
		ch->ChatPacket(CHAT_TYPE_COMMAND, buf2);
	}
};

void CWarMap::UpdateUserCount()
{
	FSendUserCount f(
			m_TeamData[0].dwID, 
			m_TeamData[0].GetAccumulatedJoinerCount(), 
			m_TeamData[1].dwID, 
			m_TeamData[1].GetAccumulatedJoinerCount(), 
			m_iObserverCount);

	std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
}

void CWarMap::IncMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	PyLog("WarMap::IncMember");
	uint32_t gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	bool isWarMember = ch->GetQuestFlag("war.is_war_member") > 0 ? true : false;

	if (isWarMember && gid != m_TeamData[0].dwID && gid != m_TeamData[1].dwID)
	{
		ch->SetQuestFlag("war.is_war_member", 0);
		isWarMember = false;
	}

	if (isWarMember)
	{
		if (gid == m_TeamData[0].dwID)
		{
			m_TeamData[0].AppendMember(ch);

		}
		else if (gid == m_TeamData[1].dwID)
		{
			m_TeamData[1].AppendMember(ch);

		}

		event_cancel(&m_pTimeoutEvent);

		PyLog("WarMap +m {}(cur:{}, acc:{}) vs {}(cur:{}, acc:{})",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());
	}
	else
	{
		++m_iObserverCount; 
		PyLog("WarMap +o {}", m_iObserverCount);
		ch->SetObserverMode(true);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can participate in the guild battle in viewer mode."));
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("While choosing a character, an icon appears."));
	}

	UpdateUserCount();

	m_set_pChr.insert(ch);

	LPDESC d = ch->GetDesc();

	SendWarPacket(d);
	SendScorePacket(0, d);
	SendScorePacket(1, d);
}

void CWarMap::DecMember(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	PyLog("WarMap::DecMember");
	uint32_t gid = 0;

	if (ch->GetGuild())
		gid = ch->GetGuild()->GetID();

	if (!ch->IsObserverMode())
	{
		if (gid == m_TeamData[0].dwID)
			m_TeamData[0].RemoveMember(ch);
		else if (gid == m_TeamData[1].dwID)
			m_TeamData[1].RemoveMember(ch);

		if (m_kMapInfo.bType == WAR_MAP_TYPE_FLAG)
		{
			CAffect* pAff = ch->FindAffect(AFFECT_WAR_FLAG);

			if (pAff)
			{
				uint8_t idx;

				if (GetTeamIndex(pAff->lApplyValue, idx))
					AddFlag(idx, ch->GetX(), ch->GetY());

				ch->RemoveAffect(AFFECT_WAR_FLAG);
			}
		}

		PyLog("WarMap -m {}(cur:{}, acc:{}) vs {}(cur:{}, acc:{})",
				m_TeamData[0].dwID, m_TeamData[0].GetCurJointerCount(), m_TeamData[0].GetAccumulatedJoinerCount(),
				m_TeamData[1].dwID, m_TeamData[1].GetCurJointerCount(), m_TeamData[1].GetAccumulatedJoinerCount());

		CheckWarEnd();
		ch->SetQuestFlag("war.is_war_member", 0);
	}
	else
	{
		--m_iObserverCount;

		PyLog("WarMap -o {}", m_iObserverCount);
		ch->SetObserverMode(false);
	}

	UpdateUserCount();

	m_set_pChr.erase(ch);
}

struct FExitGuildWar
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->IsPC())
		{
			ch->ExitToSavedLocation();
		}
	}
};

void CWarMap::ExitAll()
{
	FExitGuildWar f;
	std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
}

void CWarMap::CheckWarEnd()
{
	if (m_bEnded)
		return;

	if (m_TeamData[0].iMemberCount == 0 || m_TeamData[1].iMemberCount == 0)
	{
		if (m_bTimeout)
			return;

		if (m_pTimeoutEvent)
			return;

		Notice(LC_TEXT("There are no opponents."));
		Notice(LC_TEXT("If no enemy can be found, the guild war will be ended automatically."));

		PyLog("CheckWarEnd: Timeout begin {} vs {}", m_TeamData[0].dwID, m_TeamData[1].dwID);

		war_map_info* info = AllocEventInfo<war_map_info>();
		info->pWarMap = this;

		SetTimeoutEvent(event_create(war_timeout_event, info, PASSES_PER_SEC(60)));
	}
	else
		CheckScore();
}

int32_t CWarMap::GetRewardGold(uint8_t bWinnerIdx)
{
	int32_t iRewardGold = m_WarInfo.iWarPrice;
	iRewardGold += (m_TeamData[bWinnerIdx].iUsePotionPrice * m_WarInfo.iWinnerPotionRewardPctToWinner) / 100;
	iRewardGold += (m_TeamData[bWinnerIdx ? 0 : 1].iUsePotionPrice * m_WarInfo.iLoserPotionRewardPctToWinner) / 100;
	return iRewardGold;
}

void CWarMap::Draw()
{
	CGuildManager::GetInstance()->RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, 0, 0);
}

void CWarMap::Timeout()
{
	SetTimeoutEvent(nullptr);

	if (m_bTimeout)
		return;

	if (m_bEnded)
		return;

	uint32_t dwWinner = 0;
	uint32_t dwLoser = 0;
	int32_t iRewardGold = 0;

	if (get_dword_time() - m_dwStartTime < 60000 * 5)
	{
		Notice(LC_TEXT("Because the guild war finished early, the result will judged as a draw."));
		dwWinner = 0;
		dwLoser = 0;
	}
	else
	{
		int32_t iWinnerIdx = -1;

		if (m_TeamData[0].iMemberCount == 0)
			iWinnerIdx = 1;
		else if (m_TeamData[1].iMemberCount == 0)
			iWinnerIdx = 0;

		if (iWinnerIdx == -1)
		{
			dwWinner = GetWinnerGuild();

			if (dwWinner == m_TeamData[0].dwID)
			{
				iRewardGold = GetRewardGold(0);
				dwLoser = m_TeamData[1].dwID;
			}
			else if (dwWinner == m_TeamData[1].dwID)
			{
				iRewardGold = GetRewardGold(1);
				dwLoser = m_TeamData[0].dwID;
			}

			SysLog("WarMap: member count is not zero, guild1 {} {} guild2 {} {}, winner {}",
					m_TeamData[0].dwID, m_TeamData[0].iMemberCount,
					m_TeamData[1].dwID, m_TeamData[1].iMemberCount,
					dwWinner);
		}
		else
		{
			dwWinner = m_TeamData[iWinnerIdx].dwID;
			dwLoser = m_TeamData[iWinnerIdx == 0 ? 1 : 0].dwID;

			iRewardGold = GetRewardGold(iWinnerIdx);
		}
	}

	PyLog("WarMap: Timeout {} {} winner {} loser {} reward {} map {}",
			m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, dwLoser, iRewardGold, m_kMapInfo.lMapIndex);

	if (dwWinner)
		CGuildManager::GetInstance()->RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold);
	else
		CGuildManager::GetInstance()->RequestWarOver(m_TeamData[0].dwID, m_TeamData[1].dwID, dwWinner, iRewardGold);

	m_bTimeout = true;
}

namespace
{
	struct FPacket
	{
		FPacket(const void* p, int32_t size) : m_pvData(p), m_iSize(size)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			ch->GetDesc()->Packet(m_pvData, m_iSize);
		}

		const void* m_pvData;
		int32_t m_iSize;
	};

	struct FNotice
	{
		FNotice(const char* psz) : m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char* m_psz;
	};
};

void CWarMap::Notice(const char* psz)
{
	FNotice f(psz);
	std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
}

void CWarMap::Packet(const void* p, int32_t size)
{
	FPacket f(p, size);
	std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);
}

void CWarMap::SendWarPacket(LPDESC d)
{
	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_WAR;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.dwGuildSelf	= m_TeamData[0].dwID;
	pack2.dwGuildOpp	= m_TeamData[1].dwID;
	pack2.bType		= CGuildManager::GetInstance()->TouchGuild(m_TeamData[0].dwID)->GetGuildWarType(m_TeamData[1].dwID);
	pack2.bWarState	= CGuildManager::GetInstance()->TouchGuild(m_TeamData[0].dwID)->GetGuildWarState(m_TeamData[1].dwID);

	d->BufferedPacket(&pack, sizeof(pack));
	d->Packet(&pack2, sizeof(pack2));
}

void CWarMap::SendScorePacket(uint8_t bIdx, LPDESC d)
{
	TPacketGCGuild p;

	p.header = HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
	p.size = sizeof(p) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int32_t);

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(&m_TeamData[bIdx].dwID, sizeof(uint32_t));
	buf.write(&m_TeamData[bIdx ? 0 : 1].dwID, sizeof(uint32_t));
	buf.write(&m_TeamData[bIdx].iScore, sizeof(int32_t));

	if (d)
		d->Packet(buf.read_peek(), buf.size());
	else
		Packet(buf.read_peek(), buf.size());
}

void CWarMap::UpdateScore(uint32_t g1, int32_t score1, uint32_t g2, int32_t score2)
{
	uint8_t idx;

	if (GetTeamIndex(g1, idx))
	{
		if (m_TeamData[idx].iScore != score1)
		{
			m_TeamData[idx].iScore = score1;
			SendScorePacket(idx);
		}
	}

	if (GetTeamIndex(g2, idx))
	{
		if (m_TeamData[idx].iScore != score2)
		{
			m_TeamData[idx].iScore = score2;
			SendScorePacket(idx);
		}
	}

	CheckScore();
}

bool CWarMap::CheckScore()
{
	if (m_bEnded)
		return true;

	if (get_dword_time() - m_dwStartTime < 30000)
		return false;

	if (m_TeamData[0].iScore == m_TeamData[1].iScore)
		return false;

	int32_t iEndScore = m_WarInfo.iEndScore;

	if (test_server) iEndScore /= 10;

	uint32_t dwWinner;
	uint32_t dwLoser;

	if (m_TeamData[0].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[0].dwID;
		dwLoser = m_TeamData[1].dwID;
	}
	else if (m_TeamData[1].iScore >= iEndScore)
	{
		dwWinner = m_TeamData[1].dwID;
		dwLoser = m_TeamData[0].dwID;
	}
	else
		return false;

	int32_t iRewardGold = 0;

	if (dwWinner == m_TeamData[0].dwID)
		iRewardGold = GetRewardGold(0);
	else if (dwWinner == m_TeamData[1].dwID)
		iRewardGold = GetRewardGold(1);

	PyLog("WarMap::CheckScore end score {} guild1 {} score guild2 {} {} score {} winner {} reward {}", 
			iEndScore,
			m_TeamData[0].dwID,
			m_TeamData[0].iScore,
			m_TeamData[1].dwID,
			m_TeamData[1].iScore,
			dwWinner,
			iRewardGold);

	CGuildManager::GetInstance()->RequestWarOver(dwWinner, dwLoser, dwWinner, iRewardGold);
	return true;
}

bool CWarMap::SetEnded()
{
	PyLog("WarMap::SetEnded {}", m_kMapInfo.lMapIndex);

	if (m_pEndEvent)
		return false;

	if (m_TeamData[0].pChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pChrFlag);
		m_TeamData[0].pChrFlag = nullptr;
	}

	if (m_TeamData[0].pChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[0].pChrFlagBase);
		m_TeamData[0].pChrFlagBase = nullptr;
	}

	if (m_TeamData[1].pChrFlag)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pChrFlag);
		m_TeamData[1].pChrFlag = nullptr;
	}

	if (m_TeamData[1].pChrFlagBase)
	{
		M2_DESTROY_CHARACTER(m_TeamData[1].pChrFlagBase);
		m_TeamData[1].pChrFlagBase = nullptr;
	}

	event_cancel(&m_pResetFlagEvent);
	m_bEnded = true;

	war_map_info* info = AllocEventInfo<war_map_info>();

	info->pWarMap = this;
	info->iStep = 0;
	SetEndEvent(event_create(war_end_event, info, PASSES_PER_SEC(10)));
	return true;
}

void CWarMap::OnKill(LPCHARACTER killer, LPCHARACTER ch)
{
	if (m_bEnded)
		return;

	uint32_t dwKillerGuild = 0;
	uint32_t dwDeadGuild = 0;

	if (killer->GetGuild())
		dwKillerGuild = killer->GetGuild()->GetID();

	if (ch->GetGuild())
		dwDeadGuild = ch->GetGuild()->GetID();

	uint8_t idx;

	PyLog("WarMap::OnKill {} {}", dwKillerGuild, dwDeadGuild);

	if (!GetTeamIndex(dwKillerGuild, idx))
		return;

	if (!GetTeamIndex(dwDeadGuild, idx))
		return;

	switch (m_kMapInfo.bType)
	{
		case WAR_MAP_TYPE_NORMAL:
			SendGuildWarScore(dwKillerGuild, dwDeadGuild, 1, ch->GetLevel());
			break;

		case WAR_MAP_TYPE_FLAG:
			{
				CAffect* pAff = ch->FindAffect(AFFECT_WAR_FLAG);

				if (pAff)
				{
					if (GetTeamIndex(pAff->lApplyValue, idx))
						AddFlag(idx, ch->GetX(), ch->GetY());

					ch->RemoveAffect(AFFECT_WAR_FLAG);
				}
			}
			break;

		default:
			SysLog("unknown war map type {} index {}", m_kMapInfo.bType, m_kMapInfo.lMapIndex);
			break;
	}
}

void CWarMap::AddFlagBase(uint8_t bIdx, uint32_t x, uint32_t y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData& r = m_TeamData[bIdx];

	if (r.pChrFlagBase)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pChrFlagBase = CHARACTER_MANAGER::GetInstance()->SpawnMob(warmap::WAR_FLAG_BASE_VNUM, m_kMapInfo.lMapIndex, x, y, 0);

	r.pChrFlagBase->SetPoint(POINT_STAT, r.dwID);
	r.pChrFlagBase->SetWarMap(this);
}

void CWarMap::AddFlag(uint8_t bIdx, uint32_t x, uint32_t y)
{
	if (m_bEnded)
		return;

	assert(bIdx < 2);

	TeamData& r = m_TeamData[bIdx];

	if (r.pChrFlag)
		return;

	if (x == 0)
	{
		x = m_kMapInfo.posStart[bIdx].x;
		y = m_kMapInfo.posStart[bIdx].y;
	}

	r.pChrFlag = CHARACTER_MANAGER::GetInstance()->SpawnMob(bIdx == 0 ? warmap::WAR_FLAG_VNUM0 : warmap::WAR_FLAG_VNUM1, m_kMapInfo.lMapIndex, x, y, 0);
	r.pChrFlag->SetPoint(POINT_STAT, r.dwID);
	r.pChrFlag->SetWarMap(this);
}

void CWarMap::RemoveFlag(uint8_t bIdx)
{
	assert(bIdx < 2);

	TeamData& r = m_TeamData[bIdx];

	if (!r.pChrFlag)
		return;

	r.pChrFlag->Dead(NULL, true);
	r.pChrFlag = nullptr;
}

bool CWarMap::IsFlagOnBase(uint8_t bIdx)
{
	assert(bIdx < 2);

	TeamData& r = m_TeamData[bIdx];

	if (!r.pChrFlag)
		return false;

	const PIXEL_POSITION & pos = r.pChrFlag->GetXYZ();

	if (pos.x == m_kMapInfo.posStart[bIdx].x && pos.y == m_kMapInfo.posStart[bIdx].y)
		return true;

	return false;
}

EVENTFUNC(war_reset_flag_event)
{
	war_map_info* info = dynamic_cast<war_map_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("war_reset_flag_event> <Factor> Null pointer");
		return 0;
	}

	CWarMap* pMap = info->pWarMap;

	pMap->AddFlag(0);
	pMap->AddFlag(1);

	pMap->SetResetFlagEvent(nullptr);
	return 0;
}

struct FRemoveFlagAffect
{
	void operator() (LPCHARACTER ch)
	{
		if (ch->FindAffect(AFFECT_WAR_FLAG))
			ch->RemoveAffect(AFFECT_WAR_FLAG);
	}
};

void CWarMap::ResetFlag()
{
	if (m_kMapInfo.bType != WAR_MAP_TYPE_FLAG)
		return;

	if (m_pResetFlagEvent)
		return;

	if (m_bEnded)
		return;

	FRemoveFlagAffect f;
	std::for_each(m_set_pChr.begin(), m_set_pChr.end(), f);

	RemoveFlag(0);
	RemoveFlag(1);

	war_map_info* info = AllocEventInfo<war_map_info>();

	info->pWarMap = this;
	info->iStep = 0;
	SetResetFlagEvent(event_create(war_reset_flag_event, info, PASSES_PER_SEC(10)));
}

/////////////////////////////////////////////////////////////////////////////////
// WarMapManager
/////////////////////////////////////////////////////////////////////////////////
CWarMapManager::CWarMapManager()
{
}

CWarMapManager::~CWarMapManager()
{
	for(std::map<int32_t, TWarMapInfo *>::const_iterator iter = m_map_kWarMapInfo.begin() ; iter != m_map_kWarMapInfo.end() ; ++iter)
	{
		M2_DELETE(iter->second);
	}

	m_map_kWarMapInfo.clear();
}

bool CWarMapManager::LoadWarMapInfo(const char* c_pszFileName)
{
	TWarMapInfo * k;

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_NORMAL;

	k->lMapIndex = 110;
	k->posStart[0].x = 48 * 100 + 32000;
	k->posStart[0].y = 52 * 100 + 0;
	k->posStart[1].x = 183 * 100 + 32000;
	k->posStart[1].y = 206 * 100 + 0;
	k->posStart[2].x = 141 * 100 + 32000;
	k->posStart[2].y = 117 * 100 + 0;

	m_map_kWarMapInfo.insert(std::make_pair(k->lMapIndex, k));

	k = M2_NEW TWarMapInfo;
	k->bType = WAR_MAP_TYPE_FLAG;

	k->lMapIndex = 111;
	k->posStart[0].x = 68 * 100 + 57600;
	k->posStart[0].y = 69 * 100 + 0;
	k->posStart[1].x = 171 * 100 + 57600;
	k->posStart[1].y = 182 * 100 + 0; 
	k->posStart[2].x = 122 * 100 + 57600;
	k->posStart[2].y = 131 * 100 + 0;

	m_map_kWarMapInfo.insert(std::make_pair(k->lMapIndex, k));
	return true;
}

bool CWarMapManager::IsWarMap(int32_t lMapIndex)
{
	return GetWarMapInfo(lMapIndex) ? true : false;
}

bool CWarMapManager::GetStartPosition(int32_t lMapIndex, uint8_t bIdx, PIXEL_POSITION & pos)
{
	assert(bIdx < 3);

	TWarMapInfo* pi = GetWarMapInfo(lMapIndex);

	if (!pi)
	{
		PyLog("GetStartPosition FAILED [{}] WarMapInfoSize({})", lMapIndex, m_map_kWarMapInfo.size());

		for (auto it = m_map_kWarMapInfo.begin(); it != m_map_kWarMapInfo.end(); ++it)
		{
			PIXEL_POSITION& cur=it->second->posStart[bIdx];
			PyLog("WarMap[{}]=Pos({}, {})", it->first, cur.x, cur.y);
		}
		return false;
	}

	pos = pi->posStart[bIdx];
	return true;
}

int32_t CWarMapManager::CreateWarMap(const TGuildWarInfo& guildWarInfo, uint32_t dwGuildID1, uint32_t dwGuildID2)
{
	TWarMapInfo* pInfo = GetWarMapInfo(guildWarInfo.lMapIndex);
	if (!pInfo)
	{
		SysLog("GuildWar.CreateWarMap.NOT_FOUND_MAPINFO[{}]", guildWarInfo.lMapIndex);
		return 0;
	}

	uint32_t lMapIndex = SECTREE_MANAGER::GetInstance()->CreatePrivateMap(guildWarInfo.lMapIndex);

	if (lMapIndex)
	{
		m_mapWarMap.insert(std::make_pair(lMapIndex, M2_NEW CWarMap(lMapIndex, guildWarInfo, pInfo, dwGuildID1, dwGuildID2)));
	}

	return lMapIndex;
}

TWarMapInfo * CWarMapManager::GetWarMapInfo(int32_t lMapIndex)
{
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	auto it = m_map_kWarMapInfo.find(lMapIndex);

	if (m_map_kWarMapInfo.end() == it)
		return NULL;

	return it->second;
}

void CWarMapManager::DestroyWarMap(CWarMap* pMap)
{
	int32_t mapIdx = pMap->GetMapIndex();

	PyLog("WarMap::DestroyWarMap : {}", mapIdx);

	m_mapWarMap.erase(pMap->GetMapIndex());
	M2_DELETE(pMap);

	SECTREE_MANAGER::GetInstance()->DestroyPrivateMap(mapIdx);
}

CWarMap * CWarMapManager::Find(int32_t lMapIndex)
{
	auto it = m_mapWarMap.find(lMapIndex);

	if (it == m_mapWarMap.end())
		return NULL;

	return it->second;
}

void CWarMapManager::OnShutdown()
{
	auto it = m_mapWarMap.begin();

	while (it != m_mapWarMap.end())
		(it++)->second->Draw();
}

