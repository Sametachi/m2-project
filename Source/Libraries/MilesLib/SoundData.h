#pragma once
#include <soloud_c.h>
#include <soloud.h>
#include <soloud_wav.h>
#include <soloud_wavstream.h>

class CSoundData
{
public:
	CSoundData() = default;
	~CSoundData();

	bool Create(const char* filename);
	const char* GetFileName() const
	{ 
		return m_filename.c_str(); 
	}
	void Destroy();
	SoLoud::AudioSource& GetSample() 
	{ 
		return m_wav; 
	}

protected:
	std::string		m_filename;
	SoLoud::Wav		m_wav;
};
