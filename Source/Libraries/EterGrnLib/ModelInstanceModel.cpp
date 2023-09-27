#include "StdAfx.h"
#include "ModelInstance.h"
#include "Model.h"

void CGrannyModelInstance::Clear()
{
	m_kMtrlPal.Clear();

	DestroyDeviceObjects();
	__DestroyMeshBindingVector();
	__DestroyMeshMatrices();
	__DestroyModelInstance();
	__DestroyWorldPose();
	__Initialize();
}

void CGrannyModelInstance::SetLinkedModelPointer(CGrannyModel* model, CGrannyModelInstance* skeletonInstance)
{
	Clear();

	if (m_model)
		m_model->Release();

	m_model = model;
	m_model->AddReference();

	__CreateDynamicVertexBuffer();
	__CreateModelInstance();

	m_skeletonInstance = skeletonInstance;

	if (skeletonInstance) 
	{
		__CreateMeshBindingVector(skeletonInstance->GetModel());
	}
	else 
	{
		__CreateWorldPose();
		__CreateMeshBindingVector(m_model);
	}

	__CreateMeshMatrices();

	ResetLocalTime();

	m_kMtrlPal = model->GetMaterialPalette();
}

granny_world_pose* CGrannyModelInstance::__GetWorldPosePtr() const
{
	if (m_worldPose)
		return m_worldPose;

	if (m_skeletonInstance)
		return m_skeletonInstance->__GetWorldPosePtr();

	return nullptr;
}

const granny_int32x* CGrannyModelInstance::__GetMeshBoneIndices(uint32_t iMeshBinding) const
{
	assert(iMeshBinding < m_vct_pgrnMeshBinding.size());
	return GrannyGetMeshBindingToBoneIndices(m_vct_pgrnMeshBinding[iMeshBinding]);
}

bool CGrannyModelInstance::__CreateMeshBindingVector(CGrannyModel* skeletonModel)
{
	assert(m_vct_pgrnMeshBinding.empty());

	if (!m_model)
		return false;

	granny_model* sourceSkel = m_model->GetGrannyModelPointer();
	assert(sourceSkel && "No source skeleton model");

	granny_model* destSkel = skeletonModel->GetGrannyModelPointer();
	assert(destSkel && "No destination skeleton model");

	m_vct_pgrnMeshBinding.reserve(sourceSkel->MeshBindingCount);
	for (int32_t i = 0; i != sourceSkel->MeshBindingCount; ++i)
	{
		granny_mesh_binding* Binding = GrannyNewMeshBinding(sourceSkel->MeshBindings[i].Mesh, sourceSkel->Skeleton, destSkel->Skeleton);
		m_vct_pgrnMeshBinding.emplace_back(Binding);
	}

	return true;
}

void CGrannyModelInstance::__DestroyMeshBindingVector()
{
	std::for_each(m_vct_pgrnMeshBinding.begin(), m_vct_pgrnMeshBinding.end(), GrannyFreeMeshBinding);
	m_vct_pgrnMeshBinding.clear();
}

void CGrannyModelInstance::__CreateWorldPose()
{
	assert(m_modelInstance != nullptr);
	assert(m_worldPose == nullptr);

	granny_skeleton* grnSkeleton = GrannyGetSourceSkeleton(m_modelInstance);
	m_worldPose = GrannyNewWorldPose(grnSkeleton->BoneCount);
}

void CGrannyModelInstance::__DestroyWorldPose()
{
	if (!m_worldPose)
		return;

	GrannyFreeWorldPose(m_worldPose);
	m_worldPose = nullptr;
}

void CGrannyModelInstance::__CreateModelInstance()
{
	assert(m_model);
	assert(m_modelInstance == nullptr);

	m_modelInstance = GrannyInstantiateModel(m_model->GetGrannyModelPointer());
}

void CGrannyModelInstance::__DestroyModelInstance()
{
	if (!m_modelInstance)
		return;

	GrannyFreeModelInstance(m_modelInstance);
	m_modelInstance = nullptr;
}

void CGrannyModelInstance::__CreateMeshMatrices()
{
	assert(m_model != nullptr);

	if (m_model->GetMeshCount() <= 0) // Sometimes there are models without a mesh (such as a camera). you dont say lol!
		return;

	const int32_t meshCount = m_model->GetMeshCount();
	m_meshMatrices = new D3DXMATRIX[meshCount];
}

void CGrannyModelInstance::__DestroyMeshMatrices()
{
	if (!m_meshMatrices)
		return;

	delete[] m_meshMatrices;
	m_meshMatrices = nullptr;
}

uint32_t CGrannyModelInstance::GetDeformableVertexCount()
{
	if (!m_model)
		return 0;

	return m_model->GetDeformVertexCount();
}

uint32_t CGrannyModelInstance::GetVertexCount()
{
	if (!m_model)
		return 0;

	return m_model->GetVertexCount();
}

CGraphicVertexBuffer& CGrannyModelInstance::__GetDeformableVertexBufferRef()
{
	return m_deformVertexBuffer;
}

void CGrannyModelInstance::__CreateDynamicVertexBuffer()
{
	assert(m_model != nullptr);
	assert(m_deformVertexBuffer.IsEmpty());

	const int32_t vtxCount = m_model->GetDeformVertexCount();
	if (vtxCount == 0)
		return;

	const uint32_t fvf = m_model->GetFvF();

	if (!m_deformVertexBuffer.Create(vtxCount, fvf, D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED))
		return;
}

void CGrannyModelInstance::__DestroyDynamicVertexBuffer()
{
	m_deformVertexBuffer.Destroy();
}

bool CGrannyModelInstance::GetBoneIndexByName(const char* c_szBoneName, int32_t* pBoneIndex) const
{
	assert(m_modelInstance != nullptr);

	granny_skeleton const* grnSkeleton = GrannyGetSourceSkeleton(m_modelInstance);

	if (!GrannyFindBoneByName(grnSkeleton, c_szBoneName, pBoneIndex))
		return false;

	return true;
}

const float* CGrannyModelInstance::GetBoneMatrixPointer(int32_t iBone) const
{
	const float* bones = GrannyGetWorldPose4x4(__GetWorldPosePtr(), iBone);
	if (!bones)
	{
		granny_model* pModel = m_model->GetGrannyModelPointer();
		SysLog("GrannyModelInstance({0}).GetBoneMatrixPointer(boneIndex({1})).NOT_FOUND_BONE", pModel->Name, iBone);
		return nullptr;
	}
	return bones;
}

const float* CGrannyModelInstance::GetCompositeBoneMatrixPointer(int32_t iBone) const
{
	// NOTE: GrannyGetWorldPose4x4 may have wrong scale values.
	return GrannyGetWorldPoseComposite4x4(__GetWorldPosePtr(), iBone);
}

void CGrannyModelInstance::ReloadTexture()
{
	assert(m_model != nullptr);
	CGrannyMaterialPalette& c_rGrannyMaterialPalette = const_cast<CGrannyMaterialPalette&>(m_model->GetMaterialPalette());
	const uint32_t dwMaterialCount = c_rGrannyMaterialPalette.GetMaterialCount();

	for (uint32_t dwMtrIndex = 0; dwMtrIndex < dwMaterialCount; ++dwMtrIndex)
	{
		const CGrannyMaterial& c_rGrannyMaterial = c_rGrannyMaterialPalette.GetMaterialRef(dwMtrIndex);

		CGraphicImage* pImageStage0 = c_rGrannyMaterial.GetImagePointer(0);
		if (pImageStage0)
			pImageStage0->Reload();

		CGraphicImage* pImageStage1 = c_rGrannyMaterial.GetImagePointer(1);
		if (pImageStage1)
			pImageStage1->Reload();
	}
}
