#pragma once

#include "../../../Libraries/gamelib/ActorInstance.h"

class CActorInstanceAccessor : public CActorInstance
{
	public:
		CActorInstanceAccessor();
		virtual ~CActorInstanceAccessor();

		void ClearModel();
		void ClearMotion();
		void ClearAttachingEffect();

		BOOL IsModelThing();
		BOOL IsMotionThing();

		BOOL SetAccessorModel(CGraphicThing * pThing);
		BOOL SetAccessorMotion(CGraphicThing * pThing);

		void SetMotion();
		void SetMotionData(CRaceMotionData * pMotionData);
		float GetMotionDuration();

		uint32_t GetBoneCount();
		void SetLocalTime(float fLocalTime);
		BOOL GetBoneName(uint32_t dwIndex, std::string * pstrBoneName);

		BOOL GetBoneMatrix(uint32_t dwBoneIndex, D3DXMATRIX ** ppMatrix);
		BOOL GetBoneIndexByName(const char * c_szBoneName, int32_t * pBoneIndex) const;

		void ClearAttachingObject();
		void AttachObject(const char * c_szFileName, const char * c_szBoneName);
		void UpdateAttachingObject();
		void RenderAttachingObject();

	protected:
		CGraphicThing * m_pModelThing;
		CGraphicThing * m_pMotionThing;

		struct SAttachingModelInstance
		{
			std::string strBoneName;
			CGraphicThing * pThing;
			CGrannyModelInstance * pModelInstance;
		};
		std::list<SAttachingModelInstance> m_AttachingObjectList;
};