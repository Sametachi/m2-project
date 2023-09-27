#include "StdAfx.h"
#include "PythonPlayerEventHandler.h"
#include "PythonPlayer.h"
#include "PythonCharacterManager.h"
#include "PythonNetworkStream.h"
#include <GameLib/RaceManager.h>

CPythonPlayerEventHandler& CPythonPlayerEventHandler::GetSingleton()
{
	static CPythonPlayerEventHandler s_kPlayerEventHandler;
	return s_kPlayerEventHandler;
}

CPythonPlayerEventHandler::~CPythonPlayerEventHandler()
{
}

void CPythonPlayerEventHandler::OnClearAffects()
{
	CPythonPlayer::GetInstance()->ClearAffects();
}

void CPythonPlayerEventHandler::OnSetAffect(UINT uAffect)
{
	CPythonPlayer::GetInstance()->SetAffect(uAffect);
}

void CPythonPlayerEventHandler::OnResetAffect(UINT uAffect)
{
	CPythonPlayer::GetInstance()->ResetAffect(uAffect);
}

void CPythonPlayerEventHandler::OnSyncing(const SState& c_rkState)
{
	const TPixelPosition& c_rkPPosCurSyncing=c_rkState.kPPosSelf;
	m_kPPosPrevWaiting=c_rkPPosCurSyncing;
}

void CPythonPlayerEventHandler::OnWaiting(const SState& c_rkState)
{
	uint32_t dwCurTime=ELTimer_GetMSec();
	if (m_dwNextWaitingNotifyTime>dwCurTime)
		return;

	m_dwNextWaitingNotifyTime=dwCurTime+100;

	const TPixelPosition& c_rkPPosCurWaiting=c_rkState.kPPosSelf;
	float dx=m_kPPosPrevWaiting.x-c_rkPPosCurWaiting.x;
	float dy=m_kPPosPrevWaiting.y-c_rkPPosCurWaiting.y;
	float len=sqrt(dx*dx+dy*dy);

	if (len<1.0f)
		return;

	m_kPPosPrevWaiting=c_rkPPosCurWaiting;

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_WAIT, 0);

	//Trace("waiting\n");
}

void CPythonPlayerEventHandler::OnMoving(const SState& c_rkState)
{
	uint32_t dwCurTime=ELTimer_GetMSec();		
	if (m_dwNextMovingNotifyTime>dwCurTime)
		return;
	
	m_dwNextMovingNotifyTime=dwCurTime+300;
	
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_MOVE, 0);

//	Trace("moving\n");
}

void CPythonPlayerEventHandler::OnMove(const SState& c_rkState)
{
	uint32_t dwCurTime=ELTimer_GetMSec();
	m_dwNextWaitingNotifyTime=dwCurTime+100;
	m_dwNextMovingNotifyTime=dwCurTime+300;

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_MOVE, 0);

//	Trace("move\n");
}

void CPythonPlayerEventHandler::OnStop(const SState& c_rkState)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_WAIT, 0);

//	Trace("stop\n");
}

void CPythonPlayerEventHandler::OnWarp(const SState& c_rkState)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_WAIT, 0);
}

void CPythonPlayerEventHandler::OnAttack(const SState& c_rkState, uint16_t wMotionIndex)
{
//	Tracef("CPythonPlayerEventHandler::OnAttack [%d]\n", wMotionIndex);
	assert(wMotionIndex < 255);

	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_COMBO, wMotionIndex);

}

void CPythonPlayerEventHandler::OnUseSkill(const SState& c_rkState, UINT uMotSkill, UINT uArg)
{
	auto rkNetStream=CPythonNetworkStream::GetInstance();
	rkNetStream->SendCharacterStatePacket(c_rkState.kPPosSelf, c_rkState.fAdvRotSelf, CInstanceBase::FUNC_SKILL|uMotSkill, uArg);
}

void CPythonPlayerEventHandler::OnUpdate()
{
}

void CPythonPlayerEventHandler::OnChangeShape()
{
	CPythonPlayer::GetInstance()->NEW_Stop();
}

void CPythonPlayerEventHandler::OnHit(UINT uSkill, CActorInstance& rkActorVictim, BOOL isSendPacket)
{
	uint32_t dwVIDVictim=rkActorVictim.GetVirtualID();

	// Update Target
	CPythonPlayer::GetInstance()->SetTarget(dwVIDVictim, FALSE);
	// Update Target

	if (isSendPacket)
	{

		auto rkStream=CPythonNetworkStream::GetInstance();
		rkStream->SendAttackPacket(uSkill, dwVIDVictim);
	}

	if (!rkActorVictim.IsPushing())
		return;

	if (CRaceManager::GetInstance()->IsHugeRace(rkActorVictim.GetRace()))
		return;

	CPythonCharacterManager::GetInstance()->AdjustCollisionWithOtherObjects(&rkActorVictim);

	const TPixelPosition& kPPosLast=rkActorVictim.NEW_GetLastPixelPositionRef();

	SVictim kVictim;
	kVictim.m_dwVID=dwVIDVictim;
	kVictim.m_lPixelX=int32_t(kPPosLast.x);
	kVictim.m_lPixelY=int32_t(kPPosLast.y);

	rkActorVictim.TEMP_Push(kVictim.m_lPixelX, kVictim.m_lPixelY);

	m_kVctkVictim.push_back(kVictim);
}

void CPythonPlayerEventHandler::FlushVictimList()
{
	if (m_kVctkVictim.empty())
		return;

	// #0000682: [M2EU] 대진각 스킬 사용시 튕김 
	uint32_t SYNC_POSITION_COUNT_LIMIT = 16;
	uint32_t uiVictimCount = m_kVctkVictim.size();

	auto rkStream=CPythonNetworkStream::GetInstance();

	TPacketCGSyncPosition kPacketSyncPos;
	kPacketSyncPos.bHeader=HEADER_CG_SYNC_POSITION;
	kPacketSyncPos.wSize=sizeof(kPacketSyncPos)+sizeof(TPacketCGSyncPositionElement) * uiVictimCount;

	rkStream->Send(sizeof(kPacketSyncPos), &kPacketSyncPos);

	for (uint32_t i = 0; i < uiVictimCount; ++i)
	{
		const SVictim& rkVictim =  m_kVctkVictim[i];
		rkStream->SendSyncPositionElementPacket(rkVictim.m_dwVID, rkVictim.m_lPixelX, rkVictim.m_lPixelY);
	}

	m_kVctkVictim.clear();
}

CPythonPlayerEventHandler::CPythonPlayerEventHandler()
{
	m_dwPrevComboIndex=0;
	m_dwNextMovingNotifyTime=0;
	m_dwNextWaitingNotifyTime=0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::OnSetFlyTarget()
{
	SState s;
	m_pInstMain->NEW_GetPixelPosition(&s.kPPosSelf);
	s.fAdvRotSelf=m_pInstMain->GetGraphicThingInstancePtr()->GetTargetRotation();

	auto rpns=CPythonNetworkStream::GetInstance();
	rpns->SendFlyTargetingPacket(m_pInstTarget->GetVirtualID(), m_pInstTarget->GetGraphicThingInstancePtr()->OnGetFlyTargetPosition());
}
void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::OnShoot(uint32_t dwSkillIndex)
{
	auto rpns=CPythonNetworkStream::GetInstance();
	rpns->SendShootPacket(dwSkillIndex);
}

void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::Set(CPythonPlayerEventHandler * pParent, CInstanceBase * pInstMain, CInstanceBase * pInstTarget)
{
	m_pParent=(pParent);
	m_pInstMain=(pInstMain);
	m_pInstTarget=(pInstTarget);
}

void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::SetTarget(CInstanceBase* pInstTarget)
{
	m_pInstTarget = pInstTarget;
}

void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::OnExplodingAtAnotherTarget(uint32_t dwSkillIndex, uint32_t dwVID)
{
	return;

	TraceLog("Shoot : 다른 target에 맞았습니다 : {}, {}\n", dwSkillIndex, dwVID);

	auto rkStream=CPythonNetworkStream::GetInstance();
	rkStream->SendAttackPacket(dwSkillIndex, dwVID);

	auto rkChrMgr = CPythonCharacterManager::GetInstance();
	CInstanceBase * pInstance = rkChrMgr->GetInstancePtr(dwVID);
	if (pInstance)
	{
		pInstance->GetGraphicThingInstanceRef().OnShootDamage();
	}
}
void CPythonPlayerEventHandler::CNormalBowAttack_FlyEventHandler_AutoClear::OnExplodingAtTarget(uint32_t dwSkillIndex)
{
//	Tracef("Shoot : 원하는 target에 맞았습니다 : %d, %d\n", dwSkillIndex, m_pInstTarget->GetVirtualID());
//	CPythonNetworkStream& rkStream=CPythonNetworkStream::Instance();
//	rkStream.SendAttackPacket(dwSkillIndex, m_pInstTarget->GetVirtualID());
}
