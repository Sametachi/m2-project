#include "StdAfx.h"
#include "PythonSafeBox.h"

void CPythonSafeBox::OpenSafeBox(int32_t iSize)
{
	m_dwMoney = 0;
	m_ItemInstanceVector.clear();
	m_ItemInstanceVector.resize(SAFEBOX_SLOT_X_COUNT * iSize);

	for (uint32_t i = 0; i < m_ItemInstanceVector.size(); ++i)
	{
		TItemData& rInstance = m_ItemInstanceVector[i];
		ZeroMemory(&rInstance, sizeof(rInstance));
	}
}

void CPythonSafeBox::SetItemData(uint32_t dwSlotIndex, const TItemData& rItemInstance)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceLog("CPythonSafeBox::SetItemData(dwSlotIndex={}) - Strange slot index", dwSlotIndex);
		return;
	}

	m_ItemInstanceVector[dwSlotIndex] = rItemInstance;
}

void CPythonSafeBox::DelItemData(uint32_t dwSlotIndex)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceLog("CPythonSafeBox::DelItemData(dwSlotIndex={}) - Strange slot index", dwSlotIndex);
		return;
	}

	TItemData& rInstance = m_ItemInstanceVector[dwSlotIndex];
	ZeroMemory(&rInstance, sizeof(rInstance));
}

void CPythonSafeBox::SetMoney(uint32_t dwMoney)
{
	m_dwMoney = dwMoney;
}

uint32_t CPythonSafeBox::GetMoney()
{
	return m_dwMoney;
}

int32_t CPythonSafeBox::GetCurrentSafeBoxSize()
{
	return m_ItemInstanceVector.size();
}

BOOL CPythonSafeBox::GetSlotItemID(uint32_t dwSlotIndex, uint32_t* pdwItemID)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceLog("CPythonSafeBox::GetSlotItemID(dwSlotIndex={}) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*pdwItemID = m_ItemInstanceVector[dwSlotIndex].vnum;

	return TRUE;
}

BOOL CPythonSafeBox::GetItemDataPtr(uint32_t dwSlotIndex, TItemData** ppInstance)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceLog("CPythonSafeBox::GetItemData(dwSlotIndex={}) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*ppInstance = &m_ItemInstanceVector[dwSlotIndex];

	return TRUE;
}

CPythonSafeBox::CPythonSafeBox()
{
	m_dwMoney = 0;
}

CPythonSafeBox::~CPythonSafeBox()
{
}

static int32_t safeboxGetCurrentSafeboxSize()
{
	return  CPythonSafeBox::GetInstance()->GetCurrentSafeBoxSize();
}

static uint32_t safeboxGetItemID(uint32_t ipos)
{

	TItemData * pInstance;
	if (!CPythonSafeBox::GetInstance()->GetItemDataPtr(ipos, &pInstance))
		throw std::runtime_error("invalid safebox pos");

	return  pInstance->vnum;
}

static uint8_t safeboxGetItemCount(uint32_t ipos)
{

	TItemData * pInstance;
	if (!CPythonSafeBox::GetInstance()->GetItemDataPtr(ipos, &pInstance))
		throw std::runtime_error("invalid safebox pos");

	return  pInstance->count;
}

static uint32_t safeboxGetItemFlags(uint32_t ipos)
{

	TItemData * pInstance;
	if (!CPythonSafeBox::GetInstance()->GetItemDataPtr(ipos, &pInstance))
		throw std::runtime_error("invalid safebox pos");

	return  pInstance->flags;
}

static int32_t safeboxGetItemMetinSocket(uint32_t iSlotIndex, size_t iSocketIndex)
{

	if (iSocketIndex >= ITEM::SOCKET_MAX_NUM)
		throw std::runtime_error("invalid safebox pos");

	TItemData * pItemData;
	if (!CPythonSafeBox::GetInstance()->GetItemDataPtr(iSlotIndex, &pItemData))
		throw std::runtime_error("invalid safebox pos");

	return  pItemData->alSockets[iSocketIndex];
}

static std::tuple<uint8_t, int16_t> safeboxGetItemAttribute(uint32_t iSlotIndex, size_t iAttrSlotIndex)
{

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM::ATTRIBUTE_MAX_NUM)
	{
		TItemData* pItemData;
		if (CPythonSafeBox::GetInstance()->GetItemDataPtr(iSlotIndex, &pItemData))
			return std::make_tuple(pItemData->aAttr[iAttrSlotIndex].bType, pItemData->aAttr[iAttrSlotIndex].sValue);
	}

	return std::make_tuple(0, 0);
}

static uint32_t safeboxGetMoney()
{
	return  CPythonSafeBox::GetInstance()->GetMoney();
}

PYBIND11_EMBEDDED_MODULE(safebox, m)
{
	m.def("GetCurrentSafeboxSize",	safeboxGetCurrentSafeboxSize);
	m.def("GetItemID",	safeboxGetItemID);
	m.def("GetItemCount",	safeboxGetItemCount);
	m.def("GetItemFlags",	safeboxGetItemFlags);
	m.def("GetItemMetinSocket",	safeboxGetItemMetinSocket);
	m.def("GetItemAttribute",	safeboxGetItemAttribute);
	m.def("GetMoney",	safeboxGetMoney);

	m.attr("SAFEBOX_SLOT_X_COUNT") = int32_t(CPythonSafeBox::SAFEBOX_SLOT_X_COUNT);
	m.attr("SAFEBOX_SLOT_Y_COUNT") = int32_t(CPythonSafeBox::SAFEBOX_SLOT_Y_COUNT);
	m.attr("SAFEBOX_PAGE_SIZE") = int32_t(CPythonSafeBox::SAFEBOX_PAGE_SIZE);
}
