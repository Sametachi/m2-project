#include "StdAfx.h"
#include "ActorInstance.h"
#include "RaceManager.h"
#include "ItemManager.h"
#include "RaceData.h"

#include <EterLib/ResourceManager.h>
#include <EterGrnLib/Util.h>
#include <Core/Race/MotionProto.hpp>

uint32_t CActorInstance::GetVirtualID()
{
	return m_dwSelfVID;
}

void CActorInstance::SetVirtualID(uint32_t dwVID)
{
	m_dwSelfVID = dwVID;
}

void CActorInstance::UpdateAttribute()
{
	if (!m_pAttributeInstance)
		return;

	if (!m_bNeedUpdateCollision)
		return;

	m_bNeedUpdateCollision = false;

	const CStaticCollisionDataVector& c_rkVec_ColliData = m_pAttributeInstance->GetObjectPointer()->GetCollisionDataVector();
	UpdateCollisionData(&c_rkVec_ColliData);

	m_pAttributeInstance->RefreshObject(GetTransform());
	UpdateHeightInstance(m_pAttributeInstance);
}

void CActorInstance::__CreateAttributeInstance(CAttributeData* pData)
{
	m_pAttributeInstance = new CAttributeInstance();
	m_pAttributeInstance->Clear();
	m_pAttributeInstance->SetObjectPointer(pData);
}

uint32_t CActorInstance::GetRace()
{
	return m_eRace;
}

bool CActorInstance::SetRace(uint32_t eRace)
{
	m_eRace = eRace;
	m_pkCurRaceData = nullptr;

	const auto raceData = CRaceManager::GetInstance()->GetRaceDataPointer(eRace);
	if (!raceData)
		return false;

	m_pkCurRaceData = raceData.value();

	// Motion id is the base race id
	CGraphicThingInstance::SetMotionID(m_pkCurRaceData->GetRaceIndex());

	CAttributeData* pAttributeData = m_pkCurRaceData->GetAttributeDataPtr();
	if (pAttributeData)
		__CreateAttributeInstance(pAttributeData);

	std::memset(m_adwPartItemID, 0, sizeof(m_adwPartItemID));

	__ClearAttachingEffect();

	CGraphicThingInstance::Clear();

	if (IsPC() || (IsNPC() && GetRace() < 10) /* || IsRenderTarget()*/)
	{
		CGraphicThingInstance::ReserveModelThing(PART_MAX_NUM);
		CGraphicThingInstance::ReserveModelInstance(PART_MAX_NUM);
	}
	else
	{
		CGraphicThingInstance::ReserveModelThing(1);
		CGraphicThingInstance::ReserveModelInstance(1);
	}

	m_pkCurRaceData->LoadMotions();
	return true;
}

void CActorInstance::SetHair(uint32_t eHair)
{
	assert(m_pkCurRaceData && "");

	CRaceData::SHair* pkHair = m_pkCurRaceData->FindHair(eHair);
	if (!pkHair)
	{
		SysLog("Failed to find hair {0} for race {1}", eHair, m_eRace);
		return;
	}

	if (pkHair->m_stModelFileName.empty())
	{
		SysLog("Hair {0} for race {0}: model path empty", eHair, m_eRace);
		return;
	}

	CGraphicThing* model = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(pkHair->m_stModelFileName);
	if (!model)
	{
		SysLog("Hair {0} for race {1}: model {2} is null", eHair, m_eRace, pkHair->m_stModelFileName.c_str());
		return;
	}

	RegisterModelThing(PART_HAIR, model);
	SetModelInstance(PART_HAIR, PART_HAIR, 0, PART_MAIN);

	for (const CRaceData::SSkin& skin : pkHair->m_kVct_kSkin)
	{
		CGraphicImage* r = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(skin.m_stDstFileName);
		if (!r)
		{
			SysLog("Hair {0} for race {1}: skin {2} is null", eHair, m_eRace, skin.m_stDstFileName.c_str());
			continue;
		}

		SetMaterialImagePointer(PART_HAIR, skin.m_stSrcFileName.c_str(), r);
	}
}

void CActorInstance::SetShape(uint32_t eShape, float fSpecular)
{
	//assert(m_pkCurRaceData && "");

	if (!m_pkCurRaceData)
		return;

	m_eShape = eShape;

	CRaceData::SShape* pkShape = m_pkCurRaceData->FindShape(eShape);
	if (pkShape)
	{
		CResourceManager* resMgr = CResourceManager::GetInstance();

		if (pkShape->m_stModelFileName.empty())
		{
			CGraphicThing* r = m_pkCurRaceData->GetBaseModelThing();
			if (!r)
			{
				SysLog("Race {0} has no base model", m_eRace);
				return;
			}

			RegisterModelThing(0, r);
		}
		else
		{
			CGraphicThing* r = resMgr->LoadResource<CGraphicThing>(pkShape->m_stModelFileName);
			if (!r)
			{
				SysLog("Failed to load {0} for shape {1} / race {2}", pkShape->m_stModelFileName.c_str(), eShape, m_eRace);
				return;
			}

			RegisterModelThing(0, r);
		}

		SetModelInstance(0, 0, 0);

		for (const auto& skin : pkShape->m_kVct_kSkin)
		{
			CGraphicImage* r = resMgr->LoadResource<CGraphicImage>(skin.m_stDstFileName.c_str());
			if (!r)
			{
				SysLog("Failed to load {0} to replace {1} for shape {2} / race {3}", skin.m_stDstFileName.c_str(), skin.m_stSrcFileName.c_str(), eShape, m_eRace);
				continue;
			}

			if (fSpecular > 0.0f)
			{
				SMaterialData kMaterialData;
				kMaterialData.pImage = r;
				kMaterialData.isSpecularEnable = true;
				kMaterialData.fSpecularPower = fSpecular;
				kMaterialData.bSphereMapIndex = 0;
				SetMaterialData(skin.m_ePart, skin.m_stSrcFileName.c_str(), kMaterialData);
			}
			else
			{
				SetMaterialImagePointer(skin.m_ePart, skin.m_stSrcFileName.c_str(), r);
			}
		}

		for (const auto& data : pkShape->m_attachingData)
		{
			switch (data.dwType)
			{
			case NRaceData::ATTACHING_DATA_TYPE_EFFECT:
			{
				if (data.isAttaching)
					AttachEffectByName(0, data.strAttachingBoneName.c_str(), data.pEffectData->strFileName.c_str());
				else
					AttachEffectByName(0, 0, data.pEffectData->strFileName.c_str());
				break;
			}
			}
		}
	}
	else
	{
		CGraphicThing* model = m_pkCurRaceData->GetBaseModelThing();
		if (!model)
		{
			SysLog("Failed to load base model for race {0}", m_eRace);
			return;
		}

		RegisterModelThing(0, model);
		SetModelInstance(0, 0, 0);
	}

	for (const auto& data : m_pkCurRaceData->GetAttachingData())
	{
		switch (data.dwType)
		{
		case NRaceData::ATTACHING_DATA_TYPE_EFFECT:
		{
			if (data.isAttaching)
				AttachEffectByName(0, data.strAttachingBoneName.c_str(), data.pEffectData->strFileName.c_str());
			else
				AttachEffectByName(0, 0, data.pEffectData->strFileName.c_str());
			break;
		}
		}
	}

	RefreshCollisionAttachments();
}

void CActorInstance::ChangeMaterial(const char* c_szFileName)
{
	assert(m_pkCurRaceData && "");

	CRaceData::SShape* pkShape = m_pkCurRaceData->FindShape(m_eShape);
	if (!pkShape)
		return;

	const auto& skins = pkShape->m_kVct_kSkin;
	if (skins.empty())
		return;

	const CRaceData::SSkin& skin = skins[0];

	CGraphicImage* r = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(c_szFileName);
	if (!r)
	{
		SysLog("Failed to load {0} to replace {1}", c_szFileName, skin.m_stSrcFileName.c_str());
		return;
	}

	SetMaterialImagePointer(skin.m_ePart, skin.m_stSrcFileName.c_str(), r);
}

uint32_t CActorInstance::GetPartItemID(uint32_t dwPartIndex)
{
	if (dwPartIndex >= PART_MAX_NUM)
	{
		SysLog("CActorInstance::GetPartIndex(dwPartIndex={0}/PART_MAX_NUM={1})", dwPartIndex, PART_MAX_NUM);
		return 0;
	}

	return m_adwPartItemID[dwPartIndex];
}

void CActorInstance::SetSpecularInfo(bool bEnable, int32_t iPart, float fAlpha)
{
	assert(m_pkCurRaceData && "");

	CRaceData::SShape* pkShape = m_pkCurRaceData->FindShape(m_eShape);
	if (pkShape->m_kVct_kSkin.empty())
		return;

	std::string filename = pkShape->m_kVct_kSkin[0].m_stSrcFileName;
	StringPath(filename);

	CGraphicThingInstance::SetSpecularInfo(iPart, filename.c_str(), bEnable, fAlpha);
}

void CActorInstance::SetSpecularInfoForce(bool bEnable, int32_t iPart, float fAlpha)
{
	CGraphicThingInstance::SetSpecularInfo(iPart, nullptr, bEnable, fAlpha);
}
