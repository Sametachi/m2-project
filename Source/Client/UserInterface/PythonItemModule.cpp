#include "StdAfx.h"
#include "PythonItem.h"

#include "../../Libraries/gameLib/ItemManager.h"
#include "InstanceBase.h"
#include "PythonApplication.h"

static void itemSetUseSoundFileName(uint32_t iUseSound, std::string szFileName)
{
	
	auto rkItem=CPythonItem::GetInstance();
	rkItem->SetUseSoundFileName(iUseSound, szFileName);

}

static void itemSetDropSoundFileName(uint32_t iDropSound, std::string szFileName)
{
	
	auto rkItem=CPythonItem::GetInstance();
	rkItem->SetDropSoundFileName(iDropSound, szFileName);

}

static void itemSelectItem(uint32_t iIndex)
{
	if (!CItemManager::GetInstance()->SelectItemData(iIndex))
	{
		TraceLog("Cannot find item by {}", iIndex);
		CItemManager::GetInstance()->SelectItemData(60001);
	}

}

static std::string itemGetItemName()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetName();
}

static std::string itemGetItemDescription()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetDescription();
}

static std::string itemGetItemSummary()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetSummary();
}

static CGraphicSubImage* itemGetIconImage()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

//	if (ITEM::TYPE_SKILLBOOK == pItemData->GetType())
//	{
//		char szItemName[64+1];
//		_snprintf(szItemName, "d:/ymir work/ui/items/etc/book_%02d.sub", );
//		CGraphicImage * pImage = (CGraphicImage *)CResourceManager::GetInstance()->GetResourcePointer(szItemName);
//	}

	return  pItemData->GetIconImage();
}

static std::string itemGetIconImageFileName()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	CGraphicSubImage * pImage = pItemData->GetIconImage();
	if (!pImage)
		return  "Noname";

	return  pImage->GetFileName();
}

static std::tuple<int32_t,uint8_t> itemGetItemSize()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return std::make_tuple(1, pItemData->GetSize());
}

static uint8_t itemGetItemType()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetType();
}

static uint8_t itemGetItemSubType()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetSubType();
}

static uint32_t itemGetIBuyItemPrice()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetIBuyItemPrice();
}

static uint32_t itemGetISellItemPrice()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->GetISellItemPrice();
}

static BOOL itemIsAntiFlag(uint32_t iFlag)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->IsAntiFlag(iFlag);
}

static BOOL itemIsFlag(uint32_t iFlag)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->IsFlag(iFlag);
}

static BOOL itemIsWearableFlag(uint32_t iFlag)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->IsWearableFlag(iFlag);
}

static BOOL itemIs1GoldItem()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("no selected item data");

	return  pItemData->IsFlag(ITEM::FLAG_COUNT_PER_1GOLD);
}

static std::tuple<uint8_t,int32_t> itemGetLimit(uint8_t iValueIndex)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	TItemLimit ItemLimit;
	if (!pItemData->GetLimit(iValueIndex, &ItemLimit))
		throw std::runtime_error("Cannot get limit");

	return std::make_tuple( ItemLimit.bType, ItemLimit.lValue);
}

static std::tuple<uint8_t,int32_t> itemGetAffect(uint8_t iValueIndex)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	TItemApply ItemApply;
	if (!pItemData->GetApply(iValueIndex, &ItemApply))
		throw std::runtime_error("Cannot get apply");

	if ((ITEM::APPLY_ATT_SPEED == ItemApply.bType) && (ITEM::TYPE_WEAPON == pItemData->GetType()) && (ITEM::WEAPON_TWO_HANDED == pItemData->GetSubType()))
	{
		ItemApply.lValue -= 10;
	}

	return std::make_tuple( ItemApply.bType, ItemApply.lValue);
}

static int32_t itemGetValue(uint8_t iValueIndex)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	return  pItemData->GetValue(iValueIndex);
}

static int32_t itemGetSocket(uint8_t iValueIndex)
{

	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	return  pItemData->GetSocket(iValueIndex);
}

static auto itemGetIconInstance()
{
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	CGraphicSubImage * pImage = pItemData->GetIconImage();
	if (!pImage)
		throw std::runtime_error("Cannot get icon image by " + std::to_string(pItemData->GetIndex()));

	auto pImageInstance = new CGraphicImageInstance;
	pImageInstance->SetImagePointer(pImage);
	return pybind11::capsule(pImageInstance, ImageCapsuleDestroyer);
}

static std::string itemGetUseType(uint32_t iItemVID)
{

	CItemManager::GetInstance()->SelectItemData(iItemVID);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");
	
	return  pItemData->GetUseTypeString();
}

static void itemDeleteIconInstance(pybind11::capsule iHandle)
{

}

static BOOL itemIsEquipmentVID(uint32_t iItemVID)
{

	CItemManager::GetInstance()->SelectItemData(iItemVID);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Not yet select item data");

	return  pItemData->IsEquipment();
}

static bool itemIsRefineScroll(uint32_t iItemIndex)
{

	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");

	if (pItemData->GetType() != ITEM::TYPE_USE)
		return  false;

	switch (pItemData->GetSubType())
	{
		case ITEM::USE_TUNING:
			return true;
		default:
			break;
	}
	
	return false;
}

static bool itemIsDetachScroll(uint32_t iItemIndex)
{

	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");

	int32_t iType = pItemData->GetType();
	int32_t iSubType = pItemData->GetSubType();

	return iType == ITEM::TYPE_USE && iSubType == ITEM::USE_DETACHMENT;
}

static bool itemIsKey(uint32_t iItemIndex)
{

	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");

	return ITEM::TYPE_TREASURE_KEY == pItemData->GetType();
}

static bool itemIsMetin(uint32_t iItemIndex)
{

	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");

	return ITEM::TYPE_METIN == pItemData->GetType();
}

static bool itemCanAddToQuickSlotItem(uint32_t iItemIndex)
{

	CItemManager::GetInstance()->SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::GetInstance()->GetSelectedItemDataPointer();
	if (!pItemData)
		throw std::runtime_error("Can't find select item data");

	return ITEM::TYPE_USE == pItemData->GetType() || ITEM::TYPE_QUEST == pItemData->GetType();
}

static void itemUpdate()
{
	auto rkApp=CPythonApplication::GetInstance();

	POINT ptMouse;
	rkApp->GetMousePosition(&ptMouse);

	CPythonItem::GetInstance()->Update(ptMouse);

}

static void itemRender()
{
	CPythonItem::GetInstance()->Render();

}

static void itemCreateItem(uint32_t iVirtualID, uint32_t iVirtualNumber, float x, float y, float z, bool bDrop)
{
		CPythonItem::GetInstance()->CreateItem(iVirtualID, iVirtualNumber, x, y, z, bDrop);


}

static void itemDeleteItem(uint32_t iVirtualID)
{

	CPythonItem::GetInstance()->DeleteItem(iVirtualID);

}

static int64_t itemPick()
{
	uint32_t dwItemID;
	if (CPythonItem::GetInstance()->GetPickedItemID(&dwItemID))
		return  dwItemID;

	return  -1;
}

static void itemLoadItemTable(std::string szFileName)
{

	CItemManager::GetInstance()->LoadItemTable(szFileName.c_str());

}



PYBIND11_EMBEDDED_MODULE(item, m)
{
	m.def("SetUseSoundFileName",	itemSetUseSoundFileName);
	m.def("SetDropSoundFileName",	itemSetDropSoundFileName);
	m.def("SelectItem",	itemSelectItem);
	m.def("GetItemName",	itemGetItemName);
	m.def("GetItemDescription",	itemGetItemDescription);
	m.def("GetItemSummary",	itemGetItemSummary);
	m.def("GetIconImage",	itemGetIconImage);
	m.def("GetIconImageFileName",	itemGetIconImageFileName);
	m.def("GetItemSize",	itemGetItemSize);
	m.def("GetItemType",	itemGetItemType);
	m.def("GetItemSubType",	itemGetItemSubType);
	m.def("GetIBuyItemPrice",	itemGetIBuyItemPrice);
	m.def("GetISellItemPrice",	itemGetISellItemPrice);
	m.def("IsAntiFlag",	itemIsAntiFlag);
	m.def("IsFlag",	itemIsFlag);
	m.def("IsWearableFlag",	itemIsWearableFlag);
	m.def("Is1GoldItem",	itemIs1GoldItem);
	m.def("GetLimit",	itemGetLimit);
	m.def("GetAffect",	itemGetAffect);
	m.def("GetValue",	itemGetValue);
	m.def("GetSocket",	itemGetSocket);
	m.def("GetIconInstance",	itemGetIconInstance);
	m.def("GetUseType",	itemGetUseType);
	m.def("DeleteIconInstance",	itemDeleteIconInstance);
	m.def("IsEquipmentVID",	itemIsEquipmentVID);
	m.def("IsRefineScroll",	itemIsRefineScroll);
	m.def("IsDetachScroll",	itemIsDetachScroll);
	m.def("IsKey",	itemIsKey);
	m.def("IsMetin",	itemIsMetin);
	m.def("CanAddToQuickSlotItem",	itemCanAddToQuickSlotItem);
	m.def("Update",	itemUpdate);
	m.def("Render",	itemRender);
	m.def("CreateItem",	itemCreateItem);
	m.def("DeleteItem",	itemDeleteItem);
	m.def("Pick",	itemPick);
	m.def("LoadItemTable",	itemLoadItemTable);

	m.attr("USESOUND_ACCESSORY") = int32_t(CPythonItem::USESOUND_ACCESSORY);
	m.attr("USESOUND_ARMOR") = int32_t(CPythonItem::USESOUND_ARMOR);
	m.attr("USESOUND_BOW") = int32_t(CPythonItem::USESOUND_BOW);
	m.attr("USESOUND_DEFAULT") = int32_t(CPythonItem::USESOUND_DEFAULT);
	m.attr("USESOUND_WEAPON") = int32_t(CPythonItem::USESOUND_WEAPON);
	m.attr("USESOUND_POTION") = int32_t(CPythonItem::USESOUND_POTION);
	m.attr("USESOUND_PORTAL") = int32_t(CPythonItem::USESOUND_PORTAL);
	m.attr("DROPSOUND_ACCESSORY") = int32_t(CPythonItem::DROPSOUND_ACCESSORY);
	m.attr("DROPSOUND_ARMOR") = int32_t(CPythonItem::DROPSOUND_ARMOR);
	m.attr("DROPSOUND_BOW") = int32_t(CPythonItem::DROPSOUND_BOW);
	m.attr("DROPSOUND_DEFAULT") = int32_t(CPythonItem::DROPSOUND_DEFAULT);
	m.attr("DROPSOUND_WEAPON") = int32_t(CPythonItem::DROPSOUND_WEAPON);
	m.attr("EQUIPMENT_COUNT") = c_Equipment_Count;
	m.attr("EQUIPMENT_HEAD") = c_Equipment_Head;
	m.attr("EQUIPMENT_BODY") = c_Equipment_Body;
	m.attr("EQUIPMENT_WEAPON") = c_Equipment_Weapon;
	m.attr("EQUIPMENT_WRIST") = c_Equipment_Wrist;
	m.attr("EQUIPMENT_SHOES") = c_Equipment_Shoes;
	m.attr("EQUIPMENT_NECK") = c_Equipment_Neck;
	m.attr("EQUIPMENT_EAR") = c_Equipment_Ear;
	m.attr("EQUIPMENT_UNIQUE1") = c_Equipment_Unique1;
	m.attr("EQUIPMENT_UNIQUE2") = c_Equipment_Unique2;
	m.attr("EQUIPMENT_ARROW") = c_Equipment_Arrow;

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	m.attr("EQUIPMENT_RING1") = c_Equipment_Ring1;
	m.attr("EQUIPMENT_RING2") = c_Equipment_Ring2;
	m.attr("EQUIPMENT_BELT") = c_Equipment_Belt;
#endif

	m.attr("ITEM_TYPE_NONE") = int32_t(ITEM::TYPE_NONE);
	m.attr("ITEM_TYPE_WEAPON") = int32_t(ITEM::TYPE_WEAPON);
	m.attr("ITEM_TYPE_ARMOR") = int32_t(ITEM::TYPE_ARMOR);
	m.attr("ITEM_TYPE_USE") = int32_t(ITEM::TYPE_USE);
	m.attr("ITEM_TYPE_AUTOUSE") = int32_t(ITEM::TYPE_AUTOUSE);
	m.attr("ITEM_TYPE_MATERIAL") = int32_t(ITEM::TYPE_MATERIAL);
	m.attr("ITEM_TYPE_SPECIAL") = int32_t(ITEM::TYPE_SPECIAL);
	m.attr("ITEM_TYPE_TOOL") = int32_t(ITEM::TYPE_TOOL);
	m.attr("ITEM_TYPE_LOTTERY") = int32_t(ITEM::TYPE_LOTTERY);
	m.attr("ITEM_TYPE_ELK") = int32_t(ITEM::TYPE_ELK);
	m.attr("ITEM_TYPE_METIN") = int32_t(ITEM::TYPE_METIN);
	m.attr("ITEM_TYPE_CONTAINER") = int32_t(ITEM::TYPE_CONTAINER);
	m.attr("ITEM_TYPE_FISH") = int32_t(ITEM::TYPE_FISH);
	m.attr("ITEM_TYPE_ROD") = int32_t(ITEM::TYPE_ROD);
	m.attr("ITEM_TYPE_RESOURCE") = int32_t(ITEM::TYPE_RESOURCE);
	m.attr("ITEM_TYPE_CAMPFIRE") = int32_t(ITEM::TYPE_CAMPFIRE);
	m.attr("ITEM_TYPE_UNIQUE") = int32_t(ITEM::TYPE_UNIQUE);
	m.attr("ITEM_TYPE_SKILLBOOK") = int32_t(ITEM::TYPE_SKILLBOOK);
	m.attr("ITEM_TYPE_QUEST") = int32_t(ITEM::TYPE_QUEST);
	m.attr("ITEM_TYPE_POLYMORPH") = int32_t(ITEM::TYPE_POLYMORPH);
	m.attr("ITEM_TYPE_TREASURE_BOX") = int32_t(ITEM::TYPE_TREASURE_BOX);
	m.attr("ITEM_TYPE_TREASURE_KEY") = int32_t(ITEM::TYPE_TREASURE_KEY);
	m.attr("ITEM_TYPE_PICK") = int32_t(ITEM::TYPE_PICK);
	m.attr("ITEM_TYPE_BLEND") = int32_t(ITEM::TYPE_BLEND);
	m.attr("ITEM_TYPE_DS") = int32_t(ITEM::TYPE_DS);
	m.attr("ITEM_TYPE_SPECIAL_DS") = int32_t(ITEM::TYPE_SPECIAL_DS);
	m.attr("ITEM_TYPE_RING") = int32_t(ITEM::TYPE_RING);
	m.attr("ITEM_TYPE_BELT") = int32_t(ITEM::TYPE_BELT);

	m.attr("ITEM_TYPE_COSTUME") = int32_t(ITEM::TYPE_COSTUME);
	m.attr("COSTUME_TYPE_BODY") = int32_t(ITEM::COSTUME_BODY);
	m.attr("COSTUME_TYPE_HAIR") = int32_t(ITEM::COSTUME_HAIR);
	m.attr("COSTUME_SLOT_START") = c_Costume_Slot_Start;
	m.attr("COSTUME_SLOT_COUNT") = c_Costume_Slot_Count;
	m.attr("COSTUME_SLOT_BODY") = c_Costume_Slot_Body;
	m.attr("COSTUME_SLOT_HAIR") = c_Costume_Slot_Hair;
	m.attr("COSTUME_SLOT_END") = c_Costume_Slot_End;

#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	m.attr("BELT_INVENTORY_SLOT_START") = c_Belt_Inventory_Slot_Start;
	m.attr("BELT_INVENTORY_SLOT_COUNT") = c_Belt_Inventory_Slot_Count;
	m.attr("BELT_INVENTORY_SLOT_END") =	c_Belt_Inventory_Slot_End;
#endif

	m.attr("WEAPON_SWORD") = int32_t(ITEM::WEAPON_SWORD);
	m.attr("WEAPON_DAGGER") = int32_t(ITEM::WEAPON_DAGGER);
	m.attr("WEAPON_BOW") = int32_t(ITEM::WEAPON_BOW);
	m.attr("WEAPON_TWO_HANDED") = int32_t(ITEM::WEAPON_TWO_HANDED);
	m.attr("WEAPON_BELL") = int32_t(ITEM::WEAPON_BELL);
	m.attr("WEAPON_FAN") = int32_t(ITEM::WEAPON_FAN);
	m.attr("WEAPON_ARROW") = int32_t(ITEM::WEAPON_ARROW);
	m.attr("WEAPON_NUM_TYPES") = int32_t(ITEM::WEAPON_NUM_TYPES);
	m.attr("USE_POTION") = int32_t(ITEM::USE_POTION);
	m.attr("USE_TALISMAN") = int32_t(ITEM::USE_TALISMAN);
	m.attr("USE_TUNING") = int32_t(ITEM::USE_TUNING);
	m.attr("USE_MOVE") = int32_t(ITEM::USE_MOVE);
	m.attr("USE_TREASURE_BOX") = int32_t(ITEM::USE_TREASURE_BOX);
	m.attr("USE_MONEYBAG") = int32_t(ITEM::USE_MONEYBAG);
	m.attr("USE_BAIT") = int32_t(ITEM::USE_BAIT);
	m.attr("USE_ABILITY_UP") = int32_t(ITEM::USE_ABILITY_UP);
	m.attr("USE_AFFECT") = int32_t(ITEM::USE_AFFECT);
	m.attr("USE_CREATE_STONE") = int32_t(ITEM::USE_CREATE_STONE);
	m.attr("USE_SPECIAL") = int32_t(ITEM::USE_SPECIAL);
	m.attr("USE_POTION_NODELAY") = int32_t(ITEM::USE_POTION_NODELAY);
	m.attr("USE_CLEAR") = int32_t(ITEM::USE_CLEAR);
	m.attr("USE_INVISIBILITY") = int32_t(ITEM::USE_INVISIBILITY);
	m.attr("USE_DETACHMENT") = int32_t(ITEM::USE_DETACHMENT);
	m.attr("USE_TIME_CHARGE_PER") = int32_t(ITEM::USE_TIME_CHARGE_PER);
	m.attr("USE_TIME_CHARGE_FIX") = int32_t(ITEM::USE_TIME_CHARGE_FIX);
	m.attr("MATERIAL_DS_REFINE_NORMAL") = int32_t(ITEM::MATERIAL_DS_REFINE_NORMAL);
	m.attr("MATERIAL_DS_REFINE_BLESSED") = int32_t(ITEM::MATERIAL_DS_REFINE_BLESSED);
	m.attr("MATERIAL_DS_REFINE_HOLLY") = int32_t(ITEM::MATERIAL_DS_REFINE_HOLLY);
	m.attr("METIN_NORMAL") = int32_t(ITEM::METIN_NORMAL);
	m.attr("METIN_GOLD") = int32_t(ITEM::METIN_GOLD);
	m.attr("LIMIT_NONE") = int32_t(ITEM::LIMIT_NONE);
	m.attr("LIMIT_LEVEL") = int32_t(ITEM::LIMIT_LEVEL);
	m.attr("LIMIT_STR") = int32_t(ITEM::LIMIT_STR);
	m.attr("LIMIT_DEX") = int32_t(ITEM::LIMIT_DEX);
	m.attr("LIMIT_INT") = int32_t(ITEM::LIMIT_INT);
	m.attr("LIMIT_CON") = int32_t(ITEM::LIMIT_CON);
	m.attr("LIMIT_REAL_TIME") = int32_t(ITEM::LIMIT_REAL_TIME);
	m.attr("LIMIT_REAL_TIME_START_FIRST_USE") = int32_t(ITEM::LIMIT_REAL_TIME_START_FIRST_USE);
	m.attr("LIMIT_TIMER_BASED_ON_WEAR") = int32_t(ITEM::LIMIT_TIMER_BASED_ON_WEAR);
	m.attr("LIMIT_TYPE_MAX_NUM") = int32_t(ITEM::LIMIT_MAX_NUM);
	m.attr("LIMIT_MAX_NUM") = int32_t(ITEM::LIMIT_SLOT_MAX_NUM);
	m.attr("ITEM_ANTIFLAG_FEMALE") = int32_t(ITEM::ANTIFLAG_FEMALE);
	m.attr("ITEM_ANTIFLAG_MALE") = int32_t(ITEM::ANTIFLAG_MALE);
	m.attr("ITEM_ANTIFLAG_WARRIOR") = int32_t(ITEM::ANTIFLAG_WARRIOR);
	m.attr("ITEM_ANTIFLAG_ASSASSIN") = int32_t(ITEM::ANTIFLAG_ASSASSIN);
	m.attr("ITEM_ANTIFLAG_SURA") = int32_t(ITEM::ANTIFLAG_SURA);
	m.attr("ITEM_ANTIFLAG_SHAMAN") = int32_t(ITEM::ANTIFLAG_SHAMAN);
	m.attr("ITEM_ANTIFLAG_GET") = int32_t(ITEM::ANTIFLAG_GET);
	m.attr("ITEM_ANTIFLAG_DROP") = int32_t(ITEM::ANTIFLAG_DROP);
	m.attr("ITEM_ANTIFLAG_SELL") = int32_t(ITEM::ANTIFLAG_SELL);
	m.attr("ITEM_ANTIFLAG_EMPIRE_A") = int32_t(ITEM::ANTIFLAG_EMPIRE_A);
	m.attr("ITEM_ANTIFLAG_EMPIRE_B") = int32_t(ITEM::ANTIFLAG_EMPIRE_B);
	m.attr("ITEM_ANTIFLAG_EMPIRE_R") = int32_t(ITEM::ANTIFLAG_EMPIRE_R);
	m.attr("ITEM_ANTIFLAG_SAVE") = int32_t(ITEM::ANTIFLAG_SAVE);
	m.attr("ITEM_ANTIFLAG_GIVE") = int32_t(ITEM::ANTIFLAG_GIVE);
	m.attr("ITEM_ANTIFLAG_PKDROP") = int32_t(ITEM::ANTIFLAG_PKDROP);
	m.attr("ITEM_ANTIFLAG_STACK") = int32_t(ITEM::ANTIFLAG_STACK);
	m.attr("ITEM_ANTIFLAG_MYSHOP") = int32_t(ITEM::ANTIFLAG_MYSHOP);
	m.attr("ITEM_FLAG_UNIQUE") = int32_t(ITEM::FLAG_UNIQUE);
	m.attr("ITEM_FLAG_CONFIRM_WHEN_USE") = int32_t(ITEM::FLAG_CONFIRM_WHEN_USE);
	m.attr("ANTIFLAG_FEMALE") = int32_t(ITEM::ANTIFLAG_FEMALE);
	m.attr("ANTIFLAG_MALE") = int32_t(ITEM::ANTIFLAG_MALE);
	m.attr("ANTIFLAG_WARRIOR") = int32_t(ITEM::ANTIFLAG_WARRIOR);
	m.attr("ANTIFLAG_ASSASSIN") = int32_t(ITEM::ANTIFLAG_ASSASSIN);
	m.attr("ANTIFLAG_SURA") = int32_t(ITEM::ANTIFLAG_SURA);
	m.attr("ANTIFLAG_SHAMAN") = int32_t(ITEM::ANTIFLAG_SHAMAN);
	m.attr("ANTIFLAG_GET") = int32_t(ITEM::ANTIFLAG_GET);
	m.attr("ANTIFLAG_DROP") = int32_t(ITEM::ANTIFLAG_DROP);
	m.attr("ANTIFLAG_SELL") = int32_t(ITEM::ANTIFLAG_SELL);
	m.attr("ANTIFLAG_EMPIRE_A") = int32_t(ITEM::ANTIFLAG_EMPIRE_A);
	m.attr("ANTIFLAG_EMPIRE_B") = int32_t(ITEM::ANTIFLAG_EMPIRE_B);
	m.attr("ANTIFLAG_EMPIRE_R") = int32_t(ITEM::ANTIFLAG_EMPIRE_R);
	m.attr("ANTIFLAG_SAVE") = int32_t(ITEM::ANTIFLAG_SAVE);
	m.attr("ANTIFLAG_GIVE") = int32_t(ITEM::ANTIFLAG_GIVE);
	m.attr("ANTIFLAG_PKDROP") = int32_t(ITEM::ANTIFLAG_PKDROP);
	m.attr("ANTIFLAG_STACK") = int32_t(ITEM::ANTIFLAG_STACK);
	m.attr("ANTIFLAG_MYSHOP") = int32_t(ITEM::ANTIFLAG_MYSHOP);
	m.attr("WEARABLE_BODY") = int32_t(ITEM::WEARABLE_BODY);
	m.attr("WEARABLE_HEAD") = int32_t(ITEM::WEARABLE_HEAD);
	m.attr("WEARABLE_FOOTS") = int32_t(ITEM::WEARABLE_FOOTS);
	m.attr("WEARABLE_WRIST") = int32_t(ITEM::WEARABLE_WRIST);
	m.attr("WEARABLE_WEAPON") = int32_t(ITEM::WEARABLE_WEAPON);
	m.attr("WEARABLE_NECK") = int32_t(ITEM::WEARABLE_NECK);
	m.attr("WEARABLE_EAR") = int32_t(ITEM::WEARABLE_EAR);
	m.attr("WEARABLE_UNIQUE") = int32_t(ITEM::WEARABLE_UNIQUE);
	m.attr("WEARABLE_SHIELD") = int32_t(ITEM::WEARABLE_SHIELD);
	m.attr("WEARABLE_ARROW") = int32_t(ITEM::WEARABLE_ARROW);
	m.attr("ARMOR_BODY") = int32_t(ITEM::ARMOR_BODY);
	m.attr("ARMOR_HEAD") = int32_t(ITEM::ARMOR_HEAD);
	m.attr("ARMOR_SHIELD") = int32_t(ITEM::ARMOR_SHIELD);
	m.attr("ARMOR_WRIST") = int32_t(ITEM::ARMOR_WRIST);
	m.attr("ARMOR_FOOTS") = int32_t(ITEM::ARMOR_FOOTS);
	m.attr("ARMOR_NECK") = int32_t(ITEM::ARMOR_NECK);
	m.attr("ARMOR_EAR") = int32_t(ITEM::ARMOR_EAR);
	m.attr("ITEM_APPLY_MAX_NUM") = int32_t(ITEM::APPLY_MAX_NUM);
	m.attr("ITEM_SOCKET_MAX_NUM") = int32_t(ITEM::SOCKET_MAX_NUM);
	m.attr("APPLY_NONE") = int32_t(ITEM::APPLY_NONE);
	m.attr("APPLY_STR") = int32_t(ITEM::APPLY_STR);
	m.attr("APPLY_DEX") = int32_t(ITEM::APPLY_DEX);
	m.attr("APPLY_CON") = int32_t(ITEM::APPLY_CON);
	m.attr("APPLY_INT") = int32_t(ITEM::APPLY_INT);
	m.attr("APPLY_MAX_HP") = int32_t(ITEM::APPLY_MAX_HP);
	m.attr("APPLY_MAX_SP") = int32_t(ITEM::APPLY_MAX_SP);
	m.attr("APPLY_HP_REGEN") = int32_t(ITEM::APPLY_HP_REGEN);
	m.attr("APPLY_SP_REGEN") = int32_t(ITEM::APPLY_SP_REGEN);
	m.attr("APPLY_DEF_GRADE_BONUS") = int32_t(ITEM::APPLY_DEF_GRADE_BONUS);
	m.attr("APPLY_ATT_GRADE_BONUS") = int32_t(ITEM::APPLY_ATT_GRADE_BONUS);
	m.attr("APPLY_ATT_SPEED") = int32_t(ITEM::APPLY_ATT_SPEED);
	m.attr("APPLY_MOV_SPEED") = int32_t(ITEM::APPLY_MOV_SPEED);
	m.attr("APPLY_CAST_SPEED") = int32_t(ITEM::APPLY_CAST_SPEED);
	m.attr("APPLY_MAGIC_ATT_GRADE") = int32_t(ITEM::APPLY_MAGIC_ATT_GRADE);
	m.attr("APPLY_MAGIC_DEF_GRADE") = int32_t(ITEM::APPLY_MAGIC_DEF_GRADE);
	m.attr("APPLY_SKILL") = int32_t(ITEM::APPLY_SKILL);
	m.attr("APPLY_ATTBONUS_ANIMAL") = int32_t(ITEM::APPLY_ATTBONUS_ANIMAL);
	m.attr("APPLY_ATTBONUS_UNDEAD") = int32_t(ITEM::APPLY_ATTBONUS_UNDEAD);
	m.attr("APPLY_ATTBONUS_DEVIL") = int32_t(ITEM::APPLY_ATTBONUS_DEVIL);
	m.attr("APPLY_ATTBONUS_HUMAN") = int32_t(ITEM::APPLY_ATTBONUS_HUMAN);
	m.attr("APPLY_BOW_DISTANCE") = int32_t(ITEM::APPLY_BOW_DISTANCE);
	m.attr("APPLY_RESIST_BOW") = int32_t(ITEM::APPLY_RESIST_BOW);
	m.attr("APPLY_RESIST_FIRE") = int32_t(ITEM::APPLY_RESIST_FIRE);
	m.attr("APPLY_RESIST_ELEC") = int32_t(ITEM::APPLY_RESIST_ELEC);
	m.attr("APPLY_RESIST_MAGIC") = int32_t(ITEM::APPLY_RESIST_MAGIC);
	m.attr("APPLY_POISON_PCT") = int32_t(ITEM::APPLY_POISON_PCT);
	m.attr("APPLY_SLOW_PCT") = int32_t(ITEM::APPLY_SLOW_PCT);
	m.attr("APPLY_STUN_PCT") = int32_t(ITEM::APPLY_STUN_PCT);
	m.attr("APPLY_CRITICAL_PCT") = int32_t(ITEM::APPLY_CRITICAL_PCT);
	m.attr("APPLY_PENETRATE_PCT") = int32_t(ITEM::APPLY_PENETRATE_PCT);
	m.attr("APPLY_ATTBONUS_ORC") = int32_t(ITEM::APPLY_ATTBONUS_ORC);
	m.attr("APPLY_ATTBONUS_MILGYO") = int32_t(ITEM::APPLY_ATTBONUS_MILGYO);
	m.attr("APPLY_STEAL_HP") = int32_t(ITEM::APPLY_STEAL_HP);
	m.attr("APPLY_STEAL_SP") = int32_t(ITEM::APPLY_STEAL_SP);
	m.attr("APPLY_MANA_BURN_PCT") = int32_t(ITEM::APPLY_MANA_BURN_PCT);
	m.attr("APPLY_DAMAGE_SP_RECOVER") = int32_t(ITEM::APPLY_DAMAGE_SP_RECOVER);
	m.attr("APPLY_BLOCK") = int32_t(ITEM::APPLY_BLOCK);
	m.attr("APPLY_DODGE") = int32_t(ITEM::APPLY_DODGE);
	m.attr("APPLY_RESIST_SWORD") = int32_t(ITEM::APPLY_RESIST_SWORD);
	m.attr("APPLY_RESIST_TWOHAND") = int32_t(ITEM::APPLY_RESIST_TWOHAND);
	m.attr("APPLY_RESIST_DAGGER") = int32_t(ITEM::APPLY_RESIST_DAGGER);
	m.attr("APPLY_RESIST_BELL") = int32_t(ITEM::APPLY_RESIST_BELL);
	m.attr("APPLY_RESIST_FAN") = int32_t(ITEM::APPLY_RESIST_FAN);
	m.attr("APPLY_RESIST_WIND") = int32_t(ITEM::APPLY_RESIST_WIND);
	m.attr("APPLY_REFLECT_MELEE") = int32_t(ITEM::APPLY_REFLECT_MELEE);
	m.attr("APPLY_REFLECT_CURSE") = int32_t(ITEM::APPLY_REFLECT_CURSE);
	m.attr("APPLY_POISON_REDUCE") = int32_t(ITEM::APPLY_POISON_REDUCE);
	m.attr("APPLY_KILL_SP_RECOVER") = int32_t(ITEM::APPLY_KILL_SP_RECOVER);
	m.attr("APPLY_EXP_DOUBLE_BONUS") = int32_t(ITEM::APPLY_EXP_DOUBLE_BONUS);
	m.attr("APPLY_GOLD_DOUBLE_BONUS") = int32_t(ITEM::APPLY_GOLD_DOUBLE_BONUS);
	m.attr("APPLY_ITEM_DROP_BONUS") = int32_t(ITEM::APPLY_ITEM_DROP_BONUS);
	m.attr("APPLY_POTION_BONUS") = int32_t(ITEM::APPLY_POTION_BONUS);
	m.attr("APPLY_KILL_HP_RECOVER") = int32_t(ITEM::APPLY_KILL_HP_RECOVER);
	m.attr("APPLY_IMMUNE_STUN") = int32_t(ITEM::APPLY_IMMUNE_STUN);
	m.attr("APPLY_IMMUNE_SLOW") = int32_t(ITEM::APPLY_IMMUNE_SLOW);
	m.attr("APPLY_IMMUNE_FALL") = int32_t(ITEM::APPLY_IMMUNE_FALL);
	m.attr("APPLY_MAX_STAMINA") = int32_t(ITEM::APPLY_MAX_STAMINA);
	m.attr("APPLY_ATTBONUS_WARRIOR") = int32_t(ITEM::APPLY_ATT_BONUS_TO_WARRIOR);
	m.attr("APPLY_ATTBONUS_ASSASSIN") = int32_t(ITEM::APPLY_ATT_BONUS_TO_ASSASSIN);
	m.attr("APPLY_ATTBONUS_SURA") = int32_t(ITEM::APPLY_ATT_BONUS_TO_SURA);
	m.attr("APPLY_ATTBONUS_SHAMAN") = int32_t(ITEM::APPLY_ATT_BONUS_TO_SHAMAN);
	m.attr("APPLY_ATTBONUS_MONSTER") = int32_t(ITEM::APPLY_ATT_BONUS_TO_MONSTER);
	m.attr("APPLY_MALL_ATTBONUS") = int32_t(ITEM::APPLY_MALL_ATTBONUS);
	m.attr("APPLY_MALL_DEFBONUS") = int32_t(ITEM::APPLY_MALL_DEFBONUS);
	m.attr("APPLY_MALL_EXPBONUS") = int32_t(ITEM::APPLY_MALL_EXPBONUS);
	m.attr("APPLY_MALL_ITEMBONUS") = int32_t(ITEM::APPLY_MALL_ITEMBONUS);
	m.attr("APPLY_MALL_GOLDBONUS") = int32_t(ITEM::APPLY_MALL_GOLDBONUS);
	m.attr("APPLY_MAX_HP_PCT") = int32_t(ITEM::APPLY_MAX_HP_PCT);
	m.attr("APPLY_MAX_SP_PCT") = int32_t(ITEM::APPLY_MAX_SP_PCT);
	m.attr("APPLY_SKILL_DAMAGE_BONUS") = int32_t(ITEM::APPLY_SKILL_DAMAGE_BONUS);
	m.attr("APPLY_NORMAL_HIT_DAMAGE_BONUS") = int32_t(ITEM::APPLY_NORMAL_HIT_DAMAGE_BONUS);
	m.attr("APPLY_SKILL_DEFEND_BONUS") = int32_t(ITEM::APPLY_SKILL_DEFEND_BONUS);
	m.attr("APPLY_NORMAL_HIT_DEFEND_BONUS") = int32_t(ITEM::APPLY_NORMAL_HIT_DEFEND_BONUS);
	m.attr("APPLY_RESIST_WARRIOR") = int32_t(ITEM::APPLY_RESIST_WARRIOR);
	m.attr("APPLY_RESIST_ASSASSIN") = int32_t(ITEM::APPLY_RESIST_ASSASSIN);
	m.attr("APPLY_RESIST_SURA") = int32_t(ITEM::APPLY_RESIST_SURA);
	m.attr("APPLY_RESIST_SHAMAN") = int32_t(ITEM::APPLY_RESIST_SHAMAN);
	m.attr("APPLY_COSTUME_ATTR_BONUS") = int32_t(ITEM::APPLY_COSTUME_ATTR_BONUS);
	m.attr("APPLY_MAGIC_ATTBONUS_PER") = int32_t(ITEM::APPLY_MAGIC_ATTBONUS_PER);
	m.attr("APPLY_MELEE_MAGIC_ATTBONUS_PER") = int32_t(ITEM::APPLY_MELEE_MAGIC_ATTBONUS_PER);
	m.attr("APPLY_RESIST_ICE") = int32_t(ITEM::APPLY_RESIST_ICE);
	m.attr("APPLY_RESIST_EARTH") = int32_t(ITEM::APPLY_RESIST_EARTH);
	m.attr("APPLY_RESIST_DARK") = int32_t(ITEM::APPLY_RESIST_DARK);
	m.attr("APPLY_ANTI_CRITICAL_PCT") = int32_t(ITEM::APPLY_ANTI_CRITICAL_PCT);
	m.attr("APPLY_ANTI_PENETRATE_PCT") = int32_t(ITEM::APPLY_ANTI_PENETRATE_PCT);
}
