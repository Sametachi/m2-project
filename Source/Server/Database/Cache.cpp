#include "stdafx.h"
#include <Core/Constants/Item.hpp>

#include "Cache.h"
#include "QID.h"
#include "ClientManager.h"
#include "Main.h"

extern CPacketInfo g_item_info;
extern int32_t g_iPlayerCacheFlushSeconds;
extern int32_t g_iItemCacheFlushSeconds;

extern int32_t g_iItemPriceListTableCacheFlushSeconds;
extern uint32_t g_item_count;

CItemCache::CItemCache()
{
	m_expireTime = MIN(1800, g_iItemCacheFlushSeconds);
}

CItemCache::~CItemCache()
{
}

// This is weird...
// If Delete is done, shouldn't the cache be released as well?
// But there is no part to release the cache.
// Couldn't find it?
// If you do this, keep deleting items every time the time runs out...
// This item has already disappeared... Confirmed kill?
// fixme
// by rtsummit
void CItemCache::Delete()
{
	if (m_data.vnum == 0)
		return;

	TraceLog("ItemCache::Delete : DELETE {}", m_data.id);

	m_data.vnum = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(nullptr);
	OnFlush();
}

void CItemCache::OnFlush()
{
	if (m_data.vnum == 0)
	{
		char szQuery[128];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), m_data.id);
		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEM_DESTROY, 0, nullptr);
		TraceLog("ItemCache::Flush : DELETE {} {}", m_data.id, szQuery);
	}
	else
	{
		int32_t alSockets[ITEM::SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM::ATTRIBUTE_MAX_NUM];
		bool isSocket = false, isAttr = false;

		memset(&alSockets, 0, sizeof(int32_t) * ITEM::SOCKET_MAX_NUM);
		memset(&aAttr, 0, sizeof(TPlayerItemAttribute) * ITEM::ATTRIBUTE_MAX_NUM);

		TPlayerItem* p = &m_data;

		if (memcmp(alSockets, p->alSockets, sizeof(int32_t) * ITEM::SOCKET_MAX_NUM))
			isSocket = true;

		if (memcmp(aAttr, p->aAttr, sizeof(TPlayerItemAttribute) * ITEM::ATTRIBUTE_MAX_NUM))
			isAttr = true;

		char szColumns[512];
		char szValues[1024];

		int32_t iLen = snprintf(szColumns, sizeof(szColumns), "id, owner_id, window, pos, count, vnum");

		int32_t iValueLen = snprintf(szValues, sizeof(szValues), "%u, %u, %d, %d, %u, %u",
				p->id, p->owner, p->window, p->pos, p->count, p->vnum);

		if (isSocket)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ", socket0, socket1, socket2");
			iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %lu, %lu, %lu", p->alSockets[0], p->alSockets[1], p->alSockets[2]);
		}

		if (isAttr)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, 
					", attrtype0, attrvalue0, attrtype1, attrvalue1, attrtype2, attrvalue2, attrtype3, attrvalue3"
					", attrtype4, attrvalue4, attrtype5, attrvalue5, attrtype6, attrvalue6");

			iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
					p->aAttr[0].bType, p->aAttr[0].sValue,
					p->aAttr[1].bType, p->aAttr[1].sValue,
					p->aAttr[2].bType, p->aAttr[2].sValue,
					p->aAttr[3].bType, p->aAttr[3].sValue,
					p->aAttr[4].bType, p->aAttr[4].sValue,
					p->aAttr[5].bType, p->aAttr[5].sValue,
					p->aAttr[6].bType, p->aAttr[6].sValue);
		}

		char szItemQuery[2048];
		snprintf(szItemQuery, sizeof(szItemQuery), "REPLACE INTO item%s (%s) VALUES(%s)", GetTablePostfix(), szColumns, szValues);

		TraceLog("ItemCache::Flush :REPLACE  ({})", szItemQuery);

		CDBManager::GetInstance()->ReturnQuery(szItemQuery, QID_ITEM_SAVE, 0, nullptr);

		++g_item_count;
	}

	m_bNeedQuery = false;
}

CPlayerTableCache::CPlayerTableCache()
{
	m_expireTime = MIN(1800, g_iPlayerCacheFlushSeconds);
}

CPlayerTableCache::~CPlayerTableCache()
{
}

void CPlayerTableCache::OnFlush()
{
	TraceLog("PlayerTableCache::Flush : {}", m_data.name);

	char szQuery[QUERY_MAX_LEN];
	CreatePlayerSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, nullptr);
}

const int32_t CItemPriceListTableCache::s_nMinFlushSec = 1800;

CItemPriceListTableCache::CItemPriceListTableCache()
{
	m_expireTime = MIN(s_nMinFlushSec, g_iItemPriceListTableCacheFlushSeconds);
}

void CItemPriceListTableCache::UpdateList(const TItemPriceListTable* pUpdateList)
{
	// Finds items that are already cached and duplicates, and puts non-duplicated previous information into tmpvec.
	std::vector<TItemPriceInfo> tmpvec;

	for (uint idx = 0; idx < m_data.byCount; ++idx)
	{
		const TItemPriceInfo* pos = pUpdateList->aPriceInfo;
		for (; pos != pUpdateList->aPriceInfo + pUpdateList->byCount && m_data.aPriceInfo[idx].dwVnum != pos->dwVnum; ++pos)
			;

		if (pos == pUpdateList->aPriceInfo + pUpdateList->byCount)
			tmpvec.push_back(m_data.aPriceInfo[idx]);
	}

	// Copy the remaining space to tmpvec and copy the remaining space from the front of tmpvec.
	if (pUpdateList->byCount > SHOP_PRICELIST_MAX_NUM)
	{
		SysLog("Count overflow!");
		return;
	}

	m_data.byCount = pUpdateList->byCount;

	memcpy(m_data.aPriceInfo, pUpdateList->aPriceInfo, sizeof(TItemPriceInfo)* pUpdateList->byCount);

	int32_t nDeletedNum;

	if (pUpdateList->byCount < SHOP_PRICELIST_MAX_NUM)
	{
		size_t sizeAddOldDataSize = SHOP_PRICELIST_MAX_NUM - pUpdateList->byCount;

		if (tmpvec.size() < sizeAddOldDataSize)
			sizeAddOldDataSize = tmpvec.size();

		memcpy(m_data.aPriceInfo + pUpdateList->byCount, &tmpvec[0], sizeof(TItemPriceInfo) * sizeAddOldDataSize);
		m_data.byCount += sizeAddOldDataSize;

		nDeletedNum = tmpvec.size() - sizeAddOldDataSize;
	}
	else
		nDeletedNum = tmpvec.size();

	m_bNeedQuery = true;

	TraceLog(
			"ItemPriceListTableCache::UpdateList : OwnerID[{}] Update [{}] Items, Delete [{}] Items, Total [{}] Items", 
			m_data.dwOwnerID, pUpdateList->byCount, nDeletedNum, m_data.byCount);
}

void CItemPriceListTableCache::OnFlush()
{
	char szQuery[256];

	// Delete all item price information stored in the DB for the owner of this cache.
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM myshop_pricelist%s WHERE owner_id = %u", GetTablePostfix(), m_data.dwOwnerID);
	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEMPRICE_DESTROY, 0, nullptr);

	// Write all cache contents to DB.
	for (int32_t idx = 0; idx < m_data.byCount; ++idx)
	{
		snprintf(szQuery, sizeof(szQuery),
				"INSERT INTO myshop_pricelist%s(owner_id, item_vnum, price) VALUES(%u, %u, %u)", 
				GetTablePostfix(), m_data.dwOwnerID, m_data.aPriceInfo[idx].dwVnum, m_data.aPriceInfo[idx].dwPrice);

		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEMPRICE_SAVE, 0, nullptr);
	}

	TraceLog("ItemPriceListTableCache::Flush : OwnerID[{}] Update [{}]Items", m_data.dwOwnerID, m_data.byCount);
	
	m_bNeedQuery = false;
}