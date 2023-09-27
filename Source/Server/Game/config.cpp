#include "stdafx.h"
#include <sstream>
#ifndef __WIN32__
#include <ifaddrs.h>
#endif

#include "constants.h"
#include "utils.h"
#include "log.h"
#include "desc.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "p2p.h"
#include "char.h"
#include "ip_ban.h"
#include "war_map.h"
#include "locale_service.h"
#include "config.h"
#include "db.h"
#include "skill_power.h"

using std::string;

uint8_t	g_bChannel = 0;
uint16_t	mother_port = 50080;
int32_t		passes_per_sec = 25;
uint16_t	db_port = 0;
uint16_t	p2p_port = 50900;
char	db_addr[ADDRESS_MAX_LEN + 1];
int32_t		save_event_second_cycle = passes_per_sec * 120;	
int32_t		ping_event_second_cycle = passes_per_sec * 60;
bool	g_bNoMoreClient = false;
bool	g_bNoRegen = false;

// TRAFFIC_PROFILER
bool		g_bTrafficProfileOn = false;
uint32_t		g_dwTrafficProfileFlushCycle = 3600;
// END_OF_TRAFFIC_PROFILER

bool			test_server = false;
bool			speed_server = false;

bool		distribution_test_server = false;
bool		china_event_server = false;
bool		guild_mark_server = true;
uint8_t		guild_mark_min_level = 3;
bool		no_wander = false;
int32_t		g_iUserLimit = 32768;

char		g_szInternalIP[16] = "0";
char		g_szPublicIP[16] = "0";
char		g_szBindIP[16] = "0";
bool		g_bPublicIPOverride = false; 

bool		g_bSkillDisable = false;
int32_t			g_iFullUserCount = 1200;
int32_t			g_iBusyUserCount = 650;
bool		g_bEmpireWhisper = true;
bool		g_bAuthServer = false;

string	g_stAuthMasterIP;
uint16_t		g_wAuthMasterPort = 0;

static std::set<uint32_t> s_set_dwFileCRC;
static std::set<uint32_t> s_set_dwProcessCRC;

string g_stHostname = "";
string g_table_postfix = "";

string g_stDefaultQuestObjectDir = "";
std::set<string> g_setQuestObjectDir;

std::vector<std::string>	g_stAdminPageIP;
std::string	g_stAdminPagePassword = "SHOWMETHEMONEY";

string g_stBlockDate = "30000705";

extern string g_stLocale;

int32_t SPEEDHACK_LIMIT_COUNT   = 50;
int32_t SPEEDHACK_LIMIT_BONUS   = 80;
int32_t g_iSyncHackLimitCount = 10;

int32_t VIEW_RANGE = 5000;
int32_t VIEW_BONUS_RANGE = 500;

int32_t g_server_id = 0;
string g_strWebMallURL = "www.metin2.de";

uint32_t g_uiSpamBlockDuration = 60 * 15;
uint32_t g_uiSpamBlockScore = 100;
uint32_t g_uiSpamReloadCycle = 60 * 10; 

bool		g_bCheckMultiHack = true;

int32_t			g_iSpamBlockMaxLevel = 10;

void		LoadStateUserCount();
void		LoadValidCRCList();
bool            g_protectNormalPlayer   = false;
bool            g_noticeBattleZone      = false;

int32_t gPlayerMaxLevel = 99;

bool g_BlockCharCreation = false;


int32_t		openid_server = 0;
char	openid_host[256];
char	openid_uri[256];

bool is_string_true(const char* string)
{
	int32_t	result = 0;
	if (isnhdigit(*string))
	{
		str_to_number(result, string);
		return result > 0 ? true : false;
	}
	else if (LOWER(*string) == 't')
		return true;
	else
		return false;
}

static std::set<int32_t> s_set_map_allows;

bool map_allow_find(int32_t index)
{
	if (g_bAuthServer)
		return false;

	if (s_set_map_allows.find(index) == s_set_map_allows.end())
		return false;

	return true;
}

void map_allow_log()
{
	std::set<int32_t>::iterator i;

	for (i = s_set_map_allows.begin(); i != s_set_map_allows.end(); ++i)
		PyLog("MAP_ALLOW: {}", *i);
}

void map_allow_add(int32_t index)
{
	if (map_allow_find(index))
	{
		PyLog("!!! FATAL ERROR !!! multiple MAP_ALLOW setting!!\n");
		exit(1);
	}

	PyLog("MAP ALLOW {}\n", index);
	s_set_map_allows.insert(index);
}

void map_allow_copy(int32_t* pl, int32_t size)
{
	int32_t iCount = 0;
	std::set<int32_t>::iterator it = s_set_map_allows.begin();

	while (it != s_set_map_allows.end())
	{
		int32_t i = *(it++);
		*(pl++) = i;

		if (++iCount > size)
			break;
	}
}

static void FN_add_adminpageIP(char* line)
{
	char* last = nullptr;
	const char* delim = " \t\r\n";
	char* v = strtok_r(line, delim, &last);

	while (v)
	{
		g_stAdminPageIP.push_back(v);
		v = strtok_r(NULL, delim, &last);
	}
}

static void FN_log_adminpage()
{
}


uint32_t IPToUInt(const std::string ip) {
	int32_t a, b, c, d;
	uint32_t addr = 0;

	if (sscanf(ip.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
		return 0;

	addr = a << 24;
	addr |= b << 16;
	addr |= c << 8;
	addr |= d;
	return addr;
}

bool IsIPInRange(const std::string ip, const std::string network, const std::string mask) {
	uint32_t ip_addr = IPToUInt(ip);
	uint32_t network_addr = IPToUInt(network);
	uint32_t mask_addr = IPToUInt(mask);

	uint32_t net_lower = (network_addr & mask_addr);
	uint32_t net_upper = (net_lower | (~mask_addr));

	if (ip_addr >= net_lower &&
		ip_addr <= net_upper)
		return true;
	return false;
}

bool GetIPInfo()
{
#ifndef __WIN32__
	struct ifaddrs* ifaddrp = nullptr;

	if (0 != getifaddrs(&ifaddrp))
		return false;

	for (struct ifaddrs* ifap = ifaddrp; NULL != ifap; ifap = ifap->ifa_next)
	{
		struct sockaddr_in* sai = (struct sockaddr_in*)ifap->ifa_addr;

		if (!ifap->ifa_netmask ||  // ignore if no netmask
			sai->sin_addr.s_addr == 0 || // ignore if address is 0.0.0.0
			sai->sin_addr.s_addr == 16777343) // ignore if address is 127.0.0.1
			continue;
#else
	WSADATA wsa_data;
		char host_name[100];
		HOSTENT* host_ent;
		int32_t n = 0;

		if (WSAStartup(0x0101, &wsa_data)) {
			return false;
		}

	gethostname(host_name, sizeof(host_name));
	host_ent = gethostbyname(host_name);
	if (host_ent == nullptr) {
		return false;
	}
	for (; host_ent->h_addr_list[n] != nullptr; ++n) {
		struct sockaddr_in addr;
		struct sockaddr_in* sai = &addr;
		memcpy(&sai->sin_addr.s_addr, host_ent->h_addr_list[n], host_ent->h_length);
#endif

		char* netip = inet_ntoa(sai->sin_addr);

		// Check if netip is a private address, according to IETF RFC 1918
		if (
			IsIPInRange(netip, "10.0.0.0", "255.0.0.0") ||
			IsIPInRange(netip, "172.16.0.0", "255.240.0.0") ||
			IsIPInRange(netip, "192.168.0.0", "255.255.0.0")
			) {

			// If we didn't allocate a private IP, allocate it now
			if (g_szInternalIP[0] == '0') {
				strlcpy(g_szInternalIP, netip, sizeof(g_szInternalIP));
#ifndef __WIN32__
				SysLog("INTERNAL_IP: {} interface {}\n", netip, ifap->ifa_name);
#else
				SysLog("INTERNAL_IP: {}\n", netip);
#endif
			}
		}

		// If it's not, then save it as a public IP
		else if (g_szPublicIP[0] == '0')
		{
			strlcpy(g_szPublicIP, netip, sizeof(g_szPublicIP));
#ifndef __WIN32__
			SysLog("PUBLIC_IP: {} interface {}\n", netip, ifap->ifa_name);
#else
			SysLog("PUBLIC_IP: {}\n", netip);
#endif
		}
	}

#ifndef __WIN32__
	freeifaddrs(ifaddrp);
#else
	WSACleanup();
#endif

	if (g_szInternalIP[0] == '0' && (g_szPublicIP[0] != '0' || g_bPublicIPOverride))
	{
		SysLog("Could not get internal IP! Will use public IP for binding instead!\n");
	}
	else if (g_szInternalIP[0] == '0' && g_bPublicIPOverride) 
	{
		SysLog("Could not get internal IP! Cannot use public IP override!\n");
		return false;
	}
	else if (g_szPublicIP[0] == '0' && g_bPublicIPOverride == false) {
		SysLog("Could not get public IP! Please specify a public IP override in the config file!\n");
		return false;
	}

	return true;
		}

void ConfigInit()
{
	FILE	*fp;

	char	buf[256];
	char	token_string[256];
	char	value_string[256];

	if (!(fp = fopen("CONFIG", "r")))
	{
		SysLog("Can not open CONFIG");
		exit(1);
	}

	char db_host[2][64], db_user[2][64], db_pwd[2][64], db_db[2][64];
	// ... Ah... db_port already exists... What should I do with the naming...
	int32_t mysql_db_port[2];

	for (int32_t n = 0; n < 2; ++n)
	{
		*db_host[n]	= '\0';
		*db_user[n] = '\0';
		*db_pwd[n]= '\0';
		*db_db[n]= '\0';
		mysql_db_port[n] = 0;
	}

	char log_host[64], log_user[64], log_pwd[64], log_db[64];
	int32_t log_port = 0;

	*log_host = '\0';
	*log_user = '\0';
	*log_pwd = '\0';
	*log_db = '\0';


	bool isCommonSQL = false;	
	bool isPlayerSQL = false;

	FILE* fpOnlyForDB;

	if (!(fpOnlyForDB = fopen("CONFIG", "r")))
	{
		SysLog("Can not open db CONFIG");
		exit(1);
	}

	while (fgets(buf, 256, fpOnlyForDB))
	{
		parse_token(buf, token_string, value_string);

		TOKEN("BLOCK_LOGIN")
		{
			g_stBlockDate = value_string;
		}

		TOKEN("adminpage_ip")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[0] = value_string;
		}

		TOKEN("adminpage_ip1")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[0] = value_string;
		}

		TOKEN("adminpage_ip2")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[1] = value_string;
		}

		TOKEN("adminpage_ip3")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[2] = value_string;
		}

		TOKEN("adminpage_password")
		{
			g_stAdminPagePassword = value_string;
		}

		TOKEN("hostname")
		{
			g_stHostname = value_string;
			PyLog("HOSTNAME: {}\n", g_stHostname.c_str());
			continue;
		}

		TOKEN("channel")
		{
			str_to_number(g_bChannel, value_string);
			continue;
		}

		TOKEN("player_sql")
		{
			const char* line = two_arguments(value_string, db_host[0], sizeof(db_host[0]), db_user[0], sizeof(db_user[0]));
			line = two_arguments(line, db_pwd[0], sizeof(db_pwd[0]), db_db[0], sizeof(db_db[0]));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(mysql_db_port[0], buf);
			}

			if (!*db_host[0] || !*db_user[0] || !*db_pwd[0] || !*db_db[0])
			{
				SysLog("PLAYER_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "PLAYER_SQL: %s %s %s %s %d", db_host[0], db_user[0], db_pwd[0], db_db[0], mysql_db_port[0]);
			isPlayerSQL = true;
			continue;
		}

		TOKEN("common_sql")
		{
			const char* line = two_arguments(value_string, db_host[1], sizeof(db_host[1]), db_user[1], sizeof(db_user[1]));
			line = two_arguments(line, db_pwd[1], sizeof(db_pwd[1]), db_db[1], sizeof(db_db[1]));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(mysql_db_port[1], buf);
			}

			if (!*db_host[1] || !*db_user[1] || !*db_pwd[1] || !*db_db[1])
			{
				SysLog("COMMON_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "COMMON_SQL: %s %s %s %s %d", db_host[1], db_user[1], db_pwd[1], db_db[1], mysql_db_port[1]);
			isCommonSQL = true;
			continue;
		}

		TOKEN("log_sql")
		{
			const char* line = two_arguments(value_string, log_host, sizeof(log_host), log_user, sizeof(log_user));
			line = two_arguments(line, log_pwd, sizeof(log_pwd), log_db, sizeof(log_db));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(log_port, buf);
			}

			if (!*log_host || !*log_user || !*log_pwd || !*log_db)
			{
				SysLog("LOG_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "LOG_SQL: %s %s %s %s %d", log_host, log_user, log_pwd, log_db, log_port);
			continue;
		}

		
		//OPENID		
		TOKEN("WEB_AUTH")
		{
			const char* line = two_arguments(value_string, openid_host, sizeof(openid_host), openid_uri, sizeof(openid_uri));

			if (!*openid_host || !*openid_uri)
			{
				SysLog("WEB_AUTH syntax error (ex: WEB_AUTH <host(metin2.co.kr) uri(/kyw/gameauth.php)>\n");
				exit(1);
			}

			char buf[1024];
			openid_server = 1;
			snprintf(buf, sizeof(buf), "WEB_AUTH: %s %s", openid_host, openid_uri);
			continue;
		}
	}

	// After processing is complete, close the file.
	fclose(fpOnlyForDB);

	// CONFIG_SQL_INFO_ERROR
	if (!isCommonSQL)
	{
		puts("LOAD_COMMON_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("COMMON_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	if (!isPlayerSQL)
	{
		puts("LOAD_PLAYER_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("PLAYER_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	AccountDB::GetInstance()->Connect(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1]);

	if (!AccountDB::GetInstance()->IsConnected())
	{
		SysLog("cannot start server while no common sql connected\n");
		exit(1);
	}

	PyLog("CommonSQL connected\n");

	PyLog("Setting DB to locale {}\n", g_stLocale.c_str());

	AccountDB::GetInstance()->SetLocale(g_stLocale);

	AccountDB::GetInstance()->ConnectAsync(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1], g_stLocale.c_str());

	DBManager::GetInstance()->Connect(db_host[0], mysql_db_port[0], db_user[0], db_pwd[0], db_db[0]);

	if (!DBManager::GetInstance()->IsConnected())
	{
		SysLog("PlayerSQL.ConnectError\n");
		exit(1);
	}

	PyLog("PlayerSQL connected\n");

	if (!g_bAuthServer) // If it is not an authentication server
	{
		LogManager::GetInstance()->Connect(log_host, log_port, log_user, log_pwd, log_db);

		if (!LogManager::GetInstance()->IsConnected())
		{
			SysLog("LogSQL.ConnectError\n");
			exit(1);
		}

		PyLog("LogSQL connected\n");

		LogManager::GetInstance()->BootLog(g_stHostname.c_str(), g_bChannel);
	}

	{
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT mValue FROM locale WHERE mKey='SKILL_POWER_BY_LEVEL'");
		std::unique_ptr<SQLMsg> pMsg(AccountDB::GetInstance()->DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows == 0)
		{
			SysLog("[SKILL_PERCENT] Query failed: {}", szQuery);
			exit(1);
		}

		MYSQL_ROW row; 

		row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		const char* p = row[0];
		int32_t cnt = 0;
		char num[128];
		int32_t aiBaseSkillPowerByLevelTable[SKILL_MAX_LEVEL+1];

		PyLog("SKILL_POWER_BY_LEVEL {}\n", p);
		while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
		{
			p = one_argument(p, num, sizeof(num));
			aiBaseSkillPowerByLevelTable[cnt++] = atoi(num);

			//PyLog("{} {}\n", cnt - 1, aiBaseSkillPowerByLevelTable[cnt - 1]);
			if (*p == '\0')
			{
				if (cnt != (SKILL_MAX_LEVEL + 1))
				{
					SysLog("[SKILL_PERCENT] locale table has not enough skill information! (count: {} query: {})", cnt, szQuery);
					exit(1);
				}

				PyLog("SKILL_POWER_BY_LEVEL: Done! (count {})\n", cnt);
				break;
			}
		}

		for (int32_t job = 0; job < JOB_MAX_NUM * 2; ++job)
		{
			snprintf(szQuery, sizeof(szQuery), "SELECT mValue from locale where mKey='SKILL_POWER_BY_LEVEL_TYPE%d' ORDER BY CAST(mValue AS unsigned)", job);
			std::unique_ptr<SQLMsg> pMsg(AccountDB::GetInstance()->DirectQuery(szQuery));

			if (pMsg->Get()->uiNumRows == 0)
			{
				CTableBySkill::GetInstance()->SetSkillPowerByLevelFromType(job, aiBaseSkillPowerByLevelTable);
				continue;
			}

			row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			cnt = 0;
			p = row[0];
			int32_t aiSkillTable[SKILL_MAX_LEVEL + 1];

			PyLog("SKILL_POWER_BY_JOB {} {}\n", job, p);
			while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
			{			
				p = one_argument(p, num, sizeof(num));
				aiSkillTable[cnt++] = atoi(num);

				if (*p == '\0')
				{
					if (cnt != (SKILL_MAX_LEVEL + 1))
					{
						SysLog("[SKILL_PERCENT] locale table has not enough skill information! (count: {} query: {})", cnt, szQuery);
						exit(1);
					}

					PyLog("SKILL_POWER_BY_JOB: Done! (job: {} count: {})\n", job, cnt);
					break;
				}
			}

			CTableBySkill::GetInstance()->SetSkillPowerByLevelFromType(job, aiSkillTable);
		}		
	}
	// END_SKILL_POWER_BY_LEVEL

	while (fgets(buf, 256, fp))
	{
		parse_token(buf, token_string, value_string);

		TOKEN("empire_whisper")
		{
			bool b_value = 0;
			str_to_number(b_value, value_string);
			g_bEmpireWhisper = !!b_value;
			continue;
		}

		TOKEN("mark_server")
		{
			guild_mark_server = is_string_true(value_string);
			continue;
		}

		TOKEN("mark_min_level")
		{
			str_to_number(guild_mark_min_level, value_string);
			guild_mark_min_level = MINMAX(0, guild_mark_min_level, GUILD_MAX_LEVEL);
			continue;
		}

		TOKEN("port")
		{
			str_to_number(mother_port, value_string);
			continue;
		}

		TOKEN("passes_per_sec")
		{
			str_to_number(passes_per_sec, value_string);
			continue;
		}

		TOKEN("p2p_port")
		{
			str_to_number(p2p_port, value_string);
			continue;
		}

		TOKEN("db_port")
		{
			str_to_number(db_port, value_string);
			continue;
		}

		TOKEN("db_addr")
		{
			strlcpy(db_addr, value_string, sizeof(db_addr));

			for (int32_t n =0; n < ADDRESS_MAX_LEN; ++n)
			{
				if (db_addr[n] == ' ')
					db_addr[n] = '\0';
			}

			continue;
		}

		TOKEN("save_event_second_cycle")
		{
			int32_t	cycle = 0;
			str_to_number(cycle, value_string);
			save_event_second_cycle = cycle* passes_per_sec;
			continue;
		}

		TOKEN("ping_event_second_cycle")
		{
			int32_t	cycle = 0;
			str_to_number(cycle, value_string);
			ping_event_second_cycle = cycle* passes_per_sec;
			continue;
		}

		TOKEN("table_postfix")
		{
			g_table_postfix = value_string;
			continue;
		}

		TOKEN("test_server")
		{
			printf("-----------------------------------------------\n");
			printf("TEST_SERVER\n");
			printf("-----------------------------------------------\n");
			test_server = is_string_true(value_string);
			continue;
		}

		TOKEN("speed_server")
		{
			printf("-----------------------------------------------\n");
			printf("SPEED_SERVER\n");
			printf("-----------------------------------------------\n");
			speed_server = is_string_true(value_string);
			continue;
		}

		TOKEN("distribution_test_server")
		{
			str_to_number(distribution_test_server, value_string);
			continue;
		}

		TOKEN("china_event_server")
		{
			str_to_number(china_event_server, value_string);
			continue;
		}

		TOKEN("shutdowned")
		{
			g_bNoMoreClient = true;
			continue;
		}

		TOKEN("no_regen")
		{
			g_bNoRegen = true;
			continue;
		}

		TOKEN("traffic_profile")
		{
			g_bTrafficProfileOn = true;
			continue;
		}


		TOKEN("map_allow")
		{
			char* p = value_string;
			string stNum;

			for (;* p; p++)
			{   
				if (isnhspace(*p))
				{
					if (stNum.length())
					{
						int32_t	index = 0;
						str_to_number(index, stNum.c_str());
						map_allow_add(index);
						stNum.clear();
					}
				}
				else
					stNum +=* p;
			}

			if (stNum.length())
			{
				int32_t	index = 0;
				str_to_number(index, stNum.c_str());
				map_allow_add(index);
			}

			continue;
		}

		TOKEN("no_wander")
		{
			no_wander = true;
			continue;
		}

		TOKEN("user_limit")
		{
			str_to_number(g_iUserLimit, value_string);
			continue;
		}

		TOKEN("skill_disable")
		{
			str_to_number(g_bSkillDisable, value_string);
			continue;
		}

		TOKEN("auth_server")
		{
			char szIP[32];
			char szPort[32];

			two_arguments(value_string, szIP, sizeof(szIP), szPort, sizeof(szPort));

			if (!*szIP || (!*szPort && strcasecmp(szIP, "master")))
			{
				SysLog("AUTH_SERVER: syntax error: <ip|master> <port>\n");
				exit(1);
			}

			g_bAuthServer = true;

			LoadBanIP("BANIP");

			if (!strcasecmp(szIP, "master"))
			{
				PyLog("AUTH_SERVER: I am the master\n");
			}
			else
			{
				g_stAuthMasterIP = szIP;
				str_to_number(g_wAuthMasterPort, szPort);

				PyLog("AUTH_SERVER: master {} {}\n", g_stAuthMasterIP.c_str(), g_wAuthMasterPort);
			}
			continue;
		}

		TOKEN("quest_dir")
		{
			PyLog("QUEST_DIR SETTING : {}", value_string);
			g_stQuestDir = value_string;
		}

		TOKEN("quest_object_dir")
		{
			//g_stQuestObjectDir = value_string;
			std::istringstream is(value_string);
			PyLog("QUEST_OBJECT_DIR SETTING : {}", value_string);
			string dir;
			while (!is.eof())
			{
				is >> dir;
				if (is.fail())
					break;
				g_setQuestObjectDir.insert(dir);
				PyLog("QUEST_OBJECT_DIR INSERT : {}", dir .c_str());
			}
		}

		TOKEN("synchack_limit_count")
		{
			str_to_number(g_iSyncHackLimitCount, value_string);
		}

		TOKEN("speedhack_limit_count")
		{
			str_to_number(SPEEDHACK_LIMIT_COUNT, value_string);
		}

		TOKEN("speedhack_limit_bonus")
		{
			str_to_number(SPEEDHACK_LIMIT_BONUS, value_string);
		}

		TOKEN("server_id")
		{
			str_to_number(g_server_id, value_string);
		}

		TOKEN("mall_url")
		{
			g_strWebMallURL = value_string;
		}

		TOKEN("bind_ip")
		{
			strlcpy(g_szPublicIP, value_string, sizeof(g_szPublicIP));
		}

		TOKEN("view_range")
		{
			str_to_number(VIEW_RANGE, value_string);
		}

		TOKEN("spam_block_duration")
		{
			str_to_number(g_uiSpamBlockDuration, value_string);
		}

		TOKEN("spam_block_score")
		{
			str_to_number(g_uiSpamBlockScore, value_string);
			g_uiSpamBlockScore = MAX(1, g_uiSpamBlockScore);
		}

		TOKEN("spam_block_reload_cycle")
		{
			str_to_number(g_uiSpamReloadCycle, value_string);
			g_uiSpamReloadCycle = MAX(60, g_uiSpamReloadCycle);
		}

		TOKEN("check_multihack")
		{
			str_to_number(g_bCheckMultiHack, value_string);
		}

		TOKEN("spam_block_max_level")
		{
			str_to_number(g_iSpamBlockMaxLevel, value_string);
		}
		TOKEN("protect_normal_player")
		{
			str_to_number(g_protectNormalPlayer, value_string);
		}
		TOKEN("notice_battle_zone")
		{
			str_to_number(g_noticeBattleZone, value_string);
		}

		TOKEN("pk_protect_level")
		{
		    str_to_number(PK_PROTECT_LEVEL, value_string);
		    SysLog("PK_PROTECT_LEVEL: {}", PK_PROTECT_LEVEL);
		}

		TOKEN("max_level")
		{
			str_to_number(gPlayerMaxLevel, value_string);

			gPlayerMaxLevel = MINMAX(1, gPlayerMaxLevel, PLAYER_MAX_LEVEL_CONST);

			SysLog("PLAYER_MAX_LEVEL: {}\n", gPlayerMaxLevel);
		}

		TOKEN("block_char_creation")
		{
			int32_t tmp = 0;

			str_to_number(tmp, value_string);

			if (0 == tmp)
				g_BlockCharCreation = false;
			else
				g_BlockCharCreation = true;

			continue;
		}
	}

	if (g_setQuestObjectDir.empty())
		g_setQuestObjectDir.insert(g_stDefaultQuestObjectDir);

	if (0 == db_port)
	{
		SysLog("DB_PORT not configured\n");
		exit(1);
	}

	if (0 == g_bChannel)
	{
		SysLog("CHANNEL not configured\n");
		exit(1);
	}

	if (g_stHostname.empty())
	{
		SysLog("HOSTNAME must be configured.\n");
		exit(1);
	}

	// LOCALE_SERVICE 
	LoadLocaleServiceSettings();
	// END_OF_LOCALE_SERVICE

	fclose(fp);

	if ((fp = fopen("CMD", "r")))
	{
		while (fgets(buf, 256, fp))
		{
			char cmd[32], levelname[32];
			int32_t level;

			two_arguments(buf, cmd, sizeof(cmd), levelname, sizeof(levelname));

			if (!*cmd || !*levelname)
			{
				SysLog("CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			if (!strcasecmp(levelname, "LOW_WIZARD"))
				level = GM_LOW_WIZARD;
			else if (!strcasecmp(levelname, "WIZARD"))
				level = GM_WIZARD;
			else if (!strcasecmp(levelname, "HIGH_WIZARD"))
				level = GM_HIGH_WIZARD;
			else if (!strcasecmp(levelname, "GOD"))
				level = GM_GOD;
			else if (!strcasecmp(levelname, "IMPLEMENTOR"))
				level = GM_IMPLEMENTOR;
			else if (!strcasecmp(levelname, "DISABLE"))
				level = GM_IMPLEMENTOR + 1;
			else
			{
				SysLog("CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			interpreter_set_privilege(cmd, level);
		}

		fclose(fp);
	}

	if (!GetIPInfo())
	{
		SysLog("Can not get public ip address\n");
		exit(1);
	}

	LoadValidCRCList();
	LoadStateUserCount();

	CWarMapManager::GetInstance()->LoadWarMapInfo(nullptr);

	FN_log_adminpage();
}

const char* get_table_postfix()
{
	return g_table_postfix.c_str();
}

void LoadValidCRCList()
{
	s_set_dwProcessCRC.clear();
	s_set_dwFileCRC.clear();

	FILE * fp;
	char buf[256];

	if ((fp = fopen("CRC", "r")))
	{
		while (fgets(buf, 256, fp))
		{
			if (!*buf)
				continue;

			uint32_t dwValidClientProcessCRC;
			uint32_t dwValidClientFileCRC;

			sscanf(buf, " %u %u ", &dwValidClientProcessCRC, &dwValidClientFileCRC);

			s_set_dwProcessCRC.insert(dwValidClientProcessCRC);
			s_set_dwFileCRC.insert(dwValidClientFileCRC);

			SysLog("CLIENT_CRC: {} {}\n", dwValidClientProcessCRC, dwValidClientFileCRC);
		}

		fclose(fp);
	}
}

void LoadStateUserCount()
{
	FILE * fp = fopen("state_user_count", "r");

	if (!fp)
		return;

	fscanf(fp, " %d %d ", &g_iFullUserCount, &g_iBusyUserCount);

	fclose(fp);
}

bool IsValidProcessCRC(uint32_t dwCRC)
{
	return s_set_dwProcessCRC.find(dwCRC) != s_set_dwProcessCRC.end();
}

bool IsValidFileCRC(uint32_t dwCRC)
{
	return s_set_dwFileCRC.find(dwCRC) != s_set_dwFileCRC.end();
}


