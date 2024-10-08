#pragma once
#include <cstdint>
#include <storm/String.hpp>
#include <Core/Race/RaceConstans.hpp>
#include <Core/Constants/Controls.hpp>

#define COMBO_KEY uint32_t
#define MAKE_COMBO_KEY(motion_mode, combo_type)		(	(uint32_t(motion_mode) << 16) | (uint32_t(combo_type))	)
#define COMBO_KEY_GET_MOTION_MODE(key)				(	uint16_t(uint32_t(key) >> 16 & 0xFFFF)						)
#define COMBO_KEY_GET_COMBO_TYPE(key)				(	uint16_t(uint32_t(key) & 0xFFFF)							)

enum MotionLimits
{
	MOTION_EVENT_MAX_NUM = 50,
};

enum MotionType
{
	MOTION_TYPE_NONE = 0,
	MOTION_TYPE_WAIT = 1,
	MOTION_TYPE_MOVE = 2,
	MOTION_TYPE_ATTACK = 3,
	MOTION_TYPE_COMBO = 4,
	MOTION_TYPE_DAMAGE = 5,
	MOTION_TYPE_KNOCKDOWN = 6,
	MOTION_TYPE_DIE = 7,
	MOTION_TYPE_SKILL = 8,
	MOTION_TYPE_STANDUP = 9,
	MOTION_TYPE_EVENT = 10,
	MOTION_TYPE_FISHING = 11,
	MOTION_TYPE_EMOTION = 12,
	MOTION_TYPE_NUM,
};

enum MotionMode
{
	MOTION_MODE_RESERVED = 0,
	MOTION_MODE_GENERAL = 1,
	MOTION_MODE_ONEHAND_SWORD = 2,
	MOTION_MODE_TWOHAND_SWORD = 3,
	MOTION_MODE_DUALHAND_SWORD = 4,
	MOTION_MODE_BOW = 5,
	MOTION_MODE_FAN = 6,
	MOTION_MODE_BELL = 7,
	MOTION_MODE_FISHING = 8,
	MOTION_MODE_HORSE = 9,
	MOTION_MODE_HORSE_ONEHAND_SWORD = 10,
	MOTION_MODE_HORSE_TWOHAND_SWORD = 11,
	MOTION_MODE_HORSE_DUALHAND_SWORD = 12,
	MOTION_MODE_HORSE_BOW = 13,
	MOTION_MODE_HORSE_FAN = 14,
	MOTION_MODE_HORSE_BELL = 15,
	MOTION_MODE_WEDDING_DRESS = 16,
	MOTION_MODE_MAX_NUM,
};

enum MotionName
{
	MOTION_NONE,
	MOTION_WAIT,
	MOTION_WALK,
	MOTION_RUN,
	MOTION_CHANGE_WEAPON,
	MOTION_DAMAGE,
	MOTION_DAMAGE_FLYING,
	MOTION_STAND_UP,
	MOTION_DAMAGE_BACK,
	MOTION_DAMAGE_FLYING_BACK,
	MOTION_STAND_UP_BACK,
	MOTION_DEAD,
	MOTION_DEAD_BACK,
	MOTION_NORMAL_ATTACK,
	MOTION_COMBO_ATTACK_1,
	MOTION_COMBO_ATTACK_2,
	MOTION_COMBO_ATTACK_3,
	MOTION_COMBO_ATTACK_4,
	MOTION_COMBO_ATTACK_5,
	MOTION_COMBO_ATTACK_6,
	MOTION_COMBO_ATTACK_7,
	MOTION_COMBO_ATTACK_8,
	MOTION_INTRO_WAIT,
	MOTION_INTRO_SELECTED,
	MOTION_INTRO_NOT_SELECTED,
	MOTION_SPAWN,
	MOTION_FISHING_THROW,
	MOTION_FISHING_WAIT,
	MOTION_FISHING_STOP,
	MOTION_FISHING_REACT,
	MOTION_FISHING_CATCH,
	MOTION_FISHING_FAIL,
	MOTION_STOP,
	MOTION_SPECIAL_1,
	MOTION_SPECIAL_2,
	MOTION_SPECIAL_3,
	MOTION_SPECIAL_4,
	MOTION_SPECIAL_5,
	MOTION_SPECIAL_6,
	MOTION_SKILL = 50,
	MOTION_SKILL_END = MOTION_SKILL + SKILL_MAX_NUM,

	// Emotions
	MOTION_CLAP,
	MOTION_CHEERS_1,
	MOTION_CHEERS_2,
	MOTION_KISS_START,
	MOTION_KISS_WITH_WARRIOR = MOTION_KISS_START + 0,
	MOTION_KISS_WITH_ASSASSIN = MOTION_KISS_START + 1,
	MOTION_KISS_WITH_SURA = MOTION_KISS_START + 2,
	MOTION_KISS_WITH_SHAMAN = MOTION_KISS_START + 3,
	MOTION_FRENCH_KISS_START,
	MOTION_FRENCH_KISS_WITH_WARRIOR = MOTION_FRENCH_KISS_START + 0,
	MOTION_FRENCH_KISS_WITH_ASSASSIN = MOTION_FRENCH_KISS_START + 1,
	MOTION_FRENCH_KISS_WITH_SURA = MOTION_FRENCH_KISS_START + 2,
	MOTION_FRENCH_KISS_WITH_SHAMAN = MOTION_FRENCH_KISS_START + 3,
	MOTION_SLAP_HIT_START,
	MOTION_SLAP_HIT_WITH_WARRIOR = MOTION_SLAP_HIT_START + 0,
	MOTION_SLAP_HIT_WITH_ASSASSIN = MOTION_SLAP_HIT_START + 1,
	MOTION_SLAP_HIT_WITH_SURA = MOTION_SLAP_HIT_START + 2,
	MOTION_SLAP_HIT_WITH_SHAMAN = MOTION_SLAP_HIT_START + 3,
	MOTION_SLAP_HURT_START,
	MOTION_SLAP_HURT_WITH_WARRIOR = MOTION_SLAP_HURT_START + 0,
	MOTION_SLAP_HURT_WITH_ASSASSIN = MOTION_SLAP_HURT_START + 1,
	MOTION_SLAP_HURT_WITH_SURA = MOTION_SLAP_HURT_START + 2,
	MOTION_SLAP_HURT_WITH_SHAMAN = MOTION_SLAP_HURT_START + 3,

	MOTION_DIG, // This is considered as Emotion for some reason? ymir?

	MOTION_DANCE_1,
	MOTION_DANCE_2,
	MOTION_DANCE_3,
	MOTION_DANCE_4,
	MOTION_DANCE_5,
	MOTION_DANCE_6,
	MOTION_DANCE_END = MOTION_DANCE_1 + 16,
	MOTION_CONGRATULATION,
	MOTION_FORGIVE,
	MOTION_ANGRY,
	MOTION_ATTRACTIVE,
	MOTION_SAD,
	MOTION_SHY,
	MOTION_CHEERUP,
	MOTION_BANTER,
	MOTION_JOY,
	MOTION_MAX_NUM,
};

enum MotionEventType
{
	MOTION_EVENT_TYPE_NONE = 0,
	MOTION_EVENT_TYPE_EFFECT = 1,
	MOTION_EVENT_TYPE_SCREEN_WAVING = 2,
	MOTION_EVENT_TYPE_SCREEN_FLASHING = 3,
	MOTION_EVENT_TYPE_SPECIAL_ATTACKING = 4,
	MOTION_EVENT_TYPE_SOUND = 5,
	MOTION_EVENT_TYPE_FLY = 6,
	MOTION_EVENT_TYPE_CHARACTER_SHOW = 7,
	MOTION_EVENT_TYPE_CHARACTER_HIDE = 8,
	MOTION_EVENT_TYPE_WARP = 9,
	MOTION_EVENT_TYPE_EFFECT_TO_TARGET = 10,
	MOTION_EVENT_TYPE_RELATIVE_MOVE_ON = 11,
	MOTION_EVENT_TYPE_RELATIVE_MOVE_OFF = 12,
	MOTION_EVENT_TYPE_MAX_NUM,
};

bool GetMotionModeString(storm::StringRef& s, uint32_t val);
bool GetMotionModeValue(const storm::StringRef& s, uint32_t& val);

bool GetMotionString(storm::StringRef& s, uint32_t val);
bool GetMotionValue(const storm::StringRef& s, uint32_t& val);
