#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "regen.h"
#include "mob_manager.h"
#include "dungeon.h"

LPREGEN	regen_list = nullptr;
LPREGEN_EXCEPTION regen_exception_list = nullptr;

enum ERegenModes
{
	MODE_TYPE = 0,
	MODE_SX,
	MODE_SY,
	MODE_EX,
	MODE_EY,
	MODE_Z_SECTION,
	MODE_DIRECTION,
	MODE_REGEN_TIME,
	MODE_REGEN_PERCENT,
	MODE_MAX_COUNT,
	MODE_VNUM
};

static bool get_word(FILE *fp, char* buf)
{
	int32_t i = 0;
	int32_t c;

	int32_t semicolon_mode = 0;

	while ((c = fgetc(fp)) != EOF)
	{
		if (i == 0)
		{
			if (c == '"')
			{
				semicolon_mode = 1;
				continue;
			}

			if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
				continue;
		}

		if (semicolon_mode)
		{
			if (c == '"')
			{
				buf[i] = '\0';
				return true;
			}

			buf[i++] = c;
		}
		else
		{
			if ((c == ' ' || c == '\t' || c == '\n' || c == '\r'))
			{
				buf[i] = '\0';
				return true;
			}

			buf[i++] = c;
		}

		if (i == 2 && buf[0] == '/' && buf[1] == '/')
		{
			buf[i] = '\0';
			return true;
		}
	}

	buf[i] = '\0';
	return (i != 0);
}

static void next_line(FILE *fp)
{
	int32_t c;

	while ((c = fgetc(fp)) != EOF)
		if (c == '\n')
			return;
}

static bool read_line(FILE *fp, LPREGEN regen)
{
	char szTmp[256];

	int32_t mode = MODE_TYPE;
	int32_t tmpTime;
	uint32_t i;

	while (get_word(fp, szTmp))
	{
		if (!strncmp(szTmp, "//", 2))
		{
			next_line(fp);
			continue;
		}

		switch (mode)
		{
			case MODE_TYPE:
				if (szTmp[0] == 'm')
					regen->type = REGEN_TYPE_MOB;
				else if (szTmp[0] == 'g')
				{
					regen->type = REGEN_TYPE_GROUP;

					if (szTmp[1] == 'a')
						regen->is_aggressive = true;
				}
				else if (szTmp[0] == 'e')
					regen->type = REGEN_TYPE_EXCEPTION;
				else if (szTmp[0] == 'r')
					regen->type = REGEN_TYPE_GROUP_GROUP;
				else if (szTmp[0] == 's')
					regen->type = REGEN_TYPE_ANYWHERE;
				else
				{
					SysLog("read_line: unknown regen type %c", szTmp[0]);
					exit(1);
				}

				++mode;
				break;

			case MODE_SX:
				str_to_number(regen->sx, szTmp);
				++mode;
				break;

			case MODE_SY:
				str_to_number(regen->sy, szTmp);
				++mode;
				break;

			case MODE_EX:
				{
					int32_t iX = 0;
					str_to_number(iX, szTmp);

					regen->sx -= iX;
					regen->ex = regen->sx + iX * 2;

					regen->sx *= 100;
					regen->ex *= 100;

					++mode;
				}
				break;

			case MODE_EY:
				{
					int32_t iY = 0;
					str_to_number(iY, szTmp);

					regen->sy -= iY;
					regen->ey = regen->sy + iY * 2;

					regen->sy *= 100;
					regen->ey *= 100;

					++mode;
				}
				break;

			case MODE_Z_SECTION:
				str_to_number(regen->z_section, szTmp);

				if (regen->type == REGEN_TYPE_EXCEPTION)
					return true;

				++mode;
				break;

			case MODE_DIRECTION:
				str_to_number(regen->direction, szTmp);
				++mode;
				break;

			case MODE_REGEN_TIME:
				regen->time = 0;
				tmpTime = 0;

				for (i = 0; i < strlen(szTmp); ++i)
				{
					switch (szTmp[i])
					{
						case 'h':
							regen->time += tmpTime * 3600;
							tmpTime = 0;
							break;

						case 'm':
							regen->time += tmpTime * 60;
							tmpTime = 0;
							break;

						case 's':
							regen->time += tmpTime;
							tmpTime = 0;
							break;

						default:
							if (szTmp[i] >= '0' && szTmp[i] <= '9')
							{
								tmpTime *= 10;
								tmpTime += (szTmp[i] - '0');
							}
					}
				}

				++mode;
				break;

			case MODE_REGEN_PERCENT:
				++mode;
				break;

			case MODE_MAX_COUNT:
				regen->count = 0;
				str_to_number(regen->max_count, szTmp);
				++mode;
				break;

			case MODE_VNUM:
				str_to_number(regen->vnum, szTmp);
				++mode;
				return true;
		}
	}

	return false;
}

bool is_regen_exception(int32_t x, int32_t y)
{
	LPREGEN_EXCEPTION exc;

	for (exc = regen_exception_list; exc; exc = exc->next)
	{
		if (exc->sx <= x && exc->sy <= y)
			if (exc->ex >= x && exc->ey >= y)
				return true;
	}

	return false;
}

static void regen_spawn_dungeon(LPREGEN regen, LPDUNGEON pDungeon, bool bOnce)
{
	uint32_t	num;
	uint32_t	i;

	num = (regen->max_count - regen->count);

	if (!num)
		return;

	for (i = 0; i < num; ++i)
	{
		LPCHARACTER ch = nullptr;

		if (regen->type == REGEN_TYPE_ANYWHERE)
		{
			ch = CHARACTER_MANAGER::GetInstance()->SpawnMobRandomPosition(regen->vnum, regen->lMapIndex);

			if (ch)
			{
				++regen->count;
				ch->SetDungeon(pDungeon);
			}
		}
		else if (regen->sx == regen->ex && regen->sy == regen->ey)
		{
			ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(regen->vnum,
					regen->lMapIndex,
					regen->sx,
					regen->sy,
					regen->z_section,
					false,
					regen->direction == 0 ? number(0, 7) * 45 : (regen->direction - 1) * 45);

			if (ch)
			{
				++regen->count;
				ch->SetDungeon(pDungeon);
			}
		}
		else
		{
			if (regen->type == REGEN_TYPE_MOB)
			{
				ch = CHARACTER_MANAGER::GetInstance()->SpawnMobRange(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, true);

				if (ch)
				{
					++regen->count;
					ch->SetDungeon(pDungeon);
				}
			}
			else if (regen->type == REGEN_TYPE_GROUP)
			{
				if (CHARACTER_MANAGER::GetInstance()->SpawnGroup(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, bOnce ? NULL : regen, regen->is_aggressive, pDungeon))
					++regen->count;
			}
			else if (regen->type == REGEN_TYPE_GROUP_GROUP)
			{
				if (CHARACTER_MANAGER::GetInstance()->SpawnGroupGroup(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, bOnce ? NULL : regen, regen->is_aggressive, pDungeon))
					++regen->count;
			}
		}

		if (ch && !bOnce)
			ch->SetRegen(regen);
	}
}

static void regen_spawn(LPREGEN regen, bool bOnce)
{
	uint32_t	num;
	uint32_t	i;

	num = (regen->max_count - regen->count);

	if (!num)
		return;

	for (i = 0; i < num; ++i)
	{
		LPCHARACTER ch = nullptr;

		if (regen->type == REGEN_TYPE_ANYWHERE)
		{
			ch = CHARACTER_MANAGER::GetInstance()->SpawnMobRandomPosition(regen->vnum, regen->lMapIndex);

			if (ch)
				++regen->count;
		}
		else if (regen->sx == regen->ex && regen->sy == regen->ey)
		{
			ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(regen->vnum,
					regen->lMapIndex,
					regen->sx,
					regen->sy,
					regen->z_section,
					false,
					regen->direction == 0 ? number(0, 7) * 45 : (regen->direction - 1) * 45);

			if (ch)
				++regen->count;
		}
		else
		{
			if (regen->type == REGEN_TYPE_MOB)
			{
				ch = CHARACTER_MANAGER::GetInstance()->SpawnMobRange(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, true, regen->is_aggressive, regen->is_aggressive);

				if (ch)
					++regen->count;
			}
			else if (regen->type == REGEN_TYPE_GROUP)
			{
				if (CHARACTER_MANAGER::GetInstance()->SpawnGroup(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, bOnce ? NULL : regen, regen->is_aggressive))
					++regen->count;
			}
			else if (regen->type == REGEN_TYPE_GROUP_GROUP)
			{
				if (CHARACTER_MANAGER::GetInstance()->SpawnGroupGroup(regen->vnum, regen->lMapIndex, regen->sx, regen->sy, regen->ex, regen->ey, bOnce ? NULL : regen, regen->is_aggressive))
					++regen->count;
			}
		}

		if (ch && !bOnce)
			ch->SetRegen(regen);
	}
}

EVENTFUNC(dungeon_regen_event)
{
	dungeon_regen_event_info* info = dynamic_cast<dungeon_regen_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("dungeon_regen_event> <Factor> Null pointer");
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Find(info->dungeon_id);
	if (pDungeon == nullptr) {
		return 0;
	}

	LPREGEN	regen = info->regen;
	if (regen->time == 0)
	{
		regen->event = nullptr;
	}

	regen_spawn_dungeon(regen, pDungeon, false);
	return PASSES_PER_SEC(regen->time);
}

bool regen_do(const char* filename, int32_t lMapIndex, int32_t base_x, int32_t base_y, LPDUNGEON pDungeon, bool bOnce)
{
	if (g_bNoRegen)
		return true;

	if (lMapIndex >= 114 && lMapIndex <= 117)
		return true;

	LPREGEN regen = nullptr;
	FILE* fp = fopen(filename, "rt");

	if (!fp)
	{
		SysLog("SYSTEM: regen_do: {}: file not found", filename);
		return false;
	}

	while (true)
	{
		REGEN tmp;

		memset(&tmp, 0, sizeof(tmp));

		if (!read_line(fp, &tmp))
			break;

		if (tmp.type == REGEN_TYPE_MOB ||
			tmp.type == REGEN_TYPE_GROUP ||
			tmp.type == REGEN_TYPE_GROUP_GROUP ||
			tmp.type == REGEN_TYPE_ANYWHERE)
		{
			if (!bOnce)
			{
				regen = M2_NEW REGEN;
				memcpy(regen, &tmp, sizeof(REGEN));
			}
			else
				regen = &tmp;

			if (pDungeon)
				regen->is_aggressive = true;

			regen->lMapIndex = lMapIndex;
			regen->count = 0;

			regen->sx += base_x;
			regen->ex += base_x;

			regen->sy += base_y;
			regen->ey += base_y;

			if (regen->sx > regen->ex)
			{
				regen->sx ^= regen->ex;
				regen->ex ^= regen->sx;
				regen->sx ^= regen->ex;
			}

			if (regen->sy > regen->ey)
			{
				regen->sy ^= regen->ey;
				regen->ey ^= regen->sy;
				regen->sy ^= regen->ey;
			}

			if (regen->type == REGEN_TYPE_MOB)
			{
				const CMob* p = CMobManager::GetInstance()->Get(regen->vnum);

				if (!p)
				{
					SysLog("No mob data by vnum {}", regen->vnum);
					if (!bOnce) {
						M2_DELETE(regen);
					}
					continue;
				}
			}

			if (!bOnce && pDungeon != nullptr)
			{
				dungeon_regen_event_info* info = AllocEventInfo<dungeon_regen_event_info>();

				info->regen = regen;
				info->dungeon_id = pDungeon->GetId();

				regen->event = event_create(dungeon_regen_event, info, PASSES_PER_SEC(number(0, 16)) + PASSES_PER_SEC(regen->time));

				pDungeon->AddRegen(regen);
				// regen_id should be determined at this point,
				// before the call to CHARACTER::SetRegen()
			}

			regen_spawn_dungeon(regen, pDungeon, bOnce);

		}
	}

	fclose(fp);
	return true;
}

bool regen_load_in_file(const char* filename, int32_t lMapIndex, int32_t base_x, int32_t base_y)
{
	if (g_bNoRegen)
		return true;

	LPREGEN regen = nullptr;
	FILE * fp = fopen(filename, "rt");

	if (!fp)
	{
		SysLog("SYSTEM: regen_do: {}: file not found", filename);
		return false;
	}

	while (true)
	{
		REGEN tmp;

		memset(&tmp, 0, sizeof(tmp));

		if (!read_line(fp, &tmp))
			break;

		if (tmp.type == REGEN_TYPE_MOB ||
			tmp.type == REGEN_TYPE_GROUP ||
			tmp.type == REGEN_TYPE_GROUP_GROUP ||
			tmp.type == REGEN_TYPE_ANYWHERE)
		{
			regen = &tmp;

			regen->is_aggressive = true;

			regen->lMapIndex = lMapIndex;
			regen->count = 0;

			regen->sx += base_x;
			regen->ex += base_x;

			regen->sy += base_y;
			regen->ey += base_y;

			if (regen->sx > regen->ex)
			{
				regen->sx ^= regen->ex;
				regen->ex ^= regen->sx;
				regen->sx ^= regen->ex;
			}

			if (regen->sy > regen->ey)
			{
				regen->sy ^= regen->ey;
				regen->ey ^= regen->sy;
				regen->sy ^= regen->ey;
			}

			if (regen->type == REGEN_TYPE_MOB)
			{
				const CMob* p = CMobManager::GetInstance()->Get(regen->vnum);

				if (!p)
				{
					SysLog("No mob data by vnum {}", regen->vnum);
					continue;
				}
			}

			regen_spawn(regen, true);
		}
	}

	fclose(fp);
	return true;
}

EVENTFUNC(regen_event)
{
	regen_event_info* info = dynamic_cast<regen_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("regen_event> <Factor> Null pointer");
		return 0;
	}

	LPREGEN	regen = info->regen;

	if (regen->time == 0)
		regen->event = nullptr;

	regen_spawn(regen, false);
	return PASSES_PER_SEC(regen->time);
}

bool regen_load(const char* filename, int32_t lMapIndex, int32_t base_x, int32_t base_y)
{
	if (g_bNoRegen)
		return true;

	LPREGEN regen = nullptr;
	FILE* fp = fopen(filename, "rt");

	if (!fp)
	{
		PyLog("SYSTEM: regen_load: {}: file not found", filename);
		return false;
	}

	while (true)
	{
		REGEN tmp;

		memset(&tmp, 0, sizeof(tmp));

		if (!read_line(fp, &tmp))
			break;

		if (tmp.type == REGEN_TYPE_MOB ||
			tmp.type == REGEN_TYPE_GROUP ||
			tmp.type == REGEN_TYPE_GROUP_GROUP ||
			tmp.type == REGEN_TYPE_ANYWHERE)
		{
			if (test_server)
			{
				CMobManager::GetInstance()->IncRegenCount(tmp.type, tmp.vnum, tmp.max_count, tmp.time);
			}

			regen = M2_NEW REGEN;
			memcpy(regen, &tmp, sizeof(REGEN));
			INSERT_TO_TW_LIST(regen, regen_list, prev, next);

			regen->lMapIndex = lMapIndex;
			regen->count = 0;

			regen->sx += base_x;
			regen->ex += base_x;

			regen->sy += base_y;
			regen->ey += base_y;

			if (regen->sx > regen->ex)
			{
				regen->sx ^= regen->ex;
				regen->ex ^= regen->sx;
				regen->sx ^= regen->ex;
			}

			if (regen->sy > regen->ey)
			{
				regen->sy ^= regen->ey;
				regen->ey ^= regen->sy;
				regen->sy ^= regen->ey;
			}

			if (regen->type == REGEN_TYPE_MOB)
			{
				const CMob* p = CMobManager::GetInstance()->Get(regen->vnum);

				if (!p)
				{
					SysLog("No mob data by vnum {}", regen->vnum);
				}
				else if (p->m_table.bType == CHAR_TYPE_NPC || p->m_table.bType == CHAR_TYPE_WARP || p->m_table.bType == CHAR_TYPE_GOTO)
				{
					SECTREE_MANAGER::GetInstance()->InsertNPCPosition(lMapIndex,
							p->m_table.bType,
							p->m_table.szLocaleName,
							(regen->sx+regen->ex) / 2 - base_x,
							(regen->sy+regen->ey) / 2 - base_y);
				}
			}

			if (regen->time != 0)
			{
				regen_spawn(regen, false);

				regen_event_info* info = AllocEventInfo<regen_event_info>();

				info->regen = regen;

				regen->event = event_create(regen_event, info, PASSES_PER_SEC(number(0, 16)) + PASSES_PER_SEC(regen->time)); 
			}
		}
		else if (tmp.type == REGEN_TYPE_EXCEPTION)
		{
			LPREGEN_EXCEPTION exc;

			exc = M2_NEW REGEN_EXCEPTION;

			exc->sx = tmp.sx;
			exc->sy = tmp.sy;
			exc->ex = tmp.ex;
			exc->ey = tmp.ey;
			exc->z_section = tmp.z_section;

			INSERT_TO_TW_LIST(exc, regen_exception_list, prev, next);
		}
	}

	fclose(fp);
	return true;
}

void regen_free(void)
{
	LPREGEN		regen, next_regen;
	LPREGEN_EXCEPTION	exc, next_exc;

	for (regen = regen_list; regen; regen = next_regen)
	{
		next_regen = regen->next;

		event_cancel(&regen->event);
		M2_DELETE(regen);
	}

	regen_list = nullptr;

	for (exc = regen_exception_list; exc; exc = next_exc)
	{
		next_exc = exc->next;

		M2_DELETE(exc);
	}

	regen_exception_list = nullptr;
}

void regen_reset(int32_t x, int32_t y)
{
	LPREGEN regen;

	for (regen = regen_list; regen; regen = regen->next)
	{
		if (!regen->event)
			continue;

		if (x != 0 || y != 0)
		{
			if (x >= regen->sx && x <= regen->ex)
				if (y >= regen->sy && y <= regen->ey)
					event_reset_time(regen->event, 1);
		}
		else
			event_reset_time(regen->event, 1);
	}
}

