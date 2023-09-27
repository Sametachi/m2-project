#include "stdafx.h"
#include "constants.h"
#include "priv_manager.h"
#include "char.h"
#include "desc_client.h"
#include "guild.h"
#include "guild_manager.h"
#include "unique_item.h"
#include "utils.h"
#include "log.h"

static const char* GetEmpireName(int32_t priv)
{
	return LC_TEXT(c_apszEmpireNames[priv]);
}

static const char* GetPrivName(int32_t priv)
{
	return LC_TEXT(c_apszPrivNames[priv]);
}

CPrivManager::CPrivManager()
{
	memset(m_aakPrivEmpireData, 0, sizeof(m_aakPrivEmpireData));
}

void CPrivManager::RequestGiveGuildPriv(uint32_t guild_id, uint8_t type, int32_t value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RequestGiveGuildPriv: wrong guild priv type({})", type);
		return;
	}

	value = MINMAX(0, value, 50);
	duration_sec = MINMAX(0, duration_sec, 60*60*24*7);

	TPacketGiveGuildPriv p;
	p.type = type;
	p.value = value;
	p.guild_id = guild_id;
	p.duration_sec = duration_sec;

	db_clientdesc->DBPacket(HEADER_GD_REQUEST_GUILD_PRIV, 0, &p, sizeof(p));
}

void CPrivManager::RequestGiveEmpirePriv(uint8_t empire, uint8_t type, int32_t value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RequestGiveEmpirePriv: wrong empire priv type({})", type);
		return;
	}

	value = MINMAX(0, value, 200);
	duration_sec = MINMAX(0, duration_sec, 60*60*24*7);

	TPacketGiveEmpirePriv p;
	p.type = type;
	p.value = value;
	p.empire = empire;
	p.duration_sec = duration_sec;

	db_clientdesc->DBPacket(HEADER_GD_REQUEST_EMPIRE_PRIV, 0, &p, sizeof(p));
}

void CPrivManager::RequestGiveCharacterPriv(uint32_t pid, uint8_t type, int32_t value)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RequestGiveCharacterPriv: wrong char priv type({})", type);
		return;
	}

	value = MINMAX(0, value, 100);

	TPacketGiveCharacterPriv p;
	p.type = type;
	p.value = value;
	p.pid = pid;

	db_clientdesc->DBPacket(HEADER_GD_REQUEST_CHARACTER_PRIV, 0, &p, sizeof(p));
}

void CPrivManager::GiveGuildPriv(uint32_t guild_id, uint8_t type, int32_t value, uint8_t bLog, time_t end_time_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: GiveGuildPriv: wrong guild priv type({})", type);
		return;
	}

	PyLog("Set Guild Priv: guild_id({}) type({}) value({}) duration_sec({})", guild_id, type, value, end_time_sec - get_global_time());

	value = MINMAX(0, value, 50);
	end_time_sec = MINMAX(0, end_time_sec, get_global_time()+60*60*24*7);

	m_aPrivGuild[type][guild_id].value = value;
	m_aPrivGuild[type][guild_id].end_time_sec = end_time_sec;

	CGuild* g = CGuildManager::GetInstance()->FindGuild(guild_id);

	if (g)
	{
		if (value)
		{
			char buf[100];
			snprintf(buf, sizeof(buf), LC_TEXT("%s of the Guild %s raised up to %d%% !"), g->GetName(), GetPrivName(type), value);
			SendNotice(buf);
		}
		else
		{
			char buf[100];
			snprintf(buf, sizeof(buf), LC_TEXT("%s of the Guild %s normal again."), g->GetName(), GetPrivName(type));
			SendNotice(buf);
		}

		if (bLog)
		{
			LogManager::GetInstance()->CharLog(0, guild_id, type, value, "GUILD_PRIV", "", "");
		}
	}
}

void CPrivManager::GiveCharacterPriv(uint32_t pid, uint8_t type, int32_t value, uint8_t bLog)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: GiveCharacterPriv: wrong char priv type({})", type);
		return;
	}

	PyLog("Set Character Priv {} {} {}", pid, type, value);

	value = MINMAX(0, value, 100);

	m_aPrivChar[type][pid] = value;

	if (bLog)
		LogManager::GetInstance()->CharLog(pid, 0, type, value, "CHARACTER_PRIV", "", "");
}

void CPrivManager::GiveEmpirePriv(uint8_t empire, uint8_t type, int32_t value, uint8_t bLog, time_t end_time_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: GiveEmpirePriv: wrong empire priv type({})", type);
		return;
	}

	PyLog("Set Empire Priv: empire({}) type({}) value({}) duration_sec({})", empire, type, value, end_time_sec-get_global_time());

	value = MINMAX(0, value, 200);
	end_time_sec = MINMAX(0, end_time_sec, get_global_time()+60*60*24*7);

	SPrivEmpireData& rkPrivEmpireData=m_aakPrivEmpireData[type][empire];
	rkPrivEmpireData.m_value = value;
	rkPrivEmpireData.m_end_time_sec = end_time_sec;

	if (value)
	{
		char buf[100];
		snprintf(buf, sizeof(buf), LC_TEXT("%s: %s has increased by %d%%!"), GetEmpireName(empire), GetPrivName(type), value);

		if (empire)
			SendNotice(buf);
		else
			SendLog(buf);
	}
	else
	{
		char buf[100];
		snprintf(buf, sizeof(buf), LC_TEXT("%s 's %s normal again."), GetEmpireName(empire), GetPrivName(type));

		if (empire)
			SendNotice(buf);
		else
			SendLog(buf);
	}

	if (bLog)
	{
		LogManager::GetInstance()->CharLog(0, empire, type, value, "EMPIRE_PRIV", "", "");
	}
}

void CPrivManager::RemoveGuildPriv(uint32_t guild_id, uint8_t type)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RemoveGuildPriv: wrong guild priv type({})", type);
		return;
	}

	m_aPrivGuild[type][guild_id].value = 0;
	m_aPrivGuild[type][guild_id].end_time_sec = 0;
}

void CPrivManager::RemoveEmpirePriv(uint8_t empire, uint8_t type)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RemoveEmpirePriv: wrong empire priv type({})", type);
		return;
	}

	SPrivEmpireData& rkPrivEmpireData=m_aakPrivEmpireData[type][empire];
	rkPrivEmpireData.m_value = 0;
	rkPrivEmpireData.m_end_time_sec = 0;
}

void CPrivManager::RemoveCharacterPriv(uint32_t pid, uint8_t type)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: RemoveCharacterPriv: wrong char priv type({})", type);
		return;
	}

	auto it = m_aPrivChar[type].find(pid);

	if (it != m_aPrivChar[type].end())
		m_aPrivChar[type].erase(it);
}

int32_t CPrivManager::GetPriv(LPCHARACTER ch, uint8_t type)
{
	int32_t val_ch = GetPrivByCharacter(ch->GetPlayerID(), type);

	if (val_ch < 0 && !ch->IsEquipUniqueItem(UNIQUE_ITEM_NO_BAD_LUCK_EFFECT))
		return val_ch;
	else
	{
		int32_t val;

		val = MAX(val_ch, GetPrivByEmpire(0, type));
		val = MAX(val, GetPrivByEmpire(ch->GetEmpire(), type));

		if (ch->GetGuild())
			val = MAX(val, GetPrivByGuild(ch->GetGuild()->GetID(), type));

		return val;
	}
}

int32_t CPrivManager::GetPrivByEmpire(uint8_t bEmpire, uint8_t type)
{
	SPrivEmpireData* pPrivEmpireData = GetPrivByEmpireEx(bEmpire, type);

	if (pPrivEmpireData)
		return pPrivEmpireData->m_value;

	return 0;
}

CPrivManager::SPrivEmpireData* CPrivManager::GetPrivByEmpireEx(uint8_t bEmpire, uint8_t type)
{
	if (type >= MAX_PRIV_NUM)
		return NULL;

	if (bEmpire >= EMPIRE_MAX_NUM)
		return NULL;

	return &m_aakPrivEmpireData[type][bEmpire];
}

int32_t CPrivManager::GetPrivByGuild(uint32_t guild_id, uint8_t type)
{
	if (type >= MAX_PRIV_NUM)
		return 0;

	auto itFind = m_aPrivGuild[ type ].find(guild_id);

	if (itFind == m_aPrivGuild[ type ].end())
		return 0;

	return itFind->second.value;
}

const CPrivManager::SPrivGuildData* CPrivManager::GetPrivByGuildEx(uint32_t dwGuildID, uint8_t byType) const
{
	if (byType >= MAX_PRIV_NUM)
		return NULL;

	auto itFind = m_aPrivGuild[ byType ].find(dwGuildID);

	if (itFind == m_aPrivGuild[ byType ].end())
		return NULL;

	return &itFind->second;
}

int32_t CPrivManager::GetPrivByCharacter(uint32_t pid, uint8_t type)
{
	if (type >= MAX_PRIV_NUM)
		return 0;

	auto it = m_aPrivChar[type].find(pid);

	if (it != m_aPrivChar[type].end())
		return it->second;

	return 0;
}

