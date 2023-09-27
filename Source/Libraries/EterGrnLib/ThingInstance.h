#pragma once
#include <Eterlib/GrpObjectInstance.h>
#include <EterLib/GrpImage.h>
#include "Thing.h"
#include <Basic/robin_hood.hpp>
#include <vector>

class CGrannyModelInstance;

struct SMaterialData;
class CGraphicThingInstance : public CGraphicObjectInstance
{
public:
	enum
	{
		ID = THING_OBJECT
	};

	int32_t GetType() const override { return ID; }

	CGraphicThingInstance();
	~CGraphicThingInstance() override = default;

	void OnInitialize() override;

	void DeformNoSkin();
	void TransformAttachment();

	void UpdateLODLevel();
	void UpdateTime();
		
	bool LessRenderOrder(CGraphicThingInstance* pkThingInst);

	bool Picking(const D3DXVECTOR3& v, const D3DXVECTOR3& dir, float& out_x, float& out_y);

	bool CreateDeviceObjects();
	void DestroyDeviceObjects();

	void ReserveModelInstance(int32_t iCount);
	void ReserveModelThing(int32_t iCount);

	void RegisterModelThing(int32_t iModelThing, CGraphicThing* pModelThing);
	bool CheckModelInstanceIndex(int32_t iModelInstance);
	bool CheckModelThingIndex(int32_t iModelThing);

	static void RegisterMotionThing(uint32_t motionId, uint32_t dwMotionKey, CGraphicThing* pMotionThing);
	static bool CheckMotionThingIndex(uint32_t dwMotionKey, uint32_t motionId);
	static bool GetMotionThingPointer(uint32_t motionId, uint32_t dwKey, CGraphicThing** ppMotion);
	bool IsMotionThing();

	bool SetModelInstance(int32_t iDstModelInstance, int32_t iSrcModelThing, int32_t iSrcModel, int32_t skeletonModelInstance = -1);
	void AttachModelInstance(int32_t iDstModelInstance, CGraphicThingInstance& rsrcInstance, int32_t iSrcModelInstance, int32_t boneIndex);
	void DetachModelInstance(int32_t iDstModelInstance, CGraphicThingInstance& rSrcInstance, int32_t SrcModelInstance);

	bool FindBoneIndex(int32_t iModelInstance, const char* c_szBoneName, int32_t* iRetBone);
	bool GetBoneMatrixPointer(int32_t iModelInstance, const char* c_szBoneName, float** boneMatrix);
	bool GetBoneMatrixPointer(int32_t iModelInstance, const char* c_szBoneName, D3DXMATRIX** boneMatrix);
	bool GetBonePosition(int32_t iModelIndex, int32_t iBoneIndex, float* pfx, float* pfy, float* pfz);

	void ResetLocalTime();
	void InsertDelay(float fDelay);
	void __SetLocalTime(float fLocalTime); // Only Used by Tools
	float GetLastLocalTime() const;
	float GetLocalTime() const;
	float GetSecondElapsed() const;
	float GetAverageSecondElapsed() const;

	void SetMaterialImagePointer(uint32_t ePart, const char* c_szImageName, CGraphicImage* pImage);
	void SetMaterialData(uint32_t ePart, const char* c_szImageName, const SMaterialData& kMaterialData);
	void SetSpecularInfo(uint32_t ePart, const char* c_szMtrlName, bool bEnable, float fPower);

	float GetHeight();

	void RenderWithOneTexture(bool showWeapon);
	void RenderWithTwoTexture();
	void BlendRenderWithOneTexture();
	void BlendRenderWithTwoTexture();

	uint32_t GetModelInstanceCount() const;
	CGrannyModelInstance* GetModelInstancePointer(uint32_t dwModelIndex) const;

	void ReloadTexture();
	CGraphicThing* GetBaseThingPtr();

	bool SetMotion(uint32_t dwMotionKey, float blendTime = 0.0f, int32_t loopCount = 0, float speedRatio = 1.0f);
	bool ChangeMotion(uint32_t dwMotionKey, int32_t loopCount = 0, float speedRatio = 1.0f);

	void SetEndStopMotion() { SetMotionAtEnd(); }
	void SetMotionAtEnd();

	bool Intersect(float* pu, float* pv, float* pt);
	void GetBoundBox(D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax);
	bool GetBoundBox(uint32_t dwModelInstanceIndex, D3DXVECTOR3* vtMin, D3DXVECTOR3* vtMax);
	bool GetBoneMatrix(uint32_t dwModelInstanceIndex, uint32_t dwBoneIndex, D3DXMATRIX** ppMatrix);
	bool GetCompositeBoneMatrix(uint32_t dwModelInstanceIndex, uint32_t dwBoneIndex, D3DXMATRIX** ppMatrix);
	void UpdateTransform(D3DXMATRIX* pMatrix, float fSecondsElapsed = 0.0f, int32_t iModelInstanceIndex = 0);

	void BuildBoundingSphere();
	void BuildBoundingAABB();
	virtual void CalculateBBox();
	bool GetBoundingSphere(D3DXVECTOR3& v3Center, float& fRadius) override;
	virtual bool GetBoundingAABB(D3DXVECTOR3& v3Min, D3DXVECTOR3& v3Max);

	bool HaveBlendThing();

	void SetMotionID(uint32_t motionID)
	{
		m_dwMotionID = motionID;
	}

	uint32_t GetMotionID()
	{
		return m_dwMotionID;
	}

protected:
	void OnClear() override;
	void OnDeform() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnBlendRender() override;
	void OnRenderToShadowMap(bool showWeapon) override;
	void OnRenderShadow() override;
	void OnRenderPCBlocker() override;

	void OnUpdateCollisionData(const CStaticCollisionDataVector* pscdVector) override;
	void OnUpdateHeighInstance(CAttributeInstance* pAttributeInstance) override;
	bool OnGetObjectHeight(float fX, float fY, float* pfHeight) override;

	bool m_bUpdated;
	float m_fLastLocalTime;
	float m_fLocalTime;
	float m_fDelay;
	float m_fSecondElapsed;
	float m_fAverageSecondElapsed;
	float m_fRadius;
	float m_allowedError;
	D3DXVECTOR3 m_v3Center;
	D3DXVECTOR3	m_v3Min, m_v3Max;

	std::vector<CGrannyModelInstance*> m_modelInstances;
	std::vector<CGraphicThing::TRef*>  m_pModelThings;
	uint32_t m_dwMotionID;
	static robin_hood::unordered_map<uint32_t, robin_hood::unordered_map<uint32_t, CGraphicThing::TRef*>> m_roMotionThingMap;

public:
	static void CreateSystem(uint32_t uCapacity);
	static void DestroySystem();
	static CGraphicThingInstance* New();
	static void Delete(CGraphicThingInstance* pkInst);
	static CDynamicPool<CGraphicThingInstance> ms_kPool;
};
