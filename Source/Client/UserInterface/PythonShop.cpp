#include "StdAfx.h"
#include "PythonShop.h"

#include "PythonNetworkStream.h"

//BOOL CPythonShop::GetSlotItemID(uint32_t dwSlotPos, uint32_t* pdwItemID)
//{
//	if (!CheckSlotIndex(dwSlotPos))
//		return FALSE;
//	const TShopItemData * itemData;
//	if (!GetItemData(dwSlotPos, &itemData))
//		return FALSE;
//	*pdwItemID=itemData->vnum;
//	return TRUE;
//}
void CPythonShop::SetTabCoinType(uint8_t tabIdx, uint8_t coinType)
{
	if (tabIdx >= m_bTabCount)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}.", tabIdx, SHOP_TAB_COUNT_MAX);
		return;
	}
	m_aShoptabs[tabIdx].coinType = coinType;
}

uint8_t CPythonShop::GetTabCoinType(uint8_t tabIdx)
{
	if (tabIdx >= m_bTabCount)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}.", tabIdx, SHOP_TAB_COUNT_MAX);
		return 0xff;
	}
	return m_aShoptabs[tabIdx].coinType;
}

void CPythonShop::SetTabName(uint8_t tabIdx, const char* name)
{
	if (tabIdx >= m_bTabCount)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}.", tabIdx, SHOP_TAB_COUNT_MAX);
		return;
	}
	m_aShoptabs[tabIdx].name = name;
}

const char* CPythonShop::GetTabName(uint8_t tabIdx)
{
	if (tabIdx >= m_bTabCount)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}.", tabIdx, SHOP_TAB_COUNT_MAX);
		return NULL;
	}

	return m_aShoptabs[tabIdx].name.c_str();
}

void CPythonShop::SetItemData(uint32_t dwIndex, const TShopItemData& c_rShopItemData)
{
	uint8_t tabIdx = dwIndex / SHOP_HOST_ITEM_MAX_NUM;
	uint32_t dwSlotPos = dwIndex % SHOP_HOST_ITEM_MAX_NUM;

	SetItemData(tabIdx, dwSlotPos, c_rShopItemData);
}

BOOL CPythonShop::GetItemData(uint32_t dwIndex, const TShopItemData** c_ppItemData)
{
	uint8_t tabIdx = dwIndex / SHOP_HOST_ITEM_MAX_NUM;
	uint32_t dwSlotPos = dwIndex % SHOP_HOST_ITEM_MAX_NUM;

	return GetItemData(tabIdx, dwSlotPos, c_ppItemData);
}

void CPythonShop::SetItemData(uint8_t tabIdx, uint32_t dwSlotPos, const TShopItemData& c_rShopItemData)
{
	if (tabIdx >= SHOP_TAB_COUNT_MAX || dwSlotPos >= SHOP_HOST_ITEM_MAX_NUM)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}. dwSlotPos({}) must be less than {}", tabIdx, SHOP_TAB_COUNT_MAX, dwSlotPos, SHOP_HOST_ITEM_MAX_NUM);
		return;
	}

	m_aShoptabs[tabIdx].items[dwSlotPos] = c_rShopItemData;
}

BOOL CPythonShop::GetItemData(uint8_t tabIdx, uint32_t dwSlotPos, const TShopItemData** c_ppItemData)
{
	if (tabIdx >= SHOP_TAB_COUNT_MAX || dwSlotPos >= SHOP_HOST_ITEM_MAX_NUM)
	{
		TraceLog("Out of Index. tabIdx({}) must be less than {}. dwSlotPos({}) must be less than {}", tabIdx, SHOP_TAB_COUNT_MAX, dwSlotPos, SHOP_HOST_ITEM_MAX_NUM);
		return FALSE;
	}

	*c_ppItemData = &m_aShoptabs[tabIdx].items[dwSlotPos];

	return TRUE;
}
//
//BOOL CPythonShop::CheckSlotIndex(uint32_t dwSlotPos)
//{
//	if (dwSlotPos >= SHOP_HOST_ITEM_MAX_NUM * SHOP_TAB_COUNT_MAX)
//		return FALSE;
//
//	return TRUE;
//}

void CPythonShop::ClearPrivateShopStock()
{
	m_PrivateShopItemStock.clear();
}
void CPythonShop::AddPrivateShopItemStock(TItemPos ItemPos, uint8_t dwDisplayPos, uint32_t dwPrice)
{
	DelPrivateShopItemStock(ItemPos);

	TShopItemTable SellingItem;
	SellingItem.vnum = 0;
	SellingItem.count = 0;
	SellingItem.pos = ItemPos;
	SellingItem.price = dwPrice;
	SellingItem.display_pos = dwDisplayPos;
	m_PrivateShopItemStock.insert(std::make_pair(ItemPos, SellingItem));
}
void CPythonShop::DelPrivateShopItemStock(TItemPos ItemPos)
{
	if (m_PrivateShopItemStock.end() == m_PrivateShopItemStock.find(ItemPos))
		return;

	m_PrivateShopItemStock.erase(ItemPos);
}
int32_t CPythonShop::GetPrivateShopItemPrice(TItemPos ItemPos)
{
	TPrivateShopItemStock::iterator itor = m_PrivateShopItemStock.find(ItemPos);

	if (m_PrivateShopItemStock.end() == itor)
		return 0;

	TShopItemTable& rShopItemTable = itor->second;
	return rShopItemTable.price;
}
struct ItemStockSortFunc
{
	bool operator() (TShopItemTable& rkLeft, TShopItemTable& rkRight)
	{
		return rkLeft.display_pos < rkRight.display_pos;
	}
};
void CPythonShop::BuildPrivateShop(const char* c_szName)
{
	std::vector<TShopItemTable> ItemStock;
	ItemStock.reserve(m_PrivateShopItemStock.size());

	TPrivateShopItemStock::iterator itor = m_PrivateShopItemStock.begin();
	for (; itor != m_PrivateShopItemStock.end(); ++itor)
	{
		ItemStock.push_back(itor->second);
	}

	std::sort(ItemStock.begin(), ItemStock.end(), ItemStockSortFunc());

	CPythonNetworkStream::GetInstance()->SendBuildPrivateShopPacket(c_szName, ItemStock);
}

void CPythonShop::Open(BOOL isPrivateShop, BOOL isMainPrivateShop)
{
	m_isShoping = TRUE;
	m_isPrivateShop = isPrivateShop;
	m_isMainPlayerPrivateShop = isMainPrivateShop;
}

void CPythonShop::Close()
{
	m_isShoping = FALSE;
	m_isPrivateShop = FALSE;
	m_isMainPlayerPrivateShop = FALSE;
}

BOOL CPythonShop::IsOpen()
{
	return m_isShoping;
}

BOOL CPythonShop::IsPrivateShop()
{
	return m_isPrivateShop;
}

BOOL CPythonShop::IsMainPlayerPrivateShop()
{
	return m_isMainPlayerPrivateShop;
}

void CPythonShop::Clear()
{
	m_isShoping = FALSE;
	m_isPrivateShop = FALSE;
	m_isMainPlayerPrivateShop = FALSE;
	ClearPrivateShopStock();
	m_bTabCount = 1;

	for (int32_t i = 0; i < SHOP_TAB_COUNT_MAX; i++)
		memset(m_aShoptabs[i].items, 0, sizeof(TShopItemData) * SHOP_HOST_ITEM_MAX_NUM);
}

CPythonShop::CPythonShop(void)
{
	Clear();
}

CPythonShop::~CPythonShop(void)
{
}

static void shopOpen(BOOL isPrivateShop, BOOL isMainPrivateShop)
{
	auto rkShop=CPythonShop::GetInstance();
	rkShop->Open(isPrivateShop, isMainPrivateShop);

}

static void shopClose()
{
	auto rkShop=CPythonShop::GetInstance();
	rkShop->Close();

}

static BOOL shopIsOpen()
{
	auto rkShop=CPythonShop::GetInstance();
	return  rkShop->IsOpen();
}

static BOOL shopIsPrivateShop()
{
	auto rkShop = CPythonShop::GetInstance();
	return rkShop->IsPrivateShop();
}

static BOOL shopIsMainPlayerPrivateShop()
{
	auto rkShop=CPythonShop::GetInstance();
	return  rkShop->IsMainPlayerPrivateShop();
}

static uint32_t shopGetItemID(uint32_t nPos)
{

	const TShopItemData * c_pItemData;
	if (CPythonShop::GetInstance()->GetItemData(nPos, &c_pItemData))
		return  c_pItemData->vnum;

	return  0U;
}

static uint8_t shopGetItemCount(uint32_t iIndex)
{

	const TShopItemData * c_pItemData;
	if (CPythonShop::GetInstance()->GetItemData(iIndex, &c_pItemData))
		return  c_pItemData->count;

	return  0;
}

static uint32_t shopGetItemPrice(uint32_t iIndex)
{

	const TShopItemData * c_pItemData;
	if (CPythonShop::GetInstance()->GetItemData(iIndex, &c_pItemData))
		return  c_pItemData->price;

	return  0;
}

static int32_t shopGetItemMetinSocket(uint32_t iIndex, size_t iMetinSocketIndex)
{

	const TShopItemData * c_pItemData;
	if (CPythonShop::GetInstance()->GetItemData(iIndex, &c_pItemData))
		return  c_pItemData->alSockets[iMetinSocketIndex];

	return  0;
}

static std::tuple<uint8_t,int16_t> shopGetItemAttribute(uint32_t iIndex, size_t iAttrSlotIndex)
{

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM::ATTRIBUTE_MAX_NUM)
	{
		const TShopItemData * c_pItemData;
		if (CPythonShop::GetInstance()->GetItemData(iIndex, &c_pItemData))
			return std::make_tuple( c_pItemData->aAttr[iAttrSlotIndex].bType, c_pItemData->aAttr[iAttrSlotIndex].sValue);
	}

	return std::make_tuple( 0, 0);
}

static uint8_t shopGetTabCount()
{
	return  CPythonShop::GetInstance()->GetTabCount();
}

static std::string shopGetTabName(uint8_t bTabIdx)
{

	return  CPythonShop::GetInstance()->GetTabName(bTabIdx);
}

static int shopGetTabCoinType(uint8_t bTabIdx)
{

	return  CPythonShop::GetInstance()->GetTabCoinType(bTabIdx);
}

static void shopClearPrivateShopStock()
{
	CPythonShop::GetInstance()->ClearPrivateShopStock();

}

static void shopAddPrivateShopItemStock(uint8_t bItemWindowType, uint16_t wItemSlotIndex, uint8_t iDisplaySlotIndex, uint32_t iPrice)
{

	CPythonShop::GetInstance()->AddPrivateShopItemStock(TItemPos(bItemWindowType, wItemSlotIndex), iDisplaySlotIndex, iPrice);

}

static void shopDelPrivateShopItemStock(uint8_t bItemWindowType, uint16_t wItemSlotIndex)
{

	CPythonShop::GetInstance()->DelPrivateShopItemStock(TItemPos(bItemWindowType, wItemSlotIndex));

}

static int shopGetPrivateShopItemPrice(uint8_t bItemWindowType, uint16_t wItemSlotIndex)
{

	int32_t iValue = CPythonShop::GetInstance()->GetPrivateShopItemPrice(TItemPos(bItemWindowType, wItemSlotIndex));
	return  iValue;
}

static void shopBuildPrivateShop(std::string szName)
{

	CPythonShop::GetInstance()->BuildPrivateShop(szName.c_str());

}



PYBIND11_EMBEDDED_MODULE(shop, m)
{
	m.def("Open",	shopOpen);
	m.def("Close",	shopClose);
	m.def("IsOpen",	shopIsOpen);
	m.def("IsPrivateShop",	shopIsPrivateShop);
	m.def("IsMainPlayerPrivateShop",	shopIsMainPlayerPrivateShop);
	m.def("GetItemID",	shopGetItemID);
	m.def("GetItemCount",	shopGetItemCount);
	m.def("GetItemPrice",	shopGetItemPrice);
	m.def("GetItemMetinSocket",	shopGetItemMetinSocket);
	m.def("GetItemAttribute",	shopGetItemAttribute);
	m.def("GetTabCount",	shopGetTabCount);
	m.def("GetTabName",	shopGetTabName);
	m.def("GetTabCoinType",	shopGetTabCoinType);
	m.def("ClearPrivateShopStock",	shopClearPrivateShopStock);
	m.def("AddPrivateShopItemStock",	shopAddPrivateShopItemStock);
	m.def("DelPrivateShopItemStock",	shopDelPrivateShopItemStock);
	m.def("GetPrivateShopItemPrice",	shopGetPrivateShopItemPrice);
	m.def("BuildPrivateShop",	shopBuildPrivateShop);

	m.attr("SHOP_SLOT_COUNT") = int32_t(SHOP_HOST_ITEM_MAX_NUM);
	m.attr("SHOP_COIN_TYPE_GOLD") = int32_t(SHOP_COIN_TYPE_GOLD);
	m.attr("SHOP_COIN_TYPE_SECONDARY_COIN") = int32_t(SHOP_COIN_TYPE_SECONDARY_COIN);
}
