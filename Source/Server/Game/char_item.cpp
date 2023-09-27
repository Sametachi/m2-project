#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "BattleArena.h"
#include "arena.h"
#include "threeway_war.h"

#include "safebox.h"
#include "shop.h"

#include <Common/VnumHelper.h>
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"

#include <Core/Constants/Item.hpp>
#include <Core/Constants/Controls.hpp>

const int32_t ITEM_BROKEN_METIN_VNUM = 28960;

// CHANGE_ITEM_ATTRIBUTES
const uint32_t CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const uint8_t g_aBuffOnAttrPoints[] = { POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<uint32_t, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone())
			{
				m_mapStone[(uint32_t)pChar->GetVID()] = pChar;
			}
		}
	}
};


// return part, return memory part, wedding ring
static bool IS_SUMMON_ITEM(int32_t vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int32_t map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int32_t map_index)
{
	// Monkey Dungeon
	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	// Castle
	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 :
		case 71 :
		case 72 :
		case 73 :
		case 193 :
#if 0
		case 184 :
		case 185 :
		case 186 :
		case 187 :
		case 188 :
		case 189 :
#endif
//		case 206 :
		case 216 :
		case 217 :
		case 208 :
			return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	// non-warpable for all private maps
	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int32_t nMapIndex)
{
	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}
	
	return false;
}

// Check if item socket is equal to prototype -- by mhh
static bool FN_check_item_socket(LPITEM item)
{
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

// copy item socket -- by mhh
static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{

	if (IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_FEMALE)) 
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

	if (IsWarping())
		return false;

	return true;
}

LPITEM CHARACTER::GetInventoryItem(uint16_t wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}
LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return NULL;
	uint16_t wCell = Cell.cell;
	uint8_t window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			SysLog("CHARACTER::GetInventoryItem: invalid item cell {}", wCell);
			return NULL;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			SysLog("CHARACTER::GetInventoryItem: invalid DS item cell {}", wCell);
			return NULL;
		}
		return m_pointsInstant.pDSItems[wCell];

	default:
		return NULL;
	}
	return NULL;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem)
{
	uint16_t wCell = Cell.cell;
	uint8_t window_type = Cell.window_type;
	if ((uint32_t)((CItem*)pItem) == 0xff || (uint32_t)((CItem*)pItem) == 0xffffffff)
	{
		SysLog("!!! FATAL ERROR !!! item == 0xff (char: {} cell: {})", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}
	// basic inventory
	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				SysLog("CHARACTER::SetItem: invalid item cell {}", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int32_t i = 0; i < pOld->GetSize(); ++i)
					{
						int32_t p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
							continue;

						m_pointsInstant.bItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int32_t i = 0; i < pItem->GetSize(); ++i)
					{
						int32_t p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						// Doing wCell + 1 is the same as checking for empty space
						// Item is for exception handling
						m_pointsInstant.bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pItems[wCell] = pItem;
		}
		break;
	// Dragon Soul Inventory
	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pDSItems[wCell];

			if (pOld)
			{
				if (wCell < ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int32_t i = 0; i < pOld->GetSize(); ++i)
					{
						int32_t p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
							continue;

						m_pointsInstant.wDSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					SysLog("CHARACTER::SetItem: invalid DS item cell {}", wCell);
					return;
				}

				if (wCell < ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int32_t i = 0; i < pItem->GetSize(); ++i)
					{
						int32_t p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						// Doing wCell + 1 is the same as checking for empty space
						// Item is for exception handling
						m_pointsInstant.wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pDSItems[wCell] = pItem;
		}
		break;
	default:
		SysLog("Invalid Inventory type {}", window_type);
		return;
	}

	if (GetDesc())
	{
		// Extended item: send item flag information from server
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags	= pItem->GetAntiFlag();
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);


			memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDel pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDel));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
		}
	}
}

LPITEM CHARACTER::GetWear(uint8_t bCell) const
{
	// > WEAR_MAX_NUM : Dragon Soul Stone slots.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * ITEM::DS_SLOT_MAX)
	{
		SysLog("CHARACTER::GetWear: invalid wear cell {}", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(uint8_t bCell, LPITEM item)
{
	// > WEAR_MAX_NUM : Dragon Soul Stone slots.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * ITEM::DS_SLOT_MAX)
	{
		SysLog("CHARACTER::SetItem: invalid item cell {}", bCell);
		return;
	}

	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);

	if (!item && bCell == WEAR_WEAPON)
	{
		// When using a ghost sword, the effect must be removed if it is taken off.
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}
}

void CHARACTER::ClearItem()
{
	int32_t		i;
	LPITEM	item;
	
	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, uint8_t bSize, int32_t iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			uint8_t bCell = Cell.cell;

			// bItemCell is processed by + 1 to indicate that 0 is false.
			// Therefore, 1 is added to the iExceptionCell for comparison.
			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (!beltItem)
					return false;

				if (!CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_pointsInstant.bItemGrid[bCell])
				{
					if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
			else if (bCell >= INVENTORY_MAX_NUM)
				return false;

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int32_t j = 1;
					uint8_t bPage = bCell / (INVENTORY_MAX_NUM / 2);

					do
					{
						uint8_t p = bCell + (5 * j);

						if (p >= INVENTORY_MAX_NUM)
							return false;

						if (p / (INVENTORY_MAX_NUM / 2) != bPage)
							return false;

						if (m_pointsInstant.bItemGrid[p])
							if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// If the size is 1, it occupies one space, so just return
			if (1 == bSize)
				return true;
			else
			{
				int32_t j = 1;
				uint8_t bPage = bCell / (INVENTORY_MAX_NUM / 2);

				do
				{
					uint8_t p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / 2) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	case DRAGON_SOUL_INVENTORY:
		{
			uint16_t wCell = Cell.cell;
			if (wCell >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			// bItemCell is processed by + 1 to indicate that 0 is false.
			// Therefore, 1 is added to the iExceptionCell for comparison.
			iExceptionCell++;

			if (m_pointsInstant.wDSItemGrid[wCell])
			{
				if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int32_t j = 1;

					do
					{
						uint8_t p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.wDSItemGrid[p])
							if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// If the size is 1, it occupies one space, so just return
			if (1 == bSize)
				return true;
			else
			{
				int32_t j = 1;

				do
				{
					uint8_t p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
	}
	return false;
}

int32_t CHARACTER::GetEmptyInventory(uint8_t size) const
{
	// NOTE: Currently, this function is used to find empty spaces in the inventory when performing actions such as item payment and acquisition.
	// Belt inventory is a special inventory, so don't check it. (Basic inventory: scan up to INVENTORY_MAX_NUM)
	for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}

int32_t CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (!pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	uint8_t bSize = pItem->GetSize();
	uint16_t wBaseCell = DSManager::GetInstance()->GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int32_t i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<uint16_t>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

int32_t CHARACTER::CountEmptyInventory() const
{
	int32_t	count = 0;

	for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (INVENTORY_MAX_NUM - count);
}

void TransformRefineItem(LPITEM pOldItem, LPITEM pNewItem)
{
	// ACCESSORY_REFINE
	if (pOldItem->IsAccessoryForSocket())
	{
		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			pNewItem->SetSocket(i, pOldItem->GetSocket(i));
		}
		//pNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		// Broken stones are automatically cleaned here
		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			if (!pOldItem->GetSocket(i))
				break;
			else
				pNewItem->SetSocket(i, 1);
		}

		// socket settings
		int32_t slot = 0;

		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			int32_t socket = pOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pNewItem->SetSocket(slot++, socket);
		}

	}

	// Magic Item Settings
	pOldItem->CopyAttributeTo(pNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != nullptr)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");

		LogManager::GetInstance()->RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int32_t success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");

		LogManager::GetInstance()->RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if (ch != nullptr)
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}
	
	// Refinement time limit: In upgrade_refine_scroll.quest, general improvement is performed within 5 minutes after improvement.
	//can't proceed
	if (quest::CQuestManager::GetInstance()->GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::GetInstance()->GetEventFlag("update_refine_time") + (60 * 5))
		{
			PyLog("can't refine {} {}", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable* prt = CRefineManager::GetInstance()->GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	uint32_t result_vnum = item->GetRefinedVnum();

	// REFINE_COST
	int32_t cost = ComputeRefineFee(prt->cost);

	int32_t RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");

	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM::TYPE_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The free weapon improvements can only be used on weapons up to level 20."));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No further improvements possible."));
		return false;
	}

	if (item->GetType() == ITEM::TYPE_USE && item->GetSubType() == ITEM::USE_TUNING)
		return false;

	TItemTable* pProto = ITEM_MANAGER::GetInstance()->GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		SysLog("DoRefine NOT GET ITEM PROTO {}", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
		return false;
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang to use this item."));
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		for (int32_t i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not the right material for an upgrade."));
				return false;
			}
		}

		for (int32_t i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int32_t prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	// END_OF_REFINE_COST

	if (prob <= prt->prob)
	{
		// success! All items disappear, and another item with the same attribute is acquired
		LPITEM pNewItem = ITEM_MANAGER::GetInstance()->CreateItem(result_vnum, 1, 0, false);

		if (pNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pNewItem);
			LogManager::GetInstance()->ItemLog(this, pNewItem, "REFINE SUCCESS", pNewItem->GetName());

			uint8_t bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(pNewItem);

			PyLog("Refine Success {}", cost);
			pNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			PyLog("PayPee {}", cost);
			PayRefineFee(cost);
			PyLog("PayPee End {}", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG
			// Failed to create item -> regarded as improvement failure
			SysLog("cannot create item {}", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		//failure! All items disappear.
		DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		item->AttrLog();
		ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN   = 1, // Used only in China
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL   = 3,
	YAGONG_SCROLL  = 4,
	MEMO_SCROLL	   = 5,
	BDRAGON_SCROLL	= 6,
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	// Refinement time limit: In upgrade_refine_scroll.quest, general improvement is performed within 5 minutes after improvement.
	//can't proceed
	if (quest::CQuestManager::GetInstance()->GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::GetInstance()->GetEventFlag("update_refine_time") + (60 * 5))
		{
			PyLog("can't refine {} {}", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable* prt = CRefineManager::GetInstance()->GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pItemScroll;

	// improvement letter check
	if (m_iRefineAdditionalCell < 0)
		return false;

	pItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pItemScroll)
		return false;

	if (!(pItemScroll->GetType() == ITEM::TYPE_USE && pItemScroll->GetSubType() == ITEM::USE_TUNING))
		return false;

	if (pItemScroll->GetVnum() == item->GetVnum())
		return false;

	uint32_t result_vnum = item->GetRefinedVnum();
	uint32_t result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No further improvements possible."));
		return false;
	}

	// MUSIN_SCROLL
	if (pItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot upgrade items with this Scroll."));
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be absorbed."));
			return false;
		}
	}
	else if (pItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM::TYPE_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
			return false;
		}
	}

	TItemTable* pProto = ITEM_MANAGER::GetInstance()->GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		SysLog("DoRefineWithScroll NOT GET ITEM PROTO {}", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
		return false;
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough Yang to use this item."));
		return false;
	}

	for (int32_t i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Not the right material for an upgrade."));
			return false;
		}
	}

	for (int32_t i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int32_t prob = number(1, 100);
	int32_t success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pItemScroll->GetValue(0) == HYUNIRON_CHN || 
		pItemScroll->GetValue(0) == YONGSIN_SCROLL || 
		pItemScroll->GetValue(0) == YAGONG_SCROLL) // Process Hyeon-cheol, Dragon God's Blessing, and Night Gong's Scroll
	{
		const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };
		const char hyuniron_prob_euckr[9] = { 100, 75, 65, 55, 45, 40, 35, 30, 25 };

		const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };
		const char yagong_prob_euckr[9] = { 100, 100, 90, 80, 70, 60, 50, 40, 30 };

		if (pItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			success_prob = hyuniron_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			success_prob = yagong_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else
		{
			SysLog("REFINE : Unknown refine scroll item. Value0: {}", pItemScroll->GetValue(0));
		}

		if (test_server) 
		{
			ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
		}
		if (pItemScroll->GetValue(0) == HYUNIRON_CHN) // Hyeoncheol should break the item.
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pItemScroll->GetValue(0) == MUSIN_SCROLL)// God's Blessing is 100% successful (up to +4)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	pItemScroll->SetCount(pItemScroll->GetCount() - 1);

	if (prob <= success_prob)
	{
		// success! All items disappear, and another item with the same attribute is acquired
		LPITEM pNewItem = ITEM_MANAGER::GetInstance()->CreateItem(result_vnum, 1, 0, false);

		if (pNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pNewItem);
			LogManager::GetInstance()->ItemLog(this, pNewItem, "REFINE SUCCESS", pNewItem->GetName());

			uint8_t bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(pNewItem);
			pNewItem->AttrLog();
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// Failed to create item -> Considered as improvement failure
			SysLog("cannot create item {}", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		// failure! All items disappear, and lower-grade items of the same attribute are acquired
		LPITEM pNewItem = ITEM_MANAGER::GetInstance()->CreateItem(result_fail_vnum, 1, 0, false);

		if (pNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pNewItem);
			LogManager::GetInstance()->ItemLog(this, pNewItem, "REFINE FAIL", pNewItem->GetName());

			uint8_t bCell = item->GetCell();

			DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (REFINE FAIL)");

			pNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::GetInstance()->FlushDelayedSave(pNewItem);

			pNewItem->AttrLog();

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// Failed to create item -> Considered as improvement failure
			SysLog("cannot create item {}", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType); // Item does not disappear when upgrading
		
		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(uint8_t bCell, uint8_t bType, int32_t iAdditionalCell)
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	// REFINE_COST
	if (bType == ITEM::REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only be rewarded once for the Demon Tower Quest."));
		return false;
	}
	// END_OF_REFINE_COST

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		SysLog("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
		return false;
	}

	if (item->GetType() == ITEM::TYPE_USE && item->GetSubType() == ITEM::USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be advanced this way."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot bind this item to your soul."));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can combine the Blessing Scroll with the Magic Iron Ore."));
				return false;
			}
		}
	}

	auto rm = CRefineManager::GetInstance();

	const TRefineTable* prt = rm->GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		SysLog("RefineInformation NOT GET REFINE SET {}", item->GetRefineSet());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
		return false;
	}

	// REFINE_COST
	
	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM::TYPE_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The free weapon improvements can only be used on weapons up to level 20."));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);
	
	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == ITEM::REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pItem, LPITEM pTarget)
{
	if (!CanHandleItem())
		return false;

	if (pItem->GetSubType() == ITEM::USE_TUNING)
	{
		// XXX performance, socket improvements are gone...
		// XXX performance improvement book became a blessing book!
		// MUSIN_SCROLL
		if (pItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pTarget->GetCell(), ITEM::REFINE_TYPE_MUSIN, pItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pTarget->GetCell(), ITEM::REFINE_TYPE_HYUNIRON, pItem->GetCell());
		else if (pItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pTarget->GetRefineSet() != 702) return false;
			RefineInformation(pTarget->GetCell(), ITEM::REFINE_TYPE_BDRAGON, pItem->GetCell());
		}
		else
		{
			if (pTarget->GetRefineSet() == 501) return false;
			RefineInformation(pTarget->GetCell(), ITEM::REFINE_TYPE_SCROLL, pItem->GetCell());
		}
	}
	else if (pItem->GetSubType() == ITEM::USE_DETACHMENT && IS_SET(pTarget->GetFlag(), ITEM::FLAG_REFINEABLE))
	{
		LogManager::GetInstance()->ItemLog(this, pTarget, "USE_DETACHMENT", pTarget->GetName());

		bool bHasMetinStone = false;

		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; i++)
		{
			int32_t socket = pTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
			{
				int32_t socket = pTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::GetInstance()->GetTable(pTarget->GetSocket(i));
					//pTarget->SetSocket(i, pTable->alValues[2]);
					// Replace with broken stone
					pTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pItem->SetCount(pItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no Stone available to take out."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("kill_campfire_event> <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == nullptr) { // <Factor>
		return 0;
	}
	ch->m_pMiningEvent = nullptr;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int32_t idx = GetMapIndex();
	int32_t iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot store this location."));
		return false;
	}

	int32_t pos;

	if (item->GetCount() == 1)	// If there is only one item, just set it.
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1) // If not, find another inventory slot.
	{
		LPITEM item2 = ITEM_MANAGER::GetInstance()->CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There isn't enough space in your inventory."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int32_t idx;

	if ((idx = SECTREE_MANAGER::GetInstance()->GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int32_t iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
		// When the evil dragon was also
		case 301:
		case 302:
		case 303:
		case 304:
			if(GetLevel() < 90)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your level is too low to use this item."));
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot teleport to a safe position in a foreign Kingdom."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		TraceLog("Recall: {} {} {} -> {} {}", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only open the shop if you take off your armour."));
			break;
	}
}

// MYSHOP_PRICE_LIST
void CHARACTER::SendMyShopPriceListCmd(uint32_t dwItemVnum, uint32_t dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
}

//
// Sends the list received from the DB cache to the user and sends a command to open a store.
//
void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)
		// There is no price list. Sends a command with dummy data.
		SendMyShopPriceListCmd(1, 0);
	else {
		for (int32_t idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[ idx ].dwVnum, pInfo[ idx ].dwPrice);
	}

	__OpenPrivateShop();
}

//
// When opening a store for the first time after this connection, a price information list request packet is sent to the DB cache to load the list.
// From then on, a response to open the store is sent immediately.
//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		uint32_t dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(uint32_t));
		m_bNoOpenedShop = false;
	} else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST


int32_t CalculateConsume(LPCHARACTER ch)
{
	static const int32_t WARP_NEED_LIFE_PERCENT	= 30;
	static const int32_t WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int32_t consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int32_t curLife		= ch->GetHP();
		const int32_t needPercent	= WARP_NEED_LIFE_PERCENT;
		const int32_t needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough HP."));
			return -1;
		}

		consumeLife = needLife;


		// CheckMinLifeForWarp: You must not be killed by poison, so leave a minimum amount of life.
		const int32_t minPercent	= WARP_MIN_LIFE_PERCENT;
		const int32_t minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int32_t CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int32_t NEED_WARP_SP_PERCENT = 30;

	const int32_t curSP = lpChar->GetSP();
	const int32_t needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You don't have enough Spell Points (SP) to use this."));
		return -1;
	}

	return needSP;
}

bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	int32_t iLimitRealtimeStartFirstUseFlagIndex = -1;
	int32_t iLimitTimerBasedOnWearFlagIndex = -1;

	uint16_t wDestCell = DestCell.cell;
	uint8_t bDestInven = DestCell.window_type;
	for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
	{
		int32_t limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case ITEM::LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your level is too low to use this item."));
					return false;
				}
				break;

			case ITEM::LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case ITEM::LIMIT_TIMER_BASED_ON_WEAR:
				iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		PyLog("USE_ITEM {}, Inven {}, Cell {}, ItemType {}, SubType {}", item->GetName(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if (CArenaManager::GetInstance()->IsLimitedItem(GetMapIndex(), item->GetVnum()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
		return false;
	}

	// After the first use of the item, the time is deducted even if it is not used.
	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		// Whether an item has been used at least once is judged by looking at Socket1. (Record the number of uses in Socket1)
		if (0 == item->GetSocket(1))
		{
			// For the available time, use the Limit Value as the default value, but if there is a value in Socket0, use that value. (unit is seconds)
			int32_t duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}	

		if (!item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);		
	}

	switch (item->GetType())
	{
	case ITEM::TYPE_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM::TYPE_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM::TYPE_QUEST:
			if (GetArena() != nullptr || IsObserverMode())
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
					return false;
				}
			}

			if (!IS_SET(item->GetFlag(), ITEM::FLAG_QUEST_USE | ITEM::FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::GetInstance()->UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::GetInstance()->SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
			break;

		case ITEM::TYPE_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(GetMapIndex(), (int32_t)(GetX()+fx), (int32_t)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot build a campfire here."));
					return false;
				}

				if (tree->IsAttr((int32_t)(GetX()+fx), (int32_t)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot build a campfire under water."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::GetInstance()->SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (int32_t)(GetX()+fx), (int32_t)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM::TYPE_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case ITEM::USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case ITEM::APPLY_MOV_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_ATT_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_RESIST_MAGIC:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS, 
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case ITEM::APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == ITEM::USE_SPECIAL)
							{
								PyLog("ITEM_UNIQUE: USE_SPECIAL {}", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049: // silk bag
										UseSilkBotary();
										break;
								}
							}
							else
							{
								if (!item->IsEquipped())
									EquipItem(item);
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;

		case ITEM::TYPE_COSTUME:
		case ITEM::TYPE_WEAPON:
		case ITEM::TYPE_ARMOR:
		case ITEM::TYPE_ROD:
		case ITEM::TYPE_RING:
		case ITEM::TYPE_BELT:
			// MINING
		case ITEM::TYPE_PICK:
			// END_OF_MINING
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		// Dragon Soul Stones that are not worn cannot be used.
		// If it is a normal Clara, it is not possible to send item use packets about the Dragon Soul Stone.
		// Wearing the Dragon Soul Stone is done with the item move packet.
		// Extract the worn dragon soul stone.
		case ITEM::TYPE_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::GetInstance()->PullOut(this, NPOS, item);
			break;
			}
		case ITEM::TYPE_SPECIAL_DS:
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		
		case ITEM::TYPE_FISH:
			{
				if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
					return false;
				}

				if (item->GetSubType() == ITEM::FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM::TYPE_TREASURE_BOX:
			{
				return false;
				//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("Closed. You should look for the key."));
			}
			break;

		case ITEM::TYPE_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() != ITEM::TYPE_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("This item cannot be opened with a key."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("That's the right key."));
					uint32_t dwBoxVnum = item2->GetVnum();
					std::vector <uint32_t> dwVnums;
					std::vector <uint32_t> dwCounts;
					std::vector <LPITEM> item_gets(0);
					int32_t count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
						ITEM_MANAGER::GetInstance()->RemoveItem(item);
						ITEM_MANAGER::GetInstance()->RemoveItem(item2);
						
						for (int32_t i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), dwCounts[i]);
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A mysterious light comes out of the box."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d experience points."), dwCounts[i]);
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the red smoke coming out of the box, your speed will increase!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box suddenly exploded! You have lost Hit Points (HP)."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the green smoke that is coming out of the box, the poison will spread through your body!"));
									break;
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Receive: %s - %d"), item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box contains %s."), item_gets[i]->GetName());

									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("This key does not seem to fit the lock."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("This key does not seem to fit the lock."));
					return false;
				}
			}
			break;

		case ITEM::TYPE_GIFTBOX:
			{
				uint32_t dwBoxVnum = item->GetVnum();
				std::vector <uint32_t> dwVnums;
				std::vector <uint32_t> dwCounts;
				std::vector <LPITEM> item_gets(0);
				int32_t count = 0;


				if((dwBoxVnum > 51500 && dwBoxVnum < 52000) || (dwBoxVnum >= 50255 && dwBoxVnum <= 50260))	// dragon soul stones
				{
					if(!(this->DragonSoul_IsQualified()))
					{
						ChatPacket(CHAT_TYPE_INFO,LC_TEXT("Before you open the Cor Draconis, you have to complete the Dragon Stone quest and activate the Dragon Stone Alchemy."));
						return false;
					}
				}

				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
				{
					item->SetCount(item->GetCount()-1);

					for (int32_t i = 0; i < count; i++){
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), dwCounts[i]);
							break;
						case CSpecialItemGroup::EXP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A mysterious light comes out of the box."));
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d experience points."), dwCounts[i]);
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the red smoke coming out of the box, your speed will increase!"));
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box suddenly exploded! You have lost Hit Points (HP)."));
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the green smoke that is coming out of the box, the poison will spread through your body!"));
							break;
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
							break;
						default:
							if (item_gets[i])
							{
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Receive: %s - %d"), item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box contains %s."), item_gets[i]->GetName());
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("You have not received anything."));
					return false;
				}
			}
			break;

		case ITEM::TYPE_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::GetInstance()->RemoveItem(item);
					return false;
				}

				uint32_t dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::GetInstance()->RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have lowered your Skill Level."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot lower your Skill Level."));
			}
			break;

		case ITEM::TYPE_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
					return false;
				}

				uint32_t dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					// The new training book has a skill number at value 0, so use it.
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::GetInstance()->RemoveItem(item);

					return false;
				}

				if (LearnSkillByBook(dwVnum))
				{
					ITEM_MANAGER::GetInstance()->RemoveItem(item);

					int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM::TYPE_USE:
			{
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						PyLog("ADD addtional effect : vnum({}) subtype({})", item->GetOriginalVnum(), item->GetSubType());

					int32_t affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int32_t apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int32_t apply_value = item->GetValue(2);
					int32_t apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case ITEM::USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case ITEM::APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case ITEM::APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case ITEM::APPLY_STR:
									case ITEM::APPLY_DEX:
									case ITEM::APPLY_CON:
									case ITEM::APPLY_INT:
									case ITEM::APPLY_CAST_SPEED:
									case ITEM::APPLY_RESIST_MAGIC:
									case ITEM::APPLY_ATT_GRADE_BONUS:
									case ITEM::APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case ITEM::USE_AFFECT :
						{
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
							}
							else
							{
								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case ITEM::USE_POTION_NODELAY:
						{
							if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
							{
								if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("That is over the limit."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
										return false;
										break;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0)
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0)
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Used Moon Cake or Seed."));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
						return false;
					}
				}

				if (test_server)
				{
					 PyLog("USE_ITEM {} Type {} SubType {} vnum {}", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case ITEM::USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (!pDestItem)
							{
								return false;
							}
							// First of all, let's talk about the Dragon Soul Stone.
							if (pDestItem->IsDragonSoul())
							{
								int32_t ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM::SOCKET_CHARGING_AMOUNT_IDX));
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%d}", ret, item->GetVnum(), ITEM::SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM::SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM::VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d seconds have been added."), ret);
									item->SetCount(item->GetCount() - 1);
									LogManager::GetInstance()->ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%d}", item->GetVnum(), ITEM::SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM::SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM::VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recharge this Dragon Stone."));
									LogManager::GetInstance()->ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case ITEM::USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (!pDestItem)
							{
								return false;
							}
							// First of all, let's talk about the Dragon Soul Stone.
							if (pDestItem->IsDragonSoul())
							{
								int32_t ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d seconds have been added."), ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM::VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
									LogManager::GetInstance()->ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot recharge this Dragon Stone."));
									sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM::VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM::VALUE_CHARGING_AMOUNT_IDX));
									LogManager::GetInstance()->ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case ITEM::USE_SPECIAL:
						
						switch (item->GetVnum())
						{
							//Christmas Lanzhou
							case ITEM_NOG_POCKET:
								{
									/*
										Ranju ability value: Meaning of item_proto value
										movement speed value 1
										Attack value 2
										experience value 3
										duration value 0 (in seconds)
									*/
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
										return false;
									}
									int32_t time = item->GetValue(0);
									int32_t moveSpeedPer	= item->GetValue(1);
									int32_t attPer	= item->GetValue(2);
									int32_t expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
								
							case ITEM_RAMADAN_CANDY:
								{
									/*
										Candy stat: Meaning of item_proto value
										movement speed value 1
										Attack value 2
										experience value 3
										duration value 0 (in seconds)
									*/
									
									int32_t time = item->GetValue(0);
									int32_t moveSpeedPer	= item->GetValue(1);
									int32_t attPer	= item->GetValue(2);
									int32_t expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									marriage::TMarriage* pMarriage = marriage::CManager::GetInstance()->Get(GetPlayerID());
									if (pMarriage)
									{
										if (pMarriage->ch1 != nullptr)
										{
											if (CArenaManager::GetInstance()->IsArenaMap(pMarriage->ch1->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
												break;
											}
										}

										if (pMarriage->ch2 != nullptr)
										{
											if (CArenaManager::GetInstance()->IsArenaMap(pMarriage->ch2->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
												break;
											}
										}

										int32_t consumeSP = CalculateConsumeSP(this);

										if (consumeSP < 0)
											return false;

										PointChange(POINT_SP, -consumeSP, false);

										WarpToPID(pMarriage->GetOther(GetPlayerID()));
									}
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot wear a Wedding Ring if you are not married."));
								}
								break;

							case UNIQUE_ITEM_CAPE_OF_COURAGE:
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
								AggregateMonster();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								{
									const int32_t MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										uint32_t count;
										int32_t prob;
										uint32_t vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									struct LuckyBagInfo b2[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27002 },
										{ 10,	100,	27005 },
										{ 10,	50,	27005 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};
	
									LuckyBagInfo * bi = nullptr;
									bi = b1;

									int32_t pct = number(1, 1000);

									int32_t i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										// The skill training book is given specially.
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::GetInstance()->RemoveItem(item);
								}
								break;

							case 50004: // Detectors for Events
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										// first time use
										int32_t iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::GetInstance()->GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use the Event Detector from this position."));
											return false;
										}
									}

									int32_t dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{
										// discovery!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Event Detector vanished in a mysterious light."));

										// Different items are given depending on the number of times used.
										struct TEventStoneInfo
										{
											uint32_t dwVnum;
											int32_t count;
											int32_t prob;
										};
										const int32_t EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int32_t prob = number(1, 100);

											for (int32_t i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													if (info[i].dwVnum == 50001)
													{
														uint32_t* pdw = M2_NEW uint32_t[2];

														pdw[0] = info[i].dwVnum;
														pdw[1] = info[i].count;

														// lottery set socket
														DBManager::GetInstance()->ReturnQuery(QID_LOTTO, GetPlayerID(), pdw,
																"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())", 
																get_table_postfix(), GetPlayerID());
													}
													else
														AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int32_t len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (uint32_t)GetVID());

										if (len < 0 || len >= (int32_t) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // Send up to \0 character

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									// It disappears after a lot of use.
									const int32_t STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Event Detector has vanished."));
										ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int32_t len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(uint32_t)GetVID(), dist, (int32_t)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int32_t) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // send up to \0 character

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989: // Young Stone Detector
							case 76006: // Gift Detector
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(GetMapIndex());

									if (pMap != nullptr)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<uint32_t, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											uint32_t max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												uint32_t dist = (uint32_t)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != nullptr)
											{
												int32_t val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (uint32_t)GetVID(), val,
														(int32_t)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is currently no Metin Stone on this map."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is currently no Metin Stone on this map."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (uint32_t)GetVID());
											ITEM_MANAGER::GetInstance()->RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996: // poison
								item->SetCount(item->GetCount() - 1);
								break;

							case 27987: // clam
								{
									item->SetCount(item->GetCount() - 1);

									int32_t r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You find a simple Piece of Stone in the Clam."));
										AutoGiveItem(27990);
									}
									else
									{
										const int32_t prob_table_euckr[] =
										{
											80, 90, 97
										};

										const int32_t prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int32_t* prob_table = prob_table_euckr;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Clam has vanished."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is a White Pearl inside the Clam."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is a Blue Pearl inside the Clam."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is a Blood-Red Pearl inside the Clam."));
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013: // festive firecrackers
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100: // Firecracker
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200: // bag
								__OpenPrivateShop();
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301: // leadership training book
							case 50302:
							case 50303:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
										return false;
									}

									int32_t lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("It isn't easy to understand this book."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This book will not help you."));
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50304: // Linkage training book
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to have a minimum level of 30 to understand this book."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of 50 to understand this book."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can't train any more Combos."));
										return false;
									}

									int32_t iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311: // language training book
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									uint32_t dwSkillVnum = item->GetValue(0);
									int32_t iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already understand this language."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 : // Japanese horse summoning skill training book
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									uint32_t dwSkillVnum = item->GetValue(0);
									int32_t iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316: // transformation training book
							case 50323: case 50324: // blood pressure training book
							case 50325: case 50326: // iron training book
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change your status while you are transformed."));
										return false;
									}
									
									int32_t iSkillLevelLowLimit = item->GetValue(0);
									int32_t iSkillLevelHighLimit = item->GetValue(1);
									int32_t iPct = MINMAX(0, item->GetValue(2), 100);
									int32_t iLevelLimit = item->GetValue(3);
									uint32_t dwSkillVnum = 0;
									
									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to improve your Level to read this Book."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("It isn't easy to understand this book."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train with this Book any more."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									uint32_t dwSkillVnum = SKILL_CREATE;
									int32_t iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									uint32_t dwSkillVnum = SKILL_MINING;
									int32_t iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot train this skill."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::GetInstance()->RemoveItem(item);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read a book while you are transformed."));
										return false;
										
									}
									uint32_t dwSkillVnum = SKILL_HORSE;
									int32_t iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need a minimum level of 50 to get riding training."));
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											// Ignoring the time limit while using the magic spell
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have escaped the evil ghost curse with the help of an Exorcism Scroll."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 || 
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot read any more Riding Guides."));
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You read the Horse Riding Manual and received a Riding Point."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can use this point to improve your riding skill!"));
										PointChange(POINT_HORSE_SKILL, 1);

										int32_t iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You did not understand the riding guide."));
									}

									ITEM_MANAGER::GetInstance()->RemoveItem(item);
								}
								break;

							case 70102: // head
							case 70103: // head
								{
									if (GetAlignment() >= 0)
										return false;

									int32_t delta = MIN(-GetAlignment(), item->GetValue(0));

									PyLog("{} ALIGNMENT ITEM {}", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("Your mind is clear. You can concentrate really well now."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your rank has increased by %d points."), delta/10);
									}
								}
								break;

							case 71107: // nectarine peach
								{
									int32_t val = item->GetValue(0);
									int32_t interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::GetInstance()->GetPC(GetPlayerID());
									int32_t last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot be used yet."));
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The time limit on the test server has been exceeded."));
										}
									}
									
									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The rank points cannot be increased further."));
										return false;
									}
									
									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}
									
									int32_t old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);
									
									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("Your mind is clear. You can concentrate really well now."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your rank has increased by %d points."), val);

									char buf[256 + 1];
									snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
									LogManager::GetInstance()->CharLog(this, val, "MYTHICAL_PEACH", buf);
								}
								break;

							case 71109: // exit book
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging())
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch(item2->GetType())
									{
										case ITEM::TYPE_WEAPON:
											break;
										case ITEM::TYPE_ARMOR:
											switch (item2->GetSubType())
											{
											case ITEM::ARMOR_EAR:
											case ITEM::ARMOR_WRIST:
											case ITEM::ARMOR_NECK:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no stone available for removal."));
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<int32_t> socket;

									for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int32_t idx = ITEM::SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no stone available for removal."));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != nullptr)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)", 
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::GetInstance()->ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:
							case 70202:
							case 70203:
							case 70204:
							case 70205:
							case 70206:
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot dye or bleach your current hairstyle."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										auto q = quest::CQuestManager::GetInstance();
										quest::PC* pPC = q->GetPC(GetPlayerID());

										if (pPC)
										{
											int32_t last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need to have reached level %d to be able to dye your hair again."), last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									uint32_t dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <uint32_t> dwVnums;
									std::vector <uint32_t> dwCounts;
									std::vector <LPITEM> item_gets;
									int32_t count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int32_t i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), dwCounts[i]);
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									uint32_t dwBoxVnum = item->GetVnum();
									std::vector <uint32_t> dwVnums;
									std::vector <uint32_t> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int32_t count = 0;


									if (item->GetVnum() == ITEM_VALENTINE_ROSE && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_VALENTINE_CHOCOLATE && SEX_FEMALE==GET_SEX(this))
									{
										// Can't write because gender doesn't match.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item can only be opened by the another gender."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									uint32_t dwBoxVnum = item->GetVnum();
									std::vector <uint32_t> dwVnums;
									std::vector <uint32_t> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int32_t count = 0;


									if (item->GetVnum() == ITEM_WHITEDAY_CANDY && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_WHITEDAY_ROSE && SEX_FEMALE==GET_SEX(this))
									{
										// 성Can't write because gender doesn't match.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item can only be opened by the another gender."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case 50011: // moonlight
								{
									uint32_t dwBoxVnum = 50011;
									std::vector <uint32_t> dwVnums;
									std::vector <uint32_t> dwCounts;
									std::vector <LPITEM> item_gets(0);
									int32_t count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int32_t i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::GetInstance()->ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::GetInstance()->RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), dwCounts[i]);
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("A mysterious light comes out of the box."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d experience points."), dwCounts[i]);
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the red smoke coming out of the box, your speed will increase!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box suddenly exploded! You have lost Hit Points (HP)."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("If you inhale the green smoke that is coming out of the box, the poison will spread through your body!"));
												break;

											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Look what came out of the box!"));
												break;

											default:
												if (item_gets[i])
												{
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Receive: %s - %d"), item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The box contains %s."), item_gets[i]->GetName());
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("You have not received anything."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
									EffectPacket(SE_CHINA_FIREWORK);
									// Increases stun attack
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
										return false;
									}

									EffectPacket(SE_SPIN_TOP);
									// Increases stun attack
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM::ELK_VNUM: // bundle of money
								{
									int32_t iGold = item->GetSocket(0);
									ITEM_MANAGER::GetInstance()->RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have received %d Yang."), iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;

							//mark of the monarch
							case 70021:
								{
									int32_t HealPrice = quest::CQuestManager::GetInstance()->GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::GetInstance()->HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), LC_TEXT("When the Blessing of the Emperor is used %s the HP and SP are restored again."), EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);
										
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Emperor Blessing is activated."));
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 : //Temporary transformation dismantling unit
								{
									if (m_pChrTarget != nullptr)
									{
										if (m_pChrTarget->IsPolymorphed())
										{
											m_pChrTarget->SetPolymorph(0);
											m_pChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

							case 71051:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging())
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
										return false;
									}
									
									if (item2->AddRareAttribute())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Upgrade successfully added."));

										int32_t iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										LogManager::GetInstance()->ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No upgrade added."));
									}
								}
								break;

							case 71052:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging())
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
										return false;
									}

									if (item2->ChangeRareAttribute())
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::GetInstance()->ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Could not change the upgrade of this item."));
									}
								}
								break;

							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS: 
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S: 
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS: 
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
									if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
										return false;
									}

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int32_t pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There isn't enough space in your inventory."));
											break;
										}

										item->SetCount(item->GetCount() - 1);

										LPITEM item2 = ITEM_MANAGER::GetInstance()->CreateItem(item->GetVnum(), 1);
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect(type);

									if (!pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess(type);
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect(pAffect);

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID(pAffect->dwFlag);

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect(pAffect);

											EPointTypes bonus = POINT_NONE;

											if (isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect(type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess(type);
										}
									}
								}
								break;
						}
						break;

					case ITEM::USE_CLEAR:
						{
							RemoveBadAffect();
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case ITEM::USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								auto q = quest::CQuestManager::GetInstance();
								quest::PC* pPC = q->GetPC(GetPlayerID());

								if (pPC != nullptr)
								{
									int32_t last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot be used yet."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case ITEM::USE_POTION_NODELAY:
						{
							if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
							{
								if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("That is over the limit."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
										return false;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0)
							{
								if (GetHP() < GetMaxHP()) // HP
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP %
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP %
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Used Moon Cake or Seed."));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case ITEM::USE_POTION:
						if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
						{
							if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
								return false;
							}
						
							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::GetInstance()->GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("That is over the limit."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
									return false;
							}
						}
						
						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case ITEM::USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case ITEM::USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case ITEM::APPLY_MOV_SPEED:
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_ATT_SPEED:
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_STR:
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_DEX:
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_CON:
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_INT:
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_CAST_SPEED:
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS, 
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case ITEM::APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case ITEM::USE_TALISMAN:
						{
							const int32_t TOWN_PORTAL	= 1;
							const int32_t MEMORY_PORTAL = 2;


							// gm_guild_build, disables the return memory in the oxevent map
							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this from your current position."));
								return false;
							}

							if (CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item in a duel."));
								return false;
							}

							if (m_pWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are ready to warp, so you cannot use the Scroll of the Location."));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int32_t consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL) // return
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										SysLog("CHARACTER::UseItem : cannot find spawn position (name {}, {} x {})", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are being brought back to the place of origin."));	

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL) // return memory
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s%s cannot be used in a dungeon."),
												item->GetName(),
												"");
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case ITEM::USE_TUNING:
					case ITEM::USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging())
								return false;
	
							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343) // Spirit stone +3
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("+3 Spirit Stones cannot be upgraded with this item."));
								return false;
							}
							
							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)  // Spirit stone +4
							{
								if (item->GetVnum() == 71056) // Blue Dragon's Breath
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Spirit Stones cannot be upgraded with this item."));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES

					case ITEM::USE_PUT_INTO_BELT_SOCKET:
					case ITEM::USE_PUT_INTO_RING_SOCKET:
					case ITEM::USE_PUT_INTO_ACCESSORY_SOCKET:
					case ITEM::USE_ADD_ACCESSORY_SOCKET:
					case ITEM::USE_CLEAN_SOCKET:
					case ITEM::USE_CHANGE_ATTRIBUTE:
					case ITEM::USE_CHANGE_ATTRIBUTE2 :
					case ITEM::USE_ADD_ATTRIBUTE:
					case ITEM::USE_ADD_ATTRIBUTE2:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							// [NOTE] There was a request to give random properties to costume items when the item is first created, but to prevent financial costs, etc.
							// Originally, it was planned to add an item flag such as ANTI_CHANGE_ATTRIBUTE so that it can be controlled flexibly at the planning level.
							// I don't need anything like that, so shut up and get it done quickly, so I'm just stopping here... -_-
							if (ITEM::TYPE_COSTUME == item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
								return false;
							}

							if (item2->IsExchanging())
								return false;

							switch (item->GetSubType())
							{
								case ITEM::USE_CLEAN_SOCKET:
									{
										int32_t i;
										for (i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM::SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There aren't any Pieces of Broken Stone available for removal."));
											return false;
										}

										int32_t j = 0;

										for (i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM::SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::GetInstance()->ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case ITEM::USE_CHANGE_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
										return false;
									}

									if (item2->GetAttributeCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There is no upgrade that you can change."));
										return false;
									}

									if (GM_PLAYER == GetGMLevel() && !test_server)
									{
										//
										// Check whether sufficient time has elapsed from the time the item property was changed previously through Event Flag
										// If enough time has elapsed, set the time for the current property change.
										//

										uint32_t dwChangeItemAttrCycle = quest::CQuestManager::GetInstance()->GetEventFlag(msc_szChangeItemAttrCycleFlag);
										if (dwChangeItemAttrCycle < msc_dwDefaultChangeItemAttrCycle)
											dwChangeItemAttrCycle = msc_dwDefaultChangeItemAttrCycle;

										quest::PC* pPC = quest::CQuestManager::GetInstance()->GetPC(GetPlayerID());

										if (pPC)
										{
											uint32_t dwNowMin = get_global_time() / 60;

											uint32_t dwLastChangeItemAttrMin = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

											if (dwLastChangeItemAttrMin + dwChangeItemAttrCycle > dwNowMin)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only do this %d minutes after an upgrade. (%d minutes left)"),
														dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowMin - dwLastChangeItemAttrMin));
												return false;
											}

											pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowMin);
										}
									}

									if (item->GetSubType() == ITEM::USE_CHANGE_ATTRIBUTE2)
									{
										int32_t aiChangeProb[ITEM::ATTRIBUTE_MAX_LEVEL] = 
										{
											0, 0, 30, 40, 3
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else if (item->GetVnum() == 76014)
									{
										int32_t aiChangeProb[ITEM::ATTRIBUTE_MAX_LEVEL] = 
										{
											0, 10, 50, 39, 1
										};

										item2->ChangeAttribute(aiChangeProb);
									}

									else
									{
										// Special handling of Yeonjae-kyung
										// Hard-coded because serialization will never be added.
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
											if ((item2->GetType() == ITEM::TYPE_WEAPON)
												|| (item2->GetType() == ITEM::TYPE_ARMOR && item2->GetSubType() == ITEM::ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == ITEM::LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (!bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The level of your item is too high."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Can only be used on weapons and armor."));
												break;
											}
										}
										item2->ChangeAttribute();
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have changed the upgrade."));
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::GetInstance()->ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
									}

									item->SetCount(item->GetCount() - 1);
									break;

								case ITEM::USE_ADD_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
										return false;
									}

									if (item2->GetAttributeCount() < 4)
									{
										// Special handling of serialization
										// Hard-coded because serialization will never be added.
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
											if ((item2->GetType() == ITEM::TYPE_WEAPON)
												|| (item2->GetType() == ITEM::TYPE_ARMOR && item2->GetSubType() == ITEM::ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == ITEM::LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (!bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The level of your item is too high."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Can only be used on weapons and armor."));
												break;
											}
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Upgrade successfully added."));

											int32_t iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::GetInstance()->ItemLog(
													GetPlayerID(), 
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(), 
													"ADD_ATTRIBUTE_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No upgrade added."));
											LogManager::GetInstance()->ItemLog(this, item, "ADD_ATTRIBUTE_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You must use the Blessing Marble in order to add another bonus to this item."));
									}
									break;

								case ITEM::USE_ADD_ATTRIBUTE2 :
									// Blessing Orb
									// Add one more attribute to the item to which 4 attributes have been added through the home secretary.
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the upgrade of this item."));
										return false;
									}

									// Attributes can be added only when 4 attributes have already been added.
									if (item2->GetAttributeCount() == 4)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Upgrade successfully added."));

											int32_t iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::GetInstance()->ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE2_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No upgrade added."));
											LogManager::GetInstance()->ItemLog(this, item, "ADD_ATTRIBUTE2_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item can no longer be improved. The maximum number of bonuses has been reached."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can only use the Blessing Marble with an item which already has 4 bonuses."));
									}
									else
									{
										// wtf ?!
										SysLog("ADD_ATTRIBUTE2 : Item has wrong AttributeCount({})", item2->GetAttributeCount());
									}
									break;

								case ITEM::USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												if (number(1, 100) <= 50)
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Socket successfully added."));
													LogManager::GetInstance()->ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No socket added."));
													LogManager::GetInstance()->ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No additional sockets could be added to this item."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot add a socket to this item."));
										}
									}
									break;

								case ITEM::USE_PUT_INTO_BELT_SOCKET:
								case ITEM::USE_PUT_INTO_ACCESSORY_SOCKET:
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
										{
											if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
											{
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Arming successful."));
												LogManager::GetInstance()->ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Arming has failed."));
												LogManager::GetInstance()->ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
											}

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to add a socket first. Use a diamond in order to do this."));
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("There are no sockets for gemstones in this item."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to add a socket if you want to use a Diamond."));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No more gems can be added to this item."));
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("These items cannot be used together."));
									}
									break;
							}
							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case ITEM::USE_BAIT:
						{

							if (m_pFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the Bait whilst fishing."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM::TYPE_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are exchanging the current Bait for %s."), item->GetName());
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You attached %s to the hook as bait."), item->GetName());
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case ITEM::USE_MOVE:
					case ITEM::USE_TREASURE_BOX:
					case ITEM::USE_MONEYBAG:
						break;

					case ITEM::USE_AFFECT :
						{
							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
							}
							else
							{
								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case ITEM::USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					// Process recipe for potion making skill
					case ITEM::USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							uint32_t dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							uint32_t dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == nullptr)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are missing some ingredients to make the potion."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == nullptr)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are missing some ingredients to make the potion."));
									return false;
								}
							}

							if (pSource1 != nullptr)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough material (%s)."), pSource1->GetName());
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != nullptr)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough material (%s)."), pSource2->GetName());
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You do not have enough empty bottles."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The potion production has failed."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM::TYPE_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() == ITEM::TYPE_PICK) return false;
				if (item2->GetType() == ITEM::TYPE_ROD) return false;

				int32_t i;

				for (i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
				{
					uint32_t dwVnum;   

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable* p = ITEM_MANAGER::GetInstance()->GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot attach several stones of the same type."));
						return false;
					}
				}

				if (item2->GetType() == ITEM::TYPE_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), ITEM::WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), ITEM::WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This Spirit Stone cannot be attached to this type of item."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM::TYPE_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), ITEM::WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot attach this Spirit Stone to a weapon."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No slot free."));
					return false;
				}

				for (i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
						// seat odds
						if (number(1, 100) <= 30)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have attached the Spirit Stone successfully."));
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The Spirit Stone broke while being attached."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::GetInstance()->ItemLog(this, item2, "SOCKET", item->GetName());
						ITEM_MANAGER::GetInstance()->RemoveItem(item, "REMOVE (METIN)");
						break;
					}

				if (i == ITEM::SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("No slot free."));
			}
			break;

		case ITEM::TYPE_AUTOUSE:
		case ITEM::TYPE_MATERIAL:
		case ITEM::TYPE_SPECIAL:
		case ITEM::TYPE_TOOL:
		case ITEM::TYPE_LOTTERY:
			break;

		case ITEM::TYPE_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM::TYPE_BLEND:
			// new herbs
			PyLog("ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int32_t		affect_type		= AFFECT_BLEND;
				if (item->GetSocket(0) >= _countof(aApplyInfo))
				{
					SysLog("INVALID BLEND ITEM(id : {}, vnum : {}). APPLY TYPE IS {}.", item->GetID(), item->GetVnum(), item->GetSocket(0));
					return false;
				}
				int32_t		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int32_t		apply_value		= item->GetSocket(1);
				int32_t		apply_duration	= item->GetSocket(2);
				
				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This effect is already activated."));
					}
					else
					{
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
					}
				}
			}
			break;
		case ITEM::TYPE_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (!pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case ITEM::EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::GetInstance()->PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case ITEM::EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::GetInstance()->ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM::TYPE_NONE:
			SysLog("Item type NONE {}", item->GetName());
			break;

		default:
			PyLog("UseItemEx: Unknown type {} {}", item->GetName(), item->GetType());
			return false;
	}

	return true;
}

int32_t g_nPortalLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	uint16_t wCell = Cell.cell;
	uint8_t window_type = Cell.window_type;
	uint16_t wDestCell = DestCell.cell;
	uint8_t bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
			return false;

	PyLog("{}: USE_ITEM {} (inven {}, cell: {})", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item because you do not fulfil all the requirements."));
		return false;
	}

	if (IsStun())
		return false;

	if (!FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are not able to use that item because you do not have the right gender."));
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (!IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This function is not available right now."));
			return false;
		}

		// WtpToPC() checks whether the other party is in SUMMONABLE_ZONE when using a wedding ring
		
		//In the three-way control map, it blocks the return unit.
		if (CThreeWayWar::GetInstance()->IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use a Scroll of the Location whilst taking part in a kingdom battle."));
			return false;
		}
		int32_t iPulse = thecore_pulse();

		// Check after warehouse opening
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After opening the Storeroom you cannot use a Scroll of the Location for %d seconds."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false; 
		}

		// Check transaction related window
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use a Scroll of the Location while another window is open."));
			return false;
		}

		//PREVENT_REFINE_HACK
		// Time check after upgrade
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After a trade, you cannot use a scroll for another %d seconds."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK
		

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After opening a storeroom you cannot use a Scroll of the Location for another %d seconds."), g_nPortalLimitTime);
				return false;
			}
			
		}
		//END_PREVENT_ITEM_COPY
		

		//Return distance check
		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int32_t x = 0;
			int32_t y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;
			//return memory
			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}
			//return
			else if (item->GetVnum() == 22000) 
			{
				SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use the Scroll of the Location because the distance is too small."));				
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant {} nNowDist {}", nDistant,nDist); 
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE
		//교환 후 시간체크
		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("After a trade you cannot use a Scroll of the Location for %d seconds."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	//Check limit of transaction window when using bundle silk
	if (item->GetVnum() == 50200 | item->GetVnum() == 71049)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot open the Storeroom if another window is already open."));
			return false;
		}

	}
	//END_PREVENT_TRADE_WINDOW

	if (IS_SET(item->GetFlag(), ITEM::FLAG_LOG)) // Handling items that leave a usage log
	{
		uint32_t vid = item->GetVID();
		uint32_t oldCount = item->GetCount();
		uint32_t vnum = item->GetVnum();

		char hint[ITEM::NAME_MAX_LEN + 32 + 1];
		int32_t len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int32_t) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);

		if (!ITEM_MANAGER::GetInstance()->FindByVID(vid)) // UseItemEx The item has been deleted from Leave a delete log
		{
			LogManager::GetInstance()->ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::GetInstance()->ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

bool CHARACTER::DropItem(TItemPos Cell, uint8_t bCount)
{
	LPITEM item = nullptr; 

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot move the item within the refinement window."));
		return false;
	}

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->isLocked())
		return false;

	if (quest::CQuestManager::GetInstance()->GetPCForce(GetPlayerID())->IsRunning())
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_DROP | ITEM::ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot drop this item."));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);	// Quickslot erased from

	LPITEM pItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				PyLog("[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::GetInstance()->FlushDelayedSave(item);

		pItemToDrop = ITEM_MANAGER::GetInstance()->CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pItemToDrop->GetID(), pItemToDrop->GetCount());
		LogManager::GetInstance()->ItemLog(this, item, "ITEM_SPLIT", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		// There are many genuine users in Korea who throw away items and want to restore them.
		// When an item is thrown on the floor, an attribute log is left.
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The dropped item will vanish in 5 minutes."));
		pItemToDrop->StartDestroyEvent();

		ITEM_MANAGER::GetInstance()->FlushDelayedSave(pItemToDrop);
		
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pItemToDrop->GetName(), pItemToDrop->GetCount(), pItemToDrop->GetOriginalVnum());
		LogManager::GetInstance()->ItemLog(this, pItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

bool CHARACTER::DropGold(int32_t gold)
{
	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

	if (0 != g_GoldDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastGoldDropTime+g_GoldDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot throw the Gold away yet."));
			return false;
		}
	}

	m_dwLastGoldDropTime = get_dword_time();

	LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			//Motion(MOTION_PICKUP);
			PointChange(POINT_GOLD, -gold, true);

			// there is a bug that money disappears in Brazil,
			// One of the possible scenarios is,
			// Use macros or hacks to keep throwing money under 1000 won to make gold 0,
			// It may be a request for recovery because the money is gone.
			// So log the low gold to catch such cases.
			if (gold > 1000) // 천원 이상만 기록한다.
				LogManager::GetInstance()->CharLog(this, gold, "DROP_GOLD", "");

			item->StartDestroyEvent(60);
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The dropped item will disappear after %d minutes."), 3);
		}

		Save();
		return true;
	}

	return false;
}

bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, uint8_t count)
{
	LPITEM item = nullptr;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM::FLAG_IRREMOVABLE))
		return false;

	if (item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot move the item within the refinement window."));
		return false;
	}

	// 기At the request of the creator, only certain types of items can be placed in the belt inventory.
	if (DestCell.IsBeltInventoryPosition() && !CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot equip this item in your belt inventory."));			
		return false;
	}

	// When moving an item that is already worn to another location, check if it is possible to 'Remove the Book' and move it
	if (Cell.IsEquipPosition() && !CanUnequipNow(item))
		return false;

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))	// In the case of equipment, only one location may be inspected.
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have already equipped this kind of Dragon Stone."));
			
			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::GetInstance()->PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::GetInstance()->IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}
		// Items that are not dragon soul stones cannot be placed in the dragon soul stone inventory.
		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM::ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // For items that can be combined
		{
			for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			PyLog("{}: ITEM_STACK {} (window: {}, cell : {}) -> (window:{}, cell {}) count {}", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			count = MIN(200 - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_STACK))
		{
			PyLog("{}: ITEM_MOVE {} (window: {}, cell : {}) -> (window:{}, cell {}) count {}", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);
			
			item->RemoveFromCharacter();
			SetItem(DestCell, item);

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			PyLog("{}: ITEM_SPLIT {} (window: {}, cell : {}) -> (window:{}, cell {}) count {}", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::GetInstance()->CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::GetInstance()->ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item) 
			: item(item), owner(nullptr)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int32_t		total;
		int32_t		x, y;

		FCountNearMember(LPCHARACTER center)
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int32_t		total;
		LPCHARACTER	c;
		int32_t		x, y;
		int32_t		iMoney;

		FMoneyDistributor(LPCHARACTER center, int32_t iMoney) 
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney) 
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);

					if (iMoney > 1000) // Records over 1,000 won.
						LogManager::GetInstance()->CharLog(ch, iMoney, "GET_GOLD", "");
				}
		}
	};
}

void CHARACTER::GiveGold(int32_t iAmount)
{
	if (iAmount <= 0)
		return;

	PyLog("GIVE_GOLD: {} {}", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		// If there is a party, share it.
		uint32_t dwTotal = iAmount;
		uint32_t dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			uint32_t dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

		if (dwMyAmount > 1000) // Record more than 1,000 won.
			LogManager::GetInstance()->CharLog(this, dwMyAmount, "GET_GOLD", "");
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);

		// there is a bug that money disappears in Brazil,
		// One of the possible scenarios is,
		// Use macros or hacks to keep throwing money under 1000 won to make gold 0,
		// It may be a request for recovery because the money is gone.
		// So log the low gold to catch such cases.
		if (iAmount > 1000) // Record more than 1,000 won.
			LogManager::GetInstance()->CharLog(this, iAmount, "GET_GOLD", "");
	}
}

bool CHARACTER::PickupItem(uint32_t dwVID)
{
	LPITEM item = ITEM_MANAGER::GetInstance()->FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		if (item->IsOwnership(this))
		{
			// 만If the item you are about to give is an elk
			if (item->GetType() == ITEM::TYPE_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}
			//If it's an ordinary item
			else
			{
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_STACK))
				{
					uint8_t bCount = item->GetCount();

					for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int32_t j;

							for (j = 0; j < ITEM::SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM::SOCKET_MAX_NUM)
								continue;

							uint8_t bCount2 = MIN(200 - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), item2->GetName());
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM::TYPE_QUEST)
									quest::CQuestManager::GetInstance()->PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}

				int32_t iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						PyLog("No empty ds inventory pid {} size {}d itemid {}", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have too many items in your inventory."));
						return false;
					}
				}
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						PyLog("No empty inventory pid {} size {}d itemid {}", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have too many items in your inventory."));
						return false;
					}
				}

				item->RemoveFromGround();
				
				if (item->IsDragonSoul())
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
				else
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::GetInstance()->ItemLog(this, item, "GET", szHint);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), item->GetName());

				if (item->GetType() == ITEM::TYPE_QUEST)
					quest::CQuestManager::GetInstance()->PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_GIVE | ITEM::ANTIFLAG_DROP) && GetParty())
		{
			// If you want to give ownership items to other party members
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;

			int32_t iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have too many items in your inventory."));
						return false;
					}
				}
			}
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have too many items in your inventory."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
			else
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::GetInstance()->ItemLog(owner, item, "GET", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), item->GetName());
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s receives %s."), GetName(), item->GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Item Trade: %s, %s"), owner->GetName(), item->GetName());
			}

			if (item->GetType() == ITEM::TYPE_QUEST)
				quest::CQuestManager::GetInstance()->PickupItem (owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(uint8_t bCell, uint8_t bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	// Check if the cell is correct
	// Dragon Soul Stone cannot be swapped, so it gets stuck here.
	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	// Same CELL Cognitive Test
	if (bCell == bDestCell)
		return false;

	// If both are located in the equipment window, swapping is not possible.
	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	// so that item2 is in the equipment window.
	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;
	
	if (item1 == item2)
	{
	    PyLog("[WARNING][WARNING][HACK USER!] : {} {} {}", m_stName.c_str(), bCell, bDestCell);
	    return false;
	}

	// Check if item2 can enter the bCell location.
	if (!IsEmptyItemGrid(TItemPos (INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	// If the item to be replaced is in the equipment window
	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		uint8_t bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		uint8_t bInvenCell = item1->GetCell();

		// You can take off the item you are wearing and proceed only when the item to be worn can be worn
		if (!CanUnequipNow(item2) || !CanEquipNow(item1))
			return false;

		if (bEquipCell != item1->FindEquipCell(this)) // Only allowed when in the same location
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
		else
			SysLog("SwapItem cannot equip {}! item1 {}", item2->GetName(), item1->GetName());
	}
	else
	{
		uint8_t bCell1 = item1->GetCell();
		uint8_t bCell2 = item2->GetCell();
		
		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
	int32_t pos;

	if (!CanUnequipNow(item))
		return false;

	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
	}
	else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));

	CheckMaximumPoints();

	return true;
}

//
// @version	05/07/05 Bang2ni - Do not wear equipment within 1.5 seconds of using a skill
//
bool CHARACTER::EquipItem(LPITEM item, int32_t iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (!item->IsEquipable())
		return false;

	if (!CanEquipNow(item))
		return false;

	int32_t iWearCell = item->FindEquipCell(this, iCandidateCell);

	if (iWearCell < 0)
		return false;

	//Do not wear a tuxedo while riding something
	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use it while riding a Horse."));
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot change the equipped item while you are transformed."));
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are not able to use that item because you do not have the right gender."));
		return false;
	}

	//When using a new mount, check whether an existing horse is used
	if(item->IsRideItem() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You're already riding. Get off first."));
		return false;
	}

	// Except for arrows, equipment can be changed after the last attack time or 1.5 after using the skill
	uint32_t dwCurTime = get_dword_time();

	if (iWearCell != WEAR_ARROW 
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have to stand still to equip the item."));
		return false;
	}

	// Dragon Soul Stone Special Treatment
	if (item->IsDragonSoul())
	{
		// If a dragon soul stone of the same type is already included, it cannot be worn.
		// Dragon Soul Stone must not support swap.
		if(GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "You are already carrying a Dragon Stone of this type.");
			return false;
		}
		
		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	// It's not a dragon stone.
	else
	{
		// If there is an item to wear,
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM::FLAG_IRREMOVABLE))
		{
			// Once this item is embedded, it cannot be changed. Swap is also completely impossible.
			if (item->GetWearFlag() == ITEM::WEARABLE_ABILITY) 
				return false;

			if (!SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			uint8_t bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (item->IsEquipped())
	{
		// After the first use of the item, the time is deducted even if it is not used.
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			// Whether an item has been used at least once is judged by looking at Socket1. (Record the number of uses in Socket1)
			if (0 == item->GetSocket(1))
			{
				// For the available time, use the Limit Value as the default value, but if there is a value in Socket0, use that value. (unit is seconds)
				int32_t duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[item->GetProto()->cLimitRealTimeFirstUseIndex].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const uint32_t& dwVnum = item->GetVnum();

		// Ramadan Event Effect is activated when Crescent Ring (71135) is worn
		if (CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}
		// Effect activates when wearing Halloween Candy (71136)
		else if (CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
		// Effect activates when wearing Ring of Happiness (71143)
		else if (CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		// Effect activates when wearing Pendant of Love (71145)
		else if (CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		//In case of ITEM_UNIQUE, it is defined in SpecialItemGroup, (item->GetSIGVnum() != nullptr)
		// 
		else if (ITEM::TYPE_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::GetInstance()->GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::GetInstance()->GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}

		if (ITEM::UNIQUE_SPECIAL_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM::FLAG_QUEST_USE))
		{
			quest::CQuestManager::GetInstance()->UseItem(GetPlayerID(), item, false);
		}
	}

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (int32_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (int32_t i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(uint8_t bType, uint8_t bOldValue, uint8_t bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_COSTUME_ATTR_BONUS:
				{
					static uint8_t abSlot[] = { WEAR_COSTUME_BODY, WEAR_COSTUME_HAIR };
					static std::vector <uint8_t> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));

		}
		else
			pBuff = it->second;
			
		pBuff->On(bNewValue);
	}
	else
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->ChangeBuffValue(bNewValue);
	}
}


LPITEM CHARACTER::FindSpecifyItem(uint32_t vnum) const
{
	for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

	return NULL;
}

LPITEM CHARACTER::FindItemByID(uint32_t id) const
{
	for (int32_t i=0 ; i < INVENTORY_MAX_NUM ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int32_t i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int32_t CHARACTER::CountSpecifyItem(uint32_t vnum) const
{
	int32_t	count = 0;
	LPITEM item;

	for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			// If it is an item registered in a personal store, it will be skipped.
			if (m_pMyShop && m_pMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyItem(uint32_t vnum, uint32_t count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (!GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		// If it is an item registered in a personal store, it will be skipped. 
		// (It is a problem if you enter this part when it is sold in a private store!)
		if(m_pMyShop)
		{
			bool isItemSelling = m_pMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (vnum >= 80003 && vnum <= 80007)
			LogManager::GetInstance()->GoldBarLog(GetPlayerID(), GetInventoryItem(i)->GetID(), QUEST, "RemoveSpecifyItem");

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}

	// Exception handling is weak.
	if (count)
		PyLog("CHARACTER::RemoveSpecifyItem cannot remove enough item vnum {}, still remain {}", vnum, count);
}

int32_t CHARACTER::CountSpecifyTypeItem(uint8_t type) const
{
	int32_t	count = 0;

	for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != nullptr && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(uint8_t type, uint32_t count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (!GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		// If it is an item registered in a personal store, it will be skipped. 
		// (It is a problem if you enter this part when it is sold in a private store!)
		if(m_pMyShop)
		{
			bool isItemSelling = m_pMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (!item)
	{
		SysLog("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		SysLog("item {} 's owner exists!",item->GetID());
		return;
	}
	
	int32_t cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
	else
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		LogManager::GetInstance()->ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM::TYPE_USE && item->GetSubType() == ITEM::USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);
		LogManager::GetInstance()->ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}
}

LPITEM CHARACTER::AutoGiveItem(uint32_t dwItemVnum, uint8_t bCount, int32_t iRarePct, bool bMsg)
{
	TItemTable* p = ITEM_MANAGER::GetInstance()->GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM::FLAG_STACKABLE && p->bType != ITEM::TYPE_BLEND) 
	{
		for (int32_t i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM::FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				uint8_t bCount2 = MIN(200 - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), item->GetName());

					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::GetInstance()->CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		SysLog("cannot create item by vnum {} (name: {})", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM::TYPE_BLEND)
	{
		for (int32_t i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == nullptr) continue;

			if (inv_item->GetType() == ITEM::TYPE_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
							inv_item->GetSocket(2) == item->GetSocket(2) &&
							inv_item->GetCount() < ITEM::MAX_COUNT)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						return inv_item;
					}
				}
			}
		}
	}

	int32_t iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s received"), item->GetName());

		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
		LogManager::GetInstance()->ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM::TYPE_USE && item->GetSubType() == ITEM::USE_POTION)
		{
			TQuickslot* pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();
		// For items with anti-drop flag,
		// If there is no empty space in the inventory and you have to drop it,
		// Maintain ownership until the item disappears (300 seconds).
		if (IS_SET(item->GetAntiFlag(), ITEM::ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
		LogManager::GetInstance()->ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM::TYPE_FISH && 
					(item->GetSubType() == ITEM::FISH_ALIVE || item->GetSubType() == ITEM::FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM::TYPE_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM::TYPE_WEAPON && 
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
			if (item->GetType() == ITEM::TYPE_ARMOR && 
					(item->GetSubType() == ITEM::ARMOR_BODY || item->GetSubType() == ITEM::ARMOR_SHIELD || item->GetSubType() == ITEM::ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetType() == ITEM::TYPE_ARMOR &&
					!(item->GetSubType() == ITEM::ARMOR_BODY || item->GetSubType() == ITEM::ARMOR_SHIELD || item->GetSubType() == ITEM::ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:
			// beginner horse
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Monkey Herbs cannot be fed to living horses. It is used to revive dead horses."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot feed a dead Horse."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:
			// intermediate horse
			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Monkey Herbs cannot be fed to living horses. It is used to revive dead horses."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot feed a dead Horse."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:
			// advanced horse
			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Monkey Herbs cannot be fed to living horses. It is used to revive dead horses."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot feed a dead Horse."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM::TYPE_FISH && (item->GetSubType() == ITEM::FISH_ALIVE || item->GetSubType() == ITEM::FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::GetInstance()->TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC 
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), ITEM::REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), ITEM::REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This item cannot be improved."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You fed the Horse with Herbs."));
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have fed the Horse."));
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			PyLog("TakeItem {} {} {}", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::GetInstance()->TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(uint32_t dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	// If it is a language ring, check whether it is a language ring (sample).
	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(uint32_t dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int32_t) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int32_t) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int32_t iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC(nullptr);
}

bool CHARACTER::GiveItemFromSpecialItemGroup(uint32_t dwGroupNum, std::vector<uint32_t> &dwItemVnums, 
											std::vector<uint32_t> &dwItemCounts, std::vector <LPITEM> &item_gets, int32_t &count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::GetInstance()->GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		SysLog("cannot find special item group {}", dwGroupNum);
		return false;
	}

	std::vector <int32_t> idxes;
	int32_t n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int32_t i = 0; i < n; i++)
	{
		bSuccess = false;
		int32_t idx = idxes[i];
		uint32_t dwVnum = pGroup->GetVnum(idx);
		uint32_t dwCount = pGroup->GetCount(idx);
		int32_t	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = nullptr;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, dwCount);
				LogManager::GetInstance()->CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::GetInstance()->CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					PyLog("CSpecialItemGroup::MOB {}", dwCount);
					int32_t x = GetX() + number(-500, 500);
					int32_t y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					PyLog("CSpecialItemGroup::SLOW {}", -(int32_t)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int32_t)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int32_t iDropHP = GetMaxHP()*dwCount/100;
					PyLog("CSpecialItemGroup::DRAIN_HP {}", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					PyLog("CSpecialItemGroup::DRAIN_HP {}", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(nullptr);
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB_GROUP:
				{
					int32_t sx = GetX() - number(300, 500);
					int32_t sy = GetY() - number(300, 500);
					int32_t ex = GetX() + number(300, 500);
					int32_t ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::GetInstance()->SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;
			default:
				{
					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

					if (item_get)
					{
						bSuccess = true;
					}
				}
				break;
		}
	
		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int32_t iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		// level capped
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your level is too low to wear this Hairstyle."));
		return false;
	}

	uint32_t hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000; // 73001 - 72000 = Hair number starts from 1001
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;

		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You already have this Hairstyle."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have already transformed."));
		return false;
	}

	if (IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot transform while you are riding."));
		return false;
	}

	uint32_t dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("That's the wrong trading item."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::GetInstance()->Get(dwVnum);

	if (pMob == nullptr)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("That's the wrong trading item."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				// processing of armor
				PyLog("USE_POLYMORPH_BALL PID({}) vnum({})", GetPlayerID(), dwVnum);

				// level limit check
				int32_t iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot transform into a monster who has a higher level than you."));
					return false;
				}

				int32_t iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				uint32_t dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID) / 40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);
				
				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				// Hold

				// handle disguise
				// socket 0 socket 1 socket 2
				// Monster number to be transformed into Training level
				PyLog("USE_POLYMORPH_BOOK: {}({}) vnum({})", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::GetInstance()->PolymorphCharacter(this, item, pMob))
				{
					CPolymorphUtils::GetInstance()->UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			SysLog("POLYMORPH invalid item passed PID({}) vnum({})", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;

	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);

	if(NULL != Unique1)
	{
		if(UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup())
		{
			return UnequipItem(Unique1);
		}
	}

	if(NULL != Unique2)
	{
		if(UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup())
		{
			return UnequipItem(Unique2);
		}
	}

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (IsDead() || IsStun())
		return;

	if (!IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const uint32_t stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(uint32_t) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && pItem->GetSocket(0))
		{
			if (!CArenaManager::GetInstance()->IsArenaMap(GetMapIndex()))
			{
				const int32_t amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const int32_t amount_of_full = pItem->GetSocket(idx_of_amount_of_full);
				
				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
					if (avail > amount)
					{
						const int32_t pct_of_used = amount_of_used * 100 / amount_of_full;
						const int32_t pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;
						// Log in units of 10% of usage
						// (In % of usage, log each time the tens place changes.)
						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::GetInstance()->RemoveItem(pItem);
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange(POINT_HP_RECOVERY, amount);
						EffectPacket(SE_AUTO_HPUP);
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange(POINT_SP_RECOVERY, amount);
						EffectPacket(SE_AUTO_SPUP);
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}
		else
		{
			RemoveAffect(const_cast<CAffect*>(pAffect));
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	uint8_t window_type = Pos.window_type;
	uint16_t cell = Pos.cell;
	
	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (ITEM::DRAGON_SOUL_INVENTORY_MAX_NUM);

	case SAFEBOX:
		if (NULL != m_pSafebox)
			return m_pSafebox->IsValidPosition(cell);
		else
			return false;

	default:
		return false;
	}
}


// Macro I made because I was bothered.. A macro that outputs msg and returns false if exp is true (it may be confusing because of the name, as it is slightly opposite to the normal use of verify because of return..)
#define VERIFY_MSG(exp, msg)  \
	if ((exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

		
/// A function that checks whether the given item can be worn based on the current character's state, and if not, informs the character of the reason
bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	uint8_t itemType = item->GetType();
	uint8_t itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM::ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM::ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM::ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM::ANTIFLAG_SURA)
				return false;
			break;
	}

	for (int32_t i = 0; i < ITEM::LIMIT_SLOT_MAX_NUM; ++i)
	{
		int32_t limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case ITEM::LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your level is too low to equip this item."));
					return false;
				}
				break;

			case ITEM::LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are not strong enough to equip yourself with this item."));
					return false;
				}
				break;

			case ITEM::LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your intelligence is too low to equip yourself with this item."));
					return false;
				}
				break;

			case ITEM::LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your dexterity is too low to equip yourself with this item."));
					return false;
				}
				break;

			case ITEM::LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Your vitality is too low to equip yourself with this item."));
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & ITEM::WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot equip this item twice."));
			return false;
		}

		if (marriage::CManager::GetInstance()->IsMarriageUniqueItem(item->GetVnum()) && 
			!marriage::CManager::GetInstance()->IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this item because you are not married."));
			return false;
		}

	}

	return true;
}

/// A function that checks whether the item being worn can be taken off based on the current character's state, and if not, informs the character of the reason
bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{	

	if (ITEM::TYPE_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "You can only discard the belt when there are no longer any items in its inventory.");

	// Items that can't be unlocked forever
	if (IS_SET(item->GetFlag(), ITEM::FLAG_IRREMOVABLE))
		return false;

	// 아Check if there is an empty space when moving an item to the inventory when unequip
	{
		int32_t pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG(-1 == pos, "There isn't enough space in your inventory.");
	}


	return true;
}
