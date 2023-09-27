#pragma once

#include <Core/Constants/NonPlayer.hpp>
#include <Core/Tables.hpp>

class CPythonNonPlayer : public Singleton<CPythonNonPlayer>
{
public:
	enum  EClickEvent
	{
		ON_CLICK_EVENT_NONE = 0,
		ON_CLICK_EVENT_BATTLE = 1,
		ON_CLICK_EVENT_SHOP = 2,
		ON_CLICK_EVENT_TALK = 3,
		ON_CLICK_EVENT_VEHICLE = 4,

		ON_CLICK_EVENT_MAX_NUM,
	};

	using TMobTableList = std::list<TMobTable*>;
	using TNonPlayerDataMap = std::map<uint32_t, TMobTable>;
	using NameMap = std::unordered_map<uint32_t, std::string>;

public:
	CPythonNonPlayer();
	virtual ~CPythonNonPlayer();

	void Clear();
	void Destroy();

	bool LoadMobProto(const char* c_szFileName);
	bool LoadMobDesc(const char* c_szFileName);
	const TMobTable* GetTable(uint32_t dwVnum);
	std::optional<std::string> GetName(uint32_t dwVnum);
	bool GetInstanceType(uint32_t dwVnum, uint8_t* pbType);
	uint8_t GetEventType(uint32_t dwVnum);
	uint8_t GetEventTypeByVID(uint32_t dwVID);
	uint32_t GetMonsterColor(uint32_t dwVnum);
	std::string GetMonsterName(uint32_t dwVnum);

	uint32_t GetMonsterMaxHP(uint32_t dwVnum);
	uint32_t GetMonsterRaceFlag(uint32_t dwVnum);
	uint32_t GetMonsterDamage1(uint32_t dwVnum);
	uint32_t GetMonsterDamage2(uint32_t dwVnum);
	uint32_t GetMonsterExp(uint32_t dwVnum);
	float GetMonsterDamageMultiply(uint32_t dwVnum);
	uint32_t GetMonsterST(uint32_t dwVnum);
	uint32_t GetMonsterDX(uint32_t dwVnum);
	bool IsMonsterStone(uint32_t dwVnum);
	uint8_t GetMobRegenCycle(uint32_t dwVnum);
	uint8_t GetMobRegenPercent(uint32_t dwVnum);
	uint32_t GetMobGoldMin(uint32_t dwVnum);
	uint32_t GetMobGoldMax(uint32_t dwVnum);

protected:
	TNonPlayerDataMap m_NonPlayerDataMap;
	NameMap m_nameMap;
};