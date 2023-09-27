#include "stdafx.h"
#include "SoundInstance.h"
#include "SoundManager.h"
#include <VFE/Include/VFE.hpp>
#include "../eterBase/Timer.h"

CSoundInstance::CSoundInstance(SoLoud::Soloud& sld) : ISoundInstance(sld), m_sampleHandle(0)
{
}

CSoundInstance::~CSoundInstance()
{
	Destroy();
}

void CSoundInstance::Destroy()
{
	m_isValid = false;
}

bool CSoundInstance::Create(CSoundData* data, bool b3d)
{
	if (b3d)
		m_sampleHandle = m_soloud.play3d(data->GetSample(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true);
	else
		m_sampleHandle = m_soloud.play(data->GetSample(), 0.0f, 0.0f, true);

	m_isValid = true;	
	return true;
}

bool CSoundInstance::IsDone() const
{
	if (!m_isValid)
		return true;

	return !m_soloud.isValidVoiceHandle(m_sampleHandle);
}

void CSoundInstance::Play(bool loop, uint32_t dwPlayCycleTimeLimit)
{
	if (!m_isValid)
		return;

	auto dwCurTime = ELTimer_GetMSec();

	if (dwCurTime - m_dwPlayTime < dwPlayCycleTimeLimit)
		return;

	m_dwPlayTime = dwCurTime;
	m_soloud.setLooping(m_sampleHandle, loop);

	Resume();
}

void CSoundInstance::Pause() const
{
	m_soloud.setPause(m_sampleHandle, true);
}

void CSoundInstance::Resume() const
{
	m_soloud.setPause(m_sampleHandle, false);
}

void CSoundInstance::Stop()
{
	m_soloud.stop(m_sampleHandle);
	m_sampleHandle = 0;
}

float CSoundInstance::GetVolume() const
{
	return m_soloud.getVolume(m_sampleHandle);
}

void CSoundInstance::SetVolume(float volume) const
{
	volume = std::max(0.0f, std::min(1.0f, volume));
	m_soloud.setVolume(m_sampleHandle, volume);
}

void CSoundInstance::SetPosition(float x, float y, float z) const
{
	m_soloud.set3dSourcePosition(m_sampleHandle, x, y, -z);
}

void CSoundInstance::SetOrientation(float x_face, float y_face, float z_face, float x_normal, float y_normal, float z_normal) const
{
	assert(!" CSoundInstance::SetOrientation - Not implemented");
}

void CSoundInstance::SetVelocity(float fDistanceX, float fDistanceY, float fDistanceZ) const
{
	m_soloud.set3dSourceVelocity(m_sampleHandle, fDistanceX, fDistanceY, fDistanceZ);
}
