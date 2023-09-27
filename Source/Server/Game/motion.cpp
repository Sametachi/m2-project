#include "stdafx.h"
#include <Common/stl.h>
#include "constants.h"
#include "motion.h"
#include "text_file_loader.h"
#include "mob_manager.h"
#include "char.h"

static float MSA_GetNormalAttackDuration(const char* msaPath)
{
	float duration = 99.0f;
	FILE * fp = fopen(msaPath, "rt");
	if (!fp)
		return duration;

	char line[1024];
	while (fgets(line, sizeof(line), fp))
	{
		char key[1024];
		char val[1024];

		sscanf(line, "%s %s", key, val);
		if (strcmp(key, "MotionDuration") == 0)
		{
			duration = atof(val);
			break;
		}
	}
	fclose(fp);

	return duration;
}

static float MOB_GetNormalAttackDuration(TMobTable* mobTable)
{
	float minDuration = 99.0f;

	const char* folder = mobTable->szFolder;

	char motlistPath[1024];
	snprintf(motlistPath, sizeof(motlistPath), "data/monster/%s/motlist.txt", folder);

	FILE * fp = fopen(motlistPath, "rt");
	if (!fp)
		return minDuration;

	char line[1024];
	while (fgets(line, sizeof(line), fp))
	{
		char mode[1024];
		char type[1024];
		char msaName[1024];
		int32_t percent;

		sscanf(line, "%s %s %s %d", mode, type, msaName, &percent);
		if (strcmp(mode, "GENERAL") == 0 && strncmp(type, "NORMAL_ATTACK", 13) == 0)
		{
			char msaPath[1024];
			snprintf(msaPath, sizeof(msaPath), "data/monster/%s/%s", folder, msaName);
			float curDuration = MSA_GetNormalAttackDuration(msaPath);
			if (curDuration < minDuration)
				minDuration = curDuration;
		}
	}
	fclose(fp);

	return minDuration;
}

static const char* GetMotionFileName(TMobTable* mobTable, EPublicMotion motion)
{
	char buf[1024];
	const char* folder = mobTable->szFolder;
	snprintf(buf, sizeof(buf), "data/monster/%s/motlist.txt", folder);

	FILE * fp = fopen(buf, "rt");
	char*  v[4];

	if (fp != nullptr)
	{
		const char* field = nullptr;

		switch (motion)
		{
			case MOTION_WALK				: field = "WALK"; break;
			case MOTION_RUN					: field = "RUN"; break;
			case MOTION_NORMAL_ATTACK		: field = "NORMAL_ATTACK"; break;
			case MOTION_SPECIAL_1			: field = "SPECIAL"; break;
			case MOTION_SPECIAL_2			: field = "SPECIAL1"; break;
			case MOTION_SPECIAL_3			: field = "SPECIAL2"; break;
			case MOTION_SPECIAL_4			: field = "SPECIAL3"; break;
			case MOTION_SPECIAL_5			: field = "SPECIAL4"; break;

			default:
				fclose(fp);
				SysLog("Motion: no process for this motion({}) vnum({})", motion, mobTable->dwVnum);
				return NULL;
		}

		while (fgets(buf, 1024, fp))
		{
			v[0] = strtok(buf,  " \t\r\n");
			v[1] = strtok(NULL, " \t\r\n");
			v[2] = strtok(NULL, " \t\r\n");
			v[3] = strtok(NULL, " \t\r\n");

			if (NULL != v[0] && NULL != v[1] && NULL != v[2] && NULL != v[3] && !strcasecmp(v[1], field))
			{
				fclose(fp);

				static std::string str;
				str = "data/monster/";
				str += folder;
				str += "/";
				str += v[2];

				return str.c_str();
			}
		}

		fclose(fp);
	}
	else
	{
		SysLog("Motion: {} have not motlist.txt vnum({}) folder({})", folder, mobTable->dwVnum, mobTable->szFolder);
	}

	return NULL;
}

static void LoadMotion(CMotionSet* pMotionSet, TMobTable* mob_table, EPublicMotion motion)
{
	const char* cpFileName = GetMotionFileName(mob_table, motion);

	if (cpFileName == nullptr)
	{
		return;
	}

	CMotion* pMotion = M2_NEW CMotion;

	if (pMotion->LoadFromFile(cpFileName))
	{
		if (motion == MOTION_RUN)
			if (0.0f == pMotion->GetAccumVector().y)
				SysLog("cannot find accumulation data in file '{}'", cpFileName);

		pMotionSet->Insert(MAKE_MOTION_KEY(MOTION_MODE_GENERAL, motion), pMotion);
	}
	else
	{
		M2_DELETE(pMotion);
		SysLog("Motion: Load failed vnum({}) motion({}) file({})", mob_table->dwVnum, motion, cpFileName);
	}
}

static void LoadSkillMotion(CMotionSet* pMotionSet, CMob* pMob, EPublicMotion motion)
{
	int32_t idx = 0;

	switch (motion)
	{
		case MOTION_SPECIAL_1 : idx = 0; break;
		case MOTION_SPECIAL_2 : idx = 1; break;
		case MOTION_SPECIAL_3 : idx = 2; break;
		case MOTION_SPECIAL_4 : idx = 3; break;
		case MOTION_SPECIAL_5 : idx = 4; break;

		default :
			return;					
	}

	TMobTable* mob_table = &pMob->m_table;

	if (mob_table->Skills[idx].dwVnum == 0) return;

	const char* cpFileName = GetMotionFileName(mob_table, motion);
	if (cpFileName == nullptr) return;

	CMotion* pMotion = M2_NEW CMotion;

	if (pMotion->LoadMobSkillFromFile(cpFileName, pMob, idx))
	{
		pMotionSet->Insert(MAKE_MOTION_KEY(MOTION_MODE_GENERAL, motion), pMotion);
	}
	else
	{
		if (mob_table->Skills[idx].dwVnum != 0)
		{
			SysLog("Motion: Skill exist but no motion data for index {} mob {} skill {}",
				   	idx, mob_table->dwVnum, mob_table->Skills[idx].dwVnum);
		}
		M2_DELETE(pMotion);
	}
}

CMotionManager::CMotionManager()
{
}

CMotionManager::~CMotionManager()
{
	iterator it = m_map_pMotionSet.begin();
	for (; it != m_map_pMotionSet.end(); ++it) {
		M2_DELETE(it->second);
	}
}

const CMotionSet * CMotionManager::GetMotionSet(uint32_t dwVnum)
{
	iterator it = m_map_pMotionSet.find(dwVnum);

	if (m_map_pMotionSet.end() == it)
		return NULL;

	return it->second;
}

const CMotion * CMotionManager::GetMotion(uint32_t dwVnum, uint32_t dwKey)
{
	const CMotionSet* pMotionSet = GetMotionSet(dwVnum);

	if (!pMotionSet)
		return NULL;

	return pMotionSet->GetMotion(dwKey);
}

float CMotionManager::GetMotionDuration(uint32_t dwVnum, uint32_t dwKey)
{
	const CMotion* pMotion = GetMotion(dwVnum, dwKey);
	return pMotion ? pMotion->GetDuration() : 0.0f;
}

float	CMotionManager::GetNormalAttackDuration(uint32_t dwVnum)
{
	std::map<uint32_t, float>::iterator f = m_map_normalAttackDuration.find(dwVnum);
	if (m_map_normalAttackDuration.end() == f)
		return 0.0f;
	else
		return f->second;
}

enum EMotionEventType
{
	MOTION_EVENT_TYPE_NONE,					// 0
	MOTION_EVENT_TYPE_EFFECT,				// 1
	MOTION_EVENT_TYPE_SCREEN_WAVING,		// 2
	MOTION_EVENT_TYPE_SCREEN_FLASHING,		// 3
	MOTION_EVENT_TYPE_SPECIAL_ATTACKING,	// 4
	MOTION_EVENT_TYPE_SOUND,				// 5
	MOTION_EVENT_TYPE_FLY,					// 6
	MOTION_EVENT_TYPE_CHARACTER_SHOW,		// 7
	MOTION_EVENT_TYPE_CHARACTER_HIDE,		// 8
	MOTION_EVENT_TYPE_WARP,					// 9
	MOTION_EVENT_TYPE_EFFECT_TO_TARGET,		// 10
	MOTION_EVENT_TYPE_MAX_NUM,				// 11
};

bool CMotionManager::Build()
{
	const char* c_apszFolderName[MAIN_RACE_MAX_NUM] =
	{
		"data/pc/warrior",
		"data/pc/assassin",
		"data/pc/sura",
		"data/pc/shaman",
		"data/pc2/warrior",
		"data/pc2/assassin",
		"data/pc2/sura",
		"data/pc2/shaman"
	};
	
	for (int32_t i = 0; i < MAIN_RACE_MAX_NUM; ++i)
	{
		CMotionSet* pMotionSet = M2_NEW CMotionSet;
		m_map_pMotionSet.insert(TContainer::value_type(i, pMotionSet));

		char sz[256];

		snprintf(sz, sizeof(sz), "%s/general/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_GENERAL, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/general/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_GENERAL, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/twohand_sword/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_TWOHAND_SWORD, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/twohand_sword/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_TWOHAND_SWORD, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/onehand_sword/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_ONEHAND_SWORD, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/onehand_sword/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_ONEHAND_SWORD, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/dualhand_sword/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_DUALHAND_SWORD, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/dualhand_sword/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_DUALHAND_SWORD, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/bow/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_BOW, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/bow/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_BOW, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/bell/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_BELL, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/bell/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_BELL, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/fan/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_FAN, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/fan/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_FAN, MOTION_WALK);

		snprintf(sz, sizeof(sz), "%s/horse/run.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_HORSE, MOTION_RUN);
		snprintf(sz, sizeof(sz), "%s/horse/walk.msa", c_apszFolderName[i]);
		pMotionSet->Load(sz, MOTION_MODE_HORSE, MOTION_WALK);
	}

	CMobManager::iterator it = CMobManager::GetInstance()->begin();

	while (it != CMobManager::GetInstance()->end())
	{
		CMob* pMob = (it++)->second;
		TMobTable * t = &pMob->m_table;

		if ('\0' != t->szFolder[0])
		{
			CMotionSet* pMotionSet = M2_NEW CMotionSet;
			m_map_pMotionSet.insert(TContainer::value_type(t->dwVnum, pMotionSet));

			LoadMotion(pMotionSet, t, MOTION_WALK);
			LoadMotion(pMotionSet, t, MOTION_RUN);
			LoadMotion(pMotionSet, t, MOTION_NORMAL_ATTACK);

			LoadSkillMotion(pMotionSet, pMob, MOTION_SPECIAL_1);
			LoadSkillMotion(pMotionSet, pMob, MOTION_SPECIAL_2);
			LoadSkillMotion(pMotionSet, pMob, MOTION_SPECIAL_3);
			LoadSkillMotion(pMotionSet, pMob, MOTION_SPECIAL_4);
			LoadSkillMotion(pMotionSet, pMob, MOTION_SPECIAL_5);

			float normalAttackDuration = MOB_GetNormalAttackDuration(t);
			PyLog("mob_normal_attack_duration:{}:{}:%.2f", t->dwVnum, t->szFolder, normalAttackDuration);
			m_map_normalAttackDuration.insert(std::map<uint32_t, float>::value_type(t->dwVnum, normalAttackDuration));
		}
	}

	return true;
}

CMotionSet::CMotionSet()
{
}

CMotionSet::~CMotionSet()
{
	iterator it = m_map_pMotion.begin();
	for (; it != m_map_pMotion.end(); ++it) {
		M2_DELETE(it->second);
	}
}

const CMotion * CMotionSet::GetMotion(uint32_t dwKey) const
{
	const_iterator it = m_map_pMotion.find(dwKey);

	if (it == m_map_pMotion.end())
		return NULL;

	return it->second;
}

void CMotionSet::Insert(uint32_t dwKey, CMotion* pMotion)
{
	m_map_pMotion.insert(TContainer::value_type(dwKey, pMotion));
}

bool CMotionSet::Load(const char* szFileName, int32_t mode, int32_t motion)
{
	CMotion* pMotion = M2_NEW CMotion;

	if (!pMotion->LoadFromFile(szFileName))
	{
		M2_DELETE(pMotion);
		return false;
	}

	Insert(MAKE_MOTION_KEY(mode, motion), pMotion);

	return true;
}

CMotion::CMotion() : m_isEmpty(true), m_fDuration(0.0f), m_isAccumulation(false)
{
	m_vec3Accumulation.x = 0.0f;
	m_vec3Accumulation.y = 0.0f;
	m_vec3Accumulation.z = 0.0f;
}  

CMotion::~CMotion()
{
}

bool CMotion::LoadMobSkillFromFile(const char* c_pszFileName, CMob* pMob, int32_t iSkillIndex)
{
	CTextFileLoader rkTextFileLoader;
	if (!rkTextFileLoader.Load(c_pszFileName))
		return false;

	rkTextFileLoader.SetTop();

	if (!rkTextFileLoader.GetTokenFloat("motionduration", &m_fDuration))
	{
		SysLog("Motion: no motion duration {}", c_pszFileName);
		return false;
	}

	std::string strNodeName;
	for (uint32_t i = 0; i < rkTextFileLoader.GetChildNodeCount(); ++i)
	{
		rkTextFileLoader.SetChildNode(i);

		rkTextFileLoader.GetCurrentNodeName(&strNodeName);

		if (0 == strNodeName.compare("motioneventdata"))
		{
			uint32_t dwMotionEventDataCount;

			if (!rkTextFileLoader.GetTokenDoubleWord("motioneventdatacount", &dwMotionEventDataCount))
				continue;

			for (uint32_t j = 0; j < dwMotionEventDataCount; ++j)
			{
				if (!rkTextFileLoader.SetChildNode("event", j))
				{
					SysLog("Motion: no event data {} {}", j, c_pszFileName);
					return false;
				}

				int32_t iType;
				if (!rkTextFileLoader.GetTokenInteger("motioneventtype", &iType))
				{
					SysLog("Motion: no motioneventtype data {}", c_pszFileName);
					return false;
				}

				D3DVECTOR v3Position;

				switch (iType)
				{
					case MOTION_EVENT_TYPE_FLY:
					case MOTION_EVENT_TYPE_EFFECT:
					case MOTION_EVENT_TYPE_SCREEN_WAVING:
					case MOTION_EVENT_TYPE_SOUND:
					case MOTION_EVENT_TYPE_CHARACTER_SHOW:
					case MOTION_EVENT_TYPE_CHARACTER_HIDE:
					case MOTION_EVENT_TYPE_WARP:
					case MOTION_EVENT_TYPE_EFFECT_TO_TARGET:
						rkTextFileLoader.SetParentNode();
						continue;

					case MOTION_EVENT_TYPE_SPECIAL_ATTACKING:
						if (!rkTextFileLoader.SetChildNode("spheredata", 0))
						{
							SysLog("Motion: no sphere data {}", c_pszFileName);
							return false;
						}

						if (!rkTextFileLoader.GetTokenPosition("position", &v3Position))
						{
							SysLog("Motion: no position data {}", c_pszFileName);
							return false;
						}

						rkTextFileLoader.SetParentNode();
						break;
					default:
						assert(!" CRaceMotionData::LoadMotionData - Strange Event Type");
						return false;
						break;
				}

				float fStartingTime;

				if (!rkTextFileLoader.GetTokenFloat("startingtime", &fStartingTime))
				{
					SysLog("Motion: no startingtime data {}", c_pszFileName);
					return false;
				}

				pMob->AddSkillSplash(iSkillIndex, 100 + uint32_t(fStartingTime * 1000), -(int32_t)(v3Position.y));

				rkTextFileLoader.SetParentNode();
			}
		}
		rkTextFileLoader.SetParentNode();
	}

	pMob->m_mobSkillInfo[iSkillIndex].dwSkillVnum = pMob->m_table.Skills[iSkillIndex].dwVnum;
	pMob->m_mobSkillInfo[iSkillIndex].bSkillLevel = pMob->m_table.Skills[iSkillIndex].bLevel;
	m_isEmpty = false;
	return true;
}

bool CMotion::LoadFromFile(const char* c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
	{
		PyLog("Motion: LoadFromFile fail: {}", c_pszFileName);
		return false;
	}

	if (!loader.GetTokenFloat("motionduration", &m_fDuration))
	{
		SysLog("Motion: {} does not have a duration", c_pszFileName);
		return false;
	}

	if (loader.GetTokenPosition("accumulation", &m_vec3Accumulation))
		m_isAccumulation = true;

	TraceLog("{}s {} {}", strchr(c_pszFileName, '/') + 1, GetDuration(), GetAccumVector().y);

	m_isEmpty = false;
	return true;
}

float CMotion::GetDuration() const
{
	return m_fDuration;
}

const D3DVECTOR & CMotion::GetAccumVector() const
{
	return m_vec3Accumulation;
}

bool CMotion::IsEmpty()
{
	return m_isEmpty;
}

