#include "StdAfx.h"
#include <EterLib/ResourceManager.h>
#include <EterLib/StateManager.h>
#include <EffectLib/EffectManager.h>
#include <EterBase/Timer.h>
#include <SpeedTreeLib/SpeedTreeForestDirectX9.h>
#include <MilesLib/SoundManager.h>

#include "Area.h"
#include "PropertyManager.h"
#include "Property.h"
#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <execution>

using namespace prt;

void CArea::SObjectData::InitializeRotation()
{
	m_fYaw = m_fPitch = m_fRoll = 0.0f;
}

CArea::SObjectInstance::~SObjectInstance()
{
	if (pTree)
	{
		auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
		rkForest->DeleteInstance(pTree);
	}

	if (pThingInstance)
		CGraphicThingInstance::Delete(pThingInstance);

	if (effect)
		CEffectInstance::Delete(effect);
}

CArea::SObjectInstance::SObjectInstance(SObjectInstance&& other)
	: dwType(std::move(other.dwType)), map(other.map), pTree(std::move(other.pTree)),
	isShadowFlag(std::move(other.isShadowFlag)), pThingInstance(std::move(other.pThingInstance)),
	pAttributeInstance(std::move(other.pAttributeInstance)), effect(std::move(other.effect)),
	pAmbienceInstance(std::move(other.pAmbienceInstance)), pDungeonBlock(std::move(other.pDungeonBlock))
{
	other.dwType = PROPERTY_TYPE_NONE;
	other.pTree = nullptr;
	other.pThingInstance = nullptr;
	other.effect = nullptr;
	other.map = nullptr;
}

CArea::SObjectInstance& CArea::SObjectInstance::operator=(SObjectInstance&& other)
{
	dwType = std::move(other.dwType);
	pTree = std::move(other.pTree);
	isShadowFlag = std::move(other.isShadowFlag);
	pThingInstance = std::move(other.pThingInstance);
	pAttributeInstance = std::move(other.pAttributeInstance);
	effect = std::move(other.effect);
	pAmbienceInstance = std::move(other.pAmbienceInstance);
	pDungeonBlock = std::move(other.pDungeonBlock);
	map = other.map;

	other.dwType = PROPERTY_TYPE_NONE;
	other.pTree = nullptr;
	other.pThingInstance = nullptr;
	other.effect = nullptr;
	other.map = nullptr;

	return *this;
}

void CArea::__UpdateAniThingList()
{
	{
		CGraphicThingInstance* pkThingInst;

		TThingInstanceVector::iterator i = m_ThingCloneInstaceVector.begin();
		while (i != m_ThingCloneInstaceVector.end())
		{
			pkThingInst = *i++;
			if (pkThingInst->isShow())
			{
				pkThingInst->UpdateLODLevel();
			}
		}
	}

	{
		CGraphicThingInstance* pkThingInst;

		TThingInstanceVector::iterator i = m_AniThingCloneInstanceVector.begin();
		while (i != m_AniThingCloneInstanceVector.end())
		{
			pkThingInst = *i++;
			pkThingInst->Deform();
			pkThingInst->Update();
		}
	}
}

void CArea::__UpdateEffectList()
{
	for (auto i = m_effectInstances.begin(); i != m_effectInstances.end();)
	{
		auto pEffectInstance = i->second;

		pEffectInstance->Update();

		if (pEffectInstance->isAlive())
		{
			++i;
			continue;
		}

		i->first->effect = nullptr;

		i = m_effectInstances.erase(i);
		CEffectInstance::Delete(pEffectInstance);
	}
}

void CArea::Update()
{
	__UpdateAniThingList();
}

void CArea::UpdateAroundAmbience(float fX, float fY, float fZ)
{
	// Ambience
	TAmbienceInstanceVector::iterator i;
	for (i = m_AmbienceCloneInstanceVector.begin(); i != m_AmbienceCloneInstanceVector.end(); ++i)
	{
		TAmbienceInstance* pInstance = *i;
		pInstance->__Update(fX, fY, fZ);
	}
}

struct CArea_LessEffectInstancePtrRenderOrder
{
	bool operator()(CEffectInstance* pkLeft, CEffectInstance* pkRight)
	{
		return pkLeft->LessRenderOrder(pkRight);
	}
};

struct CArea_FEffectInstanceRender
{
	inline void operator()(CEffectInstance* pkEftInst)
	{
		pkEftInst->Render();
	}
};

void CArea::RenderEffect()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CArea::RenderEffect **");

	__UpdateEffectList();

	STATEMANAGER->SetTexture(0, NULL);
	STATEMANAGER->SetTexture(1, NULL);

	static std::vector<CEffectInstance*> effects;
	effects.reserve(m_effectInstances.size());
	effects.clear();

	for (auto& inst : m_effectInstances)
	{
		if (inst.second->isShow())
			effects.emplace_back(inst.second);
	}

	std::sort(std::execution::par, effects.begin(), effects.end(), CArea_LessEffectInstancePtrRenderOrder());
	std::for_each(effects.begin(), effects.end(), CArea_FEffectInstanceRender());

	D3DPERF_EndEvent();
}

void CArea::CollectRenderingObject(std::vector<CGraphicThingInstance*>& rkVct_pkOpaqueThingInst)
{
	for (auto& i : m_ThingCloneInstaceVector)
	{
		auto pkThingInst = i;
		if (pkThingInst->isShow())
		{
			if (!pkThingInst->HaveBlendThing())
				rkVct_pkOpaqueThingInst.emplace_back(i);
		}
	}
}

void CArea::CollectBlendRenderingObject(std::vector<CGraphicThingInstance*>& rkVct_pkBlendThingInst)
{
	TThingInstanceVector::iterator i;
	for (i = m_ThingCloneInstaceVector.begin(); i != m_ThingCloneInstaceVector.end(); ++i)
	{
		CGraphicThingInstance* pkThingInst = *i;
		if (pkThingInst->isShow())
		{
			if (pkThingInst->HaveBlendThing())
				rkVct_pkBlendThingInst.emplace_back(*i);
		}
	}
}

void CArea::Render()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** CArea::Render **");

	{
		CGraphicThingInstance* pkThingInst;

		TThingInstanceVector::iterator i = m_AniThingCloneInstanceVector.begin();
		while (i != m_AniThingCloneInstanceVector.end())
		{
			pkThingInst = *i++;
			pkThingInst->Deform();
		}
	}

	CGraphicThingInstance* pkThingInst;

	TThingInstanceVector::iterator i = m_ThingCloneInstaceVector.begin();

	while (i != m_ThingCloneInstaceVector.end())
	{
		pkThingInst = *i++;
		pkThingInst->Render();
	}

	D3DPERF_EndEvent();
}

void CArea::RenderCollision()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** Area RenderCollision **");
	uint32_t i;

	STATEMANAGER->SetTexture(0, NULL);
	STATEMANAGER->SetTexture(1, NULL);
	DWORD lighting = STATEMANAGER->GetRenderState(D3DRS_LIGHTING);

	STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, false);
	STATEMANAGER->SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	STATEMANAGER->SetRenderState(D3DRS_LIGHTING, false);
	STATEMANAGER->SetRenderState(D3DRS_TEXTUREFACTOR, 0xff000000);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	for (i = 0; i < GetObjectInstanceCount(); i++)
	{
		const TObjectInstance* po;
		if ((po = GetObjectInstancePointer(i)))
		{
			if (po->pTree && po->pTree->isShow())
			{
				uint32_t j;
				for (j = 0; j < po->pTree->GetCollisionInstanceCount(); j++)
				{
					po->pTree->GetCollisionInstanceData(j)->Render();
				}
			}
			if (po->pThingInstance && po->pThingInstance->isShow())
			{
				uint32_t j;
				for (j = 0; j < po->pThingInstance->GetCollisionInstanceCount(); j++)
				{
					po->pThingInstance->GetCollisionInstanceData(j)->Render();
				}
			}
			if (po->pDungeonBlock && po->pDungeonBlock->isShow())
			{
				uint32_t j;
				for (j = 0; j < po->pDungeonBlock->GetCollisionInstanceCount(); j++)
				{
					po->pDungeonBlock->GetCollisionInstanceData(j)->Render();
				}
			}
		}
	}

	STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_CULLMODE);

	D3DPERF_EndEvent();
}

void CArea::RenderAmbience()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** RenderAmbience **");

	DWORD dwColorArg1, dwColorOp;
	STATEMANAGER->GetTextureStageState(0, D3DTSS_COLORARG1, &dwColorArg1);
	STATEMANAGER->GetTextureStageState(0, D3DTSS_COLOROP, &dwColorOp);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	TAmbienceInstanceVector::iterator itor = m_AmbienceCloneInstanceVector.begin();
	for (; itor != m_AmbienceCloneInstanceVector.end(); ++itor)
	{
		TAmbienceInstance* pInstance = *itor;
		pInstance->Render();
	}
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, dwColorArg1);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, dwColorOp);
	D3DPERF_EndEvent();
}

void CArea::RenderDungeon()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** Render Dungeon **");
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

	for (const auto& clone : m_DungeonBlockCloneInstanceVector)
		clone->Render();

	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	D3DPERF_EndEvent();
}

void CArea::Refresh()
{
	m_ThingCloneInstaceVector.clear();
	m_DungeonBlockCloneInstanceVector.clear();
	m_AniThingCloneInstanceVector.clear();
	m_ShadowThingCloneInstaceVector.clear();
	m_AmbienceCloneInstanceVector.clear();

	for (auto& pObjectInstance : m_ObjectInstanceVector)
	{
		if (PROPERTY_TYPE_TREE == pObjectInstance.dwType)
		{
			if (pObjectInstance.pTree)
			{
				const float* pfPosition;
				pfPosition = pObjectInstance.pTree->GetPosition();
				pObjectInstance.pTree->UpdateBoundingSphere();
				pObjectInstance.pTree->UpdateCollisionData();
			}
		}
		else if (PROPERTY_TYPE_BUILDING == pObjectInstance.dwType)
		{
			pObjectInstance.pThingInstance->Update();
			pObjectInstance.pThingInstance->Transform();
			pObjectInstance.pThingInstance->Show();
			pObjectInstance.pThingInstance->Deform();
			m_ThingCloneInstaceVector.emplace_back(pObjectInstance.pThingInstance);

			pObjectInstance.pThingInstance->BuildBoundingSphere();
			pObjectInstance.pThingInstance->UpdateBoundingSphere();

			if (pObjectInstance.pThingInstance->IsMotionThing())
			{
				m_AniThingCloneInstanceVector.emplace_back(pObjectInstance.pThingInstance);
				pObjectInstance.pThingInstance->SetMotion(0);
			}

			if (pObjectInstance.isShadowFlag)
				m_ShadowThingCloneInstaceVector.emplace_back(pObjectInstance.pThingInstance);

			if (pObjectInstance.pAttributeInstance)
			{
				pObjectInstance.pThingInstance->UpdateCollisionData(&pObjectInstance.pAttributeInstance->GetObjectPointer()->GetCollisionDataVector());
				pObjectInstance.pAttributeInstance->RefreshObject(pObjectInstance.pThingInstance->GetTransform());
				pObjectInstance.pThingInstance->UpdateHeightInstance(pObjectInstance.pAttributeInstance.get());
			}
		}
		else if (PROPERTY_TYPE_EFFECT == pObjectInstance.dwType)
		{
		}
		else if (PROPERTY_TYPE_AMBIENCE == pObjectInstance.dwType)
		{
			m_AmbienceCloneInstanceVector.emplace_back(pObjectInstance.pAmbienceInstance.get());
		}
		else if (PROPERTY_TYPE_DUNGEON_BLOCK == pObjectInstance.dwType)
		{
			pObjectInstance.pDungeonBlock->Update();
			pObjectInstance.pDungeonBlock->Deform();
			pObjectInstance.pDungeonBlock->UpdateBoundingSphere();
			m_DungeonBlockCloneInstanceVector.emplace_back(pObjectInstance.pDungeonBlock.get());

			if (pObjectInstance.pAttributeInstance)
			{
				pObjectInstance.pDungeonBlock->UpdateCollisionData(&pObjectInstance.pAttributeInstance->GetObjectPointer()->GetCollisionDataVector());
				pObjectInstance.pAttributeInstance->RefreshObject(pObjectInstance.pDungeonBlock->GetTransform());
				pObjectInstance.pDungeonBlock->UpdateHeightInstance(pObjectInstance.pAttributeInstance.get());
			}
		}
	}
}

void CArea::__Load_BuildObjectInstances()
{
	const auto count = m_ObjectDataVector.size();
	std::sort(m_ObjectDataVector.begin(), m_ObjectDataVector.end(), ObjectDataComp());

	m_ObjectInstanceVector.clear();
	m_ObjectInstanceVector.resize(count);

	for (std::size_t i = 0; i != count; ++i)
	{
		m_ObjectInstanceVector[i].map = m_pOwnerOutdoorMap;
		__SetObjectInstance(&m_ObjectInstanceVector[i], &m_ObjectDataVector[i]);
	}

	Refresh();
}

void CArea::__SetObjectInstance(TObjectInstance* pObjectInstance, const TObjectData* c_pData)
{
	CProperty* pProperty;
	if (!CPropertyManager::GetInstance()->Get(c_pData->dwCRC, &pProperty))
		return;

	const char* c_szPropertyType;

	if (!pProperty->GetString("PropertyType", &c_szPropertyType))
		return;

	switch (GetPropertyType(c_szPropertyType))
	{
	case PROPERTY_TYPE_TREE:
		__SetObjectInstance_SetTree(pObjectInstance, c_pData, pProperty);
		break;

	case PROPERTY_TYPE_BUILDING:
		__SetObjectInstance_SetBuilding(pObjectInstance, c_pData, pProperty);
		break;

	case PROPERTY_TYPE_EFFECT:
		__SetObjectInstance_SetEffect(pObjectInstance, c_pData, pProperty);
		break;

	case PROPERTY_TYPE_AMBIENCE:
		__SetObjectInstance_SetAmbience(pObjectInstance, c_pData, pProperty);
		break;

	case PROPERTY_TYPE_DUNGEON_BLOCK:
		__SetObjectInstance_SetDungeonBlock(pObjectInstance, c_pData, pProperty);
		break;
	default:
		break;
	}
}

void CArea::__SetObjectInstance_SetEffect(TObjectInstance* pObjectInstance, const TObjectData* c_pData, CProperty* pProperty)
{
	TPropertyEffect Data;
	if (!PropertyEffectStringToData(pProperty, &Data))
		return;

	const auto effectID = GetCaseCRC32(Data.strFileName.c_str(), Data.strFileName.size());

	auto rem = CEffectManager::GetInstance();

	CEffectData* data;
	if (!rem->GetEffectData(effectID, &data))
	{
		if (!rem->RegisterEffect(Data.strFileName.c_str()))
		{
			SysLog("CArea::SetEffect effect register error {0}", Data.strFileName.c_str());
			return;
		}

		rem->GetEffectData(effectID, &data);
	}

	pObjectInstance->dwType = PROPERTY_TYPE_EFFECT;

	auto* pEffectInstance = CEffectInstance::New();
	pEffectInstance->SetEffectDataPointer(data);

	D3DXMATRIX mat;
	D3DXMatrixRotationYawPitchRoll(&mat,
		D3DXToRadian(c_pData->m_fYaw),
		D3DXToRadian(c_pData->m_fPitch),
		D3DXToRadian(c_pData->m_fRoll)
	);

	mat._41 = c_pData->Position.x;
	mat._42 = c_pData->Position.y;
	mat._43 = c_pData->Position.z + c_pData->m_fHeightBias;

	pEffectInstance->SetGlobalMatrix(mat);
	pObjectInstance->effect = pEffectInstance;
	m_effectInstances.emplace(pObjectInstance, pEffectInstance);
}

void CArea::__SetObjectInstance_SetTree(TObjectInstance* pObjectInstance, const TObjectData* c_pData, CProperty* pProperty)
{
	const char* c_szTreeName;
	if (!pProperty->GetString("TreeFile", &c_szTreeName))
		return;

	pObjectInstance->SetTree(
		c_pData->Position.x,
		c_pData->Position.y,
		c_pData->Position.z + c_pData->m_fHeightBias,
		c_pData->dwCRC,
		c_szTreeName
	);
}

void CArea::TObjectInstance::SetTree(float x, float y, float z, uint32_t dwTreeCRC, const char* c_szTreeName)
{
	if (map)
	{
		auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
		pTree = rkForest->CreateInstance(x, y, z, dwTreeCRC, c_szTreeName);
		if (!pTree)
			return;

		dwType = PROPERTY_TYPE_TREE;
	}
	else
	{
	}
}

void CArea::__SetObjectInstance_SetBuilding(TObjectInstance* pObjectInstance, const TObjectData* c_pData, CProperty* pProperty)
{
	TPropertyBuilding Data;
	if (!PropertyBuildingStringToData(pProperty, &Data))
		return;

	auto rkResMgr = CResourceManager::GetInstance();

	CGraphicThing* pThing = rkResMgr->LoadResource<CGraphicThing>(Data.strFileName);
	pThing->AddReference();

	if (pThing->IsEmpty())
	{
		ConsoleLog("CArea::SetBuilding: There is no data: {0}", Data.strFileName.c_str());
		return;
	}

	const int32_t iModelCount = pThing->GetModelCount();
	const int32_t iMotionCount = pThing->GetMotionCount();

	pObjectInstance->dwType = PROPERTY_TYPE_BUILDING;
	pObjectInstance->pThingInstance = CGraphicThingInstance::New();
	pObjectInstance->pThingInstance->Initialize();
	pObjectInstance->pThingInstance->ReserveModelThing(iModelCount);
	pObjectInstance->pThingInstance->ReserveModelInstance(iModelCount);
	pObjectInstance->pThingInstance->RegisterModelThing(0, pThing);
	for (int32_t j = 0; j < PORTAL_ID_MAX_NUM; ++j)
		if (0 != c_pData->abyPortalID[j])
			pObjectInstance->pThingInstance->SetPortal(j, c_pData->abyPortalID[j]);

	for (int32_t i = 0; i < iModelCount; ++i)
		pObjectInstance->pThingInstance->SetModelInstance(i, 0, i);

	if (iMotionCount)
		pObjectInstance->pThingInstance->RegisterMotionThing(pObjectInstance->pThingInstance->GetMotionID(), 0, pThing);

	pObjectInstance->pThingInstance->SetPosition(c_pData->Position.x, c_pData->Position.y, c_pData->Position.z + c_pData->m_fHeightBias);
	pObjectInstance->pThingInstance->SetRotation(c_pData->m_fYaw, c_pData->m_fPitch, c_pData->m_fRoll);
	pObjectInstance->isShadowFlag = Data.isShadowFlag;
	pObjectInstance->pThingInstance->RegisterBoundingSphere();
	__LoadAttribute(pObjectInstance, Data.strAttributeDataFileName.c_str());
	pThing->Release();
}

void CArea::__SetObjectInstance_SetAmbience(TObjectInstance* pObjectInstance, const TObjectData* c_pData, CProperty* pProperty)
{
	pObjectInstance->pAmbienceInstance.reset(new TAmbienceInstance);
	if (!PropertyAmbienceStringToData(pProperty, &pObjectInstance->pAmbienceInstance->AmbienceData))
		return;

	pObjectInstance->dwType = PROPERTY_TYPE_AMBIENCE;

	auto pAmbienceInstance = pObjectInstance->pAmbienceInstance.get();
	pAmbienceInstance->fx = c_pData->Position.x;
	pAmbienceInstance->fy = c_pData->Position.y;
	pAmbienceInstance->fz = c_pData->Position.z + c_pData->m_fHeightBias;
	pAmbienceInstance->dwRange = c_pData->dwRange;
	pAmbienceInstance->fMaxVolumeAreaPercentage = c_pData->fMaxVolumeAreaPercentage;

	if ("ONCE" == pAmbienceInstance->AmbienceData.strPlayType)
		pAmbienceInstance->Update = &TAmbienceInstance::UpdateOnceSound;
	else if ("STEP" == pAmbienceInstance->AmbienceData.strPlayType)
		pAmbienceInstance->Update = &TAmbienceInstance::UpdateStepSound;
	else if ("LOOP" == pAmbienceInstance->AmbienceData.strPlayType)
		pAmbienceInstance->Update = &TAmbienceInstance::UpdateLoopSound;
}

void CArea::__SetObjectInstance_SetDungeonBlock(TObjectInstance* pObjectInstance, const TObjectData* c_pData, CProperty* pProperty)
{
	TPropertyDungeonBlock Data;
	if (!PropertyDungeonBlockStringToData(pProperty, &Data))
		return;

	pObjectInstance->dwType = PROPERTY_TYPE_DUNGEON_BLOCK;
	pObjectInstance->pDungeonBlock.reset(new CDungeonBlock());
	pObjectInstance->pDungeonBlock->Load(Data.strFileName.c_str());
	pObjectInstance->pDungeonBlock->SetPosition(c_pData->Position.x, c_pData->Position.y, c_pData->Position.z + c_pData->m_fHeightBias);
	pObjectInstance->pDungeonBlock->SetRotation(c_pData->m_fYaw, c_pData->m_fPitch, c_pData->m_fRoll);
	pObjectInstance->pDungeonBlock->Update();
	pObjectInstance->pDungeonBlock->BuildBoundingSphere();
	pObjectInstance->pDungeonBlock->RegisterBoundingSphere();
	for (int32_t j = 0; j < PORTAL_ID_MAX_NUM; ++j)
		if (0 != c_pData->abyPortalID[j])
			pObjectInstance->pDungeonBlock->SetPortal(j, c_pData->abyPortalID[j]);
	__LoadAttribute(pObjectInstance, Data.strAttributeDataFileName.c_str());
}

void CArea::__LoadAttribute(TObjectInstance* pObjectInstance, const char* c_szAttributeFileName)
{
	CAttributeInstance* pAttrInstance = new CAttributeInstance;
	auto pAttributeData = CResourceManager::GetInstance()->LoadResource<CAttributeData>(c_szAttributeFileName);

	if (!pAttributeData)
	{
		std::string attrFileName(c_szAttributeFileName);
		boost::algorithm::to_lower(attrFileName);
		const bool bIsDungeonObject = (std::string::npos != attrFileName.find("/dungeon/")) || (std::string::npos != attrFileName.find("\\dungeon\\"));

		pAttributeData = new CAttributeData(attrFileName);

		if (pAttributeData->IsEmpty() && false == bIsDungeonObject)
		{
			if (nullptr != pObjectInstance && nullptr != pObjectInstance->pThingInstance)
			{
				CGraphicThingInstance* object = pObjectInstance->pThingInstance;

				D3DXVECTOR3 v3Min, v3Max;

				object->GetBoundingAABB(v3Min, v3Max);

				CStaticCollisionData collision;
				collision.dwType = COLLISION_TYPE_OBB;
				D3DXQuaternionRotationYawPitchRoll(&collision.quatRotation, object->GetYaw(), object->GetPitch(), object->GetRoll());
				strcpy_s(collision.szName, "DummyCollisionOBB");
				collision.v3Position = (v3Min + v3Max) * 0.5f;

				D3DXVECTOR3 vDelta = (v3Max - v3Min);
				collision.fDimensions[0] = vDelta.x * 0.5f;
				collision.fDimensions[1] = vDelta.y * 0.5f;
				collision.fDimensions[2] = vDelta.z * 0.5f;

				pAttributeData->AddCollisionData(collision);
			}
		}
	}

	pAttrInstance->SetObjectPointer(pAttributeData);
	pObjectInstance->pAttributeInstance.reset(pAttrInstance);
}

bool CArea::Load(const char* c_szPathName)
{
	Clear();

	std::string strObjectDataFileName = c_szPathName + std::string("AreaData.txt");
	std::string strAmbienceDataFileName = c_szPathName + std::string("AreaAmbienceData.txt");

	__Load_LoadObject(strObjectDataFileName.c_str());
	__Load_LoadAmbience(strAmbienceDataFileName.c_str());
	__Load_BuildObjectInstances();

	return true;
}

bool CArea::__Load_LoadObject(const char* c_szFileName)
{
	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(c_szFileName, stTokenVectorMap))
	{
		SysLog("CArea::Load File Load {0} ERROR", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("areadatafile"))
	{
		SysLog("CArea::__LoadObject File Format {0} ERROR 1", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("objectcount"))
	{
		SysLog("CArea::__LoadObject File Format {0} ERROR 2", c_szFileName);
		return false;
	}

	const std::string& c_rstrCount = stTokenVectorMap["objectcount"][0];

	uint32_t dwCount = std::stof(c_rstrCount.c_str());

	char szObjectName[32 + 1];

	for (uint32_t i = 0; i < dwCount; ++i)
	{
		_snprintf(szObjectName, sizeof(szObjectName), "object%03d", i);

		if (stTokenVectorMap.end() == stTokenVectorMap.find(szObjectName))
			continue;

		const CTokenVector& rVector = stTokenVectorMap[szObjectName];

		const auto& c_rstrxPosition = rVector[0];
		const auto& c_rstryPosition = rVector[1];
		const auto& c_rstrzPosition = rVector[2];
		const auto& c_rstrCRC = rVector[3];

		TObjectData ObjectData;
		ZeroMemory(&ObjectData, sizeof(ObjectData));
		ObjectData.Position.x = std::stof(c_rstrxPosition.c_str());
		ObjectData.Position.y = std::stof(c_rstryPosition.c_str());
		ObjectData.Position.z = std::stof(c_rstrzPosition.c_str());
		ObjectData.dwCRC = atoi(c_rstrCRC.c_str());

		ObjectData.InitializeRotation();
		if (rVector.size() > 4)
		{
			std::string::size_type s = rVector[4].find('#');
			if (s != rVector[4].npos)
			{
				ObjectData.m_fYaw = std::stof(rVector[4].substr(0, s - 1));
				int p = s + 1;
				s = rVector[4].find('#', p);
				ObjectData.m_fPitch = std::stof(rVector[4].substr(p, s - 1 - p + 1).c_str());
				ObjectData.m_fRoll = std::stof(rVector[4].substr(s + 1).c_str());
			}
			else
			{
				ObjectData.m_fYaw = 0.0f;
				ObjectData.m_fPitch = 0.0f;
				ObjectData.m_fRoll = std::stof(rVector[4]);
			}
		}

		ObjectData.m_fHeightBias = 0.0f;
		if (rVector.size() > 5)
		{
			ObjectData.m_fHeightBias = std::stof(rVector[5]);
		}

		if (rVector.size() > 6)
		{
			for (auto portalIdx = 0; portalIdx < std::min<int>(rVector.size() - 6, PORTAL_ID_MAX_NUM); ++portalIdx)
			{
				ObjectData.abyPortalID[portalIdx] = std::stof(rVector[6 + portalIdx].c_str());
			}
		}

		// If data is not inside property, then delete it.
		CProperty* pProperty;
		if (!CPropertyManager::GetInstance()->Get(ObjectData.dwCRC, &pProperty))
		{
			SysLog("{0} Object{1}: Failed to find object with CRC {2}", c_szFileName, i, ObjectData.dwCRC);
			continue;
		}

		m_ObjectDataVector.emplace_back(ObjectData);
	}

	return true;
}

bool CArea::__Load_LoadAmbience(const char* c_szFileName)
{
	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(c_szFileName, stTokenVectorMap))
	{
		SysLog("CArea::Load File Load {0} ERROR", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("areaambiencedatafile"))
	{
		SysLog("CArea::__LoadAmbience File Format {0} ERROR 1", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("objectcount"))
	{
		SysLog("CArea::__LoadAmbience File Format {0} ERROR 2", c_szFileName);
		return false;
	}

	const std::string& c_rstrCount = stTokenVectorMap["objectcount"][0];

	uint32_t dwCount = std::stof(c_rstrCount.c_str());

	char szObjectName[32 + 1];

	for (uint32_t i = 0; i < dwCount; ++i)
	{
		_snprintf(szObjectName, sizeof(szObjectName), "object%03d", i);

		if (stTokenVectorMap.end() == stTokenVectorMap.find(szObjectName))
			continue;

		const CTokenVector& rVector = stTokenVectorMap[szObjectName];

		const std::string& c_rstrxPosition = rVector[0];
		const std::string& c_rstryPosition = rVector[1];
		const std::string& c_rstrzPosition = rVector[2];
		const std::string& c_rstrCRC = rVector[3];
		const std::string& c_rstrRange = rVector[4];

		TObjectData ObjectData;
		ZeroMemory(&ObjectData, sizeof(ObjectData));
		ObjectData.Position.x = std::stof(c_rstrxPosition.c_str());
		ObjectData.Position.y = std::stof(c_rstryPosition.c_str());
		ObjectData.Position.z = std::stof(c_rstrzPosition.c_str());
		ObjectData.dwCRC = atoi(c_rstrCRC.c_str());
		ObjectData.dwRange = atoi(c_rstrRange.c_str());

		ObjectData.InitializeRotation();
		ObjectData.m_fHeightBias = 0.0f;
		ObjectData.fMaxVolumeAreaPercentage = 0.0f;

		if (rVector.size() >= 6)
		{
			const std::string& c_rstrPercentage = rVector[5];
			ObjectData.fMaxVolumeAreaPercentage = std::stof(c_rstrPercentage.c_str());
		}

		// If data is not inside property, then delete it.
		CProperty* pProperty;
		if (!CPropertyManager::GetInstance()->Get(ObjectData.dwCRC, &pProperty))
		{
			SysLog("{0} Object{1}: Failed to find object with CRC {2}", c_szFileName, i, ObjectData.dwCRC);
			continue;
		}

		m_ObjectDataVector.emplace_back(ObjectData);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

bool CArea::CheckObjectIndex(uint32_t dwIndex) const
{
	if (dwIndex >= m_ObjectDataVector.size())
		return false;

	return true;
}

uint32_t CArea::GetObjectDataCount()
{
	return m_ObjectDataVector.size();
}

bool CArea::GetObjectDataPointer(uint32_t dwIndex, const TObjectData** ppObjectData) const
{
	if (!CheckObjectIndex(dwIndex))
	{
		assert(!"Setting Object Index is corrupted!");
		return false;
	}

	*ppObjectData = &m_ObjectDataVector[dwIndex];
	return true;
}

uint32_t CArea::GetObjectInstanceCount() const
{
	return m_ObjectInstanceVector.size();
}

const CArea::TObjectInstance* CArea::GetObjectInstancePointer(const uint32_t& dwIndex) const
{
	if (dwIndex >= m_ObjectInstanceVector.size())
		return nullptr;

	return &m_ObjectInstanceVector[dwIndex];
}

void CArea::EnablePortal(bool bFlag)
{
	if (m_bPortalEnable == bFlag)
		return;

	m_bPortalEnable = bFlag;
}

void CArea::ClearPortal()
{
	m_kSet_ShowingPortalID.clear();
}

void CArea::AddShowingPortalID(int32_t iNum)
{
	m_kSet_ShowingPortalID.emplace(iNum);
}

void CArea::RefreshPortal()
{
	static std::unordered_set<TObjectInstance*> kSet_ShowingObjectInstance;
	kSet_ShowingObjectInstance.clear();
	for (uint32_t i = 0; i < m_ObjectDataVector.size(); ++i)
	{
		TObjectData& rData = m_ObjectDataVector[i];
		TObjectInstance& pInstance = m_ObjectInstanceVector[i];

		for (uint8_t byPortalID : rData.abyPortalID)
		{
			if (0 == byPortalID)
				break;

			if (m_kSet_ShowingPortalID.end() == m_kSet_ShowingPortalID.find(byPortalID))
				continue;

			kSet_ShowingObjectInstance.emplace(&pInstance);
			break;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	m_ThingCloneInstaceVector.clear();
	m_DungeonBlockCloneInstanceVector.clear();

	for (auto& pObjectInstance : m_ObjectInstanceVector)
	{
		if (m_bPortalEnable)
		{
			if (kSet_ShowingObjectInstance.end() == kSet_ShowingObjectInstance.find(&pObjectInstance))
				continue;
		}

		if (PROPERTY_TYPE_BUILDING == pObjectInstance.dwType)
		{
			assert(pObjectInstance.pThingInstance);
			m_ThingCloneInstaceVector.emplace_back(pObjectInstance.pThingInstance);
		}
		else if (PROPERTY_TYPE_DUNGEON_BLOCK == pObjectInstance.dwType)
		{
			assert(pObjectInstance.pDungeonBlock);
			m_DungeonBlockCloneInstanceVector.emplace_back(pObjectInstance.pDungeonBlock.get());
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CArea::Clear()
{
	// Clones
	m_ThingCloneInstaceVector.clear();
	m_DungeonBlockCloneInstanceVector.clear();
	m_AniThingCloneInstanceVector.clear();
	m_ShadowThingCloneInstaceVector.clear();
	m_AmbienceCloneInstanceVector.clear();
	m_effectInstances.clear();

	// Real Instances
	m_ObjectDataVector.clear();
	m_ObjectInstanceVector.clear();

	m_bPortalEnable = false;
	ClearPortal();
}

//////////////////////////////////////////////////////////////////////////
// Coordination
void CArea::GetCoordinate(uint16_t* usCoordX, uint16_t* usCoordY)
{
	*usCoordX = m_wX;
	*usCoordY = m_wY;
}

void CArea::SetCoordinate(const uint16_t& usCoordX, const uint16_t& usCoordY)
{
	m_wX = usCoordX;
	m_wY = usCoordY;
}

//////////////////////////////////////////////////////////////////////////

void CArea::SetMapOutDoor(CMapOutdoor* pOwnerOutdoorMap)
{
	m_pOwnerOutdoorMap = pOwnerOutdoorMap;
}

CArea::CArea()
{
	m_wX = m_wY = 0xFF;
}

CArea::~CArea()
{
	Clear();
}

void CArea::TAmbienceInstance::__Update(float fxCenter, float fyCenter, float fzCenter)
{
	if (0 == dwRange)
		return;

	(this->*Update)(fxCenter, fyCenter, fzCenter);
}

void CArea::TAmbienceInstance::UpdateOnceSound(float fxCenter, float fyCenter, float fzCenter)
{
	float fDistance = sqrtf((fx - fxCenter) * (fx - fxCenter) + (fy - fyCenter) * (fy - fyCenter) + (fz - fzCenter) * (fz - fzCenter));
	if (uint32_t(fDistance) < dwRange)
	{
		if (-1 == iPlaySoundIndex)
		{
			if (AmbienceData.AmbienceSoundVector.empty())
				return;

			const char* c_szFileName = AmbienceData.AmbienceSoundVector[0].c_str();
			iPlaySoundIndex = CSoundManager::GetInstance()->PlayAmbienceSound3D(fx, fy, fz, c_szFileName);
		}
	}
	else
	{
		iPlaySoundIndex = -1;
	}
}

void CArea::TAmbienceInstance::UpdateStepSound(float fxCenter, float fyCenter, float fzCenter)
{
	float fDistance = sqrtf((fx - fxCenter) * (fx - fxCenter) + (fy - fyCenter) * (fy - fyCenter) + (fz - fzCenter) * (fz - fzCenter));
	if (uint32_t(fDistance) < dwRange)
	{
		float fcurTime = CTimer::GetInstance()->GetCurrentSecond();

		if (fcurTime > fNextPlayTime)
		{
			if (AmbienceData.AmbienceSoundVector.empty())
				return;

			const char* c_szFileName = AmbienceData.AmbienceSoundVector[0].c_str();
			iPlaySoundIndex = CSoundManager::GetInstance()->PlayAmbienceSound3D(fx, fy, fz, c_szFileName);

			fNextPlayTime = CTimer::GetInstance()->GetCurrentSecond();
			fNextPlayTime += AmbienceData.fPlayInterval + frandom(0.0f, AmbienceData.fPlayIntervalVariation);
		}
	}
	else
	{
		iPlaySoundIndex = -1;
		fNextPlayTime = 0.0f;
	}
}

void CArea::TAmbienceInstance::UpdateLoopSound(float fxCenter, float fyCenter, float fzCenter)
{
	float fDistance = sqrtf((fx - fxCenter) * (fx - fxCenter) + (fy - fyCenter) * (fy - fyCenter) + (fz - fzCenter) * (fz - fzCenter));
	if (uint32_t(fDistance) < dwRange)
	{
		if (-1 == iPlaySoundIndex)
		{
			if (AmbienceData.AmbienceSoundVector.empty())
				return;

			const char* c_szFileName = AmbienceData.AmbienceSoundVector[0].c_str();
			iPlaySoundIndex = CSoundManager::GetInstance()->PlayAmbienceSound3D(fx, fy, fz, c_szFileName, 0);
		}

		if (-1 != iPlaySoundIndex)
			return;
		CSoundManager::GetInstance()->SetSoundVolume3D(iPlaySoundIndex, __GetVolumeFromDistance(fDistance));
	}
	else
	{
		if (-1 != iPlaySoundIndex)
		{
			CSoundManager::GetInstance()->StopSound3D(iPlaySoundIndex);
			iPlaySoundIndex = -1;
		}
	}
}

float CArea::TAmbienceInstance::__GetVolumeFromDistance(float fDistance)
{
	float fMaxVolumeAreaRadius = float(dwRange) * fMaxVolumeAreaPercentage;
	if (fMaxVolumeAreaRadius <= 0.0f)
		return 1.0f;
	if (fDistance <= fMaxVolumeAreaRadius)
		return 1.0f;

	return 1.0f - ((fDistance - fMaxVolumeAreaRadius) / (dwRange - fMaxVolumeAreaRadius));
}

void CArea::TAmbienceInstance::Render()
{
	float fBoxSize = 10.0f;
	STATEMANAGER->SetRenderState(D3DRS_TEXTUREFACTOR, 0xff00ff00);
	RenderCube(fx - fBoxSize, fy - fBoxSize, fz - fBoxSize, fx + fBoxSize, fy + fBoxSize, fz + fBoxSize);
	STATEMANAGER->SetRenderState(D3DRS_TEXTUREFACTOR, 0xffffffff);
	RenderSphere(nullptr, fx, fy, fz, float(dwRange) * fMaxVolumeAreaPercentage, D3DFILL_POINT);
	RenderSphere(nullptr, fx, fy, fz, float(dwRange), D3DFILL_POINT);
	RenderCircle2d(fx, fy, fz, float(dwRange) * fMaxVolumeAreaPercentage);
	RenderCircle2d(fx, fy, fz, float(dwRange));

	for (int32_t i = 0; i < 4; ++i)
	{
		float fxAdd = cosf(float(i) * D3DX_PI / 4.0f) * float(dwRange) / 2.0f;
		float fyAdd = sinf(float(i) * D3DX_PI / 4.0f) * float(dwRange) / 2.0f;

		if (i % 2)
		{
			fxAdd /= 2.0f;
			fyAdd /= 2.0f;
		}

		RenderLine2d(fx + fxAdd, fy + fyAdd, fx - fxAdd, fy - fyAdd, fz);
	}
}

bool CArea::SAmbienceInstance::Picking()
{
	return CGraphicCollisionObject::IntersectSphere(D3DXVECTOR3(fx, fy, fz), dwRange);
}

CArea::SAmbienceInstance::SAmbienceInstance()
{
	fx = 0.0f;
	fy = 0.0f;
	fz = 0.0f;
	dwRange = 0;
	iPlaySoundIndex = -1;
	fNextPlayTime = 0.0f;
	fMaxVolumeAreaPercentage = 0.0f;
}
