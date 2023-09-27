#pragma once
#include "RaceData.h"
#include <optional>

class CRaceManager : public Singleton<CRaceManager>
{
public:
	using TRaceDataMap = robin_hood::unordered_map<uint32_t, CRaceData*>;
	using TRaceDataIterator = TRaceDataMap::iterator;

public:
	CRaceManager();
	~CRaceManager();

	void Destroy();

	void RegisterRace(uint32_t race, const std::string& path);
	void SetPathName(const std::string& c_szPathName);
	const char* GetFullPathFileName(const char* c_szFileName);

	// Handling
	void CreateRace(uint32_t dwRaceIndex);
	void SelectRace(uint32_t dwRaceIndex);
	void RegisterCacheMotionData(uint8_t motionMode, uint16_t motionIndex, const std::string& filename, uint8_t weight = 100);
	CRaceData* GetSelectedRaceDataPointer();
	// Handling

	std::optional<CRaceData*> GetRaceDataPointer(uint32_t dwRaceIndex);
	bool IsHugeRace(uint32_t race);
	void SetHugeRace(uint32_t race);

	bool IsTransparentRace(uint32_t race);
	void SetTransparentRace(uint32_t race);

	std::optional<float> GetRaceHeight(uint32_t race);
	void SetRaceHeight(uint32_t race, float height);
	bool PreloadRace(uint32_t dwRaceIndex);

private:
	CRaceData* __LoadRaceData(uint32_t dwRaceIndex);
	bool __LoadRaceMotionList(CRaceData& rkRaceData, const std::string& path);

	void __Initialize();
	void __DestroyRaceDataMap();

	// Ownership of the contained instances lies somewhere else.
	TRaceDataMap m_RaceDataMap;

	// This vector owns CRaceData instances created via CreateRace(vnum), since they cannot be stored in m_pathToData (MSM path unknown).
	std::vector<std::unique_ptr<CRaceData> > m_createdRaces;

	// This map owns CRaceData instances registered via RegisterRace.
	robin_hood::unordered_map<uint32_t, float> m_raceHeight;
	robin_hood::unordered_map<std::string, std::unique_ptr<CRaceData>, std::hash<std::string>> m_pathToData;
	robin_hood::unordered_map<uint32_t, std::string> m_racePaths;
	std::vector<uint32_t> m_hugeRace;
	std::vector<uint32_t> m_transparentRace;

	std::string m_strPathName;
	CRaceData* m_pSelectedRaceData;
};