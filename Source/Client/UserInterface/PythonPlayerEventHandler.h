#pragma once

#include "../../Libraries/gameLib/ActorInstance.h"
#include "../../Libraries/gameLib/FlyHandler.h"

#include "PythonNetworkStream.h"
#include "InstanceBase.h"

class CPythonPlayerEventHandler : public CActorInstance::IEventHandler
{
	public:
		static CPythonPlayerEventHandler& GetSingleton();

	public:
		virtual ~CPythonPlayerEventHandler();

		virtual void OnSyncing(const SState& c_rkState);
		virtual void OnWaiting(const SState& c_rkState);
		virtual void OnMoving(const SState& c_rkState);
		virtual void OnMove(const SState& c_rkState);
		virtual void OnStop(const SState& c_rkState);
		virtual void OnWarp(const SState& c_rkState);
		virtual void OnClearAffects();
		virtual void OnSetAffect(UINT uAffect);
		virtual void OnResetAffect(UINT uAffect);
		virtual void OnAttack(const SState& c_rkState, uint16_t wMotionIndex);
		virtual void OnUseSkill(const SState& c_rkState, UINT uMotSkill, UINT uArg);
		virtual void OnUpdate();
		virtual void OnChangeShape();
		virtual void OnHit(UINT uSkill, CActorInstance& rkActorVictim, BOOL isSendPacket);

		void FlushVictimList();

	protected:
		CPythonPlayerEventHandler();

	protected:
		struct SVictim
		{
			uint32_t	m_dwVID;
			int32_t	m_lPixelX;
			int32_t	m_lPixelY;
		};

	protected:
		std::vector<SVictim> m_kVctkVictim;

		uint32_t m_dwPrevComboIndex;
		uint32_t m_dwNextWaitingNotifyTime;
		uint32_t m_dwNextMovingNotifyTime;
		TPixelPosition m_kPPosPrevWaiting;

	private:
		class CNormalBowAttack_FlyEventHandler_AutoClear : public IFlyEventHandler
		{
			public:
				CNormalBowAttack_FlyEventHandler_AutoClear() {}
				virtual ~CNormalBowAttack_FlyEventHandler_AutoClear() {}

				void Set(CPythonPlayerEventHandler * pParent, CInstanceBase * pInstMain, CInstanceBase * pInstTarget);
				void SetTarget(CInstanceBase* pInstTarget);

				virtual void OnSetFlyTarget();
				virtual void OnShoot(uint32_t dwSkillIndex);

				virtual void OnNoTarget() { /*Tracenf("Shoot : target�� �����ϴ�.");*/ }
				virtual void OnExplodingOutOfRange() { /*Tracenf("Shoot : �����Ÿ��� �������ϴ�.");*/ }
				virtual void OnExplodingAtBackground() { /*Tracenf("Shoot : ��濡 �¾ҽ��ϴ�.");*/ }
				virtual void OnExplodingAtAnotherTarget(uint32_t dwSkillIndex, uint32_t dwVID);
				virtual void OnExplodingAtTarget(uint32_t dwSkillIndex);

			protected:
				CPythonPlayerEventHandler * m_pParent;
				CInstanceBase * m_pInstMain;
				CInstanceBase * m_pInstTarget;
		} m_NormalBowAttack_FlyEventHandler_AutoClear;

		public:
			IFlyEventHandler * GetNormalBowAttackFlyEventHandler(CInstanceBase* pInstMain, CInstanceBase* pInstTarget)
			{
				m_NormalBowAttack_FlyEventHandler_AutoClear.Set(this,pInstMain,pInstTarget);
				return &m_NormalBowAttack_FlyEventHandler_AutoClear;
			}

			void ChangeFlyTarget(CInstanceBase* pInstTarget)
			{
				m_NormalBowAttack_FlyEventHandler_AutoClear.SetTarget(pInstTarget);
			}
};