#include "StdAfx.h"
#include <EffectLib/EffectManager.h>
#include <EterGrnLib/ModelInstance.h>

#include "ActorInstance.h"
#include "ItemData.h"
#include "ItemManager.h"
#include "RaceData.h"
#include "WeaponTrace.h"

uint32_t CActorInstance::AttachSmokeEffect(uint32_t eSmoke)
{
	if (!m_pkCurRaceData)
		return 0;

	uint32_t dwSmokeEffectID=m_pkCurRaceData->GetSmokeEffectID(eSmoke);

	return AttachEffectByID(0, m_pkCurRaceData->GetSmokeBone().c_str(), dwSmokeEffectID);
}

bool CActorInstance::__IsLeftHandWeapon(uint32_t type)
{
	if (ITEM::WEAPON_DAGGER == type || (ITEM::WEAPON_FAN == type && __IsMountingHorse()))
		return true;
	else if (ITEM::WEAPON_BOW == type)
		return true;
	else
		return false;
}

bool CActorInstance::__IsRightHandWeapon(uint32_t type)
{
	if (ITEM::WEAPON_DAGGER == type || (ITEM::WEAPON_FAN == type && __IsMountingHorse()))
		return true;
	else if (ITEM::WEAPON_BOW == type)
		return false;
	else
		return true;
}

bool CActorInstance::__IsWeaponTrace(uint32_t weaponType)
{
	switch(weaponType)
	{
	case ITEM::WEAPON_BELL:
	case ITEM::WEAPON_FAN:
	case ITEM::WEAPON_BOW:
		return false;
	default:
		return true;

	}
}

void CActorInstance::AttachWeapon(uint32_t dwItemIndex, uint32_t dwParentPartIndex)
{
	m_adwPartItemID[PART_WEAPON] = dwItemIndex;

	CItemData* pItemData = CItemManager::GetInstance()->GetProto(dwItemIndex);
	if (!pItemData)
	{
		RegisterModelThing(PART_WEAPON, nullptr);
		SetModelInstance(PART_WEAPON, PART_WEAPON, 0);

		RegisterModelThing(PART_WEAPON_LEFT, nullptr);
		SetModelInstance(PART_WEAPON_LEFT, PART_WEAPON_LEFT, 0);

		RefreshActorInstance();
		return;
	}

	__DestroyWeaponTrace();
	if (__IsRightHandWeapon(pItemData->GetWeaponType()))
		AttachWeapon(PART_MAIN, PART_WEAPON, pItemData);
	if (__IsLeftHandWeapon(pItemData->GetWeaponType()))
		AttachWeapon(PART_MAIN, PART_WEAPON_LEFT, pItemData);
}

void CActorInstance::AttachWeapon(uint32_t dwParentPartIndex, uint32_t dwPartIndex, CItemData* pItemData)
{
	if (!pItemData)
		return;

	const char* szBoneName;
	if (!GetAttachingBoneName(dwPartIndex, &szBoneName))
		return;

	int32_t boneIndex;
	if (!FindBoneIndex(dwParentPartIndex, szBoneName, &boneIndex))
		return;

	if (PART_WEAPON_LEFT == dwPartIndex)
		RegisterModelThing(dwPartIndex, pItemData->GetSubModelThing());
	else
		RegisterModelThing(dwPartIndex, pItemData->GetModelThing());

	SetModelInstance(dwPartIndex, dwPartIndex, 0);
	AttachModelInstance(PART_MAIN, *this, dwPartIndex, boneIndex);

	SMaterialData kMaterialData{};
	kMaterialData.pImage = nullptr;
	kMaterialData.isSpecularEnable = true;
	kMaterialData.fSpecularPower = pItemData->GetSpecularPowerf();
	kMaterialData.bSphereMapIndex = 1;
	SetMaterialData(dwPartIndex, nullptr, kMaterialData);

	// Weapon Trace
	if (__IsWeaponTrace(pItemData->GetWeaponType()))
	{
		CWeaponTrace* pWeaponTrace = CWeaponTrace::New();
		pWeaponTrace->SetWeaponInstance(this, dwPartIndex, szBoneName);
		m_WeaponTraceVector.emplace_back(pWeaponTrace);
	}

	CGrannyModelInstance* pModelInstance = m_modelInstances[dwPartIndex];
	if (pModelInstance && pModelInstance->GetModelInstance())
	{
		CGraphicThing* pItemGraphicThing = pItemData->GetModelThing();
		if (std::shared_ptr<CGrannyMotion> pItemMotion = pItemGraphicThing->GetMotionPointer(0))
		{
			pModelInstance->SetMotionPointer(pItemMotion);
		}
	}
}

void CActorInstance::AttachAcce(uint32_t dwItemIndex)
{
	/*
	m_adwPartItemID[PART_ACCE] = dwItemIndex;
	CItemData* itemData = CItemManager::Instance().GetProto(dwItemIndex);
	if (!itemData)
	{
		RegisterModelThing(PART_ACCE, nullptr);
		SetModelInstance(PART_ACCE, PART_ACCE, 0);

		RefreshActorInstance();
		return;
	}

	int boneIndex;
	if (!FindBoneIndex(PART_MAIN, "Bip01 Spine2", &boneIndex))
		return;

	RegisterModelThing(PART_ACCE, itemData->GetModelThing());

	SetModelInstance(PART_ACCE, PART_ACCE, 0);
	AttachModelInstance(PART_MAIN, *this, PART_ACCE, boneIndex);

	SMaterialData kMaterialData;
	kMaterialData.pImage = NULL;
	kMaterialData.isSpecularEnable = TRUE;
	kMaterialData.fSpecularPower = itemData->GetSpecularPowerf();
	kMaterialData.bSphereMapIndex = 1;
	SetMaterialData(PART_ACCE, NULL, kMaterialData);

	CGrannyModelInstance* pModelInstance = m_modelInstances[ACCE];
	if (pModelInstance && pModelInstance->GetModelInstance())
	{
		CGraphicThing* pItemGraphicThing = pItemData->GetModelThing();
		if (std::shared_ptr<CGrannyMotion> pItemMotion = pItemGraphicThing->GetMotionPointer(0))
		{
			pModelInstance->SetMotionPointer(pItemMotion);
		}
	}

	*/
}

bool CActorInstance::GetAttachingBoneName(uint32_t dwPartIndex, const char** c_pszBoneName)
{
	return m_pkCurRaceData->GetAttachingBoneName(dwPartIndex, c_pszBoneName);
}

void  CActorInstance::DettachEffect(uint32_t dwEID)
{
	std::list<TAttachingEffect>::iterator i = m_AttachingEffectList.begin();

	while (i != m_AttachingEffectList.end())
	{
		TAttachingEffect & rkAttEft = (*i);

		if (rkAttEft.dwEffectIndex == dwEID)
		{
			i = m_AttachingEffectList.erase(i);
			CEffectManager::GetInstance()->DestroyEffectInstance(dwEID);
		}
		else
		{
			++i;
		}
	}
}

uint32_t CActorInstance::AttachEffectByName(uint32_t dwParentPartIndex, const char * c_pszBoneName, const char * c_pszEffectName)
{
	std::string str;
	uint32_t dwCRC;
	StringPath(c_pszEffectName, str);
	dwCRC = GetCaseCRC32(str.c_str(), str.length());

	return AttachEffectByID(dwParentPartIndex, c_pszBoneName, dwCRC);
}

uint32_t CActorInstance::AttachEffectByID(uint32_t dwParentPartIndex, const char * c_pszBoneName, uint32_t dwEffectID, const D3DXVECTOR3 * c_pv3Position)
{
	TAttachingEffect ae;
	ae.iLifeType = EFFECT_LIFE_INFINITE;
	ae.dwEndTime = 0;
	ae.dwModelIndex = dwParentPartIndex;
	ae.dwEffectIndex = CEffectManager::GetInstance()->GetEmptyIndex();
	ae.isAttaching = TRUE;
	if (c_pv3Position)
	{
		D3DXMatrixTranslation(&ae.matTranslation, c_pv3Position->x, c_pv3Position->y, c_pv3Position->z);
	}
	else
	{
		D3DXMatrixIdentity(&ae.matTranslation);
	}
	auto rkEftMgr=CEffectManager::GetInstance();
	rkEftMgr->CreateEffectInstance(ae.dwEffectIndex, dwEffectID);

	if (c_pszBoneName)
	{
		int32_t iBoneIndex;

		if (!FindBoneIndex(dwParentPartIndex,c_pszBoneName, &iBoneIndex))
		{
			ae.iBoneIndex = -1;
			//Tracef("Cannot get Bone Index\n");
			//assert(false && "Cannot get Bone Index");
		}
		else
		{
			ae.iBoneIndex = iBoneIndex;
		}
	}
	else
	{
		ae.iBoneIndex = -1;
	}

	m_AttachingEffectList.push_back(ae);

	return ae.dwEffectIndex;
}

void CActorInstance::RefreshActorInstance()
{
	if (!m_pkCurRaceData)
	{
		SysLog("void CActorInstance::RefreshActorInstance() - m_pkCurRaceData=nullptr");
		return;
	}

	// This is Temporary place before making the weapon detection system
	// Setup Collison Detection Data
	m_BodyPointInstanceList.clear();
	//m_AttackingPointInstanceList.clear();
	m_DefendingPointInstanceList.clear();

	for (const auto& data : m_pkCurRaceData->GetAttachingData())
	{
		switch (data.dwType)
		{
		case NRaceData::ATTACHING_DATA_TYPE_COLLISION_DATA:
		{
			const NRaceData::TCollisionData* c_pCollisionData = data.pCollisionData;

			TCollisionPointInstance PointInstance;
			if (NRaceData::COLLISION_TYPE_ATTACKING == c_pCollisionData->iCollisionType)
				continue;

			if (!CreateCollisionInstancePiece(PART_MAIN, &data, &PointInstance))
				continue;

			switch (c_pCollisionData->iCollisionType)
			{
			case NRaceData::COLLISION_TYPE_ATTACKING:
				break;
			case NRaceData::COLLISION_TYPE_DEFENDING:
				m_DefendingPointInstanceList.emplace_back(PointInstance);
				break;
			case NRaceData::COLLISION_TYPE_BODY:
				m_BodyPointInstanceList.emplace_back(PointInstance);
				break;
			}
		}
		break;

		case NRaceData::ATTACHING_DATA_TYPE_EFFECT:
			if (data.isAttaching)
				AttachEffectByName(0, data.strAttachingBoneName.c_str(), data.pEffectData->strFileName.c_str());
			else
				AttachEffectByName(0, nullptr, data.pEffectData->strFileName.c_str());
			break;

		case NRaceData::ATTACHING_DATA_TYPE_OBJECT:
			break;

		default:
			assert(false/*NOT_IMPLEMENTED*/);
			break;
		}
	}

	for (uint32_t j = 0; j < PART_MAX_NUM; ++j)
	{
		if (0 == m_adwPartItemID[j])
			continue;

		CItemData* pItemData;
		if (!CItemManager::GetInstance()->GetItemDataPointer(m_adwPartItemID[j], &pItemData))
			return;

		for (uint32_t k = 0; k < pItemData->GetAttachingDataCount(); ++k)
		{
			const NRaceData::TAttachingData* c_pAttachingData;

			if (!pItemData->GetAttachingDataPointer(k, &c_pAttachingData))
				continue;

			switch (c_pAttachingData->dwType)
			{
			case NRaceData::ATTACHING_DATA_TYPE_COLLISION_DATA:
			{
				const NRaceData::TCollisionData* c_pCollisionData = c_pAttachingData->pCollisionData;

				TCollisionPointInstance PointInstance;
				if (NRaceData::COLLISION_TYPE_ATTACKING == c_pCollisionData->iCollisionType)
					continue;
				if (!CreateCollisionInstancePiece(j, c_pAttachingData, &PointInstance))
					continue;

				switch (c_pCollisionData->iCollisionType)
				{
				case NRaceData::COLLISION_TYPE_ATTACKING:
					break;
				case NRaceData::COLLISION_TYPE_DEFENDING:
					m_DefendingPointInstanceList.emplace_back(PointInstance);
					break;
				case NRaceData::COLLISION_TYPE_BODY:
					m_BodyPointInstanceList.emplace_back(PointInstance);
					break;
				}
			}
			break;

			case NRaceData::ATTACHING_DATA_TYPE_EFFECT:
				if (!m_bEffectInitialized)
				{
					uint32_t dwCRC;
					StringPath(c_pAttachingData->pEffectData->strFileName);
					dwCRC = GetCaseCRC32(c_pAttachingData->pEffectData->strFileName.c_str(),
						c_pAttachingData->pEffectData->strFileName.length());

					TAttachingEffect ae;
					ae.iLifeType = EFFECT_LIFE_INFINITE;
					ae.dwEndTime = 0;
					ae.dwModelIndex = j;
					ae.dwEffectIndex = CEffectManager::GetInstance()->GetEmptyIndex();
					ae.isAttaching = TRUE;
					CEffectManager::GetInstance()->CreateEffectInstance(ae.dwEffectIndex, dwCRC);

					int32_t iBoneIndex;
					if (!FindBoneIndex(j, c_pAttachingData->strAttachingBoneName.c_str(), &iBoneIndex))
					{
						ConsoleLog("Cannot get Bone Index\n");
						assert(false/*Cannot get Bone Index*/);
					}

					ae.iBoneIndex = iBoneIndex;

					m_AttachingEffectList.emplace_back(ae);
				}
				break;

			case NRaceData::ATTACHING_DATA_TYPE_OBJECT:
				break;

			default:
				assert(false/*NOT_IMPLEMENTED*/);
				break;
			}
		}
	}

	m_bEffectInitialized = true;
}

void CActorInstance::RefreshCollisionAttachments()
{
	assert(m_pkCurRaceData);
	if (!m_pkCurRaceData)
	{
		SysLog("m_pkCurRaceData = NULL");
		return;
	}

	m_BodyPointInstanceList.clear();
	m_DefendingPointInstanceList.clear();

	for (const auto& data : m_pkCurRaceData->GetAttachingData())
	{
		switch (data.dwType)
		{
		case NRaceData::ATTACHING_DATA_TYPE_COLLISION_DATA:
		{
			const NRaceData::TCollisionData* c_pCollisionData = data.pCollisionData;

			if (NRaceData::COLLISION_TYPE_ATTACKING == c_pCollisionData->iCollisionType)
				continue;

			TCollisionPointInstance PointInstance;
			if (!CreateCollisionInstancePiece(PART_MAIN, &data, &PointInstance))
				continue;

			switch (c_pCollisionData->iCollisionType)
			{
			case NRaceData::COLLISION_TYPE_ATTACKING:
				ConsoleLog("Obsolete attacking collision data encountered");
				break;
			case NRaceData::COLLISION_TYPE_DEFENDING:
				m_DefendingPointInstanceList.emplace_back(PointInstance);
				break;
			case NRaceData::COLLISION_TYPE_BODY:
				m_BodyPointInstanceList.emplace_back(PointInstance);
				break;
			}
		}
		}
	}
}

void CActorInstance::SetWeaponTraceTexture(const char * szTextureName)
{
	std::vector<CWeaponTrace*>::iterator it;
	for (it = m_WeaponTraceVector.begin(); it != m_WeaponTraceVector.end(); ++it)
	{
		(*it)->SetTexture(szTextureName);
	}
}

void CActorInstance::UseTextureWeaponTrace()
{
	for_each(
			m_WeaponTraceVector.begin(),
			m_WeaponTraceVector.end(),
			std::void_mem_fun(&CWeaponTrace::UseTexture)
			);
}

void CActorInstance::UseAlphaWeaponTrace()
{
	for_each(
			m_WeaponTraceVector.begin(),
			m_WeaponTraceVector.end(),
			std::void_mem_fun(&CWeaponTrace::UseAlpha)
			);
}

void CActorInstance::UpdateAttachingInstances()
{
	auto rkEftMgr=CEffectManager::GetInstance();

	std::list<TAttachingEffect>::iterator it;
	uint32_t dwCurrentTime = CTimer::GetInstance()->GetCurrentMillisecond();
	for (it = m_AttachingEffectList.begin(); it!= m_AttachingEffectList.end();)
	{
		if (EFFECT_LIFE_WITH_MOTION == it->iLifeType)
		{
			++it;
			continue;
		}

		if ((EFFECT_LIFE_NORMAL == it->iLifeType && it->dwEndTime < dwCurrentTime) ||
			!rkEftMgr->IsAliveEffect(it->dwEffectIndex))
		{
			rkEftMgr->DestroyEffectInstance(it->dwEffectIndex);
			it = m_AttachingEffectList.erase(it);
		}
		else
		{
			if (it->isAttaching)
			{
				rkEftMgr->SelectEffectInstance(it->dwEffectIndex);

				if (it->iBoneIndex == -1)
				{
					D3DXMATRIX matTransform;
					matTransform = it->matTranslation;
					matTransform *= m_worldMatrix;
					rkEftMgr->SetEffectInstanceGlobalMatrix(matTransform);
				}
				else
				{
					D3DXMATRIX * pBoneMat;
					if (GetBoneMatrix(it->dwModelIndex, it->iBoneIndex, &pBoneMat))
					{
						D3DXMATRIX matTransform;
						matTransform = *pBoneMat;
						matTransform *= it->matTranslation;
						matTransform *= m_worldMatrix;
						rkEftMgr->SetEffectInstanceGlobalMatrix(matTransform);
					}
					else
					{
						//TraceError("GetBoneMatrix(modelIndex(%d), boneIndex(%d)).NOT_FOUND_BONE",
						//	it->dwModelIndex, it->iBoneIndex);
					}
				}
			}

			++it;
		}
	}
}

void CActorInstance::ShowAllAttachingEffect()
{
	std::list<TAttachingEffect>::iterator it;
	for(it = m_AttachingEffectList.begin(); it!= m_AttachingEffectList.end();++it)
	{
		CEffectManager::GetInstance()->SelectEffectInstance(it->dwEffectIndex);
		CEffectManager::GetInstance()->ShowEffect();
	}
}

void CActorInstance::HideAllAttachingEffect()
{
	std::list<TAttachingEffect>::iterator it;
	for(it = m_AttachingEffectList.begin(); it!= m_AttachingEffectList.end();++it)
	{
		CEffectManager::GetInstance()->SelectEffectInstance(it->dwEffectIndex);
		CEffectManager::GetInstance()->HideEffect();
	}
}

void CActorInstance::__ClearAttachingEffect()
{
	m_bEffectInitialized = false;

	std::list<TAttachingEffect>::iterator it;
	for(it = m_AttachingEffectList.begin(); it!= m_AttachingEffectList.end();++it)
	{
		CEffectManager::GetInstance()->DestroyEffectInstance(it->dwEffectIndex);
	}
	m_AttachingEffectList.clear();
}
