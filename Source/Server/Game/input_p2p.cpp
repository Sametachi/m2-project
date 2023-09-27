#include "stdafx.h" 
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "castle.h"
#include "locale_service.h"
#include "questmanager.h"
#include "skill.h"
#include "threeway_war.h"


////////////////////////////////////////////////////////////////////////////////
// Input Processor
CInputP2P::CInputP2P()
{
	BindPacketInfo(&m_packetInfoGG);
}

void CInputP2P::Login(LPDESC d, const char* c_pData)
{
	P2P_MANAGER::GetInstance()->Login(d, (TPacketGGLogin *) c_pData);
}

void CInputP2P::Logout(LPDESC d, const char* c_pData)
{
	TPacketGGLogout* p = (TPacketGGLogout *) c_pData;
	P2P_MANAGER::GetInstance()->Logout(p->szName);
}

int32_t CInputP2P::Relay(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGRelay* p = (TPacketGGRelay *) c_pData;

	if (uiBytes < sizeof(TPacketGGRelay) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		SysLog("invalid packet length {}", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	PyLog("InputP2P::Relay : {} size {}", p->szName, p->lSize);

	LPCHARACTER pChr = CHARACTER_MANAGER::GetInstance()->FindPC(p->szName);

	const uint8_t* c_pbData = (const uint8_t*) (c_pData + sizeof(TPacketGGRelay));

	if (!pChr)
		return p->lSize;

	if (*c_pbData == HEADER_GC_WHISPER)
	{
		if (pChr->IsBlockMode(BLOCK_WHISPER))
		{
			return p->lSize;
		}

		char buf[1024];
		memcpy(buf, c_pbData, MIN(p->lSize, sizeof(buf)));

		TPacketGCWhisper* p2 = (TPacketGCWhisper*) buf;
		uint8_t bToEmpire = (p2->bType >> 4);
		p2->bType = p2->bType & 0x0F;

		if(p2->bType == 0x0F) 
		{
			p2->bType = WHISPER_TYPE_SYSTEM;
		} 

		pChr->GetDesc()->Packet(buf, p->lSize);
	}
	else
		pChr->GetDesc()->Packet(c_pbData, p->lSize);

	return (p->lSize);
}

int32_t CInputP2P::Notice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGNotice* p = (TPacketGGNotice *) c_pData;

	if (uiBytes < sizeof(TPacketGGNotice) + p->lSize)
		return -1;

	if (p->lSize < 0)
	{
		SysLog("invalid packet length {}", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendNotice(szBuf);
	return (p->lSize);
}

int32_t CInputP2P::MonarchNotice(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGMonarchNotice* p = (TPacketGGMonarchNotice *) c_pData;

	if (uiBytes < p->lSize + sizeof(TPacketGGMonarchNotice))
		return -1;

	if (p->lSize < 0)
	{
		SysLog("invalid packet length {}", p->lSize);
		d->SetPhase(PHASE_CLOSE);
		return -1;
	}

	char szBuf[256+1];
	strlcpy(szBuf, c_pData + sizeof(TPacketGGMonarchNotice), MIN(p->lSize + 1, sizeof(szBuf)));
	SendMonarchNotice(p->bEmpire, szBuf);
	return (p->lSize);
}

int32_t CInputP2P::MonarchTransfer(LPDESC d, const char* c_pData)
{
	TPacketMonarchGGTransfer* p = (TPacketMonarchGGTransfer*) c_pData;
	LPCHARACTER pTargetChar = CHARACTER_MANAGER::GetInstance()->FindByPID(p->dwTargetPID);

	if (pTargetChar != nullptr)
	{
		uint32_t qIndex = quest::CQuestManager::GetInstance()->GetQuestIndexByName("monarch_transfer");

		if (qIndex != 0)
		{
			pTargetChar->SetQuestFlag("monarch_transfer.x", p->x);
			pTargetChar->SetQuestFlag("monarch_transfer.y", p->y);
			quest::CQuestManager::GetInstance()->Letter(pTargetChar->GetPlayerID(), qIndex, 0);
		}
	}

	return 0;
}

int32_t CInputP2P::Guild(LPDESC d, const char* c_pData, size_t uiBytes)
{
	TPacketGGGuild* p = (TPacketGGGuild *) c_pData;
	uiBytes -= sizeof(TPacketGGGuild);
	c_pData += sizeof(TPacketGGGuild);

	CGuild * g = CGuildManager::GetInstance()->FindGuild(p->dwGuild);

	switch (p->bSubHeader)
	{
		case GUILD_SUBHEADER_GG_CHAT:
			{
				if (uiBytes < sizeof(TPacketGGGuildChat))
					return -1;

				TPacketGGGuildChat* p = (TPacketGGGuildChat *) c_pData;

				if (g)
					g->P2PChat(p->szText);

				return sizeof(TPacketGGGuildChat);
			}
			
		case GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS:
			{
				if (uiBytes < sizeof(int32_t))
					return -1;

				int32_t iBonus = *((int32_t*) c_pData);
				CGuild* pGuild = CGuildManager::GetInstance()->FindGuild(p->dwGuild);
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(iBonus);
				}
				return sizeof(int32_t);
			}
		default:
			SysLog("UNKNOWN GUILD SUB PACKET");
			break;
	}
	return 0;
}


struct FuncShout
{
	const char* m_str;
	uint8_t m_bEmpire;

	FuncShout(const char* str, uint8_t bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}   

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && d->GetEmpire() != m_bEmpire))
			return;

		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char* szText, uint8_t bEmpire)
{
	const DESC_MANAGER::DESC_SET& c_ref_set = DESC_MANAGER::GetInstance()->GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::Shout(const char* c_pData)
{
	TPacketGGShout* p = (TPacketGGShout *) c_pData;
	SendShout(p->szText, p->bEmpire);
}

void CInputP2P::Disconnect(const char* c_pData)
{
	TPacketGGDisconnect* p = (TPacketGGDisconnect *) c_pData;

	LPDESC d = DESC_MANAGER::GetInstance()->FindByLoginName(p->szLogin);

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, const char* c_pData)
{
	TPacketGGSetup* p = (TPacketGGSetup *) c_pData;
	PyLog("P2P: Setup {}:{}", d->GetHostName(), p->wPort);
	d->SetP2P(d->GetHostName(), p->wPort, p->bChannel);
}

void CInputP2P::MessengerAdd(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger *) c_pData;
	PyLog("P2P: Messenger Add {} {}", p->szAccount, p->szCompanion);
	MessengerManager::GetInstance()->__AddToList(p->szAccount, p->szCompanion);
}

void CInputP2P::MessengerRemove(const char* c_pData)
{
	TPacketGGMessenger* p = (TPacketGGMessenger *) c_pData;
	PyLog("P2P: Messenger Remove {} {}", p->szAccount, p->szCompanion);
	MessengerManager::GetInstance()->__RemoveFromList(p->szAccount, p->szCompanion);
}

void CInputP2P::FindPosition(LPDESC d, const char* c_pData)
{
	TPacketGGFindPosition* p = (TPacketGGFindPosition*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(p->dwTargetPID);
	if (ch && ch->GetMapIndex() < 10000)
	{
		TPacketGGWarpCharacter pw;
		pw.header = HEADER_GG_WARP_CHARACTER;
		pw.pid = p->dwFromPID;
		pw.x = ch->GetX();
		pw.y = ch->GetY();
		d->Packet(&pw, sizeof(pw));
	}
}

void CInputP2P::WarpCharacter(const char* c_pData)
{
	TPacketGGWarpCharacter* p = (TPacketGGWarpCharacter*) c_pData;
	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindByPID(p->pid);
	if (ch)
	{
		ch->WarpSet(p->x, p->y);
	}
}

void CInputP2P::GuildWarZoneMapIndex(const char* c_pData)
{
	TPacketGGGuildWarMapIndex* p = (TPacketGGGuildWarMapIndex*) c_pData;
	auto gm = CGuildManager::GetInstance();

	PyLog("P2P: GuildWarZoneMapIndex g1({}) vs g2({}), mapIndex({})", p->dwGuildID1, p->dwGuildID2, p->lMapIndex);

	CGuild * g1 = gm->FindGuild(p->dwGuildID1);
	CGuild * g2 = gm->FindGuild(p->dwGuildID2);

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->dwGuildID2, p->lMapIndex);
		g2->SetGuildWarMapIndex(p->dwGuildID1, p->lMapIndex);
	}
}

void CInputP2P::Transfer(const char* c_pData)
{
	TPacketGGTransfer* p = (TPacketGGTransfer *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindPC(p->szName);

	if (ch)
		ch->WarpSet(p->lX, p->lY);
}

void CInputP2P::XmasWarpSanta(const char* c_pData)
{
	TPacketGGXmasWarpSanta* p =(TPacketGGXmasWarpSanta *) c_pData;

	if (p->bChannel == g_bChannel && map_allow_find(p->lMapIndex))
	{
		int32_t	iNextSpawnDelay = 50 * 60;

		xmas::SpawnSanta(p->lMapIndex, iNextSpawnDelay);

		TPacketGGXmasWarpSantaReply pack_reply;
		pack_reply.bHeader = HEADER_GG_XMAS_WARP_SANTA_REPLY;
		pack_reply.bChannel = g_bChannel;
		P2P_MANAGER::GetInstance()->Send(&pack_reply, sizeof(pack_reply));
	}
}

void CInputP2P::XmasWarpSantaReply(const char* c_pData)
{
	TPacketGGXmasWarpSantaReply* p = (TPacketGGXmasWarpSantaReply*) c_pData;

	if (p->bChannel == g_bChannel)
	{
		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(xmas::MOB_SANTA_VNUM, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end()) {
				M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}

void CInputP2P::LoginPing(LPDESC d, const char* c_pData)
{
	TPacketGGLoginPing* p = (TPacketGGLoginPing *) c_pData;

	if (!g_pAuthMasterDesc) // If I am master, I have to broadcast
		P2P_MANAGER::GetInstance()->Send(p, sizeof(TPacketGGLoginPing), d);
}

// BLOCK_CHAT
void CInputP2P::BlockChat(const char* c_pData)
{
	TPacketGGBlockChat* p = (TPacketGGBlockChat *) c_pData;

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->FindPC(p->szName);

	if (ch)
	{
		PyLog("BLOCK CHAT apply name {} dur {}", p->szName, p->lBlockDuration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->lBlockDuration, 0, true);
	}
	else
	{
		PyLog("BLOCK CHAT fail name {} dur {}", p->szName, p->lBlockDuration);
	}
}   
// END_OF_BLOCK_CHAT
//

void CInputP2P::IamAwake(LPDESC d, const char* c_pData)
{
	std::string hostNames;
	P2P_MANAGER::GetInstance()->GetP2PHostNames(hostNames);
	PyLog("P2P Awakeness check from {}. My P2P connection number is {}. and details...\n{}", d->GetHostName(), P2P_MANAGER::GetInstance()->GetDescCount(), hostNames.c_str());
}

int32_t CInputP2P::Analyze(LPDESC d, uint8_t bHeader, const char* c_pData)
{
	if (test_server)
		PyLog("CInputP2P::Anlayze[Header {}]", bHeader);

	int32_t iExtraLen = 0;

	switch (bHeader)
	{
		case HEADER_GG_SETUP:
			Setup(d, c_pData);
			break;

		case HEADER_GG_LOGIN:
			Login(d, c_pData);
			break;

		case HEADER_GG_LOGOUT:
			Logout(d, c_pData);
			break;

		case HEADER_GG_RELAY:
			if ((iExtraLen = Relay(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_NOTICE:
			if ((iExtraLen = Notice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHUTDOWN:
			SysLog("Accept shutdown p2p command from {}.", d->GetHostName());
			Shutdown(10);
			break;

		case HEADER_GG_GUILD:
			if ((iExtraLen = Guild(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_SHOUT:
			Shout(c_pData);
			break;

		case HEADER_GG_DISCONNECT:
			Disconnect(c_pData);
			break;

		case HEADER_GG_MESSENGER_ADD:
			MessengerAdd(c_pData);
			break;

		case HEADER_GG_MESSENGER_REMOVE:
			MessengerRemove(c_pData);
			break;

		case HEADER_GG_FIND_POSITION:
			FindPosition(d, c_pData);
			break;

		case HEADER_GG_WARP_CHARACTER:
			WarpCharacter(c_pData);
			break;

		case HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(c_pData);
			break;

		case HEADER_GG_TRANSFER:
			Transfer(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA:
			XmasWarpSanta(c_pData);
			break;

		case HEADER_GG_XMAS_WARP_SANTA_REPLY:
			XmasWarpSantaReply(c_pData);
			break;

		case HEADER_GG_RELOAD_CRC_LIST:
			LoadValidCRCList();
			break;

		case HEADER_GG_LOGIN_PING:
			LoginPing(d, c_pData);
			break;

		case HEADER_GG_BLOCK_CHAT:
			BlockChat(c_pData);
			break;

		case HEADER_GG_SIEGE:
			{
				TPacketGGSiege* pSiege = (TPacketGGSiege*)c_pData;
				castle_siege(pSiege->bEmpire, pSiege->bTowerCount);
			}
			break;

		case HEADER_GG_MONARCH_NOTICE:
			if ((iExtraLen = MonarchNotice(d, c_pData, m_iBufferLeft)) < 0)
				return -1;
			break;

		case HEADER_GG_MONARCH_TRANSFER :
			MonarchTransfer(d, c_pData);
			break;

		case HEADER_GG_CHECK_AWAKENESS:
			IamAwake(d, c_pData);
			break;
	}

	return (iExtraLen);
}

