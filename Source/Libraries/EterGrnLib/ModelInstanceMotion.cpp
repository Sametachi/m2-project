#include "StdAfx.h"
#include "ModelInstance.h"
#include "Model.h"
#include "Granny3D.h"

bool CGrannyModelInstance::IsMotionPlaying()
{
	if (!m_pgrnCtrl)
		return false;

	if (GrannyControlIsComplete(m_pgrnCtrl))
		return false;

	return true;
}

// This is perfect the way it is finally...
void CGrannyModelInstance::SetMotionPointer(const std::shared_ptr<CGrannyMotion> pMotion, float blendTime, int32_t loopCount, float speedRatio)
{
	if (!m_worldPose || !m_modelInstance)
		return;

	granny_model_instance* pgrnModelInstance = m_modelInstance;
	if (!pgrnModelInstance)
		return;

	float localTime = GetLocalTime();

	bool isFirst = false;
	if (m_pgrnCtrl)
	{
		GrannySetControlEaseOutCurve(m_pgrnCtrl, localTime, localTime + blendTime, 1.0f, 1.0f, 0.0f, 0.0f);
		GrannySetControlEaseIn(m_pgrnCtrl, false);
		GrannySetControlEaseOut(m_pgrnCtrl, true);
		GrannyCompleteControlAt(m_pgrnCtrl, localTime + blendTime);
		GrannyFreeControlIfComplete(m_pgrnCtrl);
	}
	else
	{
		isFirst = true;
	}

	m_pgrnAni = pMotion->GetGrannyAnimationPointer();
	granny_model_instance* InstanceOfModel = pgrnModelInstance;
	granny_animation* Animation = m_pgrnAni;
	granny_real32 StartTime = localTime;

	granny_controlled_animation_builder* Builder = GrannyBeginControlledAnimation(StartTime, Animation);
	if (Builder)
	{
		granny_int32x TrackGroupIndex;
		if (GrannyFindTrackGroupForModel(Animation, GrannyGetSourceModel(InstanceOfModel)->Name, &TrackGroupIndex))
		{
			GrannySetTrackGroupLOD(Builder, TrackGroupIndex, true, 1.0f);
			GrannySetTrackGroupTarget(Builder, TrackGroupIndex, InstanceOfModel);
		}
		else 
		{
			GrannySetTrackGroupLOD(Builder, 1, true, 1.0f);

			GrannySetTrackGroupTarget(Builder, 0, InstanceOfModel);
		}
		m_pgrnCtrl = GrannyEndControlledAnimation(Builder);

	}

	if (!m_pgrnCtrl)
		return;

	GrannySetControlSpeed(m_pgrnCtrl, speedRatio);
	GrannySetControlLoopCount(m_pgrnCtrl, loopCount);

	if (isFirst)
	{
		GrannySetControlEaseIn(m_pgrnCtrl, false);
		GrannySetControlEaseOut(m_pgrnCtrl, false);
	}
	else
	{
		GrannySetControlEaseIn(m_pgrnCtrl, true);
		GrannySetControlEaseOut(m_pgrnCtrl, false);
		if (blendTime > 0.0f)
			GrannySetControlEaseInCurve(m_pgrnCtrl, localTime, localTime + blendTime, 0.0f, 0.0f, 1.0f, 1.0f);
	}
	GrannyFreeControlOnceUnused(m_pgrnCtrl);
}

void CGrannyModelInstance::ChangeMotionPointer(const std::shared_ptr<CGrannyMotion> pMotion, int32_t loopCount, float speedRatio)
{
	granny_model_instance* pgrnModelInstance = m_modelInstance;
	if (!pgrnModelInstance)
		return;

	float fSkipTime = 0.3f;
	float localTime = GetLocalTime() - fSkipTime;

	if (m_pgrnCtrl)
	{
		GrannySetControlEaseIn(m_pgrnCtrl, false);
		GrannySetControlEaseOut(m_pgrnCtrl, false);
		GrannyCompleteControlAt(m_pgrnCtrl, localTime);
		GrannyFreeControlIfComplete(m_pgrnCtrl);
	}

	m_pgrnCtrl = GrannyPlayControlledAnimation(localTime, pMotion->GetGrannyAnimationPointer(), pgrnModelInstance);
	if (!m_pgrnCtrl)
		return;

	GrannySetControlSpeed(m_pgrnCtrl, speedRatio);
	GrannySetControlLoopCount(m_pgrnCtrl, loopCount);
	GrannySetControlEaseIn(m_pgrnCtrl, false);
	GrannySetControlEaseOut(m_pgrnCtrl, false);

	GrannyFreeControlOnceUnused(m_pgrnCtrl);
}

void CGrannyModelInstance::SetMotionAtEnd()
{
	if (!m_pgrnCtrl)
		return;

	float endingTime = GrannyGetControlLocalDuration(m_pgrnCtrl);
	GrannySetControlRawLocalClock(m_pgrnCtrl, endingTime);
}

uint32_t CGrannyModelInstance::GetLoopIndex()
{
	if (m_pgrnCtrl)
		return GrannyGetControlLoopIndex(m_pgrnCtrl);
	return 0;
}

void CGrannyModelInstance::PrintControls()
{
	// Not needed anymore..
}