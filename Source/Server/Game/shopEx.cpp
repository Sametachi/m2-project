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
#include "desc_client.h"
#include "shopEx.h"
#include "group_text_parse_tree.h"

bool CShopEx::Create(uint32_t dwVnum, uint32_t dwNPCVnum)
{
	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;
	return true;
}

bool CShopEx::AddShopTable(TShopTableEx& shopTable)
{
	for (auto it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& _shopTable = *it;
		if (0 != _shopTable.dwVnum && _shopTable.dwVnum == shopTable.dwVnum)
			return false;
		if (0 != _shopTable.dwNPCVnum && _shopTable.dwNPCVnum == shopTable.dwVnum)
			return false;
	}
	m_vec_shopTabs.push_back(shopTable);
	return true;
}

bool CShopEx::AddGuest(LPCHARACTER ch,uint32_t owner_vid, bool bOtherEmpire)
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
	pack.subheader	= SHOP_SUBHEADER_GC_START_EX;

	TPacketGCShopStartEx pack2;

	memset(&pack2, 0, sizeof(pack2));
	
	pack2.owner_vid = owner_vid;
	pack2.shop_tab_count = m_vec_shopTabs.size();
	char temp[8096];
	char* buf = &temp[0];
	size_t size = 0;
	for (auto it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& shop_tab = *it;
		TPacketGCShopStartEx::TSubPacketShopTab pack_tab;
		pack_tab.coin_type = shop_tab.coinType;
		memcpy(pack_tab.name, shop_tab.name.c_str(), SHOP_TAB_NAME_MAX);
		
		for (uint8_t i = 0; i < SHOP_HOST_ITEM_MAX_NUM; i++)
		{
			pack_tab.items[i].vnum = shop_tab.items[i].vnum;
			pack_tab.items[i].count = shop_tab.items[i].count;
			switch(shop_tab.coinType)
			{
			case SHOP_COIN_TYPE_GOLD:
				if (bOtherEmpire) // no empire price penalty for pc shop
					pack_tab.items[i].price = shop_tab.items[i].price * 3;
				else
					pack_tab.items[i].price = shop_tab.items[i].price;
				break;
			case SHOP_COIN_TYPE_SECONDARY_COIN:
				pack_tab.items[i].price = shop_tab.items[i].price;
				break;
			}
			memset(pack_tab.items[i].aAttr, 0, sizeof(pack_tab.items[i].aAttr));
			memset(pack_tab.items[i].alSockets, 0, sizeof(pack_tab.items[i].alSockets));
		}

		memcpy(buf, &pack_tab, sizeof(pack_tab));
		buf += sizeof(pack_tab);
		size += sizeof(pack_tab);
	}

	pack.size = sizeof(pack) + sizeof(pack2) + size;

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->BufferedPacket(&pack2, sizeof(TPacketGCShopStartEx));
	ch->GetDesc()->Packet(temp, size);

	return true;
}

int32_t CShopEx::Buy(LPCHARACTER ch, uint8_t pos)
{
	uint8_t tabIdx = pos / SHOP_HOST_ITEM_MAX_NUM;
	uint8_t slotPos = pos % SHOP_HOST_ITEM_MAX_NUM;
	if (tabIdx >= GetTabCount())
	{
		PyLog("ShopEx::Buy : invalid position {} : {}", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	PyLog("ShopEx::Buy : name {} pos {}", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	TShopTableEx& shopTab = m_vec_shopTabs[tabIdx];
	TShopItemTable& r_item = shopTab.items[slotPos];

	if (r_item.price <= 0)
	{
		LogManager::GetInstance()->HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	uint32_t dwPrice = r_item.price;

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		if (it->second)	// if other empire, price is triple
			dwPrice *= 3;

		if (ch->GetGold() < (int32_t) dwPrice)
		{
			TraceLog("ShopEx::Buy : Not enough money : {} has {}, price {}", ch->GetName(), ch->GetGold(), dwPrice);
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
		}
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		{
			int32_t count = ch->CountSpecifyTypeItem(ITEM::TYPE_SECONDARY_COIN);
			if (count < dwPrice)
			{
				TraceLog("ShopEx::Buy : Not enough myeongdojun : {} has {}, price {}", ch->GetName(), count, dwPrice);
				return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX;
			}
		}
		break;
	}
	
	LPITEM item;

	item = ITEM_MANAGER::GetInstance()->CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return SHOP_SUBHEADER_GC_SOLD_OUT;

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
		TraceLog("ShopEx::Buy : Inventory full : {} size {}", ch->GetName(), item->GetSize());
		M2_DESTROY_ITEM(item);
		return SHOP_SUBHEADER_GC_INVENTORY_FULL;
	}

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		ch->PointChange(POINT_GOLD, -(int32_t)dwPrice, false);
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		ch->RemoveSpecifyTypeItem(ITEM::TYPE_SECONDARY_COIN, dwPrice);
		break;
	}


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

	if (item)
		PyLog("ShopEx: BUY: name {} {}(x {}):{} price {}", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice);

	ch->Save();

    return (SHOP_SUBHEADER_GC_OK);
}

