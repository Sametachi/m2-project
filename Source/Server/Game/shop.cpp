#include "stdafx.h"
#include <Core/Grid.h>
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "locale_service.h"

CShop::CShop()
	: m_dwVnum(0), m_dwNPCVnum(0), m_pPC(nullptr)
{
	m_pGrid = M2_NEW CGrid(5, 9);
}

CShop::~CShop()
{
	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	Broadcast(&pack, sizeof(pack));

	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		ch->SetShop(nullptr);
		++it;
	}

	M2_DELETE(m_pGrid);
}

void CShop::SetPCShop(LPCHARACTER ch)
{
	m_pPC = ch;
}

bool CShop::Create(uint32_t dwVnum, uint32_t dwNPCVnum, TShopItemTable* pTable)
{
	/*
	   if (!CMobManager::GetInstance()->Get(dwNPCVnum))
	   {
	   SysLog("No such a npc by vnum {}", dwNPCVnum);
	   return false;
	   }
	 */
	PyLog("SHOP #{} (Shopkeeper {})", dwVnum, dwNPCVnum);

	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;

	uint8_t bItemCount;

	for (bItemCount = 0; bItemCount < SHOP_HOST_ITEM_MAX_NUM; ++bItemCount)
		if (0 == (pTable + bItemCount)->vnum)
			break;

	SetShopItems(pTable, bItemCount);
	return true;
}

void CShop::SetShopItems(TShopItemTable* pTable, uint8_t bItemCount)
{
	if (bItemCount > SHOP_HOST_ITEM_MAX_NUM)
		return;

	m_pGrid->Clear();

	m_itemVector.resize(SHOP_HOST_ITEM_MAX_NUM);
	memset(&m_itemVector[0], 0, sizeof(SHOP_ITEM) * m_itemVector.size());

	for (int32_t i = 0; i < bItemCount; ++i)
	{
		LPITEM pItem = nullptr;
		const TItemTable * item_table;

		if (m_pPC)
		{
			pItem = m_pPC->GetItem(pTable->pos);

			if (!pItem)
			{
				SysLog("cannot find item on pos ({}, {}) (name: {})", pTable->pos.window_type, pTable->pos.cell, m_pPC->GetName());
				continue;
			}

			item_table = pItem->GetProto();
		}
		else
		{
			if (!pTable->vnum)
				continue;

			item_table = ITEM_MANAGER::GetInstance()->GetTable(pTable->vnum);
		}

		if (!item_table)
		{
			SysLog("Shop: no item table by item vnum #{}", pTable->vnum);
			continue;
		}

		int32_t iPos;

		if (IsPCShop())
		{
			PyLog("MyShop: use position {}", pTable->display_pos);
			iPos = pTable->display_pos;
		}
		else
			iPos = m_pGrid->FindBlank(1, item_table->bSize);

		if (iPos < 0)
		{
			SysLog("not enough shop window");
			continue;
		}

		if (!m_pGrid->IsEmpty(iPos, 1, item_table->bSize))
		{
			if (IsPCShop())
			{
				SysLog("not empty position for pc shop {}[{}]", m_pPC->GetName(), m_pPC->GetPlayerID());
			}
			else
			{
				SysLog("not empty position for npc shop");
			}
			continue;
		}

		m_pGrid->Put(iPos, 1, item_table->bSize);

		SHOP_ITEM & item = m_itemVector[iPos];

		item.pItem = pItem;
		item.itemid = 0;

		if (item.pItem)
		{
			item.vnum = pItem->GetVnum();
			item.count = pItem->GetCount();
			item.price = pTable->price;
			item.itemid	= pItem->GetID();
		}
		else
		{
			item.vnum = pTable->vnum;
			item.count = pTable->count;

			if (IS_SET(item_table->dwFlags, ITEM::FLAG_COUNT_PER_1GOLD))
			{
				if (item_table->dwIBuyItemPrice == 0)
					item.price = item.count;
				else
					item.price = item.count / item_table->dwIBuyItemPrice;
			}
			else
				item.price = item_table->dwIBuyItemPrice * item.count;
		}

		PyLog("SHOP_ITEM: ITEM_NAME {} ITEM_VNUM {} ITEM_COUNT {} PRICE {}", item_table->szName, (int32_t)item.vnum, item.count, item.price);
		++pTable;
	}
}

int32_t CShop::Buy(LPCHARACTER ch, uint8_t pos)
{
	if (pos >= m_itemVector.size())
	{
		PyLog("Shop::Buy : invalid position {} : {}", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	PyLog("Shop::Buy : name {} pos {}", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	SHOP_ITEM& r_item = m_itemVector[pos];

	if (r_item.price <= 0)
	{
		LogManager::GetInstance()->HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	LPITEM pSelectedItem = ITEM_MANAGER::GetInstance()->Find(r_item.itemid);

	if (IsPCShop())
	{
		if (!pSelectedItem)
		{
			PyLog("Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:{} SellerPID:{}",
					ch->GetPlayerID(),
					m_pPC->GetPlayerID());

			return false;
		}

		if ((pSelectedItem->GetOwner() != m_pPC))
		{
			PyLog("Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:{} SellerPID:{}",
					ch->GetPlayerID(),
					m_pPC->GetPlayerID());

			return false;
		}
	}

	uint32_t dwPrice = r_item.price;

	if (it->second)	// if other empire, price is triple
		dwPrice *= 3;

	if (ch->GetGold() < (int32_t) dwPrice)
	{
		TraceLog("Shop::Buy : Not enough money : {} has {}, price {}", ch->GetName(), ch->GetGold(), dwPrice);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	LPITEM item;

	if (m_pPC)
		item = r_item.pItem;
	else
		item = ITEM_MANAGER::GetInstance()->CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return SHOP_SUBHEADER_GC_SOLD_OUT;

	if (!m_pPC)
	{
		if (quest::CQuestManager::GetInstance()->GetEventFlag("hivalue_item_sell") == 0)
		{ 
			if (item->GetVnum() == 70024 || item->GetVnum() == 70035)
			{
				return SHOP_SUBHEADER_GC_END;
			}
		}
	}

	int32_t iEmptyPos;
	if (item->IsDragonSoul())
	{
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
	}
	else
	{
		iEmptyPos = ch->GetEmptyInventory(item->GetSize());
	}

	if (iEmptyPos < 0)
	{
		if (m_pPC)
		{
			TraceLog("Shop::Buy at PC Shop : Inventory full : {} size {}", ch->GetName(), item->GetSize());
			return SHOP_SUBHEADER_GC_INVENTORY_FULL;
		}
		else
		{
			TraceLog("Shop::Buy : Inventory full : {} size {}", ch->GetName(), item->GetSize());
			M2_DESTROY_ITEM(item);
			return SHOP_SUBHEADER_GC_INVENTORY_FULL;
		}
	}

	ch->PointChange(POINT_GOLD, -(int32_t)dwPrice, false);

	uint32_t dwTax = 0;
	int32_t iVal = quest::CQuestManager::GetInstance()->GetEventFlag("personal_shop");

	if (0 < iVal)
	{
		if (iVal > 100)
			iVal = 100;

		dwTax = dwPrice * iVal / 100;
		dwPrice = dwPrice - dwTax;
	}
	else
	{
		iVal = 0;
		dwTax = 0;
	}

	if (!m_pPC) 
	{
		CMonarch::GetInstance()->SendtoDBAddMoney(dwTax, ch->GetEmpire(), ch);
	}

	if (m_pPC)
	{
		m_pPC->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);

		char buf[512];

		if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
		{
			snprintf(buf, sizeof(buf), "%s FROM: %u TO: %u PRICE: %u", item->GetName(), ch->GetPlayerID(), m_pPC->GetPlayerID(), dwPrice);
			LogManager::GetInstance()->GoldBarLog(ch->GetPlayerID(), item->GetID(), SHOP_BUY, buf);
			LogManager::GetInstance()->GoldBarLog(m_pPC->GetPlayerID(), item->GetID(), SHOP_SELL, buf);
		}
			
		item->RemoveFromCharacter();
		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ITEM_MANAGER::GetInstance()->FlushDelayedSave(item);
			

		snprintf(buf, sizeof(buf), "%s %u(%s) %u %u", item->GetName(), m_pPC->GetPlayerID(), m_pPC->GetName(), dwPrice, item->GetCount());
		LogManager::GetInstance()->ItemLog(ch, item, "SHOP_BUY", buf);

		snprintf(buf, sizeof(buf), "%s %u(%s) %u %u", item->GetName(), ch->GetPlayerID(), ch->GetName(), dwPrice, item->GetCount());
		LogManager::GetInstance()->ItemLog(m_pPC, item, "SHOP_SELL", buf);

		r_item.pItem = nullptr;
		BroadcastUpdateItem(pos);

		m_pPC->PointChange(POINT_GOLD, dwPrice, false);

		if (iVal > 0)
			m_pPC->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This sale will be taxed %d%%."), iVal);

		CMonarch::GetInstance()->SendtoDBAddMoney(dwTax, m_pPC->GetEmpire(), m_pPC);
	}
	else
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
		else
			item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
		ITEM_MANAGER::GetInstance()->FlushDelayedSave(item);
		LogManager::GetInstance()->ItemLog(ch, item, "BUY", item->GetName());

		if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
		{
			LogManager::GetInstance()->GoldBarLog(ch->GetPlayerID(), item->GetID(), PERSONAL_SHOP_BUY, "");
		}

		DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -(int32_t)dwPrice);
	}

	if (item)
		PyLog("SHOP: BUY: name {} {}(x {}):{} price {}", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice);

    ch->Save();

    return (SHOP_SUBHEADER_GC_OK);
}

bool CShop::AddGuest(LPCHARACTER ch, uint32_t owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_START;

	TPacketGCShopStart pack2;

	memset(&pack2, 0, sizeof(pack2));
	pack2.owner_vid = owner_vid;

	for (uint32_t i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

		//HIVALUE_ITEM_EVENT
		if (quest::CQuestManager::GetInstance()->GetEventFlag("hivalue_item_sell") == 0)
		{ 
			if (item.vnum == 70024 || item.vnum == 70035)
			{				
				continue;
			}
		}
		//END_HIVALUE_ITEM_EVENT
		if (m_pPC && !item.pItem)
			continue;

		pack2.items[i].vnum = item.vnum;

		if (bOtherEmpire) // no empire price penalty for pc shop
			pack2.items[i].price = item.price * 3;
		else
			pack2.items[i].price = item.price;

		pack2.items[i].count = item.count;

		if (item.pItem)
		{
			memcpy(pack2.items[i].alSockets, item.pItem->GetSockets(), sizeof(pack2.items[i].alSockets));
			memcpy(pack2.items[i].aAttr, item.pItem->GetAttributes(), sizeof(pack2.items[i].aAttr));
		}
	}

	pack.size = sizeof(pack) + sizeof(pack2);

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->Packet(&pack2, sizeof(TPacketGCShopStart));
	return true;
}

void CShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetShop() != this)
		return;

	m_map_guest.erase(ch);
	ch->SetShop(nullptr);

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_END;
	pack.size		= sizeof(TPacketGCShop);

	ch->GetDesc()->Packet(&pack, sizeof(pack));
}

void CShop::Broadcast(const void* data, int32_t bytes)
{
	GuestMapType::iterator it;

	it = m_map_guest.begin();

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;

		if (ch->GetDesc())
			ch->GetDesc()->Packet(data, bytes);

		++it;
	}
}

void CShop::BroadcastUpdateItem(uint8_t pos)
{
	TPacketGCShop pack;
	TPacketGCShopUpdateItem pack2;

	TEMP_BUFFER	buf;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_UPDATE_ITEM;
	pack.size		= sizeof(pack) + sizeof(pack2);

	pack2.pos		= pos;

	if (m_pPC && !m_itemVector[pos].pItem)
		pack2.item.vnum = 0;
	else
	{
		pack2.item.vnum	= m_itemVector[pos].vnum;
		if (m_itemVector[pos].pItem)
		{
			memcpy(pack2.item.alSockets, m_itemVector[pos].pItem->GetSockets(), sizeof(pack2.item.alSockets));
			memcpy(pack2.item.aAttr, m_itemVector[pos].pItem->GetAttributes(), sizeof(pack2.item.aAttr));
		}
		else
		{
			memset(pack2.item.alSockets, 0, sizeof(pack2.item.alSockets));
			memset(pack2.item.aAttr, 0, sizeof(pack2.item.aAttr));
		}
	}

	pack2.item.price	= m_itemVector[pos].price;
	pack2.item.count	= m_itemVector[pos].count;

	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));

	Broadcast(buf.read_peek(), buf.size());
}

int32_t CShop::GetNumberByVnum(uint32_t dwVnum)
{
	int32_t itemNumber = 0;

	for (uint32_t i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

		if (item.vnum == dwVnum)
		{
			itemNumber += item.count;
		}
	}

	return itemNumber;
}

bool CShop::IsSellingItem(uint32_t itemID)
{
	bool isSelling = false;

	for (uint32_t i = 0; i < m_itemVector.size() && i < SHOP_HOST_ITEM_MAX_NUM; ++i)
	{
		if (m_itemVector[i].itemid == itemID)
		{
			isSelling = true;
			break;
		}
	}

	return isSelling;

}
