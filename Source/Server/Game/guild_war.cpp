#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "db.h"
#include "affect.h"
#include "p2p.h"
#include "war_map.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "locale_service.h"
#include "guild_manager.h"

enum
{
	GUILD_WAR_WAIT_START_DURATION = 10*60
};

// 
// Packet
//
void CGuild::GuildWarPacket(uint32_t dwOppGID, uint8_t bWarType, uint8_t bWarState)
{
	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_WAR;
	pack.size		= sizeof(pack) + sizeof(pack2);
	pack2.dwGuildSelf	= GetID();
	pack2.dwGuildOpp	= dwOppGID;
	pack2.bWarState	= bWarState;
	pack2.bType		= bWarType;

	for (auto it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;

		if (bWarState == GUILD_WAR_ON_WAR)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("[Guild] There are no experience points for hunting during a guild war."));

		LPDESC d = ch->GetDesc();

		if (d)
		{
			ch->SendGuildName(dwOppGID);

			d->BufferedPacket(&pack, sizeof(pack));
			d->Packet(&pack2, sizeof(pack2));
		}
	}
}

void CGuild::SendEnemyGuild(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_WAR;
	pack.size = sizeof(pack) + sizeof(pack2);
	pack2.dwGuildSelf = GetID();

	TPacketGCGuild p;
	p.header = HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
	p.size = sizeof(p) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int32_t);

	for (auto it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		ch->SendGuildName(it->first);

		pack2.dwGuildOpp = it->first;
		pack2.bType = it->second.type;
		pack2.bWarState = it->second.state;

		d->BufferedPacket(&pack, sizeof(pack));
		d->Packet(&pack2, sizeof(pack2));

		if (it->second.state == GUILD_WAR_ON_WAR)
		{
			int32_t lScore;

			lScore = GetWarScoreAgainstTo(pack2.dwGuildOpp);

			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&pack2.dwGuildSelf, sizeof(uint32_t));
			d->BufferedPacket(&pack2.dwGuildOpp, sizeof(uint32_t));
			d->Packet(&lScore, sizeof(int32_t));

			lScore = CGuildManager::GetInstance()->TouchGuild(pack2.dwGuildOpp)->GetWarScoreAgainstTo(pack2.dwGuildSelf);

			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&pack2.dwGuildOpp, sizeof(uint32_t));
			d->BufferedPacket(&pack2.dwGuildSelf, sizeof(uint32_t));
			d->Packet(&lScore, sizeof(int32_t));
		}
	}
}

//
// War Login
//
int32_t CGuild::GetGuildWarState(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return GUILD_WAR_NONE;

	auto it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) ? (it->second.state) : GUILD_WAR_NONE;
} 

int32_t CGuild::GetGuildWarType(uint32_t dwOppGID)
{
	auto git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return GUILD_WAR_TYPE_FIELD;

	return git->second.type;
}

uint32_t CGuild::GetGuildWarMapIndex(uint32_t dwOppGID)
{
	auto git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return 0;

	return git->second.map_index;
}

bool CGuild::CanStartWar(uint8_t bGuildWarType)
{
	if (bGuildWarType >= GUILD_WAR_TYPE_MAX_NUM)
		return false;

	if (test_server || quest::CQuestManager::GetInstance()->GetEventFlag("guild_war_test") != 0)
		return GetLadderPoint() > 0;

	return GetLadderPoint() > 0 && GetMemberCount() >= GUILD_WAR_MIN_MEMBER_COUNT;
}

bool CGuild::UnderWar(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return false;

	auto it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) && (it->second.IsWarBegin());
} 

uint32_t CGuild::UnderAnyWar(uint8_t bType)
{
	for (auto it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		if (bType < GUILD_WAR_TYPE_MAX_NUM)
			if (it->second.type != bType)
				continue;

		if (it->second.IsWarBegin())
			return it->first;
	}

	return 0;
}

void CGuild::SetWarScoreAgainstTo(uint32_t dwOppGID, int32_t iScore)
{
	uint32_t dwSelfGID = GetID();

	PyLog("GuildWarScore Set {} from {} {}", dwSelfGID, dwOppGID, iScore);
	auto it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		it->second.score = iScore;

		if (it->second.type != GUILD_WAR_TYPE_FIELD)
		{
			CGuild * gOpp = CGuildManager::GetInstance()->TouchGuild(dwOppGID);
			CWarMap* pMap = CWarMapManager::GetInstance()->Find(it->second.map_index);

			if (pMap)
				pMap->UpdateScore(dwSelfGID, iScore, dwOppGID, gOpp->GetWarScoreAgainstTo(dwSelfGID));
		}
		else
		{
			TPacketGCGuild p;

			p.header = HEADER_GC_GUILD;
			p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
			p.size = sizeof(p) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(int32_t);

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(p));

			buf.write(&dwSelfGID, sizeof(uint32_t));
			buf.write(&dwOppGID, sizeof(uint32_t));
			buf.write(&iScore, sizeof(int32_t));

			Packet(buf.read_peek(), buf.size());

			CGuild * gOpp = CGuildManager::GetInstance()->TouchGuild(dwOppGID);

			if (gOpp)
				gOpp->Packet(buf.read_peek(), buf.size());
		}
	}
}

int32_t CGuild::GetWarScoreAgainstTo(uint32_t dwOppGID)
{
	auto it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		PyLog("GuildWarScore Get {} from {} {}", GetID(), dwOppGID, it->second.score);
		return it->second.score;
	}

	PyLog("GuildWarScore Get {} from {} No data", GetID(), dwOppGID);
	return 0;
}

uint32_t CGuild::GetWarStartTime(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return 0;

	auto it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return 0;

	return it->second.war_start_time;
}

const TGuildWarInfo& GuildWar_GetTypeInfo(unsigned type)
{
	return KOR_aGuildWarInfo[type];
}

unsigned GuildWar_GetTypeMapIndex(unsigned type)
{
	return GuildWar_GetTypeInfo(type).lMapIndex;
}

bool GuildWar_IsWarMap(unsigned type)
{
	if (type == GUILD_WAR_TYPE_FIELD)
		return true;

	unsigned mapIndex = GuildWar_GetTypeMapIndex(type);

	if (SECTREE_MANAGER::GetInstance()->GetMapRegion(mapIndex))
		return true;

	return false;
}

void CGuild::NotifyGuildMaster(const char* msg)
{
	LPCHARACTER ch = GetMasterCharacter();
	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, msg);
}

extern void map_allow_log();


void CGuild::RequestDeclareWar(uint32_t dwOppGID, uint8_t type)
{
	if (dwOppGID == GetID())
	{
		PyLog("GuildWar.DeclareWar.DECLARE_WAR_SELF id({} -> {}), type({})", GetID(), dwOppGID, type);
		return;
	}

	if (type >= GUILD_WAR_TYPE_MAX_NUM)
	{
		PyLog("GuildWar.DeclareWar.UNKNOWN_WAR_TYPE id({} -> {}), type({})", GetID(), dwOppGID, type);
		return;
	}

	auto it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		if (!GuildWar_IsWarMap(type))
		{
			SysLog("GuildWar.DeclareWar.NOT_EXIST_MAP id({} -> {}), type({}), map({})", 
					GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

			map_allow_log();
			NotifyGuildMaster(LC_TEXT("[Guild] The enemy's guild leader is offline."));
			return;
		}

		TPacketGuildWar p;
		p.bType = type;
		p.bWar = GUILD_WAR_SEND_DECLARE;
		p.dwGuildFrom = GetID();
		p.dwGuildTo = dwOppGID;
		db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
		PyLog("GuildWar.DeclareWar id({} -> {}), type({})", GetID(), dwOppGID, type);
		return;
	}

	switch (it->second.state)
	{	
		case GUILD_WAR_RECV_DECLARE:
			{
				const unsigned saved_type = it->second.type;

				if (saved_type == GUILD_WAR_TYPE_FIELD)
				{
					TPacketGuildWar p;
					p.bType = saved_type;
					p.bWar = GUILD_WAR_ON_WAR;
					p.dwGuildFrom = GetID();
					p.dwGuildTo = dwOppGID;
					db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
					PyLog("GuildWar.AcceptWar id({} -> {}), type({})", GetID(), dwOppGID, saved_type);
					return;
				}

				if (!GuildWar_IsWarMap(saved_type))
				{
					SysLog("GuildWar.AcceptWar.NOT_EXIST_MAP id({} -> {}), type({}), map({})", 
							GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

					map_allow_log();
					NotifyGuildMaster(LC_TEXT("[Guild] The enemy's guild leader is offline."));
					return;
				}

				const TGuildWarInfo& guildWarInfo = GuildWar_GetTypeInfo(saved_type);

				TPacketGuildWar p;
				p.bType = saved_type;
				p.bWar = GUILD_WAR_WAIT_START;
				p.dwGuildFrom = GetID();
				p.dwGuildTo = dwOppGID;
				p.lWarPrice = guildWarInfo.iWarPrice;
				p.lInitialScore = guildWarInfo.iInitialScore;

				if (test_server) 
					p.lInitialScore /= 10;

				db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));

				PyLog("GuildWar.WaitStartSendToDB id({} vs {}), type({}), bet({}), map_index({})", 
						GetID(), dwOppGID, saved_type, guildWarInfo.iWarPrice, guildWarInfo.lMapIndex);

			}
			break;
		case GUILD_WAR_SEND_DECLARE:
			{
				NotifyGuildMaster(LC_TEXT("[Guild] This guild is already participating in a war."));
			}
			break;
		default:
			SysLog("GuildWar.DeclareWar.UNKNOWN_STATE[{}]: id({} vs {}), type({}), guild({}:{})", 
					it->second.state, GetID(), dwOppGID, type, GetName(), GetID());
			break;
	}
}

bool CGuild::DeclareWar(uint32_t dwOppGID, uint8_t type, uint8_t state)
{
	if (m_EnemyGuild.find(dwOppGID) != m_EnemyGuild.end())
		return false;

	TGuildWar gw(type);
	gw.state = state;

	m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));

	GuildWarPacket(dwOppGID, type, state);
	return true;
}

bool CGuild::CheckStartWar(uint32_t dwOppGID)
{
	auto it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return false;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return false;

	return true;
}

void CGuild::StartWar(uint32_t dwOppGID)
{
	auto it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return;

	gw.state = GUILD_WAR_ON_WAR;
	gw.war_start_time = get_global_time();

	GuildWarPacket(dwOppGID, gw.type, GUILD_WAR_ON_WAR);

	if (gw.type != GUILD_WAR_TYPE_FIELD)
		GuildWarEntryAsk(dwOppGID);
}

bool CGuild::WaitStartWar(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
	{
		PyLog("GuildWar.WaitStartWar.DECLARE_WAR_SELF id({} -> {})", GetID(), dwOppGID);
		return false;
	}

	auto it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		PyLog("GuildWar.WaitStartWar.UNKNOWN_ENEMY id({} -> {})", GetID(), dwOppGID);
		return false;
	}

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_WAIT_START)
	{
		PyLog("GuildWar.WaitStartWar.UNKNOWN_STATE id({} -> {}), state({})", GetID(), dwOppGID, gw.state);
		return false;
	}

	gw.state = GUILD_WAR_WAIT_START;

	CGuild* g = CGuildManager::GetInstance()->FindGuild(dwOppGID);
	if (!g)
	{
		PyLog("GuildWar.WaitStartWar.NOT_EXIST_GUILD id({} -> {})", GetID(), dwOppGID);
		return false;
	}

	const TGuildWarInfo& rkGuildWarInfo = GuildWar_GetTypeInfo(gw.type);
	

	if (gw.type == GUILD_WAR_TYPE_FIELD)
	{
		PyLog("GuildWar.WaitStartWar.FIELD_TYPE id({} -> {})", GetID(), dwOppGID);
		return true;
	}		

	PyLog("GuildWar.WaitStartWar.CheckWarServer id({} -> {}), type({}), map({})", 
			GetID(), dwOppGID, gw.type, rkGuildWarInfo.lMapIndex);

	if (!map_allow_find(rkGuildWarInfo.lMapIndex))
	{
		PyLog("GuildWar.WaitStartWar.SKIP_WAR_MAP id({} -> {})", GetID(), dwOppGID);
		return true;
	}


	uint32_t id1 = GetID();
	uint32_t id2 = dwOppGID;

	if (id1 > id2)
		std::swap(id1, id2);

	uint32_t lMapIndex = CWarMapManager::GetInstance()->CreateWarMap(rkGuildWarInfo, id1, id2);
	if (!lMapIndex) 
	{
		SysLog("GuildWar.WaitStartWar.CREATE_WARMAP_ERROR id({} vs {}), type({}), map({})", id1, id2, gw.type, rkGuildWarInfo.lMapIndex);
		CGuildManager::GetInstance()->RequestEndWar(GetID(), dwOppGID);
		return false;
	}

	PyLog("GuildWar.WaitStartWar.CreateMap id({} vs {}), type({}), map({}) -> map_inst({})", id1, id2, gw.type, rkGuildWarInfo.lMapIndex, lMapIndex);

	gw.map_index = lMapIndex;

	SetGuildWarMapIndex(dwOppGID, lMapIndex);
	g->SetGuildWarMapIndex(GetID(), lMapIndex);

	///////////////////////////////////////////////////////
	TPacketGGGuildWarMapIndex p;

	p.bHeader	= HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX;
	p.dwGuildID1	= id1;
	p.dwGuildID2 	= id2;
	p.lMapIndex	= lMapIndex;

	P2P_MANAGER::GetInstance()->Send(&p, sizeof(p));
	///////////////////////////////////////////////////////

	return true;
}

void CGuild::RequestRefuseWar(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	auto it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && it->second.state == GUILD_WAR_RECV_DECLARE)
	{
		TPacketGuildWar p;
		p.bWar = GUILD_WAR_REFUSE;
		p.dwGuildFrom = GetID();
		p.dwGuildTo = dwOppGID;

		db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
	}
}

void CGuild::RefuseWar(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	auto it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && (it->second.state == GUILD_WAR_SEND_DECLARE || it->second.state == GUILD_WAR_RECV_DECLARE))
	{
		uint8_t type = it->second.type;
		m_EnemyGuild.erase(dwOppGID);

		GuildWarPacket(dwOppGID, type, GUILD_WAR_END);
	}
}

void CGuild::ReserveWar(uint32_t dwOppGID, uint8_t type)
{
	if (dwOppGID == GetID())
		return;

	auto it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
	{
		TGuildWar gw(type);
		gw.state = GUILD_WAR_RESERVE;
		m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));
	}
	else
		it->second.state = GUILD_WAR_RESERVE;

	PyLog("Guild::ReserveWar {}", dwOppGID);
}

void CGuild::EndWar(uint32_t dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	auto it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		CWarMap* pMap = CWarMapManager::GetInstance()->Find(it->second.map_index);

		if (pMap)
			pMap->SetEnded();

		GuildWarPacket(dwOppGID, it->second.type, GUILD_WAR_END);
		m_EnemyGuild.erase(it);

		if (!UnderAnyWar())
		{
			for (auto it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPCHARACTER ch = *it;
				ch->RemoveAffect(GUILD_SKILL_BLOOD);
				ch->RemoveAffect(GUILD_SKILL_BLESS);
				ch->RemoveAffect(GUILD_SKILL_SEONGHWI);
				ch->RemoveAffect(GUILD_SKILL_ACCEL);
				ch->RemoveAffect(GUILD_SKILL_BUNNO);
				ch->RemoveAffect(GUILD_SKILL_JUMUN);

				ch->RemoveBadAffect();
			}
		}
	}
}

void CGuild::SetGuildWarMapIndex(uint32_t dwOppGID, int32_t lMapIndex)
{
	auto it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	it->second.map_index = lMapIndex;
	PyLog("GuildWar.SetGuildWarMapIndex id({} -> {}), map({})", GetID(), dwOppGID, lMapIndex);
}

void CGuild::GuildWarEntryAccept(uint32_t dwOppGID, LPCHARACTER ch)
{
	auto git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return;

	TGuildWar & gw(git->second);

	if (gw.type == GUILD_WAR_TYPE_FIELD)
		return;

	if (gw.state != GUILD_WAR_ON_WAR)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("The war is already over."));
		return;
	}

	if (!gw.map_index)
		return;

	PIXEL_POSITION pos;

	if (!CWarMapManager::GetInstance()->GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
		return;

	quest::PC* pPC = quest::CQuestManager::GetInstance()->GetPC(ch->GetPlayerID());
	pPC->SetFlag("war.is_war_member", 1);

	ch->SaveExitLocation();
	ch->WarpSet(pos.x, pos.y, gw.map_index);
}

void CGuild::GuildWarEntryAsk(uint32_t dwOppGID)
{
	auto git = m_EnemyGuild.find(dwOppGID);
	if (git == m_EnemyGuild.end())
	{
		SysLog("GuildWar.GuildWarEntryAsk.UNKNOWN_ENEMY({})", dwOppGID);
		return;
	}

	TGuildWar & gw(git->second);

	PyLog("GuildWar.GuildWarEntryAsk id({} vs {}), map({})", GetID(), dwOppGID, gw.map_index);
	if (!gw.map_index)
	{
		SysLog("GuildWar.GuildWarEntryAsk.NOT_EXIST_MAP id({} vs {})", GetID(), dwOppGID);
		return;
	}

	PIXEL_POSITION pos;
	if (!CWarMapManager::GetInstance()->GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
	{
		SysLog("GuildWar.GuildWarEntryAsk.START_POSITION_ERROR id({} vs {}), pos({}, {})", GetID(), dwOppGID, pos.x, pos.y);
		return;
	}

	PyLog("GuildWar.GuildWarEntryAsk.OnlineMemberCount({})", m_memberOnline.size());

	auto it = m_memberOnline.begin();

	while (it != m_memberOnline.end())
	{
		LPCHARACTER ch = *it++;

		using namespace quest;
		uint32_t questIndex=CQuestManager::GetInstance()->GetQuestIndexByName("guild_war_join");
		if (questIndex)
		{
			PyLog("GuildWar.GuildWarEntryAsk.SendLetterToMember pid({}), qid({})", ch->GetPlayerID(), questIndex);
			CQuestManager::GetInstance()->Letter(ch->GetPlayerID(), questIndex, 0);
		}
		else
		{
			SysLog("GuildWar.GuildWarEntryAsk.SendLetterToMember.QUEST_ERROR pid({}), quest_name('guild_war_join.quest')", 
					ch->GetPlayerID(), questIndex);
			break;
		}
	}
}

//
// LADDER POINT
//
void CGuild::SetLadderPoint(int32_t point) 
{ 
	if (m_data.ladder_point != point)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), LC_TEXT("[Guild] The guild has reached %d points."), point);
		for (auto it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		{
			LPCHARACTER ch = (*it);
			ch->ChatPacket(CHAT_TYPE_INFO, buf);
		}
	}
	m_data.ladder_point = point; 
}

void CGuild::ChangeLadderPoint(int32_t iChange)
{
	TPacketGuildLadderPoint p;
	p.dwGuild = GetID();
	p.lChange = iChange;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_LADDER_POINT, 0, &p, sizeof(p));
}


