#pragma once

#include <Common/cache.h>
static uint32_t g_item_count;

class CItemCache : public cache<TPlayerItem>
{
public:
	CItemCache();
	virtual ~CItemCache();

	void Delete();
	virtual void OnFlush();
};

class CPlayerTableCache : public cache<TPlayerTable>
{
public:
	CPlayerTableCache();
	virtual ~CPlayerTableCache();

	virtual void OnFlush();

	uint32_t GetLastUpdateTime() { return m_lastUpdateTime; }
};

class CItemPriceListTableCache : public cache< TItemPriceListTable >
{
public:
	CItemPriceListTableCache(void);

	void	UpdateList(const TItemPriceListTable* pUpdateList);

	virtual void	OnFlush(void);

 private:

	static const int32_t	s_nMinFlushSec;
};