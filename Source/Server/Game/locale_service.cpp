#include "stdafx.h"
#include "locale_service.h"
#include "constants.h"
#include "banword.h"
#include "utils.h"
#include "mob_manager.h"
#include "config.h"
#include "skill_power.h"

std::string g_stServiceName = "uk";
std::string g_stLocale = "latin1";
std::string g_stServiceBaseDir = "base";
std::string g_stServiceMapDir = "map";
std::string g_stQuestDir = "quest";

uint8_t PK_PROTECT_LEVEL = 15;

int32_t (*check_name) (const char* str) = nullptr;
int32_t (*is_twobyte) (const char* str) = nullptr;

int32_t is_twobyte_euckr(const char* str)
{
	return ishan(*str);
}

int32_t check_name_independent(const char* str)
{
	if (CBanwordManager::GetInstance()->CheckString(str, strlen(str)))
		return 0;

	char szTmp[256];
	str_lower(str, szTmp, sizeof(szTmp));

	if (CMobManager::GetInstance()->Get(szTmp, false))
		return 0;

	return 1;
}

int32_t check_name_latin1(const char* str)
{
	int32_t		code;
	const char* tmp;

	if (!str || !*str)
		return 0;

	if (strlen(str) < 2)
		return 0;

	for (tmp = str; *tmp; ++tmp)
	{
		if (isnhspace(*tmp))
			return 0;

		if (isnhdigit(*tmp))
			continue;

		if (!ishan(*tmp) && isalpha(*tmp))
			continue;

		uint8_t uc_tmp = *tmp;

		if (uc_tmp == 145 || uc_tmp == 146 || uc_tmp == 196
			|| uc_tmp == 214 || uc_tmp == 220 || uc_tmp == 223
			|| uc_tmp == 228 || uc_tmp == 246 || uc_tmp == 252)
			continue;
		code = *tmp;
		code += 256;

		if (code < 176 || code > 200)
			return 0;

		++tmp;

		if (!*tmp)
			break;
	}

	return check_name_independent(str);
}

int32_t check_name_alphabet(const char* str)
{
	const char* tmp;

	if (!str || !*str)
		return 0;

	if (strlen(str) < 2)
		return 0;

	for (tmp = str; *tmp; ++tmp)
	{
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;
		else
			return 0;
	}

	return check_name_independent(str);
}
void LoadLocaleServiceSettings()
{
	g_setQuestObjectDir.clear();
	g_setQuestObjectDir.insert("quest/object");

	if (!check_name)
		check_name = check_name_alphabet;

	if (!is_twobyte)
		is_twobyte = is_twobyte_euckr;

	if (!exp_table)
		exp_table = exp_table_common;

	if (!CTableBySkill::GetInstance()->Check())
	{
		FatalLog("Skill table check failed.");
		exit(1);
	}

	if (!aiPercentByDeltaLevForBoss)
		aiPercentByDeltaLevForBoss = aiPercentByDeltaLevForBoss_euckr;

	if (!aiPercentByDeltaLev)
		aiPercentByDeltaLev = aiPercentByDeltaLev_euckr;

	if (!aiChainLightningCountBySkillLevel)
		aiChainLightningCountBySkillLevel = aiChainLightningCountBySkillLevel_euckr;
}

const std::string& LocaleService_GetBasePath()
{
	return g_stServiceBaseDir;
}

const std::string& LocaleService_GetMapPath()
{
	return g_stServiceMapDir;
}

const std::string& LocaleService_GetQuestPath()
{
	return g_stQuestDir;
}