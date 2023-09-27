#pragma once
#include "SoundData.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <soloud.h>

typedef robin_hood::unordered_map<uint32_t, CSoundData*> TSoundDataMap;

class CSoundBase
{
public:
	CSoundBase();
	virtual ~CSoundBase();

	void Initialize(uint32_t nBackend = SoLoud::Soloud::AUTO);
	void Destroy();

	CSoundData* AddFile(uint32_t dwFileCRC, const char* filename);
	uint32_t GetFileCRC(const char* filename) const;

	SoLoud::Soloud& GetDriver() 
	{ 
		return m_sldDriver; 
	}

protected:
	SoLoud::Soloud m_sldDriver;
	TSoundDataMap m_dataMap;
	bool m_bInitialized;
};