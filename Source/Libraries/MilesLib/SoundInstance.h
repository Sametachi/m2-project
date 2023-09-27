#pragma once
#include "SoundBase.h"
#include "SoundData.h"
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>
#include <soloud_file.h>

class CSoundManager;

class ISoundInstance
{
public:
	virtual ~ISoundInstance() = default;

	virtual void Destroy() = 0;
	virtual bool Create(CSoundData* data, bool b3d) = 0;
	virtual void Play(bool iLoopCount = false, uint32_t dwPlayCycleTimeLimit = 0) = 0;
	virtual void Pause() const = 0;
	virtual void Resume() const = 0;
	virtual void Stop() = 0;
	virtual float GetVolume() const = 0;
	virtual void SetVolume(float volume) const = 0;
	virtual bool IsDone() const = 0;
	virtual void SetPosition(float x, float y, float z) const = 0;
	virtual void SetOrientation(float x_face, float y_face, float z_face, float x_normal, float y_normal, float z_normal) const = 0;
	virtual void SetVelocity(float x, float y, float z) const = 0;

protected:
	ISoundInstance(SoLoud::Soloud& sld) : m_soloud(sld), m_isValid(false), m_dwPlayTime(0) {}

	SoLoud::Soloud& m_soloud;
	bool m_isValid;
	uint32_t m_dwPlayTime;
};

class CSoundInstance : public ISoundInstance
{
public:
	CSoundInstance(SoLoud::Soloud& sld);
	virtual ~CSoundInstance();

	void Destroy();

	bool Create(CSoundData* data, bool b3d);
	void Play(bool doLoop = false, uint32_t dwPlayCycleTimeLimit = 0);
	void Pause() const;
	void Resume() const;
	void Stop();
	float GetVolume() const;
	void SetVolume(float volume) const;
	bool IsDone() const;

	void SetPosition(float x, float y, float z) const;
	void SetOrientation(float x_face, float y_face, float z_face, float x_normal, float y_normal, float z_normal) const;
	void SetVelocity(float fx, float fy, float fz) const;

private:
	SoLoud::handle m_sampleHandle;
};

class CSoundInstanceStream : public ISoundInstance
{
public:
	CSoundInstanceStream(SoLoud::Soloud& sld);
	virtual ~CSoundInstanceStream();

	void Destroy();

	bool SetStream(const char* stream, bool is3d);
	bool Create(CSoundData* pSound, bool is3d);

	void Play(bool iLoopCount = false, uint32_t dwPlayCycleTimeLimit = 0);
	void Pause() const;
	void Resume() const;
	void Stop();
	float GetVolume() const;
	void SetVolume(float volume) const;
	bool IsDone() const;

	void SetPosition(float x, float y, float z) const;
	void SetOrientation(float x_face, float y_face, float z_face, float x_normal, float y_normal, float z_normal) const;
	void SetVelocity(float fx, float fy, float fz) const;

	void FadeIn();
	void FadeOut(float f = 0.0f);
	void SetVolumeSpeed(float f) 
	{ 
		m_fSpeed = f; 
	}

private:
	SoLoud::time CalcVolumeFromSpeed() const;
	SoLoud::WavStream m_sample;
	SoLoud::handle m_sampleHandle;
	float m_fSpeed;
};
