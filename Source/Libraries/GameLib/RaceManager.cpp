#include "StdAfx.h"
#include "RaceManager.h"
#include "RaceMotionData.h"

#include <Core/Race/RaceMotionConstants.hpp>
#include <EterLib/ResourceManager.h>

CRaceData* CRaceManager::__LoadRaceData(uint32_t dwRaceIndex)
{
	auto it = m_racePaths.find(dwRaceIndex);
	if (m_racePaths.end() == it)
		return nullptr;

	std::string& path = it->second;
	if (path.empty())
		return nullptr;

	const auto it2 = m_pathToData.find(path);
	if (it2 != m_pathToData.end())
		return it2->second.get();

	std::unique_ptr<CRaceData> pRaceData = std::make_unique<CRaceData>();
	pRaceData->SetRace(dwRaceIndex);

	if (!pRaceData->LoadRaceData(path.c_str()))
	{
		SysLog("CRaceManager::RegisterRacePath(race={0}).LoadRaceData({1})", dwRaceIndex, path.c_str());
		return nullptr;
	}

	__LoadRaceMotionList(*pRaceData, CFileNameHelper::GetPath(path));

	CRaceData* rawPtr = pRaceData.get();
	m_pathToData.emplace(path, std::move(pRaceData));
	return rawPtr;
}

bool CRaceManager::__LoadRaceMotionList(CRaceData& rkRaceData, const std::string& path)
{
	std::string motionListFilename(path);
	motionListFilename += rkRaceData.GetMotionListFileName();

	auto data = CallFS().LoadFileToString(CallFS(), motionListFilename.c_str());
	if (!data)
		return false;

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(data.value());

	rkRaceData.RegisterMotionMode(MOTION_MODE_GENERAL);

	char szMode[256] = { 0 };
	char szType[256] = { 0 };
	char szFile[256] = { 0 };
	int32_t nPercent = 0;
	bool isSpawn = false;

	static std::string stSpawnMotionFileName;
	static std::string stMotionFileName;

	stSpawnMotionFileName = "";
	stMotionFileName = "";

	uint32_t uLineCount = kTextFileLoader.GetLineCount();
	for (uint32_t uLineIndex = 0; uLineIndex < uLineCount; ++uLineIndex)
	{
		const std::string& c_rstLine = kTextFileLoader.GetLineString(uLineIndex);
		if (sscanf(c_rstLine.c_str(), "%s %s %s %d", szMode, szType, szFile, &nPercent))
		{
			uint32_t motionType = MOTION_NONE;
			if (!GetMotionValue(szType, motionType))
				continue;

			stMotionFileName.assign(path.data(), path.size());
			stMotionFileName += szFile;

			rkRaceData.RegisterMotionData(MOTION_MODE_GENERAL, static_cast<uint16_t>(motionType), stMotionFileName.c_str(), static_cast<uint8_t>(nPercent));

			switch (motionType)
			{
			case MOTION_SPAWN:
				isSpawn = true;
				break;
			case MOTION_DAMAGE:
				stSpawnMotionFileName = stMotionFileName;
				break;
			}
		}
	}

	if (!isSpawn && stSpawnMotionFileName != "")
	{
		rkRaceData.RegisterMotionData(MOTION_MODE_GENERAL, MOTION_SPAWN, stSpawnMotionFileName.c_str(), static_cast<uint8_t>(nPercent));
	}

	rkRaceData.RegisterNormalAttack(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK);
	return true;
}

void CRaceManager::RegisterRace(uint32_t race, const std::string& path)
{
	CGraphicThing* pRaceThing = CResourceManager::GetInstance()->LoadResource<CGraphicThing>(path);
	if (!pRaceThing)
	{
		SysLog("CRaceManager::RegisterRace -> Couldn't load: {0}", path);
		return;
	}

	auto r = m_racePaths.emplace(race, path);
	if (!r.second)
	{
		SysLog("Race {0} already registered", race);
	}
}

void CRaceManager::CreateRace(uint32_t dwRaceIndex)
{
	if (m_RaceDataMap.end() != m_RaceDataMap.find(dwRaceIndex))
	{
		SysLog("RaceManager::CreateRace : Race {0} already created", dwRaceIndex);
		return;
	}

	std::unique_ptr<CRaceData> pRaceData(new CRaceData());
	pRaceData->SetRace(dwRaceIndex);

	m_RaceDataMap.emplace(dwRaceIndex, pRaceData.get());
	m_createdRaces.emplace_back(std::move(pRaceData));

	ConsoleLog("CRaceManager::CreateRace(dwRaceIndex={0})", dwRaceIndex);
}

void CRaceManager::SelectRace(uint32_t dwRaceIndex)
{
	auto itor = m_RaceDataMap.find(dwRaceIndex);
	if (m_RaceDataMap.end() == itor)
	{
		assert(!"Failed to select race data!");
		return;
	}

	m_pSelectedRaceData = itor->second;
}

auto CRaceManager::RegisterCacheMotionData(uint8_t motionMode, uint16_t motionIndex, const std::string& filename, uint8_t weight) -> void
{
	CRaceData* pRaceData = GetSelectedRaceDataPointer();
	if (!pRaceData)
	{
		SysLog("RaceData has not been selected");
		return;
	}

	pRaceData->RegisterMotionData(motionMode, motionIndex, GetFullPathFileName(filename.c_str()), weight);
}

CRaceData* CRaceManager::GetSelectedRaceDataPointer()
{
	return m_pSelectedRaceData;
}

std::optional<CRaceData*> CRaceManager::GetRaceDataPointer(uint32_t dwRaceIndex)
{
	auto itor = m_RaceDataMap.find(dwRaceIndex);
	if (m_RaceDataMap.end() == itor)
	{
		CRaceData* pRaceData = __LoadRaceData(dwRaceIndex);
		if (pRaceData)
		{
			m_RaceDataMap.emplace(dwRaceIndex, pRaceData);
			return pRaceData;
		}

		ConsoleLog("CRaceManager::GetRaceDataPointer: cannot load data by dwRaceIndex {0}", dwRaceIndex);
		return std::nullopt;
	}

	return itor->second;
}

bool CRaceManager::PreloadRace(uint32_t dwRaceIndex)
{
	auto itor = m_RaceDataMap.find(dwRaceIndex);
	if (m_RaceDataMap.end() == itor)
	{
		CRaceData* pRaceData = __LoadRaceData(dwRaceIndex);
		if (pRaceData)
		{
			m_RaceDataMap.emplace(dwRaceIndex, pRaceData);
			return true;
		}

		SysLog("CRaceManager::GetRaceDataPointer: cannot load data by dwRaceIndex {0}", dwRaceIndex);
		return false;
	}

	return true;
}

bool CRaceManager::IsHugeRace(uint32_t race)
{
	auto it = std::find(m_hugeRace.begin(), m_hugeRace.end(), race);

	if (it == m_hugeRace.end())
		return false;

	return true;
}

void CRaceManager::SetHugeRace(uint32_t race)
{
	auto it = std::find(m_hugeRace.begin(), m_hugeRace.end(), race);

	if (it != m_hugeRace.end())
		return;

	m_hugeRace.emplace_back(race);
}

bool CRaceManager::IsTransparentRace(uint32_t race)
{
	auto it = std::find(m_transparentRace.begin(), m_transparentRace.end(), race);

	if (it == m_transparentRace.end())
		return false;

	return true;
}

void CRaceManager::SetTransparentRace(uint32_t race)
{
	auto it = std::find(m_transparentRace.begin(), m_transparentRace.end(), race);

	if (it != m_transparentRace.end())
		return;

	m_transparentRace.emplace_back(race);
}

std::optional<float> CRaceManager::GetRaceHeight(uint32_t race)
{
	auto it = m_raceHeight.find(race);

	if (it == m_raceHeight.end())
		return std::nullopt;

	return it->second;
}

void CRaceManager::SetRaceHeight(uint32_t race, float height)
{
	auto it = m_raceHeight.find(race);

	if (it != m_raceHeight.end())
		return;

	m_raceHeight[race] = height;
}

void CRaceManager::SetPathName(const std::string& c_szPathName)
{
	m_strPathName = c_szPathName;
}

const char* CRaceManager::GetFullPathFileName(const char* c_szFileName)
{
	static std::string s_stFileName;

	if (c_szFileName[0] != '.')
	{
		s_stFileName = m_strPathName;
		s_stFileName += c_szFileName;
	}
	else
	{
		s_stFileName = c_szFileName;
	}

	return s_stFileName.c_str();
}

void CRaceManager::__Initialize()
{
	m_pSelectedRaceData = nullptr;
}

void CRaceManager::__DestroyRaceDataMap()
{
	m_pathToData.clear();
	m_createdRaces.clear();
}

void CRaceManager::Destroy()
{
	__DestroyRaceDataMap();

	__Initialize();
}

CRaceManager::CRaceManager()
{
	__Initialize();
}

CRaceManager::~CRaceManager()
{
	Destroy();
}
