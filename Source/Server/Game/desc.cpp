#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "packet.h"
#include "messenger_manager.h"
#include "sectree_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "guild.h"
#include "guild_manager.h"
#include "locale_service.h"
#include "log.h"

extern int32_t max_bytes_written;
extern int32_t current_bytes_written;
extern int32_t total_bytes_written;

DESC::DESC()
{
	Initialize();
}

DESC::~DESC()
{
}

void DESC::Initialize()
{
	m_bDestroyed = false;

	m_pInputProcessor = nullptr;
	m_lpFdw = nullptr;
	m_sock = INVALID_SOCKET;
	m_iPhase = PHASE_CLOSE;
	m_dwHandle = 0;

	m_wPort = 0;
	m_LastTryToConnectTime = 0;

	m_lpInputBuffer = nullptr;
	m_iMinInputBufferLen = 0;

	m_dwHandshake = 0;
	m_dwHandshakeSentTime = 0;
	m_iHandshakeRetry = 0;
	m_dwClientTime = 0;
	m_bHandshaking = false;

	m_lpBufferedOutputBuffer = nullptr;
	m_lpOutputBuffer = nullptr;

	m_pPingEvent = nullptr;
	m_lpCharacter = nullptr;
	memset(&m_accountTable, 0, sizeof(m_accountTable));

	memset(&m_SockAddr, 0, sizeof(m_SockAddr));
	memset(&m_UDPSockAddr, 0, sizeof(m_UDPSockAddr));

	m_pLogFile = nullptr;
	m_bEncrypted = false;

	m_wP2PPort = 0;
	m_bP2PChannel = 0;

	m_bAdminMode = false;
	m_bPong = true;
	m_bChannelStatusRequested = false;


	m_pLoginKey = nullptr;
	m_dwLoginKey = 0;

	memset(m_adwDecryptionKey, 0, sizeof(m_adwDecryptionKey));
	memset(m_adwEncryptionKey, 0, sizeof(m_adwEncryptionKey));

	m_bCRCMagicCubeIdx = 0;
	m_dwProcCRC = 0;
	m_dwFileCRC = 0;
	m_bHackCRCQuery = 0;

	m_outtime = 0;
	m_playtime = 0;
	m_offtime = 0;

	m_pDisconnectEvent = nullptr;

	m_seq_vector.clear();
}

void DESC::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	if (m_pLoginKey)
		m_pLoginKey->Expire();

	if (GetAccountTable().id)
		DESC_MANAGER::GetInstance()->DisconnectAccount(GetAccountTable().login);

	if (m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = nullptr;
	}

	if (m_lpCharacter)
	{
		m_lpCharacter->Disconnect("DESC::~DESC");
		m_lpCharacter = nullptr;
	}

	SAFE_BUFFER_DELETE(m_lpOutputBuffer);
	SAFE_BUFFER_DELETE(m_lpInputBuffer);

	event_cancel(&m_pPingEvent);
	event_cancel(&m_pDisconnectEvent);

	if (!g_bAuthServer)
	{
		if (m_accountTable.login[0] && m_accountTable.passwd[0])
		{
			TLogoutPacket pack;

			strlcpy(pack.login, m_accountTable.login, sizeof(pack.login));
			strlcpy(pack.passwd, m_accountTable.passwd, sizeof(pack.passwd));

			db_clientdesc->DBPacket(HEADER_GD_LOGOUT, m_dwHandle, &pack, sizeof(TLogoutPacket));
		}
	}

	if (m_sock != INVALID_SOCKET)
	{
		PyLog("SYSTEM: closing socket. DESC #{}", m_sock);
		Log("SYSTEM: closing socket. DESC #%d", m_sock);
		fdwatch_del_fd(m_lpFdw, m_sock);

		socket_close(m_sock);
		m_sock = INVALID_SOCKET;
	}

	m_seq_vector.clear();
}

EVENTFUNC(ping_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("ping_event> <Factor> Null pointer");
		return 0;
	}

	LPDESC desc = info->desc;

	if (desc->IsAdminMode())
		return (ping_event_second_cycle);

	if (!desc->IsPong())
	{
		PyLog("PING_EVENT: no pong {}", desc->GetHostName());

		desc->SetPhase(PHASE_CLOSE);

		return (ping_event_second_cycle);
	}
	else
	{
		TPacketGCPing p;
		p.header = HEADER_GC_PING;
		desc->Packet(&p, sizeof(struct packet_ping));
		desc->SetPong(false);
	}

	desc->SendHandshake(get_dword_time(), 0);

	return (ping_event_second_cycle);
}

bool DESC::IsPong()
{
	return m_bPong;
}

void DESC::SetPong(bool b)
{
	m_bPong = b;
}

bool DESC::Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in& c_rSockAddr, uint32_t _handle, uint32_t _handshake)
{
	m_lpFdw		= _fdw;
	m_sock		= _fd;

	m_stHost		= inet_ntoa(c_rSockAddr.sin_addr);
	m_wPort			= c_rSockAddr.sin_port;
	m_dwHandle		= _handle;
	m_lpOutputBuffer = buffer_new(DEFAULT_PACKET_BUFFER_SIZE * 2);

	m_iMinInputBufferLen = MAX_INPUT_LEN >> 1;
	m_lpInputBuffer = buffer_new(MAX_INPUT_LEN);

	m_SockAddr = c_rSockAddr;

	fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);

	// Ping Event 
	desc_event_info* info = AllocEventInfo<desc_event_info>();

	info->desc = this;
	assert(m_pPingEvent == nullptr);

	m_pPingEvent = event_create(ping_event, info, ping_event_second_cycle);

	memcpy(m_adwEncryptionKey, "testtesttesttest", sizeof(uint32_t) * 4);
	memcpy(m_adwDecryptionKey, "testtesttesttest", sizeof(uint32_t) * 4);

	// Set Phase to handshake
	SetPhase(PHASE_HANDSHAKE);
	StartHandshake(_handshake);

	PyLog("SYSTEM: new connection from [{}] fd: {} handshake {} output input_len {}", m_stHost.c_str(), m_sock, m_dwHandshake, buffer_size(m_lpInputBuffer));
	Log("SYSTEM: new connection from [%s] fd: %d handshake %u ptr %p", m_stHost.c_str(), m_sock, m_dwHandshake, this);
	return true;
}

int32_t DESC::ProcessInput()
{
	ssize_t bytes_read;

	if (!m_lpInputBuffer)
	{
		SysLog("DESC::ProcessInput : nil input buffer");
		return -1;
	}

	buffer_adjust_size(m_lpInputBuffer, m_iMinInputBufferLen);
	bytes_read = socket_read(m_sock, (char* ) buffer_write_peek(m_lpInputBuffer), buffer_has_space(m_lpInputBuffer));

	if (bytes_read < 0)
		return -1;
	else if (bytes_read == 0)
		return 0;

	buffer_write_proceed(m_lpInputBuffer, bytes_read);

	if (!m_pInputProcessor)
	{
		SysLog("no input processor");
	}
	else if (!m_bEncrypted)
	{
		int32_t iBytesProceed = 0;

		while (!m_pInputProcessor->Process(this, buffer_read_peek(m_lpInputBuffer), buffer_size(m_lpInputBuffer), iBytesProceed))
		{
			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
			iBytesProceed = 0;
		}

		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	}
	else
	{
		int32_t iSizeBuffer = buffer_size(m_lpInputBuffer);

		if (iSizeBuffer & 7)
			iSizeBuffer -= iSizeBuffer & 7;

		if (iSizeBuffer > 0)
		{
			TEMP_BUFFER	tempbuf;
			LPBUFFER lpBufferDecrypt = tempbuf.getptr();
			buffer_adjust_size(lpBufferDecrypt, iSizeBuffer);

			int32_t iSizeAfter = TEA_Decrypt((uint32_t*) buffer_write_peek(lpBufferDecrypt),
					(uint32_t*) buffer_read_peek(m_lpInputBuffer),
					GetDecryptionKey(),
					iSizeBuffer);

			buffer_write_proceed(lpBufferDecrypt, iSizeAfter);

			int32_t iBytesProceed = 0;

			while (!m_pInputProcessor->Process(this, buffer_read_peek(lpBufferDecrypt), buffer_size(lpBufferDecrypt), iBytesProceed))
			{
				if (iBytesProceed > iSizeBuffer)
				{
					buffer_read_proceed(m_lpInputBuffer, iSizeBuffer);
					iSizeBuffer = 0;
					iBytesProceed = 0;
					break;
				}

				buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
				iSizeBuffer -= iBytesProceed;

				buffer_read_proceed(lpBufferDecrypt, iBytesProceed);
				iBytesProceed = 0;
			}

			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
		}
	}

	return (bytes_read);
}

int32_t DESC::ProcessOutput()
{
	if (buffer_size(m_lpOutputBuffer) <= 0)
		return 0;

	int32_t buffer_left = fdwatch_get_buffer_size(m_lpFdw, m_sock);

	if (buffer_left <= 0)
		return 0;

	int32_t bytes_to_write = MIN(buffer_left, buffer_size(m_lpOutputBuffer));

	if (bytes_to_write == 0)
		return 0;

	int32_t result = socket_write(m_sock, (const char* ) buffer_read_peek(m_lpOutputBuffer), bytes_to_write);

	if (result == 0)
	{
		//PyLog("{} bytes written to {} first {}", bytes_to_write, GetHostName(), *(uint8_t*) buffer_read_peek(m_lpOutputBuffer));
		//Log("%d bytes written", bytes_to_write);
		max_bytes_written = MAX(bytes_to_write, max_bytes_written);

		total_bytes_written += bytes_to_write;
		current_bytes_written += bytes_to_write;

		buffer_read_proceed(m_lpOutputBuffer, bytes_to_write);

		if (buffer_size(m_lpOutputBuffer) != 0)
			fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
	}

	return (result);
}

void DESC::BufferedPacket(const void* c_pvData, int32_t iSize)
{
	if (m_iPhase == PHASE_CLOSE)
		return;

	if (!m_lpBufferedOutputBuffer)
		m_lpBufferedOutputBuffer = buffer_new(MAX(1024, iSize));

	buffer_write(m_lpBufferedOutputBuffer, c_pvData, iSize);
}

void DESC::Packet(const void* c_pvData, int32_t iSize)
{
	assert(iSize > 0);

	if (m_iPhase == PHASE_CLOSE) // Do not send if disconnected.
		return;

	if (m_stRelayName.length() != 0)
	{
		// Relay Packets are not encrypted.
		TPacketGGRelay p;

		p.bHeader = HEADER_GG_RELAY;
		strlcpy(p.szName, m_stRelayName.c_str(), sizeof(p.szName));
		p.lSize = iSize;

		if (!packet_encode(m_lpOutputBuffer, &p, sizeof(p)))
		{
			m_iPhase = PHASE_CLOSE;
			return;
		}

		m_stRelayName.clear();

		if (!packet_encode(m_lpOutputBuffer, c_pvData, iSize))
		{
			m_iPhase = PHASE_CLOSE;
			return;
		}
	}
	else
	{
		if (m_lpBufferedOutputBuffer)
		{
			buffer_write(m_lpBufferedOutputBuffer, c_pvData, iSize);

			c_pvData = buffer_read_peek(m_lpBufferedOutputBuffer);
			iSize = buffer_size(m_lpBufferedOutputBuffer);
		}

		if (!m_bEncrypted)
		{
			if (!packet_encode(m_lpOutputBuffer, c_pvData, iSize))
			{
				m_iPhase = PHASE_CLOSE;
			}
		}
		else
		{
			if (buffer_has_space(m_lpOutputBuffer) < iSize + 8)
			{
				SysLog("desc buffer mem_size overflow. memsize({}) write_pos({}) iSize({})", 
						m_lpOutputBuffer->mem_size, m_lpOutputBuffer->write_point_pos, iSize);

				m_iPhase = PHASE_CLOSE;
			}
			else
			{
				// Secure enough buffer size for encryption.
				/* buffer_adjust_size(m_lpOutputBuffer, iSize + 8); */
				uint32_t* pdwWritePoint = (uint32_t*) buffer_write_peek(m_lpOutputBuffer);

				if (packet_encode(m_lpOutputBuffer, c_pvData, iSize))
				{
					int32_t iSize2 = TEA_Encrypt(pdwWritePoint, pdwWritePoint, GetEncryptionKey(), iSize);

					if (iSize2 > iSize)
						buffer_write_proceed(m_lpOutputBuffer, iSize2 - iSize);
				}
			}
		}

		SAFE_BUFFER_DELETE(m_lpBufferedOutputBuffer);
	}

	//PyLog("{} bytes written (first uint8_t {})", iSize, *(uint8_t*) c_pvData);
	if (m_iPhase != PHASE_CLOSE)
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
}

void DESC::LargePacket(const void* c_pvData, int32_t iSize)
{
	buffer_adjust_size(m_lpOutputBuffer, iSize);
	PyLog("LargePacket Size {}", iSize, buffer_size(m_lpOutputBuffer));

	Packet(c_pvData, iSize);
}

void DESC::SetPhase(int32_t _phase)
{
	m_iPhase = _phase;

	TPacketGCPhase pack;
	pack.header = HEADER_GC_PHASE;
	pack.phase = _phase;
	Packet(&pack, sizeof(TPacketGCPhase));

	switch (m_iPhase)
	{
		case PHASE_CLOSE:
			m_pInputProcessor = &m_inputClose;
			break;

		case PHASE_HANDSHAKE:
			m_pInputProcessor = &m_inputHandshake;
			break;

		case PHASE_SELECT:
		case PHASE_LOGIN:
		case PHASE_LOADING:
			m_pInputProcessor = &m_inputLogin;
			break;

		case PHASE_GAME:
		case PHASE_DEAD:
			m_pInputProcessor = &m_inputMain;
			break;

		case PHASE_AUTH:
			m_pInputProcessor = &m_inputAuth;
			PyLog("AUTH_PHASE");
			break;
	}
}

void DESC::BindAccountTable(TAccountTable* pAccountTable)
{
	assert(pAccountTable != nullptr);
	memcpy(&m_accountTable, pAccountTable, sizeof(TAccountTable));
	DESC_MANAGER::GetInstance()->ConnectAccount(m_accountTable.login, this);
}

void DESC::UDPGrant(const struct sockaddr_in& c_rSockAddr)
{
	m_UDPSockAddr = c_rSockAddr;

	TPacketGCBindUDP pack;

	pack.header	= HEADER_GC_BINDUDP;
	pack.addr	= m_UDPSockAddr.sin_addr.s_addr;
	pack.port	= m_UDPSockAddr.sin_port;

	Packet(&pack, sizeof(TPacketGCBindUDP));
}

void DESC::Log(const char* format, ...)
{
	if (!m_pLogFile)
		return;

	va_list args;

	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);

	fprintf(m_pLogFile,
			"%02d %02d %02d:%02d:%02d | ",
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	va_start(args, format);
	vfprintf(m_pLogFile, format, args);
	va_end(args);

	fputs("\n", m_pLogFile);

	fflush(m_pLogFile);
}

void DESC::StartHandshake(uint32_t _handshake)
{
	// Handshake
	m_dwHandshake = _handshake;

	SendHandshake(get_dword_time(), 0);

	m_iHandshakeRetry = 0;
}

void DESC::SendHandshake(uint32_t dwCurTime, int32_t lNewDelta)
{
	TPacketGCHandshake pack;

	pack.bHeader		= HEADER_GC_HANDSHAKE;
	pack.dwHandshake	= m_dwHandshake;
	pack.dwTime			= dwCurTime;
	pack.lDelta			= lNewDelta;

	Packet(&pack, sizeof(TPacketGCHandshake));

	m_dwHandshakeSentTime = dwCurTime;
	m_bHandshaking = true;
}

bool DESC::HandshakeProcess(uint32_t dwTime, int32_t lDelta, bool bInfiniteRetry)
{
	uint32_t dwCurTime = get_dword_time();

	if (lDelta < 0)
	{
		SysLog("Desc::HandshakeProcess : value error (lDelta {}, ip {})", lDelta, m_stHost.c_str());
		return false;
	}

	int32_t bias = (int32_t) (dwCurTime - (dwTime + lDelta));

	if (bias >= 0 && bias <= 50)
	{
		if (bInfiniteRetry)
		{
			uint8_t bHeader = HEADER_GC_TIME_SYNC;
			Packet(&bHeader, sizeof(uint8_t));
		}

		if (GetCharacter())
		{
			PyLog("Handshake: client_time {} server_time {} name: {}", m_dwClientTime, dwCurTime, GetCharacter()->GetName());
		}
		else
			PyLog("Handshake: client_time {} server_time {}", m_dwClientTime, dwCurTime, lDelta);

		m_dwClientTime = dwCurTime;
		m_bHandshaking = false;
		return true; 
	}

	int32_t lNewDelta = (int32_t) (dwCurTime - dwTime) / 2;

	if (lNewDelta < 0)
	{
		PyLog("Handshake: lower than zero {}", lNewDelta);
		lNewDelta = (dwCurTime - m_dwHandshakeSentTime) / 2;
	}

	TraceLog("Handshake: ServerTime {} dwTime {} lDelta {} SentTime {} lNewDelta {}", dwCurTime, dwTime, lDelta, m_dwHandshakeSentTime, lNewDelta);

	if (!bInfiniteRetry)
		if (++m_iHandshakeRetry > HANDSHAKE_RETRY_LIMIT)
		{
			SysLog("handshake retry limit reached! (limit {} character {})", 
					HANDSHAKE_RETRY_LIMIT, GetCharacter() ? GetCharacter()->GetName() : "!NO CHARACTER!");
			SetPhase(PHASE_CLOSE);
			return false;
		}

	SendHandshake(dwCurTime, lNewDelta);
	return false;
}

bool DESC::IsHandshaking()
{
	return m_bHandshaking;
}

uint32_t DESC::GetClientTime()
{
	return m_dwClientTime;
}

void DESC::SetRelay(const char* c_pszName)
{
	m_stRelayName = c_pszName;
}

void DESC::BindCharacter(LPCHARACTER ch)
{
	m_lpCharacter = ch;
}

void DESC::FlushOutput()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	if (buffer_size(m_lpOutputBuffer) <= 0)
		return;

	struct timeval sleep_tv, now_tv, start_tv;
	int32_t event_triggered = false;

	gettimeofday(&start_tv, NULL);

	socket_block(m_sock);
	PyLog("FLUSH START {}", buffer_size(m_lpOutputBuffer));

	while (buffer_size(m_lpOutputBuffer) > 0)
	{
		gettimeofday(&now_tv, NULL);

		int32_t iSecondsPassed = now_tv.tv_sec - start_tv.tv_sec;

		if (iSecondsPassed > 10)
		{
			if (!event_triggered || iSecondsPassed > 20)
			{
				SetPhase(PHASE_CLOSE);
				break;
			}
		}

		sleep_tv.tv_sec = 0;
		sleep_tv.tv_usec = 10000;

		int32_t num_events = fdwatch(m_lpFdw, &sleep_tv);

		if (num_events < 0)
		{
			SysLog("num_events < 0 : {}", num_events);
			break;
		}

		int32_t event_idx;

		for (event_idx = 0; event_idx < num_events; ++event_idx)
		{
			LPDESC d2 = (LPDESC) fdwatch_get_client_data(m_lpFdw, event_idx);

			if (d2 != this)
				continue;

			switch (fdwatch_check_event(m_lpFdw, m_sock, event_idx))
			{
				case FDW_WRITE:
					event_triggered = true;

					if (ProcessOutput() < 0)
					{
						SysLog("Cannot flush output buffer");
						SetPhase(PHASE_CLOSE);
					}
					break;

				case FDW_EOF:
					SetPhase(PHASE_CLOSE);
					break;
			}
		}

		if (IsPhase(PHASE_CLOSE))
			break;
	}

	if (buffer_size(m_lpOutputBuffer) == 0)
	{
		PyLog("FLUSH SUCCESS");
	}
	else
		PyLog("FLUSH FAIL");

	usleep(250000);
}

EVENTFUNC(disconnect_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>(event->info);

	if (info == nullptr)
	{
		SysLog("disconnect_event> <Factor> Null pointer");
		return 0;
	}

	LPDESC d = info->desc;

	d->m_pDisconnectEvent = nullptr;
	d->SetPhase(PHASE_CLOSE);
	return 0;
}

bool DESC::DelayedDisconnect(int32_t iSec)
{
	if (m_pDisconnectEvent != nullptr) {
		return false;
	}

	desc_event_info* info = AllocEventInfo<desc_event_info>();
	info->desc = this;

	m_pDisconnectEvent = event_create(disconnect_event, info, PASSES_PER_SEC(iSec));
	return true;
}

void DESC::DisconnectOfSameLogin()
{
	if (GetCharacter())
	{
		if (m_pDisconnectEvent)
			return;

		GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Someone has logged into your account. You will be disconnected from the server."));
		DelayedDisconnect(5);
	}
	else
	{
		SetPhase(PHASE_CLOSE);
	}
}

void DESC::SetAdminMode()
{
	m_bAdminMode = true;
}

bool DESC::IsAdminMode()
{
	return m_bAdminMode;
}

void DESC::SendLoginSuccessPacket()
{
	TAccountTable& rTable = GetAccountTable();

	TPacketGCLoginSuccess p;

	p.bHeader    = HEADER_GC_LOGIN_SUCCESS;

	p.handle     = GetHandle();
	p.random_key = DESC_MANAGER::GetInstance()->MakeRandomKey(GetHandle()); // FOR MARK
	memcpy(p.players, rTable.players, sizeof(rTable.players));

	for (int32_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{   
		CGuild* g = CGuildManager::GetInstance()->GetLinkedGuild(rTable.players[i].dwID);

		if (g)
		{   
			p.guild_id[i] = g->GetID();
			strlcpy(p.guild_name[i], g->GetName(), sizeof(p.guild_name[i]));
		}   
		else
		{
			p.guild_id[i] = 0;
			p.guild_name[i][0] = '\0';
		}
	}

	Packet(&p, sizeof(TPacketGCLoginSuccess));
}

//void DESC::SendServerStatePacket(int32_t nIndex)
//{
//	TPacketGCStateCheck rp;
//
//	int32_t iTotal; 
//	int32_t* paiEmpireUserCount;
//	int32_t iLocal;
//
//	DESC_MANAGER::GetInstance()->GetUserCount(iTotal, &paiEmpireUserCount, iLocal);
//
//	rp.header	= 1; 
//	rp.key		= 0;
//	rp.index	= nIndex;
//
//	if (g_bNoMoreClient) rp.state = 0;
//	else rp.state = iTotal > g_iFullUserCount ? 3 : iTotal > g_iBusyUserCount ? 2 : 1;
//	
//	this->Packet(&rp, sizeof(rp));
//	//printf("STATE_CHECK PACKET PROCESSED.\n");
//}

void DESC::SetLoginKey(uint32_t dwKey)
{
	m_dwLoginKey = dwKey;
}

void DESC::SetLoginKey(CLoginKey* pKey)
{
	m_pLoginKey = pKey;
	PyLog("SetLoginKey {}", m_pLoginKey->m_dwKey);
}

uint32_t DESC::GetLoginKey()
{
	if (m_pLoginKey)
		return m_pLoginKey->m_dwKey;

	return m_dwLoginKey;
}

const uint8_t* GetKey_20050304Myevan()
{   
	static bool bGenerated = false;
	static uint32_t s_adwKey[1938]; 

	if (!bGenerated) 
	{
		bGenerated = true;
		uint32_t seed = 1491971513; 

		for (UINT i = 0; i < uint8_t(seed); ++i)
		{
			seed ^= 2148941891ul;
			seed += 3592385981ul;

			s_adwKey[i] = seed;
		}
	}

	return (const uint8_t*)s_adwKey;
}

void DESC::SetSecurityKey(const uint32_t* c_pdwKey)
{
	const uint8_t* c_pszKey = (const uint8_t*) "JyTxtHljHJlVJHorRM301vf@4fvj10-v";

	memcpy(&m_adwDecryptionKey, c_pdwKey, 16);
	TEA_Encrypt(&m_adwEncryptionKey[0], &m_adwDecryptionKey[0], (const uint32_t*) c_pszKey, 16);

	PyLog("SetSecurityKey decrypt {} {} {} {} encrypt {} {} {} {}", 
			m_adwDecryptionKey[0], m_adwDecryptionKey[1], m_adwDecryptionKey[2], m_adwDecryptionKey[3],
			m_adwEncryptionKey[0], m_adwEncryptionKey[1], m_adwEncryptionKey[2], m_adwEncryptionKey[3]);
}

void DESC::AssembleCRCMagicCube(uint8_t bProcPiece, uint8_t bFilePiece)
{
	static uint8_t abXORTable[32] =
	{
		102,  30, 0, 0, 0, 0, 0, 0,
		188,  44, 0, 0, 0, 0, 0, 0,
		39, 201, 0, 0, 0, 0, 0, 0,
		43,   5, 0, 0, 0, 0, 0, 0,
	};

	bProcPiece = (bProcPiece ^ abXORTable[m_bCRCMagicCubeIdx]);
	bFilePiece = (bFilePiece ^ abXORTable[m_bCRCMagicCubeIdx+1]);

	m_dwProcCRC |= bProcPiece << m_bCRCMagicCubeIdx;
	m_dwFileCRC |= bFilePiece << m_bCRCMagicCubeIdx;

	m_bCRCMagicCubeIdx += 8;

	if (!(m_bCRCMagicCubeIdx & 31))
	{
		m_dwProcCRC = 0;
		m_dwFileCRC = 0;
		m_bCRCMagicCubeIdx = 0;
	}
}

void DESC::push_seq(uint8_t hdr, uint8_t seq)
{
	if (m_seq_vector.size()>=20)
	{
		m_seq_vector.erase(m_seq_vector.begin());
	}

	seq_t info = { hdr, seq };
	m_seq_vector.push_back(info);
}

uint8_t DESC::GetEmpire()
{
	return m_accountTable.bEmpire;
}

void DESC::ChatPacket(uint8_t type, const char* format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int32_t len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	struct packet_chat pack_chat;

	pack_chat.header    = HEADER_GC_CHAT;
	pack_chat.size      = sizeof(struct packet_chat) + len;
	pack_chat.type      = type;
	pack_chat.id        = 0;
	pack_chat.bEmpire   = GetEmpire();

	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(struct packet_chat));
	buf.write(chatbuf, len);

	Packet(buf.read_peek(), buf.size());
}

