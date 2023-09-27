#pragma once

class CModelInstanceAccessor : public CGrannyModelInstance
{
	public:
		CModelInstanceAccessor();
		virtual ~CModelInstanceAccessor();

		void ClearModel();
		void ClearMotion();
		void ReleaseArtificialMotion();

		void UpdateLocalTime(float fLocalTime);
		float GetLocalTime();
		float GetDuration();

		BOOL SetAccessorModel(CGraphicThing * pThing);
		BOOL SetAccessorMotion(CGraphicThing * pThing);
		void SetArtificialMotion(const CGrannyMotion * pMotion);

		// Interceptor Functions
		void Deform(const D3DXMATRIX* c_pWorldMatrix);
		void UpdateWorldPose(const D3DXMATRIX* c_pWorldMatrix);

		// Access Data
		BOOL IsModelThing();
		BOOL IsMotionThing();
		uint32_t GetBoneCount();
		BOOL GetBoneName(uint32_t dwIndex, std::string * pstrBoneName);

	protected:
		float m_fLocalTime;

		CGraphicThing * m_pModelThing;
		CGraphicThing * m_pcurMotionThing;

		std::list<CGraphicThing *> m_pMotionBackupList;

		int32_t m_boneCount;
		granny_local_pose *	m_pgrnLocalPose;
};