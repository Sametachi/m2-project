#include "StdAfx.h"
#include "MapAccessorArea.h"
#include "../../../Client/EffectLib/EffectManager.h"
#include "../../../Client/gamelib/Property.h"
#include "MapAccessorOutdoor.h"

FILE * CAreaAccessor::ms_LogFile;
std::set<std::string> CAreaAccessor::ms_NonAttributeObjectSet;
std::vector<CAreaAccessor::TCollisionDataCounter> CAreaAccessor::ms_CollisionDataCountVec;

//////////////////////////////////////////////////////////////////////////
// CAreaAccessor
//////////////////////////////////////////////////////////////////////////

int32_t CAreaAccessor::GetPickedObjectIndex()
{
	for (uint32_t i = 0; i < m_ObjectInstanceVector.size(); ++i)
	{
		const TObjectInstance * c_pObjectInstance = m_ObjectInstanceVector[i];

		if (prt::PROPERTY_TYPE_BUILDING == c_pObjectInstance->dwType)
		{
			if (c_pObjectInstance->pThingInstance)
			{
				float fu, fv, ft;
				if (c_pObjectInstance->pThingInstance->Intersect(&fu, &fv, &ft))
					return i;
			}
		}
		else if (prt::PROPERTY_TYPE_DUNGEON_BLOCK == c_pObjectInstance->dwType)
		{
			if (c_pObjectInstance->pDungeonBlock)
			{
				float fu, fv, ft;
				D3DXMATRIX & rmatTransform = c_pObjectInstance->pDungeonBlock->GetTransform();
				if (c_pObjectInstance->pDungeonBlock->Intersect(&fu, &fv, &ft))
					return i;
			}
		}
		else if (prt::PROPERTY_TYPE_AMBIENCE == c_pObjectInstance->dwType)
		{
			TAmbienceInstance * pAmbienceInstance = c_pObjectInstance->pAmbienceInstance;
			if (pAmbienceInstance->Picking())
				return i;
		}
	}

	return -1;
}

BOOL CAreaAccessor::Picking()
{
	for (uint32_t i = 0; i < m_ObjectInstanceVector.size(); ++i)
	{
		const TObjectInstance * c_pObjectInstance = m_ObjectInstanceVector[i];

		if (prt::PROPERTY_TYPE_TREE == c_pObjectInstance->dwType)
		{
			CSpeedTreeWrapper * pTree = c_pObjectInstance->pTree;
			UINT uiCollisionObjectCount = pTree->GetCollisionObjectCount();

			for (UINT i = 0; i < uiCollisionObjectCount; ++i)
			{
				CSpeedTreeRT::ECollisionObjectType ObjectType;
				D3DXVECTOR3 v3Position(0.0f, 0.0f, 0.0f);
				float fDimensional[3] = {0}, fEulerAngles[3] = {0};

#ifdef _SPEEDTREE_42_
				pTree->GetCollisionObject(i, ObjectType, (float *)&v3Position, fDimensional, fEulerAngles);
#else
				pTree->GetCollisionObject(i, ObjectType, (float *)&v3Position, fDimensional);
#endif

				if (ObjectType == CSpeedTreeRT::CO_SPHERE)
				{
				}
				else if (ObjectType == CSpeedTreeRT::CO_BOX)
				{
				}
			}
		}
		else if (prt::PROPERTY_TYPE_BUILDING == c_pObjectInstance->dwType)
		{
			if (c_pObjectInstance->pThingInstance)
			{
				float fu, fv, ft;
				if (c_pObjectInstance->pThingInstance->Intersect(&fu, &fv, &ft))
				{
					__ClickObject(i);
					return TRUE;
				}
			}
		}
		else if (prt::PROPERTY_TYPE_DUNGEON_BLOCK == c_pObjectInstance->dwType)
		{
			if (c_pObjectInstance->pDungeonBlock)
			{
				float fu, fv, ft;
				D3DXMATRIX & rmatTransform = c_pObjectInstance->pDungeonBlock->GetTransform();
				if (c_pObjectInstance->pDungeonBlock->Intersect(&fu, &fv, &ft))
				{
					__ClickObject(i);
					return TRUE;
				}
			}
		}
		else if (prt::PROPERTY_TYPE_AMBIENCE == c_pObjectInstance->dwType)
		{
			TAmbienceInstance * pAmbienceInstance = c_pObjectInstance->pAmbienceInstance;
			if (pAmbienceInstance->Picking())
			{
				if (!IsSelectedObject(i))
				{
					CancelSelect();
					SelectObject(i);
				}
				return TRUE;
			}
		}
	}

	return FALSE;
}

BOOL CAreaAccessor::IsSelected()
{
	return (!m_SelectObjectList.empty());
}

void CAreaAccessor::RenderSelectedObject()
{
	typedef struct SSelectObjectRenderer : public CScreen
	{
		SSelectObjectRenderer(CAreaAccessor::TObjectData & rObjectData, CAreaAccessor::TObjectInstance & rObjectInstance)
			: m_rObjectData(rObjectData),
			  m_rObjectInstance(rObjectInstance)
		{
			m_fRadius = 100.0f;
			m_bBoundBox = FALSE;

			CProperty * pProperty;
			if (!CPropertyManager::Instance().Get(m_rObjectData.dwCRC, &pProperty))
				return;

			const char * c_szPropertyType;
			if (!pProperty->GetString("PropertyType", &c_szPropertyType))
				return;

			switch (prt::GetPropertyType(c_szPropertyType))
			{
				case prt::PROPERTY_TYPE_BUILDING:
				{
					if (m_rObjectInstance.pThingInstance) // @fixme121
					{
						m_rObjectInstance.pThingInstance->GetBoundBox(&m_v3Min, &m_v3Max);
						m_fRotation = m_rObjectInstance.pThingInstance->GetRotation();
						m_bBoundBox = TRUE;
					}
					break;
				}
				case prt::PROPERTY_TYPE_DUNGEON_BLOCK:
				{
					if (m_rObjectInstance.pDungeonBlock) // @fixme121
					{
						m_rObjectInstance.pDungeonBlock->GetBoundBox(&m_v3Min, &m_v3Max);
						m_fRotation = m_rObjectInstance.pDungeonBlock->GetRotation();
						m_bBoundBox = TRUE;
					}
					break;
				}
				case prt::PROPERTY_TYPE_TREE:
				{
					m_fRadius = 50.0f;
					break;
				}
				case prt::PROPERTY_TYPE_AMBIENCE:
				{
					m_fRadius = m_rObjectData.dwRange;
					break;
				}
			}
		}
		void Render()
		{
			SetDiffuseColor(globals::dft::RENDER_CURSOR_COLOR_R,globals::dft::RENDER_CURSOR_COLOR_G,globals::dft::RENDER_CURSOR_COLOR_B);

			if (m_bBoundBox)
			{
				ms_lpd3dMatStack->Push();
				ms_lpd3dMatStack->RotateAxis(&D3DXVECTOR3(0.0f, 0.0f, 1.0f), D3DXToRadian(m_fRotation));
				ms_lpd3dMatStack->Translate(m_rObjectData.Position.x, m_rObjectData.Position.y, m_rObjectData.Position.z);
				RenderLineCube(m_v3Min.x, m_v3Min.y, m_v3Min.z,
							   m_v3Max.x, m_v3Max.y, m_v3Max.z);
				ms_lpd3dMatStack->Pop();
			}
			else
			{
				STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
				RenderCircle2d(m_rObjectData.Position.x, m_rObjectData.Position.y, m_rObjectData.Position.z, m_fRadius);
				RenderCircle2d(m_rObjectData.Position.x, m_rObjectData.Position.y, m_rObjectData.Position.z, m_fRadius*2.0f);
				RenderCircle2d(m_rObjectData.Position.x, m_rObjectData.Position.y, m_rObjectData.Position.z, m_fRadius*3.0f);
			}
		}

		BOOL m_bBoundBox;
		D3DXVECTOR3 m_v3Min;
		D3DXVECTOR3 m_v3Max;

		float m_fRotation;
		float m_fRadius;
		CAreaAccessor::TObjectData & m_rObjectData;
		CAreaAccessor::TObjectInstance & m_rObjectInstance;
	} TSelectObjectRenderer;

	TSelectObjectList::iterator itor = m_SelectObjectList.begin();
	for (; itor != m_SelectObjectList.end(); ++itor)
	{
		SSelectObject & rSelectObject = *itor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		if (!pObjectInstance) // @fixme121
			continue;
		TSelectObjectRenderer ObjectData(*pObjectData, *pObjectInstance);
		ObjectData.Render();
	}
}

BOOL CAreaAccessor::SelectObject(float fxStart, float fyStart, float fxEnd, float fyEnd)
{
	BOOL bNeedChange = FALSE;

	for (uint32_t i = 0; i < m_ObjectDataVector.size(); ++i)
	{
		TObjectData & rObjectData = m_ObjectDataVector[i];

		if (rObjectData.Position.x > fxStart)
		if (rObjectData.Position.y > fyStart)
		if (rObjectData.Position.x < fxEnd)
		if (rObjectData.Position.y < fyEnd)
		{
			if (!IsSelectedObject(i))
			{
				SelectObject(i);

				bNeedChange = TRUE;
			}
		}
	}

	std::list<uint32_t> kList_iDeletingObjectList;
	for (TSelectObjectList::iterator itor=m_SelectObjectList.begin(); itor!=m_SelectObjectList.end(); ++itor)
	{
		SSelectObject & rSelectObject = *itor;
		uint32_t dwNum = rSelectObject.dwIndex;
		TObjectData & rObjectData = m_ObjectDataVector[dwNum];

		if (rObjectData.Position.x < fxStart ||
			rObjectData.Position.y < fyStart ||
			rObjectData.Position.x > fxEnd ||
			rObjectData.Position.y > fyEnd)
		{
			if (IsSelectedObject(dwNum))
			{
				kList_iDeletingObjectList.push_back(dwNum);
				bNeedChange = TRUE;
			}
		}
	}
	for (std::list<uint32_t>::iterator itorDel=kList_iDeletingObjectList.begin(); itorDel!=kList_iDeletingObjectList.end(); ++itorDel)
	{
		__DeselectObject(*itorDel);
	}

	return bNeedChange;
}

void CAreaAccessor::DeleteSelectedObject()
{
	TSelectObjectList::reverse_iterator ritor;
	for (ritor = m_SelectObjectList.rbegin(); ritor != m_SelectObjectList.rend(); ++ritor)
	{
		SSelectObject & rSelectObject = *ritor;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
		{
			assert(!"CAreaAccessor::DeleteObject() : instance index overflow!");
		}
		else
		{
			if (pObjectInstance->dwEffectInstanceIndex!=0xffffffff)
			{
				TEffectInstanceIterator f= m_EffectInstanceMap.find(pObjectInstance->dwEffectInstanceIndex);
				if (m_EffectInstanceMap.end()!=f)
				{
					CEffectInstance * pEffectInstance = f->second;
					m_EffectInstanceMap.erase(f);

					if (CEffectManager::InstancePtr())
						CEffectManager::Instance().DestroyUnsafeEffectInstance(pEffectInstance);
				}
				pObjectInstance->dwEffectInstanceIndex = 0xffffffff;
			}

			if (pObjectInstance->pAttributeInstance)
			{
				pObjectInstance->pAttributeInstance->Clear();
				ms_AttributeInstancePool.Free(pObjectInstance->pAttributeInstance);
				pObjectInstance->pAttributeInstance = NULL;
			}

			if (pObjectInstance->pTree)
			{
				pObjectInstance->pTree->Clear();
				CSpeedTreeForestDirectX8::Instance().DeleteInstance(pObjectInstance->pTree);
				pObjectInstance->pTree = NULL;
			}

			if (pObjectInstance->pThingInstance)
			{
				CGraphicThingInstance::Delete(pObjectInstance->pThingInstance);
				pObjectInstance->pThingInstance = NULL;
			}

			if (pObjectInstance->pAmbienceInstance)
			{
				ms_AmbienceInstancePool.Free(pObjectInstance->pAmbienceInstance);
				pObjectInstance->pAmbienceInstance = NULL;
			}
			pObjectInstance->Clear();

			ms_ObjectInstancePool.Free(pObjectInstance);
		}
	}

	std::vector<TObjectInstance *> kVec_pkLeftObjectInstance;
	std::vector<TObjectData> kVec_kLeftObjectData;

	assert(m_ObjectInstanceVector.size() == m_ObjectDataVector.size());
	for (uint32_t i = 0; i < m_ObjectInstanceVector.size(); ++i)
	{
		if (IsSelectedObject(i))
			continue;

		kVec_pkLeftObjectInstance.push_back(m_ObjectInstanceVector[i]);
		kVec_kLeftObjectData.push_back(m_ObjectDataVector[i]);
	}

	m_ObjectInstanceVector.clear();
	m_ObjectInstanceVector = kVec_pkLeftObjectInstance;
	m_ObjectDataVector.clear();
	m_ObjectDataVector = kVec_kLeftObjectData;

	((CMapOutdoorAccessor *)GetOwner())->SetSelectedObjectName("");

	CSoundManager::Instance().StopAllSound3D();
	Refresh();

	CancelSelect();
}

void CAreaAccessor::MoveSelectedObject(float fx, float fy)
{
	__RefreshObjectPosition(fx, fy, 0.0f);
}

void CAreaAccessor::MoveSelectedObjectHeight(float fHeight)
{
	__RefreshObjectPosition(0.0f, 0.0f, fHeight);
}

void CAreaAccessor::AddSelectedAmbienceScale(int32_t iAddScale)
{
	TSelectObjectList::reverse_iterator ritor;
	for (ritor = m_SelectObjectList.rbegin(); ritor != m_SelectObjectList.rend(); ++ritor)
	{
		SSelectObject & rSelectObject = *ritor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		if (prt::PROPERTY_TYPE_AMBIENCE != pObjectInstance->dwType)
			continue;

		TAmbienceInstance * pAmbienceInstance = pObjectInstance->pAmbienceInstance;
		if (!pAmbienceInstance)
			continue;

		pObjectData->dwRange = max(10, int32_t(pObjectData->dwRange) + iAddScale);
		pAmbienceInstance->dwRange = max(10, int32_t(pAmbienceInstance->dwRange) + iAddScale);
	}
}

void CAreaAccessor::AddSelectedAmbienceMaxVolumeAreaPercentage(float fPercentage)
{
	TSelectObjectList::reverse_iterator ritor;
	for (ritor = m_SelectObjectList.rbegin(); ritor != m_SelectObjectList.rend(); ++ritor)
	{
		SSelectObject & rSelectObject = *ritor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		if (prt::PROPERTY_TYPE_AMBIENCE != pObjectInstance->dwType)
			continue;

		TAmbienceInstance * pAmbienceInstance = pObjectInstance->pAmbienceInstance;
		if (!pAmbienceInstance)
			continue;

		pObjectData->fMaxVolumeAreaPercentage += fPercentage;
		pObjectData->fMaxVolumeAreaPercentage = fMIN(1.0f, pObjectData->fMaxVolumeAreaPercentage);
		pObjectData->fMaxVolumeAreaPercentage = fMAX(0.0f, pObjectData->fMaxVolumeAreaPercentage);
		pAmbienceInstance->fMaxVolumeAreaPercentage = pObjectData->fMaxVolumeAreaPercentage;
	}
}

void CAreaAccessor::AddSelectedObjectRotation(float fYaw, float fPitch, float fRoll)
{
	TSelectObjectList::iterator itor;
	for (itor = m_SelectObjectList.begin(); itor != m_SelectObjectList.end(); ++itor)
	{
		SSelectObject & rSelectObject = *itor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		pObjectData->m_fYaw = fmod(pObjectData->m_fYaw + fYaw + 360.0f, 360.0f);
		pObjectData->m_fPitch = fmod(pObjectData->m_fPitch + fPitch + 360.0f, 360.0f);
		pObjectData->m_fRoll = fmod(pObjectData->m_fRoll + fRoll + 360.0f, 360.0f);

		////////////////////////////
		// SetPosition
		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		if (pObjectInstance->dwEffectInstanceIndex!=0xffffffff)
		{
			TEffectInstanceIterator f = m_EffectInstanceMap.find(pObjectInstance->dwEffectInstanceIndex);
			if (m_EffectInstanceMap.end()!=f)
			{
				CEffectInstance * pEffectInstance = f->second;
				pEffectInstance->SetRotation(pObjectData->m_fYaw, pObjectData->m_fPitch, pObjectData->m_fRoll);
			}
		}
		if (pObjectInstance->pTree)
		{
			pObjectInstance->pTree->SetRotation(pObjectData->m_fYaw, pObjectData->m_fPitch, pObjectData->m_fRoll);
		}
		if (pObjectInstance->pThingInstance)
		{
			pObjectInstance->pThingInstance->SetRotation(pObjectData->m_fYaw, pObjectData->m_fPitch, pObjectData->m_fRoll);
			pObjectInstance->pThingInstance->Update();
			pObjectInstance->pThingInstance->Transform();
			pObjectInstance->pThingInstance->Deform();
		}
		if (pObjectInstance->pDungeonBlock)
		{
			pObjectInstance->pDungeonBlock->SetRotation(pObjectData->m_fYaw, pObjectData->m_fPitch, pObjectData->m_fRoll);
			pObjectInstance->pDungeonBlock->Update();
			pObjectInstance->pDungeonBlock->Transform();
			pObjectInstance->pDungeonBlock->Deform();
		}
	}
}

void CAreaAccessor::SetSelectedObjectPortalNumber(int32_t iID)
{
	TSelectObjectList::reverse_iterator ritor;
	for (ritor = m_SelectObjectList.rbegin(); ritor != m_SelectObjectList.rend(); ++ritor)
	{
		SSelectObject & rSelectObject = *ritor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		BOOL hasSameID = FALSE;
		int32_t i;
		for (i = 0; i < PORTAL_ID_MAX_NUM; ++i)
		{
			if (iID == pObjectData->abyPortalID[i])
			{
				hasSameID = TRUE;
				break;
			}
			if (0 == pObjectData->abyPortalID[i])
			{
				break;
			}
		}

		if (hasSameID)
			continue;

		if (i >= PORTAL_ID_MAX_NUM)
		{
			assert(!"You have exceeded the number of Portal Slots");
			continue;
		}

		pObjectData->abyPortalID[i] = iID;
	}

	__RefreshSelectedInfo();
}

void CAreaAccessor::DelSelectedObjectPortalNumber(int32_t iID)
{
	TSelectObjectList::reverse_iterator ritor;
	for (ritor = m_SelectObjectList.rbegin(); ritor != m_SelectObjectList.rend(); ++ritor)
	{
		SSelectObject & rSelectObject = *ritor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		for (int32_t i = 0; i < PORTAL_ID_MAX_NUM; ++i)
		{
			if (iID == pObjectData->abyPortalID[i])
			{
				for (int32_t j = i; j < PORTAL_ID_MAX_NUM-1; ++j)
				{
					pObjectData->abyPortalID[j] = pObjectData->abyPortalID[j+1];
				}

				pObjectData->abyPortalID[PORTAL_ID_MAX_NUM-1] = 0;
				break;
			}
		}
	}

	__RefreshSelectedInfo();
}

void CAreaAccessor::CollectPortalNumber(std::set<int32_t> * pkSet_iPortalNumber)
{
	for (uint32_t i = 0; i < m_ObjectDataVector.size(); ++i)
	{
		TObjectData & rObjectData = m_ObjectDataVector[i];
		for (int32_t j = 0; j < PORTAL_ID_MAX_NUM; ++j)
		{
			if (0 == rObjectData.abyPortalID[j])
				break;

			pkSet_iPortalNumber->insert(rObjectData.abyPortalID[j]);
		}
	}
}

void CAreaAccessor::AddObject(const TObjectData * c_pObjectData)
{
	TObjectInstance * pObjectInstance = ms_ObjectInstancePool.Alloc();
	pObjectInstance->Clear();
	CArea::__SetObjectInstance(pObjectInstance, c_pObjectData);

	m_ObjectDataVector.push_back(*c_pObjectData);
	m_ObjectInstanceVector.push_back(pObjectInstance);

	Refresh();
}

bool CAreaAccessor::GetObjectPointer(uint32_t dwIndex, TObjectData ** ppObjectData)
{
	if (!CheckObjectIndex(dwIndex))
	{
		assert(!"CAreaAccessor::GetObjectPointer() : Object Index overflow!");
		return false;
	}

	*ppObjectData = &m_ObjectDataVector[dwIndex];
	return true;
}

bool CAreaAccessor::Save(const std::string & c_rstrMapName)
{
	uint32_t dwID = (uint32_t) (m_wX) * 1000L + (uint32_t) (m_wY);

	char szFileName[256];
	sprintf(szFileName, "%s\\%06u\\AreaData.txt", c_rstrMapName.c_str(), dwID);

	char szAmbiencePathName[256+1];
	char szAmbienceFileName[256+1];

	if (!__SaveObjects(szFileName))
		return false;

	_snprintf(szAmbiencePathName, 256, "%s\\%06u\\", c_rstrMapName.c_str(), dwID);
	_snprintf(szAmbienceFileName, 256, "%s\\%06u\\AreaAmbienceData.txt", c_rstrMapName.c_str(), dwID);
	if (!__SaveAmbiences(szAmbiencePathName, szAmbienceFileName))
		return false;

//	_snprintf(szAmbiencePathName, 256, "Ambience_%s\\%06u\\", c_rstrMapName.c_str(), dwID);
//	_snprintf(szAmbienceFileName, 256, "Ambience_%s\\%06u\\AreaAmbienceData.txt", c_rstrMapName.c_str(), dwID);
//	if (!__SaveAmbiences(szAmbiencePathName, szAmbienceFileName))
//		return false;

	return true;
}

bool CAreaAccessor::__SaveObjects(const char * c_szFileName)
{
	FILE * File = fopen(c_szFileName, "w");

	if (!File)
		return false;

	fprintf(File, "AreaDataFile\n");
	fprintf(File, "\n");

	uint32_t dwObjectDataCount = 0;

	for (uint32_t i = 0; i < GetObjectDataCount(); ++i)
	{
		const TObjectData * c_pObjectData;

		if (!GetObjectDataPointer(i, &c_pObjectData))
			continue;

		CProperty * pProperty;
		if (!CPropertyManager::Instance().Get(c_pObjectData->dwCRC, &pProperty))
			continue;
		const char * c_szPropertyType;
		if (!pProperty->GetString("PropertyType", &c_szPropertyType))
			continue;

		uint32_t dwPropertyType = prt::GetPropertyType(c_szPropertyType);

		if (prt::PROPERTY_TYPE_AMBIENCE == dwPropertyType)
			continue;

		fprintf(File, "Start Object%03d\n", dwObjectDataCount);
		fprintf(File, "    %f %f %f\n", c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z);
		fprintf(File, "    %u\n", c_pObjectData->dwCRC);
		fprintf(File, "    %f#%f#%f\n", c_pObjectData->m_fYaw, c_pObjectData->m_fPitch, c_pObjectData->m_fRoll);
		fprintf(File, "    %f\n", c_pObjectData->m_fHeightBias);

		if (0 != c_pObjectData->abyPortalID[0])
		{
			std::set<int32_t> kSet_iPortalID;

			fprintf(File, "   ");
			for (int32_t p = 0; p < PORTAL_ID_MAX_NUM; ++p)
			{
				uint8_t byPortalID = c_pObjectData->abyPortalID[p];
				if (0 == byPortalID)
					break;
				if (kSet_iPortalID.end() != kSet_iPortalID.find(byPortalID))
					continue;

				fprintf(File, " %d", byPortalID);
				kSet_iPortalID.insert(byPortalID);
			}
			fprintf(File, "\n");
		}

		fprintf(File, "End Object\n");

		++dwObjectDataCount;
	}

	fprintf(File, "\n");
	fprintf(File, "ObjectCount %d\n", dwObjectDataCount);

	fclose(File);
	return true;
}

bool CAreaAccessor::__SaveAmbiences(const char * c_szPathName, const char * c_szFileName)
{
	if (-1 == _access(c_szPathName, 0))
		MyCreateDirectory(c_szPathName);

	//////////////////////////////////////////////////////

	FILE * File = fopen(c_szFileName, "w");

	if (!File)
		return false;

	fprintf(File, "AreaAmbienceDataFile\n");
	fprintf(File, "\n");

	uint32_t dwObjectDataCount = 0;

	for (uint32_t i = 0; i < GetObjectDataCount(); ++i)
	{
		const TObjectData * c_pObjectData;

		if (!GetObjectDataPointer(i, &c_pObjectData))
			continue;

		CProperty * pProperty;
		if (!CPropertyManager::Instance().Get(c_pObjectData->dwCRC, &pProperty))
			continue;
		const char * c_szPropertyType;
		if (!pProperty->GetString("PropertyType", &c_szPropertyType))
			continue;

		uint32_t dwPropertyType = prt::GetPropertyType(c_szPropertyType);

		if (prt::PROPERTY_TYPE_AMBIENCE == dwPropertyType)
		{
			fprintf(File, "Start Object%03d\n", dwObjectDataCount);
			fprintf(File, "    %f %f %f\n", c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z);
			fprintf(File, "    %u\n", c_pObjectData->dwCRC);
			fprintf(File, "    %u\n", c_pObjectData->dwRange);
			fprintf(File, "    %f\n", c_pObjectData->fMaxVolumeAreaPercentage);
			fprintf(File, "End Object\n");

			++dwObjectDataCount;
		}
	}

	fprintf(File, "\n");
	fprintf(File, "ObjectCount %d\n", dwObjectDataCount);

	fclose(File);
	return true;
}

bool CAreaAccessor::SaveCollisionData(const char * c_szLoadingAreaFileName, FILE * SavingFile)
{
	uint32_t dwPlaneCount = 0;

	std::vector<CBaseCollisionInstance * > CollisionInstanceVector;
	Clear();

	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(c_szLoadingAreaFileName, stTokenVectorMap))
	{
		fwrite(&dwPlaneCount, 1, sizeof(uint32_t), SavingFile);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("areadatafile"))
	{
		fwrite(&dwPlaneCount, 1, sizeof(uint32_t), SavingFile);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("objectcount"))
	{
		fwrite(&dwPlaneCount, 1, sizeof(uint32_t), SavingFile);
		return false;
	}

	const std::string & c_rstrCount = stTokenVectorMap["objectcount"][0];

	uint32_t dwCount = atoi(c_rstrCount.c_str());

	m_ObjectDataVector.clear();
	uint32_t dwSize = m_ObjectDataVector.size();

	char szObjectName[32 + 1];

	for (uint32_t i = 0; i < dwCount; ++i)
	{
		_snprintf(szObjectName, 32, "object%03d", i);

		if (stTokenVectorMap.end() == stTokenVectorMap.find(szObjectName))
			continue;

		const CTokenVector & rVector = stTokenVectorMap[szObjectName];

		const std::string & c_rstrxPosition = rVector[0].c_str();
		const std::string & c_rstryPosition = rVector[1].c_str();
		const std::string & c_rstrzPosition = rVector[2].c_str();
		const std::string & c_rstrCRC = rVector[3].c_str();

		TObjectData ObjectData;
		ObjectData.Position.x = atof(c_rstrxPosition.c_str());
		ObjectData.Position.y = atof(c_rstryPosition.c_str());
		ObjectData.Position.z = atof(c_rstrzPosition.c_str());
		ObjectData.dwCRC = atoi(c_rstrCRC.c_str());

		ObjectData.m_fYaw = ObjectData.m_fPitch = ObjectData.m_fRoll = 0;

		if (rVector.size() > 4)
		{
			std::string::size_type s=rVector[4].find('#');
			if (s!=rVector[4].npos)
			{
				ObjectData.m_fYaw = atoi(rVector[4].substr(0,s-1).c_str());
				int32_t p = s+1;
				s = rVector[4].find('#',p);
				ObjectData.m_fPitch = atoi(rVector[4].substr(p,s-1-p+1).c_str());
				ObjectData.m_fRoll = atoi(rVector[4].substr(s+1).c_str());
			}
			else
			{
				ObjectData.m_fYaw = 0.0f;
				ObjectData.m_fPitch = 0.0f;
				ObjectData.m_fRoll = atoi(rVector[4].c_str());
			}
		}

		ObjectData.m_fHeightBias = 0;
		if (rVector.size() > 5)
			ObjectData.m_fHeightBias = atoi(rVector[5].c_str());

		// If data is not inside property, then delete it.
		CProperty * pProperty;

		if (!CPropertyManager::Instance().Get(ObjectData.dwCRC, &pProperty))
			continue;

		m_ObjectDataVector.push_back(ObjectData);
	}

	/////
	for (uint32_t j = 0; j < GetObjectDataCount(); ++j)
	{
		const TObjectData * c_pObjectData;

		if (!GetObjectDataPointer(j, &c_pObjectData))
			continue;

		CProperty * pProperty;

		if (!CSingleton<CPropertyManager>::Instance().Get(c_pObjectData->dwCRC, &pProperty))
			continue;

		const char * c_szPropertyType;

		if (!pProperty->GetString("PropertyType", &c_szPropertyType))
			continue;

		D3DXMATRIX matWorld;
		D3DXMATRIX matTranslation;
		D3DXMATRIX matRotation;

		switch (prt::GetPropertyType(c_szPropertyType))
		{
			case prt::PROPERTY_TYPE_TREE:
			{
				const char * c_szTreeName;

				if (!pProperty->GetString("TreeFile", &c_szTreeName))
					continue;

				const char * c_szTreeSize;

				if (!pProperty->GetString("TreeSize", &c_szTreeSize))
					continue;

				CSpeedTreeWrapper * pMainTree;

				if (!CSpeedTreeForestDirectX8::Instance().GetMainTree(c_pObjectData->dwCRC, &pMainTree, c_szTreeName))
				{
					TraceError("Cannot get main tree from forest (filename: %s)", c_szTreeName);
					return false;
				}

				TObjectInstance * pObjectInstance = new TObjectInstance;

				// Make Tree Instance
				pObjectInstance->pTree = pMainTree->MakeInstance();
				pObjectInstance->pTree->SetPosition(c_pObjectData->Position.x,
													c_pObjectData->Position.y,
													c_pObjectData->Position.z + c_pObjectData->m_fHeightBias);

				pObjectInstance->pTree->UpdateCollisionData();
				for(uint32_t i=0; i<pObjectInstance->pTree->GetCollisionInstanceCount();i++)
				{
					if (pObjectInstance->pTree->GetCollisionInstanceCount() >= 2)
					if (dynamic_cast<CSphereCollisionInstance*>(pObjectInstance->pTree->GetCollisionInstanceData(i)))
						continue;

					CollisionInstanceVector.push_back(pObjectInstance->pTree->GetCollisionInstanceData(i));
				}

				delete pObjectInstance;

				break;
			}

			case prt::PROPERTY_TYPE_BUILDING:
			{
				const char * c_szBuildingfileName;
				if (!pProperty->GetString("buildingfile", &c_szBuildingfileName))
				{
					ms_NonAttributeObjectSet.insert(pProperty->GetFileName());
					break;
				}

				std::string strBuildFileName = c_szBuildingfileName;
				std::string strAttributeFileName = CFileNameHelper::NoExtension(strBuildFileName) + ".mdatr";

				if (!CResourceManager::Instance().IsFileExist(strAttributeFileName.c_str()))
				{
					ms_NonAttributeObjectSet.insert(pProperty->GetFileName());
					break;
				}

				D3DXMatrixTranslation(&matTranslation, c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z + c_pObjectData->m_fHeightBias);
				D3DXMatrixRotationYawPitchRoll(&matRotation, D3DXToRadian(c_pObjectData->m_fYaw), D3DXToRadian(c_pObjectData->m_fPitch), D3DXToRadian(c_pObjectData->m_fRoll));
				matWorld = matRotation * matTranslation;

				CAttributeData * pAttributeData = (CAttributeData *)CResourceManager::Instance().GetResourcePointer(strAttributeFileName.c_str());
				pAttributeData->AddReference();
				CGraphicThingInstance * pThingInstance = new CGraphicThingInstance;
				const CStaticCollisionDataVector & v = pAttributeData->GetCollisionDataVector();
				CStaticCollisionDataVector::const_iterator it;

				for(it = v.begin();it!=v.end();++it)
					pThingInstance->AddCollision(&(*it),&matWorld);

				for(uint32_t i=0; i<pThingInstance->GetCollisionInstanceCount();i++)
				{
					CollisionInstanceVector.push_back(pThingInstance->GetCollisionInstanceData(i));
				}

				pAttributeData->Release();
				delete pThingInstance;

				/////

				std::vector<TCollisionDataCounter>::iterator itor;
				for (itor = ms_CollisionDataCountVec.begin(); itor != ms_CollisionDataCountVec.end(); ++itor)
				{
					if (itor->stName == strAttributeFileName.c_str())
						break;
				}

				if (itor == ms_CollisionDataCountVec.end())
				{
					TCollisionDataCounter col;
					col.stName = strAttributeFileName.c_str();
					col.count = pAttributeData->GetCollisionDataVector().size();
					ms_CollisionDataCountVec.push_back(col);
				}

				break;
			}
		}
	}

	/////
	uint8_t byCollisionDataType;
	uint32_t dwCollisionDataCount = CollisionInstanceVector.size();//GetPlaneCount() + GetCylinderCount();
	fwrite(&dwCollisionDataCount, 1, sizeof(uint32_t), SavingFile);

	for(uint32_t iData=0; iData<dwCollisionDataCount;iData++)
	{
		CBaseCollisionInstance * pBase = CollisionInstanceVector[iData];
		if (CSphereCollisionInstance * pSphere = dynamic_cast<CSphereCollisionInstance*>(pBase))
		{
			byCollisionDataType = COLLISION_TYPE_SPHERE;
			fwrite(&byCollisionDataType, 1, sizeof(uint8_t), SavingFile);
			fwrite(&pSphere->GetAttribute(), 1, sizeof(TSphereData), SavingFile);
		}
		else if (CPlaneCollisionInstance * pPlane = dynamic_cast<CPlaneCollisionInstance*>(pBase))
		{
			byCollisionDataType = COLLISION_TYPE_PLANE;
			fwrite(&byCollisionDataType, 1, sizeof(uint8_t), SavingFile);
			fwrite(&pPlane->GetAttribute(), 1, sizeof(TPlaneData), SavingFile);
		}
		else if (CCylinderCollisionInstance * pCylinder = dynamic_cast<CCylinderCollisionInstance*>(pBase))
		{
			byCollisionDataType = COLLISION_TYPE_CYLINDER;
			fwrite(&byCollisionDataType, 1, sizeof(uint8_t), SavingFile);
			fwrite(&pCylinder->GetAttribute(), 1, sizeof(TCylinderData), SavingFile);
		}
	}
	return true;
}

void CAreaAccessor::RefreshArea()
{
	__Load_BuildObjectInstances();
	Refresh();
}

void CAreaAccessor::UpdateObject(uint32_t dwIndex, const TObjectData * c_pObjectData)
{
	TObjectInstance * pObjectInstance;
	if (!GetInstancePointer(dwIndex, &pObjectInstance))
	{
		assert(!"Too large index to update!");
		return;
	}

	if (prt::PROPERTY_TYPE_TREE == pObjectInstance->dwType)
	{
		pObjectInstance->pTree->SetPosition(c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z + c_pObjectData->m_fHeightBias);
	}
	else if (prt::PROPERTY_TYPE_BUILDING == pObjectInstance->dwType)
	{
		pObjectInstance->pThingInstance->SetPosition(c_pObjectData->Position.x, c_pObjectData->Position.y, c_pObjectData->Position.z + c_pObjectData->m_fHeightBias);
		pObjectInstance->pThingInstance->Transform();
		pObjectInstance->pThingInstance->Update();
		pObjectInstance->pThingInstance->Deform();
	}
}

bool CAreaAccessor::CheckInstanceIndex(uint32_t dwIndex)
{
	if (dwIndex >= m_ObjectInstanceVector.size())
		return false;

	return true;
}

bool CAreaAccessor::GetInstancePointer(uint32_t dwIndex, TObjectInstance ** ppObjectInstance)
{
	if (!CheckInstanceIndex(dwIndex))
		return false;

	*ppObjectInstance = m_ObjectInstanceVector[dwIndex];
	return true;
}

struct FCompareSelectObjectList
{
	uint32_t m_dwCheckIndex;
	FCompareSelectObjectList(uint32_t dwCheckIndex) : m_dwCheckIndex(dwCheckIndex)
	{
	}
	int32_t operator () (const CAreaAccessor::SSelectObject & c_rSelectObject)
	{
		return c_rSelectObject.dwIndex == m_dwCheckIndex;
	}
};

BOOL CAreaAccessor::IsSelectedObject(uint32_t dwIndex)
{
	TSelectObjectList::iterator itor = std::find_if(m_SelectObjectList.begin(), m_SelectObjectList.end(), FCompareSelectObjectList(dwIndex));
	return itor != m_SelectObjectList.end();
}

void CAreaAccessor::__ClickObject(uint32_t dwIndex)
{
	if (IsSelectedObject(dwIndex))
	{
		__DeselectObject(dwIndex);
		return;
	}

	SelectObject(dwIndex);
}

int32_t CAreaAccessor::GetSelectedObjectCount()
{
	return m_SelectObjectList.size();
}

void CAreaAccessor::SelectObject(uint32_t dwIndex)
{
	if (dwIndex >= m_ObjectDataVector.size())
		return;

	if (IsSelectedObject(dwIndex))
		return;

	TObjectData & rObjectData = m_ObjectDataVector[dwIndex];

	SSelectObject SelectObject;
	SelectObject.dwIndex = dwIndex;
	SelectObject.dwCRC32 = rObjectData.dwCRC;
	m_SelectObjectList.push_back(SelectObject);

	__RefreshSelectedInfo();
}

void CAreaAccessor::__DeselectObject(uint32_t dwIndex)
{
	for (TSelectObjectList::iterator itor=m_SelectObjectList.begin(); itor!=m_SelectObjectList.end(); ++itor)
	{
		SSelectObject & rSelectObject = *itor;
		if (dwIndex == rSelectObject.dwIndex)
		{
			if (1 == m_SelectObjectList.size())
			{
				CancelSelect();
				return;
			}

			m_SelectObjectList.erase(itor);
			__RefreshSelectedInfo();
			return;
		}
	}
}

void CAreaAccessor::CancelSelect()
{
	m_SelectObjectList.clear();
	__RefreshSelectedInfo();
}

void CAreaAccessor::__RefreshSelectedInfo()
{
	CMapOutdoorAccessor * pMapAccessor = (CMapOutdoorAccessor *)GetOwner();
	pMapAccessor->ClearSelectedPortalNumber();
	if (m_SelectObjectList.empty())
	{
		pMapAccessor->SetSelectedObjectName("No object selected");
	}
	else
	{
		// Name
		std::string objectName = "";
		for (auto & rSelectObject : m_SelectObjectList)
		{
			TObjectData & rObjectData = m_ObjectDataVector[rSelectObject.dwIndex];

			CProperty * pProperty;
			if (CPropertyManager::Instance().Get(rObjectData.dwCRC, &pProperty))
			{
				objectName += _getf("[%u](%s)\n", pProperty->GetCRC(), pProperty->GetFileName());
			}
		}
		pMapAccessor->SetSelectedObjectName(objectName.c_str());

		// Portal Number
		std::map<uint32_t, uint32_t> kMap_iPortalNumber;
		for (TSelectObjectList::iterator itor=m_SelectObjectList.begin(); itor!=m_SelectObjectList.end(); ++itor)
		{
			SSelectObject & rSelectObject = *itor;
			TObjectData & rObjectData = m_ObjectDataVector[rSelectObject.dwIndex];

			CProperty * pProperty;
			if (!CPropertyManager::Instance().Get(rObjectData.dwCRC, &pProperty))
				continue;
			for (int32_t i = 0; i < PORTAL_ID_MAX_NUM; ++i)
			{
				uint8_t byNumber = rObjectData.abyPortalID[i];
				if (0 == byNumber)
					continue;

				if (kMap_iPortalNumber.end() == kMap_iPortalNumber.find(byNumber))
				{
					kMap_iPortalNumber[byNumber] = 0;
				}

				++kMap_iPortalNumber[byNumber];
			}
		}

		std::map<uint32_t, uint32_t>::iterator itorNum = kMap_iPortalNumber.begin();
		for (; itorNum != kMap_iPortalNumber.end(); ++itorNum)
		{
			if (itorNum->second >= m_SelectObjectList.size())
			{
				uint8_t byNumber = itorNum->first;
				pMapAccessor->AddSelectedObjectPortalNumber(byNumber);
			}
		}
	}
}

const CArea::TObjectData* CAreaAccessor::GetLastSelectedObjectData() const
{
	if (m_SelectObjectList.empty())
		return NULL;

	const SSelectObject& rSelectObject = m_SelectObjectList.back();

	return &m_ObjectDataVector[rSelectObject.dwIndex];
}

void CAreaAccessor::__RefreshObjectPosition(float fx, float fy, float fz)
{
	TSelectObjectList::iterator itor;
	for (itor = m_SelectObjectList.begin(); itor != m_SelectObjectList.end(); ++itor)
	{
		SSelectObject & rSelectObject = *itor;

		TObjectData * pObjectData;
		if (!GetObjectPointer(rSelectObject.dwIndex, &pObjectData))
			continue;

		D3DXVECTOR3 & rv3Position = pObjectData->Position;
		rv3Position.x += fx;
		rv3Position.y += fy;
		rv3Position.z += fz;

		////////////////////////////
		// SetPosition
		TObjectInstance * pObjectInstance;
		if (!GetInstancePointer(rSelectObject.dwIndex, &pObjectInstance))
			continue;

		if (pObjectInstance->dwEffectInstanceIndex!=0xffffffff)
		{
			TEffectInstanceIterator f = m_EffectInstanceMap.find(pObjectInstance->dwEffectInstanceIndex);
			if (m_EffectInstanceMap.end()!=f)
			{
				CEffectInstance * pEffectInstance = f->second;
				pEffectInstance->SetPosition(rv3Position.x, rv3Position.y, rv3Position.z);
			}
		}
		if (pObjectInstance->pTree)
		{
			pObjectInstance->pTree->SetPosition(rv3Position.x, rv3Position.y, rv3Position.z);
		}
		if (pObjectInstance->pThingInstance)
		{
			pObjectInstance->pThingInstance->SetPosition(rv3Position.x, rv3Position.y, rv3Position.z);
			pObjectInstance->pThingInstance->Update();
			pObjectInstance->pThingInstance->Transform();
			pObjectInstance->pThingInstance->Deform();
		}
		if (pObjectInstance->pDungeonBlock)
		{
			pObjectInstance->pDungeonBlock->SetPosition(rv3Position.x, rv3Position.y, rv3Position.z);
			pObjectInstance->pDungeonBlock->Update();
			pObjectInstance->pDungeonBlock->Transform();
			pObjectInstance->pDungeonBlock->Deform();
		}
		if (pObjectInstance->pAmbienceInstance)
		{
			pObjectInstance->pAmbienceInstance->fx = rv3Position.x;
			pObjectInstance->pAmbienceInstance->fy = rv3Position.y;
			pObjectInstance->pAmbienceInstance->fz = rv3Position.z;
		}
	}
}

//////////
// Log
void CAreaAccessor::OpenCollisionDataCountMapLog()
{
	ms_LogFile = fopen("MapDataLog.txt", "w");
}

static bool CountCompare(const CAreaAccessor::TCollisionDataCounter & lhs, const CAreaAccessor::TCollisionDataCounter & rhs)
{
	return (lhs.count > rhs.count) ? true : false;
}

void CAreaAccessor::CloseCollisionDataCountMapLog()
{
	fprintf(ms_LogFile, " << Collision Count List >>\n");
	std::sort(ms_CollisionDataCountVec.begin(), ms_CollisionDataCountVec.end(), CountCompare);
	std::for_each(ms_CollisionDataCountVec.begin(), ms_CollisionDataCountVec.end(), PrintCounter);
	fprintf(ms_LogFile, "\n");

	fprintf(ms_LogFile, " << Non Attribute Object List >>\n");
	std::set<std::string>::iterator itor = ms_NonAttributeObjectSet.begin();
	for (; itor != ms_NonAttributeObjectSet.end(); ++itor)
	{
		fprintf(ms_LogFile, "%s\n", (*itor).c_str());
	}
	fclose(ms_LogFile);
}
// Log
//////////

//////////////////////////////////////////////////////////////////////////
// ShadowMap
void CAreaAccessor::RenderToShadowMap()
{
	for (uint32_t i = 0; i < m_ShadowThingCloneInstaceVector.size(); ++i)
		m_ShadowThingCloneInstaceVector[i]->RenderToShadowMap();
}
// ShadowMap
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
void CAreaAccessor::ReloadBuildingTexture()
{
	CGraphicThingInstance* pkThingInst;
	TThingInstanceVector::iterator i=m_ThingCloneInstaceVector.begin();
	while (i!=m_ThingCloneInstaceVector.end())
	{
		pkThingInst=*i++;
		pkThingInst->ReloadTexture();
	}
}
//////////////////////////////////////////////////////////////////////////

void CAreaAccessor::Clear()
{
	m_SelectObjectList.clear();
	CArea::Clear();
}

CAreaAccessor::CAreaAccessor() : CArea()
{
}

CAreaAccessor::~CAreaAccessor()
{
}
