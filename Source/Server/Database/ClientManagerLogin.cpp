
#include "stdafx.h"

#include "ClientManager.h"

#include "Main.h"
#include "Config.h"
#include "QID.h"
#include "Cache.h"

extern std::string g_stLocale;
extern bool CreatePlayerTableFromRes(MYSQL_RES* res, TPlayerTable* pTab);

bool CClientManager::InsertLogonAccount(const char* c_pszLogin, uint32_t dwHandle, const char* c_pszIP)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	auto it = m_map_kLogonAccount.find(szLogin);

	if (m_map_kLogonAccount.end() != it)
		return false;

	CLoginData* pLD = GetLoginDataByLogin(c_pszLogin);

	if (!pLD)
		return false;

	pLD->SetConnectedPeerHandle(dwHandle);
	pLD->SetIP(c_pszIP);

	m_map_kLogonAccount.emplace(szLogin, pLD);
	return true;
}

bool CClientManager::DeleteLogonAccount(const char* c_pszLogin, uint32_t dwHandle)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	auto it = m_map_kLogonAccount.find(szLogin);

	if (it == m_map_kLogonAccount.end())
		return false;

	CLoginData* pLD = it->second;

	if (pLD->GetConnectedPeerHandle() != dwHandle)
	{
		SysLog("{} tried to logout in other pPeer handle {}, current handle {}", szLogin, dwHandle, pLD->GetConnectedPeerHandle());
		return false;
	}

	if (pLD->IsPlay())
	{
		pLD->SetPlay(false);
	}

	if (pLD->IsDeleted())
	{
		delete pLD;
	}

	m_map_kLogonAccount.erase(it);
	return true;
}

bool CClientManager::FindLogonAccount(const char* c_pszLogin)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	if (m_map_kLogonAccount.end() == m_map_kLogonAccount.find(szLogin))
		return false;

	return true;
}

void CClientManager::QUERY_LOGIN_BY_KEY(LPPEER pPeer, uint32_t dwHandle, TPacketGDLoginByKey* p)
{
#ifdef ENABLE_LIMIT_TIME
	static int32_t s_updateCount = 0;
	static int32_t s_curTime = time(nullptr);
	if (s_updateCount > 100)
	{
		s_curTime = time(nullptr);
		s_updateCount = 0;
	}
	++s_updateCount;

	if (s_curTime >= GLOBAL_LIMIT_TIME)
	{
		FatalLog("Server life time expired.");
		exit(0);
		return;
	}
#endif

	CLoginData* pLoginData = GetLoginData(p->dwLoginKey);
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(p->szLogin, szLogin, sizeof(szLogin));

	if (!pLoginData)
	{
		PyLog("LOGIN_BY_KEY key not exist {} {}", szLogin, p->dwLoginKey);
		pPeer->EncodeReturn(HEADER_DG_LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	TAccountTable& r = pLoginData->GetAccountRef();

	if (FindLogonAccount(r.login))
	{
		PyLog("LOGIN_BY_KEY already login {} {}", r.login, p->dwLoginKey);
		TPacketDGLoginAlready ptog;
		strlcpy(ptog.szLogin, szLogin, sizeof(ptog.szLogin));
		pPeer->EncodeHeader(HEADER_DG_LOGIN_ALREADY, dwHandle, sizeof(TPacketDGLoginAlready));
		pPeer->Encode(&ptog, sizeof(TPacketDGLoginAlready));
		return;
	}

	if (strcasecmp(r.login, szLogin))
	{
		PyLog("LOGIN_BY_KEY login differ {} {} input {}", r.login, p->dwLoginKey, szLogin);
		pPeer->EncodeReturn(HEADER_DG_LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	if (memcmp(pLoginData->GetClientKey(), p->adwClientKey, sizeof(uint32_t) * 4))
	{
		const uint32_t* pdwClientKey = pLoginData->GetClientKey();

		PyLog("LOGIN_BY_KEY client key differ {} {} {} {} {}, {} {} {} {}",
				r.login,
				p->adwClientKey[0], p->adwClientKey[1], p->adwClientKey[2], p->adwClientKey[3],
				pdwClientKey[0], pdwClientKey[1], pdwClientKey[2], pdwClientKey[3]);

		pPeer->EncodeReturn(HEADER_DG_LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	TAccountTable* pTab = new TAccountTable;
	memset(pTab, 0, sizeof(TAccountTable));

	pTab->id = r.id;
	trim_and_lower(r.login, pTab->login, sizeof(pTab->login));
	strlcpy(pTab->passwd, r.passwd, sizeof(pTab->passwd));
	strlcpy(pTab->social_id, r.social_id, sizeof(pTab->social_id));
	strlcpy(pTab->status, "OK", sizeof(pTab->status));

	ClientHandleInfo* info = new ClientHandleInfo(dwHandle);
	info->pAccountTable = pTab;
	strlcpy(info->ip, p->szIP, sizeof(info->ip));

	PyLog("LOGIN_BY_KEY success {} {} {}", r.login, p->dwLoginKey, info->ip);
	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index%s WHERE id=%u", GetTablePostfix(), r.id);
	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_LOGIN_BY_KEY, pPeer->GetHandle(), info);
}

void CClientManager::RESULT_LOGIN_BY_KEY(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* info = (ClientHandleInfo*) qi->pvData;

	if (msg->uiSQLErrno != 0)
	{
		pPeer->EncodeReturn(HEADER_DG_LOGIN_NOT_EXIST, info->dwHandle);
		delete info;
		return;
	}

	char szQuery[256];

	if (msg->Get()->uiNumRows == 0)
	{
		uint32_t account_id = info->pAccountTable->id;
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index%s WHERE id=%u", GetTablePostfix(), account_id);
		std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_PLAYER));
		
		PyLog("RESULT_LOGIN_BY_KEY FAIL player_index's NULL : ID:{}", account_id);

		if (pMsg->Get()->uiNumRows == 0)
		{
			PyLog("RESULT_LOGIN_BY_KEY FAIL player_index's NULL : ID:{}", account_id);

			snprintf(szQuery, sizeof(szQuery), "INSERT INTO player_index%s (id) VALUES(%u)", GetTablePostfix(), info->pAccountTable->id);
			CDBManager::GetInstance()->ReturnQuery(szQuery, QID_PLAYER_INDEX_CREATE, pPeer->GetHandle(), info);
		}
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

	int32_t col = 0;

	for (; col < PLAYER_PER_ACCOUNT; ++col)
		str_to_number(info->pAccountTable->players[col].dwID, row[col]);

	str_to_number(info->pAccountTable->bEmpire, row[col++]);
	info->account_index = 1;

	extern std::string g_stLocale;

	snprintf(szQuery, sizeof(szQuery),
			"SELECT id, name, job, level, playtime, st, ht, dx, iq, part_main, part_hair, x, y, skill_group, change_name FROM player%s WHERE account_id=%u",
			GetTablePostfix(), info->pAccountTable->id);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_LOGIN, pPeer->GetHandle(), info);
}

void CClientManager::RESULT_PLAYER_INDEX_CREATE(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* info = (ClientHandleInfo*) qi->pvData;

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index%s WHERE id=%u", GetTablePostfix(), 
			info->pAccountTable->id);
	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_LOGIN_BY_KEY, pPeer->GetHandle(), info);
}

TAccountTable * CreateAccountTableFromRes(MYSQL_RES* res)
{
	char input_pwd[PASSWD_MAX_LEN + 1];
	MYSQL_ROW row = nullptr;
	uint32_t col;

	row = mysql_fetch_row(res);
	col = 0;

	TAccountTable* pTab = new TAccountTable;
	memset(pTab, 0, sizeof(TAccountTable));
	
	// Refer only to the first column (for JOIN query)
	strlcpy(input_pwd, row[col++], sizeof(input_pwd));
	str_to_number(pTab->id, row[col++]);
	strlcpy(pTab->login, row[col++], sizeof(pTab->login));
	strlcpy(pTab->passwd, row[col++], sizeof(pTab->passwd));
	strlcpy(pTab->social_id, row[col++], sizeof(pTab->social_id));
	str_to_number(pTab->bEmpire, row[col++]);

	for (auto& rPlayer : pTab->players)
		str_to_number(rPlayer.dwID, row[col++]);

	strlcpy(pTab->status, row[col++], sizeof(pTab->status));

	if (strcmp(pTab->passwd, input_pwd))
	{
		delete pTab;
		return nullptr;
	}

	return pTab;
}

void CreateAccountPlayerDataFromRes(MYSQL_RES* pRes, TAccountTable* pTab)
{
	if (!pRes)
		return;

	for (uint32_t i = 0; i < mysql_num_rows(pRes); ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes);
		int32_t col = 0;

		uint32_t player_id = 0;
		!row[col++] ? 0 : str_to_number(player_id, row[col - 1]);

		if (!player_id)
			continue;

		int32_t j;

		for (j = 0; j < PLAYER_PER_ACCOUNT; ++j)
		{
			if (pTab->players[j].dwID == player_id)
			{
				CPlayerTableCache* pc = CClientManager::GetInstance()->GetPlayerCache(player_id);
				TPlayerTable* pt = pc ? pc->Get(false) : nullptr;

				if (pt)
				{
					strlcpy(pTab->players[j].szName, pt->name, sizeof(pTab->players[j].szName));

					pTab->players[j].byJob				= pt->job;
					pTab->players[j].byLevel			= pt->level;
					pTab->players[j].dwPlayMinutes		= pt->playtime;
					pTab->players[j].byST				= pt->st;
					pTab->players[j].byHT				= pt->ht;
					pTab->players[j].byDX				= pt->dx;
					pTab->players[j].byIQ				= pt->iq;
					pTab->players[j].wMainPart			= pt->parts[PART_MAIN];
					pTab->players[j].wHairPart			= pt->parts[PART_HAIR];
					pTab->players[j].x					= pt->x;
					pTab->players[j].y					= pt->y;
					pTab->players[j].skill_group		= pt->skill_group;
					pTab->players[j].bChangeName		= 0;
				}
				else
				{
					if (!row[col++])
						*pTab->players[j].szName = '\0';
					else
						strlcpy(pTab->players[j].szName, row[col - 1], sizeof(pTab->players[j].szName));

					pTab->players[j].byJob			= 0;
					pTab->players[j].byLevel		= 0;
					pTab->players[j].dwPlayMinutes	= 0;
					pTab->players[j].byST			= 0;
					pTab->players[j].byHT			= 0;
					pTab->players[j].byDX			= 0;
					pTab->players[j].byIQ			= 0;
					pTab->players[j].wMainPart		= 0;
					pTab->players[j].wHairPart		= 0;
					pTab->players[j].x				= 0;
					pTab->players[j].y				= 0;
					pTab->players[j].skill_group	= 0;
					pTab->players[j].bChangeName	= 0;

					str_to_number(pTab->players[j].byJob, row[col++]);
					str_to_number(pTab->players[j].byLevel, row[col++]);
					str_to_number(pTab->players[j].dwPlayMinutes, row[col++]);
					str_to_number(pTab->players[j].byST, row[col++]);
					str_to_number(pTab->players[j].byHT, row[col++]);
					str_to_number(pTab->players[j].byDX, row[col++]);
					str_to_number(pTab->players[j].byIQ, row[col++]);
					str_to_number(pTab->players[j].wMainPart, row[col++]);
					str_to_number(pTab->players[j].wHairPart, row[col++]);
					str_to_number(pTab->players[j].x, row[col++]);
					str_to_number(pTab->players[j].y, row[col++]);
					str_to_number(pTab->players[j].skill_group, row[col++]);
					str_to_number(pTab->players[j].bChangeName, row[col++]);
				}

				PyLog("{} {} {} hair {}",
						pTab->players[j].szName, pTab->players[j].x, pTab->players[j].y, pTab->players[j].wHairPart);
				break;
			}
		}
	}
}

void CClientManager::RESULT_LOGIN(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* info = (ClientHandleInfo*) qi->pvData;

	if (info->account_index == 0)
	{
		if (msg->Get()->uiNumRows == 0)
		{
			PyLog("RESULT_LOGIN: no account");
			pPeer->EncodeHeader(HEADER_DG_LOGIN_NOT_EXIST, info->dwHandle, 0);
			delete info;
			return;
		}

		info->pAccountTable = CreateAccountTableFromRes(msg->Get()->pSQLResult);

		if (!info->pAccountTable)
		{
			PyLog("RESULT_LOGIN: no account : WRONG_PASSWD");
			pPeer->EncodeReturn(HEADER_DG_LOGIN_WRONG_PASSWD, info->dwHandle);
			delete info;
		}
		else
		{
			++info->account_index;

			char queryStr[512];
			
			snprintf(queryStr, sizeof(queryStr),
					"SELECT id, name, job, level, playtime, st, ht, dx, iq, part_main, part_hair, x, y, skill_group, change_name FROM player%s WHERE account_id=%u",
					GetTablePostfix(), info->pAccountTable->id);

			CDBManager::GetInstance()->ReturnQuery(queryStr, QID_LOGIN, pPeer->GetHandle(), info);
		}
		return;
	}
	else
	{
		if (!info->pAccountTable)
		{
			pPeer->EncodeReturn(HEADER_DG_LOGIN_WRONG_PASSWD, info->dwHandle);
			delete info;
			return;
		}

		// If another connection has already logged in.. You should send it as already connected.
		if (!InsertLogonAccount(info->pAccountTable->login, pPeer->GetHandle(), info->ip))
		{
			PyLog("RESULT_LOGIN: already log-on {}", info->pAccountTable->login);

			TPacketDGLoginAlready p;
			strlcpy(p.szLogin, info->pAccountTable->login, sizeof(p.szLogin));

			pPeer->EncodeHeader(HEADER_DG_LOGIN_ALREADY, info->dwHandle, sizeof(TPacketDGLoginAlready));
			pPeer->Encode(&p, sizeof(p));
		}
		else
		{
			PyLog("RESULT_LOGIN: login success {} rows: {}", info->pAccountTable->login, msg->Get()->uiNumRows);

			if (msg->Get()->uiNumRows > 0)
				CreateAccountPlayerDataFromRes(msg->Get()->pSQLResult, info->pAccountTable);

			CLoginData* p = GetLoginDataByLogin(info->pAccountTable->login);
			memcpy(&p->GetAccountRef(), info->pAccountTable, sizeof(TAccountTable));

			pPeer->EncodeHeader(HEADER_DG_LOGIN_SUCCESS, info->dwHandle, sizeof(TAccountTable));
			pPeer->Encode(info->pAccountTable, sizeof(TAccountTable));

		}

		delete info->pAccountTable;
		info->pAccountTable = nullptr;
		delete info;
	}
}

void CClientManager::QUERY_LOGOUT(LPPEER pPeer, uint32_t dwHandle,const char* data)
{
	TLogoutPacket* packet = (TLogoutPacket*)data;

	if (!*packet->login)
		return;

	CLoginData* pLoginData = GetLoginDataByLogin(packet->login);

	if (!pLoginData)
		return;

	int32_t pid[PLAYER_PER_ACCOUNT];

	for (int32_t n = 0; n < PLAYER_PER_ACCOUNT; ++n)
	{
		if (pLoginData->GetAccountRef().players[n].dwID == 0)
		{
			TraceLog("LOGOUT {} {}", packet->login, pLoginData->GetAccountRef().players[n].dwID);
			continue;
		}
		
		pid[n] = pLoginData->GetAccountRef().players[n].dwID;

		TraceLog("LOGOUT InsertLogoutPlayer {} {}", packet->login, pid[n]);

		InsertLogoutPlayer(pid[n]);
	}
	
	if (DeleteLogonAccount(packet->login, pPeer->GetHandle()))
	{
		TraceLog("LOGOUT {} ", packet->login);
	}
}

void CClientManager::QUERY_CHANGE_NAME(LPPEER pPeer, uint32_t dwHandle, TPacketGDChangeName* p)
{
	char queryStr[256];

	snprintf(queryStr, sizeof(queryStr),
		"SELECT COUNT(*) as count FROM player%s WHERE name='%s' AND id <> %u", GetTablePostfix(), p->name, p->pid);

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(queryStr, SQL_PLAYER));

	if (pMsg->Get()->uiNumRows)
	{
		if (!pMsg->Get()->pSQLResult)
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		if (*row[0] != '0')
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_ALREADY, dwHandle, 0);
			return;
		}
	}   
	else
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
		return;
	}

	snprintf(queryStr, sizeof(queryStr),
			"UPDATE player%s SET name='%s',change_name=0 WHERE id=%u", GetTablePostfix(), p->name, p->pid);

	std::unique_ptr<SQLMsg> pMsg0(CDBManager::GetInstance()->DirectQuery(queryStr, SQL_PLAYER));

	TPacketDGChangeName pdg;
	pPeer->EncodeHeader(HEADER_DG_CHANGE_NAME, dwHandle, sizeof(TPacketDGChangeName));
	pdg.pid = p->pid;
	strlcpy(pdg.name, p->name, sizeof(pdg.name));
	pPeer->Encode(&pdg, sizeof(TPacketDGChangeName));
}

