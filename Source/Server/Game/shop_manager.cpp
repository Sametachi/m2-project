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
#include "shop_manager.h"
#include "group_text_parse_tree.h"
#include "shopEx.h"
#include <boost/algorithm/string/predicate.hpp>
#include "shop_manager.h"
#include <cctype>

CShopManager::CShopManager()
{
}

CShopManager::~CShopManager()
{
	Destroy();
}

bool CShopManager::Initialize(TShopTable * table, int32_t size)
{
	if (!m_map_pShop.empty())
		return false;

	int32_t i; 

	for (i = 0; i < size; ++i, ++table)
	{
		LPSHOP shop = M2_NEW CShop;

		if (!shop->Create(table->dwVnum, table->dwNPCVnum, table->items))
		{
			M2_DELETE(shop);
			continue;
		}

		m_map_pShop.insert(TShopMap::value_type(table->dwVnum, shop));
		m_map_pShopByNPCVnum.insert(TShopMap::value_type(table->dwNPCVnum, shop));
	}
	char szShopTableExFileName[256];

	snprintf(szShopTableExFileName, sizeof(szShopTableExFileName),
		"%s/shop_table_ex.txt", LocaleService_GetBasePath().c_str());

	return ReadShopTableEx(szShopTableExFileName);
}

void CShopManager::Destroy()
{
	TShopMap::iterator it = m_map_pShop.begin();

	while (it != m_map_pShop.end())
	{
		M2_DELETE(it->second);
		++it;
	}

	m_map_pShop.clear();
}

LPSHOP CShopManager::Get(uint32_t dwVnum)
{
	TShopMap::const_iterator it = m_map_pShop.find(dwVnum);

	if (it == m_map_pShop.end())
		return NULL;

	return (it->second);
}

LPSHOP CShopManager::GetByNPCVnum(uint32_t dwVnum)
{
	TShopMap::const_iterator it = m_map_pShopByNPCVnum.find(dwVnum);

	if (it == m_map_pShopByNPCVnum.end())
		return NULL;

	return (it->second);
}

bool CShopManager::StartShopping(LPCHARACTER pChr, LPCHARACTER pChrShopKeeper, int32_t iShopVnum)
{
	if (pChr->GetShopOwner() == pChrShopKeeper)
		return false;
	// this method is only for NPC
	if (pChrShopKeeper->IsPC())
		return false;

	//PREVENT_TRADE_WINDOW
	if (pChr->IsOpenSafebox() || pChr->GetExchange() || pChr->GetMyShop() || pChr->IsCubeOpen())
	{
		pChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade in the shop while another window is open."));
		return false;
	}
	//END_PREVENT_TRADE_WINDOW

	int32_t distance = DISTANCE_APPROX(pChr->GetX() - pChrShopKeeper->GetX(), pChr->GetY() - pChrShopKeeper->GetY());

	if (distance >= SHOP_MAX_DISTANCE)
	{
		TraceLog("SHOP: TOO_FAR: {} distance {}", pChr->GetName(), distance);
		return false;
	}

	LPSHOP pShop;

	if (iShopVnum)
		pShop = Get(iShopVnum);
	else
		pShop = GetByNPCVnum(pChrShopKeeper->GetRaceNum());

	if (!pShop)
	{
		TraceLog("SHOP: NO SHOP");
		return false;
	}

	bool bOtherEmpire = false;

	if (pChr->GetEmpire() != pChrShopKeeper->GetEmpire())
		bOtherEmpire = true;

	pShop->AddGuest(pChr, pChrShopKeeper->GetVID(), bOtherEmpire);
	pChr->SetShopOwner(pChrShopKeeper);
	PyLog("SHOP: START: {}", pChr->GetName());
	return true;
}

LPSHOP CShopManager::FindPCShop(uint32_t dwVID)
{
	TShopMap::iterator it = m_map_pShopByPC.find(dwVID);

	if (it == m_map_pShopByPC.end())
		return NULL;

	return it->second;
}

LPSHOP CShopManager::CreatePCShop(LPCHARACTER ch, TShopItemTable* pTable, uint8_t bItemCount)
{
	if (FindPCShop(ch->GetVID()))
		return NULL;

	LPSHOP pShop = M2_NEW CShop;
	pShop->SetPCShop(ch);
	pShop->SetShopItems(pTable, bItemCount);

	m_map_pShopByPC.insert(TShopMap::value_type(ch->GetVID(), pShop));
	return pShop;
}

void CShopManager::DestroyPCShop(LPCHARACTER ch)
{
	LPSHOP pShop = FindPCShop(ch->GetVID());

	if (!pShop)
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	m_map_pShopByPC.erase(ch->GetVID());
	M2_DELETE(pShop);
}

void CShopManager::StopShopping(LPCHARACTER ch)
{
	LPSHOP shop;

	if (!(shop = ch->GetShop()))
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	shop->RemoveGuest(ch);
	PyLog("SHOP: END: {}", ch->GetName());
}

void CShopManager::Buy(LPCHARACTER ch, uint8_t pos)
{
	if (!ch->GetShop())
		return;

	if (!ch->GetShopOwner())
		return;

	if (DISTANCE_APPROX(ch->GetX() - ch->GetShopOwner()->GetX(), ch->GetY() - ch->GetShopOwner()->GetY()) > 2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too far away from the shop to buy something."));
		return;
	}

	CShop* pShop = ch->GetShop();

	//PREVENT_ITEM_COPY
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	int32_t ret = pShop->Buy(ch, pos);

	if (SHOP_SUBHEADER_GC_OK != ret)
	{
		TPacketGCShop pack;

		pack.header	= HEADER_GC_SHOP;
		pack.subheader	= ret;
		pack.size	= sizeof(TPacketGCShop);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}
}

void CShopManager::Sell(LPCHARACTER ch, uint8_t bCell, uint8_t bCount)
{
	if (!ch->GetShop())
		return;

	if (!ch->GetShopOwner())
		return;

	if (!ch->CanHandleItem())
		return;

	if (ch->GetShop()->IsPCShop())
		return;

	if (DISTANCE_APPROX(ch->GetX()-ch->GetShopOwner()->GetX(), ch->GetY()-ch->GetShopOwner()->GetY())>2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too far away from the shop to sell something."));
		return;
	}
	
	LPITEM item = ch->GetInventoryItem(bCell);

	if (!item)
		return;

	if (item->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Equipped items cannot be sold."));
		return;
	}

	if (item->isLocked())
	{
		return;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_SELL))
		return;

	uint32_t dwPrice;

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	dwPrice = item->GetShopBuyPrice();

	if (IS_SET(item->GetFlag(), ITEM::FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	dwPrice /= 5;
	
	uint32_t dwTax = 0;
	int32_t iVal = 3;

	dwTax = dwPrice * iVal/100;
	dwPrice -= dwTax;

	if (test_server)
		PyLog("Sell Item price id {} {} itemid {}", ch->GetPlayerID(), ch->GetName(), item->GetID());

	const int64_t nTotalMoney = static_cast<int64_t>(ch->GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		SysLog("[OVERFLOW_GOLD] id {} name {} gold {}", ch->GetPlayerID(), ch->GetName(), ch->GetGold());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot trade because you are carrying more than 2 billion Yang."));
		return;
	}

	PyLog("SHOP: SELL: {} item name: {}(x{}):{} price: {}", ch->GetName(), item->GetName(), bCount, item->GetID(), dwPrice);

	if (iVal > 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This sale will be taxed %d%%."), iVal);
	}

	DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), dwPrice);

	if (bCount == item->GetCount())
	{
		ITEM_MANAGER::GetInstance()->RemoveItem(item, "SELL");
	}
	else
		item->SetCount(item->GetCount() - bCount);

	CMonarch::GetInstance()->SendtoDBAddMoney(dwTax, ch->GetEmpire(), ch);

	ch->PointChange(POINT_GOLD, dwPrice, false);
}

bool CompareShopItemName(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	TItemTable* lItem = ITEM_MANAGER::GetInstance()->GetTable(lhs.vnum);
	TItemTable* rItem = ITEM_MANAGER::GetInstance()->GetTable(rhs.vnum);
	if (lItem && rItem)
		return strcmp(lItem->szLocaleName, rItem->szLocaleName) < 0;
	else
		return true;
}

bool ConvertToShopItemTable(IN CGroupNode* pNode, OUT TShopTableEx& shopTable)
{
	if (!pNode->GetValue("vnum", 0, shopTable.dwVnum))
	{
		SysLog("Group {} does not have vnum.", pNode->GetNodeName().c_str());
		return false;
	}

	if (!pNode->GetValue("name", 0, shopTable.name))
	{
		SysLog("Group {} does not have name.", pNode->GetNodeName().c_str());
		return false;
	}
	
	if (shopTable.name.length() >= SHOP_TAB_NAME_MAX)
	{
		SysLog("Shop name length must be less than {}. Error in Group {}, name {}", SHOP_TAB_NAME_MAX, pNode->GetNodeName().c_str(), shopTable.name.c_str());
		return false;
	}

	std::string stCoinType;
	if (!pNode->GetValue("cointype", 0, stCoinType))
	{
		stCoinType = "Gold";
	}
	
	if (boost::iequals(stCoinType, "Gold"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_GOLD;
	}
	else if (boost::iequals(stCoinType, "SecondaryCoin"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_SECONDARY_COIN;
	}
	else
	{
		SysLog("Group {} has undefine cointype({}).", pNode->GetNodeName().c_str(), stCoinType.c_str());
		return false;
	}

	CGroupNode* pItemGroup = pNode->GetChildNode("items");
	if (!pItemGroup)
	{
		SysLog("Group {} does not have 'group items'.", pNode->GetNodeName().c_str());
		return false;
	}

	int32_t itemGroupSize = pItemGroup->GetRowCount();
	std::vector <TShopItemTable> shopItems(itemGroupSize);
	if (itemGroupSize >= SHOP_HOST_ITEM_MAX_NUM)
	{
		SysLog("count({}) of rows of group items of group {} must be smaller than {}", itemGroupSize, pNode->GetNodeName().c_str(), SHOP_HOST_ITEM_MAX_NUM);
		return false;
	}

	for (int32_t i = 0; i < itemGroupSize; i++)
	{
		if (!pItemGroup->GetValue(i, "vnum", shopItems[i].vnum))
		{
			SysLog("row({}) of group items of group {} does not have vnum column", i, pNode->GetNodeName().c_str());
			return false;
		}
		
		if (!pItemGroup->GetValue(i, "count", shopItems[i].count))
		{
			SysLog("row({}) of group items of group {} does not have count column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price", shopItems[i].price))
		{
			SysLog("row({}) of group items of group {} does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
	}
	std::string stSort;
	if (!pNode->GetValue("sort", 0, stSort))
	{
		stSort = "None";
	}

	if (boost::iequals(stSort, "Asc"))
	{
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemName);
	}
	else if(boost::iequals(stSort, "Desc"))
	{
		std::sort(shopItems.rbegin(), shopItems.rend(), CompareShopItemName);
	}

	CGrid grid = CGrid(5, 9);
	int32_t iPos;

	memset(&shopTable.items[0], 0, sizeof(shopTable.items));

	for (int32_t i = 0; i < shopItems.size(); i++)
	{
		TItemTable * item_table = ITEM_MANAGER::GetInstance()->GetTable(shopItems[i].vnum);
		if (!item_table)
		{
			SysLog("vnum({}) of group items of group {} does not exist", shopItems[i].vnum, pNode->GetNodeName().c_str());
			return false;
		}

		iPos = grid.FindBlank(1, item_table->bSize);

		grid.Put(iPos, 1, item_table->bSize);
		shopTable.items[iPos] = shopItems[i];
	}

	shopTable.byItemCount = shopItems.size();
	return true;
}

bool CShopManager::ReadShopTableEx(const char* stFileName)
{
	FILE* fp = fopen(stFileName, "rb");
	if (!fp)
		return true;
	fclose(fp);

	CGroupTextParseTreeLoader loader;
	if (!loader.Load(stFileName))
	{
		SysLog("{} Load fail.", stFileName);
		return false;
	}

	CGroupNode* pShopNPCGroup = loader.GetGroup("shopnpc");
	if (!pShopNPCGroup)
	{
		SysLog("Group ShopNPC is not exist.");
		return false;
	}

	typedef std::multimap <uint32_t, TShopTableEx> TMapNPCshop;
	TMapNPCshop map_npcShop;
	for (int32_t i = 0; i < pShopNPCGroup->GetRowCount(); i++)
	{
		uint32_t npcVnum;
		std::string shopName;
		if (!pShopNPCGroup->GetValue(i, "npc", npcVnum) || !pShopNPCGroup->GetValue(i, "group", shopName))
		{
			SysLog("Invalid row({}). Group ShopNPC rows must have 'npc', 'group' columns", i);
			return false;
		}
		std::transform(shopName.begin(), shopName.end(), shopName.begin(), (int32_t(*)(int32_t))std::tolower);
		CGroupNode* pShopGroup = loader.GetGroup(shopName.c_str());
		if (!pShopGroup)
		{
			SysLog("Group {} is not exist.", shopName.c_str());
			return false;
		}
		TShopTableEx table;
		if (!ConvertToShopItemTable(pShopGroup, table))
		{
			SysLog("Cannot read Group {}.", shopName.c_str());
			return false;
		}
		if (m_map_pShopByNPCVnum.find(npcVnum) != m_map_pShopByNPCVnum.end())
		{
			SysLog("{} cannot have both original shop and extended shop", npcVnum);
			return false;
		}
		
		map_npcShop.insert(TMapNPCshop::value_type(npcVnum, table));	
	}

	for (TMapNPCshop::iterator it = map_npcShop.begin(); it != map_npcShop.end(); ++it)
	{
		uint32_t npcVnum = it->first;
		TShopTableEx& table = it->second;
		if (m_map_pShop.find(table.dwVnum) != m_map_pShop.end())
		{
			SysLog("Shop vnum({}) already exists", table.dwVnum);
			return false;
		}
		TShopMap::iterator shop_it = m_map_pShopByNPCVnum.find(npcVnum);
		
		LPSHOPEX pShopEx = nullptr;
		if (m_map_pShopByNPCVnum.end() == shop_it)
		{
			pShopEx = M2_NEW CShopEx;
			pShopEx->Create(0, npcVnum);
			m_map_pShopByNPCVnum.insert(TShopMap::value_type(npcVnum, pShopEx));
		}
		else
		{
			pShopEx = dynamic_cast <CShopEx*> (shop_it->second);
			if (!pShopEx)
			{
				SysLog("WTF!!! It can't be happend. NPC({}) Shop is not extended version.", shop_it->first);
				return false;
			}
		}

		if (pShopEx->GetTabCount() >= SHOP_TAB_COUNT_MAX)
		{
			SysLog("ShopEx cannot have tab more than {}", SHOP_TAB_COUNT_MAX);
			return false;
		}

		if (pShopEx->GetVnum() != 0 && m_map_pShop.find(pShopEx->GetVnum()) != m_map_pShop.end())
		{
			SysLog("Shop vnum({}) already exist.", pShopEx->GetVnum());
			return false;
		}
		m_map_pShop.insert(TShopMap::value_type (pShopEx->GetVnum(), pShopEx));
		pShopEx->AddShopTable(table);
	}

	return true;
}
