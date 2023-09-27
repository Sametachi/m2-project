#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "guild_manager.h"
#include "db.h"

#include "party.h"

extern LPFDWATCH	main_fdw;

LPCLIENT_DESC db_clientdesc = nullptr;
LPCLIENT_DESC g_pAuthMasterDesc = nullptr;
LPCLIENT_DESC g_NetmarbleDBDesc = nullptr;

static const char* GetKnownClientDescName(LPCLIENT_DESC desc) {
	if (desc == db_clientdesc) {
		return "db_clientdesc";
	} else if (desc == g_pAuthMasterDesc) {
		return "g_pAuthMasterDesc";
	} else if (desc == g_NetmarbleDBDesc) {
		return "g_NetmarbleDBDesc";
	}
	return "unknown";
}

CLIENT_DESC::CLIENT_DESC()
{
	m_iPhaseWhenSucceed = 0;
	m_bRetryWhenClosed = false;
	m_LastTryToConnectTime = 0;
	m_tLastChannelStatusUpdateTime = 0;
}

CLIENT_DESC::~CLIENT_DESC()
{
}

void CLIENT_DESC::Destroy()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	P2P_MANAGER::GetInstance()->UnregisterConnector(this);

	if (this == db_clientdesc)
	{
		CPartyManager::GetInstance()->DeleteAllParty();
		CPartyManager::GetInstance()->DisablePCParty();
		CGuildManager::GetInstance()->StopAllGuildWar();
	}

	fdwatch_del_fd(m_lpFdw, m_sock);

	PyLog("SYSTEM: closing client socket. DESC #{}", m_sock);

	socket_close(m_sock);
	m_sock = INVALID_SOCKET;

	DESC::Destroy();
}

void CLIENT_DESC::SetRetryWhenClosed(bool b)
{
	m_bRetryWhenClosed = b;
}

bool CLIENT_DESC::Connect(int32_t iPhaseWhenSucceed)
{
	if (iPhaseWhenSucceed != 0)
		m_iPhaseWhenSucceed = iPhaseWhenSucceed;

	if (get_global_time() - m_LastTryToConnectTime < 3)
		return false;

	m_LastTryToConnectTime = get_global_time();

	if (m_sock != INVALID_SOCKET)
		return false;

	PyLog("SYSTEM: Trying to connect to {}:{}", m_stHost.c_str(), m_wPort);

	m_sock = socket_connect(m_stHost.c_str(), m_wPort);

	if (m_sock != INVALID_SOCKET)
	{
		PyLog("SYSTEM: connected to server (fd {})", m_sock);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, false);
		SetPhase(m_iPhaseWhenSucceed);
		return true;
	}
	else
	{
		SetPhase(PHASE_CLIENT_CONNECTING);
		return false;
	}
}

void CLIENT_DESC::Setup(LPFDWATCH _fdw, const char* _host, uint16_t _port)
{
	m_lpFdw = _fdw;
	m_stHost = _host;
	m_wPort = _port;

	InitializeBuffers();

	m_sock = INVALID_SOCKET;
}

void CLIENT_DESC::SetPhase(int32_t iPhase)
{
	switch (iPhase)
	{
		case PHASE_CLIENT_CONNECTING:
			TraceLog("PHASE_CLIENT_DESC::CONNECTING");
			m_pInputProcessor = nullptr;
			break;

		case PHASE_DBCLIENT:
			{
				TraceLog("PHASE_DBCLIENT");

				if (!g_bAuthServer)
				{
					static bool bSentBoot = false;

					if (!bSentBoot)
					{
						bSentBoot = true;
						TPacketGDBoot p;
						p.dwItemIDRange[0] = 0;
						p.dwItemIDRange[1] = 0;
						memcpy(p.szIP, g_szPublicIP, 16);
						DBPacket(HEADER_GD_BOOT, 0, &p, sizeof(p));
					}
				}

				TEMP_BUFFER buf;

				TPacketGDSetup p;

				memset(&p, 0, sizeof(p));
				strlcpy(p.szPublicIP, g_szPublicIP, sizeof(p.szPublicIP));

				if (!g_bAuthServer)
				{
					p.bChannel	= g_bChannel;
					p.wListenPort = mother_port;
					p.wP2PPort	= p2p_port;
					p.bAuthServer = false;
					map_allow_copy(p.alMaps, 32);

					const DESC_MANAGER::DESC_SET& c_set = DESC_MANAGER::GetInstance()->GetClientSet();
					DESC_MANAGER::DESC_SET::const_iterator it;

					for (it = c_set.begin(); it != c_set.end(); ++it)
					{
						LPDESC d = *it;

						if (d->GetAccountTable().id != 0)
							++p.dwLoginCount;
					}

					buf.write(&p, sizeof(p));

					if (p.dwLoginCount)
					{
						TPacketLoginOnSetup pck;

						for (it = c_set.begin(); it != c_set.end(); ++it)
						{
							LPDESC d = *it;

							TAccountTable& r = d->GetAccountTable();

							if (r.id != 0)
							{
								pck.dwID = r.id;
								strlcpy(pck.szLogin, r.login, sizeof(pck.szLogin));
								strlcpy(pck.szSocialID, r.social_id, sizeof(pck.szSocialID));
								strlcpy(pck.szHost, d->GetHostName(), sizeof(pck.szHost));
								pck.dwLoginKey = d->GetLoginKey();
								memcpy(pck.adwClientKey, d->GetDecryptionKey(), 16);

								buf.write(&pck, sizeof(TPacketLoginOnSetup));
							}
						}
					}

					PyLog("DB_SETUP current user {} size {}", p.dwLoginCount, buf.size());

					CPartyManager::GetInstance()->EnablePCParty();
				}
				else
				{
					p.bAuthServer = true;
					buf.write(&p, sizeof(p));
				}

				DBPacket(HEADER_GD_SETUP, 0, buf.read_peek(), buf.size());
				m_pInputProcessor = &m_inputDB;
			}
			break;

		case PHASE_P2P:
			TraceLog("PHASE_P2P");
			
			if (m_lpInputBuffer)
				buffer_reset(m_lpInputBuffer);

			if (m_lpOutputBuffer)
				buffer_reset(m_lpOutputBuffer);

			m_pInputProcessor = &m_inputP2P;
			break;

		case PHASE_CLOSE:
			m_pInputProcessor = nullptr;
			break;
	}

	m_iPhase = iPhase;
}

void CLIENT_DESC::DBPacketHeader(uint8_t bHeader, uint32_t dwHandle, uint32_t dwSize)
{
	buffer_write(m_lpOutputBuffer, encode_byte(bHeader), sizeof(uint8_t));
	buffer_write(m_lpOutputBuffer, encode_4bytes(dwHandle), sizeof(uint32_t));
	buffer_write(m_lpOutputBuffer, encode_4bytes(dwSize), sizeof(uint32_t));
}

void CLIENT_DESC::DBPacket(uint8_t bHeader, uint32_t dwHandle, const void* c_pvData, uint32_t dwSize)
{
	if (m_sock == INVALID_SOCKET) {
		PyLog("CLIENT_DESC [{}] trying DBPacket() while not connected",
			GetKnownClientDescName(this));
		return;
	}
	TraceLog("DB_PACKET: header {} handle {} size {} buffer_size {}", bHeader, dwHandle, dwSize, buffer_size(m_lpOutputBuffer));
	DBPacketHeader(bHeader, dwHandle, dwSize);

	if (c_pvData)
		buffer_write(m_lpOutputBuffer, c_pvData, dwSize);
}

void CLIENT_DESC::Packet(const void* c_pvData, int32_t iSize)
{
	if (m_sock == INVALID_SOCKET) {
		PyLog("CLIENT_DESC [{}] trying Packet() while not connected",
			GetKnownClientDescName(this));
		return;
	}
	buffer_write(m_lpOutputBuffer, c_pvData, iSize);
}

bool CLIENT_DESC::IsRetryWhenClosed()
{
	return (0 == thecore_is_shutdowned() && m_bRetryWhenClosed);
}

void CLIENT_DESC::Update(uint32_t t)
{
	if (!g_bAuthServer) {
		UpdateChannelStatus(t, false);
	}
}

void CLIENT_DESC::UpdateChannelStatus(uint32_t t, bool fForce)
{
	enum {
		CHANNELSTATUS_UPDATE_PERIOD = 5*60*1000,
	};
	if (fForce || m_tLastChannelStatusUpdateTime+CHANNELSTATUS_UPDATE_PERIOD < t) {
		int32_t iTotal; 
		int32_t* paiEmpireUserCount;
		int32_t iLocal;
		DESC_MANAGER::GetInstance()->GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		TChannelStatus channelStatus;
		channelStatus.nPort = mother_port;

		if (g_bNoMoreClient) channelStatus.bStatus = 0;
		else channelStatus.bStatus = iTotal > g_iFullUserCount ? 3 : iTotal > g_iBusyUserCount ? 2 : 1;

		DBPacket(HEADER_GD_UPDATE_CHANNELSTATUS, 0, &channelStatus, sizeof(channelStatus));
		m_tLastChannelStatusUpdateTime = t;
	}
}

void CLIENT_DESC::Reset()
{
	LPFDWATCH fdw = m_lpFdw;
	std::string host = m_stHost;
	uint16_t port = m_wPort;

	Destroy();
	Initialize();

	m_lpFdw = fdw;
	m_stHost = host;
	m_wPort = port;

	InitializeBuffers();
}

void CLIENT_DESC::InitializeBuffers()
{
	m_lpOutputBuffer = buffer_new(1024 * 1024);
	m_lpInputBuffer = buffer_new(1024 * 1024);
	m_iMinInputBufferLen = 1024 * 1024;
}
