#include "StdAfx.h"
#include <math.h>

#include "SoundManager.h"
#include "../EterBase/Timer.h"
#include "../EterBase/Stl.h"

CSoundManager::CSoundManager()
{
	m_isSoundDisable = false;

	m_fxPosition = 0.0f;
	m_fyPosition = 0.0f;
	m_fzPosition = 0.0f;

	m_fSoundScale = 200.0f;
	m_fAmbienceSoundScale = 1000.0f;
	m_fSoundVolume = 1.0f;
	m_fMusicVolume = 1.0f;

	m_fBackupMusicVolume = 0.0f;
	m_fBackupSoundVolume = 0.0f;

	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		TMusicInstance& rInstance = m_MusicInstances[i];
		rInstance.dwMusicFileNameCRC = 0;
		rInstance.sState = MusicState::Off;
		rInstance.dwIndex = 0;

		m_Instances2[i] = std::make_unique<CSoundInstanceStream>(m_sldDriver);
	}

	for (auto& pInst : m_Instances)
	{
		pInst = std::make_unique<CSoundInstance>(m_sldDriver);
	}
}

CSoundManager::~CSoundManager() = default;

bool CSoundManager::Initialize()
{
	CSoundBase::Initialize();

	SetListenerPosition(0.0f, 0.0f, 0.0f);

	m_bInitialized = true;
	return true;
}

void CSoundManager::Destroy()
{
	if (!m_bInitialized)
		return;

	for (auto& pInst : m_Instances)
		pInst->Destroy();

	for (auto& pInst : m_Instances)
		pInst->Stop();


	CSoundBase::Destroy();
	m_bInitialized = false;
}

void CSoundManager::SetPosition(float fx, float fy, float fz)
{
	m_fxPosition = fx;
	m_fyPosition = fy;
	m_fzPosition = fz;
}

void CSoundManager::SetDirection(float fxDir, float fyDir, float fzDir, float fxUp, float fyUp, float fzUp)
{
	m_sldDriver.set3dListenerUp(fxUp, fyUp, -fzUp);
	m_sldDriver.set3dListenerPosition(fxDir, fyDir, fzDir);
}

void CSoundManager::Update()
{
	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		TMusicInstance& rMusicInstance = m_MusicInstances[i];
		if (MusicState::Off == rMusicInstance.sState)
			continue;

		if (rMusicInstance.sState == MusicState::FadeOut)
		{
			auto fVolume = m_Instances2[i]->GetVolume();
			if (fVolume <= 0.1f)
			{
				rMusicInstance.sState = MusicState::Off;
				rMusicInstance.dwMusicFileNameCRC = 0;
				rMusicInstance.dwIndex = 0;
			}
		}
	}

	m_sldDriver.update3dAudio();
}

void CSoundManager::UpdateSoundData(uint32_t dwcurTime, const NSound::TSoundDataVector* c_pSoundDataVector)
{
	assert(!"CSoundManager::UpdateSoundData");
}

void CSoundManager::UpdateSoundData(float fx, float fy, float fz, uint32_t dwcurTime, const NSound::TSoundDataVector* c_pSoundDataVector)
{
	assert(!"CSoundManager::UpdateSoundData");
}

void CSoundManager::UpdateSoundInstance(float fx, float fy, float fz, uint32_t dwcurFrame, const NSound::TSoundInstanceVector* c_pSoundInstanceVector, BOOL bCheckFrequency)
{
	for (uint32_t i = 0; i < c_pSoundInstanceVector->size(); ++i)
	{
		const NSound::TSoundInstance& c_rSoundInstance = c_pSoundInstanceVector->at(i);
		if (c_rSoundInstance.dwFrame == dwcurFrame)
		{
			PlayCharacterSound3D(fx, fy, fz, c_rSoundInstance.strSoundFileName.c_str(), bCheckFrequency);
		}
	}
}

void CSoundManager::UpdateSoundInstance(uint32_t dwcurFrame, const NSound::TSoundInstanceVector* c_pSoundInstanceVector)
{
	for (uint32_t i = 0; i < c_pSoundInstanceVector->size(); ++i)
	{
		const NSound::TSoundInstance& c_rSoundInstance = c_pSoundInstanceVector->at(i);

		if (c_rSoundInstance.dwFrame == dwcurFrame)
		{
			PlaySound2D(c_rSoundInstance.strSoundFileName.c_str());
		}
	}
}

float CSoundManager::GetSoundScale()
{
	return m_fSoundScale;
}

void CSoundManager::SetSoundScale(float fScale)
{
	m_fSoundScale = fScale;
}

void CSoundManager::SetAmbienceSoundScale(float fScale)
{
	m_fAmbienceSoundScale = fScale;
}

void CSoundManager::SetSoundVolume(float fVolume)
{
	if (m_isSoundDisable)
	{
		m_fBackupSoundVolume = fVolume;
		return;
	}

	fVolume = fMAX(fVolume, 0.0f);
	fVolume = fMIN(fVolume, 1.0f);
	m_fSoundVolume = fVolume;

	if (!m_isSoundDisable)
	{
		m_fBackupSoundVolume = fVolume;
	}
}

void CSoundManager::__SetMusicVolume(float fVolume)
{
	if (m_isSoundDisable)
	{
		m_fBackupMusicVolume = fVolume;
		return;
	}

	fVolume = fMAX(fVolume, 0.0f);
	fVolume = fMIN(fVolume, 1.0f);
	m_fMusicVolume = fVolume;

	if (!m_isSoundDisable)
	{
		m_fBackupMusicVolume = fVolume;
	}

	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		TMusicInstance& rMusicInstance = m_MusicInstances[i];
		if (MusicState::Play != rMusicInstance.sState)
			continue;

		if (m_bInitialized)
			m_Instances2[i]->SetVolume(fVolume);
	}
}

float CSoundManager::__ConvertRatioVolumeToApplyVolume(float fRatioVolume)
{
	if (0.1f > fRatioVolume)
		return fRatioVolume;

	return (float)pow(10.0f, (-1.0f + fRatioVolume));
}

float CSoundManager::__ConvertGradeVolumeToApplyVolume(int32_t nGradeVolume)
{
	return __ConvertRatioVolumeToApplyVolume(nGradeVolume / 5.0f);
}


void CSoundManager::SetSoundVolumeGrade(int32_t iGrade)
{
	float fVolume = __ConvertGradeVolumeToApplyVolume(iGrade);
	SetSoundVolume(fVolume);
}

void CSoundManager::SetSoundVolumeRatio(float fRatio)
{
	float fVolume = __ConvertRatioVolumeToApplyVolume(fRatio);
	SetSoundVolume(fVolume);
}

void CSoundManager::SetMusicVolume(float fVolume)
{
	__SetMusicVolume(fVolume);
}

float CSoundManager::GetSoundVolume()
{
	return m_fSoundVolume;
}

float CSoundManager::GetMusicVolume()
{
	return m_fMusicVolume;
}

void CSoundManager::PlaySound2D(const char* c_szFileName)
{
	if (0.0f == GetSoundVolume() || !m_bInitialized)
		return;

	auto id = SetInstance(c_szFileName, false);

	if (id == -1)
		return;

	m_Instances[id]->SetVolume(GetSoundVolume());
	m_Instances[id]->Play(false);
}

void CSoundManager::PlaySound3D(float fx, float fy, float fz, const char* c_szFileName, int32_t iPlayCount)
{
	if (0.0f == GetSoundVolume() || !m_bInitialized)
		return;

	int32_t iIndex = SetInstance(c_szFileName, true);

	if (iIndex < 0)
		return;

	auto& pInstance = m_Instances[iIndex];

	pInstance->SetPosition((fx - m_fxPosition) / m_fSoundScale,
		(fy - m_fyPosition) / m_fSoundScale,
		(fz - m_fzPosition) / m_fSoundScale);

	pInstance->SetVolume(GetSoundVolume());
	pInstance->Play(iPlayCount);
}

int32_t CSoundManager::PlayAmbienceSound3D(float fx, float fy, float fz, const char* c_szFileName, int32_t iPlayCount)
{
	if (0.0f == GetSoundVolume() || !m_bInitialized)
		return -1;

	auto iIndex = SetInstance(c_szFileName, true);
	if (-1 == iIndex)
		return -1;

	auto& pInstance = m_Instances[iIndex];

	pInstance->SetPosition((fx - m_fxPosition) / m_fAmbienceSoundScale,
		(fy - m_fyPosition) / m_fAmbienceSoundScale,
		(fz - m_fzPosition) / m_fAmbienceSoundScale);

	pInstance->SetVolume(GetSoundVolume());
	pInstance->Play(iPlayCount);

	return iIndex;
}

void CSoundManager::PlayCharacterSound3D(float fx, float fy, float fz, const char* c_szFileName, BOOL bCheckFrequency)
{
	if (0.0f == GetSoundVolume() || !m_bInitialized)
		return;

	if (bCheckFrequency)
	{
		static float s_fLimitDistance = 5000 * 5000;
		float fdx = (fx - m_fxPosition) * (fx - m_fxPosition);
		float fdy = (fy - m_fyPosition) * (fy - m_fyPosition);

		if (fdx + fdy > s_fLimitDistance)
			return;

		auto itor = m_PlaySoundHistoryMap.find(c_szFileName);
		if (m_PlaySoundHistoryMap.end() != itor)
		{
			float fTime = itor->second;
			if (CTimer::GetInstance()->GetCurrentSecond() - fTime < 0.3f)
			{
				return;
			}
		}

		m_PlaySoundHistoryMap.erase(c_szFileName);
		m_PlaySoundHistoryMap.emplace(std::map<std::string, float>::value_type(c_szFileName, CTimer::GetInstance()->GetCurrentSecond()));
	}

	auto iIndex = SetInstance(c_szFileName, true);
	if (-1 == iIndex)
		return;

	auto& pInstance = m_Instances[iIndex];

	pInstance->SetPosition((fx - m_fxPosition) / m_fSoundScale,
		(fy - m_fyPosition) / m_fSoundScale,
		(fz - m_fzPosition) / m_fSoundScale);

	pInstance->SetVolume(GetSoundVolume());
	pInstance->Play(false);
}

void CSoundManager::StopSound3D(uint32_t iIndex)
{
	if (iIndex >= INSTANCE_MAX_COUNT || !m_bInitialized)
		return;

	m_Instances[iIndex]->Stop();
}

void CSoundManager::SetSoundVolume3D(uint32_t iIndex, float fVolume)
{
	if (iIndex >= INSTANCE_MAX_COUNT)
		return;

	m_Instances[iIndex]->SetVolume(fVolume);
}

void CSoundManager::StopAllSound3D()
{
	for (int32_t i = 0; i < INSTANCE_MAX_COUNT; ++i)
	{
		StopSound3D(i);
	}
}

void CSoundManager::PlayMusic(const char* c_szFileName)
{
	PlayMusic(0, c_szFileName, GetMusicVolume(), 0.0f);
}

void CSoundManager::FadeInMusic(const char* c_szFileName, float fVolumeSpeed)
{
	if (!m_bInitialized)
		return;

	uint32_t dwIndex;
	if (GetMusicIndex(c_szFileName, dwIndex))
	{
		m_MusicInstances[dwIndex].sState = MusicState::Play;
		m_Instances2[dwIndex]->SetVolumeSpeed(fVolumeSpeed);
		m_Instances2[dwIndex]->FadeIn();
		return;
	}

	FadeOutAllMusic();

	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		TMusicInstance& rMusicInstance = m_MusicInstances[i];
		if (MusicState::Off != rMusicInstance.sState)
			continue;

		PlayMusic(i, c_szFileName, 0.0f, fVolumeSpeed);
	}
}

void CSoundManager::FadeLimitOutMusic(const char* c_szFileName, float fLimitVolume, float fVolumeSpeed)
{
	if (!m_bInitialized)
		return;

	uint32_t dwIndex;
	if (!GetMusicIndex(c_szFileName, dwIndex))
	{
		TraceLog("FadeOutMusic: {} - ERROR NOT EXIST", c_szFileName);
		return;
	}

	if (dwIndex >= MUSIC_INSTANCE_MAX_NUM)
	{
		TraceLog("FadeOutMusic: {} - ERROR OUT OF RANGE", c_szFileName);
		return;
	}

//	SMusicInstance& rkMusicInst = m_MusicInstances[dwIndex];
	m_Instances2[dwIndex]->FadeOut(__ConvertRatioVolumeToApplyVolume(fLimitVolume));
	m_MusicInstances[dwIndex].sState = MusicState::FadeOut;
}

void CSoundManager::FadeOutMusic(const char* c_szFileName, float fVolumeSpeed)
{
	if (!m_bInitialized)
		return;

	uint32_t dwIndex;
	if (!GetMusicIndex(c_szFileName, dwIndex))
	{
		TraceLog("FadeOutMusic: {} - ERROR NOT EXIST", c_szFileName);
		return;
	}

	if (dwIndex >= MUSIC_INSTANCE_MAX_NUM)
	{
		TraceLog("FadeOutMusic: {} - ERROR OUT OF RANGE", c_szFileName);
		return;
	}

	m_Instances2[dwIndex]->SetVolumeSpeed(fVolumeSpeed);
	m_Instances2[dwIndex]->FadeOut();
	m_MusicInstances[dwIndex].sState = MusicState::FadeOut;
}

void CSoundManager::FadeOutAllMusic()
{
	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		if (m_MusicInstances[i].sState == MusicState::Off)
			continue;

		m_Instances2[i]->FadeOut();
		m_MusicInstances[i].sState = MusicState::FadeOut;
	}
}

void CSoundManager::SaveVolume()
{
	if (m_isSoundDisable)
		return;

	float fBackupMusicVolume = m_fMusicVolume;
	float fBackupSoundVolume = m_fSoundVolume;
	__SetMusicVolume(0.0f);
	SetSoundVolume(0.0f);
	m_fBackupMusicVolume = fBackupMusicVolume;
	m_fBackupSoundVolume = fBackupSoundVolume;
	m_isSoundDisable = true;
}

void CSoundManager::RestoreVolume()
{
	m_isSoundDisable = false;
	__SetMusicVolume(m_fBackupMusicVolume);
	SetSoundVolume(m_fBackupSoundVolume);
}

void CSoundManager::PlayMusic(uint32_t dwIndex, const char* c_szFileName, float fVolume, float fVolumeSpeed)
{
	if (dwIndex >= MUSIC_INSTANCE_MAX_NUM || !m_bInitialized)
		return;

	auto& inst = m_Instances2[dwIndex];

	if (!inst->SetStream(c_szFileName, false))
	{
		TraceLog("CSoundManager::PlayMusic - Failed to load stream sound : {}", c_szFileName);
		return;
	}

	inst->SetVolumeSpeed(fVolumeSpeed);
	inst->SetVolume(fVolume);
	inst->Play(true);

	if (m_fMusicVolume > 0.000001f) // fixes when BGM is 0
		inst->FadeIn();

	TMusicInstance& rMusicInstance = m_MusicInstances[dwIndex];
	rMusicInstance.sState = MusicState::Play;
	rMusicInstance.dwIndex = dwIndex;

	std::string strFileName = stl_resource_convert(c_szFileName);
	rMusicInstance.dwMusicFileNameCRC = GetCaseCRC32(strFileName.c_str(), strFileName.length());
}

void CSoundManager::StopMusic(uint32_t dwIndex)
{
	if (dwIndex >= MUSIC_INSTANCE_MAX_NUM)
		return;

	m_Instances2[dwIndex]->Stop();

	auto& rMusicInstance = m_MusicInstances[dwIndex];
	rMusicInstance.sState = MusicState::Off;
	rMusicInstance.dwMusicFileNameCRC = 0;
	rMusicInstance.dwIndex = 0;
}

int32_t CSoundManager::SetInstance(const char* c_szFileName, bool b3d)
{
	auto dwFileCRC = GetFileCRC(c_szFileName);
	auto itor = m_dataMap.find(dwFileCRC);

	CSoundData* pkSoundData;

	if (itor == m_dataMap.end())
		pkSoundData = AddFile(dwFileCRC, c_szFileName); // CSoundBase::AddFile
	else
		pkSoundData = itor->second;

	if (!pkSoundData)
		return -1;

	for (int32_t i = 0; i < INSTANCE_MAX_COUNT; i++)
	{
		auto& pkInst = m_Instances[i];

		if (pkInst->IsDone())
		{
			if (!pkInst->Create(pkSoundData, b3d))
			{
				TraceLog("CSoundManager3D::GetInstance (filename: {})", c_szFileName);
				return -1;
			}

			return i;
		}
	}

	return -1;
}

bool CSoundManager::GetMusicIndex(const char* c_szFileName, uint32_t& pdwIndex)
{
	std::string strFileName = stl_resource_convert(c_szFileName);
	auto dwCRC = GetCaseCRC32(strFileName.c_str(), strFileName.length());

	for (int32_t i = 0; i < MUSIC_INSTANCE_MAX_NUM; ++i)
	{
		const TMusicInstance& c_rMusicInstance = m_MusicInstances[i];
		if (c_rMusicInstance.sState != MusicState::Off && c_rMusicInstance.dwMusicFileNameCRC == dwCRC)
		{
			pdwIndex = i;
			return true;
		}
	}

	return false;
}

void CSoundManager::FadeAll()
{
	FadeOutAllMusic();
}

void CSoundManager::SetListenerPosition(float fX, float fY, float fZ)
{
	m_sldDriver.set3dListenerPosition(fX, fY, -fZ);
}

void CSoundManager::SetListenerVelocity(float fDistanceX, float fDistanceY, float fDistanceZ, float fNagnitude)
{
	m_sldDriver.set3dListenerVelocity(fDistanceX, fDistanceY, -fDistanceZ);
}
