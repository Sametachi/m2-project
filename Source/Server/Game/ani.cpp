#define _ani_cpp_

#include "stdafx.h"
#include "char.h"
#include "item.h"
#include "ani.h"

const char* FN_race_name(int32_t race)
{
#define FN_NAME(race)	case race: return #race
	switch (race)
	{
		FN_NAME(MAIN_RACE_WARRIOR_M);
		FN_NAME(MAIN_RACE_ASSASSIN_W);
		FN_NAME(MAIN_RACE_SURA_M);
		FN_NAME(MAIN_RACE_SHAMAN_W);
		FN_NAME(MAIN_RACE_WARRIOR_W);
		FN_NAME(MAIN_RACE_ASSASSIN_M);
		FN_NAME(MAIN_RACE_SURA_W);
		FN_NAME(MAIN_RACE_SHAMAN_M);
		FN_NAME(MAIN_RACE_MAX_NUM);
	}

	return "UNKNOWN";
#undef FN_NAME
}

const char* FN_weapon_type(int32_t weapon)
{
#define FN_NAME(weapon)	case weapon: return #weapon
	switch (weapon)
	{
		FN_NAME(ITEM::WEAPON_SWORD);
		FN_NAME(ITEM::WEAPON_DAGGER);
		FN_NAME(ITEM::WEAPON_BOW);
		FN_NAME(ITEM::WEAPON_TWO_HANDED);
		FN_NAME(ITEM::WEAPON_BELL);
		FN_NAME(ITEM::WEAPON_FAN);
		FN_NAME(ITEM::WEAPON_ARROW);
		FN_NAME(ITEM::WEAPON_NUM_TYPES);
	}

	return "UNKNOWN";
#undef FN_NAME
}

class ANI
{
	protected:
		// [RACE][NORMAL-0, MOUNT-1][WEAPON][COMBO]
		uint32_t m_speed[MAIN_RACE_MAX_NUM][2][ITEM::WEAPON_NUM_TYPES][9];

	public:
		ANI();

	public:
		bool	load();
		bool	load_one_race(int32_t race, const char* dir_name);
		uint32_t	load_one_weapon(const char* dir_name, int32_t weapon, uint8_t combo, bool horse);
		uint32_t	attack_speed(int32_t race, int32_t weapon, uint8_t combo = 0, bool horse = false);

		void	print_attack_speed();
};

static class ANI s_ANI;

uint32_t FN_attack_speed_from_file(const char* file)
{
	FILE * fp = fopen(file, "r");

	if (!fp)
		return 0;

	int32_t speed = 1000;

	const char* key	= "DirectInputTime";
	const char* delim	= " \t\r\n";
	const char* field, *value;

	char buf[1024];

	while (fgets(buf, 1024, fp))
	{
		field	= strtok(buf, delim);
		value	= strtok(NULL, delim);

		if (field && value)
		{
			if (0 == strcasecmp(field, key))
			{
				float f_speed = strtof(value, NULL);
				speed = (int32_t) (f_speed * 1000.0);
				break;
			}
		}
	}

	fclose(fp);
	return speed;
}

ANI::ANI()
{
	// set default value
	for (int32_t race = 0; race < MAIN_RACE_MAX_NUM; ++race)
	{
		for (int32_t weapon = 0; weapon < ITEM::WEAPON_NUM_TYPES; ++weapon)
		{
			for (uint8_t combo = 0; combo <= 8; ++combo)
			{
				m_speed[race][0][weapon][combo] = 1000;
				m_speed[race][1][weapon][combo] = 1000;
			}
		}
	}
}

bool ANI::load()
{
	const char*	dir_name[MAIN_RACE_MAX_NUM] = {
		"data/pc/warrior",		// Warrior (Male)
		"data/pc/assassin",		// Ninja (Female)
		"data/pc/sura",			// Sura (Male)
		"data/pc/shaman",		// Shaman (Female)
		"data/pc2/warrior",		// Warrior (Female)
		"data/pc2/assassin",	// Ninja (Male)
		"data/pc2/sura",		// Sura (Female)
		"data/pc2/shaman"		// Shaman (Male)
	};

	for (int32_t race = 0; race <MAIN_RACE_MAX_NUM; ++race)
	{
		if (!load_one_race(race, dir_name[race]))
		{
			SysLog("ANI directory = {}", dir_name[race]);
			return false;
		}
	}

	return true;
}

uint32_t ANI::load_one_weapon(const char* dir_name, int32_t weapon, uint8_t combo, bool horse)
{
	char format[128];
	char filename[256];

	switch (weapon)
	{
		case ITEM::WEAPON_SWORD:
			strlcpy(format, "%s/%sonehand_sword/combo_%02d.msa", sizeof(format));
			break;

		case ITEM::WEAPON_DAGGER:
			strlcpy(format, "%s/%sdualhand_sword/combo_%02d.msa", sizeof(format));
			break;

		case ITEM::WEAPON_BOW:
			strlcpy(format, "%s/%sbow/attack.msa", sizeof(format));
			break;

		case ITEM::WEAPON_TWO_HANDED:
			strlcpy(format, "%s/%stwohand_sword/combo_%02d.msa", sizeof(format));
			break;

		case ITEM::WEAPON_BELL:
			strlcpy(format, "%s/%sbell/combo_%02d.msa", sizeof(format));
			break;

		case ITEM::WEAPON_FAN:
			strlcpy(format, "%s/%sfan/combo_%02d.msa", sizeof(format));
			break;

		default:
			return 1000;
	}

	snprintf(filename, sizeof(filename), format, dir_name, horse ? "horse_" : "", combo);
	uint32_t speed = FN_attack_speed_from_file(filename);

	if (speed == 0)
		return 1000;

	return speed;
}

bool ANI::load_one_race(int32_t race, const char* dir_name)
{
	if (!dir_name || '\0' == dir_name[0])
		return false;

	for (int32_t weapon = ITEM::WEAPON_SWORD; weapon < ITEM::WEAPON_NUM_TYPES; ++weapon)
	{
		for (uint8_t combo = 1; combo <= 8; ++combo)
		{
			// Normal (not riding)
			m_speed[race][0][weapon][combo] = load_one_weapon(dir_name, weapon, combo, false);
			m_speed[race][0][weapon][0] = MIN(m_speed[race][0][weapon][0], m_speed[race][0][weapon][combo]); // Min

			// Riding / Mounting
			m_speed[race][1][weapon][combo] = load_one_weapon(dir_name, weapon, combo, true);
			m_speed[race][1][weapon][0] = MIN(m_speed[race][1][weapon][0], m_speed[race][1][weapon][combo]); // Min
		}
	}

	return true;
}

uint32_t ANI::attack_speed(int32_t race, int32_t weapon, uint8_t combo, bool horse)
{
	switch (race)
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_ASSASSIN_W:
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_SHAMAN_W:
		case MAIN_RACE_WARRIOR_W:
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_SURA_W:
		case MAIN_RACE_SHAMAN_M:
			break;
		default:
			return 1000;
	}

	switch (weapon)
	{
		case ITEM::WEAPON_SWORD:
		case ITEM::WEAPON_DAGGER:
		case ITEM::WEAPON_BOW:
		case ITEM::WEAPON_TWO_HANDED:
		case ITEM::WEAPON_BELL:
		case ITEM::WEAPON_FAN:
		case ITEM::WEAPON_ARROW:
			break;
		default:
			return 1000;
	}

	return m_speed[race][horse ? 1 : 0][weapon][combo];
}

const char* FN_race_string(int32_t race)
{
	switch (race)
	{
		case MAIN_RACE_WARRIOR_M:	return "WARRIOR_M";
		case MAIN_RACE_ASSASSIN_W:	return "ASSASSIN_W";
		case MAIN_RACE_SURA_M:		return "SURA_M";
		case MAIN_RACE_SHAMAN_W:	return "SHAMAN_W";

		case MAIN_RACE_WARRIOR_W:	return "WARRIOR_W";
		case MAIN_RACE_ASSASSIN_M:	return "ASSASSIN_M";
		case MAIN_RACE_SURA_W:		return "SURA_W";
		case MAIN_RACE_SHAMAN_M:	return "SHAMAN_M";
	}

	return "UNKNOWN_RACE";
}

const char* FN_weapon_string(int32_t weapon)
{
	switch (weapon)
	{
		case ITEM::WEAPON_SWORD:		return "SWORD";
		case ITEM::WEAPON_DAGGER:		return "DAGGER";
		case ITEM::WEAPON_BOW:		return "BOW";
		case ITEM::WEAPON_TWO_HANDED:	return "TWO_HANDED";
		case ITEM::WEAPON_BELL:		return "BELL";
		case ITEM::WEAPON_FAN:		return "FAN";
		case ITEM::WEAPON_ARROW:		return "ARROW";
	}

	return "UNKNOWN";
}

void ANI::print_attack_speed()
{
	for (int32_t race = 0; race < MAIN_RACE_MAX_NUM; ++race)
	{
		for (int32_t weapon = 0; weapon < ITEM::WEAPON_NUM_TYPES; ++weapon)
		{
			printf("[%s][%s] = %u\n",
					FN_race_string(race),
					FN_weapon_string(weapon),
					attack_speed(race, weapon));
		}
		printf("\n");
	}
}

void ani_init()
{
	s_ANI.load();
}

uint32_t ani_attack_speed(LPCHARACTER ch)
{
	uint32_t speed = 1000;

	if (!ch)
		return speed;

	LPITEM item = ch->GetWear(WEAR_WEAPON);

	if (!item)
		return speed;

	if (ITEM::TYPE_WEAPON != item->GetType())
		return speed;

	int32_t race = ch->GetRaceNum();
	int32_t weapon = item->GetSubType();

	/*
	dev_log(LOG_DEB0, "%s : (race,weapon) = (%s,%s) POINT_ATT_SPEED = %d",
			ch->GetName(),
			FN_race_name(race),
			FN_weapon_type(weapon),
			ch->GetPoint(POINT_ATT_SPEED));
	*/

	/* In the case of a two-handed sword, during triple attack and horseback riding */
	/* There are a lot of errors, so let's think at the speed of a one-handed sword */
	if (weapon == ITEM::WEAPON_TWO_HANDED)
		weapon = ITEM::WEAPON_SWORD;

	return s_ANI.attack_speed(race, weapon);
}

uint32_t ani_combo_speed(LPCHARACTER ch, uint8_t combo)
{
	LPITEM item = ch->GetWear(WEAR_WEAPON);

	if (!item || combo > 8)
		return 1000;

	return s_ANI.attack_speed(ch->GetRaceNum(), item->GetSubType(), combo, ch->IsRiding());
}

void ani_print_attack_speed()
{
	s_ANI.print_attack_speed();
}

#if 0
int32_t main(int32_t argc, char* *argv)
{
	ani_init();
	ani_print_attack_speed();
	exit(0);
}
#endif
