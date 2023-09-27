#include "stdafx.h"
#include <Core/Grid.h>
#include "constants.h"
#include "safebox.h"
#include "packet.h"
#include "char.h"
#include "desc_client.h"
#include "item.h"
#include "item_manager.h"

CSafebox::CSafebox(LPCHARACTER pChrOwner, int32_t iSize, uint32_t dwGold) : m_pChrOwner(pChrOwner), m_iSize(iSize), m_lGold(dwGold)
{
	assert(m_pChrOwner != nullptr);
	memset(m_pItems, 0, sizeof(m_pItems));

	if (m_iSize)
		m_pGrid = M2_NEW CGrid(5, m_iSize);
	else
		m_pGrid = nullptr;

	m_bWindowMode = SAFEBOX;
}

CSafebox::~CSafebox()
{
	__Destroy();
}

void CSafebox::SetWindowMode(uint8_t bMode)
{
	m_bWindowMode = bMode;
}

void CSafebox::__Destroy()
{
	for (int32_t i = 0; i < SAFEBOX_MAX_NUM; ++i)
	{
		if (m_pItems[i])
		{
			m_pItems[i]->SetSkipSave(true);
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(m_pItems[i]);

			M2_DESTROY_ITEM(m_pItems[i]->RemoveFromCharacter());
			m_pItems[i] = nullptr;
		}
	}

	if (m_pGrid)
	{
		M2_DELETE(m_pGrid);
		m_pGrid = nullptr;
	}
}

bool CSafebox::Add(uint32_t dwPos, LPITEM pItem)
{
	if (!IsValidPosition(dwPos))
	{
		SysLog("SAFEBOX: item on wrong position at {} (size of grid = {})", dwPos, m_pGrid->GetSize());
		return false;
	}

	pItem->SetWindow(m_bWindowMode);
	pItem->SetCell(m_pChrOwner, dwPos);
	pItem->Save();
	ITEM_MANAGER::GetInstance()->FlushDelayedSave(pItem);

	m_pGrid->Put(dwPos, 1, pItem->GetSize());
	m_pItems[dwPos] = pItem;

	TPacketGCItemSet pack;

	pack.header	= HEADER_GC_SAFEBOX_SET;
	pack.Cell	= TItemPos(m_bWindowMode, dwPos);
	pack.vnum	= pItem->GetVnum();
	pack.count	= pItem->GetCount();
	pack.flags	= pItem->GetFlag();
	pack.anti_flags	= pItem->GetAntiFlag();
	memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
	memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

	m_pChrOwner->GetDesc()->Packet(&pack, sizeof(pack));
	TraceLog("SAFEBOX: ADD {} {} count {}", m_pChrOwner->GetName(), pItem->GetName(), pItem->GetCount());
	return true;
}

LPITEM CSafebox::Get(uint32_t dwPos)
{
	if (dwPos >= m_pGrid->GetSize())
		return NULL;

	return m_pItems[dwPos];
}

LPITEM CSafebox::Remove(uint32_t dwPos)
{
	LPITEM pItem = Get(dwPos);

	if (!pItem)
		return NULL;

	if (!m_pGrid)
	{
		SysLog("Safebox::Remove : nil grid");
	}
	else
		m_pGrid->Get(dwPos, 1, pItem->GetSize());

	pItem->RemoveFromCharacter();

	m_pItems[dwPos] = nullptr;

	TPacketGCSafeboxItemDel pack;

	pack.header	= HEADER_GC_SAFEBOX_DEL;
	pack.pos	= dwPos;

	m_pChrOwner->GetDesc()->Packet(&pack, sizeof(pack));
	TraceLog("SAFEBOX: REMOVE {} {} count {}", m_pChrOwner->GetName(), pItem->GetName(), pItem->GetCount());
	return pItem;
}

void CSafebox::Save()
{
	TSafeboxTable t;

	memset(&t, 0, sizeof(TSafeboxTable));

	t.dwID = m_pChrOwner->GetDesc()->GetAccountTable().id;
	t.dwGold = m_lGold;

	db_clientdesc->DBPacket(HEADER_GD_SAFEBOX_SAVE, 0, &t, sizeof(TSafeboxTable));
	TraceLog("SAFEBOX: SAVE {}", m_pChrOwner->GetName());
}

bool CSafebox::IsEmpty(uint32_t dwPos, uint8_t bSize)
{
	if (!m_pGrid)
		return false;

	return m_pGrid->IsEmpty(dwPos, 1, bSize);
}

void CSafebox::ChangeSize(int32_t iSize)
{
	if (m_iSize >= iSize)
		return;

	m_iSize = iSize;

	CGrid* pOldGrid = m_pGrid;

	if (pOldGrid)
		m_pGrid = M2_NEW CGrid(pOldGrid, 5, m_iSize);
	else
		m_pGrid = M2_NEW CGrid(5, m_iSize);
}

LPITEM CSafebox::GetItem(uint8_t bCell)
{
	if (bCell >= 5 * m_iSize)
	{
		SysLog("CHARACTER::GetItem: invalid item cell {}", bCell);
		return NULL;
	}

	return m_pItems[bCell];
}

bool CSafebox::MoveItem(uint8_t bCell, uint8_t bDestCell, uint8_t count)
{
	LPITEM item;

	int32_t max_position = 5 * m_iSize;

	if (bCell >= max_position || bDestCell >= max_position)
		return false;

	if (!(item = GetItem(bCell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	{
		LPITEM item2;

		if ((item2 = GetItem(bDestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM::ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum())
		{
			for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			count = MIN(200 - item2->GetCount(), count);

			if (item->GetCount() >= count)
				Remove(bCell);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);

			TraceLog("SAFEBOX: STACK {} {} -> {} {} count {}", m_pChrOwner->GetName(), bCell, bDestCell, item2->GetName(), item2->GetCount());
			return true;
		}

		if (!IsEmpty(bDestCell, item->GetSize()))
			return false;

		m_pGrid->Get(bCell, 1, item->GetSize());

		if (!m_pGrid->Put(bDestCell, 1, item->GetSize()))
		{
			m_pGrid->Put(bCell, 1, item->GetSize());
			return false;
		}
		else
		{
			m_pGrid->Get(bDestCell, 1, item->GetSize());
			m_pGrid->Put(bCell, 1, item->GetSize());
		}

		TraceLog("SAFEBOX: MOVE {} {} -> {} {} count {}", m_pChrOwner->GetName(), bCell, bDestCell, item->GetName(), item->GetCount());

		Remove(bCell);
		Add(bDestCell, item);
	}

	return true;
}

bool CSafebox::IsValidPosition(uint32_t dwPos)
{
	if (!m_pGrid)
		return false;

	if (dwPos >= m_pGrid->GetSize())
		return false;

	return true;
}

