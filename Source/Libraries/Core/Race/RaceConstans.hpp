#pragma once

enum ERaceParts
{
	PART_MAIN = 0,
	PART_WEAPON = 1,
	PART_HEAD = 2,
	PART_WEAPON_LEFT = 3,
	PART_HAIR = 4,
	PART_ACCE = 5,

	PART_MAX_NUM,
};

static const uint32_t c_iSkillIndex_Tongsol 	= 121;
static const uint32_t c_iSkillIndex_Combo 		= 122;
static const uint32_t c_iSkillIndex_Fishing 	= 123;
static const uint32_t c_iSkillIndex_Mining 		= 124;
static const uint32_t c_iSkillIndex_Making 		= 125;
static const uint32_t c_iSkillIndex_Language1 	= 126;
static const uint32_t c_iSkillIndex_Language2 	= 127;
static const uint32_t c_iSkillIndex_Language3 	= 128;
static const uint32_t c_iSkillIndex_Polymorph 	= 129;
static const uint32_t c_iSkillIndex_Riding 		= 130;
static const uint32_t c_iSkillIndex_Summon 		= 131;

enum ERaceData
{
	MAIN_RACE_WARRIOR_M		= 0,
	MAIN_RACE_ASSASSIN_W	= 1,
	MAIN_RACE_SURA_M		= 2,
	MAIN_RACE_SHAMAN_W		= 3,
	MAIN_RACE_WARRIOR_W		= 4,
	MAIN_RACE_ASSASSIN_M	= 5,
	MAIN_RACE_SURA_W		= 6,
	MAIN_RACE_SHAMAN_M		= 7,
	MAIN_RACE_MAX_NUM,
};

enum ERaceEmotions
{
	EMOTION_CLAP 			= 1,
	EMOTION_CONGRATULATION	= 2,
	EMOTION_FORGIVE			= 3,
	EMOTION_ANGRY			= 4,
	EMOTION_ATTRACTIVE		= 5,
	EMOTION_SAD				= 6,
	EMOTION_SHY				= 7,
	EMOTION_CHEERUP			= 8,
	EMOTION_BANTER			= 9,
	EMOTION_JOY				= 10,
	EMOTION_CHEERS_1		= 11,
	EMOTION_CHEERS_2		= 12,
	EMOTION_DANCE_1			= 13,
	EMOTION_DANCE_2			= 14,
	EMOTION_DANCE_3			= 15,
	EMOTION_DANCE_4			= 16,
	EMOTION_DANCE_5			= 17,
	EMOTION_DANCE_6			= 18,
	
	/* Shared Emotions */
	EMOTION_KISS 			= 51,
	EMOTION_FRENCH_KISS		= 52,
	EMOTION_SLAP			= 53,
};
