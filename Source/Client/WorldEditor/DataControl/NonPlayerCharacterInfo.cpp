// NonPlayerCharacterInfo.cpp: implementation of the NonPlayerCharacterInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../worldeditor.h"
#include "NonPlayerCharacterInfo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CNonPlayerCharacterInfo aNonPlayerCharacterInfo;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNonPlayerCharacterInfo::CNonPlayerCharacterInfo()
{
	Clear();
}

CNonPlayerCharacterInfo::~CNonPlayerCharacterInfo()
{
	Destroy();
}

bool CNonPlayerCharacterInfo::LoadNonPlayerData(const char * c_szFileName)
{
	static uint32_t s_adwMobProtoKey[4] =
	{
		4813894,
		18955,
		552631,
		6822045
	};

	CMappedFile file;
	LPCVOID pvData;

	if (!CEterPackManager::Instance().Get(file, c_szFileName, &pvData))
		return false;

	uint32_t dwFourCC, dwElements, dwDataSize;

	file.Read(&dwFourCC, sizeof(uint32_t));

	if (dwFourCC != MAKEFOURCC('M', 'M', 'P', 'T'))
	{
		TraceError("CPythonMob::LoadMobTable: invalid Mob proto type %s", c_szFileName);
		return false;
	}

	file.Read(&dwElements, sizeof(uint32_t));
	file.Read(&dwDataSize, sizeof(uint32_t));

	uint8_t * pbData = new uint8_t[dwDataSize];
	file.Read(pbData, dwDataSize);
	/////

	CLZObject zObj;

	if (!CLZO::Instance().Decompress(zObj, pbData, s_adwMobProtoKey))
	{
		delete [] pbData;
		return false;
	}

	uint32_t structSize = zObj.GetSize() / dwElements;
	uint32_t structDiff = zObj.GetSize() % dwElements;
#ifdef ENABLE_PROTOSTRUCT_AUTODETECT
	if (structDiff != 0 && !CPythonNonPlayer::TMobTableAll::IsValidStruct(structSize))
#else
	if ((zObj.GetSize() % sizeof(TMobTable)) != 0)
#endif
	{
		TraceError("CPythonNonPlayer::LoadNonPlayerData: invalid size %u check data format. structSize %u, structDiff %u", zObj.GetSize(), structSize, structDiff);
		return false;
	}

	for (uint32_t i = 0; i < dwElements; ++i)
	{
#ifdef ENABLE_PROTOSTRUCT_AUTODETECT
		CPythonNonPlayer::TMobTable t = { 0 };
		CPythonNonPlayer::TMobTableAll::Process(zObj.GetBuffer(), structSize, i, t);
#else
		CPythonNonPlayer::TMobTable& t = *((CPythonNonPlayer::TMobTable*) zObj.GetBuffer() + i);
#endif
		TMobTable* pTable = &t;

		TMobTable* pNonPlayerData = new TMobTable;
		memcpy(pNonPlayerData, pTable, sizeof(TMobTable));

		//TraceError("%d : %s type[%d] color[%d]", pNonPlayerData->dwVnum, pNonPlayerData->szLocaleName, pNonPlayerData->bType, pNonPlayerData->dwMonsterColor);
		m_NonPlayerDataMap.insert(TNonPlayerDataMap::value_type(pNonPlayerData->dwVnum, pNonPlayerData));
	}

	delete [] pbData;
	return true;
}

bool CNonPlayerCharacterInfo::LoadNPCGroupData(const char * c_szFileName)
{
	CTokenVectorMap stTokenVectorMap;

	LPCVOID pModelData;
	CMappedFile File;

	if (!CEterPackManager::Instance().Get(File, c_szFileName, &pModelData))
	{
		TraceError(" CNonPlayerCharacterInfo::LoadNPCGroupData Load File %s ERROR", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	CTokenVector stTokenVector;

	textFileLoader.Bind(File.Size(), pModelData);

	for (uint32_t i = 0; i < textFileLoader.GetLineCount();)
	{
		uint32_t dwLineIncrementCount = 1;

		if (!textFileLoader.SplitLine(i, &stTokenVector))
		{
			++i;
			continue;
		}

		stl_lowers(stTokenVector[0]);

		// Start or End
		if (0 == stTokenVector[0].compare("group"))
		{

			TNPCGroup * pNPCGroup = new TNPCGroup;
			pNPCGroup->m_FollowerIDVector.clear();

//			const std::string & c_rstrGroupID	= stTokenVector[1].c_str();
//			uint32_t dwGroupID = (uint32_t) atoi(c_rstrGroupID.c_str());

			const std::string & c_rstrGroupName = stTokenVector[1].c_str();
			strncpy(pNPCGroup->m_szName, c_rstrGroupName.c_str(), sizeof(pNPCGroup->m_szName));
			uint32_t dwGroupID = 0;

			bool bLeaderExist = false;
			bool bFollowerExist = false;

			CTokenVector stGroupTokenVector;
			for (uint32_t j = i+1; j < textFileLoader.GetLineCount(); ++j)
			{
				if (!textFileLoader.SplitLine(j, &stGroupTokenVector))
					continue;

				++dwLineIncrementCount;

				stl_lowers(stGroupTokenVector[0]);

				if (0 == stGroupTokenVector[0].compare("}"))
					break;
				else if (0 == stGroupTokenVector[0].compare("{"))
					continue;
				else if (0 == stGroupTokenVector[0].compare("vnum"))
				{
					const std::string & c_rstrGroupID	= stGroupTokenVector[1].c_str();
					dwGroupID = (uint32_t) atoi(c_rstrGroupID.c_str());
				}
				else if (0 == stGroupTokenVector[0].compare("leader"))
				{
					const std::string & c_rstrMonsterGroupLeaderName = stGroupTokenVector[1].c_str();
					const std::string & c_rstrMonsterGroupLeaderVID	= stGroupTokenVector[2].c_str();
					uint32_t dwMonsterGroupLeaderVID = (uint32_t) atoi(c_rstrMonsterGroupLeaderVID.c_str());

					pNPCGroup->m_dwLeaderID = dwMonsterGroupLeaderVID;
					bLeaderExist = true;
				}
				else
				{
					const std::string & c_rstrMonsterGroupFollowerID	= stGroupTokenVector[0].c_str();
					const std::string & c_rstrMonsterGroupFollowerName	= stGroupTokenVector[1].c_str();
					const std::string & c_rstrMonsterGroupFollowerVID	= stGroupTokenVector[2].c_str();
					uint32_t dwMonsterGroupFollowerVID = (uint32_t) atoi(c_rstrMonsterGroupFollowerVID.c_str());
					pNPCGroup->m_FollowerIDVector.push_back(dwMonsterGroupFollowerVID);
					bFollowerExist = true;
				}
			}
			if (!bLeaderExist || !bFollowerExist || 0 == dwGroupID)
			{
				TraceError("Leader or Follower not Exist in Group %d[%s]", dwGroupID, pNPCGroup->m_szName);
				delete pNPCGroup;
				pNPCGroup = NULL;
			}
			else
				m_NPCGroupDataMap.insert(TNPCGroupDataMap::value_type(dwGroupID, pNPCGroup));
		}
		i += dwLineIncrementCount;
	}

	return true;
}

const char * CNonPlayerCharacterInfo::GetNameByVID(uint32_t dwVID)
{
	const TMobTable * p = GetTable(dwVID);

	if (!p)
		return "Noname";

	return p->szLocaleName;
}

uint8_t CNonPlayerCharacterInfo::GetRankByVID(uint32_t dwVID)
{
	const TMobTable * p = GetTable(dwVID);

	if (!p)
		return 0;

	return p->bRank;
}

int32_t CNonPlayerCharacterInfo::GetLevelByVID(uint32_t dwVID)
{
	const TMobTable * p = GetTable(dwVID);

	if (!p)
	{
		return 0;
	}
	return p->bLevel;
}

uint32_t CNonPlayerCharacterInfo::GetVIDByName(std::string strName)
{
	m_NonPlayerDataMapIterator = m_NonPlayerDataMap.begin();
	while(m_NonPlayerDataMapIterator != m_NonPlayerDataMap.end())
	{
		const TMobTable * p = m_NonPlayerDataMapIterator->second;
		if (0 == strName.compare(p->szLocaleName))
		{
			return m_NonPlayerDataMapIterator->first;
		}
		++m_NonPlayerDataMapIterator;
	}
	TraceError("CNonPlayerCharacterInfo::GetVIDByName %s VID could not be found.", strName.c_str());
	return 0;
}

const char * CNonPlayerCharacterInfo::GetNameByMapIndex(uint32_t dwMapIndex)
{
	m_NonPlayerDataMapIterator = m_NonPlayerDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NonPlayerDataMapIterator;
		++dwCount;
	}
	const TMobTable * p = m_NonPlayerDataMapIterator->second;

	if (!p)
		return "Noname";

	return p->szLocaleName;
}

int32_t CNonPlayerCharacterInfo::GetLevelByMapIndex(uint32_t dwMapIndex)
{
	m_NonPlayerDataMapIterator = m_NonPlayerDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NonPlayerDataMapIterator;
		++dwCount;
	}
	const TMobTable * p = m_NonPlayerDataMapIterator->second;

	if (!p)
	{
		return 0;
	}
	return p->bLevel;
}

uint8_t CNonPlayerCharacterInfo::GetRankByMapIndex(uint32_t dwMapIndex)
{
	m_NonPlayerDataMapIterator = m_NonPlayerDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NonPlayerDataMapIterator;
		++dwCount;
	}
	const TMobTable * p = m_NonPlayerDataMapIterator->second;

	if (!p)
		return 0;

	return p->bRank;
}

uint32_t CNonPlayerCharacterInfo::GetVIDByMapIndex(uint32_t dwMapIndex)
{
	m_NonPlayerDataMapIterator = m_NonPlayerDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NonPlayerDataMapIterator;
		++dwCount;
	}
	const TMobTable * p = m_NonPlayerDataMapIterator->second;

	if (!p)
		return 0;

	return m_NonPlayerDataMapIterator->first;
}

uint8_t CNonPlayerCharacterInfo::GetInstanceType(uint32_t dwVnum)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
		return INSTANCE_TYPE_PLAYER;

	return p->bType;
}

const CNonPlayerCharacterInfo::TMobTable * CNonPlayerCharacterInfo::GetTable(uint32_t dwVnum)
{
	TNonPlayerDataMap::iterator itor = m_NonPlayerDataMap.find(dwVnum);

	if (itor == m_NonPlayerDataMap.end())
		return NULL;

	return itor->second;
}

void CNonPlayerCharacterInfo::Clear()
{
}

void CNonPlayerCharacterInfo::Destroy()
{
	for (TNonPlayerDataMap::iterator itor=m_NonPlayerDataMap.begin(); itor!=m_NonPlayerDataMap.end(); ++itor)
		delete itor->second;
	m_NonPlayerDataMap.clear();

	for (TNPCGroupDataMap::iterator aNPCGroupDataIterator = m_NPCGroupDataMap.begin(); aNPCGroupDataIterator != m_NPCGroupDataMap.end(); ++aNPCGroupDataIterator)
		delete aNPCGroupDataIterator->second;
	m_NPCGroupDataMap.clear();
}

uint32_t CNonPlayerCharacterInfo::GetNonPlayerCount()
{
	return m_NonPlayerDataMap.size();
}


// MonsterGroup
const CNonPlayerCharacterInfo::TNPCGroup * CNonPlayerCharacterInfo::GetGroup(uint32_t dwGroupID)
{
	TNPCGroupDataMap::iterator itor = m_NPCGroupDataMap.find(dwGroupID);

	if (itor == m_NPCGroupDataMap.end())
		return NULL;

	return itor->second;
}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupCount()
{
	return m_NPCGroupDataMap.size();
}

const char * CNonPlayerCharacterInfo::GetNPCGroupNameByMapIndex(uint32_t dwMapIndex)
{
	m_NPCGroupDataMapIterator = m_NPCGroupDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NPCGroupDataMapIterator;
		++dwCount;
	}
	const TNPCGroup * pNPCGroup = m_NPCGroupDataMapIterator->second;

	if (!pNPCGroup)
		return "Noname";

	return pNPCGroup->m_szName;
}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupIDByMapIndex(uint32_t dwMapIndex)
{
	m_NPCGroupDataMapIterator = m_NPCGroupDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NPCGroupDataMapIterator;
		++dwCount;
	}
	const TNPCGroup * pNPCGroup = m_NPCGroupDataMapIterator->second;

	if (!pNPCGroup)
		return 0;

	return m_NPCGroupDataMapIterator->first;
}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupLeaderVIDByMapIndex(uint32_t dwMapIndex)
{
	m_NPCGroupDataMapIterator = m_NPCGroupDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NPCGroupDataMapIterator;
		++dwCount;
	}
	const TNPCGroup * pNPCGroup = m_NPCGroupDataMapIterator->second;

	if (!pNPCGroup)
		return 0;

	return pNPCGroup->m_dwLeaderID;
}

const char * CNonPlayerCharacterInfo::GetNPCGroupLeaderNameByMapIndex(uint32_t dwMapIndex)
{
	uint32_t dwLeaderVID = GetNPCGroupLeaderVIDByMapIndex(dwMapIndex);
	if (0 == dwLeaderVID)
		return "Noname";
	else
		return GetNameByVID(dwLeaderVID);
}

std::vector<uint32_t> CNonPlayerCharacterInfo::GetNPCGroupFollowerVIDsByMapIndex(uint32_t dwMapIndex)
{
	std::vector<uint32_t> aFollowerVIDVector;

	m_NPCGroupDataMapIterator = m_NPCGroupDataMap.begin();
	uint32_t dwCount = 0;
	while (dwCount < dwMapIndex)
	{
		++m_NPCGroupDataMapIterator;
		++dwCount;
	}
	const TNPCGroup * pNPCGroup = m_NPCGroupDataMapIterator->second;

	if (!pNPCGroup)
		return aFollowerVIDVector;

	return pNPCGroup->m_FollowerIDVector;

}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupFollowerCountByMapIndex(uint32_t dwMapIndex)
{
	std::vector<uint32_t> & rFollowerVIDVector = GetNPCGroupFollowerVIDsByMapIndex(dwMapIndex);
	return rFollowerVIDVector.size();
}


uint32_t CNonPlayerCharacterInfo::GetGroupIDByGroupName(std::string strGroupName)
{
	m_NPCGroupDataMapIterator = m_NPCGroupDataMap.begin();
	while(m_NPCGroupDataMapIterator != m_NPCGroupDataMap.end())
	{
		const TNPCGroup * pNPCGroup = m_NPCGroupDataMapIterator->second;
		if (0 == strGroupName.compare(pNPCGroup->m_szName))
			return m_NPCGroupDataMapIterator->first;
		++m_NPCGroupDataMapIterator;
	}
	TraceError("CNonPlayerCharacterInfo::GetGroupIDByGroupName %s GroupID could not be found.", strGroupName.c_str());
	return 0;
}

const char * CNonPlayerCharacterInfo::GetNPCGroupNameByGroupID(uint32_t dwGroupID)
{
	const TNPCGroup * pNPCGroup = GetGroup(dwGroupID);

	if (!pNPCGroup)
		return "Noname";

	return pNPCGroup->m_szName;
}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupLeaderVIDByGroupID(uint32_t dwGroupID)
{
	const TNPCGroup * pNPCGroup = GetGroup(dwGroupID);

	if (!pNPCGroup)
		return 0;

	return pNPCGroup->m_dwLeaderID;
}

const char * CNonPlayerCharacterInfo::GetNPCGroupLeaderNameByGroupID(uint32_t dwGroupID)
{
	const TNPCGroup * pNPCGroup = GetGroup(dwGroupID);

	if (!pNPCGroup)
		return "Noname";

	return GetNameByVID(pNPCGroup->m_dwLeaderID);
}

std::vector<uint32_t>	CNonPlayerCharacterInfo::GetNPCGroupFollowerVIDsByGroupID(uint32_t dwGroupID)
{
	static std::vector<uint32_t> aEmptyVector;
	const TNPCGroup * pNPCGroup = GetGroup(dwGroupID);

	if (!pNPCGroup)
		return aEmptyVector;

	return pNPCGroup->m_FollowerIDVector;
}

uint32_t CNonPlayerCharacterInfo::GetNPCGroupFollowerCountByGroupID(uint32_t dwGroupID)
{
	std::vector<uint32_t> aGroupFollowerVector = GetNPCGroupFollowerVIDsByGroupID(dwGroupID);

	return aGroupFollowerVector.size();
}
