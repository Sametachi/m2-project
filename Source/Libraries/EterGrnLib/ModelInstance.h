#pragma once
#include <Eterlib/GrpImage.h>
#include <Eterlib/GrpCollisionObject.h>

#include "Model.h"
#include "Motion.h"

class CGrannyModelInstance : public CGraphicCollisionObject
{
public:
	static void DestroySystem();

	static CGrannyModelInstance* New();
	static void Delete(CGrannyModelInstance* pkInst);

	static CDynamicPool<CGrannyModelInstance> ms_kPool;

public:
	CGrannyModelInstance();
	virtual ~CGrannyModelInstance();

	bool IsEmpty();
	void Clear();

	bool CreateDeviceObjects();
	void DestroyDeviceObjects();

	// Update & Render
	void AdvanceTime(float elapsed);
	void UpdateTransform(D3DXMATRIX* pMatrix, float fSecondsElapsed);

	void SetWorldTransform(const D3DXMATRIX& transform);

	void UpdateSkeleton(const D3DXMATRIX* c_pWorldMatrix, float allowedError);
	void DeformNoSkin(const D3DXMATRIX* c_pWorldMatrix, float allowedError);
	void Deform(const D3DXMATRIX* c_pWorldMatrix, float allowedError);

	// Raw Render
	void RenderWithOneTexture();
	void RenderWithTwoTexture();
	void BlendRenderWithOneTexture();
	void BlendRenderWithTwoTexture();
	void RenderWithoutTexture();

	// Model
	CGrannyModel* GetModel();

	void SetMaterialImagePointer(const char* c_szImageName, CGraphicImage* pImage);
	void SetMaterialData(const char* c_szImageName, const SMaterialData& c_rkMaterialData);
	void SetSpecularInfo(const char* c_szMtrlName, bool bEnable, float fPower);
	void SetLinkedModelPointer(CGrannyModel* model, CGrannyModelInstance* skeletonInstance);

	uint32_t GetDeformableVertexCount();
	uint32_t GetVertexCount();

	// Motion
	void SetMotionPointer(const std::shared_ptr<CGrannyMotion> pMotion, float blendTime = 0.0f, int32_t loopCount = 0, float speedRatio = 1.0f);
	void ChangeMotionPointer(const std::shared_ptr<CGrannyMotion> pMotion, int32_t loopCount = 0, float speedRatio = 1.0f);
	void SetMotionAtEnd();
	bool IsMotionPlaying();
	uint32_t GetLoopIndex();
	void PrintControls();

	// Time
	void SetLocalTime(float fLocalTime);
	void ResetLocalTime();
	float GetLocalTime() const;

	// Bone & Attaching
	const float* GetBoneMatrixPointer(int32_t iBone) const;
	const float* GetCompositeBoneMatrixPointer(int32_t iBone) const;
	bool GetMeshMatrixPointer(int32_t iMesh, const D3DXMATRIX** c_ppMatrix) const;
	bool GetBoneIndexByName(const char* c_szBoneName, int32_t* pBoneIndex) const;
	void SetParentModelInstance(const CGrannyModelInstance* parent, int32_t iBone);

	// Collision Detection 
	// TODO01: Note SCALE function to Collision is implemented in Granny but not inside ObjectInstance and Collision!!
	bool Intersect(const D3DXMATRIX* c_pMatrix, float* pu, float* pv, float* pt, D3DXVECTOR3* pScaleMatrix = nullptr);
	void MakeBoundBox(TBoundBox* pBoundBox, const float* mat, const float* OBBMin, const float* OBBMax, D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax);
	void GetBoundBox(D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax, D3DXVECTOR3* scale = nullptr);

	// Reload Texture
	void ReloadTexture();

	bool HaveBlendThing() { return m_model->HaveBlendThing(); }
	const granny_model_instance* GetModelInstance() const { return m_modelInstance; }

protected:
	void __Initialize();

	void __DestroyModelInstance();
	void __DestroyMeshMatrices();
	void __DestroyDynamicVertexBuffer();

	void __CreateModelInstance();
	void __CreateMeshMatrices();
	void __CreateDynamicVertexBuffer();

	void __DestroyWorldPose();
	void __CreateWorldPose();

	bool __CreateMeshBindingVector(CGrannyModel* skeletonModel);
	void __DestroyMeshBindingVector();

	const granny_int32x* __GetMeshBoneIndices(uint32_t iMeshBinding) const;
	CGraphicVertexBuffer& __GetDeformableVertexBufferRef();
	granny_world_pose* __GetWorldPosePtr() const;

	// Update & Render
	void UpdateWorldPose(float allowedError);
	void UpdateWorldMatrices(const D3DXMATRIX* c_pWorldMatrix);
	void DeformPNTVertices(void* pvDest);

	// Raw Render
	void RenderMeshNodeListWithOneTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithTwoTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithoutTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);
	void RenderMeshNodeListWithThreeTexture(CGrannyMesh::EType eMeshType, CGrannyMaterial::EType eMtrlType);

protected:
	std::vector<granny_mesh_binding*> m_vct_pgrnMeshBinding;
	CGrannyMaterialPalette m_kMtrlPal;
	CGraphicVertexBuffer m_deformVertexBuffer;

	D3DXMATRIX m_worldTransform;
	D3DXMATRIX* m_meshMatrices;

	CGrannyModel* m_model;
	granny_model_instance* m_modelInstance;
	granny_control* m_pgrnCtrl;
	granny_animation* m_pgrnAni;
	CGrannyModelInstance* m_skeletonInstance;

	const CGrannyModelInstance* m_attachedTo;
	granny_world_pose* m_worldPose; // Physical memory allocated here
	uint64_t m_attachedToBone;
	float m_fLocalTime;
};