#include "stdafx.h"
#include <csignal>
#include "Config.h"
#include "Peer.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "GuildManager.h"
#include "HB.h"
#include "PrivManager.h"
#include "MoneyLog.h"
#include "Marriage.h"
#include "Monarch.h"
#include "ItemIDRangeManager.h"
#include <Basic/SimpleApp.hpp>

class DatabaseServer : public SimpleApp
{
public:
	DatabaseServer() = default;
	~DatabaseServer()
	{
		// Destroy everything
		SetVirtualStepFileSyster(nullptr);
	}

	int32_t		Run(int32_t argc, const char** argv);
	bool		Setup();

	FileSystem			m_FileSystem;
	ArchiveFSProvider	m_FSPack;
	VirtualDiskProvider	m_FSDisk;

	CConfig				m_Config;
	CNetPoller			m_Poller;
	CDBManager			m_DBManager;
	CClientManager		m_ClientManager;
	CPlayerHB			m_PlayerHB;
	CGuildManager		m_GuildManager;
	CPrivManager		m_PrivManager;
	CMoneyLog			m_MoneyLog;
	CMarriageManager	m_MarriageManager;
	CMonarch			m_Monarch;
	CItemIDRangeManager	m_ItemIDRangeManager;
};

void SetPlayerDBName(const char* c_pszPlayerDBName);
void SetTablePostfix(const char* c_pszTablePostfix);

std::string g_stTablePostfix;
std::string g_stLocaleNameColumn = "name";
std::string g_stLocale = "uk";
std::string g_stPlayerDBName = "";

bool g_bHotBackup = false;

int32_t g_iPlayerCacheFlushSeconds = 60*7;
int32_t g_iItemCacheFlushSeconds = 60*5;
int32_t g_iItemPriceListTableCacheFlushSeconds = 60*9;

//g_iLogoutSeconds value must be longer than other cache flush values
int32_t g_iLogoutSeconds = 60*10;

extern void WriteVersion();

void emptybeat(LPHEART heart, int32_t pulse)
{
	if (!(pulse % heart->passes_per_sec))
	{
	}
}

void SetTablePostfix(const char* c_pszTablePostfix)
{
	if (!c_pszTablePostfix || !*c_pszTablePostfix)
		g_stTablePostfix = "";
	else
		g_stTablePostfix = c_pszTablePostfix;
}

const char* GetTablePostfix()
{
	return g_stTablePostfix.c_str();
}

void SetPlayerDBName(const char* c_pszPlayerDBName)
{
	if (!c_pszPlayerDBName || !*c_pszPlayerDBName)
		g_stPlayerDBName = "";
	else
	{
		g_stPlayerDBName = c_pszPlayerDBName;
		g_stPlayerDBName += ".";
	}
}

const char* GetPlayerDBName()
{
	return g_stPlayerDBName.c_str();
}

int32_t DatabaseServer::Run(int32_t argc, const char** argv)
{
	WriteVersion();

	Logger log(ProviderType::G3log);

#ifdef _WIN32
	SetConsoleCtrlHandler([](DWORD code) -> BOOL {
		thecore_shutdown();
		return TRUE;
	}, TRUE);
#endif

#ifdef _DEBUG
	log.SetLevel(Log::LogLevel::Trace);
#else
	log.SetLevel(Log::LogLevel::Info);
#endif

	if (!log.Initialize(true
#ifdef _DEBUG
		, true, true
#endif
	))
	{
		fprintf(stderr, "Log init fail, exiting...\n");
		return -1;
	}

	// First setup VFS then set the Providers.
	SetVirtualStepFileSyster(&m_FileSystem);
	m_FileSystem.RegisterProvider(&m_FSPack);

	// If we allow loading from Disk,
	// then we silence missing file warnings
	// and then @Register the pc's hdd/ssd as disk
	m_FileSystem.ReadDiskFiles(true);
	if (CanReadDiskFiles)
	{
		// Allow VFS to load files outside of Archives.
		m_FileSystem.LogMissingFiles(false);
		m_FileSystem.RegisterProvider(&m_FSDisk);
	}
	else
		m_FileSystem.LogMissingFiles(true);

	PyLog("DBCacheServer Start");


	if (!Setup())
		return 1;

	m_GuildManager.Initialize();
	m_MarriageManager.Initialize();
	m_ItemIDRangeManager.Build();

	CClientManager::GetInstance()->MainLoop();

	signal_timer_disable();

	m_DBManager.Quit();
	int32_t iCount;

	while (true)
	{
		iCount = 0;

		iCount += CDBManager::GetInstance()->CountReturnQuery(SQL_PLAYER);
		iCount += CDBManager::GetInstance()->CountAsyncQuery(SQL_PLAYER);

		if (iCount == 0)
			break;

		usleep(1000);
		TraceLog("WAITING_QUERY_COUNT {}", iCount);
	}

	return 1;
}

bool DatabaseServer::Setup()
{
	if (!CConfig::GetInstance()->LoadFile("CONFIG"))
	{
		FatalLog("Loading CONFIG failed.");
		return false;
	}

	int32_t heart_beat = 50;
	if (!CConfig::GetInstance()->GetValue("CLIENT_HEART_FPS", &heart_beat))
	{
		FatalLog("Cannot find CLIENT_HEART_FPS configuration.");
		return false;
	}

	thecore_init(heart_beat, emptybeat);
	signal_timer_enable(60);

	char szBuf[256 + 1];

	if (CConfig::GetInstance()->GetValue("LOCALE", szBuf, 256))
	{
		g_stLocale = szBuf;
		PyLog("LOCALE set to {}", g_stLocale);
	}

	int32_t iDisableHotBackup;
	if (CConfig::GetInstance()->GetValue("DISABLE_HOTBACKUP", &iDisableHotBackup))
	{
		if (iDisableHotBackup)
		{
			PyLog("CONFIG: DISABLE_HOTBACKUP");
			g_bHotBackup = false;
		}
	}


	if (!CConfig::GetInstance()->GetValue("TABLE_POSTFIX", szBuf, 256))
	{
		WarnLog("TABLE_POSTFIX not configured use default");
		szBuf[0] = '\0';
	}

	SetTablePostfix(szBuf);

	if (CConfig::GetInstance()->GetValue("PLAYER_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iPlayerCacheFlushSeconds, szBuf);
		TraceLog("PLAYER_CACHE_FLUSH_SECONDS: {}", g_iPlayerCacheFlushSeconds);
	}

	if (CConfig::GetInstance()->GetValue("ITEM_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iItemCacheFlushSeconds, szBuf);
		TraceLog("ITEM_CACHE_FLUSH_SECONDS: {}", g_iItemCacheFlushSeconds);
	}

	if (CConfig::GetInstance()->GetValue("ITEM_PRICELIST_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iItemPriceListTableCacheFlushSeconds, szBuf);
		TraceLog("ITEM_PRICELIST_CACHE_FLUSH_SECONDS: {}", g_iItemPriceListTableCacheFlushSeconds);
	}

	if (CConfig::GetInstance()->GetValue("CACHE_FLUSH_LIMIT_PER_SECOND", szBuf, 256))
	{
		uint32_t dwVal = 0; str_to_number(dwVal, szBuf);
		CClientManager::GetInstance()->SetCacheFlushCountLimit(dwVal);
	}

	int32_t iIDStart;
	if (!CConfig::GetInstance()->GetValue("PLAYER_ID_START", &iIDStart))
	{
		FatalLog("PLAYER_ID_START not configured");
		return false;
	}

	CClientManager::GetInstance()->SetPlayerIDStart(iIDStart);

	if (CConfig::GetInstance()->GetValue("NAME_COLUMN", szBuf, 256))
	{
		TraceLog("{} {}", g_stLocaleNameColumn, szBuf);
		g_stLocaleNameColumn = szBuf;
	}

	char szAddr[64], szDB[64], szUser[64], szPassword[64];
	int32_t iPort;
	char line[256 + 1];

	if (CConfig::GetInstance()->GetValue("SQL_PLAYER", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		PyLog("connecting to MySQL server (player)");

		int32_t iRetry = 5;

		do
		{
			if (CDBManager::GetInstance()->Connect(SQL_PLAYER, szAddr, iPort, szDB, szUser, szPassword))
			{
				break;
			}

			SysLog("SQL connection failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		PyLog("Success PLAYER");
		SetPlayerDBName(szDB);
	}
	else
	{
		FatalLog("SQL_PLAYER not configured");
		return false;
	}

	if (CConfig::GetInstance()->GetValue("SQL_ACCOUNT", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		PyLog("connecting to MySQL server (account)");

		int32_t iRetry = 5;

		do
		{
			if (CDBManager::GetInstance()->Connect(SQL_ACCOUNT, szAddr, iPort, szDB, szUser, szPassword))
			{
				break;
			}

			SysLog("SQL connect failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		PyLog("Success ACCOUNT");
	}
	else
	{
		FatalLog("SQL_ACCOUNT not configured");
		return false;
	}

	if (CConfig::GetInstance()->GetValue("SQL_COMMON", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		PyLog("connecting to MySQL server (common)");

		int32_t iRetry = 5;

		do
		{
			if (CDBManager::GetInstance()->Connect(SQL_COMMON, szAddr, iPort, szDB, szUser, szPassword))
			{
				break;
			}

			SysLog("SQL connect failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		PyLog("Success COMMON");
	}
	else
	{
		FatalLog("SQL_COMMON not configured");
		return false;
	}

	if (CConfig::GetInstance()->GetValue("SQL_HOTBACKUP", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		PyLog("connecting to MySQL server (hotbackup)");

		int32_t iRetry = 5;

		do
		{
			if (CDBManager::GetInstance()->Connect(SQL_HOTBACKUP, szAddr, iPort, szDB, szUser, szPassword))
			{
				break;
			}

			SysLog("SQL connection failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);

		PyLog("Success HOTBACKUP");
	}
	else
	{
		FatalLog("SQL_HOTBACKUP not configured");
		return false;
	}

	if (!CNetPoller::GetInstance()->Create())
	{
		FatalLog("Cannot create network poller");
		return false;
	}

	TraceLog("ClientManager initialization.. ");

	if (!CClientManager::GetInstance()->Initialize())
	{
		return false;
	}

	if (!CPlayerHB::GetInstance()->Initialize())
	{
		FatalLog("cannot initialize player hotbackup");
		return false;
	}

	return true;
}

SIMPLE_APPLICATION(DatabaseServer)