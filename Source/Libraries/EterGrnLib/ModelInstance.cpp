#include "StdAfx.h"
#include "ModelInstance.h"
#include "Model.h"
#include <EterLib/ResourceManager.h>

CGrannyModel* CGrannyModelInstance::GetModel()
{
	return m_model;
}

void CGrannyModelInstance::SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage)
{
	m_kMtrlPal.SetMaterialImage(c_szImageName, pImage);
}

void CGrannyModelInstance::SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData)
{
	m_kMtrlPal.SetMaterialData(c_szImageName, c_rkMaterialData);
}

void CGrannyModelInstance::SetSpecularInfo(const char* c_szMtrlName, bool bEnable, float fPower)
{
	m_kMtrlPal.SetSpecularInfo(c_szMtrlName, bEnable, fPower);
}

void CGrannyModelInstance::SetLocalTime(float fLocalTime)
{
	m_fLocalTime = fLocalTime;
}

void CGrannyModelInstance::ResetLocalTime()
{
	m_fLocalTime = 0.0;
}

float CGrannyModelInstance::GetLocalTime() const
{
	return m_fLocalTime;
}

void CGrannyModelInstance::SetParentModelInstance(const CGrannyModelInstance* parent, int32_t iBone)
{
	m_attachedTo = parent;
	m_attachedToBone = iBone;
}

bool CGrannyModelInstance::IsEmpty()
{
	if (m_model)
	{
		if (!m_meshMatrices)
			return true;

		return false;
	}

	return true;
}

bool CGrannyModelInstance::CreateDeviceObjects()
{
	__CreateDynamicVertexBuffer();
	return true;
}

void CGrannyModelInstance::DestroyDeviceObjects()
{
	__DestroyDynamicVertexBuffer();
}

void CGrannyModelInstance::__Initialize()
{
	if (m_model)
		m_model->Release();

	m_model = nullptr;
	m_attachedTo = nullptr;
	m_attachedToBone = 0;
	m_skeletonInstance = nullptr;

	m_modelInstance = nullptr;

	m_worldPose = nullptr;

	D3DXMatrixIdentity(&m_worldTransform);

	m_meshMatrices = nullptr;
	m_pgrnCtrl = nullptr;
	m_pgrnAni = nullptr;
}

CGrannyModelInstance::CGrannyModelInstance()
{
	m_model = nullptr;
	__Initialize();
}

CGrannyModelInstance::~CGrannyModelInstance()
{
	Clear();
}

CDynamicPool<CGrannyModelInstance> CGrannyModelInstance::ms_kPool;

CGrannyModelInstance* CGrannyModelInstance::New()
{
	return ms_kPool.Alloc();
}

void CGrannyModelInstance::Delete(CGrannyModelInstance* pkInst)
{
	pkInst->Clear();
	ms_kPool.Free(pkInst);
}

void CGrannyModelInstance::DestroySystem()
{
	ms_kPool.Destroy();
}
