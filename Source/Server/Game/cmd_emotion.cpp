#include "stdafx.h"
#include <Core/Net/PacketsGC.hpp>
#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "wedding.h"

#define NEED_TARGET	(1 << 0)
#define NEED_PC		(1 << 1)
#define WOMAN_ONLY	(1 << 2)
#define OTHER_SEX_ONLY	(1 << 3)
#define SELF_DISARM	(1 << 4)
#define TARGET_DISARM	(1 << 5)
#define BOTH_DISARM	(SELF_DISARM | TARGET_DISARM)

struct emotion_type_s
{
	const char* command;
	const char* command_to_client;
	int32_t	flag;
	float	extra_delay;
} emotion_types[] = {
	{ "french_kiss",		"french_kiss",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		2.0f },
	{ "kiss",				"kiss",				NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		1.5f },
	{ "slap",				"slap",				NEED_PC | SELF_DISARM,						1.5f },

	{ "clap",				"clap",				0,				1.0f },
	{ "cheer1",				"cheer1",			0,				1.0f },
	{ "cheer2",				"cheer2",			0,				1.0f },

	{ "dance1",				"dance1",			0,				1.0f },
	{ "dance2",				"dance2",			0,				1.0f },
	{ "dance3",				"dance3",			0,				1.0f },
	{ "dance4",				"dance4",			0,				1.0f },
	{ "dance5",				"dance5",			0,				1.0f },
	{ "dance6",				"dance6",			0,				1.0f },

	{ "congratulation",		"congratulation",	0,				1.0f	},
	{ "forgive",			"forgive",			0,				1.0f	},
	{ "angry",				"angry",			0,				1.0f	},
	{ "attractive",			"attractive",		0,				1.0f	},
	{ "sad",				"sad",				0,				1.0f	},
	{ "shy",				"shy",				0,				1.0f	},
	{ "cheerup",			"cheerup",			0,				1.0f	},
	{ "banter",				"banter",			0,				1.0f	},
	{ "joy",				"joy",				0,				1.0f	},
	{ "\n",					"\n",				0,				0.0f },
};


std::set<std::pair<uint32_t, uint32_t> > s_emotion_set;

ACMD(do_emotion_allow)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use this in the duel arena."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	uint32_t	val = 0; str_to_number(val, arg1);
	s_emotion_set.insert(std::make_pair(ch->GetVID(), val));
}

bool CHARACTER_CanEmotion(CHARACTER& rch)
{
	if (marriage::WeddingManager::GetInstance()->IsWeddingMap(rch.GetMapIndex()))
		return true;

	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK))
		return true;

	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK2))
		return true;

	return false;
}

ACMD(do_emotion)
{
	int32_t i;
	{
		if (ch->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot express emotions whilst riding a horse."));
			return;
		}
	}

	for (i = 0; *emotion_types[i].command != '\n'; ++i)
	{
		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command))
			break;

		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command_to_client))
			break;
	}

	if (*emotion_types[i].command == '\n')
	{
		SysLog("cannot find emotion");
		return;
	}

	if (!CHARACTER_CanEmotion(*ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You can do this when you wear an Emotion Mask."));
		return;
	}

	if (IS_SET(emotion_types[i].flag, WOMAN_ONLY) && SEX_MALE==GET_SEX(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Only women can do this."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER victim = nullptr;

	if (*arg1)
		victim = ch->FindCharacterInView(arg1, IS_SET(emotion_types[i].flag, NEED_PC));

	if (IS_SET(emotion_types[i].flag, NEED_TARGET | NEED_PC))
	{
		if (!victim)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This person does not exist."));
			return;
		}
	}

	if (victim)
	{
		if (!victim->IsPC() || victim == ch)
			return;

		if (victim->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You cannot use emotions with a player who is riding on a Horse."));
			return;
		}

		int32_t distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

		if (distance < 10)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too near."));
			return;
		}

		if (distance > 500)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You are too far away."));
			return;
		}

		if (IS_SET(emotion_types[i].flag, OTHER_SEX_ONLY))
		{
			if (GET_SEX(ch)==GET_SEX(victim))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("This action can only be done with another gender."));
				return;
			}
		}

		if (IS_SET(emotion_types[i].flag, NEED_PC))
		{
			if (s_emotion_set.find(std::make_pair(victim->GetVID(), ch->GetVID())) == s_emotion_set.end())
			{
				if (marriage::CManager::GetInstance()->IsMarried(ch->GetPlayerID()))
				{
					const marriage::TMarriage* marriageInfo = marriage::CManager::GetInstance()->Get(ch->GetPlayerID());

					const uint32_t other = marriageInfo->GetOther(ch->GetPlayerID());

					if (0 == other || other != victim->GetPlayerID())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need your fellow player's approval for this."));
						return;
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You need your fellow player's approval for this."));
					return;
				}
			}
			s_emotion_set.insert(std::make_pair(ch->GetVID(), victim->GetVID()));
		}
	}

	char chatbuf[256+1];
	int32_t len = snprintf(chatbuf, sizeof(chatbuf), "%s %u %u", 
			emotion_types[i].command_to_client,
			(uint32_t) ch->GetVID(), victim ? (uint32_t) victim->GetVID() : 0);

	if (len < 0 || len >= (int32_t) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	++len;

	TPacketGCChat pack_chat;
	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = CHAT_TYPE_COMMAND;
	pack_chat.id = 0;
	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(TPacketGCChat));
	buf.write(chatbuf, len);

	ch->PacketAround(buf.read_peek(), buf.size());

	if (victim)
	{
		TraceLog("ACTION: {} TO {}", emotion_types[i].command, victim->GetName());
	}
	else
		TraceLog("ACTION: {}", emotion_types[i].command);
}

