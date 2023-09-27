#pragma once

enum
{
	ADDRESS_MAX_LEN = 15
};

void ConfigInit(); // default "" is CONFIG

extern char sql_addr[256];

extern uint16_t mother_port;
extern uint16_t p2p_port;

extern char db_addr[ADDRESS_MAX_LEN + 1];
extern uint16_t db_port;

extern int32_t passes_per_sec;
extern int32_t save_event_second_cycle;
extern int32_t ping_event_second_cycle;
extern bool test_server;
extern bool	guild_mark_server;
extern uint8_t guild_mark_min_level;
extern bool	distribution_test_server;
extern bool	china_event_server;

extern bool	g_bNoMoreClient;
extern bool	g_bNoRegen;

extern bool	g_bTrafficProfileOn;

extern uint8_t	g_bChannel;

extern bool	map_allow_find(int32_t index);
extern void	map_allow_copy(int32_t* pl, int32_t size);
extern bool	no_wander;

extern int32_t	g_iUserLimit;
extern time_t	g_global_time;

const char* 	get_table_postfix();

extern std::string	g_stHostname;
extern std::string	g_stLocale;
extern std::string	g_stLocaleFilename;

extern char		g_szPublicIP[16];
extern char		g_szInternalIP[16];
extern char		g_szBindIP[16];
extern bool		g_bPublicIPOverride;

extern int32_t (*is_twobyte) (const char* str);
extern int32_t (*check_name) (const char* str);

extern bool		g_bSkillDisable;

extern int32_t		g_iFullUserCount;
extern int32_t		g_iBusyUserCount;
extern void		LoadStateUserCount();

extern bool	g_bEmpireWhisper;

extern bool	g_bAuthServer;

extern uint8_t	PK_PROTECT_LEVEL;

extern void	LoadValidCRCList();
extern bool	IsValidProcessCRC(uint32_t dwCRC);
extern bool	IsValidFileCRC(uint32_t dwCRC);

extern std::string	g_stAuthMasterIP;
extern uint16_t		g_wAuthMasterPort;

extern std::string	g_stQuestDir;
extern std::set<std::string> g_setQuestObjectDir;


extern std::vector<std::string>	g_stAdminPageIP;
extern std::string	g_stAdminPagePassword;

extern int32_t	SPEEDHACK_LIMIT_COUNT;
extern int32_t 	SPEEDHACK_LIMIT_BONUS;

extern int32_t g_iSyncHackLimitCount;

extern int32_t g_server_id;
extern std::string g_strWebMallURL;

extern int32_t VIEW_RANGE;
extern int32_t VIEW_BONUS_RANGE;

extern bool g_bCheckMultiHack;
extern bool g_protectNormalPlayer;
extern bool g_noticeBattleZone;

extern uint32_t g_GoldDropTimeLimitValue;

extern int32_t gPlayerMaxLevel;

extern bool g_BlockCharCreation;

