#include "stdafx.h"
#include "SoundManager.h"
#include "SoundInstance.h"
#include <VFE/Include/VFE.hpp>

CSoundInstanceStream::CSoundInstanceStream(SoLoud::Soloud& sld) : ISoundInstance(sld), m_fSpeed(0.0f), m_sampleHandle(0)
{
}

CSoundInstanceStream::~CSoundInstanceStream()
{
	Destroy();
}

void CSoundInstanceStream::Destroy()
{
	m_isValid = false;
}

bool CSoundInstanceStream::SetStream(const char* filename, bool is3d)
{
	auto vfs = CallFS().Open(filename);
	if (!vfs)
	{
		SysLog("Failed to load {0}", filename);
		return false;
	}
	const uint32_t size = vfs->GetSize();

	storm::View data(storm::GetDefaultAllocator());
	vfs->GetView(0, data, size);

	SoLoud::result result = m_sample.loadMem(reinterpret_cast<const uint8_t*>(data.GetData()), size, true, true);

	if (result != SoLoud::SO_NO_ERROR)
	{
		SysLog("SoLoud: Cannot load sound {0} rs {1}\n", filename, result);
		return false;
	}

	m_sampleHandle = m_soloud.playBackground(m_sample, 0.0f, true);

	m_soloud.setProtectVoice(m_sampleHandle, true);

	m_isValid = true;
	return true;
}

bool CSoundInstanceStream::IsDone() const
{
	if (!m_isValid)
		return true;

	return !m_soloud.isValidVoiceHandle(m_sampleHandle);
}

void CSoundInstanceStream::Play(bool iLoopCount, uint32_t)
{
	if (!m_isValid)
		return;

	m_soloud.setLooping(m_sampleHandle, iLoopCount);
	Resume();
}

void CSoundInstanceStream::Pause() const
{
	if (!m_isValid)
		return;

	m_soloud.setPause(m_sampleHandle, true);
}

void CSoundInstanceStream::Resume() const
{
	if (!m_isValid)
		return;

	m_soloud.setPause(m_sampleHandle, false);
}

void CSoundInstanceStream::Stop()
{
	if (!m_isValid)
		return;

	m_soloud.stop(m_sampleHandle);
	m_sampleHandle = 0;
}

float CSoundInstanceStream::GetVolume() const
{
	if (!m_isValid)
		return 0.0f;

	return m_soloud.getVolume(m_sampleHandle);
}

void CSoundInstanceStream::SetVolume(float volume) const
{
	if (!m_isValid)
		return;

	volume = std::max(0.0f, std::min(1.0f, volume));
	m_soloud.setVolume(m_sampleHandle, volume);
}

bool CSoundInstanceStream::Create(CSoundData*, bool)
{
	return true;
}

void CSoundInstanceStream::SetPosition(float x, float y, float z) const
{
}

void CSoundInstanceStream::SetOrientation(float x_face, float y_face, float z_face,
	float x_normal, float y_normal, float z_normal) const
{
}

void CSoundInstanceStream::SetVelocity(float fx, float fy, float fz) const
{
}

extern float g_fGameFPS; // GameLib

SoLoud::time CSoundInstanceStream::CalcVolumeFromSpeed() const
{
	return (SoLoud::time)1.0f / (m_fSpeed * g_fGameFPS);
}

void CSoundInstanceStream::FadeIn()
{
	m_soloud.fadeVolume(m_sampleHandle, 1.0f, CalcVolumeFromSpeed());
}

void CSoundInstanceStream::FadeOut(float f)
{
	m_soloud.fadeVolume(m_sampleHandle, f, CalcVolumeFromSpeed());
	m_soloud.schedulePause(m_sampleHandle, CalcVolumeFromSpeed());
}
