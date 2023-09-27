#include "stdafx.h"
#include <Core/Attribute.h>
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "desc_manager.h"
#include "packet.h"

SECTREE::SECTREE()
{
	Initialize();
}

SECTREE::~SECTREE()
{
	Destroy();
}

void SECTREE::Initialize()
{
	m_id.package = 0;
	m_pAttribute = nullptr;
	m_iPCCount = 0;
	isClone = false;
}

void SECTREE::Destroy()
{
	if (!m_set_entity.empty())
	{
		SysLog("Sectree: entity set not empty!!");

		ENTITY_SET::iterator it = m_set_entity.begin();

		for (; it != m_set_entity.end(); ++it)
		{
			LPENTITY ent = *it;

			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				SysLog("Sectree: destroying character: {} is_pc {}", ch->GetName(), ch->IsPC() ? 1 : 0);

				if (ch->GetDesc())
					DESC_MANAGER::GetInstance()->DestroyDesc(ch->GetDesc());
				else
					M2_DESTROY_CHARACTER(ch);
			}
			else if (ent->IsType(ENTITY_ITEM))
			{
				LPITEM item = (LPITEM) ent;

				SysLog("Sectree: destroying Item: {}", item->GetName());

				M2_DESTROY_ITEM(item);
			}
			else
				SysLog("Sectree: unknown type: {}", ent->GetType());
		}
	}
	m_set_entity.clear();

	if (!isClone && m_pAttribute)
	{
		M2_DELETE(m_pAttribute);
		m_pAttribute = nullptr;
	}
}

SECTREEID SECTREE::GetID()
{
	return m_id;
}

void SECTREE::IncreasePC()
{
	LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

	while (it_tree != m_neighbor_list.end())
	{
		++(*it_tree)->m_iPCCount;
		++it_tree;
	}
}

void SECTREE::DecreasePC()
{
	LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

	while (it_tree != m_neighbor_list.end())
	{
		LPSECTREE tree = *it_tree++;

		if (--tree->m_iPCCount <= 0)
		{
			if (tree->m_iPCCount < 0)
			{
				SysLog("tree pc count lower than zero (value {} coord {} {})", tree->m_iPCCount, (int32_t)tree->m_id.coord.x, (int32_t)tree->m_id.coord.y);
				tree->m_iPCCount = 0;
			}

			ENTITY_SET::iterator it_entity = tree->m_set_entity.begin();

			while (it_entity != tree->m_set_entity.end())
			{
				LPENTITY pEnt = *(it_entity++);

				if (pEnt->IsType(ENTITY_CHARACTER))
				{
					LPCHARACTER ch = (LPCHARACTER) pEnt;
					ch->StopStateMachine();
				}
			}
		}
	}
}

bool SECTREE::InsertEntity(LPENTITY pEnt)
{
	LPSECTREE pCurTree;

	if ((pCurTree = pEnt->GetSectree()) == this)
		return false;

	if (m_set_entity.find(pEnt) != m_set_entity.end()) 
	{
		SysLog("entity already exist in this sectree!");
		return false;
	}

	if (pCurTree)
		pCurTree->m_set_entity.erase(pEnt);

	pEnt->SetSectree(this);

	m_set_entity.insert(pEnt);

	if (pEnt->IsType(ENTITY_CHARACTER))
	{
		LPCHARACTER pChr = (LPCHARACTER) pEnt;

		if (pChr->IsPC())
		{
			IncreasePC();

			if (pCurTree)
				pCurTree->DecreasePC();
		}
		else if (m_iPCCount > 0 && !pChr->IsWarp() && !pChr->IsGoto())
		{
			pChr->StartStateMachine();
		}
	}

	return true;
}

void SECTREE::RemoveEntity(LPENTITY pEnt)
{
	ENTITY_SET::iterator it = m_set_entity.find(pEnt);

	if (it == m_set_entity.end()) {
		return;
	}
	m_set_entity.erase(it);

	pEnt->SetSectree(nullptr);

	if (pEnt->IsType(ENTITY_CHARACTER))
	{
		if (((LPCHARACTER) pEnt)->IsPC())
			DecreasePC();
	}
}

void SECTREE::BindAttribute(CAttribute* pAttribute)
{
	m_pAttribute = pAttribute;
}

void SECTREE::CloneAttribute(LPSECTREE tree)
{
	m_pAttribute = tree->m_pAttribute;
	isClone = true;
}

void SECTREE::SetAttribute(uint32_t x, uint32_t y, uint32_t dwAttr)
{
	assert(m_pAttribute != nullptr);
	m_pAttribute->Set(x, y, dwAttr);
}

void SECTREE::RemoveAttribute(uint32_t x, uint32_t y, uint32_t dwAttr)
{
	assert(m_pAttribute != nullptr);
	m_pAttribute->Remove(x, y, dwAttr);
}

uint32_t SECTREE::GetAttribute(int32_t x, int32_t y)
{
	assert(m_pAttribute != nullptr);
	return m_pAttribute->Get((x % SECTREE_SIZE) / CELL_SIZE, (y % SECTREE_SIZE) / CELL_SIZE);
}

bool SECTREE::IsAttr(int32_t x, int32_t y, uint32_t dwFlag)
{
	if (IS_SET(GetAttribute(x, y), dwFlag))
		return true;

	return false;
}

int32_t SECTREE::GetEventAttribute(int32_t x, int32_t y)
{
	return GetAttribute(x, y) >> 8;
}

