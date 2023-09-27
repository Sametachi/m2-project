#include "stdafx.h"
#include "constants.h"
#include "log.h"
#include "item.h"
#include "char.h"
#include "desc.h"
#include "item_manager.h"

const int32_t MAX_NORM_ATTR_NUM = ITEM_MANAGER::MAX_NORM_ATTR_NUM;
const int32_t MAX_RARE_ATTR_NUM = ITEM_MANAGER::MAX_RARE_ATTR_NUM;

int32_t CItem::GetAttributeSetIndex()
{
	if (GetType() == ITEM::TYPE_WEAPON)
	{
		if (GetSubType() == ITEM::WEAPON_ARROW)
			return -1;

		return ATTRIBUTE_SET_WEAPON;
	}

	if (GetType() == ITEM::TYPE_ARMOR || GetType() == ITEM::TYPE_COSTUME)
	{
		switch (GetSubType())
		{
			case ITEM::ARMOR_BODY:
				return ATTRIBUTE_SET_BODY;

			case ITEM::ARMOR_WRIST:
				return ATTRIBUTE_SET_WRIST;

			case ITEM::ARMOR_FOOTS:
				return ATTRIBUTE_SET_FOOTS;

			case ITEM::ARMOR_NECK:
				return ATTRIBUTE_SET_NECK;

			case ITEM::ARMOR_HEAD:
				return ATTRIBUTE_SET_HEAD;

			case ITEM::ARMOR_SHIELD:
				return ATTRIBUTE_SET_SHIELD;

			case ITEM::ARMOR_EAR:
				return ATTRIBUTE_SET_EAR;
		}
	}

	return -1;
}

bool CItem::HasAttr(uint8_t bApply)
{
	for (int32_t i = 0; i < ITEM::APPLY_MAX_NUM; ++i)
		if (m_pProto->aApplies[i].bType == bApply)
			return true;

	for (int32_t i = 0; i < MAX_NORM_ATTR_NUM; ++i)
		if (GetAttributeType(i) == bApply)
			return true;

	return false;
}

bool CItem::HasRareAttr(uint8_t bApply)
{
	for (int32_t i = 0; i < MAX_RARE_ATTR_NUM; ++i)
		if (GetAttributeType(i + 5) == bApply)
			return true;

	return false;
}

void CItem::AddAttribute(uint8_t bApply, int16_t sValue)
{
	if (HasAttr(bApply))
		return;

	int32_t i = GetAttributeCount();

	if (i >= MAX_NORM_ATTR_NUM)
	{
		SysLog("item attribute overflow!");
	}
	else
	{
		if (sValue)
			SetAttribute(i, bApply, sValue);
	}
}

void CItem::AddAttr(uint8_t bApply, uint8_t bLevel)
{
	if (HasAttr(bApply))
		return;

	if (bLevel <= 0)
		return;

	int32_t i = GetAttributeCount();

	if (i == MAX_NORM_ATTR_NUM)
	{
		SysLog("item attribute overflow!");
	}
	else
	{
		const TItemAttrTable& r = g_map_itemAttr[bApply];
		int32_t lVal = r.lValues[MIN(4, bLevel - 1)];

		if (lVal)
			SetAttribute(i, bApply, lVal);
	}
}

void CItem::PutAttributeWithLevel(uint8_t bLevel)
{
	int32_t iAttributeSet = GetAttributeSetIndex();

	if (iAttributeSet < 0)
		return;

	if (bLevel > ITEM::ATTRIBUTE_MAX_LEVEL)
		return;

	std::vector<int32_t> avail;

	int32_t total = 0;

	for (int32_t i = 0; i < ITEM::MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable& r = g_map_itemAttr[i];

		if (r.bMaxLevelBySet[iAttributeSet] && !HasAttr(i))
		{
			avail.push_back(i);
			total += r.dwProb;
		}
	}

	uint32_t prob = number(1, total);
	int32_t attr_idx = ITEM::APPLY_NONE;

	for (uint32_t i = 0; i < avail.size(); ++i)
	{
		const TItemAttrTable& r = g_map_itemAttr[avail[i]];

		if (prob <= r.dwProb)
		{
			attr_idx = avail[i];
			break;
		}

		prob -= r.dwProb;
	}

	if (!attr_idx)
	{
		SysLog("Cannot put item attribute {} {}", iAttributeSet, bLevel);
		return;
	}

	const TItemAttrTable& r = g_map_itemAttr[attr_idx];

	if (bLevel > r.bMaxLevelBySet[iAttributeSet])
		bLevel = r.bMaxLevelBySet[iAttributeSet];

	AddAttr(attr_idx, bLevel);
}

void CItem::PutAttribute(const int32_t* aiAttrPercentTable)
{
	int32_t iAttrLevelPercent = number(1, 100);
	int32_t i;

	for (i = 0; i < ITEM::ATTRIBUTE_MAX_LEVEL; ++i)
	{
		if (iAttrLevelPercent <= aiAttrPercentTable[i])
			break;

		iAttrLevelPercent -= aiAttrPercentTable[i];
	}

	PutAttributeWithLevel(i + 1);
}

void CItem::ChangeAttribute(const int32_t* aiChangeProb)
{
	int32_t iAttributeCount = GetAttributeCount();

	ClearAttribute();

	if (iAttributeCount == 0)
		return;

	TItemTable const* pProto = GetProto();

	if (pProto && pProto->sAddonType)
	{
		ApplyAddon(pProto->sAddonType);
	}

	static const int32_t tmpChangeProb[ITEM::ATTRIBUTE_MAX_LEVEL] =
	{
		0, 10, 40, 35, 15,
	};

	for (int32_t i = GetAttributeCount(); i < iAttributeCount; ++i)
	{
		if (aiChangeProb == nullptr)
		{
			PutAttribute(tmpChangeProb);
		}
		else
		{
			PutAttribute(aiChangeProb);
		}
	}
}

void CItem::AddAttribute()
{
	static const int32_t aiItemAddAttributePercent[ITEM::ATTRIBUTE_MAX_LEVEL] = 
	{
		40, 50, 10, 0, 0
	};

	if (GetAttributeCount() < MAX_NORM_ATTR_NUM)
		PutAttribute(aiItemAddAttributePercent);
}

void CItem::ClearAttribute()
{
	for (int32_t i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		m_aAttr[i].bType = 0;
		m_aAttr[i].sValue = 0;
	}
}

int32_t CItem::GetAttributeCount()
{
	int32_t i;

	for (i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == 0)
			break;
	}

	return i;
}

int32_t CItem::FindAttribute(uint8_t bType)
{
	for (int32_t i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		if (GetAttributeType(i) == bType)
			return i;
	}

	return -1;
}

bool CItem::RemoveAttributeAt(int32_t index)
{
	if (GetAttributeCount() <= index)
		return false;

	for (int32_t i = index; i < MAX_NORM_ATTR_NUM - 1; ++i)
	{
		SetAttribute(i, GetAttributeType(i + 1), GetAttributeValue(i + 1));
	}

	SetAttribute(MAX_NORM_ATTR_NUM - 1, ITEM::APPLY_NONE, 0);
	return true;
}

bool CItem::RemoveAttributeType(uint8_t bType)
{
	int32_t index = FindAttribute(bType);
	return index != -1 && RemoveAttributeType(index);
}

void CItem::SetAttributes(const TPlayerItemAttribute* c_pAttribute)
{
	memcpy(m_aAttr, c_pAttribute, sizeof(m_aAttr));
	Save();
}

void CItem::SetAttribute(int32_t i, uint8_t bType, int16_t sValue)
{
	assert(i < MAX_NORM_ATTR_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		const char* pszIP = nullptr;

		if (GetOwner() && GetOwner()->GetDesc())
			pszIP = GetOwner()->GetDesc()->GetHostName();

		LogManager::GetInstance()->ItemLog(i, bType, sValue, GetID(), "SET_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
	}
}

void CItem::SetForceAttribute(int32_t i, uint8_t bType, int16_t sValue)
{
	assert(i < ITEM::ATTRIBUTE_MAX_NUM);

	m_aAttr[i].bType = bType;
	m_aAttr[i].sValue = sValue;
	UpdatePacket();
	Save();

	if (bType)
	{
		const char* pszIP = nullptr;

		if (GetOwner() && GetOwner()->GetDesc())
			pszIP = GetOwner()->GetDesc()->GetHostName();

		LogManager::GetInstance()->ItemLog(i, bType, sValue, GetID(), "SET_FORCE_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
	}
}


void CItem::CopyAttributeTo(LPITEM pItem)
{
	pItem->SetAttributes(m_aAttr);
}

int32_t CItem::GetRareAttrCount()
{
	int32_t ret = 0;

	if (m_aAttr[5].bType != 0)
		ret++;

	if (m_aAttr[6].bType != 0)
		ret++;

	return ret;
}

bool CItem::ChangeRareAttribute()
{
	if (GetRareAttrCount() == 0)
		return false;

	int32_t cnt = GetRareAttrCount();

	for (int32_t i = 0; i < cnt; ++i)
	{
		m_aAttr[i + 5].bType = 0;
		m_aAttr[i + 5].sValue = 0;
	}

	if (GetOwner() && GetOwner()->GetDesc())
		LogManager::GetInstance()->ItemLog(GetOwner(), this, "SET_RARE_CHANGE", "");
	else
		LogManager::GetInstance()->ItemLog(0, 0, 0, GetID(), "SET_RARE_CHANGE", "", "", GetOriginalVnum());

	for (int32_t i = 0; i < cnt; ++i)
	{
		AddRareAttribute();
	}

	return true;
}

bool CItem::AddRareAttribute()
{
	int32_t count = GetRareAttrCount();

	if (count >= 2)
		return false;

	int32_t pos = count + 5;
	TPlayerItemAttribute & attr = m_aAttr[pos];

	int32_t nAttrSet = GetAttributeSetIndex();
	std::vector<int32_t> avail;

	for (int32_t i = 0; i < ITEM::MAX_APPLY_NUM; ++i)
	{
		const TItemAttrTable& r = g_map_itemRare[i];

		if (r.dwApplyIndex != 0 && r.bMaxLevelBySet[nAttrSet] > 0 && HasRareAttr(i) != true)
		{
			avail.push_back(i);
		}
	}

	const TItemAttrTable& r = g_map_itemRare[avail[number(0, avail.size() - 1)]];
	int32_t nAttrLevel = 5;

	if (nAttrLevel > r.bMaxLevelBySet[nAttrSet])
		nAttrLevel = r.bMaxLevelBySet[nAttrSet];

	attr.bType = r.dwApplyIndex;
	attr.sValue = r.lValues[nAttrLevel - 1];

	UpdatePacket();

	Save();

	const char* pszIP = nullptr;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	LogManager::GetInstance()->ItemLog(pos, attr.bType, attr.sValue, GetID(), "SET_RARE", "", pszIP ? pszIP : "", GetOriginalVnum());
	return true;
}

