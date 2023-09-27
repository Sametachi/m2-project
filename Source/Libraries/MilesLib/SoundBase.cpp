#include "Stdafx.h"
#include "SoundBase.h"

CSoundBase::CSoundBase() : m_bInitialized(false)
{
}

CSoundBase::~CSoundBase()
{
	Destroy();
}

void CSoundBase::Destroy()
{
	for (auto& pSoundData : m_dataMap)
	{
		delete pSoundData.second;
	}

	m_dataMap.clear();

	m_sldDriver.deinit();
	m_bInitialized = false;
}

void CSoundBase::Initialize(uint32_t backend)
{
	auto rs = m_sldDriver.init(SoLoud::Soloud::CLIP_ROUNDOFF | SoLoud::Soloud::ENABLE_VISUALIZATION, backend);

	if (rs != SoLoud::SO_NO_ERROR)
	{
		TraceLog("SoLoud init error {}", rs);
		return;
	}

	m_dataMap.clear();
	m_bInitialized = true;
}

uint32_t CSoundBase::GetFileCRC(const char* filename) const
{
	return GetCRC32(filename, strlen(filename));
}

CSoundData* CSoundBase::AddFile(uint32_t dwFileCRC, const char* filename)
{
	CSoundData* pSoundData = new CSoundData();
	if (!pSoundData->Create(filename))
	{
		delete pSoundData;
		return nullptr;
	}

	m_dataMap.insert_or_assign(dwFileCRC, pSoundData);
	return pSoundData;
}
