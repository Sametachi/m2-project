#pragma once

#include "../../../Libraries/EffectLib/EffectElementBase.h"
#include "../../../Libraries/EffectLib/ParticleSystemData.h"
#include "../../../Libraries/EffectLib/EffectMesh.h"
#include "../../../Libraries/EffectLib/SimpleLightData.h"
#include "../../../Libraries/EffectLib/EffectData.h"
#include "../../../Libraries/EffectLib/EffectInstance.h"

// Data
class CEffectElementBaseAccessor : public CEffectElementBase
{
	public:
		CEffectElementBaseAccessor(){}
		virtual ~CEffectElementBaseAccessor(){}

		void SaveScript(int32_t iBaseTab, FILE * File);

		void SetStartTime(float fTime);

		uint32_t GetPositionCount();
		void DeletePosition(uint32_t dwIndex);
		void InsertPosition(float fTime);
		bool GetTimePosition(uint32_t dwIndex, float * pTime);
		void SetTimePosition(uint32_t dwIndex, float fTime);
		bool GetValuePosition(uint32_t dwIndex, TEffectPosition ** ppEffectPosition);
		void SetValuePosition(uint32_t dwIndex, const D3DXVECTOR3 & c_rVector);
		void SetValueControlPoint(uint32_t dwIndex, const D3DXVECTOR3 & c_rVector);
};
class CParticleAccessor : public CParticleSystemData
{
	public:
		CParticleAccessor();
		virtual ~CParticleAccessor();

		void ClearAccessor();
		void SetDefaultData();
		void SaveScript(int32_t iBaseTab, FILE * File, const char * c_szGlobalPath);

		void InsertTexture(const char * c_szFileName);
		void ClearOneTexture(uint32_t dwIndex);
		void ClearAllTexture();

		// Emitter Property
		uint32_t & GetMaxEmissionCountReference();

		float & GetCycleLengthReference();
		BOOL & GetCycleLoopFlagReference();
		int32_t & GetLoopCountReference();

		uint8_t & GetEmitterShapeReference();
		uint8_t & GetEmitterAdvancedTypeReference();
		BOOL & GetEmitFromEdgeFlagReference();
		D3DXVECTOR3 & GetEmittingSizeReference();
		float & GetEmittingRadiusReference();

		D3DXVECTOR3 & GetEmittingDirectionReference();

		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingSize();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingAngularVelocity();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingDirectionX();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingDirectionY();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingDirectionZ();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmittingVelocity();
		TTimeEventTableFloat * GetEmitterTimeEventTableEmissionCount();
		TTimeEventTableFloat * GetEmitterTimeEventTableLifeTime();
		TTimeEventTableFloat * GetEmitterTimeEventTableSizeX();
		TTimeEventTableFloat * GetEmitterTimeEventTableSizeY();

		// Particle Property
		uint8_t & GetSrcBlendTypeReference();
		uint8_t & GetDestBlendTypeReference();
		uint8_t & GetColorOperationTypeReference();
		uint8_t & GetBillboardTypeReference();

		uint8_t & GetTexAniTypeReference();
		BOOL & GetTexAniRandomStartFrameFlagReference();
		float & GetTexAniDelayReference();

		BOOL & GetStretchFlagReference();
		BOOL & GetAttachFlagReference();

		uint8_t & GetRotationTypeReference();
		float & GetRotationSpeedReference();
		uint16_t & GetRotationRandomStartBeginReference();
		uint16_t & GetRotationRandomStartEndReference();

		TTimeEventTableFloat * GetParticleTimeEventGravity();
		TTimeEventTableFloat * GetParticleTimeEventAirResistance();

		TTimeEventTableFloat * GetParticleTimeEventScaleX();
		TTimeEventTableFloat * GetParticleTimeEventScaleY();
		TTimeEventTableFloat * GetParticleTimeEventColorRed();
		TTimeEventTableFloat * GetParticleTimeEventColorGreen();
		TTimeEventTableFloat * GetParticleTimeEventColorBlue();
		TTimeEventTableFloat * GetParticleTimeEventAlpha();
		TTimeEventTableFloat * GetParticleTimeEventRotation();
		TTimeEventTableFloat * GetParticleTimeEventStretch();

		uint32_t GetTextureCount();
		BOOL GetImagePointer(uint32_t dwIndex, CGraphicImage ** ppImage);
		BOOL GetTextureName(uint32_t dwIndex, const char ** pszTextureName);
};
class CMeshAccessor : public CEffectMeshScript
{
	public:
		CMeshAccessor();
		virtual ~CMeshAccessor();

		void ClearAccessor();
		void SaveScript(int32_t iBaseTab, FILE * File, const char * c_szGlobalPath);

		void SetMeshAnimationFlag(bool bFlag);
		void SetMeshAnimationLoopCount(int32_t iNewCount);
		void SetMeshAnimationFrameDelay(float fDelay);
		void SetMeshFileName(const char * c_szFileName);

		void LoadMeshInstance();
		uint32_t GetMeshElementCount();
		BOOL GetMeshElementDataPointer(uint32_t dwIndex, CEffectMesh::TEffectMeshData ** ppMeshElementData);

	protected:
		CEffectMesh * m_pEffectMesh;
};

class CLightAccessor : public CLightData
{
	public:
		CLightAccessor();
		virtual ~CLightAccessor();

		void ClearAccessor();
		void SaveScript(int32_t iBaseTab, FILE * File);

		BOOL & GetLoopFlagReference();
		int32_t & GetLoopCountReference(){
			return m_iLoopCount;
		}

		float & GetMaxRangeReference();
		float & GetDurationReference();

		float & GetAttenuation0Reference();
		float & GetAttenuation1Reference();
		float & GetAttenuation2Reference();

		TTimeEventTableFloat * GetEmitterTimeEventTableRange();

		D3DXCOLOR & GetAmbientColorReference();
		D3DXCOLOR & GetDiffuseColorReference();
		void SetAmbientColor(float fr, float fg, float fb);
		void SetDiffuseColor(float fr, float fg, float fb);
};

class CEffectAccessor : public CEffectData
{
	public:
		enum
		{
			EFFECT_ELEMENT_TYPE_PARTICLE,
			EFFECT_ELEMENT_TYPE_MESH,
			EFFECT_ELEMENT_TYPE_LIGHT,
		};

		typedef struct SEffectElement
		{
			BOOL bVisible;

			int32_t iType;
			const char* strName{ "UNK" }; // @fixme126

			CEffectElementBaseAccessor * pBase;

			CParticleAccessor * pParticle;
			CMeshAccessor * pMesh;
			CLightAccessor * pLight;
			NSound::TSoundData * pSound;
		} TEffectElement;

		typedef std::vector<TEffectElement> TEffectElementVector;

	public:
		CEffectAccessor() : m_fLifeTime(5.0f){}
		virtual ~CEffectAccessor();

		void					Clear();

		CParticleSystemData *	AllocParticle();
		CEffectMeshScript *		AllocMesh();
		CLightData *			AllocLight();

		void					SetVisible(uint32_t dwIndex, BOOL bVisible);
		BOOL					GetVisible(uint32_t dwIndex);

		float					GetLifeTime();
		void					SetLifeTime(float fLifeTime);
		void					SetBoundingSphereRadius(float fRadius);
		void					SetBoundingSpherePosition(const D3DXVECTOR3 & c_rv3Pos);
		uint32_t					GetElementCount();
		BOOL					GetElement(uint32_t dwIndex, TEffectElement ** ppElement);
		BOOL					GetElementParticle(uint32_t dwIndex, CParticleAccessor ** ppParticleAccessor);
		void					DeleteElement(uint32_t dwIndex);
		void					SwapElement(uint32_t dwIndex1, uint32_t dwIndex2);

	protected:
		float					m_fLifeTime;
		TEffectElementVector	m_ElementVector;
};

class CEffectInstanceAccessor : public CEffectInstance
{
	public:
		CEffectInstanceAccessor(){}
		virtual ~CEffectInstanceAccessor(){}

		void SetEffectDataAccessorPointer(CEffectAccessor * pAccessor);
};