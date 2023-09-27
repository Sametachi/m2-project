#pragma once

void LoadLocaleServiceSettings();
const std::string& LocaleService_GetBasePath();
const std::string& LocaleService_GetMapPath();
const std::string& LocaleService_GetQuestPath();

enum ELanguages
{
	LANGUAGE_NONE,
	LANGUAGE_EN,
	LANGUAGE_AE,
	LANGUAGE_CZ,
	LANGUAGE_DK,
	LANGUAGE_FR,
	LANGUAGE_GR,
	LANGUAGE_NL,
	LANGUAGE_PL,
	LANGUAGE_HU,
	LANGUAGE_DE,
	LANGUAGE_IT,
	LANGUAGE_RU,
	LANGUAGE_PT,
	LANGUAGE_RO,
	LANGUAGE_ES,
	LANGUAGE_TR,

	LANGUAGE_MAX_NUM,
	LANGUAGE_DEFAULT = LANGUAGE_EN,
};