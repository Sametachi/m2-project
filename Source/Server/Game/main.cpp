#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include <lzo/lzo1x.h>
#include "packet.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "motion.h"
#include "sectree_manager.h"
#include "shop_manager.h"
#include "regen.h"
#include "text_file_loader.h"
#include "skill.h"
#include "pvp.h"
#include "party.h"
#include "questmanager.h"
#include "lzo_manager.h"
#include "messenger_manager.h"
#include "db.h"
#include "log.h"
#include "p2p.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "cmd.h"
#include "refine.h"
#include "banword.h"
#include "priv_manager.h"
#include "war_map.h"
#include "building.h"
#include "login_sim.h"
#include "target.h"
#include "marriage.h"
#include "wedding.h"
#include "fishing.h"
#include "item_addon.h"
#include "locale_service.h"
#include "arena.h"
#include "OXEvent.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "ani.h"
#include "BattleArena.h"
#include "over9refine.h"
#include "horsename_manager.h"
#include "MarkManager.h"
#include "spam.h"
#include "threeway_war.h"
#include "DragonLair.h"
#include "skill_power.h"
#include "DragonSoul.h"
#include <boost/bind.hpp>
#include <Basic/SimpleApp.hpp>

class GameServer : public SimpleApp
{
public:
	GameServer() = default;
	~GameServer() = default;

	int32_t		Run(int32_t argc, const char** argv);

	bool		Setup();
	void		Close();

	bool		MainLoop();
	bool		IOLoop(LPFDWATCH fdw);

	FileSystem					m_FileSystem;
	ArchiveFSProvider			m_FSPack;
	VirtualDiskProvider			m_FSDisk;

	SECTREE_MANAGER				m_SectreeManager;
	CHARACTER_MANAGER			m_CharManager;
	ITEM_MANAGER				m_ItemManager;
	CShopManager				m_ShopManager;
	CMobManager					m_MobManager;
	CMotionManager				m_MotionManager;
	CPartyManager				m_PartyManager;
	CSkillManager				m_SkillManager;
	CPVPManager					m_PVPManager;
	LZOManager					m_LZOManager;
	DBManager					m_DBManager;
	AccountDB					m_AccountDB;

	LogManager					m_LogManager;
	MessengerManager			m_MessengerManager;
	P2P_MANAGER					m_P2PManager;
	CGuildManager				m_GuildManager;
	CGuildMarkManager			m_MarkManager;
	CDungeonManager				m_DungeonManager;
	CRefineManager				m_RefineManager;
	CBanwordManager				m_BanwordManager;
	CPrivManager				m_PrivManager;
	CWarMapManager				m_WarMapManager;
	building::CManager			m_BuildingManager;
	CTargetManager				m_TargetManager;
	marriage::CManager			m_MarriageManager;
	marriage::WeddingManager	m_WeddingManager;
	CItemAddonManager			m_ItemAddonManager;
	CArenaManager				m_ArenaManager;
	COXEventManager				m_OXEventManager;
	CMonarch					m_Monarch;
	CHorseNameManager			m_HorsenameManager;
	quest::CQuestManager		m_QuestManager;

	DESC_MANAGER				m_DescManager;
	CTableBySkill				m_SkillPowerByLevel;
	CPolymorphUtils				m_PolymorphUtils;
	COver9RefineManager			m_Over9RefineManager;
	SpamManager					m_SpamManager;
	CDragonLairManager			m_DragonLairManager;
	CThreeWayWar				m_ThreeWarWar;
	DSManager					m_DSManager;
};

extern void WriteVersion();

static const uint32_t	TRAFFIC_PROFILE_FLUSH_CYCLE = 3600;
volatile int32_t	num_events_called = 0;
int32_t             max_bytes_written = 0;
int32_t             current_bytes_written = 0;
int32_t             total_bytes_written = 0;
uint8_t		g_bLogLevel = 0;

socket_t	tcp_socket = 0;
socket_t	udp_socket = 0;
socket_t	p2p_socket = 0;

LPFDWATCH	main_fdw = nullptr;

enum EProfile
{
	PROF_EVENT,
	PROF_CHR_UPDATE,
	PROF_IO,
	PROF_HEARTBEAT,
	PROF_MAX_NUM
};

static uint32_t s_dwProfiler[PROF_MAX_NUM];

int32_t g_shutdown_disconnect_pulse;
int32_t g_shutdown_disconnect_force_pulse;
int32_t g_shutdown_core_pulse;
bool g_bShutdown=false;

extern bool speed_server;

extern void CancelReloadSpamEvent();

static void CleanUpForEarlyExit()
{
	CancelReloadSpamEvent();
}

namespace
{
	struct SendDisconnectFunc
	{
		void operator () (LPDESC d)
		{
			if (d->GetCharacter())
			{
				if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
					d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
			}
		}
	};

	struct DisconnectFunc
	{
		void operator () (LPDESC d)
		{
			if (d->GetType() == DESC_TYPE_CONNECTOR)
				return;

			if (d->IsPhase(PHASE_P2P))
				return;

			d->SetPhase(PHASE_CLOSE);
		}
	};
}

extern std::map<uint32_t, CLoginSim *> g_sim; 
extern std::map<uint32_t, CLoginSim *> g_simByPID;
extern std::vector<TPlayerTable> g_vec_save;
uint32_t save_idx = 0;

void heartbeat(LPHEART ht, int32_t pulse) 
{
	uint32_t t;

	t = get_dword_time();
	num_events_called += event_process(pulse);
	s_dwProfiler[PROF_EVENT] += (get_dword_time() - t);

	t = get_dword_time();

	if (!(pulse % ht->passes_per_sec))
	{
		if (!g_bAuthServer)
		{
			TPlayerCountPacket pack;
			pack.dwCount = DESC_MANAGER::GetInstance()->GetLocalUserCount();
			db_clientdesc->DBPacket(HEADER_GD_PLAYER_COUNT, 0, &pack, sizeof(TPlayerCountPacket));
		}
		else
		{
			DESC_MANAGER::GetInstance()->ProcessExpiredLoginKey();
		}

		{
			int32_t count = 0;
			auto it = g_sim.begin();

			while (it != g_sim.end())
			{
				if (!it->second->IsCheck())
				{
					it->second->SendLogin();

					if (++count > 50)
					{
						PyLog("FLUSH_SENT");
						break;
					}
				}

				it++;
			}

			if (save_idx < g_vec_save.size())
			{
				count = MIN(100, g_vec_save.size() - save_idx);

				for (int32_t i = 0; i < count; ++i, ++save_idx)
					db_clientdesc->DBPacket(HEADER_GD_PLAYER_SAVE, 0, &g_vec_save[save_idx], sizeof(TPlayerTable));

				PyLog("SAVE_FLUSH {}", count);
			}
		}
	}

	if (!(pulse % (passes_per_sec + 4)))
		CHARACTER_MANAGER::GetInstance()->ProcessDelayedSave();

	if (!(pulse % (passes_per_sec * 5 + 2)))
	{
		ITEM_MANAGER::GetInstance()->Update();
		DESC_MANAGER::GetInstance()->UpdateLocalUserCount();
	}

	s_dwProfiler[PROF_HEARTBEAT] += (get_dword_time() - t);

	DBManager::GetInstance()->Process();
	AccountDB::GetInstance()->Process();
	CPVPManager::GetInstance()->Process();

	if (g_bShutdown)
	{
		if (thecore_pulse() > g_shutdown_disconnect_pulse)
		{
			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::GetInstance()->GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), ::SendDisconnectFunc());
			g_shutdown_disconnect_pulse = INT_MAX;
		}
		else if (thecore_pulse() > g_shutdown_disconnect_force_pulse)
		{
			const DESC_MANAGER::DESC_SET& c_set_desc = DESC_MANAGER::GetInstance()->GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), ::DisconnectFunc());
		}
		else if (thecore_pulse() > g_shutdown_disconnect_force_pulse + PASSES_PER_SEC(5))
		{
			thecore_shutdown();
		}
	}
}

int32_t GameServer::Run(int32_t argc,const char** argv)
{
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
	,true, true
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

	ilInit();

	WriteVersion();

	if (!Setup()) 
	{
		CleanUpForEarlyExit();
		return 0;
	}

	if (!m_QuestManager.Initialize()) {
		CleanUpForEarlyExit();
		return 0;
	}

	MessengerManager::GetInstance()->Initialize();
	CGuildManager::GetInstance()->Initialize();
	fishing::Initialize();
	m_OXEventManager.Initialize();

	Cube_init();
	Blend_Item_init();
	ani_init();

	while (MainLoop());

	PyLog("<shutdown> Starting...");
	g_bShutdown = true;
	g_bNoMoreClient = true;

	if (g_bAuthServer)
	{
		int32_t iLimit = DBManager::GetInstance()->CountQuery() / 50;
		int32_t i = 0;

		do
		{
			uint32_t dwCount = DBManager::GetInstance()->CountQuery();
			PyLog("Queries {}", dwCount);

			if (dwCount == 0)
				break;

			usleep(500000);

			if (++i >= iLimit)
				if (dwCount == DBManager::GetInstance()->CountQuery())
					break;
		} while (1);
	}

	PyLog("<shutdown> Destroying CArenaManager...");
	m_ArenaManager.Destroy();
	PyLog("<shutdown> Destroying COXEventManager...");
	m_OXEventManager.Destroy();

	PyLog("<shutdown> Disabling signal timer...");
	signal_timer_disable();

	PyLog("<shutdown> Shutting down CHARACTER_MANAGER...");
	m_CharManager.GracefulShutdown();
	PyLog("<shutdown> Shutting down ITEM_MANAGER...");
	m_ItemManager.GracefulShutdown();

	PyLog("<shutdown> Flushing db_clientdesc...");
	db_clientdesc->FlushOutput();
	PyLog("<shutdown> Flushing p2p_manager...");
	m_P2PManager.FlushOutput();

	PyLog("<shutdown> Destroying CShopManager...");
	m_ShopManager.Destroy();
	PyLog("<shutdown> Destroying CHARACTER_MANAGER...");
	m_CharManager.Destroy();
	PyLog("<shutdown> Destroying ITEM_MANAGER...");
	m_ItemManager.Destroy();
	PyLog("<shutdown> Destroying DESC_MANAGER...");
	m_DescManager.Destroy();
	PyLog("<shutdown> Destroying quest::CQuestManager...");
	m_QuestManager.Destroy();
	PyLog("<shutdown> Destroying building::CManager...");
	m_BuildingManager.Destroy();

	Close();

	return 1;
}

bool GameServer::Setup()
{
	ConfigInit();

	bool is_thecore_initialized = thecore_init(25, heartbeat);

	if (!is_thecore_initialized)
	{
		SysLog("Could not initialize thecore, check owner of pid, syslog\n");
		exit(0);
	}

	if (!CThreeWayWar::GetInstance()->LoadSetting("forkedmapindex.txt"))
	{
		if (!g_bAuthServer)
		{
			SysLog("Could not Load ThreeWayWar Setting file");
			exit(0);
		}
	}

	signal_timer_disable();
	
	main_fdw = fdwatch_new(4096);

	// Try and connect to the public IP
	tcp_socket = socket_tcp_bind(g_szPublicIP, mother_port);
	if (tcp_socket != INVALID_SOCKET) {
		// We successfully bound to this IP address.
		strlcpy(g_szBindIP, g_szPublicIP, sizeof(g_szBindIP));
	}
	else if (g_bPublicIPOverride) {
		// Couldn't bind to the public IP address, yet a manual override was requested
		SysLog(
			"WARNING: Couldn't bind to overridden public IP: %s! "\
			"Make sure traffic reaches the server!\n", g_szPublicIP);

		if (g_szInternalIP[0] != '0') {
			// Can we bind to the internal IP?
			SysLog("Will instead bind to internal IP: {}\n", g_szInternalIP);

			tcp_socket = socket_tcp_bind(g_szInternalIP, mother_port);
			if (tcp_socket != INVALID_SOCKET) {
				strlcpy(g_szBindIP, g_szInternalIP, sizeof(g_szBindIP));
			}
			else {
				SysLog("FATAL ERROR: Couldn't bind to internal IP: {}!\n", g_szInternalIP);
				return false;
			}
	}
		else {
			// No other options
			SysLog("FATAL ERROR: No internal IP was found.\n");
			return false;
		}
	}

	
#ifndef __UDP_BLOCK__
	if ((udp_socket = socket_udp_bind(g_szPublicIP, mother_port)) == INVALID_SOCKET)
	{
		perror("socket_udp_bind: udp_socket");
		return false;
	}
#endif	

	// if internal ip exists, p2p socket uses internal ip, if not use public ip
	//if ((p2p_socket = socket_tcp_bind(*g_szInternalIP ? g_szInternalIP : g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	if ((p2p_socket = socket_tcp_bind(g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	{
		perror("socket_tcp_bind: p2p_socket");
		return false;
	}

	fdwatch_add_fd(main_fdw, tcp_socket, NULL, FDW_READ, false);
#ifndef __UDP_BLOCK__
	fdwatch_add_fd(main_fdw, udp_socket, NULL, FDW_READ, false);
#endif
	fdwatch_add_fd(main_fdw, p2p_socket, NULL, FDW_READ, false);

	db_clientdesc = DESC_MANAGER::GetInstance()->CreateConnectionDesc(main_fdw, db_addr, db_port, PHASE_DBCLIENT, true);
	if (!g_bAuthServer) {
		db_clientdesc->UpdateChannelStatus(0, true);
	}

	if (g_bAuthServer)
	{
		if (g_stAuthMasterIP.length() != 0)
		{
			SysLog("SlaveAuth");
			g_pAuthMasterDesc = DESC_MANAGER::GetInstance()->CreateConnectionDesc(main_fdw, g_stAuthMasterIP.c_str(), g_wAuthMasterPort, PHASE_P2P, true); 
			P2P_MANAGER::GetInstance()->RegisterConnector(g_pAuthMasterDesc);
			g_pAuthMasterDesc->SetP2P(g_stAuthMasterIP.c_str(), g_wAuthMasterPort, g_bChannel);

		}
		else
		{
			SysLog("MasterAuth");
		}
	}
	else
	{
		extern uint32_t g_uiSpamBlockDuration;
		extern uint32_t g_uiSpamBlockScore;
		extern uint32_t g_uiSpamReloadCycle;

		PyLog("SPAM_CONFIG: duration {} score {} reload cycle {}\n",
				g_uiSpamBlockDuration, g_uiSpamBlockScore, g_uiSpamReloadCycle);

		extern void LoadSpamDB();
		LoadSpamDB();
	}

	signal_timer_enable(30);
	return true;
}

void GameServer::Close()
{
	PyLog("<shutdown> Canceling ReloadSpamEvent...");
	CancelReloadSpamEvent();

	PyLog("<shutdown> regen_free()...");
	regen_free();

	PyLog("<shutdown> Closing sockets...");
	socket_close(tcp_socket);
#ifndef __UDP_BLOCK__
	socket_close(udp_socket);
#endif
	socket_close(p2p_socket);

	PyLog("<shutdown> fdwatch_delete()...");
	fdwatch_delete(main_fdw);

	PyLog("<shutdown> event_destroy()...");
	event_destroy();

	PyLog("<shutdown> CTextFileLoader::DestroySystem()...");
	CTextFileLoader::DestroySystem();

	PyLog("<shutdown> thecore_destroy()...");
	thecore_destroy();
}

bool GameServer::MainLoop()
{
	static struct timeval	pta = { 0, 0 };
	static int32_t			process_time_count = 0;
	struct timeval		now;

	if (pta.tv_sec == 0)
		gettimeofday(&pta, (struct timezone*)0);

	int32_t passed_pulses;

	if (!(passed_pulses = thecore_idle()))
		return false;

	assert(passed_pulses > 0);

	uint32_t t;

	while (passed_pulses--) {
		heartbeat(thecore_heart, ++thecore_heart->pulse);

		// To reduce the possibility of abort() in checkpointing
		thecore_tick();
	}

	t = get_dword_time();
	CHARACTER_MANAGER::GetInstance()->Update(thecore_heart->pulse);
	db_clientdesc->Update(t);
	s_dwProfiler[PROF_CHR_UPDATE] += (get_dword_time() - t);

	t = get_dword_time();
	if (!IOLoop(main_fdw)) return 0;
	s_dwProfiler[PROF_IO] += (get_dword_time() - t);

	gettimeofday(&now, (struct timezone*)0);
	++process_time_count;

	if (now.tv_sec - pta.tv_sec > 0)
	{
		num_events_called = 0;
		current_bytes_written = 0;

		process_time_count = 0;
		gettimeofday(&pta, (struct timezone*)0);

		memset(&thecore_profiler[0], 0, sizeof(thecore_profiler));
		memset(&s_dwProfiler[0], 0, sizeof(s_dwProfiler));
	}

	return true;
}

bool GameServer::IOLoop(LPFDWATCH fdw)
{
	LPDESC	d;
	int32_t		num_events, event_idx;

	DESC_MANAGER::GetInstance()->DestroyClosed();
	DESC_MANAGER::GetInstance()->TryConnect();

	if ((num_events = fdwatch(fdw, 0)) < 0)
		return false;

	for (event_idx = 0; event_idx < num_events; ++event_idx)
	{
		d = (LPDESC)fdwatch_get_client_data(fdw, event_idx);

		if (!d)
		{
			if (FDW_READ == fdwatch_check_event(fdw, tcp_socket, event_idx))
			{
				DESC_MANAGER::GetInstance()->AcceptDesc(fdw, tcp_socket);
				fdwatch_clear_event(fdw, tcp_socket, event_idx);
			}
			else if (FDW_READ == fdwatch_check_event(fdw, p2p_socket, event_idx))
			{
				DESC_MANAGER::GetInstance()->AcceptP2PDesc(fdw, p2p_socket);
				fdwatch_clear_event(fdw, p2p_socket, event_idx);
			}
			/*
			else if (FDW_READ == fdwatch_check_event(fdw, udp_socket, event_idx))
			{
				char			buf[256];
				struct sockaddr_in	cliaddr;
				socklen_t		socklen = sizeof(cliaddr);

				int32_t iBytesRead;

				if ((iBytesRead = socket_udp_read(udp_socket, buf, 256, (struct sockaddr *) &cliaddr, &socklen)) > 0)
				{
					static CInputUDP s_inputUDP;

					s_inputUDP.SetSockAddr(cliaddr);

					int32_t iBytesProceed;
					s_inputUDP.Process(NULL, buf, iBytesRead, iBytesProceed);
				}

				fdwatch_clear_event(fdw, udp_socket, event_idx);
			}
			*/
			continue;
		}

		int32_t iRet = fdwatch_check_event(fdw, d->GetSocket(), event_idx);

		switch (iRet)
		{
		case FDW_READ:
			if (db_clientdesc == d)
			{
				int32_t size = d->ProcessInput();

				if (size)
					TraceLog("DB_BYTES_READ: {}", size);

				if (size < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}
			}
			else if (d->ProcessInput() < 0)
			{
				d->SetPhase(PHASE_CLOSE);
			}
			break;

		case FDW_WRITE:
			if (db_clientdesc == d)
			{
				int32_t buf_size = buffer_size(d->GetOutputBuffer());
				int32_t sock_buf_size = fdwatch_get_buffer_size(fdw, d->GetSocket());

				int32_t ret = d->ProcessOutput();

				if (ret < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}

				if (buf_size)
					TraceLog("DB_BYTES_WRITE: size {} sock_buf {} ret {}", buf_size, sock_buf_size, ret);
			}
			else if (d->ProcessOutput() < 0)
			{
				d->SetPhase(PHASE_CLOSE);
			}
			break;

		case FDW_EOF:
		{
			d->SetPhase(PHASE_CLOSE);
		}
		break;

		default:
			SysLog("fdwatch_check_event returned unknown {}", iRet);
			d->SetPhase(PHASE_CLOSE);
			break;
		}
	}

	return true;
}

SIMPLE_APPLICATION(GameServer)