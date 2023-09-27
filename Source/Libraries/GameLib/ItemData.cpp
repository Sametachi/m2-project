#include "StdAfx.h"
#include "../eterLib/ResourceManager.h"

#include "ItemData.h"

CDynamicPool<CItemData>		CItemData::ms_kPool;

extern uint32_t GetDefaultCodePage();

CItemData* CItemData::New()
{
	return ms_kPool.Alloc();
}

void CItemData::Delete(CItemData* pkItemData)
{
	pkItemData->Clear();
	ms_kPool.Free(pkItemData);
}

void CItemData::DestroySystem()
{
	ms_kPool.Destroy();
}

CGraphicThing * CItemData::GetModelThing()
{
	return m_pModelThing;
}

CGraphicThing * CItemData::GetSubModelThing()
{
	if (m_pSubModelThing)
		return m_pSubModelThing;
	else
		return m_pModelThing;
}

CGraphicThing * CItemData::GetDropModelThing()
{
	return m_pDropModelThing;
}

CGraphicSubImage * CItemData::GetIconImage()
{
	if(m_pIconImage == NULL && m_strIconFileName.empty() == false)
		__SetIconImage(m_strIconFileName.c_str());
	return m_pIconImage;
}

uint32_t CItemData::GetLODModelThingCount()
{
	return m_pLODModelThingVector.size();
}

BOOL CItemData::GetLODModelThingPointer(uint32_t dwIndex, CGraphicThing ** ppModelThing)
{
	if (dwIndex >= m_pLODModelThingVector.size())
		return FALSE;

	*ppModelThing = m_pLODModelThingVector[dwIndex];

	return TRUE;
}

uint32_t CItemData::GetAttachingDataCount()
{
	return m_AttachingDataVector.size();
}

BOOL CItemData::GetCollisionDataPointer(uint32_t dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData)
{
	if (dwIndex >= GetAttachingDataCount())
		return FALSE;

	if (NRaceData::ATTACHING_DATA_TYPE_COLLISION_DATA != m_AttachingDataVector[dwIndex].dwType)
		return FALSE;

	*c_ppAttachingData = &m_AttachingDataVector[dwIndex];
	return TRUE;
}

BOOL CItemData::GetAttachingDataPointer(uint32_t dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData)
{
	if (dwIndex >= GetAttachingDataCount())
		return FALSE;

	*c_ppAttachingData = &m_AttachingDataVector[dwIndex];
	return TRUE;
}

void CItemData::SetSummary(const std::string& c_rstSumm)
{
	m_strSummary=c_rstSumm;
}

void CItemData::SetDescription(const std::string& c_rstDesc)
{
	m_strDescription=c_rstDesc;
}

void CItemData::SetName(const std::string& c_rstName)
{
	m_strName = c_rstName;
}

void CItemData::SetDefaultItemData(const char * c_szIconFileName, const char * c_szModelFileName)
{
	if(c_szModelFileName)
	{
		m_strModelFileName = c_szModelFileName;
		m_strDropModelFileName = c_szModelFileName;
	}
	else
	{
		m_strModelFileName = "";
		m_strDropModelFileName = "d:/ymir work/item/etc/item_bag.gr2";
	}
	m_strIconFileName = c_szIconFileName;

	m_strSubModelFileName = "";
	m_strDescription = "";
	m_strSummary = "";
	memset(m_ItemTable.alSockets, 0, sizeof(m_ItemTable.alSockets));

	__LoadFiles();
}

void CItemData::__LoadFiles()
{
	// Model File Name
	if (!m_strModelFileName.empty())
		m_pModelThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(m_strModelFileName.c_str());

	if (!m_strSubModelFileName.empty())
		m_pSubModelThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(m_strSubModelFileName.c_str());

	if (!m_strDropModelFileName.empty())
		m_pDropModelThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(m_strDropModelFileName.c_str());


	if (!m_strLODModelFileNameVector.empty())
	{
		m_pLODModelThingVector.clear();
		m_pLODModelThingVector.resize(m_strLODModelFileNameVector.size());

		for (uint32_t i = 0; i < m_strLODModelFileNameVector.size(); ++i)
		{
			const std::string & c_rstrLODModelFileName = m_strLODModelFileNameVector[i];
			m_pLODModelThingVector[i] = (CGraphicThing *)CResourceManager::GetInstance()->LoadResource<CGraphicThing>(c_rstrLODModelFileName.c_str());
		}
	}
}

void CItemData::__SetIconImage(const char * c_szFileName)
{
	if (!CResourceManager::GetInstance()->IsFileExist(c_szFileName))
	{
		TraceLog("{} not found.",c_szFileName);
		m_pIconImage = NULL;

		static const char* c_szAlterIconImage = "icon/item/27995.tga";
		if (CResourceManager::GetInstance()->IsFileExist(c_szAlterIconImage))
			m_pIconImage = CResourceManager::GetInstance()->LoadResource<CGraphicSubImage>(c_szAlterIconImage);

	}
	else if (m_pIconImage == NULL)
		m_pIconImage = CResourceManager::GetInstance()->LoadResource<CGraphicSubImage>(c_szFileName);
}

void CItemData::SetItemTableData(TItemTable * pItemTable)
{
	memcpy(&m_ItemTable, pItemTable, sizeof(TItemTable));
}

const TItemTable* CItemData::GetTable() const
{
	return &m_ItemTable;
}

uint32_t CItemData::GetIndex() const
{
	return m_ItemTable.dwVnum;
}

const char * CItemData::GetName() const
{
	return m_strName.c_str();
}

const char * CItemData::GetDescription() const
{
	return m_strDescription.c_str();
}

const char * CItemData::GetSummary() const
{
	return m_strSummary.c_str();
}


uint8_t CItemData::GetType() const
{
	return m_ItemTable.bType;
}

uint8_t CItemData::GetSubType() const
{
	return m_ItemTable.bSubType;
}

#define DEF_STR(x) #x

const char* CItemData::GetUseTypeString() const
{
	if (GetType() != ITEM::TYPE_USE)
		return "NOT_USE_TYPE";

	switch (GetSubType())
	{
		case ITEM::USE_TUNING:
			return DEF_STR(USE_TUNING);
		case ITEM::USE_DETACHMENT:
			return DEF_STR(USE_DETACHMENT);
		case ITEM::USE_CLEAN_SOCKET:
			return DEF_STR(USE_CLEAN_SOCKET);
		case ITEM::USE_CHANGE_ATTRIBUTE:
			return DEF_STR(USE_CHANGE_ATTRIBUTE);
		case ITEM::USE_ADD_ATTRIBUTE:
			return DEF_STR(USE_ADD_ATTRIBUTE);
		case ITEM::USE_ADD_ATTRIBUTE2:
			return DEF_STR(USE_ADD_ATTRIBUTE2);
		case ITEM::USE_ADD_ACCESSORY_SOCKET:
			return DEF_STR(USE_ADD_ACCESSORY_SOCKET);
		case ITEM::USE_PUT_INTO_ACCESSORY_SOCKET:
			return DEF_STR(USE_PUT_INTO_ACCESSORY_SOCKET);
		case ITEM::USE_PUT_INTO_BELT_SOCKET:
			return DEF_STR(USE_PUT_INTO_BELT_SOCKET);
		case ITEM::USE_PUT_INTO_RING_SOCKET:
			return DEF_STR(USE_PUT_INTO_RING_SOCKET);
	}
	return "USE_UNKNOWN_TYPE";
}


uint32_t CItemData::GetWeaponType() const
{
#ifdef ENABLE_WEAPON_COSTUME_SYSTEM
	if (GetType()==ITEM_TYPE_COSTUME && GetSubType()==ITEM::COSTUME_WEAPON)
		return GetValue(3);
#endif
	return m_ItemTable.bSubType;
}

uint8_t CItemData::GetSize() const
{
	return m_ItemTable.bSize;
}

BOOL CItemData::IsAntiFlag(uint32_t dwFlag) const
{
	return (dwFlag & m_ItemTable.dwAntiFlags) != 0;
}

BOOL CItemData::IsFlag(uint32_t dwFlag) const
{
	return (dwFlag & m_ItemTable.dwFlags) != 0;
}

BOOL CItemData::IsWearableFlag(uint32_t dwFlag) const
{
	return (dwFlag & m_ItemTable.dwWearFlags) != 0;
}

BOOL CItemData::HasNextGrade() const
{
	return 0 != m_ItemTable.dwRefinedVnum;
}

uint32_t CItemData::GetWearFlags() const
{
	return m_ItemTable.dwWearFlags;
}

uint32_t CItemData::GetIBuyItemPrice() const
{
	return m_ItemTable.dwIBuyItemPrice;
}

uint32_t CItemData::GetISellItemPrice() const
{
	return m_ItemTable.dwISellItemPrice;
}


BOOL CItemData::GetLimit(uint8_t byIndex, TItemLimit * pItemLimit) const
{
	if (byIndex >= ITEM::LIMIT_SLOT_MAX_NUM)
	{
		assert(byIndex < ITEM::LIMIT_SLOT_MAX_NUM);
		return FALSE;
	}

	*pItemLimit = m_ItemTable.aLimits[byIndex];

	return TRUE;
}

BOOL CItemData::GetApply(uint8_t byIndex, TItemApply * pItemApply) const
{
	if (byIndex >= ITEM::APPLY_MAX_NUM)
	{
		assert(byIndex < ITEM::APPLY_MAX_NUM);
		return FALSE;
	}

	*pItemApply = m_ItemTable.aApplies[byIndex];
	return TRUE;
}

int32_t CItemData::GetValue(uint8_t byIndex) const
{
	if (byIndex >= ITEM::VALUES_MAX_NUM)
	{
		assert(byIndex < ITEM::VALUES_MAX_NUM);
		return 0;
	}

	return m_ItemTable.alValues[byIndex];
}

int32_t CItemData::SetSocket(uint8_t byIndex,uint32_t value)
{
	if (byIndex >= ITEM::SOCKET_MAX_NUM)
	{
		assert(byIndex < ITEM::SOCKET_MAX_NUM);
		return -1;
	}

	return m_ItemTable.alSockets[byIndex] = value;
}

int32_t CItemData::GetSocket(uint8_t byIndex) const
{
	if (byIndex >= ITEM::SOCKET_MAX_NUM)
	{
		assert(byIndex < ITEM::SOCKET_MAX_NUM);
		return -1;
	}

	return m_ItemTable.alSockets[byIndex];
}

int32_t CItemData::GetSocketCount() const
{
	return m_ItemTable.bGainSocketPct;
}

uint32_t CItemData::GetIconNumber() const
{
	return m_ItemTable.dwVnum;
//!@#
//	return m_ItemTable.dwIconNumber;
}

UINT CItemData::GetSpecularPoweru() const
{
	return m_ItemTable.bSpecular;
}

float CItemData::GetSpecularPowerf() const
{
	UINT uSpecularPower=GetSpecularPoweru();

	return float(uSpecularPower) / 100.0f;
}

UINT CItemData::GetRefine() const
{
	return GetIndex()%10;
}

BOOL CItemData::IsEquipment() const
{
	switch (GetType())
	{
		case ITEM::TYPE_WEAPON:
		case ITEM::TYPE_ARMOR:
			return TRUE;
			break;
	}

	return FALSE;
}

void CItemData::Clear()
{
	m_strSummary = "";
	m_strModelFileName = "";
	m_strSubModelFileName = "";
	m_strDropModelFileName = "";
	m_strIconFileName = "";
	m_strName = "";
	m_strLODModelFileNameVector.clear();

	m_pModelThing = NULL;
	m_pSubModelThing = NULL;
	m_pDropModelThing = NULL;
	m_pIconImage = NULL;
	m_pLODModelThingVector.clear();

	memset(&m_ItemTable, 0, sizeof(m_ItemTable));
}

CItemData::CItemData()
{
	Clear();
}

CItemData::~CItemData()
{
}
