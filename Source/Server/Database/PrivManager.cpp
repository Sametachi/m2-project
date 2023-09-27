#include "stdafx.h"
#include "PrivManager.h"
#include "ClientManager.h"

const int32_t PRIV_DURATION = 60*60*12;
const int32_t CHARACTER_GOOD_PRIV_DURATION = 2*60*60;
const int32_t CHARACTER_BAD_PRIV_DURATION = 60*60;

CPrivManager::CPrivManager()
{
	for (int32_t type = 0; type < MAX_PRIV_NUM; ++type)
	{
		for (int32_t empire = 0; empire < EMPIRE_MAX_NUM; ++empire)
			m_aaPrivEmpire[type][empire] = nullptr;
	}
}

CPrivManager::~CPrivManager()
{
}

void CPrivManager::Update()
{
	time_t now = CClientManager::GetInstance()->GetCurrentTime();

	while (!m_pqPrivGuild.empty() && m_pqPrivGuild.top().first <= now)
	{
		TPrivGuildData* p = m_pqPrivGuild.top().second;
		m_pqPrivGuild.pop();

		if (p->value != 0 && !p->bRemoved)
		{

			auto it = m_aPrivGuild[p->type].find(p->guild_id);

			// If a bonus is set in a guild redundantly, the value of the map is updated (modified)
			// When the pointer of TPrivGuildData is the same, it is actually deleted and cast to the game servers.
			if (it != m_aPrivGuild[p->type].end() && it->second == p) {
				m_aPrivGuild[p->type].erase(it);
				SendChangeGuildPriv(p->guild_id, p->type, 0, 0);
			}
		}

		delete p;
	}

	while (!m_pqPrivEmpire.empty() && m_pqPrivEmpire.top().first <= now)
	{
		TPrivEmpireData* p = (m_pqPrivEmpire.top().second);
		m_pqPrivEmpire.pop();

		if (p->value != 0 && !p->bRemoved)
		{
			SendChangeEmpirePriv(p->empire, p->type, 0, 0);
			m_aaPrivEmpire[p->type][p->empire] = nullptr;
		}

		delete p;
	}

	while (!m_pqPrivChar.empty() && m_pqPrivChar.top().first <= now)
	{
		TPrivCharData* p = (m_pqPrivChar.top().second);
		m_pqPrivChar.pop();

		if (!p->bRemoved)
		{
			// TODO: Send packet
			SendChangeCharPriv(p->pid, p->type, 0);
			auto it = m_aPrivChar[p->type].find(p->pid);
			if (it != m_aPrivChar[p->type].end())
				m_aPrivChar[p->type].erase(it);
		}
		delete p;
	}
}

void CPrivManager::AddCharPriv(uint32_t pid, uint8_t type, int32_t value)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: AddCharPriv: wrong char priv type({}) recved", type);
		return;
	}

	auto it = m_aPrivChar[type].find(pid);

	if (it != m_aPrivChar[type].end())
		return;

	if (!value)
		return;

	time_t now = CClientManager::GetInstance()->GetCurrentTime();
	TPrivCharData* p = new TPrivCharData(type, value, pid);

	int32_t iDuration = CHARACTER_BAD_PRIV_DURATION;

	if (value > 0)
		iDuration = CHARACTER_GOOD_PRIV_DURATION;

	m_pqPrivChar.push(std::make_pair(now+iDuration, p));
	m_aPrivChar[type].emplace(pid, p);

	// TODO send packet
	TraceLog("AddCharPriv {} {} {}", pid, type, value);
	SendChangeCharPriv(pid, type, value);
}

void CPrivManager::AddGuildPriv(uint32_t guild_id, uint8_t type, int32_t value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: AddGuildPriv: wrong guild priv type({}) recved", type);
		return;
	}

	auto it = m_aPrivGuild[type].find(guild_id);

	time_t now = CClientManager::GetInstance()->GetCurrentTime();
	time_t end = now + duration_sec;
	TPrivGuildData* p = new TPrivGuildData(type, value, guild_id, end);
	m_pqPrivGuild.push(std::make_pair(end, p));

	if (it != m_aPrivGuild[type].end())
		it->second = p;
	else
		m_aPrivGuild[type].insert(std::make_pair(guild_id, p));

	SendChangeGuildPriv(guild_id, type, value, end);

	TraceLog("Guild Priv guild({}) type({}) value({}) duration_sec({})", guild_id, type, value, duration_sec);
}

void CPrivManager::AddEmpirePriv(uint8_t empire, uint8_t type, int32_t value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		SysLog("PRIV_MANAGER: AddEmpirePriv: wrong empire priv type({}) recved", type);
		return;
	}

	if (duration_sec < 0)
		duration_sec = 0;

	time_t now = CClientManager::GetInstance()->GetCurrentTime();
	time_t end = now+duration_sec;

	// Override previous settings
	{
		if (m_aaPrivEmpire[type][empire])
			m_aaPrivEmpire[type][empire]->bRemoved = true;
	}

	TPrivEmpireData* p = new TPrivEmpireData(type, value, empire, end);
	m_pqPrivEmpire.push(std::make_pair(end, p));
	m_aaPrivEmpire[type][empire] = p;

	SendChangeEmpirePriv(empire, type, value, end);

	TraceLog("Empire Priv empire({}) type({}) value({}) duration_sec({})", empire, type, value, duration_sec);
}

struct FSendChangeGuildPriv
{
	FSendChangeGuildPriv(uint32_t guild_id, uint8_t type, int32_t value, time_t end_time_sec)
	{
		p.guild_id = guild_id;
		p.type = type;
		p.value = value;
		p.bLog = 1;
		p.end_time_sec = end_time_sec;
	}

	void operator() (LPPEER pPeer)
	{
		pPeer->EncodeHeader(HEADER_DG_CHANGE_GUILD_PRIV, 0, sizeof(TPacketDGChangeGuildPriv));
		pPeer->Encode(&p, sizeof(TPacketDGChangeGuildPriv));
		p.bLog = 0;
	}

	TPacketDGChangeGuildPriv p;
};

struct FSendChangeEmpirePriv
{
	FSendChangeEmpirePriv(uint8_t empire, uint8_t type, int32_t value, time_t end_time_sec)
	{
		p.empire = empire;
		p.type = type;
		p.value = value;
		p.bLog = 1;
		p.end_time_sec = end_time_sec;
	}

	void operator ()(LPPEER pPeer)
	{
		pPeer->EncodeHeader(HEADER_DG_CHANGE_EMPIRE_PRIV, 0, sizeof(TPacketDGChangeEmpirePriv));
		pPeer->Encode(&p, sizeof(TPacketDGChangeEmpirePriv));
		p.bLog = 0;
	}

	TPacketDGChangeEmpirePriv p;
};

struct FSendChangeCharPriv
{
	FSendChangeCharPriv(uint32_t pid, uint8_t type, int32_t value)
	{
		p.pid = pid;
		p.type = type;
		p.value = value;
		p.bLog = 1;
	}
	void operator()(LPPEER pPeer)
	{
		pPeer->EncodeHeader(HEADER_DG_CHANGE_CHARACTER_PRIV, 0, sizeof(TPacketDGChangeCharacterPriv));
		pPeer->Encode(&p, sizeof(TPacketDGChangeCharacterPriv));
		p.bLog = 0;
	}
	TPacketDGChangeCharacterPriv p;
};

void CPrivManager::SendChangeGuildPriv(uint32_t guild_id, uint8_t type, int32_t value, time_t end_time_sec)
{
	CClientManager::GetInstance()->for_each_peer(FSendChangeGuildPriv(guild_id, type, value, end_time_sec));
}

void CPrivManager::SendChangeEmpirePriv(uint8_t empire, uint8_t type, int32_t value, time_t end_time_sec)
{
	CClientManager::GetInstance()->for_each_peer(FSendChangeEmpirePriv(empire, type, value, end_time_sec));
}

void CPrivManager::SendChangeCharPriv(uint32_t pid, uint8_t type, int32_t value)
{
	CClientManager::GetInstance()->for_each_peer(FSendChangeCharPriv(pid, type, value));
}

void CPrivManager::SendPrivOnSetup(LPPEER pPeer)
{
	for (int32_t i = 1; i < MAX_PRIV_NUM; ++i)
	{
		for (int32_t e = 0; e < EMPIRE_MAX_NUM; ++e)
		{
			TPrivEmpireData* pPrivEmpireData = m_aaPrivEmpire[i][e];
			if (pPrivEmpireData)
			{
				FSendChangeEmpirePriv(e, i, pPrivEmpireData->value, pPrivEmpireData->end_time_sec)(pPeer);
			}
		}

		for (auto it = m_aPrivGuild[i].begin(); it != m_aPrivGuild[i].end();++it)
		{
			FSendChangeGuildPriv(it->first, i, it->second->value, it->second->end_time_sec)(pPeer);
		}
		
		for (auto it = m_aPrivChar[i].begin(); it != m_aPrivChar[i].end(); ++it)
		{
			FSendChangeCharPriv(it->first, i, it->second->value)(pPeer);
		}
	}
}
