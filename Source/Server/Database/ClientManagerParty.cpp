#include "stdafx.h"
#include "ClientManager.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"

void CClientManager::QUERY_PARTY_CREATE(LPPEER pPeer, TPacketPartyCreate* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];

	if (pm.find(p->dwLeaderPID) == pm.end())
	{
		pm.emplace(p->dwLeaderPID, TPartyMember());
		ForwardPacket(HEADER_DG_PARTY_CREATE, p, sizeof(TPacketPartyCreate), pPeer->GetChannel(), pPeer);
		TraceLog("PARTY Create [{}]", p->dwLeaderPID);
	}
	else
	{
		WarnLog("PARTY Create - Already exists [{}]", p->dwLeaderPID);
	}
}

void CClientManager::QUERY_PARTY_DELETE(LPPEER pPeer, TPacketPartyDelete* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];
	auto it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		WarnLog("PARTY Delete - Non exists [{}]", p->dwLeaderPID);
		return;
	}

	pm.erase(it);
	ForwardPacket(HEADER_DG_PARTY_DELETE, p, sizeof(TPacketPartyDelete), pPeer->GetChannel(), pPeer);
	TraceLog("PARTY Delete [{}]", p->dwLeaderPID);
}

void CClientManager::QUERY_PARTY_ADD(LPPEER pPeer, TPacketPartyAdd* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];
	auto it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		WarnLog("PARTY Add - Non exists [{} uyhn]", p->dwLeaderPID);
		return;
	}

	if (it->second.find(p->dwPID) == it->second.end())
	{
		it->second.emplace(p->dwPID, TPartyInfo());
		ForwardPacket(HEADER_DG_PARTY_ADD, p, sizeof(TPacketPartyAdd), pPeer->GetChannel(), pPeer);
		TraceLog("PARTY Add [{}] to [{}]", p->dwPID, p->dwLeaderPID);
	}
	else
	{
		WarnLog("PARTY Add - Already [{}] in party [{}]", p->dwPID, p->dwLeaderPID);
	}
}

void CClientManager::QUERY_PARTY_REMOVE(LPPEER pPeer, TPacketPartyRemove* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];
	auto it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		WarnLog("PARTY Remove - Non exists [{}] cannot remove [{}]",p->dwLeaderPID, p->dwPID);
		return;
	}

	auto pit = it->second.find(p->dwPID);

	if (pit != it->second.end())
	{
		it->second.erase(pit);
		ForwardPacket(HEADER_DG_PARTY_REMOVE, p, sizeof(TPacketPartyRemove), pPeer->GetChannel(), pPeer);
		TraceLog("PARTY Remove [{}] to [{}]", p->dwPID, p->dwLeaderPID);
	}
	else
	{
		WarnLog("PARTY Remove - Cannot find [{}] in party [{}]", p->dwPID, p->dwLeaderPID);
	}
}

void CClientManager::QUERY_PARTY_STATE_CHANGE(LPPEER pPeer, TPacketPartyStateChange* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];
	auto it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		WarnLog("PARTY StateChange - Non exists [{}] cannot state change [{}]",p->dwLeaderPID, p->dwPID);
		return;
	}

	auto pit = it->second.find(p->dwPID);

	if (pit == it->second.end())
	{
		WarnLog("PARTY StateChange - Cannot find [{}] in party [{}]", p->dwPID, p->dwLeaderPID);
		return;
	}

	if (p->bFlag)
		pit->second.bRole = p->bRole;
	else 
		pit->second.bRole = 0;

	ForwardPacket(HEADER_DG_PARTY_STATE_CHANGE, p, sizeof(TPacketPartyStateChange), pPeer->GetChannel(), pPeer);
	TraceLog("PARTY StateChange [{}] at [{}] from {} {}",p->dwPID, p->dwLeaderPID, p->bRole, p->bFlag);
}

void CClientManager::QUERY_PARTY_SET_MEMBER_LEVEL(LPPEER pPeer, TPacketPartySetMemberLevel* p)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];
	auto it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		WarnLog("PARTY SetMemberLevel - Non exists [{}] cannot level change [{}]",p->dwLeaderPID, p->dwPID);
		return;
	}

	auto pit = it->second.find(p->dwPID);

	if (pit == it->second.end())
	{
		WarnLog("PARTY SetMemberLevel - Cannot find [{}] in party [{}]", p->dwPID, p->dwLeaderPID);
		return;
	}

	pit->second.bLevel = p->bLevel;

	ForwardPacket(HEADER_DG_PARTY_SET_MEMBER_LEVEL, p, sizeof(TPacketPartySetMemberLevel), pPeer->GetChannel());
	TraceLog("PARTY SetMemberLevel pid [{}] level {}",p->dwPID, p->bLevel);
}
