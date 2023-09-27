#include "StdAfx.h"
#include "ActorInstance.h"
#include "RaceData.h"
#include "FlyHandler.h"
#include <EterGrnLib/Motion.h>
#include <Core/Race/MotionProto.hpp>

UINT CActorInstance::__GetMotionType()
{
	if (!m_pkCurRaceMotionData)
		return MOTION_TYPE_NONE;

	return m_pkCurRaceMotionData->GetType();
}

void CActorInstance::__MotionEventProcess(BOOL isPC)
{
	if (isAttacking())
	{
		uint32_t dwNextFrame = uint32_t(GetAttackingElapsedTime() * g_fGameFPS);
		for (; m_kCurMotNode.dwcurFrame < dwNextFrame; ++m_kCurMotNode.dwcurFrame)
		{
			MotionEventProcess();
			SoundEventProcess(!isPC);
		}
	}
	else
	{
		MotionEventProcess();
		SoundEventProcess(!isPC);

		++m_kCurMotNode.dwcurFrame;
	}
}

void CActorInstance::MotionProcess(BOOL isPC)
{
	__MotionEventProcess(isPC);
	CurrentMotionProcess();
	ReservingMotionProcess();
}

void CActorInstance::HORSE_MotionProcess(BOOL isPC)
{
	__MotionEventProcess(isPC);

	if (MOTION_TYPE_LOOP == m_kCurMotNode.iMotionType)
		if (m_kCurMotNode.dwcurFrame >= m_kCurMotNode.dwFrameCount)
			m_kCurMotNode.dwcurFrame = 0;
}

void CActorInstance::ReservingMotionProcess()
{
	if (m_MotionDeque.empty())
		return;

	TReservingMotionNode & rReservingMotionNode = m_MotionDeque.front();

	float fCurrentTime = GetLocalTime();
	if (rReservingMotionNode.fStartTime > fCurrentTime)
		return;

	uint32_t dwNextMotionIndex = GET_MOTION_INDEX(rReservingMotionNode.dwMotionKey);
	switch (dwNextMotionIndex)
	{
		case MOTION_STAND_UP:
		case MOTION_STAND_UP_BACK:
			if (IsFaint())
			{

				SetEndStopMotion();

				TMotionDeque::iterator itor = m_MotionDeque.begin();
				for (; itor != m_MotionDeque.end(); ++itor)
				{
					TReservingMotionNode & rNode = *itor;
					rNode.fStartTime += 1.0f;
				}
				return;
			}
			break;
	}

	SCurrentMotionNode kPrevMotionNode=m_kCurMotNode;

	EMotionPushType iMotionType=rReservingMotionNode.iMotionType;
	float fSpeedRatio=rReservingMotionNode.fSpeedRatio;
	float fBlendTime=rReservingMotionNode.fBlendTime;

	uint32_t dwMotionKey=rReservingMotionNode.dwMotionKey;

	m_MotionDeque.pop_front();

	uint32_t dwCurrentMotionIndex=GET_MOTION_INDEX(dwMotionKey);
	switch (dwCurrentMotionIndex)
	{
		case MOTION_STAND_UP:
		case MOTION_STAND_UP_BACK:
			if (IsDead())
			{
				m_kCurMotNode=kPrevMotionNode;
				__ClearMotion();

				SetEndStopMotion();
				return;
			}
			break;
	}

	//Tracenf("MOTION %d", GET_MOTION_INDEX(dwMotionKey));

	int32_t iLoopCount;
	if (MOTION_TYPE_ONCE == iMotionType)
		iLoopCount=1;
	else
		iLoopCount=0;

	SSetMotionData kSetMotData;
	kSetMotData.dwMotKey=dwMotionKey;
	kSetMotData.fBlendTime=fBlendTime;
	kSetMotData.fSpeedRatio=fSpeedRatio;
	kSetMotData.iLoopCount=iLoopCount;

	uint32_t dwRealMotionKey = __SetMotion(kSetMotData);

	if (0 == dwRealMotionKey)
		return;

	//float fDurationTime=rReservingMotionNode.fDuration;
	float fDurationTime = GetMotionDuration(dwRealMotionKey) / fSpeedRatio;
	float fStartTime = rReservingMotionNode.fStartTime;
	float fEndTime = fStartTime + fDurationTime;

	if (dwRealMotionKey == 16777473)
	{
		int32_t bp = 0;
		bp++;
	}

	m_kCurMotNode.uSkill = 0;
	m_kCurMotNode.iMotionType = iMotionType;
	m_kCurMotNode.fSpeedRatio = fSpeedRatio;
	m_kCurMotNode.fStartTime = fStartTime;
	m_kCurMotNode.fEndTime = fEndTime;
	m_kCurMotNode.dwMotionKey = dwRealMotionKey;
	m_kCurMotNode.dwcurFrame = 0;
	m_kCurMotNode.dwFrameCount = fDurationTime / (1.0f / g_fGameFPS);
}

void CActorInstance::CurrentMotionProcess()
{
	if (MOTION_TYPE_LOOP == m_kCurMotNode.iMotionType)
		if (m_kCurMotNode.dwcurFrame >= m_kCurMotNode.dwFrameCount)
			m_kCurMotNode.dwcurFrame = 0;

	if (IsDead())
		return;

	if (!m_MotionDeque.empty())
		return;

	float fCurrentTime = GetLocalTime();

	uint32_t dwMotionIndex=GET_MOTION_INDEX(m_kCurMotNode.dwMotionKey);

	bool isLooping=false;

	if (m_pkCurRaceMotionData && m_pkCurRaceMotionData->IsLoopMotion())
	{
		if (m_kCurMotNode.iLoopCount > 1 || m_kCurMotNode.iLoopCount == -1)
		{
			if (fCurrentTime - m_kCurMotNode.fStartTime > m_pkCurRaceMotionData->GetLoopEndTime())
			{
				m_kCurMotNode.dwcurFrame = uint32_t(m_pkCurRaceMotionData->GetLoopStartTime() * g_fGameFPS);
				__SetLocalTime(m_kCurMotNode.fStartTime + m_pkCurRaceMotionData->GetLoopStartTime());
				if (-1 != m_kCurMotNode.iLoopCount)
					--m_kCurMotNode.iLoopCount;

				isLooping=true;
			}
		}
		else if (!m_kQue_kFlyTarget.empty())
		{
			if (!m_kBackupFlyTarget.IsObject())
			{
				m_kBackupFlyTarget = m_kFlyTarget;
			}

			if (fCurrentTime - m_kCurMotNode.fStartTime > m_pkCurRaceMotionData->GetLoopEndTime())
			{
				m_kCurMotNode.dwcurFrame = uint32_t(m_pkCurRaceMotionData->GetLoopStartTime() * g_fGameFPS);
				__SetLocalTime(m_kCurMotNode.fStartTime + m_pkCurRaceMotionData->GetLoopStartTime());

				SetFlyTarget(m_kQue_kFlyTarget.front());
				m_kQue_kFlyTarget.pop_front();

				isLooping=true;
			}
		}
	}

	if (!isLooping)
	{
		if (fCurrentTime > m_kCurMotNode.fEndTime)
		{
			if (m_kBackupFlyTarget.IsValidTarget())
			{
				m_kFlyTarget = m_kBackupFlyTarget;
				m_kBackupFlyTarget.Clear();
			}

			////////////////////////////////////////////

			if (MOTION_TYPE_ONCE == m_kCurMotNode.iMotionType)
			{
				switch (dwMotionIndex)
				{
					case MOTION_DAMAGE_FLYING:
					case MOTION_DAMAGE_FLYING_BACK:
					case MOTION_DEAD:
					case MOTION_INTRO_SELECTED:
					case MOTION_INTRO_NOT_SELECTED:
						m_kCurMotNode.fEndTime+=3.0f;
						SetEndStopMotion();
						break;
					default:
						InterceptLoopMotion(MOTION_WAIT);
						break;
				}
			}
			else if (MOTION_TYPE_LOOP == m_kCurMotNode.iMotionType)
			{
				if (MOTION_WAIT == dwMotionIndex)
				{
					PushLoopMotion(MOTION_WAIT, 0.5f);
				}
			}
		}
	}
}

void CActorInstance::SetMotionMode(int32_t iMotionMode)
{
	if (IsPoly())
		iMotionMode=MOTION_MODE_GENERAL;

	m_wcurMotionMode = iMotionMode;
}

int32_t CActorInstance::GetMotionMode()
{
	return m_wcurMotionMode;
}

void CActorInstance::SetMotionLoopCount(int32_t iCount)
{
	assert(iCount >= -1 && iCount < 100);
	m_kCurMotNode.iLoopCount = iCount;
}

void CActorInstance::PushMotion(EMotionPushType iMotionType, uint32_t dwMotionKey, float fBlendTime, float fSpeedRatio)
{
	if (!CheckMotionThingIndex(dwMotionKey, m_dwMotionID))
	{
		SysLog("Not found motion : {0} {1} {2}", MakeMotionId(dwMotionKey).index, m_dwMotionID, m_eRace);
		return;
	}

	TReservingMotionNode MotionNode;

	MotionNode.iMotionType = iMotionType;
	MotionNode.dwMotionKey = dwMotionKey;
	MotionNode.fStartTime = GetLastMotionTime(fBlendTime);
	MotionNode.fBlendTime = fBlendTime;
	MotionNode.fSpeedRatio = fSpeedRatio;
	MotionNode.fDuration = GetMotionDuration(dwMotionKey);

	m_MotionDeque.push_back(MotionNode);
}

bool CActorInstance::InterceptOnceMotion(uint32_t dwMotion, float fBlendTime, UINT uSkill, float fSpeedRatio)
{
	return InterceptMotion(MOTION_TYPE_ONCE, dwMotion, fBlendTime, uSkill, fSpeedRatio);
}

bool CActorInstance::InterceptLoopMotion(uint32_t dwMotion, float fBlendTime)
{
	return InterceptMotion(MOTION_TYPE_LOOP, dwMotion, fBlendTime);
}

void CActorInstance::SetLoopMotion(uint32_t dwMotion, float fBlendTime, float fSpeedRatio)
{
	if (!m_pkCurRaceData)
	{
		TraceLog("CActorInstance::SetLoopMotion(dwMotion={}, fBlendTime={}, fSpeedRatio={})",
			dwMotion, fBlendTime, fSpeedRatio);
		return;
	}

	MOTION_KEY dwMotionKey;
	if (!m_pkCurRaceData->GetMotionKey(m_wcurMotionMode, dwMotion, &dwMotionKey))
	{
		TraceLog("CActorInstance::SetLoopMotion(dwMotion={}, fBlendTime={}, fSpeedRatio={}) - GetMotionKey(m_wcurMotionMode={}, dwMotion={}, &MotionKey) ERROR",
			dwMotion, fBlendTime, fSpeedRatio, m_wcurMotionMode, dwMotion);
		return;
	}

	__ClearMotion();

	SSetMotionData kSetMotData;
	kSetMotData.dwMotKey=dwMotionKey;
	kSetMotData.fBlendTime=fBlendTime;
	kSetMotData.fSpeedRatio=fSpeedRatio;

	uint32_t dwRealMotionKey = __SetMotion(kSetMotData);

	if (0 == dwRealMotionKey)
		return;

	m_kCurMotNode.iMotionType = MOTION_TYPE_LOOP;
	m_kCurMotNode.fStartTime = GetLocalTime();
	m_kCurMotNode.dwMotionKey = dwRealMotionKey;
	m_kCurMotNode.fEndTime = 0.0f;
	m_kCurMotNode.fSpeedRatio = fSpeedRatio;
	m_kCurMotNode.dwcurFrame = 0;
	m_kCurMotNode.dwFrameCount = GetMotionDuration(dwRealMotionKey) / (1.0f / g_fGameFPS);
	m_kCurMotNode.uSkill = 0;
}

bool CActorInstance::InterceptMotion(EMotionPushType iMotionType, uint16_t wMotion, float fBlendTime, UINT uSkill, float fSpeedRatio)
{
	if (!m_pkCurRaceData)
	{
		TraceLog("CActorInstance::InterceptMotion(iMotionType={}, wMotion={}, fBlendTime={}) - m_pkCurRaceData=NULL", iMotionType, wMotion, fBlendTime);
		return false;
	}

	MOTION_KEY dwMotionKey;
	if (!m_pkCurRaceData->GetMotionKey(m_wcurMotionMode, wMotion, &dwMotionKey))
	{
		TraceLog("CActorInstance::InterceptMotion(iLoopType={}, wMotionMode={}, wMotion={}, fBlendTime={}) - GetMotionKey(m_wcurMotionMode={}, wMotion={}, &MotionKey) ERROR",
			iMotionType, m_wcurMotionMode, wMotion, fBlendTime, m_wcurMotionMode, wMotion);
		return false;
	}

	__ClearMotion();

	int32_t iLoopCount;
	if (MOTION_TYPE_ONCE == iMotionType)
		iLoopCount=1;
	else
		iLoopCount=0;

	SSetMotionData kSetMotData;
	kSetMotData.dwMotKey=dwMotionKey;
	kSetMotData.fBlendTime=fBlendTime;
	kSetMotData.iLoopCount=iLoopCount;
	kSetMotData.fSpeedRatio=fSpeedRatio;
	kSetMotData.uSkill=uSkill;

	uint32_t dwRealMotionKey = __SetMotion(kSetMotData);

	if (0 == dwRealMotionKey)
		return false;

	if (m_pFlyEventHandler)
	{
		if (__IsNeedFlyTargetMotion())
		{
			m_pFlyEventHandler->OnSetFlyTarget();
		}
	}

	assert(NULL != m_pkCurRaceMotionData);

	// float fDuration=GetMotionDuration(dwMotionKey)/fSpeedRatio;
	float fDuration = GetMotionDuration(dwRealMotionKey) / fSpeedRatio;

	m_kCurMotNode.iMotionType = iMotionType;
	m_kCurMotNode.fStartTime = GetLocalTime();
	m_kCurMotNode.fEndTime = m_kCurMotNode.fStartTime + fDuration;
	m_kCurMotNode.dwMotionKey = dwRealMotionKey;
	m_kCurMotNode.dwcurFrame = 0;
	m_kCurMotNode.dwFrameCount = fDuration / (1.0f / g_fGameFPS);
	m_kCurMotNode.uSkill = uSkill;
	m_kCurMotNode.fSpeedRatio = fSpeedRatio;

	return true;
}

bool CActorInstance::PushOnceMotion(uint32_t dwMotion, float fBlendTime, float fSpeedRatio)
{
	assert(m_pkCurRaceData);

	MOTION_KEY MotionKey;
	if (!m_pkCurRaceData->GetMotionKey(m_wcurMotionMode, dwMotion, &MotionKey))
		return false;

	PushMotion(MOTION_TYPE_ONCE, MotionKey, fBlendTime, fSpeedRatio);
	return true;
}

bool CActorInstance::PushLoopMotion(uint32_t dwMotion, float fBlendTime, float fSpeedRatio)
{
	assert(m_pkCurRaceData);

	MOTION_KEY MotionKey;
	if (!m_pkCurRaceData->GetMotionKey(m_wcurMotionMode, dwMotion, &MotionKey))
		return false;

	PushMotion(MOTION_TYPE_LOOP, MotionKey, fBlendTime, fSpeedRatio);
	return true;
}

uint16_t CActorInstance::__GetCurrentMotionIndex()
{
	return GET_MOTION_INDEX(m_kCurMotNode.dwMotionKey);
}

uint32_t CActorInstance::__GetCurrentMotionKey()
{
	return m_kCurMotNode.dwMotionKey;
}

BOOL CActorInstance::IsUsingSkill()
{
	uint32_t dwCurMotionIndex=__GetCurrentMotionIndex();

	if (dwCurMotionIndex>=MOTION_SKILL && dwCurMotionIndex<MOTION_SKILL_END)
		return TRUE;

	switch (dwCurMotionIndex)
	{
		case MOTION_SPECIAL_1:
		case MOTION_SPECIAL_2:
		case MOTION_SPECIAL_3:
		case MOTION_SPECIAL_4:
		case MOTION_SPECIAL_5:
		case MOTION_SPECIAL_6:
			return TRUE;
	}

	return FALSE;
}

BOOL CActorInstance::IsFishing()
{
	if (!m_pkCurRaceMotionData)
		return FALSE;

	if (__GetCurrentMotionIndex() == MOTION_FISHING_WAIT ||
		__GetCurrentMotionIndex() == MOTION_FISHING_REACT)
		return TRUE;

	return FALSE;
}

BOOL CActorInstance::CanCancelSkill()
{
	assert(IsUsingSkill());
	return m_pkCurRaceMotionData->IsCancelEnableSkill();
}

BOOL CActorInstance::isLock()
{
	uint32_t dwCurMotionIndex=__GetCurrentMotionIndex();

	// Locked during attack
	switch (dwCurMotionIndex)
	{
		case MOTION_NORMAL_ATTACK:
		case MOTION_COMBO_ATTACK_1:
		case MOTION_COMBO_ATTACK_2:
		case MOTION_COMBO_ATTACK_3:
		case MOTION_COMBO_ATTACK_4:
		case MOTION_COMBO_ATTACK_5:
		case MOTION_COMBO_ATTACK_6:
		case MOTION_COMBO_ATTACK_7:
		case MOTION_COMBO_ATTACK_8:
		case MOTION_SPECIAL_1:
		case MOTION_SPECIAL_2:
		case MOTION_SPECIAL_3:
		case MOTION_SPECIAL_4:
		case MOTION_SPECIAL_5:
		case MOTION_SPECIAL_6:
		case MOTION_FISHING_THROW:
		case MOTION_FISHING_WAIT:
		case MOTION_FISHING_STOP:
		case MOTION_FISHING_REACT:
		case MOTION_FISHING_CATCH:
		case MOTION_FISHING_FAIL:
		case MOTION_CLAP:
		case MOTION_DANCE_1:
		case MOTION_DANCE_2:
		case MOTION_DANCE_3:
		case MOTION_DANCE_4:
		case MOTION_DANCE_5:
		case MOTION_DANCE_6:
		case MOTION_CONGRATULATION:
		case MOTION_FORGIVE:
		case MOTION_ANGRY:
		case MOTION_ATTRACTIVE:
		case MOTION_SAD:
		case MOTION_SHY:
		case MOTION_CHEERUP:
		case MOTION_BANTER:
		case MOTION_JOY:
		case MOTION_CHEERS_1:
		case MOTION_CHEERS_2:
		case MOTION_KISS_WITH_WARRIOR:
		case MOTION_KISS_WITH_ASSASSIN:
		case MOTION_KISS_WITH_SURA:
		case MOTION_KISS_WITH_SHAMAN:
		case MOTION_FRENCH_KISS_WITH_WARRIOR:
		case MOTION_FRENCH_KISS_WITH_ASSASSIN:
		case MOTION_FRENCH_KISS_WITH_SURA:
		case MOTION_FRENCH_KISS_WITH_SHAMAN:
		case MOTION_SLAP_HIT_WITH_WARRIOR:
		case MOTION_SLAP_HIT_WITH_ASSASSIN:
		case MOTION_SLAP_HIT_WITH_SURA:
		case MOTION_SLAP_HIT_WITH_SHAMAN:
		case MOTION_SLAP_HURT_WITH_WARRIOR:
		case MOTION_SLAP_HURT_WITH_ASSASSIN:
		case MOTION_SLAP_HURT_WITH_SURA:
		case MOTION_SLAP_HURT_WITH_SHAMAN:
			return TRUE;
			break;
	}

	// Locked during using skill
	if (IsUsingSkill())
	{
		if (m_pkCurRaceMotionData->IsCancelEnableSkill())
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

float CActorInstance::GetLastMotionTime(float fBlendTime)
{
	if (m_MotionDeque.empty())
	{
		if (MOTION_TYPE_ONCE == m_kCurMotNode.iMotionType)
			return (m_kCurMotNode.fEndTime - fBlendTime);

		return GetLocalTime();
	}

	TReservingMotionNode & rMotionNode = m_MotionDeque[m_MotionDeque.size()-1];

	return rMotionNode.fStartTime + rMotionNode.fDuration - fBlendTime;
}

float CActorInstance::GetMotionDuration(uint32_t dwMotionKey)
{
	CGraphicThing* pMotion;

	if (!GetMotionThingPointer(m_dwMotionID, dwMotionKey, &pMotion))
	{
		MotionId id = MakeMotionId(dwMotionKey);
		WarnLog("CActorInstance::GetMotionDuration - Cannot get motion: {0} / {1}, {2} race {3}", id.mode, id.index, id.subIndex, m_dwMotionID);
		return 0.0f;
	}

	if (0 == pMotion->GetMotionCount())
	{
		ConsoleLog("CActorInstance::GetMotionDuration - Invalid Motion Key : {}, {}, {}",
				GET_MOTION_MODE(dwMotionKey), GET_MOTION_INDEX(dwMotionKey), GET_MOTION_SUB_INDEX(dwMotionKey));
		return 0.0f;
	}

	std::shared_ptr<CGrannyMotion> pGrannyMotion = pMotion->GetMotionPointer(0);
	return pGrannyMotion->GetDuration();
}

MOTION_KEY CActorInstance::GetRandomMotionKey(MOTION_KEY dwMotionKey)
{
	uint16_t wMode = GET_MOTION_MODE(dwMotionKey);
	uint16_t wIndex = GET_MOTION_INDEX(dwMotionKey);

	const CRaceData::TMotionVector * c_pMotionVector;
	if (m_pkCurRaceData->GetMotionVectorPointer(wMode, wIndex, &c_pMotionVector))
	if (c_pMotionVector->size() > 1)
	{
		int32_t iPercentage = random() % 100;
		for (uint32_t i = 0; i < c_pMotionVector->size(); ++i)
		{
			const CRaceData::TMotion & c_rMotion = c_pMotionVector->at(i);
			iPercentage -= c_rMotion.byPercentage;

			if (iPercentage < 0)
			{
				dwMotionKey = MAKE_RANDOM_MOTION_KEY(wMode, wIndex, i);

				// Temporary
				// m_kCurMotNode.fEndTime = m_kCurMotNode.fStartTime + GetMotionDuration(dwMotionKey);
				// Temporary

				return dwMotionKey;
			}
		}
	}

	return dwMotionKey;
}

void CActorInstance::PreAttack()
{
}

void CActorInstance::__ClearMotion()
{
	__HideWeaponTrace();

	m_MotionDeque.clear();
	m_kCurMotNode.dwcurFrame=0;
	m_kCurMotNode.dwFrameCount=0;
	m_kCurMotNode.uSkill=0;
	m_kCurMotNode.iLoopCount=0;
	m_kCurMotNode.iMotionType=MOTION_TYPE_NONE;
}


uint32_t CActorInstance::__SetMotion(const SSetMotionData& c_rkSetMotData, uint32_t dwRandMotKey)
{
	uint32_t dwMotKey = dwRandMotKey;

	if (dwMotKey == 0)
		dwMotKey = GetRandomMotionKey(c_rkSetMotData.dwMotKey);

	UINT uNextMot = GET_MOTION_INDEX(c_rkSetMotData.dwMotKey);

	if (IsDead())
	{
		if (uNextMot!=MOTION_DAMAGE_FLYING && uNextMot!=MOTION_DAMAGE_FLYING_BACK && uNextMot!=MOTION_DEAD && uNextMot!=MOTION_DEAD_BACK)
			return 0;
	}
	if (IsUsingSkill())
	{
		__OnStop();
	}

	if (__IsStandUpMotion())
	{
		__OnStop();
	}


	if (__IsMoveMotion())
	{
		if (uNextMot==MOTION_DAMAGE || uNextMot==MOTION_DAMAGE_BACK || uNextMot==MOTION_DAMAGE_FLYING || uNextMot==MOTION_DAMAGE_FLYING_BACK)
		{
			if (!m_isMain)
			{
				SysLog("Only MainActor can receive damage when moving");
				return false;
			}
		}

		if (uNextMot!=MOTION_RUN &&
			uNextMot!=MOTION_WALK &&
			!__IsMovingSkill(c_rkSetMotData.uSkill))
		{
			__OnStop();
		}
	}
	else
	{
		if (uNextMot==MOTION_RUN || __IsMovingSkill(c_rkSetMotData.uSkill))
		{
			__OnMove();
		}
	}


	if (__IsHiding())
	{
		__ShowEvent();
	}


	if (-1 != m_iFishingEffectID)
	{
		auto rkEftMgr=CEffectManager::GetInstance();
 		rkEftMgr->DeactiveEffectInstance(m_iFishingEffectID);

		m_iFishingEffectID = -1;
	}

	if (m_pkHorse)
	{
		uint16_t wMotionIndex = GET_MOTION_INDEX(dwMotKey);
		uint16_t wMotionSubIndex = GET_MOTION_SUB_INDEX(dwMotKey);
		uint32_t dwChildMotKey = MAKE_RANDOM_MOTION_KEY(m_pkHorse->m_wcurMotionMode, wMotionIndex, wMotionSubIndex);

		if (MOTION_DEAD == wMotionIndex)
			CGraphicThingInstance::ChangeMotion(dwMotKey, c_rkSetMotData.iLoopCount, c_rkSetMotData.fSpeedRatio);
		else
			CGraphicThingInstance::SetMotion(dwMotKey, c_rkSetMotData.fBlendTime, c_rkSetMotData.iLoopCount, c_rkSetMotData.fSpeedRatio);

		m_pkHorse->SetMotion(dwChildMotKey, c_rkSetMotData.fBlendTime, c_rkSetMotData.iLoopCount, c_rkSetMotData.fSpeedRatio);
		m_pkHorse->__BindMotionData(dwChildMotKey);

		if (c_rkSetMotData.iLoopCount)
			m_pkHorse->m_kCurMotNode.iMotionType = MOTION_TYPE_ONCE;
		else
			m_pkHorse->m_kCurMotNode.iMotionType = MOTION_TYPE_LOOP;

		m_pkHorse->m_kCurMotNode.dwFrameCount	= m_pkHorse->GetMotionDuration(dwChildMotKey) / (1.0f / g_fGameFPS);
		m_pkHorse->m_kCurMotNode.dwcurFrame		= 0;
		m_pkHorse->m_kCurMotNode.dwMotionKey	= dwChildMotKey;
	}
	else
	{
		CGraphicThingInstance::SetMotion(dwMotKey, c_rkSetMotData.fBlendTime, c_rkSetMotData.iLoopCount, c_rkSetMotData.fSpeedRatio);
	}

	__HideWeaponTrace();

	if (__BindMotionData(dwMotKey))
	{
		int32_t iLoopCount = __GetLoopCount();
		SetMotionLoopCount(iLoopCount);

		if (__CanAttack())
		{
			__ShowWeaponTrace();

			m_HitDataMap.clear();
			//PreAttack();
		}

		if (__IsComboAttacking())
		{
			if (!__CanNextComboAttack())
			{
				m_dwcurComboIndex = 0;

			}
		}
	}

	return dwMotKey;
}

bool CActorInstance::__BindMotionData(uint32_t dwMotionKey)
{
	if (!m_pkCurRaceData->GetMotionDataPointer(dwMotionKey, &m_pkCurRaceMotionData))
	{
		TraceLog("Failed to bind motion.");
		m_pkCurRaceMotionData=NULL;
		m_dwcurComboIndex=0;
		return false;
	}

	return true;
}

int32_t CActorInstance::__GetLoopCount()
{
	if (!m_pkCurRaceMotionData)
	{
		TraceLog("CActorInstance::__GetLoopCount() - m_pkCurRaceMotionData==NULL");
		return -1;
	}

	return m_pkCurRaceMotionData->GetLoopCount();
}

bool CActorInstance::__CanAttack()
{
	if (!m_pkCurRaceMotionData)
	{
		TraceLog("CActorInstance::__CanAttack() - m_pkCurRaceMotionData==NULL");
		return false;
	}

	if (!m_pkCurRaceMotionData->isAttackingMotion())
		return false;

	return true;
}

bool CActorInstance::__CanNextComboAttack()
{
	if (!m_pkCurRaceMotionData)
	{
		TraceLog("CActorInstance::__CanNextComboAttack() - m_pkCurRaceMotionData==NULL");
		return false;
	}

	if (!m_pkCurRaceMotionData->IsComboInputTimeData())
		return false;

	return true;
}

bool CActorInstance::__IsComboAttacking()
{
	if (0 == m_dwcurComboIndex)
		return false;

	return true;
}

bool CActorInstance::__IsNeedFlyTargetMotion()
{
	if (!m_pkCurRaceMotionData)
		return true;

	for (uint32_t i = 0; i < m_pkCurRaceMotionData->GetMotionEventDataCount(); ++i)
	{
		const CRaceMotionData::TMotionEventData * c_pData;
		if (!m_pkCurRaceMotionData->GetMotionEventDataPointer(i, &c_pData))
			continue;

		if (c_pData->iType == MOTION_EVENT_TYPE_WARP)
			return true;

		if (c_pData->iType == MOTION_EVENT_TYPE_FLY)
			return true;

		if (c_pData->iType == MOTION_EVENT_TYPE_EFFECT_TO_TARGET)
			return true;
	}

	return false;
}

bool CActorInstance::__HasMotionFlyEvent()
{
	if (!m_pkCurRaceMotionData)
		return true;

	for (uint32_t i = 0; i < m_pkCurRaceMotionData->GetMotionEventDataCount(); ++i)
	{
		const CRaceMotionData::TMotionEventData * c_pData;
		if (!m_pkCurRaceMotionData->GetMotionEventDataPointer(i, &c_pData))
			continue;

		if (c_pData->iType == MOTION_EVENT_TYPE_FLY)
			return true;
	}
	return false;
}

bool CActorInstance::__IsWaitMotion()
{
	return (__GetMotionType()==MOTION_TYPE_WAIT);
}

bool CActorInstance::__IsMoveMotion()
{
	return (__GetMotionType()==MOTION_TYPE_MOVE);
}

bool CActorInstance::__IsAttackMotion()
{
	return (__GetMotionType()==MOTION_TYPE_ATTACK);
}

bool CActorInstance::__IsComboAttackMotion()
{
	return (__GetMotionType()==MOTION_TYPE_COMBO);
}


bool CActorInstance::__IsDamageMotion()
{
	return (__GetMotionType()==MOTION_TYPE_DAMAGE);
}

bool CActorInstance::__IsKnockDownMotion()
{
	return (__GetMotionType()==MOTION_TYPE_KNOCKDOWN);
}

bool CActorInstance::__IsEmotionMotion()
{
	return (__GetMotionType() == MOTION_TYPE_EMOTION);
}

bool CActorInstance::__IsDieMotion()
{
	if (__IsKnockDownMotion())
		return true;

	return (__GetMotionType()==MOTION_TYPE_DIE);
}

bool CActorInstance::__IsStandUpMotion()
{
	return (__GetMotionType()==MOTION_TYPE_STANDUP);
}
