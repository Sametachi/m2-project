#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonItem.h"
#include "PythonShop.h"
#include "PythonExchange.h"
#include "PythonSafeBox.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"

//////////////////////////////////////////////////////////////////////////
// SafeBox

bool CPythonNetworkStream::SendSafeBoxMoneyPacket(uint8_t byState, uint32_t dwMoney)
{
	assert(!"CPythonNetworkStream::SendSafeBoxMoneyPacket");
	return false;

//	TPacketCGSafeboxMoney kSafeboxMoney;
//	kSafeboxMoney.bHeader = HEADER_CG_SAFEBOX_MONEY;
//	kSafeboxMoney.bState = byState;
//	kSafeboxMoney.dwMoney = dwMoney;
//	if (!Send(sizeof(kSafeboxMoney), &kSafeboxMoney))
//		return false;
//
//	return true;
}

bool CPythonNetworkStream::SendSafeBoxCheckinPacket(TItemPos InventoryPos, uint8_t bySafeBoxPos)
{
	__PlayInventoryItemDropSound(InventoryPos);

	TPacketCGSafeboxCheckin kSafeboxCheckin;
	kSafeboxCheckin.bHeader = HEADER_CG_SAFEBOX_CHECKIN;
	kSafeboxCheckin.ItemPos = InventoryPos;
	kSafeboxCheckin.bSafePos = bySafeBoxPos;
	if (!Send(sizeof(kSafeboxCheckin), &kSafeboxCheckin))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSafeBoxCheckoutPacket(uint8_t bySafeBoxPos, TItemPos InventoryPos)
{
	__PlaySafeBoxItemDropSound(bySafeBoxPos);

	TPacketCGSafeboxCheckout kSafeboxCheckout;
	kSafeboxCheckout.bHeader = HEADER_CG_SAFEBOX_CHECKOUT;
	kSafeboxCheckout.bSafePos = bySafeBoxPos;
	kSafeboxCheckout.ItemPos = InventoryPos;
	if (!Send(sizeof(kSafeboxCheckout), &kSafeboxCheckout))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSafeBoxItemMovePacket(uint8_t bySourcePos, uint8_t byTargetPos, uint8_t byCount)
{
	__PlaySafeBoxItemDropSound(bySourcePos);

	TPacketCGItemMove kItemMove;
	kItemMove.header = HEADER_CG_SAFEBOX_ITEM_MOVE;
	kItemMove.Cell = TItemPos(INVENTORY, bySourcePos);
	kItemMove.count = byCount;
	kItemMove.CellTo = TItemPos(INVENTORY, byTargetPos);
	if (!Send(sizeof(kItemMove), &kItemMove))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxSetPacket()
{
	TPacketGCItemSet kItemSet;
	if (!Recv(sizeof(kItemSet), &kItemSet))
		return false;

	TItemData kItemData;
	kItemData.vnum	= kItemSet.vnum;
	kItemData.count = kItemSet.count;
	kItemData.flags = kItemSet.flags;
	kItemData.anti_flags = kItemSet.anti_flags;
	for (int32_t isocket=0; isocket< ITEM::SOCKET_MAX_NUM; ++isocket)
		kItemData.alSockets[isocket] = kItemSet.alSockets[isocket];
	for (int32_t iattr=0; iattr< ITEM::ATTRIBUTE_MAX_NUM; ++iattr)
		kItemData.aAttr[iattr] = kItemSet.aAttr[iattr];

	CPythonSafeBox::GetInstance()->SetItemData(kItemSet.Cell.cell, kItemData);

	__RefreshSafeboxWindow();

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxDelPacket()
{
	TPacketGCSafeboxItemDel kItemDel;
	if (!Recv(sizeof(kItemDel), &kItemDel))
		return false;

	CPythonSafeBox::GetInstance()->DelItemData(kItemDel.pos);

	__RefreshSafeboxWindow();

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxWrongPasswordPacket()
{
	TPacketGCSafeboxWrongPassword kSafeboxWrongPassword;

	if (!Recv(sizeof(kSafeboxWrongPassword), &kSafeboxWrongPassword))
		return false;

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnSafeBoxError");

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxSizePacket()
{
	TPacketGCSafeboxSize kSafeBoxSize;
	if (!Recv(sizeof(kSafeBoxSize), &kSafeBoxSize))
		return false;

	CPythonSafeBox::GetInstance()->OpenSafeBox(kSafeBoxSize.bSize);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenSafeboxWindow",  kSafeBoxSize.bSize);

	return true;
}

// SafeBox
//////////////////////////////////////////////////////////////////////////

// Item
// Recieve
bool CPythonNetworkStream::RecvItemSetPacket()
{
	TPacketGCItemSet packet;

	if (!Recv(sizeof(TPacketGCItemSet), &packet))
		return false;

	TItemData kItemData;
	kItemData.vnum	= packet.vnum;
	kItemData.count	= packet.count;
	kItemData.flags = packet.flags;
	kItemData.anti_flags = packet.anti_flags;
	for (int32_t i=0; i< ITEM::SOCKET_MAX_NUM; ++i)
		kItemData.alSockets[i]= packet.alSockets[i];
	for (int32_t j=0; j< ITEM::ATTRIBUTE_MAX_NUM; ++j)
		kItemData.aAttr[j]= packet.aAttr[j];

	auto rkPlayer=CPythonPlayer::GetInstance();
	
	rkPlayer->SetItemData(packet.Cell, kItemData);
	
	__RefreshInventoryWindow();
	return true;
}

bool CPythonNetworkStream::RecvItemDelPacket()
{
	TPacketGCItemDel packet;

	if (!Recv(sizeof(TPacketGCItemDel), &packet))
		return false;

	TItemData kItemData {};
	memset(&kItemData, 0, sizeof(TItemData));

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->SetItemData(packet.Cell, kItemData);

	__RefreshInventoryWindow();
	return true;
}


bool CPythonNetworkStream::RecvItemUsePacket()
{
	TPacketGCItemUse packet;

	if (!Recv(sizeof(TPacketGCItemUse), &packet))
		return false;

	__RefreshInventoryWindow();
	return true;
}

bool CPythonNetworkStream::RecvItemUpdatePacket()
{
	TPacketGCItemUpdate packet_item_update;

	if (!Recv(sizeof(TPacketGCItemUpdate), &packet_item_update))
		return false;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->SetItemCount(packet_item_update.Cell, packet_item_update.count);
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		rkPlayer->SetItemMetinSocket(packet_item_update.Cell, i, packet_item_update.alSockets[i]);
	for (int32_t j = 0; j < ITEM::ATTRIBUTE_MAX_NUM; ++j)
		rkPlayer->SetItemAttribute(packet_item_update.Cell, j, packet_item_update.aAttr[j].bType, packet_item_update.aAttr[j].sValue);

	__RefreshInventoryWindow();
	return true;
}

bool CPythonNetworkStream::RecvItemGroundAddPacket()
{
	TPacketGCItemGroundAdd packet_item_ground_add;

	if (!Recv(sizeof(TPacketGCItemGroundAdd), &packet_item_ground_add))
		return false;

	__GlobalPositionToLocalPosition(packet_item_ground_add.x, packet_item_ground_add.y);

	CPythonItem::GetInstance()->CreateItem(packet_item_ground_add.dwVID, 
									   packet_item_ground_add.dwVnum,
									   packet_item_ground_add.x,
									   packet_item_ground_add.y,
									   packet_item_ground_add.z);
	return true;
}


bool CPythonNetworkStream::RecvItemOwnership()
{
	TPacketGCItemOwnership p;

	if (!Recv(sizeof(TPacketGCItemOwnership), &p))
		return false;

	CPythonItem::GetInstance()->SetOwnership(p.dwVID, p.szName);
	return true;
}

bool CPythonNetworkStream::RecvItemGroundDelPacket()
{
	TPacketGCItemGroundDel	packet_item_ground_del;

	if (!Recv(sizeof(TPacketGCItemGroundDel), &packet_item_ground_del))
		return false;

	CPythonItem::GetInstance()->DeleteItem(packet_item_ground_del.dwVID);
	return true;
}

bool CPythonNetworkStream::RecvQuickSlotAddPacket()
{
	TPacketGCQuickSlotAdd packet_quick_slot_add;

	if (!Recv(sizeof(TPacketGCQuickSlotAdd), &packet_quick_slot_add))
		return false;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->AddQuickSlot(packet_quick_slot_add.pos, packet_quick_slot_add.slot.type, packet_quick_slot_add.slot.pos);

	__RefreshInventoryWindow();

	return true;
}

bool CPythonNetworkStream::RecvQuickSlotDelPacket()
{
	TPacketGCQuickSlotDel packet_quick_slot_del;

	if (!Recv(sizeof(TPacketGCQuickSlotDel), &packet_quick_slot_del))
		return false;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->DeleteQuickSlot(packet_quick_slot_del.pos);

	__RefreshInventoryWindow();

	return true;
}

bool CPythonNetworkStream::RecvQuickSlotMovePacket()
{
	TPacketGCQuickSlotSwap packet_quick_slot_swap;

	if (!Recv(sizeof(TPacketGCQuickSlotSwap), &packet_quick_slot_swap))
		return false;

	auto rkPlayer=CPythonPlayer::GetInstance();
	rkPlayer->MoveQuickSlot(packet_quick_slot_swap.pos, packet_quick_slot_swap.pos_to);

	__RefreshInventoryWindow();

	return true;
}



bool CPythonNetworkStream::SendShopEndPacket()
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGShop packet_shop;
	packet_shop.header = HEADER_CG_SHOP;
	packet_shop.subheader = SHOP_SUBHEADER_CG_END;

	if (!Send(sizeof(packet_shop), &packet_shop))
	{
		TraceLog("SendShopEndPacket Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendShopBuyPacket(uint8_t bPos)
{
	if (!__CanActMainInstance())
		return true;
	
	TPacketCGShop PacketShop;
	PacketShop.header = HEADER_CG_SHOP;
	PacketShop.subheader = SHOP_SUBHEADER_CG_BUY;

	if (!Send(sizeof(TPacketCGShop), &PacketShop))
	{
		TraceLog("SendShopBuyPacket Error\n");
		return false;
	}

	uint8_t bCount=1;
	if (!Send(sizeof(uint8_t), &bCount))
	{
		TraceLog("SendShopBuyPacket Error\n");
		return false;
	}

	if (!Send(sizeof(uint8_t), &bPos))
	{
		TraceLog("SendShopBuyPacket Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendShopSellPacket(uint8_t bySlot)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGShop PacketShop;
	PacketShop.header = HEADER_CG_SHOP;
	PacketShop.subheader = SHOP_SUBHEADER_CG_SELL;

	if (!Send(sizeof(TPacketCGShop), &PacketShop))
	{
		TraceLog("SendShopSellPacket Error\n");
		return false;
	}
	if (!Send(sizeof(uint8_t), &bySlot))
	{
		TraceLog("SendShopAddSellPacket Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendShopSellPacketNew(uint8_t bySlot, uint8_t byCount)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGShop PacketShop;
	PacketShop.header = HEADER_CG_SHOP;
	PacketShop.subheader = SHOP_SUBHEADER_CG_SELL2;

	if (!Send(sizeof(TPacketCGShop), &PacketShop))
	{
		TraceLog("SendShopSellPacket Error\n");
		return false;
	}
	if (!Send(sizeof(uint8_t), &bySlot))
	{
		TraceLog("SendShopAddSellPacket Error\n");
		return false;
	}
	if (!Send(sizeof(uint8_t), &byCount))
	{
		TraceLog("SendShopAddSellPacket Error\n");
		return false;
	}

	TraceLog(" SendShopSellPacketNew(bySlot={}, byCount={})\n", bySlot, byCount);

	return true;
}

// Send
bool CPythonNetworkStream::SendItemUsePacket(TItemPos pos)
{
	if (!__CanActMainInstance())
		return true;

	if (__IsEquipItemInSlot(pos))
	{
		if (CPythonExchange::GetInstance()->isTrading())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", "CANNOT_EQUIP_EXCHANGE");
			return true;
		}

		if (CPythonShop::GetInstance()->IsOpen())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage","CANNOT_EQUIP_SHOP");
			return true;
		}

		if (__IsPlayerAttacking())
			return true;
	}

	__PlayInventoryItemUseSound(pos);

	TPacketCGItemUse itemUsePacket;
	itemUsePacket.header = HEADER_CG_ITEM_USE;
	itemUsePacket.Cell = pos;

	if (!Send(sizeof(TPacketCGItemUse), &itemUsePacket))
	{
		TraceLog("SendItemUsePacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendItemUseToItemPacket(TItemPos source_pos, TItemPos target_pos)
{
	if (!__CanActMainInstance())
		return true;	

	TPacketCGItemUseToItem itemUseToItemPacket;
	itemUseToItemPacket.header = HEADER_CG_ITEM_USE_TO_ITEM;
	itemUseToItemPacket.Cell = source_pos;
	itemUseToItemPacket.TargetCell = target_pos;

	if (!Send(sizeof(TPacketCGItemUseToItem), &itemUseToItemPacket))
	{
		TraceLog("SendItemUseToItemPacket Error");
		return false;
	}

	ConsoleLog(" << SendItemUseToItemPacket(src={}, dst={})\n", source_pos.cell, target_pos.cell);
	return true;
}

bool CPythonNetworkStream::SendItemDropPacket(TItemPos pos, uint32_t elk)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGItemDrop itemDropPacket;
	itemDropPacket.header = HEADER_CG_ITEM_DROP;
	itemDropPacket.Cell = pos;
	itemDropPacket.gold = elk;

	if (!Send(sizeof(TPacketCGItemDrop), &itemDropPacket))
	{
		TraceLog("SendItemDropPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendItemDropPacketNew(TItemPos pos, uint32_t elk, uint32_t count)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGItemDrop2 itemDropPacket;
	itemDropPacket.header = HEADER_CG_ITEM_DROP2;
	itemDropPacket.Cell = pos;
	itemDropPacket.gold = elk;
	itemDropPacket.count = count;

	if (!Send(sizeof(itemDropPacket), &itemDropPacket))
	{
		TraceLog("SendItemDropPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::__IsEquipItemInSlot(TItemPos uSlotPos)
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	return rkPlayer->IsEquipItemInSlot(uSlotPos);
}

void CPythonNetworkStream::__PlayInventoryItemUseSound(TItemPos uSlotPos)
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	uint32_t dwItemID=rkPlayer->GetItemIndex(uSlotPos);

	auto rkItem=CPythonItem::GetInstance();
	rkItem->PlayUseSound(dwItemID);
}

void CPythonNetworkStream::__PlayInventoryItemDropSound(TItemPos uSlotPos)
{
	auto rkPlayer=CPythonPlayer::GetInstance();
	uint32_t dwItemID=rkPlayer->GetItemIndex(uSlotPos);

	auto rkItem=CPythonItem::GetInstance();
	rkItem->PlayDropSound(dwItemID);
}

//void CPythonNetworkStream::__PlayShopItemDropSound(UINT uSlotPos)
//{
//	uint32_t dwItemID;
//	CPythonShop& rkShop=CPythonShop::Instance();
//	if (!rkShop.GetSlotItemID(uSlotPos, &dwItemID))
//		return;
//	
//	CPythonItem& rkItem=CPythonItem::Instance();
//	rkItem.PlayDropSound(dwItemID);
//}

void CPythonNetworkStream::__PlaySafeBoxItemDropSound(UINT uSlotPos)
{
	uint32_t dwItemID;
	auto rkSafeBox=CPythonSafeBox::GetInstance();
	if (!rkSafeBox->GetSlotItemID(uSlotPos, &dwItemID))
		return;

	auto rkItem=CPythonItem::GetInstance();
	rkItem->PlayDropSound(dwItemID);
}

bool CPythonNetworkStream::SendItemMovePacket(TItemPos pos, TItemPos change_pos, uint8_t num)
{	
	if (!__CanActMainInstance())
		return true;
	
	if (__IsEquipItemInSlot(pos))
	{
		if (CPythonExchange::GetInstance()->isTrading())
		{
			if (pos.IsEquipPosition() || change_pos.IsEquipPosition())
			{
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage",  "CANNOT_EQUIP_EXCHANGE");
				return true;
			}
		}

		if (CPythonShop::GetInstance()->IsOpen())
		{
			if (pos.IsEquipPosition() || change_pos.IsEquipPosition())
			{
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage",  "CANNOT_EQUIP_SHOP");
				return true;
			}
		}

		if (__IsPlayerAttacking())
			return true;
	}

	__PlayInventoryItemDropSound(pos);

	TPacketCGItemMove	itemMovePacket;
	itemMovePacket.header = HEADER_CG_ITEM_MOVE;
	itemMovePacket.Cell = pos;
	itemMovePacket.CellTo = change_pos;
	itemMovePacket.count = num;

	if (!Send(sizeof(TPacketCGItemMove), &itemMovePacket))
	{
		TraceLog("SendItemMovePacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendItemPickUpPacket(uint32_t vid)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGItemPickup	itemPickUpPacket;
	itemPickUpPacket.header = HEADER_CG_ITEM_PICKUP;
	itemPickUpPacket.vid = vid;

	if (!Send(sizeof(TPacketCGItemPickup), &itemPickUpPacket))
	{
		TraceLog("SendItemPickUpPacket Error");
		return false;
	}

	return true;
}


bool CPythonNetworkStream::SendQuickSlotAddPacket(uint8_t wpos, uint8_t type, uint8_t pos)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGQuickslotAdd quickSlotAddPacket;

	quickSlotAddPacket.header		= HEADER_CG_QUICKSLOT_ADD;
	quickSlotAddPacket.pos			= wpos;
	quickSlotAddPacket.slot.type	= type;
	quickSlotAddPacket.slot.pos		= pos;

	if (!Send(sizeof(TPacketCGQuickslotAdd), &quickSlotAddPacket))
	{
		TraceLog("SendQuickSlotAddPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuickSlotDelPacket(uint8_t pos)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGQuickslotDel quickSlotDelPacket;

	quickSlotDelPacket.header = HEADER_CG_QUICKSLOT_DEL;
	quickSlotDelPacket.pos = pos;

	if (!Send(sizeof(TPacketCGQuickslotDel), &quickSlotDelPacket))
	{
		TraceLog("SendQuickSlotDelPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuickSlotMovePacket(uint8_t pos, uint8_t change_pos)
{
	if (!__CanActMainInstance())
		return true;

	TPacketCGQuickslotSwap quickSlotSwapPacket;

	quickSlotSwapPacket.header = HEADER_CG_QUICKSLOT_SWAP;
	quickSlotSwapPacket.pos = pos;
	quickSlotSwapPacket.change_pos = change_pos;

	if (!Send(sizeof(TPacketCGQuickslotSwap), &quickSlotSwapPacket))
	{
		TraceLog("SendQuickSlotSwapPacket Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvSpecialEffect()
{
	TPacketGCSpecialEffect kSpecialEffect;
	if (!Recv(sizeof(kSpecialEffect), &kSpecialEffect))
		return false;

	uint32_t effect = -1;
	bool bPlayPotionSound = false;	//포션을 먹을 경우는 포션 사운드를 출력하자.!!
	bool bAttachEffect = true;		//캐리터에 붙는 어태치 이펙트와 일반 이펙트 구분.!!
	switch (kSpecialEffect.type)
	{
		case SE_HPUP_RED:
			effect = CInstanceBase::EFFECT_HPUP_RED;
			bPlayPotionSound = true;
			break;
		case SE_SPUP_BLUE:
			effect = CInstanceBase::EFFECT_SPUP_BLUE;
			bPlayPotionSound = true;
			break;
		case SE_SPEEDUP_GREEN:
			effect = CInstanceBase::EFFECT_SPEEDUP_GREEN;
			bPlayPotionSound = true;
			break;
		case SE_DXUP_PURPLE:
			effect = CInstanceBase::EFFECT_DXUP_PURPLE;
			bPlayPotionSound = true;
			break;
		case SE_CRITICAL:
			effect = CInstanceBase::EFFECT_CRITICAL;
			break;
		case SE_PENETRATE:
			effect = CInstanceBase::EFFECT_PENETRATE;
			break;
		case SE_BLOCK:
			effect = CInstanceBase::EFFECT_BLOCK;
			break;
		case SE_DODGE:
			effect = CInstanceBase::EFFECT_DODGE;
			break;
		case SE_CHINA_FIREWORK:
			effect = CInstanceBase::EFFECT_FIRECRACKER;
			bAttachEffect = false;
			break;
		case SE_SPIN_TOP:
			effect = CInstanceBase::EFFECT_SPIN_TOP;
			bAttachEffect = false;
			break;
		case SE_SUCCESS :
			effect = CInstanceBase::EFFECT_SUCCESS ;
			bAttachEffect = false ;
			break ;
		case SE_FAIL :
			effect = CInstanceBase::EFFECT_FAIL ;
			break ;
		case SE_FR_SUCCESS:
			effect = CInstanceBase::EFFECT_FR_SUCCESS;
			bAttachEffect = false ;
			break;
		case SE_LEVELUP_ON_14_FOR_GERMANY:	//레벨업 14일때 ( 독일전용 )
			effect = CInstanceBase::EFFECT_LEVELUP_ON_14_FOR_GERMANY;
			bAttachEffect = false ;
			break;
		case SE_LEVELUP_UNDER_15_FOR_GERMANY: //레벨업 15일때 ( 독일전용 )
			effect = CInstanceBase::EFFECT_LEVELUP_UNDER_15_FOR_GERMANY;
			bAttachEffect = false ;
			break;
		case SE_PERCENT_DAMAGE1:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE1;
			break;
		case SE_PERCENT_DAMAGE2:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE2;
			break;
		case SE_PERCENT_DAMAGE3:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE3;
			break;
		case SE_AUTO_HPUP:
			effect = CInstanceBase::EFFECT_AUTO_HPUP;
			break;
		case SE_AUTO_SPUP:
			effect = CInstanceBase::EFFECT_AUTO_SPUP;
			break;
		case SE_EQUIP_RAMADAN_RING:
			effect = CInstanceBase::EFFECT_RAMADAN_RING_EQUIP;
			break;
		case SE_EQUIP_HALLOWEEN_CANDY:
			effect = CInstanceBase::EFFECT_HALLOWEEN_CANDY_EQUIP;
			break;
		case SE_EQUIP_HAPPINESS_RING:
 			effect = CInstanceBase::EFFECT_HAPPINESS_RING_EQUIP;
			break;
		case SE_EQUIP_LOVE_PENDANT:
			effect = CInstanceBase::EFFECT_LOVE_PENDANT_EQUIP;
			break;

		
		default:
			TraceLog("{} 는 없는 스페셜 이펙트 번호입니다.TPacketGCSpecialEffect",kSpecialEffect.type);
			break;
	}

	if (bPlayPotionSound)
	{		
		auto rkPlayer=CPythonPlayer::GetInstance();
		if(rkPlayer->IsMainCharacterIndex(kSpecialEffect.vid))
		{
			auto rkItem=CPythonItem::GetInstance();
			rkItem->PlayUsePotionSound();
		}
	}

	if (-1 != effect)
	{
		CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(kSpecialEffect.vid);
		if (pInstance)
		{
			if(bAttachEffect)
				pInstance->AttachSpecialEffect(effect);
			else
				pInstance->CreateSpecialEffect(effect);
		}
	}

	return true;
}


bool CPythonNetworkStream::RecvSpecificEffect()
{
	TPacketGCSpecificEffect kSpecificEffect;
	if (!Recv(sizeof(kSpecificEffect), &kSpecificEffect))
		return false;

	CInstanceBase * pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(kSpecificEffect.vid);
	//EFFECT_TEMP
	if (pInstance)
	{
		CInstanceBase::RegisterEffect(CInstanceBase::EFFECT_TEMP, "", kSpecificEffect.effect_file, false);
		pInstance->AttachSpecialEffect(CInstanceBase::EFFECT_TEMP);
	}

	return true;
}

bool CPythonNetworkStream::RecvDragonSoulRefine()
{
	TPacketGCDragonSoulRefine kDragonSoul;

	if (!Recv(sizeof(kDragonSoul), &kDragonSoul))
		return false;
	
	
	switch (kDragonSoul.bSubType)
	{
	case DS_SUB_HEADER_OPEN:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_Open");
		break;
	case DS_SUB_HEADER_REFINE_FAIL:
	case DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE:
	case DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL:
	case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY:
	case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL:
	case DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineFail",
			kDragonSoul.bSubType, kDragonSoul.Pos.window_type, kDragonSoul.Pos.cell);
		break;
	case DS_SUB_HEADER_REFINE_SUCCEED:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineSucceed", 
				kDragonSoul.Pos.window_type, kDragonSoul.Pos.cell);
		break;
	}

	return true;
}
