#include "stdafx.h"
#include <Common/stl.h>

#include "constants.h"
#include "config.h"
#include "p2p.h"
#include "desc_p2p.h"
#include "char.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "marriage.h"
#include "utils.h"
#include "locale_service.h"
#include <sstream>

P2P_MANAGER::P2P_MANAGER()
{
	m_pInputProcessor = nullptr;
	m_iHandleCount = 0;

	memset(m_aiEmpireUserCount, 0, sizeof(m_aiEmpireUserCount));
}

P2P_MANAGER::~P2P_MANAGER()
{
}

void P2P_MANAGER::Boot(LPDESC d)
{
	CHARACTER_MANAGER::NAME_MAP & map = CHARACTER_MANAGER::GetInstance()->GetPCMap();
	CHARACTER_MANAGER::NAME_MAP::iterator it = map.begin();

	TPacketGGLogin p;

	while (it != map.end())
	{
		LPCHARACTER ch = it->second;
		it++;

		p.bHeader = HEADER_GG_LOGIN;
		strlcpy(p.szName, ch->GetName(), sizeof(p.szName));
		p.dwPID = ch->GetPlayerID();
		p.bEmpire = ch->GetEmpire();
		p.lMapIndex = SECTREE_MANAGER::GetInstance()->GetMapIndex(ch->GetX(), ch->GetY());
		p.bChannel = g_bChannel;

		d->Packet(&p, sizeof(p));
	}
}

void P2P_MANAGER::FlushOutput()
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pPeers.begin();

	while (it != m_set_pPeers.end())
	{
		LPDESC pDesc = *it++;
		pDesc->FlushOutput();
	}
}

void P2P_MANAGER::RegisterAcceptor(LPDESC d)
{
	PyLog("P2P Acceptor opened (host {})", d->GetHostName());
	m_set_pPeers.insert(d);
	Boot(d);
}

void P2P_MANAGER::UnregisterAcceptor(LPDESC d)
{
	PyLog("P2P Acceptor closed (host {})", d->GetHostName());
	EraseUserByDesc(d);
	m_set_pPeers.erase(d);
}

void P2P_MANAGER::RegisterConnector(LPDESC d)
{
	PyLog("P2P Connector opened (host {})", d->GetHostName());
	m_set_pPeers.insert(d);
	Boot(d);

	TPacketGGSetup p;
	p.bHeader = HEADER_GG_SETUP;
	p.wPort = p2p_port;
	p.bChannel = g_bChannel;
	d->Packet(&p, sizeof(p));
}

void P2P_MANAGER::UnregisterConnector(LPDESC d)
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pPeers.find(d);

	if (it != m_set_pPeers.end())
	{
		PyLog("P2P Connector closed (host {})", d->GetHostName());
		EraseUserByDesc(d);
		m_set_pPeers.erase(it);
	}
}

void P2P_MANAGER::EraseUserByDesc(LPDESC d)
{
	TCCIMap::iterator it = m_map_pCCI.begin();

	while (it != m_map_pCCI.end())
	{
		CCI* pCCI = it->second;
		it++;

		if (pCCI->pDesc == d)
			Logout(pCCI);
	}
}

void P2P_MANAGER::Send(const void* c_pvData, int32_t iSize, LPDESC except)
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pPeers.begin();

	while (it != m_set_pPeers.end())
	{
		LPDESC pDesc = *it++;

		if (except == pDesc)
			continue;

		pDesc->Packet(c_pvData, iSize);
	}
}

void P2P_MANAGER::Login(LPDESC d, const TPacketGGLogin* p)
{
	CCI* pCCI = Find(p->szName);

	bool UpdateP2P = false;

	if (!pCCI)
	{
		UpdateP2P = true;
		pCCI = M2_NEW CCI;

		strlcpy(pCCI->szName, p->szName, sizeof(pCCI->szName));

		pCCI->dwPID = p->dwPID;
		pCCI->bEmpire = p->bEmpire;

		if (p->bChannel == g_bChannel)
		{
			if (pCCI->bEmpire < EMPIRE_MAX_NUM)
			{
				++m_aiEmpireUserCount[pCCI->bEmpire];
			}
			else
			{
				SysLog("LOGIN_EMPIRE_ERROR: {} >= MAX({})", pCCI->bEmpire, EMPIRE_MAX_NUM);
			}
		}

		m_map_pCCI.insert(std::make_pair(pCCI->szName, pCCI));
		m_map_dwPID_pCCI.insert(std::make_pair(pCCI->dwPID, pCCI));
	}

	pCCI->lMapIndex = p->lMapIndex;
	pCCI->pDesc = d;
	pCCI->bChannel = p->bChannel;
	PyLog("P2P: Login {}", pCCI->szName);

	CGuildManager::GetInstance()->P2PLoginMember(pCCI->dwPID);
	CPartyManager::GetInstance()->P2PLogin(pCCI->dwPID, pCCI->szName);

	if (UpdateP2P) {
		std::string name(pCCI->szName);
	    MessengerManager::GetInstance()->P2PLogin(name);
	}
}

void P2P_MANAGER::Logout(CCI* pCCI)
{
	if (pCCI->bChannel == g_bChannel)
	{
		if (pCCI->bEmpire < EMPIRE_MAX_NUM)
		{
			--m_aiEmpireUserCount[pCCI->bEmpire];
			if (m_aiEmpireUserCount[pCCI->bEmpire] < 0)
			{
				SysLog("m_aiEmpireUserCount[{}] < 0", pCCI->bEmpire);
			}
		}
		else
		{
			SysLog("LOGOUT_EMPIRE_ERROR: {} >= MAX({})", pCCI->bEmpire, EMPIRE_MAX_NUM);
		}
	}

	std::string name(pCCI->szName);

	CGuildManager::GetInstance()->P2PLogoutMember(pCCI->dwPID);
	CPartyManager::GetInstance()->P2PLogout(pCCI->dwPID);
	MessengerManager::GetInstance()->P2PLogout(name);
	marriage::CManager::GetInstance()->Logout(pCCI->dwPID);

	m_map_pCCI.erase(name);
	m_map_dwPID_pCCI.erase(pCCI->dwPID);
	M2_DELETE(pCCI);
}

void P2P_MANAGER::Logout(const char* c_pszName)
{
	CCI* pCCI = Find(c_pszName);

	if (!pCCI)
		return;

	Logout(pCCI);
	PyLog("P2P: Logout {}", c_pszName);
}

CCI * P2P_MANAGER::FindByPID(uint32_t pid)
{
	TPIDCCIMap::iterator it = m_map_dwPID_pCCI.find(pid);
	if (it == m_map_dwPID_pCCI.end())
		return NULL;
	return it->second;
}

CCI * P2P_MANAGER::Find(const char* c_pszName)
{
	TCCIMap::const_iterator it;

	it = m_map_pCCI.find(c_pszName);

	if (it == m_map_pCCI.end())
		return NULL;

	return it->second;
}

int32_t P2P_MANAGER::GetCount()
{
	return m_aiEmpireUserCount[1] + m_aiEmpireUserCount[2] + m_aiEmpireUserCount[3];
}

int32_t P2P_MANAGER::GetEmpireUserCount(int32_t idx)
{
	assert(idx < EMPIRE_MAX_NUM);
	return m_aiEmpireUserCount[idx];
}


int32_t P2P_MANAGER::GetDescCount()
{
	return m_set_pPeers.size();
}

void P2P_MANAGER::GetP2PHostNames(std::string& hostNames)
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pPeers.begin();

	std::ostringstream oss(std::ostringstream::out);

	while (it != m_set_pPeers.end())
	{
		LPDESC pDesc = *it++;

		oss << pDesc->GetP2PHost() << " " << pDesc->GetP2PPort() << "\n";

	}
	hostNames += oss.str();
}
