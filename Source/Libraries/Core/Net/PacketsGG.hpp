#pragma once
#include <cstdint>
#include <Core/Constants/Controls.hpp>
#include <Common/length.h>

enum EPacketsGG : uint8_t
{
	HEADER_GG_LOGIN = 1,
	HEADER_GG_LOGOUT = 2,
	HEADER_GG_RELAY = 3,
	HEADER_GG_NOTICE = 4,
	HEADER_GG_SHUTDOWN = 5,
	HEADER_GG_GUILD = 6,
	HEADER_GG_DISCONNECT = 7,
	HEADER_GG_SHOUT = 8,
	HEADER_GG_SETUP = 9,
	HEADER_GG_MESSENGER_ADD = 10,
	HEADER_GG_MESSENGER_REMOVE = 11,
	HEADER_GG_FIND_POSITION = 12,
	HEADER_GG_WARP_CHARACTER = 13,
	HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX = 15,
	HEADER_GG_TRANSFER = 16,
	HEADER_GG_XMAS_WARP_SANTA = 17,
	HEADER_GG_XMAS_WARP_SANTA_REPLY = 18,
	HEADER_GG_RELOAD_CRC_LIST = 19,
	HEADER_GG_LOGIN_PING = 20,
	HEADER_GG_BLOCK_CHAT = 22,

	HEADER_GG_BLOCK_EXCEPTION = 24,
	HEADER_GG_SIEGE = 25,
	HEADER_GG_MONARCH_NOTICE = 26,
	HEADER_GG_MONARCH_TRANSFER = 27,

	HEADER_GG_CHECK_AWAKENESS = 29,
};

typedef struct SPacketGGSetup
{
	uint8_t	bHeader;
	uint16_t	wPort;
	uint8_t	bChannel;
} TPacketGGSetup;

typedef struct SPacketGGLogin
{
	uint8_t	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	uint32_t	dwPID;
	uint8_t	bEmpire;
	int32_t	lMapIndex;
	uint8_t	bChannel;
} TPacketGGLogin;

typedef struct SPacketGGLogout
{
	uint8_t	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGLogout;

typedef struct SPacketGGRelay
{
	uint8_t	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lSize;
} TPacketGGRelay;

typedef struct SPacketGGNotice
{
	uint8_t	bHeader;
	int32_t	lSize;
} TPacketGGNotice;

typedef struct SPacketGGMonarchNotice
{
	uint8_t	bHeader;
	uint8_t	bEmpire;
	int32_t	lSize;
} TPacketGGMonarchNotice;

typedef struct SPacketGGForkedMapInfo
{
	uint8_t	bHeader;
	uint8_t	bPass;
	uint8_t	bSungzi;
} TPacketGGForkedMapInfo;
typedef struct SPacketGGShutdown
{
	uint8_t	bHeader;
} TPacketGGShutdown;

typedef struct SPacketGGGuild
{
	uint8_t	bHeader;
	uint8_t	bSubHeader;
	uint32_t	dwGuild;
} TPacketGGGuild;

enum
{
	GUILD_SUBHEADER_GG_CHAT,
	GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS,
};

typedef struct SPacketGGGuildChat
{
	uint8_t	bHeader;
	uint8_t	bSubHeader;
	uint32_t	dwGuild;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGGuildChat;

typedef struct SPacketGGParty
{
	uint8_t	header;
	uint8_t	subheader;
	uint32_t	pid;
	uint32_t	leaderpid;
} TPacketGGParty;

enum
{
	PARTY_SUBHEADER_GG_CREATE,
	PARTY_SUBHEADER_GG_DESTROY,
	PARTY_SUBHEADER_GG_JOIN,
	PARTY_SUBHEADER_GG_QUIT,
};

typedef struct SPacketGGDisconnect
{
	uint8_t	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGDisconnect;

typedef struct SPacketGGShout
{
	uint8_t	bHeader;
	uint8_t	bEmpire;
	char	szText[CHAT_MAX_LEN + 1];
} TPacketGGShout;

typedef struct SPacketGGXmasWarpSanta
{
	uint8_t	bHeader;
	uint8_t	bChannel;
	int32_t	lMapIndex;
} TPacketGGXmasWarpSanta;

typedef struct SPacketGGXmasWarpSantaReply
{
	uint8_t	bHeader;
	uint8_t	bChannel;
} TPacketGGXmasWarpSantaReply;

typedef struct SPacketGGMessenger
{
	uint8_t        bHeader;
	char        szAccount[CHARACTER_NAME_MAX_LEN + 1];
	char        szCompanion[CHARACTER_NAME_MAX_LEN + 1];
} TPacketGGMessenger;

typedef struct SPacketGGFindPosition
{
	uint8_t header;
	uint32_t dwFromPID;
	uint32_t dwTargetPID;
} TPacketGGFindPosition;

typedef struct SPacketGGWarpCharacter
{
	uint8_t header;
	uint32_t pid;
	int32_t x;
	int32_t y;
} TPacketGGWarpCharacter;

typedef struct SPacketGGGuildWarMapIndex
{
	uint8_t bHeader;
	uint32_t dwGuildID1;
	uint32_t dwGuildID2;
	int32_t lMapIndex;
} TPacketGGGuildWarMapIndex;

typedef struct SPacketGGTransfer
{
	uint8_t	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lX, lY;
} TPacketGGTransfer;

typedef struct SPacketGGLoginPing
{
	uint8_t	bHeader;
	char	szLogin[LOGIN_MAX_LEN + 1];
} TPacketGGLoginPing;

typedef struct SPacketGGBlockChat
{
	uint8_t	bHeader;
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	int32_t	lBlockDuration;
} TPacketGGBlockChat;

typedef struct tag_GGSiege
{
	uint8_t	bHeader;
	uint8_t	bEmpire;
	uint8_t	bTowerCount;
} TPacketGGSiege;

typedef struct SPacketGGMonarchTransfer
{
	uint8_t	bHeader;
	uint32_t	dwTargetPID;
	int32_t	x;
	int32_t	y;
} TPacketMonarchGGTransfer;

typedef struct SPacketGGCheckAwakeness
{
	uint8_t bHeader;
} TPacketGGCheckAwakeness;
