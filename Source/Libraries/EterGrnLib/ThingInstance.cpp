#include "StdAfx.h"
#include <EterLib/Camera.h>
#include <EterBase/Timer.h>
#include <EterLib/Engine.h>
#include <GameLib/GameType.h>
#include <GameLib/RaceData.h>
#include <Core/Race/RaceConstans.hpp>

#include "ThingInstance.h"
#include "Thing.h"
#include "ModelInstance.h"
#include "Granny3D.h"

CDynamicPool<CGraphicThingInstance>		CGraphicThingInstance::ms_kPool;
robin_hood::unordered_map<uint32_t, robin_hood::unordered_map<uint32_t, CGraphicThing::TRef*>>CGraphicThingInstance::m_roMotionThingMap;

CGraphicThingInstance::CGraphicThingInstance()
{
	Initialize();
}

void CGraphicThingInstance::OnInitialize()
{
	m_bUpdated = false;
	m_fLastLocalTime = 0.0f;
	m_fLocalTime = 0.0f;
	m_fDelay = 0.0f;
	m_fSecondElapsed = 0.0f;
	m_fAverageSecondElapsed = 0.03f;
	m_fRadius = -1.0f;
	m_v3Center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_allowedError = 0.0f;
	m_dwMotionID = std::numeric_limits<uint32_t>::max();
	ResetLocalTime();
}

void CGraphicThingInstance::DeformNoSkin()
{
	m_bUpdated = true;
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->DeformNoSkin(&m_worldMatrix, m_allowedError);
	}
}

void CGraphicThingInstance::TransformAttachment()
{
	const CGrannyModelInstance* mainModelInstance = m_modelInstances[0];
	if (mainModelInstance)
	{
		int32_t boneIndex = 0;
		mainModelInstance->GetBoneIndexByName("Bip01 Spine2", &boneIndex);

		const D3DXMATRIX* matBone = reinterpret_cast<const D3DXMATRIX*>(mainModelInstance->GetBoneMatrixPointer(boneIndex));
		if (matBone) 
		{
			D3DXVECTOR3 scalingCenter(matBone->_41, matBone->_42, matBone->_43);
			D3DXQUATERNION scalingRotation(0.0f, 0.0f, 0.0f, 0.0f);
			D3DXMatrixTransformation(&m_matScaleTransformation, &scalingCenter, &scalingRotation, &m_v3ScaleAttachment, 0, 0, 0);
		}

		D3DXMATRIX scaleTransformed;
		D3DXMatrixMultiply(&scaleTransformed, &m_matScale, &m_matScaleTransformation);
		D3DXMatrixMultiply(&m_matAttachedWorldTransform, &scaleTransformed, &m_mRotation);
		m_matAttachedWorldTransform._41 += m_v3Position.x;
		m_matAttachedWorldTransform._42 += m_v3Position.y;
		m_matAttachedWorldTransform._43 += m_v3Position.z;
	}
}

void CGraphicThingInstance::OnDeform()
{
	for (size_t i = 0; i < m_modelInstances.size(); i++)
	{
		CGrannyModelInstance* modelInstance = m_modelInstances[i];
		if (!modelInstance)
			continue;

		if (i == 5)
		{
			TransformAttachment();
			modelInstance->Deform(&m_matAttachedWorldTransform, m_allowedError);
			continue;
		}

		modelInstance->Deform(&m_worldMatrix, m_allowedError);
	}
	m_bUpdated = true;
}

void CGraphicThingInstance::UpdateLODLevel()
{
	CCamera* pcurCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	if (!pcurCamera)
	{
		SysLog("CGraphicThingInstance::UpdateLODLevel - GetCurrentCamera() == nullptr");
		return;
	}

	const D3DXVECTOR3& c_rv3CameraPosition = pcurCamera->GetEye();
	const D3DXVECTOR3& c_v3Position = GetPosition();
	const D3DXVECTOR3 asd = (c_rv3CameraPosition - c_v3Position);
	float distanceFromCamera = D3DXVec3Length(&asd);

	/*
		I mean... This is still unclear but honestly there isnt much option to costumize this...
	*/

	m_allowedError = GrannyFindAllowedLODError(1.0f, // error in pixels
		ms_iHeight,									// screen height
		GetFOV(),									// fov
		distanceFromCamera);						// distance
}

void CGraphicThingInstance::UpdateTime()
{
	m_fSecondElapsed = (float)CTimer::GetInstance()->GetElapsedSecond();

	if (m_fDelay > m_fSecondElapsed)
	{
		m_fDelay -= m_fSecondElapsed;
		m_fSecondElapsed = 0.0f;
	}
	else
	{
		m_fSecondElapsed -= m_fDelay;
		m_fDelay = 0.0f;
	}

	m_fLastLocalTime = m_fLocalTime;
	m_fLocalTime += m_fSecondElapsed;
	m_fAverageSecondElapsed = m_fAverageSecondElapsed + (m_fSecondElapsed - m_fAverageSecondElapsed) / 4.0f;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->AdvanceTime(m_fSecondElapsed);
	}
}

bool CGraphicThingInstance::LessRenderOrder(CGraphicThingInstance * pkThingInst)
{
	return (GetBaseThingPtr() < pkThingInst->GetBaseThingPtr());
}

bool CGraphicThingInstance::Picking(const D3DXVECTOR3 & v, const D3DXVECTOR3 & dir, float& out_x, float& out_y)
{
	if (!m_pHeightAttributeInstance)
		return false;
	return m_pHeightAttributeInstance->Picking(v, dir, out_x, out_y);
}

bool CGraphicThingInstance::CreateDeviceObjects()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->CreateDeviceObjects();
	}
	return true;
}

void CGraphicThingInstance::DestroyDeviceObjects()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->DestroyDeviceObjects();
	}
}

void CGraphicThingInstance::ReserveModelInstance(int32_t iCount)
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		CGrannyModelInstance::Delete(modelInstance);
	}

	m_modelInstances.clear();
	m_modelInstances.resize(iCount);
}

void CGraphicThingInstance::ReserveModelThing(int32_t iCount)
{
	m_pModelThings.resize(iCount);
}

void CGraphicThingInstance::RegisterModelThing(int32_t iModelThing, CGraphicThing * pModelThing)
{
	if (!CheckModelThingIndex(iModelThing)) 
	{
		SysLog("CGraphicThingInstance::RegisterModelThing(iModelThing={0})", iModelThing);
		return;
	}

	CGraphicThing::TRef* pMotionRef = new CGraphicThing::TRef;
	pMotionRef->SetPointer(pModelThing);

	m_pModelThings[iModelThing] = pMotionRef;
}

bool CGraphicThingInstance::CheckModelInstanceIndex(int32_t iModelInstance)
{
	if (iModelInstance < 0)
		return false;

	int32_t max = m_modelInstances.size();

	if (iModelInstance >= max)
		return false;

	return true;
}

bool CGraphicThingInstance::CheckModelThingIndex(int32_t iModelThing)
{
	if (iModelThing < 0)
		return false;

	int32_t max = m_modelInstances.size();

	if (iModelThing >= max)
		return false;

	return true;
}

// Motion management
void CGraphicThingInstance::RegisterMotionThing(uint32_t motionId, uint32_t dwMotionKey, CGraphicThing* pMotionThing)
{
	assert(pMotionThing && "null motions not allowed");

	if (CheckMotionThingIndex(dwMotionKey, motionId))
	{
		return;
	}

	CGraphicThing::TRef* pMotionRef = new CGraphicThing::TRef;
	pMotionRef->SetPointer(pMotionThing);

#ifdef _DEBUG
	WarnLog("Registered motion ({0}, {1})", motionId, dwMotionKey);
#endif
	m_roMotionThingMap[motionId][dwMotionKey] = pMotionRef;
}

bool CGraphicThingInstance::CheckMotionThingIndex(uint32_t dwMotionKey, uint32_t motionId)
{
	const auto it = m_roMotionThingMap.find(motionId);
	if (it == m_roMotionThingMap.end())
		return false;

	if (const auto itor = it->second.find(dwMotionKey); it->second.end() == itor)
		return false;

	return true;
}

bool CGraphicThingInstance::GetMotionThingPointer(uint32_t motionId, uint32_t dwMotionKey, CGraphicThing** ppMotion)
{
	if (!CheckMotionThingIndex(dwMotionKey, motionId))
		return false;

	*ppMotion = m_roMotionThingMap[motionId][dwMotionKey]->GetPointer();
	return true;
}

bool CGraphicThingInstance::IsMotionThing()
{
	return !m_roMotionThingMap[m_dwMotionID].empty();
}

bool CGraphicThingInstance::SetMotion(uint32_t dwMotionKey, float blendTime, int32_t loopCount, float speedRatio)
{
	const auto itor = m_roMotionThingMap[m_dwMotionID].find(dwMotionKey);
	if (itor == m_roMotionThingMap[m_dwMotionID].end())
	{
		return false;
	}

	CGraphicThing::TRef* proMotionThing = itor->second;
	CGraphicThing* pMotionThing = proMotionThing->GetPointer();

	if (!pMotionThing)
		return false;

	if (!pMotionThing->CheckMotionIndex(0))
		return false;

	std::shared_ptr<CGrannyMotion> motion = pMotionThing->GetMotionPointer(0);
	CGrannyModelInstance* modelInstance = nullptr;
	assert(motion && "NULL check");

	for (size_t i = 0; i < m_modelInstances.size(); i++)
	{
		CGrannyModelInstance* modelInstance = m_modelInstances[i];
		if (!modelInstance)
			continue;

		switch (i)
		{
		case PART_WEAPON:
		case PART_WEAPON_LEFT:
			//case PART_ACCE:
			break;

		default:
			modelInstance->SetMotionPointer(motion, blendTime, loopCount, speedRatio);
			break;
		}
	}

	return true;
}

bool CGraphicThingInstance::ChangeMotion(uint32_t dwMotionKey, int32_t loopCount, float speedRatio)
{
	const auto itor = m_roMotionThingMap[m_dwMotionID].find(dwMotionKey);
	if (itor == m_roMotionThingMap[m_dwMotionID].end())
	{
		return false;
	}

	CGraphicThing::TRef* proMotionThing = itor->second;
	CGraphicThing* pMotionThing = proMotionThing->GetPointer();

	if (!pMotionThing)
		return false;

	if (!pMotionThing->CheckMotionIndex(0))
		return false;

	std::shared_ptr<CGrannyMotion> motion = pMotionThing->GetMotionPointer(0);
	assert(motion && "NULL check");

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->ChangeMotionPointer(motion, loopCount, speedRatio);
	}
	return true;
}

bool CGraphicThingInstance::SetModelInstance(int32_t iDstModelInstance, int32_t iSrcModelThing, int32_t iSrcModel, int32_t skeletonModelInstance)
{
	if (!CheckModelInstanceIndex(iDstModelInstance))
	{
		SysLog("iDstModelInstance={0} pModelThing={1} iSrcModel={2}", iDstModelInstance, iSrcModelThing, iSrcModel);
		return false;
	}

	if (!CheckModelThingIndex(iSrcModelThing))
	{
		SysLog("iDstModelInstance={0} pModelThing={1} iSrcModel={2}", iDstModelInstance, iSrcModelThing, iSrcModel);
		return false;
	}

	if (m_modelInstances[iDstModelInstance])
	{
		CGrannyModelInstance::Delete(m_modelInstances[iDstModelInstance]);
		m_modelInstances[iDstModelInstance] = nullptr;
	}

	CGraphicThing::TRef* modelThing = m_pModelThings[iSrcModelThing];
	if (modelThing->IsNull())
		return false;

	if (modelThing->GetPointer()->GetModelCount() <= iSrcModel)
	{
		SysLog("iSrcModel {0} >= model count {1}", iSrcModel, modelThing->GetPointer()->GetModelCount());
		return false;
	}

	CGrannyModelInstance* skeletonMi = nullptr;
	if (skeletonModelInstance != -1)
	{
		if (!CheckModelInstanceIndex(skeletonModelInstance))
		{
			SysLog("skeletonModelInstance={0} out of range", skeletonModelInstance);
			return false;
		}

		skeletonMi = m_modelInstances[skeletonModelInstance];
	}

	CGrannyModelInstance* modelInstance = CGrannyModelInstance::New();
	modelInstance->SetLinkedModelPointer(modelThing->GetPointer()->GetModelPointer(iSrcModel), skeletonMi);
	modelInstance->DeformNoSkin(&ms_matIdentity, 0.0f);

	m_modelInstances[iDstModelInstance] = modelInstance;
	return true;
}

void CGraphicThingInstance::AttachModelInstance(int32_t iDstModelInstance, CGraphicThingInstance & rSrcInstance, int32_t iSrcModelInstance, int32_t boneIndex)
{
	if (!CheckModelInstanceIndex(iDstModelInstance))
	{
		SysLog("iDstModelInstance {0} out-of-range", iSrcModelInstance);
		return;
	}

	if (!rSrcInstance.CheckModelInstanceIndex(iSrcModelInstance))
	{
		SysLog("iSrcModelInstance {0} out-of-range", iSrcModelInstance);
		return;
	}

	CGrannyModelInstance* srcMi = rSrcInstance.m_modelInstances[iSrcModelInstance];
	CGrannyModelInstance* dstMi = m_modelInstances[iDstModelInstance];

	assert(srcMi && "No model bound");
	srcMi->SetParentModelInstance(dstMi, boneIndex);
}

void CGraphicThingInstance::DetachModelInstance(int32_t iDstModelInstance, CGraphicThingInstance & rSrcInstance, int32_t iSrcModelInstance)
{
	if (!CheckModelInstanceIndex(iDstModelInstance))
	{
		SysLog("iDstModelInstance {0} out-of-range", iSrcModelInstance);
		return;
	}

	if (!rSrcInstance.CheckModelInstanceIndex(iSrcModelInstance))
	{
		SysLog("iSrcModelInstance {1} out-of-range", iSrcModelInstance);
		return;
	}

	CGrannyModelInstance* srcMi = rSrcInstance.m_modelInstances[iSrcModelInstance];
	assert(srcMi && "No model bound");

	srcMi->SetParentModelInstance(nullptr, 0);
}

bool CGraphicThingInstance::FindBoneIndex(int32_t iModelInstance, const char* c_szBoneName, int32_t * iRetBone)
{
	assert(CheckModelInstanceIndex(iModelInstance));

	CGrannyModelInstance* pModelInstance = m_modelInstances[iModelInstance];

	if (!pModelInstance)
		return false;

	return pModelInstance->GetBoneIndexByName(c_szBoneName, iRetBone);
}

bool CGraphicThingInstance::GetBoneMatrixPointer(int32_t iModelInstance, const char* c_szBoneName, float** boneMatrix)
{
	assert(CheckModelInstanceIndex(iModelInstance));

	CGrannyModelInstance* pModelInstance = m_modelInstances[iModelInstance];
	if (!pModelInstance)
	{
		return false;
	}

	int32_t retBone;
	if (!pModelInstance->GetBoneIndexByName(c_szBoneName, &retBone)) 
	{
		*boneMatrix = nullptr;
		return false;
	}

	*boneMatrix = (float*)pModelInstance->GetBoneMatrixPointer(retBone);
	return *boneMatrix != nullptr;
}

bool CGraphicThingInstance::GetBoneMatrixPointer(int32_t iModelInstance, const char* c_szBoneName, D3DXMATRIX** boneMatrix)
{
	assert(CheckModelInstanceIndex(iModelInstance));

	CGrannyModelInstance* pModelInstance = m_modelInstances[iModelInstance];
	if (!pModelInstance) 
	{
		return false;
	}

	int32_t retBone;
	if (!pModelInstance->GetBoneIndexByName(c_szBoneName, &retBone)) 
	{
		*boneMatrix = nullptr;
		return false;
	}

	*boneMatrix = (D3DXMATRIX*)pModelInstance->GetBoneMatrixPointer(retBone);
	return *boneMatrix != nullptr;
}

bool CGraphicThingInstance::GetBonePosition(int32_t iModelIndex, int32_t iBoneIndex, float* pfx, float* pfy, float* pfz)
{
	assert(CheckModelInstanceIndex(iModelIndex));

	CGrannyModelInstance* pModelInstance = m_modelInstances[iModelIndex];

	if (!pModelInstance)
		return false;

	const float* pfMatrix = pModelInstance->GetBoneMatrixPointer(iBoneIndex);

	*pfx = pfMatrix[12];
	*pfy = pfMatrix[13];
	*pfz = pfMatrix[14];
	return true;
}

void CGraphicThingInstance::ResetLocalTime()
{
	m_fLastLocalTime = 0.0f;
	m_fLocalTime = 0.0f;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->ResetLocalTime();
	}
}

void CGraphicThingInstance::InsertDelay(float fDelay)
{
	m_fDelay = fDelay;
}

void CGraphicThingInstance::__SetLocalTime(float fLocalTime)
{
	m_fLastLocalTime = m_fLocalTime;
	m_fLocalTime = fLocalTime;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->SetLocalTime(fLocalTime);
	}
}

float CGraphicThingInstance::GetLastLocalTime() const
{
	return m_fLastLocalTime;
}

float CGraphicThingInstance::GetLocalTime() const
{
	return m_fLocalTime;
}

float CGraphicThingInstance::GetSecondElapsed() const
{
	return m_fSecondElapsed;
}

float CGraphicThingInstance::GetAverageSecondElapsed() const
{
	return m_fAverageSecondElapsed;
}

void CGraphicThingInstance::SetMaterialImagePointer(uint32_t ePart, const char* c_szImageName, CGraphicImage * pImage)
{
	if (ePart >= m_modelInstances.size())
	{
		SysLog("ePart {0} >= model instance count {1} image {2} {3}", ePart, m_modelInstances.size(), c_szImageName, pImage->GetFileName());
		return;
	}

	if (!m_modelInstances[ePart])
	{
		SysLog("ePart {0} NULL - model instance count {1} image {2} {3}", ePart, m_modelInstances.size(), c_szImageName, pImage->GetFileName());
		return;
	}

	m_modelInstances[ePart]->SetMaterialImagePointer(c_szImageName, pImage);
}

void CGraphicThingInstance::SetMaterialData(uint32_t ePart, const char* c_szImageName, const SMaterialData & kMaterialData)
{
	if (ePart >= m_modelInstances.size())
	{
		SysLog("ePart {0} >= model instance count {1} image {2}", ePart, m_modelInstances.size(), c_szImageName);
		return;
	}

	if (!m_modelInstances[ePart])
	{
		SysLog("ePart {0} NULL - model instance count {1} image {2}", ePart, m_modelInstances.size(), c_szImageName);
		return;
	}

	m_modelInstances[ePart]->SetMaterialData(c_szImageName, kMaterialData);
}

void CGraphicThingInstance::SetSpecularInfo(uint32_t ePart, const char* c_szMtrlName, bool bEnable, float fPower)
{
	if (ePart >= m_modelInstances.size())
	{
		SysLog("ePart {0} >= model instance count {1} image {2}", ePart, m_modelInstances.size(), c_szMtrlName);
		return;
	}

	if (!m_modelInstances[ePart])
	{
		SysLog("ePart {0} NULL - model instance count {1} image {2}", ePart, m_modelInstances.size(), c_szMtrlName);
		return;
	}

	m_modelInstances[ePart]->SetSpecularInfo(c_szMtrlName, bEnable, fPower);
}

float CGraphicThingInstance::GetHeight()
{
	if (m_modelInstances.empty())
		return 0.0f;

	CGrannyModelInstance* pModelInstance = m_modelInstances[0];
	if (!pModelInstance)
		return 0.0f;

	D3DXVECTOR3 vtMin, vtMax;
	pModelInstance->GetBoundBox(&vtMin, &vtMax);

	return fabs(vtMin.z - vtMax.z);
}

void CGraphicThingInstance::RenderWithOneTexture(bool showWeapon)
{
	if (!m_bUpdated)
		return;

	for (size_t i = 0; i < m_modelInstances.size(); i++)
	{
		CGrannyModelInstance* mi = m_modelInstances[i];
		if (!mi)
			continue;

		if ((i == PART_WEAPON_LEFT || i == PART_WEAPON) && !showWeapon /* && !Engine::GetSettings().IsShowWeaponShadow()*/)
			continue;

		mi->RenderWithOneTexture();
	}
}

void CGraphicThingInstance::BlendRenderWithOneTexture()
{
	if (!m_bUpdated)
		return;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->BlendRenderWithOneTexture();
	}
}

void CGraphicThingInstance::RenderWithTwoTexture()
{
	if (!m_bUpdated)
		return;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->RenderWithTwoTexture();
	}
}

void CGraphicThingInstance::BlendRenderWithTwoTexture()
{
	if (!m_bUpdated)
		return;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->BlendRenderWithTwoTexture();
	}
}

uint32_t CGraphicThingInstance::GetModelInstanceCount() const
{
	return m_modelInstances.size();
}

CGrannyModelInstance* CGraphicThingInstance::GetModelInstancePointer(uint32_t dwModelIndex) const
{
	assert(dwModelIndex < m_modelInstances.size());
	return m_modelInstances[dwModelIndex];
}

void CGraphicThingInstance::ReloadTexture()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->ReloadTexture();
	}
}

CGraphicThing* CGraphicThingInstance::GetBaseThingPtr()
{
	if (m_pModelThings.empty())
		return nullptr;

	return m_pModelThings[0]->GetPointer();
}

void CGraphicThingInstance::SetMotionAtEnd()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->SetMotionAtEnd();
	}
}

bool CGraphicThingInstance::Intersect(float* pu, float* pv, float* pt)
{
	if (!CGraphicObjectInstance::isShow())
		return false;

	if (!m_bUpdated)
		return false;

	if (m_modelInstances.empty() || !m_modelInstances[0])
		return false;

	return m_modelInstances[0]->Intersect(&GetTransform(), pu, pv, pt);
}

void CGraphicThingInstance::GetBoundBox(D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax)
{
	vtMin->x = vtMin->y = vtMin->z = 100000.0f;
	vtMax->x = vtMax->y = vtMax->z = -100000.0f;
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->GetBoundBox(vtMin, vtMax);
	}
}

bool CGraphicThingInstance::GetBoundBox(uint32_t dwModelInstanceIndex, D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax)
{
	if (!CheckModelInstanceIndex(dwModelInstanceIndex))
		return false;

	vtMin->x = vtMin->y = vtMin->z = 100000.0f;
	vtMax->x = vtMax->y = vtMax->z = -100000.0f;

	CGrannyModelInstance* modelInstance = m_modelInstances[dwModelInstanceIndex];
	if (!modelInstance)
		return false;

	modelInstance->GetBoundBox(vtMin, vtMax);
	return true;
}

bool CGraphicThingInstance::GetBoneMatrix(uint32_t dwModelInstanceIndex, uint32_t dwBoneIndex, D3DXMATRIX * *ppMatrix)
{
	if (!CheckModelInstanceIndex(dwModelInstanceIndex))
		return false;

	CGrannyModelInstance* modelInstance = m_modelInstances[dwModelInstanceIndex];
	if (!modelInstance)
		return false;

	*ppMatrix = (D3DXMATRIX*)modelInstance->GetBoneMatrixPointer(dwBoneIndex);
	if (!*ppMatrix)
		return false;

	return true;
}

bool CGraphicThingInstance::GetCompositeBoneMatrix(uint32_t dwModelInstanceIndex, uint32_t dwBoneIndex, D3DXMATRIX * *ppMatrix)
{
	if (!CheckModelInstanceIndex(dwModelInstanceIndex))
		return false;

	CGrannyModelInstance* modelInstance = m_modelInstances[dwModelInstanceIndex];
	if (!modelInstance)
	{
		return false;
	}

	*ppMatrix = (D3DXMATRIX*)modelInstance->GetCompositeBoneMatrixPointer(dwBoneIndex);
	return true;
}

void CGraphicThingInstance::UpdateTransform(D3DXMATRIX * pMatrix, float fSecondsElapsed, int32_t iModelInstanceIndex)
{
	if (iModelInstanceIndex >= m_modelInstances.size())
	{
		return;
	}

	CGrannyModelInstance* modelInstance = m_modelInstances[iModelInstanceIndex];
	if (!modelInstance)
	{
		return;
	}

	modelInstance->UpdateTransform(pMatrix, fSecondsElapsed);
}

void CGraphicThingInstance::BuildBoundingSphere()
{
	D3DXVECTOR3 v3Min, v3Max;
	GetBoundBox(0, &v3Min, &v3Max);
	m_v3Center = (v3Min + v3Max) * 0.5f;
	D3DXVECTOR3 vDelta = (v3Max - v3Min);

	m_fRadius = D3DXVec3Length(&vDelta) * 0.5f + 50.0f; // + Extra length for attached objects
}

void CGraphicThingInstance::BuildBoundingAABB()
{
	D3DXVECTOR3 v3Min, v3Max;
	GetBoundBox(0, &v3Min, &v3Max);
	m_v3Center = (v3Min + v3Max) * 0.5f;
	m_v3Min = v3Min;
	m_v3Max = v3Max;
}

void CGraphicThingInstance::CalculateBBox()
{
	GetBoundBox(&m_v3BBoxMin, &m_v3BBoxMax);

	m_v4TBBox[0] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMin.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[1] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMax.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[2] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMin.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[3] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMax.y, m_v3BBoxMin.z, 1.0f);
	m_v4TBBox[4] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMin.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[5] = D3DXVECTOR4(m_v3BBoxMin.x, m_v3BBoxMax.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[6] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMin.y, m_v3BBoxMax.z, 1.0f);
	m_v4TBBox[7] = D3DXVECTOR4(m_v3BBoxMax.x, m_v3BBoxMax.y, m_v3BBoxMax.z, 1.0f);

	const D3DXMATRIX& c_rmatTransform = GetTransform();

	for (uint32_t i = 0; i < 8; ++i)
	{
		D3DXVec4Transform(&m_v4TBBox[i], &m_v4TBBox[i], &c_rmatTransform);
		if (0 == i)
		{
			m_v3TBBoxMin.x = m_v4TBBox[i].x;
			m_v3TBBoxMin.y = m_v4TBBox[i].y;
			m_v3TBBoxMin.z = m_v4TBBox[i].z;
			m_v3TBBoxMax.x = m_v4TBBox[i].x;
			m_v3TBBoxMax.y = m_v4TBBox[i].y;
			m_v3TBBoxMax.z = m_v4TBBox[i].z;
		}
		else
		{
			if (m_v3TBBoxMin.x > m_v4TBBox[i].x)
				m_v3TBBoxMin.x = m_v4TBBox[i].x;
			if (m_v3TBBoxMax.x < m_v4TBBox[i].x)
				m_v3TBBoxMax.x = m_v4TBBox[i].x;
			if (m_v3TBBoxMin.y > m_v4TBBox[i].y)
				m_v3TBBoxMin.y = m_v4TBBox[i].y;
			if (m_v3TBBoxMax.y < m_v4TBBox[i].y)
				m_v3TBBoxMax.y = m_v4TBBox[i].y;
			if (m_v3TBBoxMin.z > m_v4TBBox[i].z)
				m_v3TBBoxMin.z = m_v4TBBox[i].z;
			if (m_v3TBBoxMax.z < m_v4TBBox[i].z)
				m_v3TBBoxMax.z = m_v4TBBox[i].z;
		}
	}
}

bool CGraphicThingInstance::GetBoundingSphere(D3DXVECTOR3 & v3Center, float& fRadius)
{
	if (m_fRadius <= 0)
	{
		BuildBoundingSphere();

		fRadius = m_fRadius;
		v3Center = m_v3Center;
	}
	else
	{
		fRadius = m_fRadius;
		v3Center = m_v3Center;
	}

	D3DXVec3TransformCoord(&v3Center, &v3Center, &GetTransform());
	return true;
}

bool CGraphicThingInstance::GetBoundingAABB(D3DXVECTOR3 & v3Min, D3DXVECTOR3 & v3Max)
{
	BuildBoundingAABB();

	v3Min = m_v3Min;
	v3Max = m_v3Max;

	D3DXVec3TransformCoord(&m_v3Center, &m_v3Center, &GetTransform());
	return true;
}

bool CGraphicThingInstance::HaveBlendThing()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		if (modelInstance->HaveBlendThing())
			return true;
	}
	return false;
}

void CGraphicThingInstance::OnClear()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		CGrannyModelInstance::Delete(modelInstance);
	}

	m_modelInstances.clear();
	m_pModelThings.clear();
}

void CGraphicThingInstance::OnUpdate()
{
	UpdateLODLevel();
}

void CGraphicThingInstance::OnRender()
{
	RenderWithOneTexture(true);
}

void CGraphicThingInstance::OnBlendRender()
{
	BlendRenderWithOneTexture();
}

void CGraphicThingInstance::OnRenderToShadowMap(bool showWeapon)
{
	if (!m_bUpdated)
		return;

	for (size_t i = 0; i < m_modelInstances.size(); i++)
	{
		CGrannyModelInstance* modelInstance = m_modelInstances[i];
		if (!modelInstance)
			continue;

		if ((i == PART_WEAPON_LEFT || i == PART_WEAPON) && !showWeapon /* && !Engine::GetSettings().IsShowWeaponShadow()*/)
			continue;

		modelInstance->RenderWithoutTexture();
	}
}

void CGraphicThingInstance::OnRenderShadow()
{
	if (!m_bUpdated)
		return;

	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
			continue;

		modelInstance->RenderWithOneTexture();
	}
}

void CGraphicThingInstance::OnRenderPCBlocker()
{
	for (CGrannyModelInstance* modelInstance : m_modelInstances)
	{
		if (!modelInstance)
		{
			continue;
		}

		modelInstance->RenderWithOneTexture();
		modelInstance->BlendRenderWithOneTexture();
	}
}

void CGraphicThingInstance::OnUpdateCollisionData(const CStaticCollisionDataVector * pscdVector)
{
	assert(pscdVector);
	for (const CStaticCollisionData& collisionD : *pscdVector)
		AddCollision(&collisionD, &GetTransform());
}

void CGraphicThingInstance::OnUpdateHeighInstance(CAttributeInstance * pAttributeInstance)
{
	assert(pAttributeInstance);
	SetHeightInstance(pAttributeInstance);
}

bool CGraphicThingInstance::OnGetObjectHeight(float fX, float fY, float* pfHeight)
{
	if (m_pHeightAttributeInstance && m_pHeightAttributeInstance->GetHeight(fX, fY, pfHeight))
		return true;
	return false;
}

/////////////////
void CGraphicThingInstance::CreateSystem(uint32_t uCapacity)
{
	ms_kPool.Create(uCapacity);
}

void CGraphicThingInstance::DestroySystem()
{
	m_roMotionThingMap.clear();
	ms_kPool.Destroy();
}

CGraphicThingInstance* CGraphicThingInstance::New()
{
	return ms_kPool.Alloc();
}

void CGraphicThingInstance::Delete(CGraphicThingInstance * pkThingInst)
{
	pkThingInst->Clear();
	ms_kPool.Free(pkThingInst);
}