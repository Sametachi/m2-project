#include "stdafx.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "regen.h"
#include "sectree.h"
#include "text_file_loader.h"
#include "questmanager.h"
#include "locale_service.h"

CMob::CMob()
{
	memset(&m_table, 0, sizeof(m_table));

	for(size_t i=0 ; i < MOB::SKILL_MAX_NUM ; ++i)
	{
		m_mobSkillInfo[i].dwSkillVnum = 0;
		m_mobSkillInfo[i].bSkillLevel = 0;
		m_mobSkillInfo[i].vecSplashAttack.clear();
	}
}

CMob::~CMob()
{
}

void CMob::AddSkillSplash(int32_t iIndex, uint32_t dwTiming, uint32_t dwHitDistance)
{
	if (iIndex >= MOB::SKILL_MAX_NUM || iIndex < 0)
		return;

	PyLog("MOB_SPLASH {} idx {} timing {} hit_distance {}",
			m_table.szLocaleName, iIndex, dwTiming, dwHitDistance);

	m_mobSkillInfo[iIndex].vecSplashAttack.push_back(TMobSplashAttackInfo(dwTiming, dwHitDistance));
}

CMobInstance::CMobInstance()
	: m_IsBerserk(false), m_IsGodSpeed(false), m_IsRevive(false)
{
	m_dwLastAttackedTime = get_dword_time();
	m_dwLastWarpTime = get_dword_time();

	memset(&m_posLastAttacked, 0, sizeof(m_posLastAttacked));
}

CMobManager::CMobManager()
{
}

CMobManager::~CMobManager()
{
}

bool CMobManager::Initialize(TMobTable* pTable, int32_t iSize)
{
	m_map_pMobByVnum.clear();
	m_map_pMobByName.clear();

	TMobTable * t = pTable;


	for (int32_t i = 0; i < iSize; ++i, ++t)
	{
		CMob* pMob = M2_NEW CMob;

		memcpy(&pMob->m_table, t, sizeof(TMobTable));

		m_map_pMobByVnum.insert(std::map<uint32_t, CMob *>::value_type(t->dwVnum, pMob));
		m_map_pMobByName.insert(std::map<std::string, CMob *>::value_type(t->szLocaleName, pMob));

		int32_t SkillCount = 0;

		for (int32_t j = 0; j < MOB::SKILL_MAX_NUM; ++j)
			if (pMob->m_table.Skills[j].dwVnum)
				++SkillCount;

		PyLog("MOB: VNUM {} NAME {} LEVEL {} HP {} DEF {} EXP {} DROP_ITEM_VNUM {} SKILL_COUNT {}", 
				t->dwVnum, t->szLocaleName, t->bLevel, t->dwMaxHP, t->wDef, t->dwExp, t->dwDropItemVnum, SkillCount);

		if (t->bType == CHAR_TYPE_NPC || t->bType == CHAR_TYPE_WARP || t->bType == CHAR_TYPE_GOTO)
			CHARACTER_MANAGER::GetInstance()->RegisterRaceNum(t->dwVnum);

		quest::CQuestManager::GetInstance()->RegisterNPCVnum(t->dwVnum);
	}


	const int32_t FILE_NAME_LEN = 256;
	char szGroupFileName[FILE_NAME_LEN];
	char szGroupGroupFileName[FILE_NAME_LEN];

	snprintf(szGroupFileName, sizeof(szGroupGroupFileName),
			"%s/group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szGroupGroupFileName, sizeof(szGroupGroupFileName),
			"%s/group_group.txt", LocaleService_GetBasePath().c_str());
	
	if (!LoadGroup(szGroupFileName))
	{
		SysLog("cannot load {}", szGroupFileName);
		thecore_shutdown();
	}
	if (!LoadGroupGroup(szGroupGroupFileName))
	{
		SysLog("cannot load {}", szGroupGroupFileName);
		thecore_shutdown();
	}
	CHARACTER_MANAGER::GetInstance()->for_each_pc(std::bind(&CMobManager::RebindMobProto, this, std::placeholders::_1));
	return true;
}

void CMobManager::RebindMobProto(LPCHARACTER ch)
{
	if (ch->IsPC())
		return;

	const CMob* pMob = Get(ch->GetRaceNum());

	if (pMob)
		ch->SetProto(pMob);
}

const CMob * CMobManager::Get(uint32_t dwVnum)
{
	std::map<uint32_t, CMob *>::iterator it = m_map_pMobByVnum.find(dwVnum);

	if (it == m_map_pMobByVnum.end())
		return NULL;

	return it->second;
}

const CMob * CMobManager::Get(const char* c_pszName, bool bIsAbbrev)
{
	std::map<std::string, CMob *>::iterator it;

	if (!bIsAbbrev)
	{
		it = m_map_pMobByName.find(c_pszName);

		if (it == m_map_pMobByName.end())
			return NULL;

		return it->second;
	}

	int32_t len = strlen(c_pszName);
	it = m_map_pMobByName.begin();

	while (it != m_map_pMobByName.end())
	{
		if (!strncmp(it->first.c_str(), c_pszName, len))
			return it->second;

		++it;
	}

	return NULL;
}

void CMobManager::IncRegenCount(uint8_t bRegenType, uint32_t dwVnum, int32_t iCount, int32_t iTime)
{
	switch (bRegenType)
	{
		case REGEN_TYPE_MOB:
			m_mapRegenCount[dwVnum] += iCount * 86400. / iTime;
			break;

		case REGEN_TYPE_GROUP:
			{
				CMobGroup* pGroup = CMobManager::GetInstance()->GetGroup(dwVnum);
				if (!pGroup)
					return;
				const std::vector<uint32_t>& c_rdwMembers = pGroup->GetMemberVector();

				for (uint32_t i=0; i<c_rdwMembers.size(); i++)
					m_mapRegenCount[c_rdwMembers[i]] += iCount * 86400. / iTime;
			}
			break;

		case REGEN_TYPE_GROUP_GROUP:
			{
				std::map<uint32_t, CMobGroupGroup *>::iterator it = m_map_pMobGroupGroup.find(dwVnum);

				if (it == m_map_pMobGroupGroup.end())
					return;

				std::vector<uint32_t>& v = it->second->m_vec_dwMemberVnum;
				for (uint32_t i=0; i<v.size(); i++)
				{
					CMobGroup* pGroup = CMobManager::GetInstance()->GetGroup(v[i]);
					if (!pGroup)
						return;
					const std::vector<uint32_t>& c_rdwMembers = pGroup->GetMemberVector();

					for (uint32_t i=0; i<c_rdwMembers.size(); i++)
						m_mapRegenCount[c_rdwMembers[i]] += iCount * 86400. / iTime / v.size();
				}
			}
			break;
	}
}

void CMobManager::DumpRegenCount(const char* c_szFilename)
{
	FILE* fp = fopen(c_szFilename, "w");

	if (fp)
	{
		std::map<uint32_t, double>::iterator it;

		fprintf(fp,"MOB_VNUM\tCOUNT\n");

		for (it = m_mapRegenCount.begin(); it != m_mapRegenCount.end(); ++it)
		{
			fprintf(fp,"%u\t%g\n", it->first, it->second);
		}

		fclose(fp);
	}
}

uint32_t CMobManager::GetGroupFromGroupGroup(uint32_t dwVnum)
{
	std::map<uint32_t, CMobGroupGroup *>::iterator it = m_map_pMobGroupGroup.find(dwVnum);

	if (it == m_map_pMobGroupGroup.end())
		return 0;

	return it->second->GetMember();
}

CMobGroup * CMobManager::GetGroup(uint32_t dwVnum)
{
	std::map<uint32_t, CMobGroup *>::iterator it = m_map_pMobGroup.find(dwVnum);

	if (it == m_map_pMobGroup.end())
		return NULL;

	return it->second;
}

bool CMobManager::LoadGroupGroup(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int32_t iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			SysLog("LoadGroupGroup : Syntax error {} : no vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		CTokenVector* pTok;

		CMobGroupGroup* pGroup = M2_NEW CMobGroupGroup(iVnum);

		for (int32_t k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				uint32_t dwMobVnum = 0;
				str_to_number(dwMobVnum, pTok->at(0).c_str());

				int32_t prob = 1;
				if (pTok->size() > 1)
					str_to_number(prob, pTok->at(1).c_str());
				
				if (dwMobVnum)
					pGroup->AddMember(dwMobVnum);

				continue;
			}

			break;
		}

		loader.SetParentNode();

		m_map_pMobGroupGroup.insert(std::make_pair((uint32_t)iVnum, pGroup));
	}

	return true;
}

bool CMobManager::LoadGroup(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (uint32_t i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int32_t iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			SysLog("LoadGroup : Syntax error {} : no vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		CTokenVector* pTok;

		if (!loader.GetTokenVector("leader", &pTok))
		{
			SysLog("LoadGroup : Syntax error {} : no leader, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		if (pTok->size() < 2)
		{
			SysLog("LoadGroup : Syntax error {} : no leader vnum, node {}", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		CMobGroup* pGroup = M2_NEW CMobGroup;

		pGroup->Create(iVnum, stName);
		uint32_t vnum = 0;
		str_to_number(vnum, pTok->at(1).c_str());
		pGroup->AddMember(vnum);

		PyLog("GROUP: VNUM {} NAME {}", iVnum, stName.c_str());
		PyLog("               {} {}", pTok->at(0).c_str(), pTok->at(1).c_str());

		for (int32_t k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				PyLog("               {} {}", pTok->at(0).c_str(), pTok->at(1).c_str());
				uint32_t vnum = 0;
				str_to_number(vnum, pTok->at(1).c_str());
				pGroup->AddMember(vnum);
				continue;
			}

			break;
		}

		loader.SetParentNode();
		m_map_pMobGroup.insert(std::map<uint32_t, CMobGroup *>::value_type(iVnum, pGroup));
	}

	return true;
}

