#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"
#include "log.h"
#include "skill.h"
#include "unique_item.h"
#include "marriage.h"
#include "item_addon.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"
#include <Common/VnumHelper.h>

CItem::CItem(uint32_t dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0), m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pDestroyEvent(nullptr), m_pUniqueExpireEvent(nullptr), m_pTimerBasedOnWearExpireEvent(nullptr), m_pRealTimeExpireEvent(nullptr),
	m_pExpireEvent(nullptr),
   	m_pAccessorySocketExpireEvent(nullptr), m_pOwnershipEvent(nullptr), m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum (0)
{
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));
}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = nullptr;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
	m_pProto = nullptr;
	m_bExchanging = false;
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));

	m_pDestroyEvent = nullptr;
	m_pOwnershipEvent = nullptr;
	m_dwOwnershipPID = 0;
	m_pUniqueExpireEvent = nullptr;
	m_pTimerBasedOnWearExpireEvent = nullptr;
	m_pRealTimeExpireEvent = nullptr;

	m_pAccessorySocketExpireEvent = nullptr;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;
}

void CItem::Destroy()
{
	event_cancel(&m_pDestroyEvent);
	event_cancel(&m_pOwnershipEvent);
	event_cancel(&m_pUniqueExpireEvent);
	event_cancel(&m_pTimerBasedOnWearExpireEvent);
	event_cancel(&m_pRealTimeExpireEvent);
	event_cancel(&m_pAccessorySocketExpireEvent);

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("item_destroy_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pItem = info->item;

	if (pItem->GetOwner())
		SysLog("item_destroy_event: Owner exist. (item {} owner {})", pItem->GetName(), pItem->GetOwner()->GetName());

	pItem->SetDestroyEvent(nullptr);
	M2_DESTROY_ITEM(pItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pEvent)
{
	m_pDestroyEvent = pEvent;
}

void CItem::StartDestroyEvent(int32_t iSec)
{
	if (m_pDestroyEvent)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION& c_pos = GetXYZ();

	struct packet_item_ground_add pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_ADD;
	pack.x		= c_pos.x;
	pack.y		= c_pos.y;
	pack.z		= c_pos.z;
	pack.dwVnum		= GetVnum();
	pack.dwVID		= m_dwVID;
	
	d->Packet(&pack, sizeof(pack));

	if (m_pOwnershipEvent != nullptr)
	{
		item_event_info * info = dynamic_cast<item_event_info *>(m_pOwnershipEvent->info);

		if (info == nullptr)
		{
			SysLog("CItem::EncodeInsertPacket> <Factor> Null pointer");
			return;
		}

		TPacketGCItemOwnership p;

		p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
		p.dwVID = m_dwVID;
		strlcpy(p.szName, info->szOwnerName, sizeof(p.szName));

		d->Packet(&p, sizeof(TPacketGCItemOwnership));
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	struct packet_item_ground_del pack;

	pack.bHeader	= HEADER_GC_ITEM_GROUND_DEL;
	pack.dwVID		= m_dwVID;

	d->Packet(&pack, sizeof(pack));
	TraceLog("Item::EncodeRemovePacket {} to {}", GetName(), ((LPCHARACTER) ent)->GetName());
}

void CItem::SetProto(const TItemTable * table)
{
	assert(table != nullptr);
	m_pProto = table;
	SetFlag(m_pProto->dwFlags);
}

void CItem::UsePacketEncode(LPCHARACTER ch, LPCHARACTER victim, struct packet_item_use* packet)
{
	if (!GetVnum())
		return;

	packet->header 	= HEADER_GC_ITEM_USE;
	packet->ch_vid 	= ch->GetVID();
	packet->victim_vid 	= victim->GetVID();
	packet->Cell = TItemPos(GetWindow(), m_wCell);
	packet->vnum	= GetVnum();
}

void CItem::RemoveFlag(int32_t bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(int32_t bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;

	TPacketGCItemUpdate pack;

	pack.header = HEADER_GC_ITEM_UPDATE;
	pack.Cell = TItemPos(GetWindow(), m_wCell);
	pack.count	= m_dwCount;

	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		pack.alSockets[i] = m_alSockets[i];

	memcpy(pack.aAttr, GetAttributes(), sizeof(pack.aAttr));

	TraceLog("UpdatePacket {} -> {}", GetName(), m_pOwner->GetName());
	m_pOwner->GetDesc()->Packet(&pack, sizeof(pack));
}

uint32_t CItem::GetCount()
{
	if (GetType() == ITEM::TYPE_ELK) return MIN(m_dwCount, INT_MAX);
	else
	{
		return MIN(m_dwCount, 200);
	}
}

bool CItem::SetCount(uint32_t count)
{
	if (GetType() == ITEM::TYPE_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
	else
	{
		m_dwCount = MIN(count, ITEM::MAX_COUNT);
	}

	if (count == 0 && m_pOwner)
	{
		if (GetSubType() == ITEM::USE_ABILITY_UP || GetSubType() == ITEM::USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			uint16_t wCell = GetCell();

			RemoveFromCharacter();

			if (!IsDragonSoul())
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, 255);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		else
		{
			if (!IsDragonSoul())
			{
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, 255);
			}
			M2_DESTROY_ITEM(RemoveFromCharacter());
		}

		return false;
	}

	UpdatePacket();

	Save();
	return true;
}

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		SysLog("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (m_bEquipped)
	{
		Unequip();
		
		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX)
		{
			if (IsDragonSoul())
			{
				if (m_wCell >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					SysLog("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				}
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
			else
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (!cell.IsDefaultInventoryPosition() && !cell.IsBeltInventoryPosition())
				{
					SysLog("CItem::RemoveFromCharacter: Invalid Item Position");
				}
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = nullptr;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell)
{
	assert(GetSectree() == nullptr);
	assert(m_pOwner == nullptr);
	uint16_t pos = Cell.cell;
	uint8_t window_type = Cell.window_type;
	
	if (INVENTORY == window_type)
	{
		if (m_wCell >= INVENTORY_MAX_NUM && BELT_INVENTORY_SLOT_START > m_wCell)
		{
			SysLog("CItem::AddToCharacter: cell overflow: {} to {} cell {}", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			SysLog("CItem::AddToCharacter: cell overflow: {} to {} cell {}", m_pProto->szName, ch->GetName(), m_wCell);
			return false;
		}
	}

	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();

	event_cancel(&m_pDestroyEvent);

	ch->SetItem(TItemPos(window_type, pos), this);
	m_pOwner = ch;

	Save();
	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(nullptr);
		
		GetSectree()->RemoveEntity(this);
		
		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(int32_t lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck)
{
	if (0 == lMapIndex)
	{
		SysLog("wrong map index argument: {}", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		SysLog("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		SysLog("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		SysLog("cannot find sectree by {}x{}", pos.x, pos.y);
		return false;
	}

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;

	int32_t iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());

	if (iDist > 300)
		return false;

	return true;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
	switch (ch->GetJob())
	{
		case JOB_WARRIOR:
			if (GetAntiFlag() & ITEM::ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (GetAntiFlag() & ITEM::ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (GetAntiFlag() & ITEM::ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (GetAntiFlag() & ITEM::ANTIFLAG_SURA)
				return false;
			break;
	}

	return true;
}

int32_t CItem::FindEquipCell(LPCHARACTER ch, int32_t iCandidateCell)
{
	if ((0 == GetWearFlag() || ITEM::TYPE_TOTEM == GetType()) && ITEM::TYPE_COSTUME != GetType() && 
		ITEM::TYPE_DS != GetType() && ITEM::TYPE_SPECIAL_DS != GetType() && ITEM::TYPE_RING != GetType() && ITEM::TYPE_BELT != GetType())
		return -1;

	if (GetType() == ITEM::TYPE_DS || GetType() == ITEM::TYPE_SPECIAL_DS)
	{
		if (iCandidateCell < 0)
		{
			return WEAR_MAX_NUM + GetSubType();
		}
		else
		{
			for (int32_t i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
			{
				if (WEAR_MAX_NUM + i * ITEM::DS_SLOT_MAX + GetSubType() == iCandidateCell)
				{
					return iCandidateCell;
				}
			}
			return -1;
		}
	}
	else if (GetType() == ITEM::TYPE_COSTUME)
	{
		if (GetSubType() == ITEM::COSTUME_BODY)
			return WEAR_COSTUME_BODY;
		else if (GetSubType() == ITEM::COSTUME_HAIR)
			return WEAR_COSTUME_HAIR;
	}
	else if (GetType() == ITEM::TYPE_RING)
	{
		if (ch->GetWear(WEAR_RING1))
			return WEAR_RING2;
		else
			return WEAR_RING1;
	}
	else if (GetType() == ITEM::TYPE_BELT)
		return WEAR_BELT;
	else if (GetWearFlag() & ITEM::WEARABLE_BODY)
		return WEAR_BODY;
	else if (GetWearFlag() & ITEM::WEARABLE_HEAD)
		return WEAR_HEAD;
	else if (GetWearFlag() & ITEM::WEARABLE_FOOTS)
		return WEAR_FOOTS;
	else if (GetWearFlag() & ITEM::WEARABLE_WRIST)
		return WEAR_WRIST;
	else if (GetWearFlag() & ITEM::WEARABLE_WEAPON)
		return WEAR_WEAPON;
	else if (GetWearFlag() & ITEM::WEARABLE_SHIELD)
		return WEAR_SHIELD;
	else if (GetWearFlag() & ITEM::WEARABLE_NECK)
		return WEAR_NECK;
	else if (GetWearFlag() & ITEM::WEARABLE_EAR)
		return WEAR_EAR;
	else if (GetWearFlag() & ITEM::WEARABLE_ARROW)
		return WEAR_ARROW;
	else if (GetWearFlag() & ITEM::WEARABLE_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE1))
			return WEAR_UNIQUE2;
		else
			return WEAR_UNIQUE1;		
	}

	else if (GetWearFlag() & ITEM::WEARABLE_ABILITY)
	{
		if (!ch->GetWear(WEAR_ABILITY1))
		{
			return WEAR_ABILITY1;
		}
		else if (!ch->GetWear(WEAR_ABILITY2))
		{
			return WEAR_ABILITY2;
		}
		else if (!ch->GetWear(WEAR_ABILITY3))
		{
			return WEAR_ABILITY3;
		}
		else if (!ch->GetWear(WEAR_ABILITY4))
		{
			return WEAR_ABILITY4;
		}
		else if (!ch->GetWear(WEAR_ABILITY5))
		{
			return WEAR_ABILITY5;
		}
		else if (!ch->GetWear(WEAR_ABILITY6))
		{
			return WEAR_ABILITY6;
		}
		else if (!ch->GetWear(WEAR_ABILITY7))
		{
			return WEAR_ABILITY7;
		}
		else if (!ch->GetWear(WEAR_ABILITY8))
		{
			return WEAR_ABILITY8;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

void CItem::ModifyPoints(bool bAdd)
{
	int32_t accessoryGrade;

	if (!IsAccessoryForSocket())
	{
		if (m_pProto->bType == ITEM::TYPE_WEAPON || m_pProto->bType == ITEM::TYPE_ARMOR)
		{
			for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
			{
				uint32_t dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				TItemTable* p = ITEM_MANAGER::GetInstance()->GetTable(dwVnum);

				if (!p)
				{
					SysLog("cannot find table by vnum {}", dwVnum);
					continue;
				}

				if (ITEM::TYPE_METIN == p->bType)
				{
					for (int32_t i = 0; i < ITEM::APPLY_MAX_NUM; ++i)
					{
						if (p->aApplies[i].bType == ITEM::APPLY_NONE)
							continue;

						if (p->aApplies[i].bType == ITEM::APPLY_SKILL)
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : p->aApplies[i].lValue ^ 0x00800000);
						else
							m_pOwner->ApplyPoint(p->aApplies[i].bType, bAdd ? p->aApplies[i].lValue : -p->aApplies[i].lValue);
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGrade(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
	}

	for (int32_t i = 0; i < ITEM::APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == ITEM::APPLY_NONE)
			continue;

		int32_t value = m_pProto->aApplies[i].lValue;

		if (m_pProto->aApplies[i].bType == ITEM::APPLY_SKILL)
		{
			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : value ^ 0x00800000);
		}
		else
		{
			if (0 != accessoryGrade)
				value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

			m_pOwner->ApplyPoint(m_pProto->aApplies[i].bType, bAdd ? value : -value);
		}
	}
	if (CItemVnumHelper::IsRamadanMoonRing(GetVnum()) || CItemVnumHelper::IsHalloweenCandy(GetVnum())
		|| CItemVnumHelper::IsHappinessRing(GetVnum()) || CItemVnumHelper::IsLovePendant(GetVnum()))
	{
		// Do not anything.
	}
	else
	{
		for (int32_t i = 0; i < ITEM::ATTRIBUTE_MAX_NUM; ++i)
		{
			if (GetAttributeType(i))
			{
				const TPlayerItemAttribute& ia = GetAttribute(i);

				if (ia.bType == ITEM::APPLY_SKILL)
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : ia.sValue ^ 0x00800000);
				else
					m_pOwner->ApplyPoint(ia.bType, bAdd ? ia.sValue : -ia.sValue);
			}
		}
	}

	switch (m_pProto->bType)
	{
		case ITEM::TYPE_PICK:
		case ITEM::TYPE_ROD:
			{
				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, m_pOwner->GetOriginalPart(PART_WEAPON));
				}
			}
			break;

		case ITEM::TYPE_WEAPON:
			{
				if (bAdd)
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == INVENTORY_MAX_NUM + WEAR_WEAPON)
						m_pOwner->SetPart(PART_WEAPON, m_pOwner->GetOriginalPart(PART_WEAPON));
				}
			}
			break;

		case ITEM::TYPE_ARMOR:
			{
				if (0 != m_pOwner->GetWear(WEAR_COSTUME_BODY))
					break;

				if (GetSubType() == ITEM::ARMOR_BODY || GetSubType() == ITEM::ARMOR_HEAD || GetSubType() == ITEM::ARMOR_FOOTS || GetSubType() == ITEM::ARMOR_SHIELD)
				{
					if (bAdd)
					{
						if (GetProto()->bSubType == ITEM::ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, GetVnum());
					}
					else
					{
						if (GetProto()->bSubType == ITEM::ARMOR_BODY)
							m_pOwner->SetPart(PART_MAIN, m_pOwner->GetOriginalPart(PART_MAIN));
					}
				}
			}
			break;

		case ITEM::TYPE_COSTUME:
			{
				uint32_t toSetValue = this->GetVnum();
				ERaceParts toSetPart = PART_MAX_NUM;

				if (GetSubType() == ITEM::COSTUME_BODY)
				{
					toSetPart = PART_MAIN;

					if (!bAdd)
					{
						const CItem* pArmor = m_pOwner->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetVnum() : m_pOwner->GetOriginalPart(PART_MAIN);						
					}
					
				}

				else if (GetSubType() == ITEM::COSTUME_HAIR)
				{
					toSetPart = PART_HAIR;

					toSetValue = (bAdd) ? this->GetValue(3) : 0;
				}

				if (PART_MAX_NUM != toSetPart)
				{
					m_pOwner->SetPart((uint8_t)toSetPart, toSetValue);
					m_pOwner->UpdatePacket();
				}
			}
			break;
		case ITEM::TYPE_UNIQUE:
			{
				if (0 != GetSIGVnum())
				{
					const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::GetInstance()->GetSpecialItemGroup(GetSIGVnum());
					if (!pItemGroup)
						break;
					uint32_t dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
					const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::GetInstance()->GetSpecialAttrGroup(dwAttrVnum);
					if (!pAttrGroup)
						break;
					for (auto it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
					{
						m_pOwner->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -(int32_t)it->apply_value);
					}
				}
			}
			break;
	}
}

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
		case ITEM::TYPE_COSTUME:
		case ITEM::TYPE_ARMOR:
		case ITEM::TYPE_WEAPON:
		case ITEM::TYPE_ROD:
		case ITEM::TYPE_PICK:
		case ITEM::TYPE_UNIQUE:
		case ITEM::TYPE_DS:
		case ITEM::TYPE_SPECIAL_DS:
		case ITEM::TYPE_RING:
		case ITEM::TYPE_BELT:
		return true;
	}

	return false;
}

bool CItem::EquipTo(LPCHARACTER ch, uint8_t bWearCell)
{
	if (!ch)
	{
		SysLog("EquipTo: nil character");
		return false;
	}

	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * ITEM::DS_SLOT_MAX)
		{
			SysLog("EquipTo: invalid dragon soul cell (this: #{} {} wearflag: {} cell: {})", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
	{
		if (bWearCell >= WEAR_MAX_NUM)
		{
			SysLog("EquipTo: invalid wear cell (this: #{} {} wearflag: {} cell: {})", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
	}

	if (ch->GetWear(bWearCell))
	{
		SysLog("EquipTo: item already exist (this: #{} {} cell: {} {})", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

	ch->SetWear(bWearCell, this);

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell	= INVENTORY_MAX_NUM + bWearCell;

	uint32_t dwImmuneFlag = 0;

	for (int32_t i = 0; i < WEAR_MAX_NUM; ++i)
		if (m_pOwner->GetWear(i))
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);

	m_pOwner->SetImmuneFlag(dwImmuneFlag);

	if (IsDragonSoul())
	{
		DSManager::GetInstance()->ActivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(true);	
		StartUniqueExpireEvent();
		if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
			StartTimerBasedOnWearExpireEvent();

		StartAccessorySocketExpireEvent();
	}

	ch->BuffOnAttr_AddBuffsFromItem(this);

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	Save();

	return (true);
}

bool CItem::Unequip()
{
	if (!m_pOwner || GetCell() < INVENTORY_MAX_NUM)
	{
		TraceLog("{} {} GetCell {}",
				GetName(), GetID(), GetCell());
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - INVENTORY_MAX_NUM))
	{
		SysLog("m_pOwner->GetWear() != this");
		return false;
	}

	if (IsRideItem())
		ClearMountAttributeAndAffect();

	if (IsDragonSoul())
	{
		DSManager::GetInstance()->DeactivateDragonSoul(this);
	}
	else
	{
		ModifyPoints(false);
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		StopTimerBasedOnWearExpireEvent();

	StopAccessorySocketExpireEvent();

	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - INVENTORY_MAX_NUM, NULL);

	uint32_t dwImmuneFlag = 0;

	for (int32_t i = 0; i < WEAR_MAX_NUM; ++i)
		if (m_pOwner->GetWear(i))
			SET_BIT(dwImmuneFlag, m_pOwner->GetWear(i)->m_pProto->dwImmuneFlag);

	m_pOwner->SetImmuneFlag(dwImmuneFlag);

	m_pOwner->ComputeBattlePoints();

	m_pOwner->UpdatePacket();

	m_pOwner = nullptr;
	m_wCell = 0;
	m_bEquipped	= false;

	return true;
}

int32_t CItem::GetValue(uint32_t idx)
{
	assert(idx < ITEM::VALUES_MAX_NUM);
	return GetProto()->alValues[idx];
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::GetInstance()->DelayedSave(this);
}

bool CItem::CreateSocket(uint8_t bSlot, uint8_t bGold)
{
	assert(bSlot < ITEM::SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		SysLog("Item::CreateSocket : socket already exist {} {}", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const int32_t* c_al)
{
	memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSocket(int32_t i, int32_t v, bool bLog)
{
	assert(i < ITEM::SOCKET_MAX_NUM);
	m_alSockets[i] = v;
	UpdatePacket();
	Save();
	if (bLog)
		LogManager::GetInstance()->ItemLog(i, v, 0, GetID(), "SET_SOCKET", "", "", GetOriginalVnum());
}

int32_t CItem::GetGold()
{
	if (IS_SET(GetFlag(), ITEM::FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->dwISellItemPrice == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->dwISellItemPrice;
	}
	else
		return GetProto()->dwISellItemPrice;
}

int32_t CItem::GetShopBuyPrice()
{
	return GetProto()->dwIBuyItemPrice;
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pOwnershipEvent)
		return true;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("ownership_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pItem = info->item;

	pItem->SetOwnershipEvent(nullptr);

	TPacketGCItemOwnership p;

	p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID	= pItem->GetVID();
	p.szName[0]	= '\0';

	pItem->PacketAround(&p, sizeof(p));
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pEvent)
{
	m_pOwnershipEvent = pEvent;
}

void CItem::SetOwnership(LPCHARACTER ch, int32_t iSec)
{
	if (!ch)
	{
		if (m_pOwnershipEvent)
		{
			event_cancel(&m_pOwnershipEvent);
			m_dwOwnershipPID = 0;

			TPacketGCItemOwnership p;

			p.bHeader	= HEADER_GC_ITEM_OWNERSHIP;
			p.dwVID	= m_dwVID;
			p.szName[0]	= '\0';

			PacketAround(&p, sizeof(p));
		}
		return;
	}

	if (m_pOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	TPacketGCItemOwnership p;

	p.bHeader = HEADER_GC_ITEM_OWNERSHIP;
	p.dwVID = m_dwVID;
	strlcpy(p.szName, ch->GetName(), sizeof(p.szName));

	PacketAround(&p, sizeof(p));
}

int32_t CItem::GetSocketCount()
{
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM::SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int32_t count = GetSocketCount();
	if (count == ITEM::SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int32_t iSocketCount)
{
	if (iSocketCount >= ITEM::SOCKET_MAX_NUM)
	{
		PyLog("Invalid Socket Count {}, set to maximum", ITEM::SOCKET_MAX_NUM);
		iSocketCount = ITEM::SOCKET_MAX_NUM;
	}

	for (int32_t i = 0; i < iSocketCount; ++i)
		SetSocket(i, 1);
}

void CItem::AlterToMagicItem()
{
	int32_t idx = GetAttributeSetIndex();

	if (idx < 0)
		return;

	//      Appeariance Second Third
	// Weapon 50        20     5
	// Armor  30        10     2
	// Acc    20        10     1

	int32_t iSecondPct;
	int32_t iThirdPct;

	switch (GetType())
	{
		case ITEM::TYPE_WEAPON:
			iSecondPct = 20;
			iThirdPct = 5;
			break;

		case ITEM::TYPE_ARMOR:
		case ITEM::TYPE_COSTUME:
			if (GetSubType() == ITEM::ARMOR_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			break;

		default:
			return;
	}

	PutAttribute(aiItemMagicAttributePercentHigh);

	if (number(1, 100) <= iSecondPct)
		PutAttribute(aiItemMagicAttributePercentLow);

	if (number(1, 100) <= iThirdPct)
		PutAttribute(aiItemMagicAttributePercentLow);
}

uint32_t CItem::GetRefineFromVnum()
{
	return ITEM_MANAGER::GetInstance()->GetRefineFromVnum(GetVnum());
}

int32_t CItem::GetRefineLevel()
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int32_t	rtn = 0;
	str_to_number(rtn, p+1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int32_t	locale_rtn = 0;
		str_to_number(locale_rtn, p+1);
		if (locale_rtn != rtn)
		{
			SysLog("refine_level_based_on_NAME({}) is not equal to refine_level_based_on_LOCALE_NAME({}).", rtn, locale_rtn);
		}
	}

	return rtn;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM::TYPE_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("unique_expire_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM pItem = info->item;

	if (pItem->GetValue(2) == 0)
	{
		if (pItem->GetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			PyLog("UNIQUE_ITEM: expire {} {}", pItem->GetName(), pItem->GetID());
			pItem->SetUniqueExpireEvent(nullptr);
			ITEM_MANAGER::GetInstance()->RemoveItem(pItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pItem->SetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME, pItem->GetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();
		
		if (pItem->GetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pItem->SetUniqueExpireEvent(nullptr);
			ITEM_MANAGER::GetInstance()->RemoveItem(pItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// There is a bug in the game where time-based items do not disappear quickly
			// correction
			// by rtsummit
			if (pItem->GetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pItem->GetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("expire_event <Factor> Null pointer");
		return 0;
	}

	LPITEM pItem = info->item;
	int32_t remain_time = pItem->GetSocket(ITEM::SOCKET_REMAIN_SEC) - processing_time/passes_per_sec;
	if (remain_time <= 0)
	{
		PyLog("ITEM EXPIRED : expired {} {}", pItem->GetName(), pItem->GetID());
		pItem->SetTimerBasedOnWearExpireEvent(nullptr);
		pItem->SetSocket(ITEM::SOCKET_REMAIN_SEC, 0);
	
		if (pItem->IsDragonSoul())
		{
			DSManager::GetInstance()->DeactivateDragonSoul(pItem);
		}
		else
		{
			ITEM_MANAGER::GetInstance()->RemoveItem(pItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		}
		return 0;
	}
	pItem->SetSocket(ITEM::SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC (MIN (60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pEvent)
{
	m_pUniqueExpireEvent = pEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pEvent)
{
	m_pTimerBasedOnWearExpireEvent = pEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (!info)
		return 0;

	const LPITEM item = ITEM_MANAGER::GetInstance()->FindByVID(info->item_vid);

	if (!item)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		if(item->IsNewMountItem())
		{
			if (item->GetSocket(2) != 0)
				item->ClearMountAttributeAndAffect();
		}

		ITEM_MANAGER::GetInstance()->RemoveItem(item, "REAL_TIME_EXPIRE");

		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pRealTimeExpireEvent)
		return;
	for (int32_t i=0 ; i < ITEM::LIMIT_SLOT_MAX_NUM ; i++)
	{
		if (ITEM::LIMIT_REAL_TIME == GetProto()->aLimits[i].bType || ITEM::LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->aLimits[i].bType)
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pRealTimeExpireEvent = event_create(real_time_expire_event, info, PASSES_PER_SEC(1));

			PyLog("REAL_TIME_EXPIRE: StartRealTimeExpireEvent");

			return;
		}
	}
}

bool CItem::IsRealTimeItem()
{
	if(!GetProto())
		return false;
	for (int32_t i=0 ; i < ITEM::LIMIT_SLOT_MAX_NUM ; i++)
	{
		if (ITEM::LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return true;
	}
	return false;
}

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM::TYPE_UNIQUE)
		return;

	if (m_pUniqueExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int32_t iSec = GetSocket(ITEM::SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	SetSocket(ITEM::SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pTimerBasedOnWearExpireEvent)
		return;

	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->cLimitTimerBasedOnWearIndex)
		return;

	int32_t iSec = GetSocket(0);
	
	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pUniqueExpireEvent)
		return;

	if (GetValue(2) != 0)
		return;

	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM::SOCKET_UNIQUE_SAVE_TIME, event_time(m_pUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pUniqueExpireEvent);

	ITEM_MANAGER::GetInstance()->SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pTimerBasedOnWearExpireEvent)
		return;

	int32_t remain_time = GetSocket(ITEM::SOCKET_REMAIN_SEC) - event_processing_time(m_pTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM::SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::GetInstance()->SaveSingleItem(this);
}

void CItem::ApplyAddon(int32_t iAddonType)
{
	CItemAddonManager::GetInstance()->ApplyAddonTo(iAddonType, this);
}

int32_t CItem::GetSpecialGroup() const
{ 
	return ITEM_MANAGER::GetInstance()->GetSpecialGroupFromItem(GetVnum()); 
}


bool CItem::IsAccessoryForSocket()
{
	return (m_pProto->bType == ITEM::TYPE_ARMOR && (m_pProto->bSubType == ITEM::ARMOR_WRIST || m_pProto->bSubType == ITEM::ARMOR_NECK || m_pProto->bSubType == ITEM::ARMOR_EAR)) ||
		(m_pProto->bType == ITEM::TYPE_BELT); // In the case of the newly added 'belt' item in February 2013, the planning team suggested using the accessory socket system as it is.
}

void CItem::SetAccessorySocketGrade(int32_t iGrade) 
{ 
	SetSocket(0, MINMAX(0, iGrade, GetAccessorySocketMaxGrade())); 

	int32_t iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

	SetAccessorySocketDownGradeTime(iDownTime);
}

void CItem::SetAccessorySocketMaxGrade(int32_t iMaxGrade) 
{ 
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM)); 
}

void CItem::SetAccessorySocketDownGradeTime(uint32_t time) 
{ 
	SetSocket(2, time); 

	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Time left until the removal of the socket of %s: %d "), GetName(), time);
}

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("accessory_socket_expire_event> <Factor> Null pointer");
		return 0;
	}

	LPITEM item = ITEM_MANAGER::GetInstance()->FindByVID(info->item_vid);

	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
degrade:
		item->SetAccessorySocketExpireEvent(nullptr);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int32_t iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade() == 0)
		return;

	int32_t iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(nullptr);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

	int32_t new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}
		
bool CItem::IsRideItem()
{
	if (ITEM::TYPE_UNIQUE == GetType() && ITEM::UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM::TYPE_UNIQUE == GetType() && ITEM::UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
	return false;
}
		
bool CItem::IsRamadanRing()
{
	if (GetVnum() == UNIQUE_ITEM_RAMADAN_RING)
		return true;
	return false;
}

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();

	ch->RemoveAffect(AFFECT_MOUNT);
	ch->RemoveAffect(AFFECT_MOUNT_BONUS);

	ch->MountVnum(0);

	ch->PointChange(POINT_ST, 0);
	ch->PointChange(POINT_DX, 0);
	ch->PointChange(POINT_HT, 0);
	ch->PointChange(POINT_IQ, 0);
}

// fixme
// I don't use this right now... But I left it just in case.
// by rtsummit
bool CItem::IsNewMountItem()
{
	switch(GetVnum())
	{
		case 76000: case 76001: case 76002: case 76003: 
		case 76004: case 76005: case 76006: case 76007:
		case 76008: case 76009: case 76010: case 76011: 
		case 76012: case 76013: case 76014:
			return true;
	}
	return false;
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pEvent)
{
	m_pAccessorySocketExpireEvent = pEvent;
}

void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade() > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A gem socketed in the %s has vanished."), GetName());
		}

		ModifyPoints(false);
		SetAccessorySocketGrade(GetAccessorySocketGrade()-1);
		ModifyPoints(true);

		int32_t iDownTime = aiAccessorySocketDegradeTime[GetAccessorySocketGrade()];

		if (test_server)
			iDownTime /= 60;

		SetAccessorySocketDownGradeTime(iDownTime);

		if (iDownTime)
			StartAccessorySocketExpireEvent();
	}
}

static const bool CanPutIntoRing(LPITEM ring, LPITEM item)
{
	const uint32_t vnum = item->GetVnum();
	return false;
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM::TYPE_BELT)
		return this->GetSubType() == ITEM::USE_PUT_INTO_BELT_SOCKET;

	else if(item->GetType() == ITEM::TYPE_RING)
		return CanPutIntoRing(item, this);

	else if (item->GetType() != ITEM::TYPE_ARMOR)
		return false;

	uint32_t vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		uint32_t jewel;
		uint32_t wrist;
		uint32_t neck;
		uint32_t ear;
	};
	const static JewelAccessoryInfo infos[] = { 
		{ 50634, 14420, 16220, 17220 }, 
		{ 50635, 14500, 16500, 17500 }, 
		{ 50636, 14520, 16520, 17520 }, 
		{ 50637, 14540, 16540, 17540 }, 
		{ 50638, 14560, 16560, 17560 }, 
	};
	
	uint32_t item_type = (item->GetVnum() / 10) * 10;
	for (int32_t i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch(item->GetSubType())
		{
		case ITEM::ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ITEM::ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ITEM::ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == GetVnum())
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		}
	}
	if (item->GetSubType() == ITEM::ARMOR_WRIST)
		vnum -= 14000;
	else if (item->GetSubType() == ITEM::ARMOR_NECK)
		vnum -= 16000;
	else if (item->GetSubType() == ITEM::ARMOR_EAR)
		vnum -= 17000;
	else
		return false;

	uint32_t type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != GetVnum())
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != GetVnum())
			return false;
		else
			return true;
	}

	return 50623 + type == GetVnum();
}

bool CItem::CheckItemUseLevel(int32_t nLevel)
{
	for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == ITEM::LIMIT_LEVEL)
		{
			if (this->m_pProto->aLimits[i].lValue > nLevel) return false;
			else return true;
		}
	}
	return true;
}

int32_t CItem::FindApplyValue(uint8_t bApplyType)
{
	if (m_pProto == nullptr)
		return 0;

	for (int32_t i = 0; i < ITEM::APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->aApplies[i].bType == bApplyType)
			return m_pProto->aApplies[i].lValue;
	}

	return 0;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}

int32_t CItem::GetAccessorySocketGrade()
{
   	return MINMAX(0, GetSocket(0), GetAccessorySocketMaxGrade()); 
}

int32_t CItem::GetAccessorySocketMaxGrade()
{
   	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}

int32_t CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGrade()]);
}

void CItem::AttrLog()
{
	const char* pszIP = nullptr;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
	{
		if (m_alSockets[i])
		{
			LogManager::GetInstance()->ItemLog(i, m_alSockets[i], 0, GetID(), "INFO_SOCKET", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}

	for (int32_t i = 0; i<ITEM::ATTRIBUTE_MAX_NUM; ++i)
	{
		int32_t	type	= m_aAttr[i].bType;
		int32_t value	= m_aAttr[i].sValue;

		if (type)
			LogManager::GetInstance()->ItemLog(i, type, value, GetID(), "INFO_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
	}
}

int32_t CItem::GetLevelLimit()
{
	for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
	{
		if (this->m_pProto->aLimits[i].bType == ITEM::LIMIT_LEVEL)
		{
			return this->m_pProto->aLimits[i].lValue;
		}
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{
	if (-1 != this->GetProto()->cLimitRealTimeFirstUseIndex)
	{
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}

	return true;
}

bool CItem::IsDragonSoul()
{
	return GetType() == ITEM::TYPE_DS;
}

int32_t CItem::GiveMoreTime_Per(float fPercent)
{
	if (IsDragonSoul())
	{
		uint32_t duration = DSManager::GetInstance()->GetDuration(this);
		int32_t remain_sec = GetSocket(ITEM::SOCKET_REMAIN_SEC);
		int32_t given_time = fPercent * duration / 100;
		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM::SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM::SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}
	// First of all, let's talk about the Dragon Soul Stone.
	else
		return 0;
}

int32_t CItem::GiveMoreTime_Fix(uint32_t dwTime)
{
	if (IsDragonSoul())
	{
		uint32_t duration = DSManager::GetInstance()->GetDuration(this);
		int32_t remain_sec = GetSocket(ITEM::SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM::SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec; 
		}
		else
		{
			SetSocket(ITEM::SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}
	else
		return 0;
}


int32_t	CItem::GetDuration()
{
	if(!GetProto())
		return -1;

	for (int32_t i=0 ; i < ITEM::LIMIT_SLOT_MAX_NUM ; i++)
	{
		if (ITEM::LIMIT_REAL_TIME == GetProto()->aLimits[i].bType)
			return GetProto()->aLimits[i].lValue;
	}
	
	if (-1 != GetProto()->cLimitTimerBasedOnWearIndex)
		return GetProto()->aLimits[GetProto()->cLimitTimerBasedOnWearIndex].lValue;	

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}