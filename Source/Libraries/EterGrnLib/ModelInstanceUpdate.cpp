#include "StdAfx.h"
#include "ModelInstance.h"
#include "Model.h"

class SharedLocalPose
{
public:
	SharedLocalPose(uint32_t boneCount): m_localPose(GrannyNewLocalPose(boneCount))
	{
		// ctor
	}

	~SharedLocalPose()
	{
		GrannyFreeLocalPose(m_localPose);
	}

	granny_local_pose* Get(uint32_t boneCount)
	{
		granny_int32x val2 = GrannyGetLocalPoseBoneCount(m_localPose);
		if (boneCount > (uint32_t)val2)
		{
			GrannyFreeLocalPose(m_localPose);
			m_localPose = GrannyNewLocalPose(boneCount);
		}
		return m_localPose;
	}

private:
	granny_local_pose* m_localPose;
};

void CGrannyModelInstance::AdvanceTime(float fElapsedTime)
{
	m_fLocalTime += fElapsedTime;
	// Sample and blend the poses.
	GrannySetModelClock(m_modelInstance, m_fLocalTime);
	// Free up any animation control! (This prevents dying in black screen)
	GrannyFreeCompletedModelControls(m_modelInstance);
}

void CGrannyModelInstance::UpdateTransform(D3DXMATRIX* pMatrix, float fSecondsElapsed)
{
	if (!m_modelInstance)
	{
		SysLog("CGrannyModelIstance::UpdateTransform - m_modelInstance = NULL");
		return;
	}

	GrannyUpdateModelMatrix(m_modelInstance, fSecondsElapsed, (const float*)pMatrix, (float*)pMatrix, false);
}

void CGrannyModelInstance::SetWorldTransform(const D3DXMATRIX& transform)
{
	m_worldTransform = transform;
}

void CGrannyModelInstance::Deform(const D3DXMATRIX * c_pWorldMatrix, float allowedError)
{
	if (IsEmpty())
		return;

	UpdateWorldPose(allowedError);
	UpdateWorldMatrices(c_pWorldMatrix);

	if (m_model->CanDeformPNTVertices())
	{
		// WORK
		CGraphicVertexBuffer& rkDeformableVertexBuffer = __GetDeformableVertexBufferRef();
		TPNTVertex* pntVertices = nullptr;
		if (rkDeformableVertexBuffer.LockRange(m_model->GetDeformVertexCount(), (void**)&pntVertices))
		{
			DeformPNTVertices(pntVertices);
			rkDeformableVertexBuffer.Unlock();
		}
		else
		{
			SysLog("GRANNY DEFORM DYNAMIC BUFFER LOCK ERROR");
		}
		// END_OF_WORK
	}	
}

void CGrannyModelInstance::UpdateSkeleton(const D3DXMATRIX * c_pWorldMatrix, float allowedError)
{	
	UpdateWorldPose(allowedError);
	UpdateWorldMatrices(c_pWorldMatrix);
}

void CGrannyModelInstance::UpdateWorldPose(float allowedError)
{
	if (m_skeletonInstance)
		return;

	assert(m_worldPose && "No world-pose?");

	/* Switched from 64 to 90! */
	static SharedLocalPose sharedLocal(90);

	granny_skeleton* pgrnSkeleton = GrannyGetSourceSkeleton(m_modelInstance);
	granny_local_pose* pgrnLocalPose = sharedLocal.Get(pgrnSkeleton->BoneCount);

	const float* pAttachBoneMatrix = nullptr;
	if (m_attachedTo)
		pAttachBoneMatrix = m_attachedTo->GetBoneMatrixPointer((int32_t)m_attachedToBone);

	GrannySampleModelAnimationsAcceleratedLOD(m_modelInstance, pgrnSkeleton->BoneCount, pAttachBoneMatrix, pgrnLocalPose, m_worldPose, allowedError);
}

void CGrannyModelInstance::UpdateWorldMatrices(const D3DXMATRIX* c_pWorldMatrix)
{
	if (!m_meshMatrices)
		return;

	assert(m_model != nullptr);
	assert(ms_lpd3dMatStack != nullptr);

	D3DXMATRIX tmp = m_worldTransform * *c_pWorldMatrix;

	int32_t meshCount = m_model->GetMeshCount();

	granny_matrix_4x4* pgrnMatCompositeBuffer = GrannyGetWorldPoseComposite4x4Array(__GetWorldPosePtr());
	D3DXMATRIX* boneMatrices = (D3DXMATRIX*)pgrnMatCompositeBuffer;

	for (int32_t i = 0; i < meshCount; ++i)
	{
		D3DXMATRIX& rWorldMatrix = m_meshMatrices[i];

		const CGrannyMesh* pMesh = m_model->GetMeshPointer(i);
		const granny_int32x* boneIndices = __GetMeshBoneIndices(i);

		if (pMesh->CanDeformPNTVertices())
			rWorldMatrix = tmp;
		else
			rWorldMatrix = boneMatrices[*boneIndices] * tmp;
	}
}

void CGrannyModelInstance::DeformPNTVertices(void* pvDest)
{
	assert(m_model != nullptr);
	assert(m_model->CanDeformPNTVertices());

	m_model->DeformPNTVertices(pvDest, __GetWorldPosePtr(), m_vct_pgrnMeshBinding);
}
