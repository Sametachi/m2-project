#include "stdafx.h"
#include <Common/stl.h>

#include "constants.h"
#include "skill.h"
#include "char.h"

void CSkillProto::SetPointVar(const std::string& strName, double dVar)
{
	kPointPoly.SetVar(strName, dVar);
	kPointPoly2.SetVar(strName, dVar);
	kPointPoly3.SetVar(strName, dVar);
	kMasterBonusPoly.SetVar(strName, dVar);
}

void CSkillProto::SetDurationVar(const std::string& strName, double dVar)
{
	kDurationPoly.SetVar(strName, dVar);
	kDurationPoly2.SetVar(strName, dVar);
	kDurationPoly3.SetVar(strName, dVar);
}

void CSkillProto::SetSPCostVar(const std::string& strName, double dVar)
{
	kSPCostPoly.SetVar(strName, dVar);
	kGrandMasterAddSPCostPoly.SetVar(strName, dVar);
}

CSkillManager::CSkillManager()
{
}

CSkillManager::~CSkillManager()
{
	auto it = m_map_pSkillProto.begin();
	for (; it != m_map_pSkillProto.end(); ++it) {
		M2_DELETE(it->second);
	}
}

struct SPointOnType
{
	const char* c_pszName;
	int32_t		 iPointOn;
} kPointOnTypes[] = {
	{ "NONE",		POINT_NONE		},
	{ "MAX_HP",		POINT_MAX_HP		},
	{ "MAX_SP",		POINT_MAX_SP		},
	{ "HP_REGEN",	POINT_HP_REGEN		},
	{ "SP_REGEN",	POINT_SP_REGEN		},
	{ "BLOCK",		POINT_BLOCK		},
	{ "HP",		POINT_HP		},
	{ "SP",		POINT_SP		},
	{ "ATT_GRADE",	POINT_ATT_GRADE_BONUS	},
	{ "DEF_GRADE",	POINT_DEF_GRADE_BONUS	},
	{ "MAGIC_ATT_GRADE",POINT_MAGIC_ATT_GRADE_BONUS	},
	{ "MAGIC_DEF_GRADE",POINT_MAGIC_DEF_GRADE_BONUS	},
	{ "BOW_DISTANCE",	POINT_BOW_DISTANCE	},
	{ "MOV_SPEED",	POINT_MOV_SPEED		},
	{ "ATT_SPEED",	POINT_ATT_SPEED		},
	{ "POISON_PCT",	POINT_POISON_PCT	},
	{ "RESIST_RANGE",   POINT_RESIST_BOW	},
	//{ "RESIST_MELEE",	POINT_RESIST_MELEE	},
	{ "CASTING_SPEED",	POINT_CASTING_SPEED	},
	{ "REFLECT_MELEE",	POINT_REFLECT_MELEE	},
	{ "ATT_BONUS",	POINT_ATT_BONUS		},
	{ "DEF_BONUS",	POINT_DEF_BONUS		},
	{ "RESIST_NORMAL",	POINT_RESIST_NORMAL_DAMAGE },
	{ "DODGE",		POINT_DODGE		},
	{ "KILL_HP_RECOVER",POINT_KILL_HP_RECOVERY	},
	{ "KILL_SP_RECOVER",POINT_KILL_SP_RECOVER	},
	{ "HIT_HP_RECOVER",	POINT_HIT_HP_RECOVERY	},
	{ "HIT_SP_RECOVER",	POINT_HIT_SP_RECOVERY	},
	{ "CRITICAL",	POINT_CRITICAL_PCT	},
	{ "MANASHIELD",	POINT_MANASHIELD	},
	{ "SKILL_DAMAGE_BONUS", POINT_SKILL_DAMAGE_BONUS	},
	{ "NORMAL_HIT_DAMAGE_BONUS", POINT_NORMAL_HIT_DAMAGE_BONUS	},
	{ "\n",		POINT_NONE		},
};

int32_t FindPointType(const char* c_sz)
{
	for (int32_t i = 0; *kPointOnTypes[i].c_pszName != '\n'; ++i)
	{
		if (!strcasecmp(c_sz, kPointOnTypes[i].c_pszName))
			return kPointOnTypes[i].iPointOn;
	}
	return -1;
}

bool CSkillManager::Initialize(TSkillTable* pTab, int32_t iSize)
{
	char buf[1024];
	std::map<uint32_t, CSkillProto *> map_pSkillProto;

	TSkillTable * t = pTab;
	bool bError = false;

	for (int32_t i = 0; i < iSize; ++i, ++t)
	{
		CSkillProto* pProto = M2_NEW CSkillProto;

		pProto->dwVnum = t->dwVnum;
		strlcpy(pProto->szName, t->szName, sizeof(pProto->szName));
		pProto->dwType = t->bType;
		pProto->bMaxLevel = t->bMaxLevel;
		pProto->dwFlag = t->dwFlag;
		pProto->dwAffectFlag = t->dwAffectFlag;
		pProto->dwAffectFlag2 = t->dwAffectFlag2;

		pProto->bLevelStep = t->bLevelStep;
		pProto->bLevelLimit = t->bLevelLimit;
		pProto->iSplashRange = t->dwSplashRange;
		pProto->dwTargetRange = t->dwTargetRange;
		pProto->preSkillVnum = t->preSkillVnum;
		pProto->preSkillLevel = t->preSkillLevel;

		pProto->lMaxHit = t->lMaxHit;

		pProto->bSkillAttrType = t->bSkillAttrType;

		int32_t iIdx = FindPointType(t->szPointOn);

		if (iIdx < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn: %s", t->szName, t->szPointOn);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		pProto->bPointOn = iIdx;

		int32_t iIdx2 = FindPointType(t->szPointOn2);

		if (iIdx2 < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn2: %s", t->szName, t->szPointOn2);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		pProto->bPointOn2 = iIdx2;

		int32_t iIdx3 = FindPointType(t->szPointOn3);

		if (iIdx3 < 0)
		{
			if (t->szPointOn3[0] == 0)
			{
				iIdx3 = POINT_NONE;
			}
			else
			{
				snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn3: %s", t->szName, t->szPointOn3);
				SysLog("{}", buf);
				SendLog(buf);
				bError = true;
				M2_DELETE(pProto);
				continue;
			}
		}

		pProto->bPointOn3 = iIdx3;

		if (!pProto->kSplashAroundDamageAdjustPoly.Analyze(t->szSplashAroundDamageAdjustPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSplashAroundDamageAdjustPoly: %s", t->szName, t->szSplashAroundDamageAdjustPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kPointPoly.Analyze(t->szPointPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly: %s", t->szName, t->szPointPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kPointPoly2.Analyze(t->szPointPoly2))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly2: %s", t->szName, t->szPointPoly2);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kPointPoly3.Analyze(t->szPointPoly3))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly3: %s", t->szName, t->szPointPoly3);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kSPCostPoly.Analyze(t->szSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSPCostPoly: %s", t->szName, t->szSPCostPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kGrandMasterAddSPCostPoly.Analyze(t->szGrandMasterAddSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szGrandMasterAddSPCostPoly: %s", t->szName, t->szGrandMasterAddSPCostPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kDurationPoly.Analyze(t->szDurationPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly: %s", t->szName, t->szDurationPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kDurationPoly2.Analyze(t->szDurationPoly2))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly2: %s", t->szName, t->szDurationPoly2);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kDurationPoly3.Analyze(t->szDurationPoly3))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly3: %s", t->szName, t->szDurationPoly3);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kDurationSPCostPoly.Analyze(t->szDurationSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationSPCostPoly: %s", t->szName, t->szDurationSPCostPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		if (!pProto->kCooldownPoly.Analyze(t->szCooldownPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szCooldownPoly: %s", t->szName, t->szCooldownPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		PyLog("Master {}", t->szMasterBonusPoly);
		if (!pProto->kMasterBonusPoly.Analyze(t->szMasterBonusPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szMasterBonusPoly: %s", t->szName, t->szMasterBonusPoly);
			SysLog("{}", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pProto);
			continue;
		}

		PyLog("#%-3d %-24s type {} flag {} affect {} point_poly: {}", 
				pProto->dwVnum, pProto->szName, pProto->dwType, pProto->dwFlag, pProto->dwAffectFlag, t->szPointPoly);

		map_pSkillProto.insert(std::map<uint32_t, CSkillProto *>::value_type(pProto->dwVnum, pProto));
	}

	if (!bError)
	{
		auto it = m_map_pSkillProto.begin();

		while (it != m_map_pSkillProto.end()) {
			M2_DELETE(it->second);
			++it;
		}

		m_map_pSkillProto.clear();

		it = map_pSkillProto.begin();

		while (it != map_pSkillProto.end())
		{
			m_map_pSkillProto.insert(std::map<uint32_t, CSkillProto *>::value_type(it->first, it->second));
			++it;
		}

		SendLog("Skill Prototype reloaded!");
	}
	else
		SendLog("There were erros when loading skill table");

	return !bError;
}

CSkillProto * CSkillManager::Get(uint32_t dwVnum)
{
	std::map<uint32_t, CSkillProto *>::iterator it = m_map_pSkillProto.find(dwVnum);

	if (it == m_map_pSkillProto.end())
		return NULL;

	return it->second;
}

CSkillProto * CSkillManager::Get(const char* c_pszSkillName)
{
	std::map<uint32_t, CSkillProto *>::iterator it = m_map_pSkillProto.begin();

	while (it != m_map_pSkillProto.end())
	{
		if (!strcasecmp(it->second->szName, c_pszSkillName))
			return it->second;

		it++;
	}

	return NULL;
}

