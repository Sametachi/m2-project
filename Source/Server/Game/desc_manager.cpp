#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "desc.h"
#include "desc_p2p.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "messenger_manager.h"
#include "p2p.h"
#include "ip_ban.h"

struct valid_ip
{
	const char* 	ip;
	uint8_t	network;
	uint8_t	mask;
};

static struct valid_ip admin_ip[] =
{
	{ "210.123.10",     128,    128     },
	{ "\n",             0,      0       }
};

int32_t IsValidIP(struct valid_ip* ip_table, const char* host)
{
	int32_t         i, j;
	char        ip_addr[256];

	for (i = 0; *(ip_table + i)->ip != '\n'; ++i)
	{
		j = 255 - (ip_table + i)->mask;

		do
		{
			snprintf(ip_addr, sizeof(ip_addr), "%s.%d", (ip_table + i)->ip, (ip_table + i)->network + j);

			if (!strcmp(ip_addr, host))
				return TRUE;

			if (!j)
				break;
		}
		while (j--);
	}

	return FALSE;
}

DESC_MANAGER::DESC_MANAGER() : m_bDestroyed(false)
{
	Initialize();
	//NOTE: What else is calling Initialize at the end of Destroy..-_-; really
}

DESC_MANAGER::~DESC_MANAGER()
{
	Destroy();
}

void DESC_MANAGER::Initialize()
{
	m_iSocketsConnected = 0;
	m_iHandleCount = 0;
	m_iLocalUserCount = 0;
	memset(m_aiEmpireUserCount, 0, sizeof(m_aiEmpireUserCount));
	m_bDisconnectInvalidCRC = false;
}

void DESC_MANAGER::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	DESC_SET::iterator i = m_set_pDesc.begin();

	while (i != m_set_pDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->GetType() == DESC_TYPE_CONNECTOR)
			continue;

		if (d->IsPhase(PHASE_P2P))
			continue;

		DestroyDesc(d, false);
		m_set_pDesc.erase(ci);
	}

	i = m_set_pDesc.begin();

	while (i != m_set_pDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		DestroyDesc(d, false);
		m_set_pDesc.erase(ci);
	}

	m_set_pClientDesc.clear();

	m_map_loginName.clear();
	m_map_handle.clear();

	Initialize();
}

uint32_t DESC_MANAGER::CreateHandshake()
{
	char crc_buf[8];
	crc_t crc;
	DESC_HANDSHAKE_MAP::iterator it;

RETRY:
	do
	{
		uint32_t val = thecore_random() % (1024 * 1024);

		*(uint32_t*) (crc_buf) = val;
		*((uint32_t*) crc_buf + 1) = get_global_time();

		crc = GetCRC32(crc_buf, 8);
		it = m_map_handshake.find(crc);
	}
	while (it != m_map_handshake.end());

	if (crc == 0)
		goto RETRY;

	return (crc);
}

LPDESC DESC_MANAGER::AcceptDesc(LPFDWATCH fdw, socket_t s)
{
	socket_t                    desc;
	LPDESC						newd;
	static struct sockaddr_in   peer;
	static char					host[MAX_HOST_LENGTH + 1];

	if ((desc = socket_accept(s, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	if (g_bAuthServer)
	{
		if (IsBanIP(peer.sin_addr))
		{
			PyLog("connection from {} was banned.", host);
			socket_close(desc);
			return NULL;
		}
	}

	if (!IsValidIP(admin_ip, host)) // The IP registered in admin_ip is not limited by the maximum number of users.
	{
		if (m_iSocketsConnected >= MAX_ALLOW_USER)
		{
			SysLog("max connection reached. MAX_ALLOW_USER = {}", MAX_ALLOW_USER);
			socket_close(desc);
			return NULL;
		}
	}

	newd = M2_NEW DESC;
	crc_t handshake = CreateHandshake();

	if (!newd->Setup(fdw, desc, peer, ++m_iHandleCount, handshake))
	{
		socket_close(desc);
		M2_DELETE(newd);
		return NULL;
	}

	m_map_handshake.insert(DESC_HANDSHAKE_MAP::value_type(handshake, newd));
	m_map_handle.insert(DESC_HANDLE_MAP::value_type(newd->GetHandle(), newd));

	m_set_pDesc.insert(newd);
	++m_iSocketsConnected;
	return (newd);
}

LPDESC DESC_MANAGER::AcceptP2PDesc(LPFDWATCH fdw, socket_t bind_fd)
{
	socket_t           fd;
	struct sockaddr_in peer;
	char               host[MAX_HOST_LENGTH + 1];

	if ((fd = socket_accept(bind_fd, &peer)) == -1)
		return NULL;

	strlcpy(host, inet_ntoa(peer.sin_addr), sizeof(host));

	LPDESC_P2P pDesc = M2_NEW DESC_P2P;

	if (!pDesc->Setup(fdw, fd, host, peer.sin_port))
	{     
		SysLog("DESC_MANAGER::AcceptP2PDesc : Setup failed");
		socket_close(fd);
		M2_DELETE(pDesc);
		return NULL;
	}

	m_set_pDesc.insert(pDesc);
	++m_iSocketsConnected;

	PyLog("DESC_MANAGER::AcceptP2PDesc  {}:{}", host, peer.sin_port);
	P2P_MANAGER::GetInstance()->RegisterAcceptor(pDesc);
	return (pDesc);
}

void DESC_MANAGER::ConnectAccount(const std::string& login, LPDESC d)
{
	m_map_loginName.insert(DESC_LOGINNAME_MAP::value_type(login,d));
}

void DESC_MANAGER::DisconnectAccount(const std::string& login)
{
	m_map_loginName.erase(login);
}

void DESC_MANAGER::DestroyDesc(LPDESC d, bool bEraseFromSet)
{
	if (bEraseFromSet)
		m_set_pDesc.erase(d);

	if (d->GetHandshake())
		m_map_handshake.erase(d->GetHandshake());

	if (d->GetHandle() != 0)
		m_map_handle.erase(d->GetHandle());
	else
		m_set_pClientDesc.erase((LPCLIENT_DESC) d);

	d->Destroy();

	M2_DELETE(d);
	--m_iSocketsConnected;
}

void DESC_MANAGER::DestroyClosed()
{
	DESC_SET::iterator i = m_set_pDesc.begin();

	while (i != m_set_pDesc.end())
	{
		LPDESC d = *i;
		DESC_SET::iterator ci = i;
		++i;

		if (d->IsPhase(PHASE_CLOSE))
		{
			if (d->GetType() == DESC_TYPE_CONNECTOR)
			{
				LPCLIENT_DESC client_desc = (LPCLIENT_DESC)d;

				if (client_desc->IsRetryWhenClosed())
				{
					client_desc->Reset();
					continue;
				}
			}

			DestroyDesc(d, false);
			m_set_pDesc.erase(ci);
		}
	}
}

LPDESC DESC_MANAGER::FindByLoginName(const std::string& login)
{
	DESC_LOGINNAME_MAP::iterator it = m_map_loginName.find(login);

	if (m_map_loginName.end() == it)
		return NULL;

	return (it->second); 
}

LPDESC DESC_MANAGER::FindByHandle(uint32_t handle)
{
	DESC_HANDLE_MAP::iterator it = m_map_handle.find(handle);

	if (m_map_handle.end() == it)
		return NULL;

	return (it->second); 
}

const DESC_MANAGER::DESC_SET & DESC_MANAGER::GetClientSet()
{
	return m_set_pDesc;
}

struct name_with_desc_func
{
	const char* m_name;

	name_with_desc_func(const char* name) : m_name(name)
	{
	}

	bool operator () (LPDESC d)
	{
		if (d->GetCharacter() && !strcmp(d->GetCharacter()->GetName(), m_name))
			return true;

		return false;
	}
};

LPDESC DESC_MANAGER::FindByCharacterName(const char* name)
{
	DESC_SET::iterator it = std::find_if (m_set_pDesc.begin(), m_set_pDesc.end(), name_with_desc_func(name));
	return (it == m_set_pDesc.end()) ? NULL : (*it);
}

LPCLIENT_DESC DESC_MANAGER::CreateConnectionDesc(LPFDWATCH fdw, const char* host, uint16_t port, int32_t iPhaseWhenSucceed, bool bRetryWhenClosed)
{
	LPCLIENT_DESC newd;

	newd = M2_NEW CLIENT_DESC;

	newd->Setup(fdw, host, port);
	newd->Connect(iPhaseWhenSucceed);
	newd->SetRetryWhenClosed(bRetryWhenClosed);

	m_set_pDesc.insert(newd);
	m_set_pClientDesc.insert(newd);

	++m_iSocketsConnected;
	return (newd);
}

struct FuncTryConnect
{
	void operator () (LPDESC d)
	{
		((LPCLIENT_DESC)d)->Connect();
	}
};

void DESC_MANAGER::TryConnect()
{
	FuncTryConnect f;
	std::for_each(m_set_pClientDesc.begin(), m_set_pClientDesc.end(), f);
}

bool DESC_MANAGER::IsP2PDescExist(const char* szHost, uint16_t wPort)
{
	CLIENT_DESC_SET::iterator it = m_set_pClientDesc.begin();
	
	while (it != m_set_pClientDesc.end())
	{
		LPCLIENT_DESC d = *(it++);

		if (!strcmp(d->GetP2PHost(), szHost) && d->GetP2PPort() == wPort)
			return true;
	}

	return false;
}

LPDESC DESC_MANAGER::FindByHandshake(uint32_t dwHandshake)
{
	DESC_HANDSHAKE_MAP::iterator it = m_map_handshake.find(dwHandshake);

	if (it == m_map_handshake.end())
		return NULL;

	return (it->second);
}

class FuncWho
{
	public:
		int32_t iTotalCount;
		int32_t aiEmpireUserCount[EMPIRE_MAX_NUM];

		FuncWho()
		{
			iTotalCount = 0;
			memset(aiEmpireUserCount, 0, sizeof(aiEmpireUserCount));
		}

		void operator() (LPDESC d)
		{
			if (d->GetCharacter())
			{
				++iTotalCount;
				++aiEmpireUserCount[d->GetEmpire()];
			}
		}
};

void DESC_MANAGER::UpdateLocalUserCount()
{
	const DESC_SET& c_ref_set = GetClientSet();
	FuncWho f;
	f = std::for_each(c_ref_set.begin(), c_ref_set.end(), f);

	m_iLocalUserCount = f.iTotalCount;
	memcpy(m_aiEmpireUserCount, f.aiEmpireUserCount, sizeof(m_aiEmpireUserCount));

	m_aiEmpireUserCount[1] += P2P_MANAGER::GetInstance()->GetEmpireUserCount(1);
	m_aiEmpireUserCount[2] += P2P_MANAGER::GetInstance()->GetEmpireUserCount(2);
	m_aiEmpireUserCount[3] += P2P_MANAGER::GetInstance()->GetEmpireUserCount(3);
}

void DESC_MANAGER::GetUserCount(int32_t & iTotal, int32_t** paiEmpireUserCount, int32_t & iLocalCount)
{
	*paiEmpireUserCount = &m_aiEmpireUserCount[0];
	
	int32_t iCount = P2P_MANAGER::GetInstance()->GetCount();
	if (iCount < 0)
	{
		SysLog("P2P_MANAGER::GetInstance()->GetCount() == -1");
	}
	iTotal = m_iLocalUserCount + iCount;
	iLocalCount = m_iLocalUserCount;
}


uint32_t DESC_MANAGER::MakeRandomKey(uint32_t dwHandle)
{ 
	uint32_t random_key = thecore_random(); 
	m_map_handle_random_key.insert(std::make_pair(dwHandle, random_key));
	return random_key;
}

bool DESC_MANAGER::GetRandomKey(uint32_t dwHandle, uint32_t* prandom_key)
{
	DESC_HANDLE_RANDOM_KEY_MAP::iterator it = m_map_handle_random_key.find(dwHandle); 

	if (it == m_map_handle_random_key.end())
		return false;

	*prandom_key = it->second;
	return true;
}

LPDESC DESC_MANAGER::FindByLoginKey(uint32_t dwKey)
{
	std::map<uint32_t, CLoginKey *>::iterator it = m_map_pLoginKey.find(dwKey);

	if (it == m_map_pLoginKey.end())
		return NULL;

	return it->second->m_pDesc;
}


uint32_t DESC_MANAGER::CreateLoginKey(LPDESC d)
{
	uint32_t dwKey = 0;

	do
	{
		dwKey = number(1, INT_MAX);

		if (m_map_pLoginKey.find(dwKey) != m_map_pLoginKey.end())
			continue;

		CLoginKey* pKey = M2_NEW CLoginKey(dwKey, d);
		d->SetLoginKey(pKey);
		m_map_pLoginKey.insert(std::make_pair(dwKey, pKey));
		break;
	} while (1);

	return dwKey;
}

void DESC_MANAGER::ProcessExpiredLoginKey()
{
	uint32_t dwCurrentTime = get_dword_time();

	std::map<uint32_t, CLoginKey *>::iterator it, it2;

	it = m_map_pLoginKey.begin();

	while (it != m_map_pLoginKey.end())
	{
		it2 = it++;

		if (it2->second->m_dwExpireTime == 0)
			continue;

		if (dwCurrentTime - it2->second->m_dwExpireTime > 60000)
		{
			M2_DELETE(it2->second);
			m_map_pLoginKey.erase(it2);
		}
	}
}

