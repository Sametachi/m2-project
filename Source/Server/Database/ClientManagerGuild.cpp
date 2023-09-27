#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "GuildManager.h"


void CClientManager::GuildCreate(LPPEER pPeer, uint32_t dwGuildID)
{
	TraceLog("GuildCreate {}", dwGuildID);
	ForwardPacket(HEADER_DG_GUILD_LOAD, &dwGuildID, sizeof(uint32_t));

	CGuildManager::GetInstance()->Load(dwGuildID);
}

void CClientManager::GuildChangeGrade(LPPEER pPeer, TPacketGuild* p)
{
	TraceLog("GuildChangeGrade {} {}", p->dwGuild, p->dwInfo);
	ForwardPacket(HEADER_DG_GUILD_CHANGE_GRADE, p, sizeof(TPacketGuild));
}

void CClientManager::GuildAddMember(LPPEER pPeer, TPacketGDGuildAddMember* p)
{
	CGuildManager::GetInstance()->TouchGuild(p->dwGuild);
	TraceLog("GuildAddMember {} {}", p->dwGuild, p->dwPID);

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), 
			"INSERT INTO guild_member%s VALUES(%u, %u, %d, 0, 0)",
			GetTablePostfix(), p->dwPID, p->dwGuild, p->bGrade);

	std::unique_ptr<SQLMsg> pmsg_insert(CDBManager::GetInstance()->DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), 
			"SELECT pid, grade, is_general, offer, level, job, name FROM guild_member%s, player%s WHERE guild_id = %u and pid = id and pid = %u", GetTablePostfix(), GetTablePostfix(), p->dwGuild, p->dwPID);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows == 0)
	{
		SysLog("Query failed when getting guild member data {}", pmsg->stQuery.c_str());
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

	if (!row[0] || !row[1])
		return;

	TPacketDGGuildMember dg;

	dg.dwGuild = p->dwGuild;
	str_to_number(dg.dwPID, row[0]);
	str_to_number(dg.bGrade, row[1]);
	str_to_number(dg.isGeneral, row[2]);
	str_to_number(dg.dwOffer, row[3]);
	str_to_number(dg.bLevel, row[4]);
	str_to_number(dg.bJob, row[5]);
	strlcpy(dg.szName, row[6], sizeof(dg.szName));

	ForwardPacket(HEADER_DG_GUILD_ADD_MEMBER, &dg, sizeof(TPacketDGGuildMember));
}

void CClientManager::GuildRemoveMember(LPPEER pPeer, TPacketGuild* p)
{
	PyLog("GuildRemoveMember {} {}", p->dwGuild, p->dwInfo);

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_member%s WHERE pid=%u and guild_id=%u", GetTablePostfix(), p->dwInfo, p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO quest%s (dwPID, szName, szState, lValue) VALUES(%u, 'guild_manage', 'withdraw_time', %u)", GetTablePostfix(), p->dwInfo, (uint32_t) GetCurrentTime());
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	ForwardPacket(HEADER_DG_GUILD_REMOVE_MEMBER, p, sizeof(TPacketGuild));
}

void CClientManager::GuildSkillUpdate(LPPEER pPeer, TPacketGuildSkillUpdate* p)
{
	TraceLog("GuildSkillUpdate {}", p->amount);
	ForwardPacket(HEADER_DG_GUILD_SKILL_UPDATE, p, sizeof(TPacketGuildSkillUpdate));
}

void CClientManager::GuildExpUpdate(LPPEER pPeer, TPacketGuildExpUpdate* p)
{
	TraceLog("GuildExpUpdate {}", p->amount);
	ForwardPacket(HEADER_DG_GUILD_EXP_UPDATE, p, sizeof(TPacketGuildExpUpdate), 0, pPeer);
}

void CClientManager::GuildChangeMemberData(LPPEER pPeer, TPacketGuildChangeMemberData* p)
{
	TraceLog("GuildChangeMemberData {} {} {} {}", p->pid, p->offer, p->level, p->grade);
	ForwardPacket(HEADER_DG_GUILD_CHANGE_MEMBER_DATA, p, sizeof(TPacketGuildChangeMemberData), 0, pPeer);
}

void CClientManager::GuildDisband(LPPEER pPeer, TPacketGuild* p)
{
	PyLog("GuildDisband {}", p->dwGuild);

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild%s WHERE id=%u", GetTablePostfix(), p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_grade%s WHERE guild_id=%u", GetTablePostfix(), p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO quest%s (dwPID, szName, szState, lValue) SELECT pid, 'guild_manage', 'withdraw_time', %u FROM guild_member%s WHERE guild_id = %u", GetTablePostfix(), (uint32_t) GetCurrentTime(), GetTablePostfix(), p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_member%s WHERE guild_id=%u", GetTablePostfix(), p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);
	
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_comment%s WHERE guild_id=%u", GetTablePostfix(), p->dwGuild);
	CDBManager::GetInstance()->AsyncQuery(szQuery);

	ForwardPacket(HEADER_DG_GUILD_DISBAND, p, sizeof(TPacketGuild));
}

const char* __GetWarType(int32_t n)
{
	switch (n)
	{
		case 0 :
			return "King";
			
		case 1 :
			return "Champion";
			
		case 2 :
			return "Guardian";
			
		default :
			return "None";
	}
}

void CClientManager::GuildWar(LPPEER pPeer, TPacketGuildWar* p)
{
	switch (p->bWar)
	{
		case GUILD_WAR_SEND_DECLARE:
			PyLog("GuildWar: GUILD_WAR_SEND_DECLARE type({}) guild({} - {})",  __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->AddDeclare(p->bType, p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_REFUSE:
			PyLog("GuildWar: GUILD_WAR_REFUSE type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->RemoveDeclare(p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_WAIT_START:
			PyLog("GuildWar: GUILD_WAR_WAIT_START type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
		case GUILD_WAR_RESERVE:
			if (p->bWar != GUILD_WAR_WAIT_START)
			{
				PyLog("GuildWar: GUILD_WAR_RESERVE type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			}
			CGuildManager::GetInstance()->RemoveDeclare(p->dwGuildFrom, p->dwGuildTo);

			if (!CGuildManager::GetInstance()->ReserveWar(p))
				p->bWar = GUILD_WAR_CANCEL;
			else
				p->bWar = GUILD_WAR_RESERVE;

			break;

		case GUILD_WAR_ON_WAR:
			PyLog("GuildWar: GUILD_WAR_ON_WAR type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->RemoveDeclare(p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->StartWar(p->bType, p->dwGuildFrom, p->dwGuildTo);
			break;

		case GUILD_WAR_OVER:
			PyLog("GuildWar: GUILD_WAR_OVER type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->RecvWarOver(p->dwGuildFrom, p->dwGuildTo, p->bType, p->lWarPrice);
			break;

		case GUILD_WAR_END:
			PyLog("GuildWar: GUILD_WAR_END type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->RecvWarEnd(p->dwGuildFrom, p->dwGuildTo);
			return; // NOTE: Since RecvWarEnd sends packets, it is not broadcast separately.

		case GUILD_WAR_CANCEL :
			PyLog("GuildWar: GUILD_WAR_CANCEL type({}) guild({} - {})", __GetWarType(p->bType), p->dwGuildFrom, p->dwGuildTo);
			CGuildManager::GetInstance()->CancelWar(p->dwGuildFrom, p->dwGuildTo);
			break;
	}

	ForwardPacket(HEADER_DG_GUILD_WAR, p, sizeof(TPacketGuildWar));
}

void CClientManager::GuildWarScore(LPPEER pPeer, TPacketGuildWarScore* p)
{
	CGuildManager::GetInstance()->UpdateScore(p->dwGuildGainPoint, p->dwGuildOpponent, p->lScore, p->lBetScore);
}

void CClientManager::GuildChangeLadderPoint(TPacketGuildLadderPoint* p)
{
	TraceLog("GuildChangeLadderPoint Recv {} {}", p->dwGuild, p->lChange);
	CGuildManager::GetInstance()->ChangeLadderPoint(p->dwGuild, p->lChange);
}

void CClientManager::GuildUseSkill(TPacketGuildUseSkill* p)
{
	TraceLog("GuildUseSkill Recv {} {}", p->dwGuild, p->dwSkillVnum);
	CGuildManager::GetInstance()->UseSkill(p->dwGuild, p->dwSkillVnum, p->dwCooltime);
	SendGuildSkillUsable(p->dwGuild, p->dwSkillVnum, false);
}

void CClientManager::SendGuildSkillUsable(uint32_t guild_id, uint32_t dwSkillVnum, bool bUsable)
{
	TraceLog("SendGuildSkillUsable Send {} {} {}", guild_id, dwSkillVnum, bUsable?"true":"false");

	TPacketGuildSkillUsableChange p;

	p.dwGuild = guild_id;
	p.dwSkillVnum = dwSkillVnum;
	p.bUsable = bUsable;

	ForwardPacket(HEADER_DG_GUILD_SKILL_USABLE_CHANGE, &p, sizeof(TPacketGuildSkillUsableChange));
}

void CClientManager::GuildChangeMaster(TPacketChangeGuildMaster* p)
{
	if (CGuildManager::GetInstance()->ChangeMaster(p->dwGuildID, p->idFrom, p->idTo))
	{
		TPacketChangeGuildMaster packet;
		packet.dwGuildID = p->dwGuildID;
		packet.idFrom = 0;
		packet.idTo = 0;

		ForwardPacket(HEADER_DG_ACK_CHANGE_GUILD_MASTER, &packet, sizeof(packet));
	}
}

