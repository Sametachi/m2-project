#include "StdAfx.h"
#include "PythonSkill.h"

#include <Core/Poly.h>
#include "InstanceBase.h"
#include "PythonPlayer.h"
#include <Core/Race/RaceConstans.hpp>

std::map<std::string, uint32_t> CPythonSkill::SSkillData::ms_StatusNameMap;
std::map<std::string, uint32_t> CPythonSkill::SSkillData::ms_NewMinStatusNameMap;
std::map<std::string, uint32_t> CPythonSkill::SSkillData::ms_NewMaxStatusNameMap;
uint32_t CPythonSkill::SSkillData::ms_dwTimeIncreaseSkillNumber = 0;

int32_t SplitLine(const char* c_szText, CTokenVector* pstTokenVector, const char* c_szDelimeter)
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();

	std::string stLine = c_szText;

	uint32_t basePos = 0;

	do
	{
		int32_t beginPos = stLine.find_first_not_of(c_szDelimeter, basePos);

		if (beginPos < 0)
			return -1;

		int32_t endPos;

		if (stLine[beginPos] == '"')
		{
			++beginPos;
			endPos = stLine.find_first_of('\"', beginPos);

			if (endPos < 0)
				return -2;

			basePos = endPos + 1;
		}
		else
		{
			endPos = stLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->emplace_back(stLine.substr(beginPos, static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(endPos) - beginPos));

		if (int32_t(stLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} while (basePos < stLine.length());

	return 0;
}

// OVERWRITE_SKILLPROTO_POLY
void string_replace_word(const char* base, int32_t base_len, const char* src, int32_t src_len, const char* dst, int32_t dst_len, std::string& result)
{
	result.resize(0);
	if (base_len > 0 && dst_len > src_len)
		result.reserve(base_len + (static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(dst_len) - src_len) * (base_len / src_len));
	else
		result.reserve(base_len);

	int32_t prev = 0;
	int32_t cur = 0;
	while (cur < base_len)
	{
		if (memcmp(base + cur, src, src_len) == 0)
		{
			result.append(base + prev, static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(cur) - prev);
			result.append(dst, dst_len);
			cur += src_len;
			prev = cur;
		}
		else
		{
			++cur;
		}
	}
	result.append(base + prev, static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(cur) - prev);
}
// END_OF_OVERWRITE_SKILLPROTO_POLY

bool CPythonSkill::RegisterSkillTable(const char* c_szFileName)
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(vfs_string.value());

	// OVERWRITE_SKILLPROTO_POLY
	std::string src_poly_rand;
	std::string src_poly_atk;
	std::string src_poly_mwep;
	// END_OF_OVERWRITE_SKILLPROTO_POLY

	CTokenVector TokenVector;
	for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		std::string strLine = textFileLoader.GetLineString(i);

		if (TABLE_TOKEN_TYPE_MAX_NUM != TokenVector.size())
		{
			TraceLog("CPythonSkill::RegisterSkillTable({}) - Strange Token Count [Line:{} / TokenCount:{}]\n", c_szFileName, i, TokenVector.size());
			continue;
		}

		int32_t iVnum = atoi(TokenVector[TABLE_TOKEN_TYPE_VNUM].c_str());

		TSkillDataMap::iterator itor = m_SkillDataMap.find(iVnum);
		if (m_SkillDataMap.end() == itor)
		{
			TraceLog("CPythonSkill::RegisterSkillTable({}) - NOT EXIST SkillDesc [Vnum:{} Line:{}]\n", c_szFileName, iVnum, i);
			continue;
		}

		TSkillData& rSkillData = itor->second;

		const std::string& c_strSPCostPoly = TokenVector[TABLE_TOKEN_TYPE_SP_COST_POLY];
		if (!c_strSPCostPoly.empty())
		{
			rSkillData.strNeedSPFormula = c_strSPCostPoly;
		}

		const std::string& c_strCooldownPoly = TokenVector[TABLE_TOKEN_TYPE_COOLDOWN_POLY];
		if (!c_strCooldownPoly.empty())
		{
			rSkillData.strCoolTimeFormula = c_strCooldownPoly;
		}

		const std::string& c_strDurationSPCostPoly = TokenVector[TABLE_TOKEN_TYPE_DURATION_SP_COST_POLY];
		if (!c_strDurationSPCostPoly.empty())
		{
			rSkillData.strContinuationSPFormula = c_strDurationSPCostPoly;
		}

		const std::string& c_strTargetRange = TokenVector[TABLE_TOKEN_TYPE_TARGET_RANGE];
		if (!c_strTargetRange.empty())
		{
			rSkillData.dwTargetRange = atoi(c_strTargetRange.c_str());
		}

		rSkillData.strDuration = TokenVector[TABLE_TOKEN_TYPE_DURATION_POLY];


		const uint32_t LevelEmpty = 1;

		const std::string& c_strMaxLevel = TokenVector[TABLE_TOKEN_TYPE_MAX_LEVEL];
		if (!c_strMaxLevel.empty())
		{
			int32_t maxLevel = atoi(c_strMaxLevel.c_str());
			if (maxLevel > LevelEmpty)
				rSkillData.byMaxLevel = maxLevel;
		}

		const std::string& c_strLevelLimit = TokenVector[TABLE_TOKEN_TYPE_LEVEL_LIMIT];
		if (!c_strLevelLimit.empty())
		{
			int32_t levelLimit = atoi(c_strLevelLimit.c_str());
			if (rSkillData.byLevelLimit > LevelEmpty)
				rSkillData.byLevelLimit = levelLimit;
		}
		const std::string& c_strPointPoly = TokenVector[TABLE_TOKEN_TYPE_POINT_POLY];

		// OVERWRITE_SKILLPROTO_POLY
		bool USE_SKILL_PROTO = true;

		switch (iVnum)
		{
		case 34:

			// GUILD_SKILL_DISPLAY_BUG_FIX
		case 151:
		case 152:
		case 153:
		case 154:
		case 155:
		case 156:
		case 157:
			// END_OF_GUILD_SKILL_DISPLAY_BUG_FIX
			USE_SKILL_PROTO = false;
			break;
		}

		if (!rSkillData.AffectDataVector.empty() && USE_SKILL_PROTO)
		{
			TAffectData& affect = rSkillData.AffectDataVector[0];

			if (strstr(c_strPointPoly.c_str(), "atk") != NULL ||
				strstr(c_strPointPoly.c_str(), "mwep") != NULL ||
				strstr(c_strPointPoly.c_str(), "number") != NULL)
			{
				src_poly_rand = "";
				src_poly_atk = "";
				src_poly_mwep = "";

				// MIN
				string_replace_word(c_strPointPoly.c_str(), c_strPointPoly.length(),
					"number", 6, "min", 3, src_poly_rand);
				string_replace_word(src_poly_rand.c_str(), src_poly_rand.length(),
					"atk", 3, "minatk", 6, src_poly_atk);
				string_replace_word(src_poly_atk.c_str(), src_poly_atk.length(),
					"mwep", 4, "minmwep", 7, affect.strAffectMinFormula);
				// END_OF_MIN

				// MAX
				string_replace_word(c_strPointPoly.c_str(), c_strPointPoly.length(),
					"number", 6, "max", 3, src_poly_rand);
				string_replace_word(src_poly_rand.c_str(), src_poly_rand.length(),
					"atk", 3, "maxatk", 6, src_poly_atk);
				string_replace_word(src_poly_atk.c_str(), src_poly_atk.length(),
					"mwep", 4, "maxmwep", 7, affect.strAffectMaxFormula);
				// END_OF_MAX

				switch (iVnum)
				{
				case 1: // �￬��
					affect.strAffectMinFormula += "* 3";
					affect.strAffectMaxFormula += "* 3";
					break;
				}

			}
			else
			{
				affect.strAffectMinFormula = c_strPointPoly;
				affect.strAffectMaxFormula = "";
			}
		}
		// END_OF_OVERWRITE_SKILLPROTO_POLY
	}

	return true;
}

void CPythonSkill::__RegisterGradeIconImage(TSkillData& rData, const char* c_szHeader, const char* c_szImageName)
{
	for (int32_t j = 0; j < SKILL_GRADE_COUNT; ++j)
	{
		TGradeData& rGradeData = rData.GradeData[j];

		char szCount[8 + 1];
		_snprintf_s(szCount, sizeof(szCount), "_%02d", j + 1);

		std::string strFileName = "";
		strFileName += c_szHeader;
		strFileName += c_szImageName;
		strFileName += szCount;
		strFileName += ".sub";
		rGradeData.pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strFileName.c_str());
	}
}

void CPythonSkill::__RegisterNormalIconImage(TSkillData& rData, const char* c_szHeader, const char* c_szImageName)
{
	std::string strFileName = "";
	strFileName += c_szHeader;
	strFileName += c_szImageName;
	strFileName += ".sub";
	rData.pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strFileName.c_str());
	for (int32_t j = 0; j < SKILL_GRADE_COUNT; ++j)
	{
		TGradeData& rGradeData = rData.GradeData[j];
		rGradeData.pImage = rData.pImage;
	}
}

bool CPythonSkill::RegisterSkillDesc(const char* c_szFileName)
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(vfs_string.value());

	CTokenVector TokenVector;
	for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		if (DESC_TOKEN_TYPE_VNUM >= TokenVector.size())
		{
			TraceLog("SkillDesc.line({}).NO_VNUM_ERROR\n", i + 1);
			continue;
		}

		uint32_t iSkillIndex = atoi(TokenVector[DESC_TOKEN_TYPE_VNUM].c_str());
		if (iSkillIndex == 0)
		{
			TraceLog("SkillDesc.line({}).NO_INDEX_ERROR\n", i + 1);
			continue;
		}

		if (DESC_TOKEN_TYPE_JOB >= TokenVector.size())
		{
			TraceLog("SkillDesc.line({}).NO_JOB_ERROR\n", i + 1);
			continue;
		}

		m_SkillDataMap.insert(std::make_pair(iSkillIndex, TSkillData()));
		TSkillData& rSkillData = m_SkillDataMap[iSkillIndex];

		// Vnum
		rSkillData.dwSkillIndex = iSkillIndex;

		// Type
		{
			std::string strJob = TokenVector[DESC_TOKEN_TYPE_JOB];
			stl_lowers(strJob);

			std::map<std::string, uint32_t>::iterator it = m_SkillTypeIndexMap.find(strJob.c_str());
			if (m_SkillTypeIndexMap.end() == it)
			{
				TraceLog("CPythonSkill::RegisterSkillDesc(dwSkillIndex={}, strType={}).STRANGE_SKILL_TYPE", iSkillIndex, strJob.c_str());
				continue;
			}

			rSkillData.byType = uint8_t(it->second);
		}

		// Name
		{
			rSkillData.strName = TokenVector[DESC_TOKEN_TYPE_NAME1];
			rSkillData.GradeData[0].strName = TokenVector[DESC_TOKEN_TYPE_NAME1];
			rSkillData.GradeData[1].strName = TokenVector[DESC_TOKEN_TYPE_NAME2];
			rSkillData.GradeData[2].strName = TokenVector[DESC_TOKEN_TYPE_NAME3];

			if (rSkillData.GradeData[1].strName.empty())
				rSkillData.GradeData[1].strName = TokenVector[DESC_TOKEN_TYPE_NAME1];
			if (rSkillData.GradeData[2].strName.empty())
				rSkillData.GradeData[2].strName = TokenVector[DESC_TOKEN_TYPE_NAME1];
		}

		// Description
		{
			rSkillData.strDescription = TokenVector[DESC_TOKEN_TYPE_DESCRIPTION];
		}

		// Condition
		{
			rSkillData.ConditionDataVector.clear();
			for (int32_t j = 0; j < CONDITION_COLUMN_COUNT; ++j)
			{
				const std::string& c_rstrCondition = TokenVector[DESC_TOKEN_TYPE_CONDITION1 + static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(j)];
				if (!c_rstrCondition.empty())
					rSkillData.ConditionDataVector.push_back(c_rstrCondition);
			}
		}

		// Attribute
		{
			const std::string& c_rstrAttribute = TokenVector[DESC_TOKEN_TYPE_ATTRIBUTE];
			CTokenVector AttributeTokenVector;
			SplitLine(c_rstrAttribute.c_str(), &AttributeTokenVector, "|");

			for (CTokenVector::iterator it = AttributeTokenVector.begin(); it != AttributeTokenVector.end(); ++it)
			{
				std::string& rstrToken = *it;
				std::map<std::string, uint32_t>::iterator it2 = m_SkillAttributeIndexMap.find(rstrToken);
				if (m_SkillAttributeIndexMap.end() == it2)
				{
					TraceLog("CPythonSkill::RegisterSkillDesc(dwSkillIndex={}) - Strange Skill Attribute(%s)", iSkillIndex, rstrToken.c_str());
					continue;
				}
				rSkillData.dwSkillAttribute |= it2->second;
			}
		}

		// Weapon
		{
			const std::string& c_rstrWeapon = TokenVector[DESC_TOKEN_TYPE_WEAPON];
			CTokenVector WeaponTokenVector;
			SplitLine(c_rstrWeapon.c_str(), &WeaponTokenVector, "|");

			for (CTokenVector::iterator it = WeaponTokenVector.begin(); it != WeaponTokenVector.end(); ++it)
			{
				std::string& rstrToken = *it;
				std::map<std::string, uint32_t>::iterator it2 = m_SkillNeedWeaponIndexMap.find(rstrToken);
				if (m_SkillNeedWeaponIndexMap.end() == it2)
				{
					TraceLog("CPythonSkill::RegisterSkillDesc(dwSkillIndex={}) - Strange Skill Need Weapon({})", iSkillIndex, rstrToken.c_str());
					continue;
				}
				rSkillData.dwNeedWeapon |= it2->second;
			}
		}

		// Icon Name
		{
			const std::string& c_rstrJob = TokenVector[DESC_TOKEN_TYPE_JOB];
			const std::string& c_rstrIconName = TokenVector[DESC_TOKEN_TYPE_ICON_NAME];

			// NOTE : Support ��ų�ӿ��� 3�ܰ踦 �����ؾ� �ؼ� ���� ó�� - [levites]
			if (c_iSkillIndex_Riding == iSkillIndex)
			{
				char szIconFileNameHeader[64 + 1];
				_snprintf(szIconFileNameHeader, sizeof(szIconFileNameHeader), "%sskill/common/support/", g_strImagePath.c_str());

				__RegisterGradeIconImage(rSkillData, szIconFileNameHeader, c_rstrIconName.c_str());
			}
			else if (m_PathNameMap.end() != m_PathNameMap.find(c_rstrJob))
			{
				char szIconFileNameHeader[64 + 1];
				_snprintf(szIconFileNameHeader, sizeof(szIconFileNameHeader), "%sskill/%s/", g_strImagePath.c_str(), m_PathNameMap[c_rstrJob].c_str());

				switch (rSkillData.byType)
				{
				case SKILL_TYPE_ACTIVE:
				{
					__RegisterGradeIconImage(rSkillData, szIconFileNameHeader, c_rstrIconName.c_str());
					break;
				}
				case SKILL_TYPE_SUPPORT:
				case SKILL_TYPE_GUILD:
				case SKILL_TYPE_HORSE:
				{
					__RegisterNormalIconImage(rSkillData, szIconFileNameHeader, c_rstrIconName.c_str());
					break;
				}
				}
			}
		}

		// Motion Index
		if (TokenVector.size() > DESC_TOKEN_TYPE_MOTION_INDEX)
		{
			const std::string& c_rstrMotionIndex = TokenVector[DESC_TOKEN_TYPE_MOTION_INDEX];
			if (c_rstrMotionIndex.empty())
			{
				rSkillData.bNoMotion = TRUE;
				rSkillData.wMotionIndex = 0;
			}
			else
			{
				rSkillData.bNoMotion = FALSE;
				rSkillData.wMotionIndex = (uint16_t)atoi(c_rstrMotionIndex.c_str());
			}
		}
		else
		{
			rSkillData.wMotionIndex = 0;
		}

		if (TokenVector.size() > DESC_TOKEN_TYPE_TARGET_COUNT_FORMULA)
			rSkillData.strTargetCountFormula = TokenVector[DESC_TOKEN_TYPE_TARGET_COUNT_FORMULA].c_str();
		if (TokenVector.size() > DESC_TOKEN_TYPE_MOTION_LOOP_COUNT_FORMULA)
			rSkillData.strMotionLoopCountFormula = TokenVector[DESC_TOKEN_TYPE_MOTION_LOOP_COUNT_FORMULA].c_str();

		rSkillData.AffectDataNewVector.clear();
		rSkillData.AffectDataNewVector.reserve(3);

		// Affect
		for (int32_t j = 0; j < AFFECT_COLUMN_COUNT; ++j)
		{
			int32_t iDescriptionSlotIndex = DESC_TOKEN_TYPE_AFFECT_DESCRIPTION_1 + j * AFFECT_STEP_COUNT;
			int32_t iMinSlotIndex = DESC_TOKEN_TYPE_AFFECT_MIN_1 + j * AFFECT_STEP_COUNT;
			int32_t iMaxSlotIndex = DESC_TOKEN_TYPE_AFFECT_MAX_1 + j * AFFECT_STEP_COUNT;

			if (TokenVector.size() > iDescriptionSlotIndex)
				if (!TokenVector[iDescriptionSlotIndex].empty())
				{
					rSkillData.AffectDataVector.push_back(TAffectData());

					TAffectData& rAffectData = *rSkillData.AffectDataVector.rbegin();
					rAffectData.strAffectDescription = "";
					rAffectData.strAffectMinFormula = "";
					rAffectData.strAffectMaxFormula = "";

					rAffectData.strAffectDescription = TokenVector[iDescriptionSlotIndex];
					if (TokenVector.size() > iMinSlotIndex)
						if (!TokenVector[iMinSlotIndex].empty())
							rAffectData.strAffectMinFormula = TokenVector[iMinSlotIndex];
					if (TokenVector.size() > iMaxSlotIndex)
						if (!TokenVector[iMaxSlotIndex].empty())
							rAffectData.strAffectMaxFormula = TokenVector[iMaxSlotIndex];
				}
		}

		if (TokenVector.size() > DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM)
		{
			int32_t numGrade = atoi(TokenVector[DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM].c_str());
			if (SKILL_EFFECT_COUNT < numGrade)
			{
				TraceLog("{}[{}] �� ��� ����[{}]�� �Ѿ���ϴ�.", rSkillData.strName.c_str(), TokenVector[DESC_TOKEN_TYPE_MOTION_INDEX_GRADE_NUM].c_str(), SKILL_EFFECT_COUNT);
				return false;
			}
			for (int32_t iGrade = 0; iGrade < numGrade; iGrade++)
			{
				//if (iGrade == SKILL_GRADE_COUNT)
				//	rSkillData.GradeData[iGrade] = rSkillData.GradeData[iGrade-1];
				rSkillData.GradeData[iGrade].wMotionIndex = rSkillData.wMotionIndex + iGrade * SKILL_GRADEGAP;
			}
			for (int32_t iSpace = numGrade; iSpace < SKILL_EFFECT_COUNT; iSpace++)
			{
				rSkillData.GradeData[iSpace].wMotionIndex = rSkillData.wMotionIndex;
			}
		}

		if (TokenVector.size() > DESC_TOKEN_TYPE_LEVEL_LIMIT)
		{
			const std::string& c_rstrLevelLimit = TokenVector[DESC_TOKEN_TYPE_LEVEL_LIMIT];
			if (c_rstrLevelLimit.empty())
				rSkillData.byLevelLimit = 0;
			else
				rSkillData.byLevelLimit = (uint16_t)atoi(c_rstrLevelLimit.c_str());
		}

		if (TokenVector.size() > DESC_TOKEN_TYPE_MAX_LEVEL)
		{
			const std::string& c_rstrMaxLevel = TokenVector[DESC_TOKEN_TYPE_MAX_LEVEL];
			const uint32_t LevelLimitEmpty = 1;
			if (c_rstrMaxLevel.empty())
			{
				if (rSkillData.byLevelLimit > LevelLimitEmpty)
					rSkillData.byMaxLevel = rSkillData.byLevelLimit;
				else
					rSkillData.byMaxLevel = 20;
			}
			else
				rSkillData.byMaxLevel = (uint8_t)atoi(c_rstrMaxLevel.c_str());
		}
	}

	return true;
}

void CPythonSkill::Destroy()
{
	m_SkillDataMap.clear();
}

bool CPythonSkill::RegisterSkill(uint32_t dwSkillIndex, const char* c_szFileName)
{
	CTextFileLoader TextFileLoader;
	if (!TextFileLoader.Load(c_szFileName))
	{
		SysLog("CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1}) - Failed to find file", dwSkillIndex, c_szFileName);
		return false;
	}

	TextFileLoader.SetTop();

	TSkillData SkillData;
	SkillData.dwSkillIndex = dwSkillIndex;

	std::string strTypeName;
	if (TextFileLoader.GetTokenString("type", &strTypeName))
	{
		stl_lowers(strTypeName);
		auto it = m_SkillTypeIndexMap.find(strTypeName);
		if (m_SkillTypeIndexMap.end() == it)
		{
			SysLog("Strange Skill Type - CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1})", dwSkillIndex, c_szFileName);
			return false;
		}

		SkillData.byType = (uint8_t)it->second;
	}
	else
	{
		SysLog("CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1}) - Failed to find key [type]", dwSkillIndex, c_szFileName);
		return false;
	}

	// Attribute
	CTokenVector* pAttributeTokenVector;
	if (TextFileLoader.GetTokenVector("attribute", &pAttributeTokenVector))
	{
		for (CTokenVector::iterator it = pAttributeTokenVector->begin(); it != pAttributeTokenVector->end(); ++it)
		{
			std::string& rstrToken = *it;
			std::map<std::string, uint32_t>::iterator it2 = m_SkillAttributeIndexMap.find(rstrToken.c_str());
			if (m_SkillAttributeIndexMap.end() == it2)
			{
				SysLog("Strange Skill Attribute - CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1})", dwSkillIndex, c_szFileName);
				continue;
			}
			SkillData.dwSkillAttribute |= it2->second;
		}
	}

	// NEED WEAPON BRO?
	CTokenVector* pNeedWeaponVector;
	if (TextFileLoader.GetTokenVector("needweapon", &pNeedWeaponVector))
	{
		for (CTokenVector::iterator it = pNeedWeaponVector->begin(); it != pNeedWeaponVector->end(); ++it)
		{
			std::string& rstrToken = *it;
			std::map<std::string, uint32_t>::iterator it2 = m_SkillNeedWeaponIndexMap.find(rstrToken.c_str());
			if (m_SkillNeedWeaponIndexMap.end() == it2)
			{
				SysLog("Incorrect skill weapon limitation - CPythonSkill::RegisterSkill(skill vnum={0}, c_szFileName={1})", dwSkillIndex, c_szFileName);
				continue;
			}
			SkillData.dwNeedWeapon |= it2->second;
		}
	}

	if (!TextFileLoader.GetTokenString("name", &SkillData.strName))
	{
		SysLog("CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1}) - Failed to find [name]", dwSkillIndex, c_szFileName);
		return false;
	}

	TextFileLoader.GetTokenString("description", &SkillData.strDescription);

	if (!TextFileLoader.GetTokenString("iconfilename", &SkillData.strIconFileName))
	{
		SysLog("CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1}) - Failed to find [iconfilename]", dwSkillIndex, c_szFileName);
		return false;
	}

	// ConditionData
	CTokenVector* pConditionDataVector;
	if (TextFileLoader.GetTokenVector("conditiondata", &pConditionDataVector))
	{
		uint32_t dwSize = pConditionDataVector->size();
		SkillData.ConditionDataVector.clear();
		SkillData.ConditionDataVector.resize(dwSize);
		for (uint32_t i = 0; i < dwSize; ++i)
		{
			SkillData.ConditionDataVector[i] = pConditionDataVector->at(i);
		}
	}

	// AffectData
	CTokenVector* pAffectDataVector;
	if (TextFileLoader.GetTokenVector("affectdata", &pAffectDataVector))
	{
		uint32_t dwSize = pAffectDataVector->size() / 3;
		SkillData.AffectDataVector.clear();
		SkillData.AffectDataVector.resize(dwSize);
		for (uint32_t i = 0; i < dwSize; ++i)
		{
			SkillData.AffectDataVector[i].strAffectDescription = pAffectDataVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(i) * 3 + 0);
			SkillData.AffectDataVector[i].strAffectMinFormula = pAffectDataVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(i) * 3 + 1);
			SkillData.AffectDataVector[i].strAffectMaxFormula = pAffectDataVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(i) * 3 + 2);
		}
	}

	// GradeData
	CTokenVector* pGradeDataVector;
	if (TextFileLoader.GetTokenVector("gradedata", &pGradeDataVector))
	{
		if (static_cast<uint64_t>(SKILL_GRADE_COUNT) * 2 != pGradeDataVector->size())
			SysLog("CPythonSkill::RegisterSkill(dwSkillIndex={0}, c_szFileName={1}) - Strange Grade Data Count", dwSkillIndex, c_szFileName);

		for (uint32_t i = 0; i < std::min<uint32_t>(SKILL_GRADE_COUNT, pGradeDataVector->size() / 2); ++i)
		{
			SkillData.GradeData[i].strName = pGradeDataVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(i) * 2 + 0);
			std::string strIconFileName = g_strImagePath + pGradeDataVector->at(static_cast<std::vector<std::string, std::allocator<std::string>>::size_type>(i) * 2 + 1);
			SkillData.GradeData[i].pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(strIconFileName);
		}
	}

	TextFileLoader.GetTokenString("cooltimeformula", &SkillData.strCoolTimeFormula);
	TextFileLoader.GetTokenString("targetcountformula", &SkillData.strTargetCountFormula);
	TextFileLoader.GetTokenString("motionloopcountformula", &SkillData.strMotionLoopCountFormula);
	TextFileLoader.GetTokenString("needspformula", &SkillData.strNeedSPFormula);
	TextFileLoader.GetTokenString("continuationspformula", &SkillData.strContinuationSPFormula);
	TextFileLoader.GetTokenDoubleWord("range", &SkillData.dwTargetRange);
	TextFileLoader.GetTokenByte("maxlevel", &SkillData.byMaxLevel);
	TextFileLoader.GetTokenByte("leveluppoint", &SkillData.byLevelUpPoint);

	uint16_t wMotionIndex;
	if (TextFileLoader.GetTokenWord("motionindex", &wMotionIndex))
		SkillData.wMotionIndex = wMotionIndex;
	else
		SkillData.wMotionIndex = 0;

	uint16_t wMotionIndexForMe;
	if (TextFileLoader.GetTokenWord("motionindexforme", &wMotionIndexForMe))
		SkillData.wMotionIndexForMe = wMotionIndexForMe;
	else
		SkillData.wMotionIndexForMe = 0;

	SkillData.strIconFileName = g_strImagePath + SkillData.strIconFileName;
	SkillData.pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(SkillData.strIconFileName);

	m_SkillDataMap.emplace(dwSkillIndex, SkillData);

	/////

	if (SkillData.IsTimeIncreaseSkill())
		SSkillData::ms_dwTimeIncreaseSkillNumber = SkillData.dwSkillIndex;

	return true;
}

bool CPythonSkill::GetSkillData(uint32_t dwSkillIndex, TSkillData** ppSkillData)
{
	auto it = m_SkillDataMap.find(dwSkillIndex);

	if (m_SkillDataMap.end() == it)
		return FALSE;

	*ppSkillData = &(it->second);
	return TRUE;
}

bool CPythonSkill::GetSkillDataByName(const char* c_szName, TSkillData** ppSkillData)
{
	auto itor = m_SkillDataMap.begin();
	for (; itor != m_SkillDataMap.end(); ++itor)
	{
		TSkillData* pData = &(itor->second);
		if (0 == pData->strName.compare(c_szName))
		{
			*ppSkillData = &(itor->second);
			return true;
		}
	}
	return false;
}

void CPythonSkill::SetPathName(const char* c_szFileName)
{
	m_strPathName = c_szFileName;
}

const char* CPythonSkill::GetPathName()
{
	return m_strPathName.c_str();
}

CPythonSkill::CPythonSkill()
{
	m_SkillTypeIndexMap.emplace("none", SKILL_TYPE_NONE);
	m_SkillTypeIndexMap.emplace("active", SKILL_TYPE_ACTIVE);
	m_SkillTypeIndexMap.emplace("support", SKILL_TYPE_SUPPORT);
	m_SkillTypeIndexMap.emplace("guild", SKILL_TYPE_GUILD);
	m_SkillTypeIndexMap.emplace("horse", SKILL_TYPE_HORSE);
	m_SkillTypeIndexMap.emplace("warrior", SKILL_TYPE_ACTIVE);
	m_SkillTypeIndexMap.emplace("assassin", SKILL_TYPE_ACTIVE);
	m_SkillTypeIndexMap.emplace("sura", SKILL_TYPE_ACTIVE);
	m_SkillTypeIndexMap.emplace("shaman", SKILL_TYPE_ACTIVE);
	m_SkillTypeIndexMap.emplace("passive", SKILL_TYPE_ACTIVE);

	m_SkillAttributeIndexMap.emplace("NEED_POISON_BOTTLE", SKILL_ATTRIBUTE_NEED_POISON_BOTTLE);
	m_SkillAttributeIndexMap.emplace("NEED_EMPTY_BOTTLE", SKILL_ATTRIBUTE_NEED_EMPTY_BOTTLE);
	m_SkillAttributeIndexMap.emplace("CAN_USE_IF_NOT_ENOUGH", SKILL_ATTRIBUTE_CAN_USE_IF_NOT_ENOUGH);
	m_SkillAttributeIndexMap.emplace("FAN_RANGE", SKILL_ATTRIBUTE_FAN_RANGE);
	m_SkillAttributeIndexMap.emplace("NEED_CORPSE", SKILL_ATTRIBUTE_NEED_CORPSE);
	m_SkillAttributeIndexMap.emplace("NEED_TARGET", SKILL_ATTRIBUTE_NEED_TARGET);
	m_SkillAttributeIndexMap.emplace("TOGGLE", SKILL_ATTRIBUTE_TOGGLE);
	m_SkillAttributeIndexMap.emplace("WEAPON_LIMITATION", SKILL_ATTRIBUTE_WEAPON_LIMITATION);
	m_SkillAttributeIndexMap.emplace("MELEE_ATTACK", SKILL_ATTRIBUTE_MELEE_ATTACK);
	m_SkillAttributeIndexMap.emplace("CHARGE_ATTACK", SKILL_ATTRIBUTE_CHARGE_ATTACK);
	m_SkillAttributeIndexMap.emplace("USE_HP", SKILL_ATTRIBUTE_USE_HP);
	m_SkillAttributeIndexMap.emplace("CAN_CHANGE_DIRECTION", SKILL_ATTRIBUTE_CAN_CHANGE_DIRECTION);
	m_SkillAttributeIndexMap.emplace("STANDING_SKILL", SKILL_ATTRIBUTE_STANDING_SKILL);
	m_SkillAttributeIndexMap.emplace("ONLY_FOR_ALLIANCE", SKILL_ATTRIBUTE_ONLY_FOR_ALLIANCE);
	m_SkillAttributeIndexMap.emplace("CAN_USE_FOR_ME", SKILL_ATTRIBUTE_CAN_USE_FOR_ME);
	m_SkillAttributeIndexMap.emplace("ATTACK_SKILL", SKILL_ATTRIBUTE_ATTACK_SKILL);
	m_SkillAttributeIndexMap.emplace("MOVING_SKILL", SKILL_ATTRIBUTE_MOVING_SKILL);
	m_SkillAttributeIndexMap.emplace("HORSE_SKILL", SKILL_ATTRIBUTE_HORSE_SKILL);
	m_SkillAttributeIndexMap.emplace("TIME_INCREASE_SKILL", SKILL_ATTRIBUTE_TIME_INCREASE_SKILL);
	m_SkillAttributeIndexMap.emplace("PASSIVE", SKILL_ATTRIBUTE_PASSIVE);
	m_SkillAttributeIndexMap.emplace("CANNOT_LEVEL_UP", SKILL_ATTRIBUTE_CANNOT_LEVEL_UP);
	m_SkillAttributeIndexMap.emplace("ONLY_FOR_GUILD_WAR", SKILL_ATTRIBUTE_ONLY_FOR_GUILD_WAR);
	m_SkillAttributeIndexMap.emplace("CIRCLE_RANGE", SKILL_ATTRIBUTE_CIRCLE_RANGE);
	m_SkillAttributeIndexMap.emplace("SEARCH_TARGET", SKILL_ATTRIBUTE_SEARCH_TARGET);

	m_SkillNeedWeaponIndexMap.emplace("SWORD", SKILL_NEED_WEAPON_SWORD);
	m_SkillNeedWeaponIndexMap.emplace("DAGGER", SKILL_NEED_WEAPON_DAGGER);
	m_SkillNeedWeaponIndexMap.emplace("BOW", SKILL_NEED_WEAPON_BOW);
	m_SkillNeedWeaponIndexMap.emplace("TWO_HANDED", SKILL_NEED_WEAPON_TWO_HANDED);
	m_SkillNeedWeaponIndexMap.emplace("DOUBLE_SWORD", SKILL_NEED_WEAPON_DOUBLE_SWORD);
	m_SkillNeedWeaponIndexMap.emplace("BELL", SKILL_NEED_WEAPON_BELL);
	m_SkillNeedWeaponIndexMap.emplace("FAN", SKILL_NEED_WEAPON_FAN);
	m_SkillNeedWeaponIndexMap.emplace("ARROW", SKILL_NEED_WEAPON_ARROW);
	m_SkillNeedWeaponIndexMap.emplace("EMPTY_HAND", SKILL_NEED_WEAPON_EMPTY_HAND);

	m_SkillWeaponTypeIndexMap.emplace("SWORD", ITEM::WEAPON_SWORD);
	m_SkillWeaponTypeIndexMap.emplace("DAGGER", ITEM::WEAPON_DAGGER);
	m_SkillWeaponTypeIndexMap.emplace("BOW", ITEM::WEAPON_BOW);
	m_SkillWeaponTypeIndexMap.emplace("TWO_HANDED", ITEM::WEAPON_TWO_HANDED);
	m_SkillWeaponTypeIndexMap.emplace("DOUBLE_SWORD", ITEM::WEAPON_DAGGER);
	m_SkillWeaponTypeIndexMap.emplace("BELL", ITEM::WEAPON_BELL);
	m_SkillWeaponTypeIndexMap.emplace("FAN", ITEM::WEAPON_FAN);
	m_SkillWeaponTypeIndexMap.emplace("ARROW", ITEM::WEAPON_ARROW);

	SSkillData::ms_StatusNameMap.emplace("chain", POINT_NONE);
	SSkillData::ms_StatusNameMap.emplace("HR", POINT_HIT_RATE);
	SSkillData::ms_StatusNameMap.emplace("LV", POINT_LEVEL);
	SSkillData::ms_StatusNameMap.emplace("Level", POINT_LEVEL);
	SSkillData::ms_StatusNameMap.emplace("MaxHP", POINT_MAX_HP);
	SSkillData::ms_StatusNameMap.emplace("MaxSP", POINT_MAX_SP);
	SSkillData::ms_StatusNameMap.emplace("MinMWEP", POINT_MIN_WEP);
	SSkillData::ms_StatusNameMap.emplace("MaxMWEP", POINT_MAX_WEP);
	SSkillData::ms_StatusNameMap.emplace("MinWEP", POINT_MIN_WEP);
	SSkillData::ms_StatusNameMap.emplace("MaxWEP", POINT_MAX_WEP);
	SSkillData::ms_StatusNameMap.emplace("MinATK", POINT_WEAPON_MIN);
	SSkillData::ms_StatusNameMap.emplace("MaxATK", POINT_WEAPON_MAX);
	SSkillData::ms_StatusNameMap.emplace("ATKSPD", POINT_ATT_SPEED);
	SSkillData::ms_StatusNameMap.emplace("AttackPower", POINT_WEAPON_MIN);
	SSkillData::ms_StatusNameMap.emplace("AtkMin", POINT_WEAPON_MIN);
	SSkillData::ms_StatusNameMap.emplace("AtkMax", POINT_WEAPON_MAX);
	SSkillData::ms_StatusNameMap.emplace("DefencePower", POINT_CLIENT_DEF_GRADE);
	SSkillData::ms_StatusNameMap.emplace("DEF", POINT_CLIENT_DEF_GRADE);
	SSkillData::ms_StatusNameMap.emplace("MWEP", POINT_MAGIC_ATT_GRADE);

	SSkillData::ms_StatusNameMap.emplace("MagicAttackPower", POINT_MAGIC_ATT_GRADE);
	SSkillData::ms_StatusNameMap.emplace("INT", POINT_IQ);
	SSkillData::ms_StatusNameMap.emplace("STR", POINT_ST);
	SSkillData::ms_StatusNameMap.emplace("DEX", POINT_DX);
	SSkillData::ms_StatusNameMap.emplace("CON", POINT_HT);

	SSkillData::ms_StatusNameMap.emplace("minatk", POINT_WEAPON_MIN);
	SSkillData::ms_StatusNameMap.emplace("maxatk", POINT_WEAPON_MAX);
	SSkillData::ms_StatusNameMap.emplace("minmtk", POINT_MIN_WEP);
	SSkillData::ms_StatusNameMap.emplace("maxmtk", POINT_MAX_WEP);

	// GUILD_SKILL_DISPLAY_BUG_FIX
	SSkillData::ms_StatusNameMap.emplace("maxhp", POINT_MAX_HP);
	SSkillData::ms_StatusNameMap.emplace("maxsp", POINT_MAX_SP);
	SSkillData::ms_StatusNameMap.emplace("odef", POINT_CLIENT_DEF_GRADE);
	// END_OF_GUILD_SKILL_DISPLAY_BUG_FIX

	SSkillData::ms_StatusNameMap.emplace("minwep", POINT_MIN_WEP);
	SSkillData::ms_StatusNameMap.emplace("maxwep", POINT_MAX_WEP);
	SSkillData::ms_StatusNameMap.emplace("minmwep", POINT_MIN_MAGIC_WEP);
	SSkillData::ms_StatusNameMap.emplace("maxmwep", POINT_MAX_MAGIC_WEP);
	SSkillData::ms_StatusNameMap.emplace("lv", POINT_LEVEL);
	SSkillData::ms_StatusNameMap.emplace("ar", POINT_HIT_RATE);
	SSkillData::ms_StatusNameMap.emplace("iq", POINT_IQ);
	SSkillData::ms_StatusNameMap.emplace("str", POINT_ST);
	SSkillData::ms_StatusNameMap.emplace("dex", POINT_DX);
	SSkillData::ms_StatusNameMap.emplace("con", POINT_HT);

	/////

	SSkillData::ms_NewMinStatusNameMap.emplace("atk", POINT_WEAPON_MIN);
	SSkillData::ms_NewMinStatusNameMap.emplace("mtk", POINT_MIN_WEP);
	SSkillData::ms_NewMinStatusNameMap.emplace("wep", POINT_MIN_WEP);
	SSkillData::ms_NewMinStatusNameMap.emplace("lv", POINT_LEVEL);
	SSkillData::ms_NewMinStatusNameMap.emplace("ar", POINT_HIT_RATE);
	SSkillData::ms_NewMinStatusNameMap.emplace("iq", POINT_IQ);
	SSkillData::ms_NewMinStatusNameMap.emplace("str", POINT_ST);
	SSkillData::ms_NewMinStatusNameMap.emplace("dex", POINT_DX);
	SSkillData::ms_NewMinStatusNameMap.emplace("con", POINT_HT);

	SSkillData::ms_NewMaxStatusNameMap.emplace("atk", POINT_WEAPON_MAX);
	SSkillData::ms_NewMaxStatusNameMap.emplace("mtk", POINT_MAX_WEP);
	SSkillData::ms_NewMaxStatusNameMap.emplace("wep", POINT_MAX_WEP);
	SSkillData::ms_NewMaxStatusNameMap.emplace("lv", POINT_LEVEL);
	SSkillData::ms_NewMaxStatusNameMap.emplace("ar", POINT_HIT_RATE);
	SSkillData::ms_NewMaxStatusNameMap.emplace("iq", POINT_IQ);
	SSkillData::ms_NewMaxStatusNameMap.emplace("str", POINT_ST);
	SSkillData::ms_NewMaxStatusNameMap.emplace("dex", POINT_DX);
	SSkillData::ms_NewMaxStatusNameMap.emplace("con", POINT_HT);

	m_PathNameMap.emplace("WARRIOR", "warrior");
	m_PathNameMap.emplace("ASSASSIN", "assassin");
	m_PathNameMap.emplace("SURA", "sura");
	m_PathNameMap.emplace("SHAMAN", "shaman");
	m_PathNameMap.emplace("PASSIVE", "passive");
	m_PathNameMap.emplace("SUPPORT", "common/support");
	m_PathNameMap.emplace("GUILD", "common/guild");
	m_PathNameMap.emplace("HORSE", "common/horse");
}

CPythonSkill::~CPythonSkill() = default;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t CPythonSkill::SSkillData::MELEE_SKILL_TARGET_RANGE = 170;

const std::string CPythonSkill::SSkillData::GetName() const
{
	return strName;
}

uint8_t CPythonSkill::SSkillData::GetType() const
{
	return byType;
}

bool CPythonSkill::SSkillData::IsMeleeSkill()
{
	if (dwSkillAttribute & SKILL_ATTRIBUTE_MELEE_ATTACK)
		return TRUE;

	return FALSE;
}

bool CPythonSkill::SSkillData::IsChargeSkill()
{
	if (dwSkillAttribute & SKILL_ATTRIBUTE_CHARGE_ATTACK)
		return TRUE;

	return FALSE;
}

bool CPythonSkill::SSkillData::IsOnlyForGuildWar()
{
	if (dwSkillAttribute & SKILL_ATTRIBUTE_ONLY_FOR_GUILD_WAR)
		return TRUE;

	return FALSE;
}

uint32_t CPythonSkill::SSkillData::GetTargetRange() const
{
	if (dwSkillAttribute & SKILL_ATTRIBUTE_MELEE_ATTACK)
		return MELEE_SKILL_TARGET_RANGE;

	if (dwSkillAttribute & SKILL_ATTRIBUTE_CHARGE_ATTACK)
		return MELEE_SKILL_TARGET_RANGE;

	return dwTargetRange;
}

bool CPythonSkill::SSkillData::CanChangeDirection()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_CAN_CHANGE_DIRECTION);
}

bool CPythonSkill::SSkillData::IsFanRange()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_FAN_RANGE);
}

bool CPythonSkill::SSkillData::IsCircleRange()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_CIRCLE_RANGE);
}

bool CPythonSkill::SSkillData::IsAutoSearchTarget()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_SEARCH_TARGET);
}

bool CPythonSkill::SSkillData::IsNeedCorpse()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_NEED_CORPSE);
}

bool CPythonSkill::SSkillData::IsNeedTarget()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_NEED_TARGET);
}

bool CPythonSkill::SSkillData::IsToggleSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_TOGGLE);
}

bool CPythonSkill::SSkillData::IsUseHPSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_USE_HP);
}

bool CPythonSkill::SSkillData::IsStandingSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_STANDING_SKILL);
}

bool CPythonSkill::SSkillData::CanUseWeaponType(uint32_t dwWeaponType)
{
	if (!(dwSkillAttribute & SKILL_ATTRIBUTE_WEAPON_LIMITATION))
		return TRUE;

	return 0 != (dwNeedWeapon & (1 << dwWeaponType));
}

bool CPythonSkill::SSkillData::IsOnlyForAlliance()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_ONLY_FOR_ALLIANCE);
}

bool CPythonSkill::SSkillData::CanUseForMe()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_CAN_USE_FOR_ME);
}

bool CPythonSkill::SSkillData::CanUseIfNotEnough()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_CAN_USE_IF_NOT_ENOUGH);
}

bool CPythonSkill::SSkillData::IsNeedEmptyBottle()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_NEED_EMPTY_BOTTLE);
}

bool CPythonSkill::SSkillData::IsNeedPoisonBottle()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_NEED_POISON_BOTTLE);
}

bool CPythonSkill::SSkillData::IsNeedBow()
{
	if (!(dwSkillAttribute & SKILL_ATTRIBUTE_WEAPON_LIMITATION))
		return FALSE;

	return 0 != (dwNeedWeapon & SKILL_NEED_WEAPON_BOW);
}

bool CPythonSkill::SSkillData::IsHorseSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_HORSE_SKILL);
}

bool CPythonSkill::SSkillData::IsMovingSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_MOVING_SKILL);
}

bool CPythonSkill::SSkillData::IsAttackSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_ATTACK_SKILL);
}

bool CPythonSkill::SSkillData::IsTimeIncreaseSkill()
{
	return 0 != (dwSkillAttribute & SKILL_ATTRIBUTE_TIME_INCREASE_SKILL);
}

bool CPythonSkill::SSkillData::GetState(const char* c_szStateName, int32_t* piState, int32_t iMinMaxType)
{
	std::map<std::string, uint32_t>* pStatusNameMap = nullptr;

	switch (iMinMaxType)
	{
	case VALUE_TYPE_FREE:
		pStatusNameMap = &ms_StatusNameMap;
		break;
	case VALUE_TYPE_MIN:
		pStatusNameMap = &ms_NewMinStatusNameMap;
		break;
	case VALUE_TYPE_MAX:
		pStatusNameMap = &ms_NewMaxStatusNameMap;
		break;
	default:
		return false;
	}

	auto it = pStatusNameMap->find(c_szStateName);

	if (it != pStatusNameMap->end())
	{
		*piState = CPythonPlayer::GetInstance()->GetStatus(it->second);
	}
	else if (0 == strncmp(c_szStateName, "JeungJi", 7))
	{
		*piState = 0;
	}

	return true;
}

float CPythonSkill::SSkillData::ProcessFormula(CPoly* pPoly, float fSkillLevel, int32_t iMinMaxType)
{
	if (pPoly->Analyze())
	{
		for (uint32_t i = 0; i < pPoly->GetVarCount(); ++i)
		{
			const char* c_szVarName = pPoly->GetVarName(i);
			float fState;
			if (!strcmp("SkillPoint", c_szVarName) || !strcmp("k", c_szVarName))
			{
				fState = fSkillLevel;
			}
			else
			{
				int32_t iState;
				if (!GetState(c_szVarName, &iState, iMinMaxType))
					return 0.0f;

				fState = float(iState);

				if (!strcmp("ar", c_szVarName))
					fState /= 100.0f;
			}
			pPoly->SetVar(c_szVarName, fState);
		}
	}
	else
	{
		SysLog("skillGetAffect - Strange Formula [{0}]", strName.c_str());
		return 0.0f;
	}

	return pPoly->Eval();
}

const char* CPythonSkill::SSkillData::GetAffectDescription(uint32_t dwIndex, float fSkillLevel)
{
	if (dwIndex >= AffectDataVector.size())
		return NULL;

	const std::string& c_rstrAffectDescription = AffectDataVector[dwIndex].strAffectDescription;
	const std::string& c_rstrAffectMinFormula = AffectDataVector[dwIndex].strAffectMinFormula;
	const std::string& c_rstrAffectMaxFormula = AffectDataVector[dwIndex].strAffectMaxFormula;

	CPoly minPoly;
	CPoly maxPoly;
	minPoly.SetStr(c_rstrAffectMinFormula.c_str());
	maxPoly.SetStr(c_rstrAffectMaxFormula.c_str());

	// OVERWRITE_SKILLPROTO_POLY
	float fMinValue = ProcessFormula(&minPoly, fSkillLevel);
	float fMaxValue = ProcessFormula(&maxPoly, fSkillLevel);

	if (fMinValue < 0.0)
		fMinValue = -fMinValue;
	if (fMaxValue < 0.0)
		fMaxValue = -fMaxValue;

	if (strstr(c_rstrAffectDescription.c_str(), "%.0f"))
	{
		fMinValue = floorf(fMinValue);
		fMaxValue = floorf(fMaxValue);
	}

	static char szDescription[64 + 1];
	_snprintf(szDescription, sizeof(szDescription), c_rstrAffectDescription.c_str(), fMinValue, fMaxValue);

	return szDescription;
}

uint32_t CPythonSkill::SSkillData::GetSkillCoolTime(float fSkillPoint)
{
	if (strCoolTimeFormula.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strCoolTimeFormula.c_str());

	return uint32_t(ProcessFormula(&poly, fSkillPoint));
}

uint32_t CPythonSkill::SSkillData::GetTargetCount(float fSkillPoint)
{
	if (strTargetCountFormula.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strTargetCountFormula.c_str());

	return uint32_t(ProcessFormula(&poly, fSkillPoint));
}

uint32_t CPythonSkill::SSkillData::GetSkillMotionIndex(int32_t iGrade)
{
	if (-1 != iGrade)
	{
		assert(iGrade >= 0 && iGrade < SKILL_EFFECT_COUNT);
		if (iGrade >= 0 && iGrade < SKILL_EFFECT_COUNT)
		{
			return GradeData[iGrade].wMotionIndex;
		}
	}

	return wMotionIndex;
}

uint8_t CPythonSkill::SSkillData::GetMaxLevel()
{
	return byMaxLevel;
}

uint8_t CPythonSkill::SSkillData::GetLevelUpPoint()
{
	return byLevelUpPoint;
}

bool CPythonSkill::SSkillData::IsNoMotion()
{
	return bNoMotion;
}

bool CPythonSkill::SSkillData::IsCanUseSkill()
{
	if (0 != (dwSkillAttribute & SKILL_ATTRIBUTE_PASSIVE))
		return false;

	return true;
}

uint32_t CPythonSkill::SSkillData::GetMotionLoopCount(float fSkillPoint)
{
	if (strMotionLoopCountFormula.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strMotionLoopCountFormula.c_str());

	return uint32_t(ProcessFormula(&poly, fSkillPoint));
}

int32_t CPythonSkill::SSkillData::GetNeedSP(float fSkillPoint)
{
	if (strNeedSPFormula.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strNeedSPFormula.c_str());

	return int32_t(ProcessFormula(&poly, fSkillPoint));
}

uint32_t CPythonSkill::SSkillData::GetContinuationSP(float fSkillPoint)
{
	if (strContinuationSPFormula.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strContinuationSPFormula.c_str());

	return uint32_t(ProcessFormula(&poly, fSkillPoint));
}

uint32_t CPythonSkill::SSkillData::GetDuration(float fSkillPoint)
{
	if (strDuration.empty())
		return 0;

	CPoly poly;
	poly.SetStr(strDuration.c_str());

	return uint32_t(ProcessFormula(&poly, fSkillPoint));
}

CPythonSkill::SSkillData::SSkillData()
{
	byType = SKILL_TYPE_ACTIVE;
	byMaxLevel = 20;
	byLevelUpPoint = 1;
	dwSkillAttribute = 0;
	dwNeedWeapon = 0;
	dwTargetRange = 0;
	strCoolTimeFormula.clear();
	strMotionLoopCountFormula.clear();
	strNeedSPFormula.clear();
	strContinuationSPFormula.clear();
	isRequirement = FALSE;
	strRequireSkillName.clear();
	byRequireSkillLevel = 0;
	strDuration.clear();
	byLevelLimit = 0;
	bNoMotion = FALSE;

	strName.clear();
	pImage = nullptr;

	dwSkillIndex = 0;
	wMotionIndex = 0;
	wMotionIndexForMe = 0;

	for (int32_t j = 0; j < SKILL_GRADE_COUNT; ++j)
	{
		TGradeData& rGradeData = GradeData[j];
		rGradeData.strName.clear();
		rGradeData.pImage = nullptr;
		rGradeData.wMotionIndex = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////


static void skillSetPathName(std::string szPathName)
{
	CPythonSkill::GetInstance()->SetPathName(szPathName.c_str());
}

static void skillRegisterSkill(uint32_t iSkillIndex, std::string szFileName)
{
	std::string strFullFileName;
	strFullFileName = CPythonSkill::GetInstance()->GetPathName();
	strFullFileName += szFileName;

	if (!CPythonSkill::GetInstance()->RegisterSkill(iSkillIndex, strFullFileName.c_str()))
		throw std::runtime_error("skill.RegisterSkill - Failed to find skill data file : " + std::to_string(iSkillIndex) + strFullFileName);
}

static void skillClearSkillData()
{
	CPythonSkill::SSkillData::ms_dwTimeIncreaseSkillNumber = 0;
}

static std::string skillGetSkillName(uint32_t iSkillIndex, size_t iSkillGrade)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillName - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iSkillGrade >= 0 && iSkillGrade < CPythonSkill::SKILL_GRADE_COUNT)
	{
		return c_pSkillData->GradeData[iSkillGrade].strName;		
	}

	return c_pSkillData->strName;
}

static std::string skillGetSkillDescription(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillDescription - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->strDescription;
}

static uint8_t skillGetSkillType(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillType - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->byType;
}

static size_t skillGetSkillConditionDescriptionCount(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillConditionDescriptionCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->ConditionDataVector.size();
}

static std::string skillGetSkillConditionDescription(uint32_t iSkillIndex, uint32_t iConditionIndex)
{
	CPythonSkill::SSkillData* c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillAffectDescriptionCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->ConditionDataVector[iConditionIndex];
}

static size_t skillGetSkillAffectDescriptionCount(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillAffectDescriptionCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->AffectDataVector.size();
}

static std::string skillGetSkillAffectDescription(uint32_t iSkillIndex, uint32_t iAffectIndex, float fSkillPoint, uint32_t skillLevel)
{
	CPythonSkill::SSkillData* c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillAffectDescriptionCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetAffectDescription(iAffectIndex, fSkillPoint);
}

static uint32_t skillGetSkillCoolTime(uint32_t iSkillIndex, float fSkillPoint)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillCoolTime - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetSkillCoolTime(fSkillPoint);
}

static int32_t skillGetSkillNeedSP(uint32_t iSkillIndex, float fSkillPoint)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillNeedSP Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetNeedSP(fSkillPoint);
}

static int skillGetSkillContinuationSP(uint32_t iSkillIndex, float fSkillPoint)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillContinuationSP - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetContinuationSP(fSkillPoint);
}

static int skillGetSkillMaxLevel(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillMaxLevel - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetMaxLevel();
}

static int skillGetSkillLevelUpPoint(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillLevelUpPoint - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetLevelUpPoint();
}

static int skillGetSkillLevelLimit(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillLevelLimit - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->byLevelLimit;
}

static bool skillIsSkillRequirement(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.IsSkillRequirement - Failed to find skill by " + std::to_string(iSkillIndex));

	if (c_pSkillData->isRequirement)
	{
		CPythonSkill::SSkillData * pRequireSkillData;
		if (!CPythonSkill::GetInstance()->GetSkillDataByName(c_pSkillData->strRequireSkillName.c_str(), &pRequireSkillData))
		{
			TraceLog("skill.IsSkillRequirement - Failed to find skill : [{}/{}] can't find [{}]\n", c_pSkillData->dwSkillIndex, c_pSkillData->strName.c_str(), c_pSkillData->strRequireSkillName.c_str());
			return false;		
		}

		uint32_t dwRequireSkillSlotIndex;
		if (!CPythonPlayer::GetInstance()->FindSkillSlotIndexBySkillIndex(pRequireSkillData->dwSkillIndex, &dwRequireSkillSlotIndex))
			return  false;
	}

	return c_pSkillData->isRequirement;
}

static std::tuple<std::string,int32_t> skillGetSkillRequirementData(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillRequirementData - Failed to find skill by " + std::to_string(iSkillIndex));

	CPythonSkill::SSkillData * pRequireSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillDataByName(c_pSkillData->strRequireSkillName.c_str(), &pRequireSkillData))
		return std::make_tuple("None", 0);

	int32_t ireqLevel = (int32_t)ceil(float(c_pSkillData->byRequireSkillLevel)/float(std::max(uint8_t(1), pRequireSkillData->byLevelUpPoint)));
	return std::make_tuple( c_pSkillData->strRequireSkillName, ireqLevel);
}

static int skillGetSkillRequireStatCount(int iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillRequireStatCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->RequireStatDataVector.size();
}

static std::tuple<int,int> skillGetSkillRequireStatData(uint32_t iSkillIndex, int iStatIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetSkillRequireStatData - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iStatIndex >= c_pSkillData->RequireStatDataVector.size())
	return std::make_tuple( 0, 0);
	const CPythonSkill::TRequireStatData & c_rRequireStatData = c_pSkillData->RequireStatDataVector[iStatIndex];

	return std::make_tuple( c_rRequireStatData.byPoint, c_rRequireStatData.byLevel);
}

static bool skillCanLevelUpSkill(uint32_t iSkillIndex, int iSkillLevel)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.CanLevelUpSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iSkillLevel >= c_pSkillData->GetMaxLevel())
		return false;

	if (c_pSkillData->isRequirement)
	{
		CPythonSkill::SSkillData * pRequireSkillData;
		if (CPythonSkill::GetInstance()->GetSkillDataByName(c_pSkillData->strRequireSkillName.c_str(), &pRequireSkillData))
		{
			uint32_t dwRequireSkillSlotIndex;
			if (CPythonPlayer::GetInstance()->FindSkillSlotIndexBySkillIndex(pRequireSkillData->dwSkillIndex, &dwRequireSkillSlotIndex))
			{
				int32_t iSkillGrade = CPythonPlayer::GetInstance()->GetSkillGrade(dwRequireSkillSlotIndex);
				int32_t iSkillLevel = CPythonPlayer::GetInstance()->GetSkillLevel(dwRequireSkillSlotIndex);
				if (iSkillGrade <= 0 && iSkillLevel < c_pSkillData->byRequireSkillLevel)
					return false;
			}
		}
	}

	for (uint32_t i = 0; i < c_pSkillData->RequireStatDataVector.size(); ++i)
	{
		const CPythonSkill::TRequireStatData & c_rRequireStatData = c_pSkillData->RequireStatDataVector[i];
		if (CPythonPlayer::GetInstance()->GetStatus(c_rRequireStatData.byPoint) < c_rRequireStatData.byLevel)
			return  false;	
	}

	if (0 != (c_pSkillData->dwSkillAttribute & CPythonSkill::SKILL_ATTRIBUTE_CANNOT_LEVEL_UP))
		return  false;

	return  true;
}

static bool skillIsLevelUpSkill(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.IsLevelUpSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	return  true;
}

static int skillCheckRequirementSueccess(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.CheckRequirementSueccess - Failed to find skill by " + std::to_string(iSkillIndex));

	if (c_pSkillData->isRequirement)
	{
		CPythonSkill::SSkillData * pRequireSkillData;
		if (CPythonSkill::GetInstance()->GetSkillDataByName(c_pSkillData->strRequireSkillName.c_str(), &pRequireSkillData))
		{
			uint32_t dwRequireSkillSlotIndex;
			if (CPythonPlayer::GetInstance()->FindSkillSlotIndexBySkillIndex(pRequireSkillData->dwSkillIndex, &dwRequireSkillSlotIndex))
			{
				int32_t iSkillGrade = CPythonPlayer::GetInstance()->GetSkillGrade(dwRequireSkillSlotIndex);
				int32_t iSkillLevel = CPythonPlayer::GetInstance()->GetSkillLevel(dwRequireSkillSlotIndex);
				if (iSkillGrade <= 0 && iSkillLevel < c_pSkillData->byRequireSkillLevel)
					return false;			
			}
		}
	}

	return true;
}

static uint8_t skillGetNeedCharacterLevel(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetNeedCharacterLevel - Failed to find skill by " + std::to_string(iSkillIndex));

	std::vector<CPythonSkill::TRequireStatData>::iterator itor = c_pSkillData->RequireStatDataVector.begin();
	for (; itor != c_pSkillData->RequireStatDataVector.end(); ++itor)
	{
		const CPythonSkill::TRequireStatData & c_rRequireStatData = *itor;

		if (POINT_LEVEL == c_rRequireStatData.byPoint)
			return  c_rRequireStatData.byLevel;
	}

	return 0;
}

static bool skillIsToggleSkill(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.IsToggleSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	return  c_pSkillData->IsToggleSkill();
}

static bool skillIsUseHPSkill(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.IsUseHPSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	return  c_pSkillData->IsUseHPSkill();
}

static bool skillIsStandingSkill(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.IsStandingSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	return  c_pSkillData->IsStandingSkill();
}

static bool skillCanUseSkill(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.CanUseSkill - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->IsCanUseSkill();
}

static std::string skillGetIconName(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.skillGetIconName - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->strIconFileName;
}

static std::string skillGetIconImage(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.skillGetIconImage - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->pImage->GetFileNameString();
}

static std::string skillGetIconImageNew(uint32_t iSkillIndex, size_t iGradeIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.skillGetIconImageNew - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iGradeIndex < 0)
		iGradeIndex = 0;

	if (iGradeIndex >= CPythonSkill::SKILL_GRADE_COUNT)		
		iGradeIndex = static_cast<size_t>(CPythonSkill::SKILL_GRADE_COUNT) - 1;

	return c_pSkillData->GradeData[iGradeIndex].pImage->GetFileNameString();
}

static auto skillGetIconInstance(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetIconInstance - Failed to find skill by " + std::to_string(iSkillIndex));

	auto pImageInstance = new CGraphicImageInstance;
	pImageInstance->SetImagePointer(c_pSkillData->pImage);

	return pybind11::capsule(pImageInstance, ImageCapsuleDestroyer);
}

static auto skillGetIconInstanceNew(uint32_t iSkillIndex, size_t iGradeIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetIconInstanceNew - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iGradeIndex >= CPythonSkill::SKILL_GRADE_COUNT)		
		iGradeIndex = static_cast<size_t>(CPythonSkill::SKILL_GRADE_COUNT) - 1;

	auto pImageInstance = new CGraphicImageInstance;
	pImageInstance->SetImagePointer(c_pSkillData->GradeData[std::min<size_t>(2, iGradeIndex)].pImage);

	return pybind11::capsule(pImageInstance, ImageCapsuleDestroyer);
}

static std::tuple<CGraphicImage*, std::string, uint16_t> skillGetGradeData(uint32_t iSkillIndex, size_t iGradeIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetGradeData - Failed to find skill by " + std::to_string(iSkillIndex));

	if (iGradeIndex < 0 || iGradeIndex >= CPythonSkill::SKILL_GRADE_COUNT)
		throw std::runtime_error("Strange grade index [" + std::to_string(iSkillIndex) + "]");

	auto& e = c_pSkillData->GradeData[iGradeIndex];
	return std::make_tuple(e.pImage, e.strName, e.wMotionIndex);
}

static size_t skillGetNewAffectDataCount(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetNewAffectDataCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->AffectDataNewVector.size();
}

static size_t skillGetNewAffectData(uint32_t iSkillIndex)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetNewAffectDataCount - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->AffectDataNewVector.size();
}

static uint32_t skillGetDuration(uint32_t iSkillIndex, float fSkillLevel)
{
	CPythonSkill::SSkillData * c_pSkillData;
	if (!CPythonSkill::GetInstance()->GetSkillData(iSkillIndex, &c_pSkillData))
		throw std::runtime_error("skill.GetDuration - Failed to find skill by " + std::to_string(iSkillIndex));

	return c_pSkillData->GetDuration(fSkillLevel);
}


PYBIND11_EMBEDDED_MODULE(skill, m)
{
	m.def("SetPathName",	skillSetPathName);
	m.def("RegisterSkill",	skillRegisterSkill);
	m.def("ClearSkillData",	skillClearSkillData);
	m.def("GetSkillName",	skillGetSkillName);
	m.def("GetSkillDescription",	skillGetSkillDescription);
	m.def("GetSkillType",	skillGetSkillType);
	m.def("GetSkillConditionDescriptionCount",	skillGetSkillConditionDescriptionCount);
	m.def("GetSkillConditionDescription",	skillGetSkillConditionDescription);
	m.def("GetSkillAffectDescriptionCount",	skillGetSkillAffectDescriptionCount);
	m.def("GetSkillAffectDescription",	skillGetSkillAffectDescription);
	m.def("GetSkillCoolTime",	skillGetSkillCoolTime);
	m.def("GetSkillNeedSP",	skillGetSkillNeedSP);
	m.def("GetSkillContinuationSP",	skillGetSkillContinuationSP);
	m.def("GetSkillMaxLevel",	skillGetSkillMaxLevel);
	m.def("GetSkillLevelUpPoint",	skillGetSkillLevelUpPoint);
	m.def("GetSkillLevelLimit",	skillGetSkillLevelLimit);
	m.def("IsSkillRequirement",	skillIsSkillRequirement);
	m.def("GetSkillRequirementData",	skillGetSkillRequirementData);
	m.def("GetSkillRequireStatCount",	skillGetSkillRequireStatCount);
	m.def("GetSkillRequireStatData",	skillGetSkillRequireStatData);
	m.def("CanLevelUpSkill",	skillCanLevelUpSkill);
	m.def("IsLevelUpSkill",	skillIsLevelUpSkill);
	m.def("CheckRequirementSueccess",	skillCheckRequirementSueccess);
	m.def("GetNeedCharacterLevel",	skillGetNeedCharacterLevel);
	m.def("IsToggleSkill",	skillIsToggleSkill);
	m.def("IsUseHPSkill",	skillIsUseHPSkill);
	m.def("IsStandingSkill",	skillIsStandingSkill);
	m.def("CanUseSkill",	skillCanUseSkill);
	m.def("GetIconName",	skillGetIconName);
	m.def("GetIconImage",	skillGetIconImage);
	m.def("GetIconImageNew",	skillGetIconImageNew);
	m.def("GetIconInstance",	skillGetIconInstance);
	m.def("GetIconInstanceNew",	skillGetIconInstanceNew);
	m.def("GetGradeData",	skillGetGradeData);
	m.def("GetNewAffectDataCount",	skillGetNewAffectDataCount);
	m.def("GetNewAffectData",	skillGetNewAffectData);
	m.def("GetDuration",	skillGetDuration);

	m.attr("SKILL_TYPE_NONE") = int32_t(CPythonSkill::SKILL_TYPE_NONE);
	m.attr("SKILL_TYPE_ACTIVE") = int32_t(CPythonSkill::SKILL_TYPE_ACTIVE);
	m.attr("SKILL_TYPE_SUPPORT") = int32_t(CPythonSkill::SKILL_TYPE_SUPPORT);
	m.attr("SKILL_TYPE_GUILD") = int32_t(CPythonSkill::SKILL_TYPE_GUILD);
	m.attr("SKILL_TYPE_HORSE") = int32_t(CPythonSkill::SKILL_TYPE_HORSE);
	m.attr("SKILL_TYPE_MAX_NUM") = int32_t(CPythonSkill::SKILL_TYPE_MAX_NUM);
	m.attr("SKILL_GRADE_COUNT") = int32_t(CPythonSkill::SKILL_GRADE_COUNT);
	m.attr("SKILL_GRADE_STEP_COUNT") = int32_t(CPythonSkill::SKILL_GRADE_STEP_COUNT);
	m.attr("SKILL_GRADEGAP") = int32_t(CPythonSkill::SKILL_GRADEGAP);
	m.attr("SKILL_EFFECT_COUNT") = int32_t(CPythonSkill::SKILL_EFFECT_COUNT);
}
