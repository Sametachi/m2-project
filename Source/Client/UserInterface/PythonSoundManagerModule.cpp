#include "StdAfx.h"
#include "PythonApplication.h"

static void ssndPlaySound(std::string szFileName)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->PlaySound2D(szFileName.c_str());

}

static void sndPlaySound3D(float fx, float fy, float fz, std::string szFileName)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->PlaySound3D(fx, fy, fz, szFileName.c_str());

}

static void sndPlayMusic(std::string szFileName)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->PlayMusic(szFileName.c_str());

}

static void sndFadeInMusic(std::string szFileName)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->FadeInMusic(szFileName.c_str());

}

static void sndFadeOutMusic(std::string szFileName)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->FadeOutMusic(szFileName.c_str());

}

static void sndFadeOutAllMusic()
{
	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->FadeOutAllMusic();

}

static void sndFadeLimitOutMusic(std::string szFileName, float fLimitVolume)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->FadeLimitOutMusic(szFileName.c_str(), fLimitVolume);

}

static void sndStopAllSound()
{
	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->StopAllSound3D();

}

static void sndSetMusicVolume(float fVolume)
{

	auto rkSndMgr=CSoundManager::GetInstance();	
	rkSndMgr->SetMusicVolume(fVolume);

}

static void sndSetSoundVolumef(float fVolume)
{

	auto rkSndMgr=CSoundManager::GetInstance();	
	rkSndMgr->SetSoundVolumeRatio(fVolume);

}

static void sndSetSoundVolume(float fVolume)
{

	auto rkSndMgr=CSoundManager::GetInstance();	
	rkSndMgr->SetSoundVolumeRatio(fVolume);

}

static void sndSetSoundScale(float fScale)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->SetSoundScale(fScale);

}

static void sndSetAmbienceSoundScale(float fScale)
{

	auto rkSndMgr=CSoundManager::GetInstance();
	rkSndMgr->SetAmbienceSoundScale(fScale);

}



PYBIND11_EMBEDDED_MODULE(snd, m)
{
	m.def("PlaySound",	ssndPlaySound);
	m.def("PlaySound3D",	sndPlaySound3D);
	m.def("PlayMusic",	sndPlayMusic);
	m.def("FadeInMusic",	sndFadeInMusic);
	m.def("FadeOutMusic",	sndFadeOutMusic);
	m.def("FadeOutAllMusic",	sndFadeOutAllMusic);
	m.def("FadeLimitOutMusic",	sndFadeLimitOutMusic);
	m.def("StopAllSound",	sndStopAllSound);
	m.def("SetMusicVolumef",	sndSetMusicVolume);
	m.def("SetMusicVolume",	sndSetMusicVolume);
	m.def("SetSoundVolumef",	sndSetSoundVolumef);
	m.def("SetSoundVolume",	sndSetSoundVolume);
	m.def("SetSoundScale",	sndSetSoundScale);
	m.def("SetAmbienceSoundScale",	sndSetAmbienceSoundScale);

}
