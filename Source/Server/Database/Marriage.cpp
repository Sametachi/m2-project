#include "stdafx.h"
#include "Marriage.h"
#include "Main.h"
#include "DBManager.h"
#include "ClientManager.h"

const uint32_t WEDDING_LENGTH = 60 * 60; // sec
bool operator < (const TWedding& lhs, const TWedding& rhs)
{
	return lhs.dwTime < rhs.dwTime;
}

bool operator > (const TWedding& lhs, const TWedding& rhs)
{
	return lhs.dwTime > rhs.dwTime;
}

bool operator > (const TWeddingInfo &lhs, const TWeddingInfo& rhs)
{
	return lhs.dwTime > rhs.dwTime;
}

CMarriageManager::CMarriageManager()
{
}

CMarriageManager::~CMarriageManager()
{
}

bool CMarriageManager::Initialize()
{
	char szQuery[1024];

	snprintf(szQuery, sizeof(szQuery),
			"SELECT pid1, pid2, love_point, time, is_married, p1.name, p2.name FROM marriage, player%s as p1, player%s as p2 WHERE p1.id = pid1 AND p2.id = pid2",
			GetTablePostfix(), GetTablePostfix());

	std::unique_ptr<SQLMsg> pmsg_delete(CDBManager::GetInstance()->DirectQuery("DELETE FROM marriage WHERE is_married = 0"));
	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	SQLResult* pRes = pmsg->Get();
	TraceLog("MarriageList(size={})", pRes->uiNumRows);

	if (pRes->uiNumRows > 0)
	{
		for (uint uiRow = 0; uiRow != pRes->uiNumRows; ++uiRow)
		{
			MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);

			uint32_t pid1 = 0; str_to_number(pid1, row[0]);
			uint32_t pid2 = 0; str_to_number(pid2, row[1]);
			int32_t love_point = 0; str_to_number(love_point, row[2]);
			uint32_t time = 0; str_to_number(time, row[3]);
			uint8_t is_married = 0; str_to_number(is_married, row[4]);
			const char* name1 = row[5];
			const char* name2 = row[6];

			TMarriage* pMarriage = new TMarriage(pid1, pid2, love_point, time, is_married, name1, name2);
			m_Marriages.insert(pMarriage);
			m_MarriageByPID.emplace(pid1, pMarriage);
			m_MarriageByPID.emplace(pid2, pMarriage);

			TraceLog("Marriage {}: LP:{} TM:{} ST:{} {}:{} {}:{} ", uiRow, love_point, time, is_married, pid1, name1, pid2, name2);
		}
	}
	return true;
}

TMarriage* CMarriageManager::Get(uint32_t dwPlayerID)
{
	auto it = m_MarriageByPID.find(dwPlayerID);

	if (it != m_MarriageByPID.end())
		return it->second;

	return nullptr;
}

void Align(uint32_t& rPID1, uint32_t& rPID2)
{
	if (rPID1 > rPID2)
		std::swap(rPID1, rPID2);
}

void CMarriageManager::Add(uint32_t dwPID1, uint32_t dwPID2, const char* szName1, const char* szName2)
{
	uint32_t now = CClientManager::GetInstance()->GetCurrentTime();
	if (IsMarried(dwPID1) || IsMarried(dwPID2))
	{
		WarnLog("cannot marry already married character. {} - {}", dwPID1, dwPID2);
		return;
	}

	Align(dwPID1, dwPID2);

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO marriage(pid1, pid2, love_point, time) VALUES (%u, %u, 0, %u)", dwPID1, dwPID2, now);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	SQLResult* res = pmsg->Get();
	if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
	{
		WarnLog("cannot insert marriage");
		return;
	}

	TraceLog("MARRIAGE ADD {} {}", dwPID1, dwPID2);

	TMarriage* pMarriage = new TMarriage(dwPID1, dwPID2, 0, now, 0, szName1, szName2);
	m_Marriages.insert(pMarriage);
	m_MarriageByPID.emplace(dwPID1, pMarriage);
	m_MarriageByPID.emplace(dwPID2, pMarriage);

	TPacketMarriageAdd p;
	p.dwPID1 = dwPID1;
	p.dwPID2 = dwPID2;
	p.tMarryTime = now;
	strlcpy(p.szName1, szName1, sizeof(p.szName1));
	strlcpy(p.szName2, szName2, sizeof(p.szName2));
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_MARRIAGE_ADD, &p, sizeof(p));
}

void CMarriageManager::Update(uint32_t dwPID1, uint32_t dwPID2, int32_t iLovePoint, uint8_t byMarried)
{
	TMarriage* pMarriage = Get(dwPID1);
	if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
	{
		WarnLog("not under marriage : {} {}", dwPID1, dwPID2);
		return;
	}

	Align(dwPID1, dwPID2);

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "UPDATE marriage SET love_point = %d, is_married = %d WHERE pid1 = %u AND pid2 = %u", 
			iLovePoint, byMarried, pMarriage->pid1, pMarriage->pid2);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	SQLResult* res = pmsg->Get();
	if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
	{
		WarnLog("cannot update marriage : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	TraceLog("MARRIAGE UPDATE PID:{} {} LP:{} ST:{}", dwPID1, dwPID2, iLovePoint, byMarried);
	pMarriage->love_point = iLovePoint;
	pMarriage->is_married = byMarried;

	TPacketMarriageUpdate p;
	p.dwPID1 = dwPID1;
	p.dwPID2 = dwPID2;
	p.iLovePoint = pMarriage->love_point;
	p.byMarried = pMarriage->is_married;
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_MARRIAGE_UPDATE, &p, sizeof(p));
}

void CMarriageManager::Remove(uint32_t dwPID1, uint32_t dwPID2)
{
	TMarriage* pMarriage = Get(dwPID1);

	if (pMarriage)
	{
		TraceLog("Break Marriage pid1 {} pid2 {} Other {}", dwPID1, dwPID2, pMarriage->GetOther(dwPID1));
	}
	if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
	{
		auto it = m_MarriageByPID.begin();

		for (; it != m_MarriageByPID.end(); ++it)
		{
			TraceLog("Marriage List pid1 {} pid2 {}", it->second->pid1, it->second->pid2);
		}

		WarnLog("not under marriage : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	Align(dwPID1, dwPID2);

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM marriage WHERE pid1 = %u AND pid2 = %u", dwPID1, dwPID2);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	SQLResult* res = pmsg->Get();
	if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
	{
		WarnLog("cannot delete marriage : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	TraceLog("MARRIAGE REMOVE PID:{} {}", dwPID1, dwPID2);

	m_Marriages.erase(pMarriage);
	m_MarriageByPID.erase(dwPID1);
	m_MarriageByPID.erase(dwPID2);

	TPacketMarriageRemove p;
	p.dwPID1 = dwPID1;
	p.dwPID2 = dwPID2;
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_MARRIAGE_REMOVE, &p, sizeof(p));

	delete pMarriage;
}

void CMarriageManager::EngageToMarriage(uint32_t dwPID1, uint32_t dwPID2)
{
	TMarriage* pMarriage = Get(dwPID1);
	if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
	{
		WarnLog("not under marriage : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	if (pMarriage->is_married)
	{
		WarnLog("already married, cannot change engage to marry : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	Align(dwPID1, dwPID2);

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "UPDATE marriage SET is_married = 1 WHERE pid1 = %u AND pid2 = %u", 
			pMarriage->pid1, pMarriage->pid2);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	SQLResult* res = pmsg->Get();
	if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
	{
		WarnLog("cannot change engage to marriage : PID:{} {}", dwPID1, dwPID2);
		return;
	}

	TraceLog("MARRIAGE ENGAGE->MARRIAGE PID:{} {}", dwPID1, dwPID2);
	pMarriage->is_married = 1;

	TPacketMarriageUpdate p;
	p.dwPID1 = dwPID1;
	p.dwPID2 = dwPID2;
	p.iLovePoint = pMarriage->love_point;
	p.byMarried = pMarriage->is_married;
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_MARRIAGE_UPDATE, &p, sizeof(p));
}

void CMarriageManager::OnSetup(LPPEER pPeer)
{
	for (auto it = m_Marriages.begin(); it != m_Marriages.end(); ++it)
	{
		TMarriage* pMarriage = *it;

		{
			TPacketMarriageAdd p;
			p.dwPID1 = pMarriage->pid1;
			p.dwPID2 = pMarriage->pid2;
			p.tMarryTime = pMarriage->time;
			strlcpy(p.szName1, pMarriage->name1.c_str(), sizeof(p.szName1));
			strlcpy(p.szName2, pMarriage->name2.c_str(), sizeof(p.szName2));
			pPeer->EncodeHeader(HEADER_DG_MARRIAGE_ADD, 0, sizeof(p));
			pPeer->Encode(&p, sizeof(p));
		}

		{
			TPacketMarriageUpdate p;
			p.dwPID1 = pMarriage->pid1;
			p.dwPID2 = pMarriage->pid2;
			p.iLovePoint = pMarriage->love_point;
			p.byMarried	= pMarriage->is_married;
			pPeer->EncodeHeader(HEADER_DG_MARRIAGE_UPDATE, 0, sizeof(p));
			pPeer->Encode(&p, sizeof(p));
		}
	}

	for (auto it = m_mapRunningWedding.begin(); it != m_mapRunningWedding.end(); ++it)
	{
		const TWedding& t = it->second;

		TPacketWeddingReady p;
		p.dwPID1 = t.dwPID1;
		p.dwPID2 = t.dwPID2;
		p.dwMapIndex = t.dwMapIndex;

		pPeer->EncodeHeader(HEADER_DG_WEDDING_READY, 0, sizeof(p));
		pPeer->Encode(&p, sizeof(p));

		TPacketWeddingStart p2;
		p2.dwPID1 = t.dwPID1;
		p2.dwPID2 = t.dwPID2;

		pPeer->EncodeHeader(HEADER_DG_WEDDING_START, 0, sizeof(p2));
		pPeer->Encode(&p2, sizeof(p2));
	}
}

void CMarriageManager::ReadyWedding(uint32_t dwMapIndex, uint32_t dwPID1, uint32_t dwPID2)
{
	uint32_t dwStartTime = CClientManager::GetInstance()->GetCurrentTime();
	m_pqWeddingStart.push(TWedding(dwStartTime + 5, dwMapIndex, dwPID1, dwPID2));
}

void CMarriageManager::EndWedding(uint32_t dwPID1, uint32_t dwPID2)
{
	auto it = m_mapRunningWedding.find(std::make_pair(dwPID1, dwPID2));
	if (it == m_mapRunningWedding.end())
	{
		WarnLog("try to end wedding {} {}", dwPID1, dwPID2);
		return;
	}

	TWedding& w = it->second;

	TPacketWeddingEnd p;
	p.dwPID1 = w.dwPID1;
	p.dwPID2 = w.dwPID2;
	CClientManager::GetInstance()->ForwardPacket(HEADER_DG_WEDDING_END, &p, sizeof(p));
	m_mapRunningWedding.erase(it);
}

void CMarriageManager::Update()
{
	uint32_t now = CClientManager::GetInstance()->GetCurrentTime();

	if (!m_pqWeddingEnd.empty())
	{
		while (!m_pqWeddingEnd.empty() && m_pqWeddingEnd.top().dwTime <= now)
		{
			TWeddingInfo wi = m_pqWeddingEnd.top();
			m_pqWeddingEnd.pop();

			auto it = m_mapRunningWedding.find(std::make_pair(wi.dwPID1, wi.dwPID2));
			if (it == m_mapRunningWedding.end())
				continue;

			TWedding& w = it->second;

			TPacketWeddingEnd p;
			p.dwPID1 = w.dwPID1;
			p.dwPID2 = w.dwPID2;
			CClientManager::GetInstance()->ForwardPacket(HEADER_DG_WEDDING_END, &p, sizeof(p));
			m_mapRunningWedding.erase(it);

			auto it_marriage = m_MarriageByPID.find(w.dwPID1);

			if (it_marriage != m_MarriageByPID.end())
			{
				TMarriage* pMarriage = it_marriage->second;
				if (!pMarriage->is_married)
				{
					Remove(pMarriage->pid1, pMarriage->pid2);
				}
			}
		}
	}
	if (!m_pqWeddingStart.empty())
	{
		while (!m_pqWeddingStart.empty() && m_pqWeddingStart.top().dwTime <= now)
		{
			TWedding w = m_pqWeddingStart.top();
			m_pqWeddingStart.pop();

			TPacketWeddingStart p;
			p.dwPID1 = w.dwPID1;
			p.dwPID2 = w.dwPID2;
			CClientManager::GetInstance()->ForwardPacket(HEADER_DG_WEDDING_START, &p, sizeof(p));

			w.dwTime += WEDDING_LENGTH;
			m_pqWeddingEnd.push(TWeddingInfo(w.dwTime, w.dwPID1, w.dwPID2));
			m_mapRunningWedding.emplace(std::make_pair(w.dwPID1, w.dwPID2), w);
		}
	}
}
