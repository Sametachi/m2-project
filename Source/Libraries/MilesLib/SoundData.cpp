#include "StdAfx.h"
#include "SoundData.h"
#include "../eterBase/Timer.h"
#include <VFE/Include/VFE.hpp>

bool CSoundData::Create(const char* filename)
{
	m_wav.setSingleInstance(true);
	auto vfs = CallFS().Open(filename, Buffered);
	if (!vfs)
	{
		SysLog("Failed to load {0}", filename);
		return false;
	}
	const uint32_t size = vfs->GetSize();

	storm::View data(storm::GetDefaultAllocator());
	vfs->GetView(0, data, size);

	SoLoud::result result = m_wav.loadMem(reinterpret_cast<const uint8_t*>(data.GetData()), size, true, true);

	if (result != SoLoud::SO_NO_ERROR)
	{
		SysLog("SoLoud error {0}: {1}", result, filename);
		return false;
	}

	m_filename = filename;

	m_wav.setSingleInstance(true); // Tldr: Disable if you don't like this
	return true;
}

void CSoundData::Destroy()
{
}

CSoundData::~CSoundData()
{
	Destroy();
}
