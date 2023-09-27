#include "stdafx.h"
#include "constants.h"
#include "pvp.h"
#include "crc32.h"
#include "packet.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "config.h"
#include "sectree_manager.h"
#include "buffer_manager.h"
#include "locale_service.h"

CPVP::CPVP(uint32_t dwPID1, uint32_t dwPID2)
{
	if (dwPID1 > dwPID2)
	{
		m_players[0].dwPID = dwPID1;
		m_players[1].dwPID = dwPID2;
		m_players[0].bAgree = true;
	}
	else
	{
		m_players[0].dwPID = dwPID2;
		m_players[1].dwPID = dwPID1;
		m_players[1].bAgree = true;
	}

	uint32_t adwID[2];
	adwID[0] = m_players[0].dwPID;
	adwID[1] = m_players[1].dwPID;
	m_dwCRC = GetFastHash((const char* ) &adwID, 8);
	m_bRevenge = false;

	SetLastFightTime();
}

CPVP::CPVP(CPVP & k)
{
	m_players[0] = k.m_players[0];
	m_players[1] = k.m_players[1];

	m_dwCRC = k.m_dwCRC;
	m_bRevenge = k.m_bRevenge;

	SetLastFightTime();
}

CPVP::~CPVP()
{
}

void CPVP::Packet(bool bDelete)
{
	if (!m_players[0].dwVID || !m_players[1].dwVID)
	{
		if (bDelete)
			SysLog("null vid when removing {} {}", m_players[0].dwVID, m_players[0].dwVID);

		return;
	}

	TPacketGCPVP pack;

	pack.bHeader = HEADER_GC_PVP;

	if (bDelete)
	{
		pack.bMode = PVP_MODE_NONE;
		pack.dwVIDSrc = m_players[0].dwVID;
		pack.dwVIDDst = m_players[1].dwVID;
	}
	else if (IsFight())
	{
		pack.bMode = PVP_MODE_FIGHT;
		pack.dwVIDSrc = m_players[0].dwVID;
		pack.dwVIDDst = m_players[1].dwVID;
	}
	else
	{
		pack.bMode = m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE;

		if (m_players[0].bAgree)
		{
			pack.dwVIDSrc = m_players[0].dwVID;
			pack.dwVIDDst = m_players[1].dwVID;
		}
		else
		{
			pack.dwVIDSrc = m_players[1].dwVID;
			pack.dwVIDDst = m_players[0].dwVID;
		}
	}

	const DESC_MANAGER::DESC_SET& c_rSet = DESC_MANAGER::GetInstance()->GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_rSet.begin();

	while (it != c_rSet.end())
	{
		LPDESC d = *it++;

		if (d->IsPhase(PHASE_GAME) || d->IsPhase(PHASE_DEAD))
			d->Packet(&pack, sizeof(pack));
	}
}

bool CPVP::Agree(uint32_t dwPID)
{
	m_players[m_players[0].dwPID != dwPID ? 1 : 0].bAgree = true;

	if (IsFight())
	{
		Packet();
		return true;
	}

	return false;
}

bool CPVP::IsFight()
{
	return (m_players[0].bAgree == m_players[1].bAgree) && m_players[0].bAgree;
}

void CPVP::Win(uint32_t dwPID)
{
	int32_t iSlot = m_players[0].dwPID != dwPID ? 1 : 0;

	m_bRevenge = true;

	m_players[iSlot].bAgree = true;
	m_players[!iSlot].bCanRevenge = true;
	m_players[!iSlot].bAgree = false;

	Packet();
}

bool CPVP::CanRevenge(uint32_t dwPID)
{
	return m_players[m_players[0].dwPID != dwPID ? 1 : 0].bCanRevenge;
}

void CPVP::SetVID(uint32_t dwPID, uint32_t dwVID)
{
	if (m_players[0].dwPID == dwPID)
		m_players[0].dwVID = dwVID;
	else
		m_players[1].dwVID = dwVID;
}

void CPVP::SetLastFightTime()
{
	m_dwLastFightTime = get_dword_time();
}

uint32_t CPVP::GetLastFightTime()
{
	return m_dwLastFightTime;
}

CPVPManager::CPVPManager()
{
}

CPVPManager::~CPVPManager()
{
}

void CPVPManager::Insert(LPCHARACTER pChr, LPCHARACTER pVictim)
{
	if (pChr->IsDead() || pVictim->IsDead())
		return;

	CPVP kPVP(pChr->GetPlayerID(), pVictim->GetPlayerID());

	CPVP* pPVP;

	if ((pPVP = Find(kPVP.m_dwCRC)))
	{
		if (pPVP->Agree(pChr->GetPlayerID()))
		{
			pVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The battle with %s has begun!"), pChr->GetName());
			pChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The battle with %s has begun!"), pVictim->GetName());
		}
		return;
	}

	pPVP = M2_NEW CPVP(kPVP);

	pPVP->SetVID(pChr->GetPlayerID(), pChr->GetVID());
	pPVP->SetVID(pVictim->GetPlayerID(), pVictim->GetVID());

	m_map_pPVP.insert(std::map<uint32_t, CPVP *>::value_type(pPVP->m_dwCRC, pPVP));

	m_map_pPVPSetByID[pChr->GetPlayerID()].insert(pPVP);
	m_map_pPVPSetByID[pVictim->GetPlayerID()].insert(pPVP);

	pPVP->Packet();

	char msg[CHAT_MAX_LEN + 1];
	snprintf(msg, sizeof(msg), LC_TEXT("%s challenged you to a battle!"), pChr->GetName());

	pVictim->ChatPacket(CHAT_TYPE_INFO, msg);
	pChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("You have challenged %s to a battle."), pVictim->GetName());

	LPDESC pVictimDesc = pVictim->GetDesc();
	if (pVictimDesc)
	{
		TPacketGCWhisper pack;

		int32_t len = MIN(CHAT_MAX_LEN, strlen(msg) + 1);

		pack.bHeader = HEADER_GC_WHISPER;
		pack.wSize = sizeof(TPacketGCWhisper) + len;
		pack.bType = WHISPER_TYPE_SYSTEM;
		strlcpy(pack.szNameFrom, pChr->GetName(), sizeof(pack.szNameFrom));

		TEMP_BUFFER buf;

		buf.write(&pack, sizeof(TPacketGCWhisper));
		buf.write(msg, len);

		pVictimDesc->Packet(buf.read_peek(), buf.size());
	}	
}

void CPVPManager::ConnectEx(LPCHARACTER pChr, bool bDisconnect)
{
	CPVPSetMap::iterator it = m_map_pPVPSetByID.find(pChr->GetPlayerID());

	if (it == m_map_pPVPSetByID.end())
		return;

	uint32_t dwVID = bDisconnect ? 0 : pChr->GetVID();

	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pPVP = *it2++;
		pPVP->SetVID(pChr->GetPlayerID(), dwVID);
	}
}

void CPVPManager::Connect(LPCHARACTER pChr)
{
	ConnectEx(pChr, false);
}

void CPVPManager::Disconnect(LPCHARACTER pChr)
{
}

void CPVPManager::GiveUp(LPCHARACTER pChr, uint32_t dwKillerPID) // This method is calling from no where yet.
{
	CPVPSetMap::iterator it = m_map_pPVPSetByID.find(pChr->GetPlayerID());

	if (it == m_map_pPVPSetByID.end())
		return;

	TraceLog("PVPManager::Dead {}", pChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pPVP = *it2++;

		uint32_t dwCompanionPID;

		if (pPVP->m_players[0].dwPID == pChr->GetPlayerID())
			dwCompanionPID = pPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pPVP->m_players[0].dwPID;

		if (dwCompanionPID != dwKillerPID)
			continue;

		pPVP->SetVID(pChr->GetPlayerID(), 0);

		m_map_pPVPSetByID.erase(dwCompanionPID);

		it->second.erase(pPVP);

		if (it->second.empty())
			m_map_pPVPSetByID.erase(it);

		m_map_pPVP.erase(pPVP->m_dwCRC);

		pPVP->Packet(true);
		M2_DELETE(pPVP);
		break;
	}
}

bool CPVPManager::Dead(LPCHARACTER pChr, uint32_t dwKillerPID)
{
	CPVPSetMap::iterator it = m_map_pPVPSetByID.find(pChr->GetPlayerID());

	if (it == m_map_pPVPSetByID.end())
		return false;

	bool found = false;

	TraceLog("PVPManager::Dead {}", pChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP* pPVP = *it2++;

		uint32_t dwCompanionPID;

		if (pPVP->m_players[0].dwPID == pChr->GetPlayerID())
			dwCompanionPID = pPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pPVP->m_players[0].dwPID;

		if (dwCompanionPID == dwKillerPID)
		{
			if (pPVP->IsFight())
			{
				pPVP->SetLastFightTime();
				pPVP->Win(dwKillerPID);
				found = true;
				break;
			}
			else if (get_dword_time() - pPVP->GetLastFightTime() <= 15000)
			{
				found = true;
				break;
			}
		}
	}

	return found;
}

bool CPVPManager::CanAttack(LPCHARACTER pChr, LPCHARACTER pVictim)
{
	switch (pVictim->GetCharType())
	{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			return false;
	}

	if (pChr == pVictim)
		return false;

	if (pVictim->IsNPC() && pChr->IsNPC() && !pChr->IsGuardNPC())
		return false;

	if(pChr->IsHorseRiding())
	{
		if(pChr->GetHorseLevel() > 0 && 1 == pChr->GetHorseGrade()) 
			return false;
	}
	else
	{
		switch(pChr->GetMountVnum())
		{
			case 0:
			case 20030:
			case 20110:
			case 20111:
			case 20112:
			case 20113:
			case 20114:
			case 20115:
			case 20116:
			case 20117:
			case 20118:
			case 20205:
			case 20206:
			case 20207:
			case 20208:
			case 20209:
			case 20210:
			case 20211:
			case 20212:
			case 20119:		
			case 20219:		
			case 20220:		
			case 20221:		
			case 20222:		
			case 20120:
			case 20121:
			case 20122:
			case 20123:
			case 20124:
			case 20125:
			case 20214:			
			case 20215:			
			case 20217:			
			case 20218:		
			case 20224:		
			case 20225:		
			case 20226:		
			case 20227:
				break;

			default:
				return false;
		}
	}

	if (pVictim->IsNPC() || pChr->IsNPC())
	{
		return true;
	}

	if (pVictim->IsObserverMode() || pChr->IsObserverMode())
		return false;

	{
		uint8_t bMapEmpire = SECTREE_MANAGER::GetInstance()->GetEmpireFromMapIndex(pChr->GetMapIndex());

		if (pChr->GetPKMode() == PK_MODE_PROTECT && pChr->GetEmpire() == bMapEmpire ||
				pVictim->GetPKMode() == PK_MODE_PROTECT && pVictim->GetEmpire() == bMapEmpire)
		{
			return false;
		}
	}

	if (pChr->GetEmpire() != pVictim->GetEmpire())
	{
		return true;
	}

	bool beKillerMode = false;

	if (pVictim->GetParty() && pVictim->GetParty() == pChr->GetParty())
	{
		return false;
		// Cannot attack same party on any pvp model
	}
	else
	{
		if (pVictim->IsKillerMode())
		{
			return true;
		}

		if (pChr->GetAlignment() < 0 && pVictim->GetAlignment() >= 0)
		{
		    if (g_protectNormalPlayer)
		    {
			// Offenders cannot attack good people in peace mode.
			if (PK_MODE_PEACE == pVictim->GetPKMode())
			    return false;
		    }
		}


		switch (pChr->GetPKMode())
		{
			case PK_MODE_PEACE:
			case PK_MODE_REVENGE:
				// Cannot attack same guild
				if (pVictim->GetGuild() && pVictim->GetGuild() == pChr->GetGuild())
					break;

				if (pChr->GetPKMode() == PK_MODE_REVENGE)
				{
					if (pChr->GetAlignment() < 0 && pVictim->GetAlignment() >= 0)
					{
						pChr->SetKillerMode(true);
						return true;
					}
					else if (pChr->GetAlignment() >= 0 && pVictim->GetAlignment() < 0)
						return true;
				}
				break;

			case PK_MODE_GUILD:
				// Same implementation from PK_MODE_FREE except for attacking same guild
				if (!pChr->GetGuild() || (pVictim->GetGuild() != pChr->GetGuild()))
				{
					if (pVictim->GetAlignment() >= 0)
						pChr->SetKillerMode(true);
					else if (pChr->GetAlignment() < 0 && pVictim->GetAlignment() < 0)
						pChr->SetKillerMode(true);

					return true;
				}
				break;

			case PK_MODE_FREE:
				if (pVictim->GetAlignment() >= 0)
					pChr->SetKillerMode(true);
				else if (pChr->GetAlignment() < 0 && pVictim->GetAlignment() < 0)
					pChr->SetKillerMode(true);

				return true;
				break;
		}
	}

	CPVP kPVP(pChr->GetPlayerID(), pVictim->GetPlayerID());
	CPVP* pPVP = Find(kPVP.m_dwCRC);

	if (!pPVP || !pPVP->IsFight())
	{
		if (beKillerMode)
			pChr->SetKillerMode(true);

		return (beKillerMode);
	}

	pPVP->SetLastFightTime();
	return true;
}

CPVP * CPVPManager::Find(uint32_t dwCRC)
{
	std::map<uint32_t, CPVP *>::iterator it = m_map_pPVP.find(dwCRC);

	if (it == m_map_pPVP.end())
		return NULL;

	return it->second;
}

void CPVPManager::Delete(CPVP* pPVP)
{
	std::map<uint32_t, CPVP *>::iterator it = m_map_pPVP.find(pPVP->m_dwCRC);

	if (it == m_map_pPVP.end())
		return;

	m_map_pPVP.erase(it);
	m_map_pPVPSetByID[pPVP->m_players[0].dwPID].erase(pPVP);
	m_map_pPVPSetByID[pPVP->m_players[1].dwPID].erase(pPVP);

	M2_DELETE(pPVP);
}

void CPVPManager::SendList(LPDESC d)
{
	std::map<uint32_t, CPVP *>::iterator it = m_map_pPVP.begin();

	uint32_t dwVID = d->GetCharacter()->GetVID();

	TPacketGCPVP pack;

	pack.bHeader = HEADER_GC_PVP;

	while (it != m_map_pPVP.end())
	{
		CPVP* pPVP = (it++)->second;

		if (!pPVP->m_players[0].dwVID || !pPVP->m_players[1].dwVID)
			continue;

		// Send only if both VIDs exist.
		if (pPVP->IsFight())
		{
			pack.bMode = PVP_MODE_FIGHT;
			pack.dwVIDSrc = pPVP->m_players[0].dwVID;
			pack.dwVIDDst = pPVP->m_players[1].dwVID;
		}
		else
		{
			pack.bMode = pPVP->m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE;

			if (pPVP->m_players[0].bAgree)
			{
				pack.dwVIDSrc = pPVP->m_players[0].dwVID;
				pack.dwVIDDst = pPVP->m_players[1].dwVID;
			}
			else
			{
				pack.dwVIDSrc = pPVP->m_players[1].dwVID;
				pack.dwVIDDst = pPVP->m_players[0].dwVID;
			}
		}

		d->Packet(&pack, sizeof(pack));
		TraceLog("PVPManager::SendList {} {}", pack.dwVIDSrc, pack.dwVIDDst);

		if (pPVP->m_players[0].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(pPVP->m_players[1].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(&pack, sizeof(pack));
			}
		}
		else if (pPVP->m_players[1].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(pPVP->m_players[0].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(&pack, sizeof(pack));
			}
		}
	}
}

void CPVPManager::Process()
{
	std::map<uint32_t, CPVP *>::iterator it = m_map_pPVP.begin();

	while (it != m_map_pPVP.end())
	{
		CPVP* pvp = (it++)->second;

		if (get_dword_time() - pvp->GetLastFightTime() > 600000) // 10분 이상 싸움이 없었으면
		{
			pvp->Packet(true);
			Delete(pvp);
		}
	}
}

