#pragma once
#include "../../../Libraries/gamelib/RaceMotionData.h"

class CRaceMotionDataAccessor : public CRaceMotionData
{
	public:
		CRaceMotionDataAccessor();
		~CRaceMotionDataAccessor();

		void ClearAccessor();

		void SetMotionFileName(const char * c_szFileName);

		void SetLoopCount(int32_t iLoopCount);

		// Duration
		float GetMotionDuration();
		void SetMotionDuration(float fDuration);

		// Motion Event
		uint32_t MakeEvent(int32_t iEventType);
		uint32_t MakeEventScreenWaving();
		uint32_t MakeEventScreenFlashing();
		uint32_t MakeEventFly();
		uint32_t MakeEventEffect();
		uint32_t MakeEventAttacking();
		uint32_t MakeEventSound();
		uint32_t MakeEventCharacterShow();
		uint32_t MakeEventCharacterHide();
		uint32_t MakeEventWarp();
		uint32_t MakeEventEffectToTarget();
		BOOL GetMotionEventDataPointer(uint32_t dwIndex, TMotionEventData ** ppData);

		void MovePosition(uint32_t dwIndex, float fTime);
		void DeleteEvent(uint32_t dwIndex);

		// Combo Data
		void SetComboInputStartTime(float fTime);
		void SetNextComboTime(float fTime);
		void SetComboInputEndTime(float fTime);

		// Attacking Data
		void SetComboMotionEnable(BOOL bFlag);
		void SetAttackingMotionEnable(BOOL bFlag);

		void SetMotionType(int32_t iType);
		void SetAttackType(int32_t iType);
		void SetHittingType(int32_t iType);
		void SetStiffenTime(float fTime);
		void SetInvisibleTime(float fTime);
		void SetExternalForceTime(float fForce);

		// Hit Data
		void SetAttackStartTime(uint32_t dwIndex, float fTime);
		void SetAttackEndTime(uint32_t dwIndex, float fTime);

		void SetAttackingWeaponLength(uint32_t dwIndex, float fWeaponLength);
		void SetAttackingBoneName(uint32_t dwIndex, const std::string & strBoneName);

		void RefreshTimeHitPosition();

		uint32_t GetHitDataCount() const;
		const NRaceData::THitData * GetHitDataPtr(uint32_t dwIndex) const;

		void InsertHitData();
		void DeleteHitData(uint32_t dwIndex);

		void SetHitLimitCount(int32_t iCount);

		// Movement
		void SetMovementEnable(BOOL bFlag);
		void SetMovementDistance(const D3DXVECTOR3 & c_rv3MovementDistance);

		// Loop
		void SetLoopMotionEnable(BOOL bFlag);
		void SetLoopStartTime(float fTime);
		void SetLoopEndTime(float fTime);

		// Cancel
		void SetCancelEnable(BOOL bEnable);

	protected:
		void UpdateActorInstanceAccessorMotion();
};