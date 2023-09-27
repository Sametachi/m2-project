#pragma once
#include <Basic/Singleton.h>
#include "SoundBase.h"
#include "SoundInstance.h"
#include "Type.h"
#include <memory>

constexpr int32_t INSTANCE_MAX_COUNT = 32;
constexpr int32_t MUSIC_INSTANCE_MAX_NUM = 3;

class CSoundManager : public CSoundBase, public Singleton<CSoundManager>
{
public:
	CSoundManager();
	virtual ~CSoundManager();

	bool Initialize();

	void SetPosition(float fx, float fy, float fz);
	void SetDirection(float fxDir, float fyDir, float fzDir, float fxUp, float fyUp, float fzUp);
	void Update();

	float GetSoundScale();
	void SetSoundScale(float fScale);
	void SetAmbienceSoundScale(float fScale);
	void SetSoundVolume(float fVolume);
	void SetSoundVolumeRatio(float fRatio);
	void SetMusicVolume(float fVolume);
	void SetSoundVolumeGrade(int32_t iGrade);
	void SaveVolume();
	void RestoreVolume();
	float GetSoundVolume();
	float GetMusicVolume();

	// Sound
	void PlaySound2D(const char* c_szFileName);
	void PlaySound3D(float fx, float fy, float fz, const char* c_szFileName, int32_t iPlayCount = 1);
	void StopSound3D(uint32_t iIndex);
	int32_t  PlayAmbienceSound3D(float fx, float fy, float fz, const char* c_szFileName, int32_t iPlayCount = 1);
	void PlayCharacterSound3D(float fx, float fy, float fz, const char* c_szFileName, BOOL bCheckFrequency = FALSE);
	void SetSoundVolume3D(uint32_t iIndex, float fVolume);
	void StopAllSound3D();

	// Music
	void PlayMusic(const char* c_szFileName);
	void FadeInMusic(const char* c_szFileName, float fVolumeSpeed = 0.016f);
	void FadeOutMusic(const char* c_szFileName, float fVolumeSpeed = 0.016f);
	void FadeLimitOutMusic(const char* c_szFileName, float fLimitVolume, float fVolumeSpeed = 0.016f);
	void FadeOutAllMusic();
	void FadeAll();

	// Sound Node
	void UpdateSoundData(uint32_t dwcurFrame, const NSound::TSoundDataVector* c_pSoundDataVector);
	void UpdateSoundData(float fx, float fy, float fz, uint32_t dwcurFrame, const NSound::TSoundDataVector* c_pSoundDataVector);
	void UpdateSoundInstance(float fx, float fy, float fz, uint32_t dwcurFrame, const NSound::TSoundInstanceVector* c_pSoundInstanceVector, BOOL bCheckFrequency = FALSE);
	void UpdateSoundInstance(uint32_t dwcurFrame, const NSound::TSoundInstanceVector* c_pSoundInstanceVector);

	// CSoundManager3D
	void SetListenerPosition(float fX, float fY, float fZ);
	void SetListenerVelocity(float fDistanceX, float fDistanceY, float fDistanceZ, float fNagnitude);

	void Destroy();

protected:
	enum class MusicState
	{
		FadeOut,
		Play,
		Off,
	};
	typedef struct SMusicInstance
	{
		uint32_t dwMusicFileNameCRC;
		uint32_t dwIndex;
		MusicState sState;
	} TMusicInstance;

	void PlayMusic(uint32_t dwIndex, const char* c_szFileName, float fVolume, float fVolumeSpeed);
	void StopMusic(uint32_t dwIndex);
	bool GetMusicIndex(const char* c_szFileName, uint32_t& pdwIndex);

	float __ConvertGradeVolumeToApplyVolume(int32_t nVolumeGrade);
	float __ConvertRatioVolumeToApplyVolume(float fVolumeRatio);
	void __SetMusicVolume(float fVolume);
	int32_t SetInstance(const char* c_szFileName, bool b3d);

protected:
	bool m_isSoundDisable;

	float m_fxPosition;
	float m_fyPosition;
	float m_fzPosition;

	float m_fSoundScale;
	float m_fAmbienceSoundScale;
	float m_fSoundVolume;
	float m_fMusicVolume;

	float m_fBackupMusicVolume;
	float m_fBackupSoundVolume;

	TMusicInstance m_MusicInstances[MUSIC_INSTANCE_MAX_NUM];
	robin_hood::unordered_map<std::string, float> m_PlaySoundHistoryMap;

	std::unique_ptr<CSoundInstance> m_Instances[INSTANCE_MAX_COUNT];
	std::unique_ptr<CSoundInstanceStream> m_Instances2[MUSIC_INSTANCE_MAX_NUM];
};
