#include "stdafx.h"
#include <cmath>
#include "GuildManager.h"
#include "Main.h"
#include "ClientManager.h"
#include "QID.h"
#include "Config.h"

extern std::string g_stLocale;

const int32_t GUILD_RANK_MAX_NUM = 20;
const int32_t GUILD_WAR_WAIT_START_DURATION = 60;
const int32_t GUILD_WAR_RESERVE_SECONDS = 180;

namespace 
{
	struct FSendPeerWar
	{
		FSendPeerWar(uint8_t bType, uint8_t bWar, uint32_t GID1, uint32_t GID2)
		{
			if (number(0, 1))
				std::swap(GID1, GID2);

			memset(&p, 0, sizeof(TPacketGuildWar));

			p.bWar = bWar;
			p.bType = bType;
			p.dwGuildFrom = GID1;
			p.dwGuildTo = GID2;
		}

		void operator() (LPPEER pPeer)
		{
			if (pPeer->GetChannel() == 0)
				return;

			pPeer->EncodeHeader(HEADER_DG_GUILD_WAR, 0, sizeof(TPacketGuildWar));
			pPeer->Encode(&p, sizeof(TPacketGuildWar));
		}

		TPacketGuildWar p;
	};

	struct FSendGuildWarScore
	{
		FSendGuildWarScore(uint32_t guild_gain, uint32_t dwOppGID, int32_t iScore, int32_t iBetScore)
		{
			pck.dwGuildGainPoint = guild_gain;
			pck.dwGuildOpponent = dwOppGID;
			pck.lScore = iScore;
			pck.lBetScore = iBetScore;
		}

		void operator() (LPPEER pPeer)
		{
			if (pPeer->GetChannel() == 0)
				return;

			pPeer->EncodeHeader(HEADER_DG_GUILD_WAR_SCORE, 0, sizeof(pck));
			pPeer->Encode(&pck, sizeof(pck));
		}

		TPacketGuildWarScore pck;
	};
}

CGuildManager::CGuildManager()
{
}

CGuildManager::~CGuildManager()
{
	while (!m_pqOnWar.empty())
	{
		if (!m_pqOnWar.top().second->bEnd)
			delete m_pqOnWar.top().second;

		m_pqOnWar.pop();
	}
}

TGuild& CGuildManager::TouchGuild(uint32_t GID)
{
	auto it = m_map_kGuild.find(GID);

	if (it != m_map_kGuild.end())
		return it->second;

	TGuild info;
	m_map_kGuild.emplace(GID, info);
	return m_map_kGuild[GID];
}

void CGuildManager::ParseResult(SQLResult* pRes)
{
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes->pSQLResult)))
	{
		uint32_t GID = strtoul(row[0], nullptr, 10);

		TGuild& r_info = TouchGuild(GID);

		strlcpy(r_info.szName, row[1], sizeof(r_info.szName));
		str_to_number(r_info.ladder_point, row[2]);
		str_to_number(r_info.win, row[3]);
		str_to_number(r_info.draw, row[4]);
		str_to_number(r_info.loss, row[5]);
		str_to_number(r_info.gold, row[6]);
		str_to_number(r_info.level, row[7]);

		PyLog(
				"GuildWar: {} ladder {} win {} draw {} loss {}", 
				r_info.szName,
				r_info.ladder_point,
				r_info.win,
				r_info.draw,
				r_info.loss);
	}
}

void CGuildManager::Initialize()
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "SELECT id, name, ladder_point, win, draw, loss, gold, level FROM guild%s", GetTablePostfix());
	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows)
		ParseResult(pmsg->Get());

	char str[128 + 1];

	if (!CConfig::GetInstance()->GetValue("POLY_POWER", str, sizeof(str)))
		*str = '\0';

	if (!polyPower.Analyze(str))
	{
		SysLog("cannot set power poly: {}", str);
	}
	else
	{
		TraceLog("POWER_POLY: {}", str);
	}

	if (!CConfig::GetInstance()->GetValue("POLY_HANDICAP", str, sizeof(str)))
		*str = '\0';

	if (!polyHandicap.Analyze(str))
	{
		SysLog("cannot set handicap poly: {}", str);
	}
	else
	{
		TraceLog("HANDICAP_POLY: {}", str);
	}

	QueryRanking();
}

void CGuildManager::Load(uint32_t dwGuildID)
{
	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "SELECT id, name, ladder_point, win, draw, loss, gold, level FROM guild%s WHERE id=%u", GetTablePostfix(), dwGuildID);
	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows)
		ParseResult(pmsg->Get());
}

void CGuildManager::QueryRanking()
{
	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT id,name,ladder_point FROM guild%s ORDER BY ladder_point DESC LIMIT 20", GetTablePostfix());

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_GUILD_RANKING, 0, nullptr);
}

int32_t CGuildManager::GetRanking(uint32_t dwGID)
{
	auto it = map_kLadderPointRankingByGID.find(dwGID);

	if (it == map_kLadderPointRankingByGID.end())
		return GUILD_RANK_MAX_NUM;

	return MINMAX(0, it->second, GUILD_RANK_MAX_NUM);
}

void CGuildManager::ResultRanking(MYSQL_RES* pRes)
{
	if (!pRes)
		return;

	int32_t iLastLadderPoint = -1;
	int32_t iRank = 0;

	map_kLadderPointRankingByGID.clear();

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes)))
	{
		uint32_t	dwGID = 0; str_to_number(dwGID, row[0]);
		int32_t	iLadderPoint = 0; str_to_number(iLadderPoint, row[2]);

		if (iLadderPoint != iLastLadderPoint)
			++iRank;

		TraceLog("GUILD_RANK: {} {} {}", row[1], iRank, iLadderPoint);

		map_kLadderPointRankingByGID.emplace(dwGID, iRank);
	}
}

void CGuildManager::Update()
{
	ProcessReserveWar();

	time_t now = CClientManager::GetInstance()->GetCurrentTime();

	if (!m_pqOnWar.empty())
	{
		while (!m_pqOnWar.empty() && (m_pqOnWar.top().first <= now || (m_pqOnWar.top().second && m_pqOnWar.top().second->bEnd)))
		{
			TGuildWarPQElement* e = m_pqOnWar.top().second;

			m_pqOnWar.pop();

			if (e)
			{
				if (!e->bEnd)
					WarEnd(e->GID[0], e->GID[1], false); 

				delete e;
			}
		}
	}

	while (!m_pqSkill.empty() && m_pqSkill.top().first <= now)
	{
		const TGuildSkillUsed& s = m_pqSkill.top().second;
		CClientManager::GetInstance()->SendGuildSkillUsable(s.GID, s.dwSkillVnum, true);
		m_pqSkill.pop();
	}

	while (!m_pqWaitStart.empty() && m_pqWaitStart.top().first <= now)
	{
		const TGuildWaitStartInfo& ws = m_pqWaitStart.top().second;
		m_pqWaitStart.pop();

		StartWar(ws.bType, ws.GID[0], ws.GID[1], ws.pReserve);

		if (ws.lInitialScore)
		{
			UpdateScore(ws.GID[0], ws.GID[1], ws.lInitialScore, 0);
			UpdateScore(ws.GID[1], ws.GID[0], ws.lInitialScore, 0);
		}

		TPacketGuildWar p;

		p.bType		= ws.bType;
		p.bWar		= GUILD_WAR_ON_WAR;
		p.dwGuildFrom	= ws.GID[0];
		p.dwGuildTo	= ws.GID[1];

		CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR, &p, sizeof(p));
		PyLog("GuildWar: GUILD sending start of wait start war {} {}", ws.GID[0], ws.GID[1]);
	}
}

#define for_all(cont, it) for (auto it = (cont).begin(); it != (cont).end(); ++it)

void CGuildManager::OnSetup(LPPEER pPeer)
{
	for_all(m_WarMap, it_cont)
		for_all(it_cont->second, it)
		{
			uint32_t g1 = it_cont->first;
			uint32_t g2 = it->first;
			TGuildWarPQElement* p = it->second.pElement;

			if (!p || p->bEnd)
				continue;

			FSendPeerWar(p->bType, GUILD_WAR_ON_WAR, g1, g2) (pPeer);
			FSendGuildWarScore(p->GID[0], p->GID[1], p->iScore[0], p->iBetScore[0]);
			FSendGuildWarScore(p->GID[1], p->GID[0], p->iScore[1], p->iBetScore[1]);
		}

	for_all(m_DeclareMap, it)
	{
		FSendPeerWar(it->bType, GUILD_WAR_SEND_DECLARE, it->dwGuildID[0], it->dwGuildID[1]) (pPeer);
	}

	for_all(m_map_kWarReserve, it)
	{
		it->second->OnSetup(pPeer);
	}
}

void CGuildManager::GuildWarWin(uint32_t GID)
{
	auto it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.win;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET win=%d WHERE id=%u", GetTablePostfix(), it->second.win, GID);
	CDBManager::GetInstance()->AsyncQuery(buf);
}

void CGuildManager::GuildWarLose(uint32_t GID)
{
	auto it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.loss;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET loss=%d WHERE id=%u", GetTablePostfix(), it->second.loss, GID);
	CDBManager::GetInstance()->AsyncQuery(buf);
}

void CGuildManager::GuildWarDraw(uint32_t GID)
{
	auto it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	++it->second.draw;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET draw=%d WHERE id=%u", GetTablePostfix(), it->second.draw, GID);
	CDBManager::GetInstance()->AsyncQuery(buf);
}

bool CGuildManager::IsHalfWinLadderPoint(uint32_t dwGuildWinner, uint32_t dwGuildLoser)
{
	uint32_t GID1 = dwGuildWinner;
	uint32_t GID2 = dwGuildLoser;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	auto it = m_mapGuildWarEndTime[GID1].find(GID2);

	if (it != m_mapGuildWarEndTime[GID1].end() && 
			it->second + GUILD_WAR_LADDER_HALF_PENALTY_TIME > CClientManager::GetInstance()->GetCurrentTime())
		return true;

	return false;
}

void CGuildManager::ProcessDraw(uint32_t dwGuildID1, uint32_t dwGuildID2)
{
	TraceLog("GuildWar: \tThe war between {} and {} is ended in draw", dwGuildID1, dwGuildID2);

	GuildWarDraw(dwGuildID1);
	GuildWarDraw(dwGuildID2);
	ChangeLadderPoint(dwGuildID1, 0);
	ChangeLadderPoint(dwGuildID2, 0);

	QueryRanking();
}

void CGuildManager::ProcessWinLose(uint32_t dwGuildWinner, uint32_t dwGuildLoser)
{
	GuildWarWin(dwGuildWinner);
	GuildWarLose(dwGuildLoser);
	TraceLog("GuildWar: \tWinner : {} Loser : {}", dwGuildWinner, dwGuildLoser);

	int32_t iPoint = GetLadderPoint(dwGuildLoser);
	int32_t gain = (int32_t)(iPoint * 0.05);
	int32_t loss = (int32_t)(iPoint * 0.07);

	if (IsHalfWinLadderPoint(dwGuildWinner, dwGuildLoser))
		gain /= 2;

	TraceLog("GuildWar: \tgain : {} loss : {}", gain, loss);

	ChangeLadderPoint(dwGuildWinner, gain);
	ChangeLadderPoint(dwGuildLoser, -loss);

	QueryRanking();
}

void CGuildManager::RemoveWar(uint32_t GID1, uint32_t GID2)
{
	TraceLog("GuildWar: RemoveWar({}, {})", GID1, GID2);

	if (GID1 > GID2)
		std::swap(GID2, GID1);

	auto it = m_WarMap[GID1].find(GID2);

	if (it == m_WarMap[GID1].end())
	{
		if (m_WarMap[GID1].empty())
			m_WarMap.erase(GID1);

		return;
	}

	if (it->second.pElement)
		it->second.pElement->bEnd = true;

	m_mapGuildWarEndTime[GID1][GID2] = CClientManager::GetInstance()->GetCurrentTime();

	m_WarMap[GID1].erase(it);

	if (m_WarMap[GID1].empty())
		m_WarMap.erase(GID1);
}

void CGuildManager::WarEnd(uint32_t GID1, uint32_t GID2, bool bForceDraw)
{
	if (GID1 > GID2)
		std::swap(GID2, GID1);

	TraceLog("GuildWar: WarEnd {} {}", GID1, GID2);

	auto itWarMap = m_WarMap[GID1].find(GID2);

	if (itWarMap == m_WarMap[GID1].end())
	{
		SysLog("GuildWar: war not exist or already ended. [1]");
		return;
	}

	TGuildWarInfo gwi = itWarMap->second;
	TGuildWarPQElement* pData = gwi.pElement;

	if (!pData || pData->bEnd)
	{
		SysLog("GuildWar: war not exist or already ended. [2]");
		return;
	}

	uint32_t win_guild = pData->GID[0];
	uint32_t lose_guild = pData->GID[1];

	bool bDraw = false;

	if (!bForceDraw) // If it is not my draw, the score is checked.
	{
		if (pData->iScore[0] > pData->iScore[1])
		{
			win_guild = pData->GID[0];
			lose_guild = pData->GID[1];
		}
		else if (pData->iScore[1] > pData->iScore[0])
		{
			win_guild = pData->GID[1];
			lose_guild = pData->GID[0];
		}
		else
			bDraw = true;
	}
	else // In case of forced draw, unconditional draw
		bDraw = true;

	if (bDraw)
		ProcessDraw(win_guild, lose_guild);
	else
		ProcessWinLose(win_guild, lose_guild);

	// Sometimes the DB server terminates itself, so a separate packet has to be sent.
	CClientManager::GetInstance()->for_each_peer(FSendPeerWar(0, GUILD_WAR_END, GID1, GID2));

	RemoveWar(GID1, GID2);
}

void CGuildManager::RecvWarOver(uint32_t dwGuildWinner, uint32_t dwGuildLoser, bool bDraw, int32_t lWarPrice)
{
	TraceLog("GuildWar: RecvWarOver : winner {} vs {} draw? {} war_price {}", dwGuildWinner, dwGuildLoser, bDraw, lWarPrice);

	uint32_t GID1 = dwGuildWinner;
	uint32_t GID2 = dwGuildLoser;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	auto it = m_WarMap[GID1].find(GID2);

	if (it == m_WarMap[GID1].end())
		return;

	TGuildWarInfo& gw = it->second;

	if (bDraw)
	{
		// Split bet money to both guilds
		DepositMoney(dwGuildWinner, lWarPrice / 2);
		DepositMoney(dwGuildLoser, lWarPrice / 2);
		ProcessDraw(dwGuildWinner, dwGuildLoser);
	}
	else
	{
		// Give bet money to winner guild
		DepositMoney(dwGuildWinner, lWarPrice);
		ProcessWinLose(dwGuildWinner, dwGuildLoser);
	}

	if (gw.pReserve)
	{
		if (bDraw || !gw.pElement)
			gw.pReserve->Draw();
		else if (gw.pElement->bType == GUILD_WAR_TYPE_BATTLE)
			gw.pReserve->End(gw.pElement->iBetScore[0], gw.pElement->iBetScore[1]);
	}

	RemoveWar(GID1, GID2);
}

void CGuildManager::RecvWarEnd(uint32_t GID1, uint32_t GID2)
{
	TraceLog("GuildWar: RecvWarEnded : {} vs {}", GID1, GID2);
	WarEnd(GID1, GID2, true);
}

void CGuildManager::StartWar(uint8_t bType, uint32_t GID1, uint32_t GID2, CGuildWarReserve* pReserve)
{
	TraceLog("GuildWar: StartWar({},{},{})", bType, GID1, GID2);

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	TGuildWarInfo& gw = m_WarMap[GID1][GID2];

	if (bType == GUILD_WAR_TYPE_FIELD)
		gw.tEndTime = CClientManager::GetInstance()->GetCurrentTime() + GUILD_WAR_DURATION;
	else
		gw.tEndTime = CClientManager::GetInstance()->GetCurrentTime() + 172800;

	gw.pElement = new TGuildWarPQElement(bType, GID1, GID2);
	gw.pReserve = pReserve;

	m_pqOnWar.push(std::make_pair(gw.tEndTime, gw.pElement));
}

void CGuildManager::UpdateScore(uint32_t dwGainGID, uint32_t dwOppGID, int32_t iScoreDelta, int32_t iBetScoreDelta)
{
	uint32_t GID1 = dwGainGID;
	uint32_t GID2 = dwOppGID;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	auto it = m_WarMap[GID1].find(GID2);

	if (it != m_WarMap[GID1].end())
	{
		TGuildWarPQElement* p = it->second.pElement;

		if (!p || p->bEnd)
		{
			SysLog("GuildWar: war not exist or already ended.");
			return;
		}

		int32_t iNewScore = 0;
		int32_t iNewBetScore = 0;

		if (p->GID[0] == dwGainGID)
		{
			p->iScore[0] += iScoreDelta;
			p->iBetScore[0] += iBetScoreDelta;

			iNewScore = p->iScore[0];
			iNewBetScore = p->iBetScore[0];
		}
		else
		{
			p->iScore[1] += iScoreDelta;
			p->iBetScore[1] += iBetScoreDelta;

			iNewScore = p->iScore[1];
			iNewBetScore = p->iBetScore[1];
		}

		TraceLog("GuildWar: SendGuildWarScore guild {} wartype {} score_delta {} betscore_delta {} result {}, {}",
				dwGainGID, p->bType, iScoreDelta, iBetScoreDelta, iNewScore, iNewBetScore);

		CClientManager::GetInstance()->for_each_peer(FSendGuildWarScore(dwGainGID, dwOppGID, iNewScore, iNewBetScore));
	}
}

void CGuildManager::AddDeclare(uint8_t bType, uint32_t guild_from, uint32_t guild_to)
{
	TGuildDeclareInfo di(bType, guild_from, guild_to);

	if (m_DeclareMap.find(di) == m_DeclareMap.end())
		m_DeclareMap.insert(di);

	TraceLog("GuildWar: AddDeclare(Type:{},from:{},to:{})", bType, guild_from, guild_to);
}

void CGuildManager::RemoveDeclare(uint32_t guild_from, uint32_t guild_to)
{
	auto it = m_DeclareMap.find(TGuildDeclareInfo(0, guild_from, guild_to));

	if (it != m_DeclareMap.end())
		m_DeclareMap.erase(it);

	it = m_DeclareMap.find(TGuildDeclareInfo(0,guild_to, guild_from));

	if (it != m_DeclareMap.end())
		m_DeclareMap.erase(it);

	TraceLog("GuildWar: RemoveDeclare(from:{},to:{})", guild_from, guild_to);
}

bool CGuildManager::TakeBetPrice(uint32_t dwGuildTo, uint32_t dwGuildFrom, int32_t lWarPrice)
{
	auto it_from = m_map_kGuild.find(dwGuildFrom);
	auto it_to = m_map_kGuild.find(dwGuildTo);

	if (it_from == m_map_kGuild.end() || it_to == m_map_kGuild.end())
	{
		SysLog("TakeBetPrice: guild not exist {} {}",
				dwGuildFrom, dwGuildTo);
		return false;
	}

	if (it_from->second.gold < lWarPrice || it_to->second.gold < lWarPrice)
	{
		TraceLog("TakeBetPrice: not enough gold {} {} to {} {}", 
				dwGuildFrom, it_from->second.gold, dwGuildTo, it_to->second.gold);
		return false;
	}

	it_from->second.gold -= lWarPrice;
	it_to->second.gold -= lWarPrice;

	MoneyChange(dwGuildFrom, it_from->second.gold);
	MoneyChange(dwGuildTo, it_to->second.gold);
	return true;
}

bool CGuildManager::WaitStart(TPacketGuildWar* p)
{
	if (p->lWarPrice > 0)
		if (!TakeBetPrice(p->dwGuildFrom, p->dwGuildTo, p->lWarPrice))
			return false;

	uint32_t dwCurTime = CClientManager::GetInstance()->GetCurrentTime();

	TGuildWaitStartInfo info(p->bType, p->dwGuildFrom, p->dwGuildTo, p->lWarPrice, p->lInitialScore, nullptr);
	m_pqWaitStart.push(std::make_pair(dwCurTime + GUILD_WAR_WAIT_START_DURATION, info));

	PyLog(
			"GuildWar: WaitStart g1 {} g2 {} price {} start at {}",
			p->dwGuildFrom,
			p->dwGuildTo,
			p->lWarPrice,
			dwCurTime + GUILD_WAR_WAIT_START_DURATION);

	return true;
}

int32_t CGuildManager::GetLadderPoint(uint32_t GID)
{
	auto it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return 0;

	return it->second.ladder_point;
}

void CGuildManager::ChangeLadderPoint(uint32_t GID, int32_t change)
{
	auto it = m_map_kGuild.find(GID);

	if (it == m_map_kGuild.end())
		return;

	TGuild& r = it->second;

	r.ladder_point += change;

	if (r.ladder_point < 0)
		r.ladder_point = 0;

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET ladder_point=%d WHERE id=%u", GetTablePostfix(), r.ladder_point, GID);
	CDBManager::GetInstance()->AsyncQuery(buf);

	TraceLog("GuildManager::ChangeLadderPoint {} {} {}", GID, r.ladder_point, buf);

	TPacketGuildLadder p;

	p.dwGuild = GID;
	p.lLadderPoint = r.ladder_point;
	p.lWin = r.win;
	p.lDraw = r.draw;
	p.lLoss = r.loss;

	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_LADDER, &p, sizeof(TPacketGuildLadder));
}

void CGuildManager::UseSkill(uint32_t GID, uint32_t dwSkillVnum, uint32_t dwCooltime)
{
	TraceLog("UseSkill(gid={}, skill={}) CoolTime({}:{})", GID, dwSkillVnum, dwCooltime, CClientManager::GetInstance()->GetCurrentTime() + dwCooltime);
	m_pqSkill.push(std::make_pair(CClientManager::GetInstance()->GetCurrentTime() + dwCooltime, TGuildSkillUsed(GID, dwSkillVnum)));
}

void CGuildManager::MoneyChange(uint32_t dwGuild, uint32_t dwGold)
{
	TraceLog("GuildManager::MoneyChange {} {}", dwGuild, dwGold);

	TPacketDGGuildMoneyChange p;
	p.dwGuild = dwGuild;
	p.iTotalGold = dwGold;
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_MONEY_CHANGE, &p, sizeof(p));

	char buf[1024];
	snprintf(buf, sizeof(buf), "UPDATE guild%s SET gold=%u WHERE id = %u", GetTablePostfix(), dwGold, dwGuild);
	CDBManager::GetInstance()->AsyncQuery(buf);
}

void CGuildManager::DepositMoney(uint32_t dwGuild, int32_t iGold)
{
	if (iGold <= 0)
		return;

	auto it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
	{
		SysLog("No guild by id {}", dwGuild);
		return;
	}

	it->second.gold += iGold;
	PyLog("GUILD: {} Deposit {} Total {}", dwGuild, iGold, it->second.gold);

	MoneyChange(dwGuild, it->second.gold);
}

void CGuildManager::WithdrawMoney(LPPEER pPeer, uint32_t dwGuild, int32_t iGold)
{
	auto it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
	{
		SysLog("No guild by id {}", dwGuild);
		return;
	}

	if (it->second.gold >= iGold)
	{
		it->second.gold -= iGold;
		PyLog("GUILD: {} Withdraw {} Total {}", dwGuild, iGold, it->second.gold);

		TPacketDGGuildMoneyWithdraw p;
		p.dwGuild = dwGuild;
		p.iChangeGold = iGold;

		pPeer->EncodeHeader(HEADER_DG_GUILD_WITHDRAW_MONEY_GIVE, 0, sizeof(TPacketDGGuildMoneyWithdraw));
		pPeer->Encode(&p, sizeof(TPacketDGGuildMoneyWithdraw));
	}
}

void CGuildManager::WithdrawMoneyReply(uint32_t dwGuild, uint8_t bGiveSuccess, int32_t iGold)
{
	auto it = m_map_kGuild.find(dwGuild);

	if (it == m_map_kGuild.end())
		return;

	TraceLog("GuildManager::WithdrawMoneyReply : guild {} success {} gold {}", dwGuild, bGiveSuccess, iGold);

	if (!bGiveSuccess)
		it->second.gold += iGold;
	else
		MoneyChange(dwGuild, it->second.gold);
}

const int32_t c_aiScoreByLevel[GUILD_MAX_LEVEL+1] =
{
	500,	
	500,	// 1
	1000,
	2000,
	3000,
	4000,
	6000,
	8000,
	10000,
	12000,
	15000,	// 10
	18000,
	21000,
	24000,
	28000,
	32000,
	36000,
	40000,
	45000,
	50000,
	55000,	// 20
};

const int32_t c_aiScoreByRanking[GUILD_RANK_MAX_NUM+1] =
{
	0,
	55000,	// 1
	50000,
	45000,
	40000,
	36000,
	32000,
	28000,
	24000,
	21000,
	18000,	// 10
	15000,
	12000,
	10000,
	8000,
	6000,
	4000,
	3000,
	2000,
	1000,
	500		// 20
};

void CGuildManager::BootReserveWar()
{
	const char* c_apszQuery[2] = 
	{
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap FROM guild_war_reservation WHERE started=1 AND winner=-1",
		"SELECT id, guild1, guild2, UNIX_TIMESTAMP(time), type, warprice, initscore, bet_from, bet_to, power1, power2, handicap FROM guild_war_reservation WHERE started=0"
	};

	for (int32_t i = 0; i < 2; ++i)
	{
		std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(c_apszQuery[i]));

		if (pmsg->Get()->uiNumRows == 0)
			continue;

		MYSQL_ROW row;

		while ((row = mysql_fetch_row(pmsg->Get()->pSQLResult)))
		{
			int32_t col = 0;

			TGuildWarReserve t;

			str_to_number(t.dwID, row[col++]);
			str_to_number(t.dwGuildFrom, row[col++]);
			str_to_number(t.dwGuildTo, row[col++]);
			str_to_number(t.dwTime, row[col++]);
			str_to_number(t.bType, row[col++]);
			str_to_number(t.lWarPrice, row[col++]);
			str_to_number(t.lInitialScore, row[col++]);
			str_to_number(t.dwBetFrom, row[col++]);
			str_to_number(t.dwBetTo, row[col++]);
			str_to_number(t.lPowerFrom, row[col++]);
			str_to_number(t.lPowerTo, row[col++]);
			str_to_number(t.lHandicap, row[col++]);
			t.bStarted = false;

			CGuildWarReserve* pReserve = new CGuildWarReserve(t);

			char buf[512];
			snprintf(buf, sizeof(buf), "GuildWar: BootReserveWar : step %d id %u GID1 %u GID2 %u", i, t.dwID, t.dwGuildFrom, t.dwGuildTo);
			
			// If i == 0, the DB bounced during the guild battle, so it is treated as a draw.
			// Alternatively, the remaining reservation guild battles with less than 5 minutes remaining will be treated as a draw. (They return their bets)
				
			if (i == 0 || (int32_t) t.dwTime - CClientManager::GetInstance()->GetCurrentTime() < 0)
			{
				if (i == 0)
				{
					PyLog("{} : DB was shutdowned while war is being.", buf);
				}
				else
				{
					PyLog("{} : left time lower than 5 minutes, will be canceled", buf);
				}

				pReserve->Draw();
				delete pReserve;
			}
			else
			{
				PyLog("{} : OK", buf);
				m_map_kWarReserve.emplace(t.dwID, pReserve);
			}
		}
	}
}

int32_t GetAverageGuildMemberLevel(uint32_t dwGID)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery), 
			"SELECT AVG(level) FROM guild_member%s, player%s AS p WHERE guild_id=%u AND guild_member%s.pid=p.id", 
			GetTablePostfix(), GetTablePostfix(), dwGID, GetTablePostfix());

	std::unique_ptr<SQLMsg> msg(CDBManager::GetInstance()->DirectQuery(szQuery));

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);

	int32_t nAverageLevel = 0; str_to_number(nAverageLevel, row[0]);
	return nAverageLevel;
}

int32_t GetGuildMemberCount(uint32_t dwGID)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM guild_member%s WHERE guild_id=%u", GetTablePostfix(), dwGID);

	std::unique_ptr<SQLMsg> msg(CDBManager::GetInstance()->DirectQuery(szQuery));

	MYSQL_ROW row;
	row = mysql_fetch_row(msg->Get()->pSQLResult);

	uint32_t dwCount = 0; str_to_number(dwCount, row[0]);
	return dwCount;
}

bool CGuildManager::ReserveWar(TPacketGuildWar* p)
{
	uint32_t GID1 = p->dwGuildFrom;
	uint32_t GID2 = p->dwGuildTo;

	if (GID1 > GID2)
		std::swap(GID1, GID2);

	if (p->lWarPrice > 0)
		if (!TakeBetPrice(GID1, GID2, p->lWarPrice))
			return false;

	TGuildWarReserve t;
	memset(&t, 0, sizeof(TGuildWarReserve));

	t.dwGuildFrom = GID1;
	t.dwGuildTo = GID2;
	t.dwTime = CClientManager::GetInstance()->GetCurrentTime() + GUILD_WAR_RESERVE_SECONDS;
	t.bType = p->bType;
	t.lWarPrice = p->lWarPrice;
	t.lInitialScore = p->lInitialScore;

	int32_t lvp, rkp, alv, mc;

	TGuild& k1 = TouchGuild(GID1);

	lvp = c_aiScoreByLevel[MIN(GUILD_MAX_LEVEL, k1.level)];
	rkp = c_aiScoreByRanking[GetRanking(GID1)];
	alv = GetAverageGuildMemberLevel(GID1);
	mc = GetGuildMemberCount(GID1);

	polyPower.SetVar("lvp", lvp);
	polyPower.SetVar("rkp", rkp);
	polyPower.SetVar("alv", alv);
	polyPower.SetVar("mc", mc);

	t.lPowerFrom = (int32_t) polyPower.Eval();
	TraceLog("GuildWar: {} lvp {} rkp {} alv {} mc {} power {}", GID1, lvp, rkp, alv, mc, t.lPowerFrom);

	TGuild& k2 = TouchGuild(GID2);

	lvp = c_aiScoreByLevel[MIN(GUILD_MAX_LEVEL, k2.level)];
	rkp = c_aiScoreByRanking[GetRanking(GID2)];
	alv = GetAverageGuildMemberLevel(GID2);
	mc = GetGuildMemberCount(GID2);

	polyPower.SetVar("lvp", lvp);
	polyPower.SetVar("rkp", rkp);
	polyPower.SetVar("alv", alv);
	polyPower.SetVar("mc", mc);

	t.lPowerTo = (int32_t) polyPower.Eval();
	TraceLog("GuildWar: {} lvp {} rkp {} alv {} mc {} power {}", GID2, lvp, rkp, alv, mc, t.lPowerTo);

	if (t.lPowerTo > t.lPowerFrom)
	{
		polyHandicap.SetVar("pA", t.lPowerTo);
		polyHandicap.SetVar("pB", t.lPowerFrom);
	}
	else
	{
		polyHandicap.SetVar("pA", t.lPowerFrom);
		polyHandicap.SetVar("pB", t.lPowerTo);
	}

	t.lHandicap = (int32_t) polyHandicap.Eval();
	TraceLog("GuildWar: handicap {}", t.lHandicap);

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO guild_war_reservation (guild1, guild2, time, type, warprice, initscore, power1, power2, handicap) "
			"VALUES(%u, %u, DATE_ADD(NOW(), INTERVAL 180 SECOND), %u, %ld, %ld, %ld, %ld, %ld)",
			GID1, GID2, p->bType, p->lWarPrice, p->lInitialScore, t.lPowerFrom, t.lPowerTo, t.lHandicap);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiInsertID == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		SysLog("GuildWar: Cannot insert row");
		return false;
	}

	t.dwID = pmsg->Get()->uiInsertID;

	m_map_kWarReserve.emplace(t.dwID, new CGuildWarReserve(t));

	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_ADD, &t, sizeof(TGuildWarReserve));
	return true;
}

void CGuildManager::ProcessReserveWar()
{
	uint32_t dwCurTime = CClientManager::GetInstance()->GetCurrentTime();

	auto it = m_map_kWarReserve.begin();

	while (it != m_map_kWarReserve.end())
	{
		auto it2 = it++;

		CGuildWarReserve* p = it2->second;
		TGuildWarReserve& r = p->GetDataRef();

		if (!r.bStarted && r.dwTime - 1800 <= dwCurTime)
		{
			int32_t iMin = (int32_t) ceil((int32_t)(r.dwTime - dwCurTime) / 60.0);

			TGuild& r_1 = m_map_kGuild[r.dwGuildFrom];
			TGuild& r_2 = m_map_kGuild[r.dwGuildTo];

			TraceLog("GuildWar: started GID1 {} GID2 {} {} time {} min {}", r.dwGuildFrom, r.dwGuildTo, r.bStarted, dwCurTime - r.dwTime, iMin);

			if (iMin <= 0)
			{
				char szQuery[128];
				snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1 WHERE id=%u", r.dwID);
				CDBManager::GetInstance()->AsyncQuery(szQuery);

				CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_DEL, &r.dwID, sizeof(uint32_t));

				r.bStarted = true;

				TGuildWaitStartInfo info(r.bType, r.dwGuildFrom, r.dwGuildTo, r.lWarPrice, r.lInitialScore, p);
				m_pqWaitStart.push(std::make_pair(dwCurTime + GUILD_WAR_WAIT_START_DURATION, info));

				TPacketGuildWar pck;

				pck.bType = r.bType;
				pck.bWar = GUILD_WAR_WAIT_START;
				pck.dwGuildFrom = r.dwGuildFrom;
				pck.dwGuildTo = r.dwGuildTo;
				pck.lWarPrice = r.lWarPrice;
				pck.lInitialScore = r.lInitialScore;

				CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR, &pck, sizeof(TPacketGuildWar));
			}
			else
			{
				if (iMin != p->GetLastNoticeMin())
				{
					p->SetLastNoticeMin(iMin);

					CClientManager::GetInstance()->SendNotice("The Guild War between %s and %s will start in %d minutes!", r_1.szName, r_2.szName, iMin);
				}
			}
		}
	}
}

bool CGuildManager::Bet(uint32_t dwID, const char* c_pszLogin, uint32_t dwGold, uint32_t dwGuild)
{
	auto it = m_map_kWarReserve.find(dwID);

	char szQuery[1024];

	if (it == m_map_kWarReserve.end())
	{
		TraceLog("WAR_RESERVE: Bet: cannot find reserve war by id {}", dwID);
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES('%s', %d, %u, NOW())",
				c_pszLogin, ITEM::ELK_VNUM, dwGold);
		CDBManager::GetInstance()->AsyncQuery(szQuery);
		return false;
	}

	if (!it->second->Bet(c_pszLogin, dwGold, dwGuild))
	{
		TraceLog("WAR_RESERVE: Bet: cannot bet id {}, login {}, gold {}, guild {}", dwID, c_pszLogin, dwGold, dwGuild);
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES('%s', %d, %u, NOW())", 
				c_pszLogin, ITEM::ELK_VNUM, dwGold);
		CDBManager::GetInstance()->AsyncQuery(szQuery);
		return false;
	}

	return true;
}

void CGuildManager::CancelWar(uint32_t GID1, uint32_t GID2)
{
	RemoveDeclare(GID1, GID2);
	RemoveWar(GID1, GID2);
}

bool CGuildManager::ChangeMaster(uint32_t dwGID, uint32_t dwFrom, uint32_t dwTo)
{
	auto iter = m_map_kGuild.find(dwGID);

	if (iter == m_map_kGuild.end())
		return false;

	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild%s SET master=%u WHERE id=%u", GetTablePostfix(), dwTo, dwGID);
	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_member%s SET grade=1 WHERE pid=%u", GetTablePostfix(), dwTo);
	std::unique_ptr<SQLMsg> pSQLMsg2(CDBManager::GetInstance()->DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_member%s SET grade=15 WHERE pid=%u", GetTablePostfix(), dwFrom);
	std::unique_ptr<SQLMsg> pSQLMsg3(CDBManager::GetInstance()->DirectQuery(szQuery));

	return true;
}

CGuildWarReserve::CGuildWarReserve(const TGuildWarReserve& rTable)
{
	memcpy(&m_data, &rTable, sizeof(TGuildWarReserve));
	m_iLastNoticeMin = -1;

	Initialize();
}

void CGuildWarReserve::Initialize()
{
	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT login, guild, gold FROM guild_war_bet WHERE war_id=%u", m_data.dwID);

	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pSQLMsg->Get()->uiNumRows)
	{
		MYSQL_RES* res = pSQLMsg->Get()->pSQLResult;
		MYSQL_ROW row;

		char szLogin[LOGIN_MAX_LEN+1];
		uint32_t dwGuild;
		uint32_t dwGold;

		while ((row = mysql_fetch_row(res)))
		{
			dwGuild = dwGold = 0;
			strlcpy(szLogin, row[0], sizeof(szLogin));
			str_to_number(dwGuild, row[1]);
			str_to_number(dwGold, row[2]);

			mapBet.emplace(szLogin, std::make_pair(dwGuild, dwGold));
		}
	}
}

void CGuildWarReserve::OnSetup(LPPEER pPeer)
{
	if (m_data.bStarted)
		return;

	FSendPeerWar(m_data.bType, GUILD_WAR_RESERVE, m_data.dwGuildFrom, m_data.dwGuildTo) (pPeer);

	pPeer->EncodeHeader(HEADER_DG_GUILD_WAR_RESERVE_ADD, 0, sizeof(TGuildWarReserve));
	pPeer->Encode(&m_data, sizeof(TGuildWarReserve));

	TPacketGDGuildWarBet pckBet;
	pckBet.dwWarID = m_data.dwID;

	auto it = mapBet.begin();

	while (it != mapBet.end())
	{
		strlcpy(pckBet.szLogin, it->first.c_str(), sizeof(pckBet.szLogin));
		pckBet.dwGuild = it->second.first;
		pckBet.dwGold = it->second.second;

		pPeer->EncodeHeader(HEADER_DG_GUILD_WAR_BET, 0, sizeof(TPacketGDGuildWarBet));
		pPeer->Encode(&pckBet, sizeof(TPacketGDGuildWarBet));

		++it;
	}
}

bool CGuildWarReserve::Bet(const char* pszLogin, uint32_t dwGold, uint32_t dwGuild)
{
	char szQuery[1024];

	if (m_data.dwGuildFrom != dwGuild && m_data.dwGuildTo != dwGuild)
	{
		TraceLog("GuildWarReserve::Bet: invalid guild id");
		return false;
	}

	if (m_data.bStarted)
	{
		TraceLog("GuildWarReserve::Bet: war is already started");
		return false;
	}

	if (mapBet.find(pszLogin) != mapBet.end())
	{
		TraceLog("GuildWarReserve::Bet: failed. already bet");
		return false;
	}

	snprintf(szQuery, sizeof(szQuery), 
			"INSERT INTO guild_war_bet (war_id, login, gold, guild) VALUES(%u, '%s', %u, %u)",
			m_data.dwID, pszLogin, dwGold, dwGuild);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		SysLog("GuildWarReserve::Bet: failed. cannot insert row to guild_war_bet table");
		return false;
	}

	if (m_data.dwGuildFrom == dwGuild)
		m_data.dwBetFrom += dwGold;
	else
		m_data.dwBetTo += dwGold;

	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR_RESERVE_ADD, &m_data, sizeof(TGuildWarReserve));

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET bet_from=%u,bet_to=%u WHERE id=%u", 
			m_data.dwBetFrom, m_data.dwBetTo, m_data.dwID);

	CDBManager::GetInstance()->AsyncQuery(szQuery);

	TraceLog("GuildWarReserve::Bet: success. {} {} war_id {} bet {} : {}", pszLogin, dwGuild, m_data.dwID, m_data.dwBetFrom, m_data.dwBetTo);
	mapBet.emplace(pszLogin, std::make_pair(dwGuild, dwGold));

	TPacketGDGuildWarBet pckBet;
	pckBet.dwWarID = m_data.dwID;
	strlcpy(pckBet.szLogin, pszLogin, sizeof(pckBet.szLogin));
	pckBet.dwGuild = dwGuild;
	pckBet.dwGold = dwGold;

	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_GUILD_WAR_BET, &pckBet, sizeof(TPacketGDGuildWarBet));
	return true;
}

// Draw handling: In most cases, it is normal to win, but in certain situations such as server problems,
// There should be draw handling.
void CGuildWarReserve::Draw() 
{
	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1,winner=0 WHERE id=%u", m_data.dwID);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	if (mapBet.empty())
		return;

	PyLog("WAR_REWARD: Draw. war_id {}", m_data.dwID);

	auto it = mapBet.begin();

	while (true)
	{
		int32_t iLen = 0;
		int32_t iRow = 0;

		iLen += snprintf(szQuery, sizeof(szQuery) - iLen, "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES");

		while (it != mapBet.end())
		{
			if (iRow == 0)
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, "('%s', %d, %u, NOW())", 
						it->first.c_str(), ITEM::ELK_VNUM, it->second.second);
			else
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",('%s', %d, %u, NOW())", 
						it->first.c_str(), ITEM::ELK_VNUM, it->second.second);

			it++;

			if (iLen > 384)
				break;

			++iRow;
		}

		if (iRow > 0)
		{
			TraceLog("WAR_REWARD: QUERY: {}", szQuery);
			CDBManager::GetInstance()->AsyncQuery(szQuery);
		}

		if (it == mapBet.end())
			break;
	}
}

void CGuildWarReserve::End(int32_t iScoreFrom, int32_t iScoreTo)
{
	uint32_t dwWinner;

	SysLog("WAR_REWARD: End: From {} {} To {} {}", m_data.dwGuildFrom, iScoreFrom, m_data.dwGuildTo, iScoreTo);

	if (m_data.lPowerFrom > m_data.lPowerTo)
	{
		if (m_data.lHandicap > iScoreFrom - iScoreTo)
		{
			TraceLog("WAR_REWARD: End: failed to overcome handicap, From is strong but To won");
			dwWinner = m_data.dwGuildTo;
		}
		else
		{
			TraceLog("WAR_REWARD: End: success to overcome handicap, From win!");
			dwWinner = m_data.dwGuildFrom;
		}
	}
	else
	{
		if (m_data.lHandicap > iScoreTo - iScoreFrom) 
		{
			TraceLog("WAR_REWARD: End: failed to overcome handicap, To is strong but From won");
			dwWinner = m_data.dwGuildFrom;
		}
		else
		{
			TraceLog("WAR_REWARD: End: success to overcome handicap, To win!");
			dwWinner = m_data.dwGuildTo;
		}
	}

	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "UPDATE guild_war_reservation SET started=1,winner=%u,result1=%d,result2=%d WHERE id=%u", 
			dwWinner, iScoreFrom, iScoreTo, m_data.dwID);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	if (mapBet.empty())
		return;

	uint32_t dwTotalBet = m_data.dwBetFrom + m_data.dwBetTo;
	uint32_t dwWinnerBet = 0;

	if (dwWinner == m_data.dwGuildFrom)
		dwWinnerBet = m_data.dwBetFrom;
	else if (dwWinner == m_data.dwGuildTo)
		dwWinnerBet = m_data.dwBetTo;
	else
	{
		SysLog("WAR_REWARD: fatal error, winner does not exist!");
		return;
	}

	if (dwWinnerBet == 0)
	{
		SysLog("WAR_REWARD: total bet money on winner is zero");
		return;
	}

	PyLog("WAR_REWARD: End: Total bet: {}, Winner bet: {}", dwTotalBet, dwWinnerBet);

	auto it = mapBet.begin();

	while (true)
	{
		int32_t iLen = 0;
		int32_t iRow = 0;

		iLen += snprintf(szQuery, sizeof(szQuery) - iLen, "INSERT INTO item_award (login, vnum, socket0, given_time) VALUES");

		while (it != mapBet.end())
		{
			if (it->second.first != dwWinner)
			{
				++it;
				continue;
			}

			double ratio = (double) it->second.second / dwWinnerBet;

			// Distribution after 10% tax deduction
			TraceLog("WAR_REWARD: {} {} ratio {}", it->first.c_str(), it->second.second, ratio);

			uint32_t dwGold = (uint32_t) (dwTotalBet* ratio * 0.9);

			if (iRow == 0)
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, "('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM::ELK_VNUM, dwGold);
			else
				iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ",('%s', %d, %u, NOW())",
						it->first.c_str(), ITEM::ELK_VNUM, dwGold);

			++it;

			if (iLen > 384)
				break;

			++iRow;
		}

		if (iRow > 0)
		{
			TraceLog("WAR_REWARD: query: {}", szQuery);
			CDBManager::GetInstance()->AsyncQuery(szQuery);
		}

		if (it == mapBet.end())
			break;
	}
}

