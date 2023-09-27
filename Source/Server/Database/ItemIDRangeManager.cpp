#include "stdafx.h"
#include "ItemIDRangeManager.h"
#include "Main.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "Peer.h"

CItemIDRangeManager::CItemIDRangeManager()
{
	m_listData.clear();
}

void CItemIDRangeManager::Build()
{
	uint32_t dwMin = 0;
	uint32_t dwMax = 0;
	TItemIDRangeTable range;

	for (int32_t i = 0; ; ++i)
	{
		dwMin = cs_dwMinimumRange * (i + 1) + 1;
		dwMax = cs_dwMinimumRange * (i + 2);

		if (dwMax == cs_dwMaxItemID)
			break;

		if (CClientManager::GetInstance()->GetItemRange().dwMin <= dwMin &&
			CClientManager::GetInstance()->GetItemRange().dwMax >= dwMax)
		{
			continue;
		}

		if (BuildRange(dwMin, dwMax, range))
		{
			m_listData.push_back(range);
		}
	}
}

struct FCheckCollision
{
	bool hasCollision;
	TItemIDRangeTable range;

	FCheckCollision(TItemIDRangeTable data)
	{
		hasCollision = false;
		range = data;
	}

	void operator() (LPPEER pPeer)
	{
		if (hasCollision == false)
		{
			hasCollision = pPeer->CheckItemIDRangeCollision(range);
		}
	}
};

TItemIDRangeTable CItemIDRangeManager::GetRange()
{
	TItemIDRangeTable ret;
	ret.dwMin = 0;
	ret.dwMax = 0;
	ret.dwUsableItemIDMin = 0;

	if (m_listData.size() > 0)
	{
		while (m_listData.size() > 0)
		{
			ret = m_listData.front();
			m_listData.pop_front();

			FCheckCollision f(ret);
			CClientManager::GetInstance()->for_each_peer(f);

			if (f.hasCollision == false) return ret;
		}
	}

	for (int32_t i = 0; i < 10; ++i)
	{
		SysLog("ItemIDRange: NO MORE ITEM ID RANGE");
	}

	return ret;
}

bool CItemIDRangeManager::BuildRange(uint32_t dwMin, uint32_t dwMax, TItemIDRangeTable& range)
{
	char szQuery[256];
	uint32_t dwItemMaxID = 0;
	MYSQL_ROW row;

	snprintf(szQuery, sizeof(szQuery), "SELECT MAX(id) FROM item%s WHERE id >= %u and id <= %u", GetTablePostfix(), dwMin, dwMax);

	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery));
	if (pSQLMsg)
	{
		if (pSQLMsg->Get()->uiNumRows > 0)
		{
			row = mysql_fetch_row(pSQLMsg->Get()->pSQLResult);
			str_to_number(dwItemMaxID, row[0]);
		}
	}

	if (dwItemMaxID == 0)
		dwItemMaxID = dwMin;
	else
		dwItemMaxID++;

	
	if ((dwMax < dwItemMaxID) || (dwMax - dwItemMaxID < cs_dwMinimumRemainCount))
	{
		TraceLog("ItemIDRange: Build {} ~ {} start: {}\tNOT USE remain count is below %u",
			   	dwMin, dwMax, dwItemMaxID, cs_dwMinimumRemainCount);
	}
	else
	{
		range.dwMin = dwMin;
		range.dwMax = dwMax;
		range.dwUsableItemIDMin = dwItemMaxID;

		snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM item%s WHERE id >= %u AND id <= %u", 
				GetTablePostfix(), range.dwUsableItemIDMin, range.dwMax);

		std::unique_ptr<SQLMsg> pSQLMsg2(CDBManager::GetInstance()->DirectQuery(szQuery));
		if (pSQLMsg2)
		{
			if (pSQLMsg2->Get()->uiNumRows > 0)
			{
				uint32_t dwcCount = 0;
				row = mysql_fetch_row(pSQLMsg2->Get()->pSQLResult);
				str_to_number(dwcCount, row[0]);

				if (dwcCount > 0)
				{
					TraceLog("ItemIDRange: Build: {} ~ {}\thave a item", range.dwUsableItemIDMin, range.dwMax);
					return false;
				}
				else
				{
					TraceLog("ItemIDRange: Build: {} ~ {} start:{}", range.dwMin, range.dwMax, range.dwUsableItemIDMin);
					return true;
				}
			}
		}
	}

	return false;
}

void CItemIDRangeManager::UpdateRange(uint32_t dwMin, uint32_t dwMax)
{
	TItemIDRangeTable range;

	if (BuildRange(dwMin, dwMax, range))
	{
		m_listData.push_back(range);
	}
}

