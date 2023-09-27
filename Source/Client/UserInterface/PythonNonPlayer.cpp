#include "stdafx.h"
#include <Core/ProtoReader.hpp>
#include <EterBase/Lzo.h>
#include "pythonnonplayer.h"
#include "InstanceBase.h"
#include "PythonCharacterManager.h"

bool CPythonNonPlayer::LoadMobProto(const char* c_szFileName)
{
	auto VFSFileString = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!VFSFileString)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(VFSFileString.value());
	CTokenVector TokenVector;

	for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		// Ignore comments
		if (textFileLoader.GetLineString(i).find("#") == 0 ||
			textFileLoader.GetLineString(i).find("//") == 0 ||
			textFileLoader.GetLineString(i).find("--") == 0)
			continue;

		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		TMobTable table {};
		if (!ReadMobProto(TokenVector, table))
		{
			SysLog("Failed to load mob proto on line {0}", i + 1);
			continue;
		}

		m_NonPlayerDataMap.emplace(table.dwVnum, table);
	}

	return true;

}

bool CPythonNonPlayer::LoadMobDesc(const char* c_szFileName)
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(vfs_string.value());

	CTokenVector args;
	for (uint32_t i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &args))
			continue;

		if (args.size() < 2)
		{
			SysLog("Mob name list '{0}' line {1} has only {2} tokens", c_szFileName, i, args.size());
			return false;
		}

		try
		{
			uint32_t vnum = std::stoul(args[0]);
			m_nameMap[vnum] = args[1];
		}
		catch (const std::invalid_argument& ia)
		{
			SysLog("Mob Names '{0}' line {1} has invalid token {2}", c_szFileName, i, args[0]);
			return false;
		}
		catch (const std::out_of_range& oor)
		{
			SysLog("Mob Names '{0}' line {1} has out of range token {2}", c_szFileName, i, args[0]);
			return false;
		}
	}
	return true;
}

std::optional<std::string> CPythonNonPlayer::GetName(uint32_t dwVnum)
{
	const auto it = m_nameMap.find(dwVnum);
	if (it != m_nameMap.end())
	{
		return  it->second;
	}

	auto p = GetTable(dwVnum);
	if (!p)
		return std::nullopt;

	return p->szLocaleName;
}

std::string CPythonNonPlayer::GetMonsterName(uint32_t dwVnum)
{
	const auto it = m_nameMap.find(dwVnum);
	if (it != m_nameMap.end())
	{
		return it->second.c_str();
	}

	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		static std::string sc_szEmpty = "";
		return sc_szEmpty;
	}

	return c_pTable->szLocaleName;
}

bool CPythonNonPlayer::GetInstanceType(uint32_t dwVnum, uint8_t* pbType)
{
	const TMobTable* p = GetTable(dwVnum);
	if (!p)
		return false;

	*pbType = p->bType;

	return true;
}

const TMobTable* CPythonNonPlayer::GetTable(uint32_t dwVnum)
{
	auto itor = m_NonPlayerDataMap.find(dwVnum);
	if (itor == m_NonPlayerDataMap.end())
		return nullptr;

	return &itor->second;
}

uint8_t CPythonNonPlayer::GetEventType(uint32_t dwVnum)
{
	const TMobTable* p = GetTable(dwVnum);

	if (!p)
	{
		return ON_CLICK_EVENT_NONE;
	}

	return p->bOnClickType;
}

uint8_t CPythonNonPlayer::GetEventTypeByVID(uint32_t dwVID)
{
	CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(dwVID);

	if (nullptr == pInstance)
	{
		return ON_CLICK_EVENT_NONE;
	}

	uint32_t dwVnum = pInstance->GetVirtualNumber();
	return GetEventType(dwVnum);
}

uint32_t CPythonNonPlayer::GetMonsterColor(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwMonsterColor;
}

uint32_t CPythonNonPlayer::GetMonsterMaxHP(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->dwMaxHP;
}

uint32_t CPythonNonPlayer::GetMonsterRaceFlag(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->dwRaceFlag;
}

uint32_t CPythonNonPlayer::GetMonsterDamage1(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->dwDamageRange[0];
}

uint32_t CPythonNonPlayer::GetMonsterDamage2(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->dwDamageRange[1];
}

uint32_t CPythonNonPlayer::GetMonsterExp(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->dwExp;
}

float CPythonNonPlayer::GetMonsterDamageMultiply(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0.f;
	}

	return c_pTable->fDamMultiply;
}

uint32_t CPythonNonPlayer::GetMonsterST(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->bStr;
}

uint32_t CPythonNonPlayer::GetMonsterDX(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		return 0;
	}

	return c_pTable->bDex;
}

bool CPythonNonPlayer::IsMonsterStone(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return false;

	return c_pTable->bType == 2;
}

uint8_t CPythonNonPlayer::GetMobRegenCycle(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->bRegenCycle;
}

uint8_t CPythonNonPlayer::GetMobRegenPercent(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->bRegenPercent;
}

uint32_t CPythonNonPlayer::GetMobGoldMin(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwGoldMin;
}

uint32_t CPythonNonPlayer::GetMobGoldMax(uint32_t dwVnum)
{
	const TMobTable* c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->dwGoldMax;
}

void CPythonNonPlayer::Clear()
{
}

void CPythonNonPlayer::Destroy()
{
	m_NonPlayerDataMap.clear();
}

CPythonNonPlayer::CPythonNonPlayer()
{
	Clear();
}

CPythonNonPlayer::~CPythonNonPlayer()
{
	Destroy();
}