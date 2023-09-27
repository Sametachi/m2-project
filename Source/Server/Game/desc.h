#pragma once
#include "constants.h"
#include "input.h"

#define MAX_ALLOW_USER                  4096
#define MAX_INPUT_LEN			65536

#define HANDSHAKE_RETRY_LIMIT		32

class CInputProcessor;

enum EDescType
{
	DESC_TYPE_ACCEPTOR,
	DESC_TYPE_CONNECTOR
};

class CLoginKey
{
	public:
		CLoginKey(uint32_t dwKey, LPDESC pDesc) : m_dwKey(dwKey), m_pDesc(pDesc)
		{
			m_dwExpireTime = 0;
		}

		void Expire()
		{
			m_dwExpireTime = get_dword_time();
			m_pDesc = nullptr;
		}

		operator uint32_t() const
		{
			return m_dwKey;
		}

		uint32_t   m_dwKey;
		uint32_t   m_dwExpireTime;
		LPDESC  m_pDesc;
};


struct seq_t
{
	uint8_t	hdr;
	uint8_t	seq;
};
typedef std::vector<seq_t>	seq_vector_t;

class DESC
{
	public:
		EVENTINFO(desc_event_info)
		{
			LPDESC desc;

			desc_event_info() 
			: desc(0)
			{
			}
		};

	public:
		DESC();
		virtual ~DESC();

		virtual uint8_t		GetType() { return DESC_TYPE_ACCEPTOR; }
		virtual void		Destroy();
		virtual void		SetPhase(int32_t _phase);

		void			FlushOutput();

		bool			Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in& c_rSockAddr, uint32_t _handle, uint32_t _handshake);

		socket_t		GetSocket() const	{ return m_sock; }
		const char* 	GetHostName()		{ return m_stHost.c_str(); }
		uint16_t			GetPort()	{ return m_wPort; }

		void			SetP2P(const char* h, uint16_t w, uint8_t b) { m_stP2PHost = h; m_wP2PPort = w; m_bP2PChannel = b; }
		const char* 	GetP2PHost()		{ return m_stP2PHost.c_str();	}
		uint16_t			GetP2PPort() const		{ return m_wP2PPort; }
		uint8_t			GetP2PChannel() const	{ return m_bP2PChannel;	}

		void			BufferedPacket(const void* c_pvData, int32_t iSize);
		void			Packet(const void* c_pvData, int32_t iSize);
		void			LargePacket(const void* c_pvData, int32_t iSize);

		int32_t			ProcessInput();
		int32_t			ProcessOutput();

		CInputProcessor	*	GetInputProcessor()	{ return m_pInputProcessor; }

		uint32_t			GetHandle() const	{ return m_dwHandle; }
		LPBUFFER		GetOutputBuffer()	{ return m_lpOutputBuffer; }

		void			BindAccountTable(TAccountTable* pTable);
		TAccountTable &		GetAccountTable()	{ return m_accountTable; }

		void			BindCharacter(LPCHARACTER ch);
		LPCHARACTER		GetCharacter()		{ return m_lpCharacter; }

		bool			IsPhase(int32_t phase) const	{ return m_iPhase == phase ? true : false; }

		const struct sockaddr_in & GetAddr()		{ return m_SockAddr;	}

		void			   UDPGrant(const struct sockaddr_in& c_rSockAddr);
		const struct sockaddr_in & GetUDPAddr()		{ return m_UDPSockAddr; }

		void			Log(const char* format, ...);

		void			StartHandshake(uint32_t _dw);
		void			SendHandshake(uint32_t dwCurTime, int32_t lNewDelta);
		bool			HandshakeProcess(uint32_t dwTime, int32_t lDelta, bool bInfiniteRetry=false);
		bool			IsHandshaking();

		uint32_t			GetHandshake() const	{ return m_dwHandshake; }
		uint32_t			GetClientTime();

		void			SetSecurityKey(const uint32_t* c_pdwKey);
		const uint32_t*	GetEncryptionKey() const { return &m_adwEncryptionKey[0]; }
		const uint32_t*	GetDecryptionKey() const { return &m_adwDecryptionKey[0]; }

		uint8_t			GetEmpire();

		void			SetRelay(const char* c_pszName);
		bool			DelayedDisconnect(int32_t iSec);
		void			DisconnectOfSameLogin();

		void			SetAdminMode();
		bool			IsAdminMode();

		void			SetPong(bool b);
		bool			IsPong();

		void			SendLoginSuccessPacket();
		
		void			SetLoginKey(uint32_t dwKey);
		void			SetLoginKey(CLoginKey* pKey);
		uint32_t			GetLoginKey();

		void			AssembleCRCMagicCube(uint8_t bProcPiece, uint8_t bFilePiece);

		bool			isChannelStatusRequested() const { return m_bChannelStatusRequested; }
		void			SetChannelStatusRequested(bool bChannelStatusRequested) { m_bChannelStatusRequested = bChannelStatusRequested; }

	protected:
		void			Initialize();

	protected:
		CInputProcessor *	m_pInputProcessor;
		CInputClose		m_inputClose;
		CInputHandshake	m_inputHandshake;
		CInputLogin		m_inputLogin;
		CInputMain		m_inputMain;
		CInputDead		m_inputDead;
		CInputAuth		m_inputAuth;


		LPFDWATCH		m_lpFdw;
		socket_t		m_sock;
		int32_t				m_iPhase;
		uint32_t			m_dwHandle;

		std::string		m_stHost;
		uint16_t			m_wPort;
		time_t			m_LastTryToConnectTime;

		LPBUFFER		m_lpInputBuffer;
		int32_t				m_iMinInputBufferLen;
	
		uint32_t			m_dwHandshake;
		uint32_t			m_dwHandshakeSentTime;
		int32_t				m_iHandshakeRetry;
		uint32_t			m_dwClientTime;
		bool			m_bHandshaking;

		LPBUFFER		m_lpBufferedOutputBuffer;
		LPBUFFER		m_lpOutputBuffer;

		LPEVENT			m_pPingEvent;
		LPCHARACTER		m_lpCharacter;
		TAccountTable		m_accountTable;

		struct sockaddr_in	m_SockAddr;
		struct sockaddr_in 	m_UDPSockAddr;

		FILE *			m_pLogFile;
		std::string		m_stRelayName;

		std::string		m_stP2PHost;
		uint16_t			m_wP2PPort;
		uint8_t			m_bP2PChannel;

		bool			m_bAdminMode;
		bool			m_bPong;

		CLoginKey *		m_pLoginKey;
		uint32_t			m_dwLoginKey;

		uint8_t                    m_bCRCMagicCubeIdx;
		uint32_t                   m_dwProcCRC;
		uint32_t                   m_dwFileCRC;
		bool			m_bHackCRCQuery;

		std::string		m_Login;
		int32_t				m_outtime;
		int32_t				m_playtime;
		int32_t				m_offtime;

		bool			m_bDestroyed;
		bool			m_bChannelStatusRequested;

		bool			m_bEncrypted;
		uint32_t			m_adwDecryptionKey[4];
		uint32_t			m_adwEncryptionKey[4];

	public:
		LPEVENT			m_pDisconnectEvent;


	public:
		void SetLogin(const std::string & login) { m_Login = login; }
		void SetLogin(const char* login) { m_Login = login; }
		const std::string& GetLogin() { return m_Login; }

		void SetOutTime(int32_t outtime) { m_outtime = outtime; }
		void SetOffTime(int32_t offtime) { m_offtime = offtime; }
		void SetPlayTime(int32_t playtime) { m_playtime = playtime; }

		void RawPacket(const void* c_pvData, int32_t iSize);
		void ChatPacket(uint8_t type, const char* format, ...);

		/* Code for finding sequence bugs */
	public:
		seq_vector_t	m_seq_vector;
		void			push_seq (uint8_t hdr, uint8_t seq);
		
};
