#include <Core/Race/RaceMotionConstants.hpp>
#include <storm/StringFlags.hpp>
#include <cctype>
#include <cwctype>
#include <clocale>

using uint_storm = storm::StringValueTable<uint32_t>;

uint_storm kMotionModes[] =
{
	{"RESERVED", MOTION_MODE_RESERVED},
	{"GENERAL", MOTION_MODE_GENERAL},
	{"ONEHAND_SWORD", MOTION_MODE_ONEHAND_SWORD},
	{"TWOHAND_SWORD", MOTION_MODE_TWOHAND_SWORD},
	{"DUALHAND_SWORD", MOTION_MODE_DUALHAND_SWORD},
	{"BOW", MOTION_MODE_BOW},
	{"FAN", MOTION_MODE_FAN},
	{"BELL", MOTION_MODE_BELL},
	{"FISHING", MOTION_MODE_FISHING},
	{"HORSE", MOTION_MODE_HORSE},
	{"HORSE_ONEHAND_SWORD", MOTION_MODE_HORSE_ONEHAND_SWORD},
	{"HORSE_TWOHAND_SWORD", MOTION_MODE_HORSE_TWOHAND_SWORD},
	{"HORSE_DUALHAND_SWORD", MOTION_MODE_HORSE_DUALHAND_SWORD},
	{"HORSE_BOW", MOTION_MODE_HORSE_BOW},
	{"HORSE_FAN", MOTION_MODE_HORSE_FAN},
	{"HORSE_BELL", MOTION_MODE_HORSE_BELL},
	{"WEDDING_DRESS", MOTION_MODE_WEDDING_DRESS},
};

uint_storm kMotions[] =
{
	{"SPAWN", MOTION_SPAWN},
	{"WAIT", MOTION_WAIT},
	{"WAIT1", MOTION_WAIT},
	{"WAIT2", MOTION_WAIT},
	{"WALK", MOTION_WALK},
	{"WALK1", MOTION_WALK},
	{"WALK2", MOTION_WALK},
	{"RUN", MOTION_RUN},
	{"RUN1", MOTION_RUN},
	{"RUN2", MOTION_RUN},
	{"STOP", MOTION_STOP},
	{"DEAD", MOTION_DEAD},
	{"COMBO_ATTACK", MOTION_COMBO_ATTACK_1},
	{"COMBO_ATTACK1", MOTION_COMBO_ATTACK_2},
	{"COMBO_ATTACK2", MOTION_COMBO_ATTACK_3},
	{"COMBO_ATTACK3", MOTION_COMBO_ATTACK_4},
	{"COMBO_ATTACK4", MOTION_COMBO_ATTACK_5},
	{"COMBO_ATTACK5", MOTION_COMBO_ATTACK_6},
	{"COMBO_ATTACK6", MOTION_COMBO_ATTACK_7},
	{"COMBO_ATTACK7", MOTION_COMBO_ATTACK_8},
	{"NORMAL_ATTACK", MOTION_NORMAL_ATTACK},
	{"FRONT_DAMAGE", MOTION_DAMAGE},
	{"FRONT_DEAD", MOTION_DEAD},
	{"FRONT_KNOCKDOWN", MOTION_DAMAGE_FLYING},
	{"FRONT_STANDUP", MOTION_STAND_UP},
	{"BACK_DAMAGE", MOTION_DAMAGE_BACK},
	{"BACK_DEAD", MOTION_DEAD_BACK},
	{"BACK_KNOCKDOWN", MOTION_DAMAGE_FLYING_BACK},
	{"BACK_STANDUP", MOTION_STAND_UP_BACK},
	{"SPECIAL", MOTION_SPECIAL_1},
	{"SPECIAL1", MOTION_SPECIAL_2},
	{"SPECIAL2", MOTION_SPECIAL_3},
	{"SPECIAL3", MOTION_SPECIAL_4},
	{"SPECIAL4", MOTION_SPECIAL_5},
	{"SPECIAL5", MOTION_SPECIAL_6},
	{"SKILL1", MOTION_SKILL + 121},
	{"SKILL2", MOTION_SKILL + 122},
	{"SKILL3", MOTION_SKILL + 123},
	{"SKILL4", MOTION_SKILL + 124},
	{"SKILL5", MOTION_SKILL + 125},
};

bool GetMotionModeString(storm::StringRef& s, uint32_t val)
{ 
	return storm::FormatValueWithTable(s, val, kMotionModes);
}

bool GetMotionModeValue(const storm::StringRef& s, uint32_t& val)
{ 
	return storm::ParseStringWithTable(s, val, kMotionModes); 
}

bool GetMotionString(storm::StringRef& s, uint32_t val)
{ 
	return storm::FormatValueWithTable(s, val, kMotions); 
}

bool GetMotionValue(const storm::StringRef& s, uint32_t& val)
{
	if (storm::ParseStringWithTable(s, val, kMotions))
		return true;

	// Motion names can have arbitrary numbers attached, causing the simple table-parsing to fail.
	// We need to manually remove them and retry.

	storm::StringRef newName(s);

	while (!newName.empty() && std::isdigit(newName.back()))
		newName.remove_suffix(1);

	return storm::ParseStringWithTable(newName, val, kMotions);
}
