
#include "stdafx.h"

#include "ClientManager.h"

#include "Main.h"
#include "QID.h"
#include "HB.h"
#include "Cache.h"

extern bool g_bHotBackup;

extern std::string g_stLocale;

bool CreateItemTableFromRes(MYSQL_RES* res, std::vector<TPlayerItem>* pVec, uint32_t dwPID)
{
	if (!res)
	{
		pVec->clear();
		return true;
	}

	int32_t rows;

	if ((rows = mysql_num_rows(res)) <= 0)
	{
		pVec->clear();
		return true;
	}

	pVec->resize(rows);

	for (int32_t i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TPlayerItem& item = pVec->at(i);

		int32_t cur = 0;

		// Check all SELECT syntax on item table before change this function!
		str_to_number(item.id, row[cur++]);
		str_to_number(item.window, row[cur++]);
		str_to_number(item.pos, row[cur++]);
		str_to_number(item.count, row[cur++]);
		str_to_number(item.vnum, row[cur++]);
		str_to_number(item.alSockets[0], row[cur++]);
		str_to_number(item.alSockets[1], row[cur++]);
		str_to_number(item.alSockets[2], row[cur++]);

		for (auto& rAttr : item.aAttr)
		{
			str_to_number(rAttr.bType, row[cur++]);
			str_to_number(rAttr.sValue, row[cur++]);
		}

		item.owner		= dwPID;
	}

	return true;
}

size_t CreatePlayerSaveQuery(char* pszQuery, size_t querySize, TPlayerTable* pTab)
{
	size_t queryLen;

	queryLen = snprintf(pszQuery, querySize,
			"UPDATE player%s SET "
			"job = %d, "
			"voice = %d, "
			"dir = %d, "
			"x = %d, "
			"y = %d, "
			"z = %d, "
			"map_index = %d, "
			"exit_x = %ld, "
			"exit_y = %ld, "
			"exit_map_index = %ld, "
			"hp = %d, "
			"mp = %d, "
			"stamina = %d, "
			"random_hp = %d, "
			"random_sp = %d, "
			"playtime = %d, "
			"level = %d, "
			"level_step = %d, "
			"st = %d, "
			"ht = %d, "
			"dx = %d, "
			"iq = %d, "
			"gold = %d, "
			"exp = %u, "
			"stat_point = %d, "
			"skill_point = %d, "
			"sub_skill_point = %d, "
			"stat_reset_count = %d, "
			"ip = '%s', "
			"part_main = %d, "
			"part_hair = %d, "
			"last_play = NOW(), "
			"skill_group = %d, "
			"alignment = %ld, "
			"horse_level = %d, "
			"horse_riding = %d, "
			"horse_hp = %d, "
			"horse_hp_droptime = %u, "
			"horse_stamina = %d, "
			"horse_skill_point = %d, "
			,
		GetTablePostfix(),
		pTab->job,
		pTab->voice,
		pTab->dir,
		pTab->x,
		pTab->y,
		pTab->z,
		pTab->lMapIndex,
		pTab->lExitX,
		pTab->lExitY,
		pTab->lExitMapIndex,
		pTab->hp,
		pTab->sp,
		pTab->stamina,
		pTab->sRandomHP,
		pTab->sRandomSP,
		pTab->playtime,
		pTab->level,
		pTab->level_step,
		pTab->st,
		pTab->ht,
		pTab->dx,
		pTab->iq,
		pTab->gold,
		pTab->exp,
		pTab->stat_point,
		pTab->skill_point,
		pTab->sub_skill_point,
		pTab->stat_reset_count,
		pTab->ip,
		pTab->parts[PART_MAIN],
		pTab->parts[PART_HAIR],
		pTab->skill_group,
		pTab->lAlignment,
		pTab->horse.bLevel,
		pTab->horse.bRiding,
		pTab->horse.sHealth,
		pTab->horse.dwHorseHealthDropTime,
		pTab->horse.sStamina,
		pTab->horse_skill_point);

	static char text[8192 + 1];

	CDBManager::GetInstance()->EscapeString(text, pTab->skills, sizeof(pTab->skills));
	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, "skill_level = '%s', ", text);

	CDBManager::GetInstance()->EscapeString(text, pTab->quickslot, sizeof(pTab->quickslot));
	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, "quickslot = '%s' ", text);

	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, " WHERE id=%d", pTab->id);
	return queryLen;
}

CPlayerTableCache* CClientManager::GetPlayerCache(uint32_t id)
{
	auto it = m_map_playerCache.find(id);

	if (it == m_map_playerCache.end())
		return nullptr;

	TPlayerTable* pTable = it->second->Get(false);
	pTable->logoff_interval = GetCurrentTime() - it->second->GetLastUpdateTime();

	return it->second.get();
}

void CClientManager::PutPlayerCache(TPlayerTable* pNew)
{
	CPlayerTableCache* pPlayerCache;

	pPlayerCache = GetPlayerCache(pNew->id);

	if (!pPlayerCache)
	{
		std::unique_ptr<CPlayerTableCache> upPlayerCache = std::make_unique<CPlayerTableCache>();
		pPlayerCache = upPlayerCache.get();

		m_map_playerCache.emplace(pNew->id, std::move(upPlayerCache));
	}

	if (g_bHotBackup)
		CPlayerHB::GetInstance()->Put(pNew->id);

	pPlayerCache->Put(pNew);
}

void CClientManager::QUERY_PLAYER_LOAD(LPPEER pPeer, uint32_t dwHandle, TPlayerLoadPacket* packet)
{
	CPlayerTableCache* pPlayerCache;
	TPlayerTable* pTab;

	CLoginData* pLoginData = GetLoginDataByAID(packet->account_id);

	if (pLoginData)
	{
		for (const auto& rPlayer : pLoginData->GetAccountRef().players)
		{
			if (rPlayer.dwID != 0)
				DeleteLogoutPlayer(rPlayer.dwID);
		}
	}

	//----------------------------------
	// 1. User information exists in DBCache: In DBCache
	// 2. User information is not in DBCache: in DB
	//----------------------------------

	//----------------------------------
	// 1. User information exists in DBCache: In DBCache
	//----------------------------------
	if ((pPlayerCache = GetPlayerCache(packet->player_id)))
	{
		CLoginData* pLD = GetLoginDataByAID(packet->account_id);

		if (!pLD || pLD->IsPlay())
		{
			TraceLog("PLAYER_LOAD_ERROR: LoginData IsPlay {}",pLD ? pLD->IsPlay() : 0);
			pPeer->EncodeHeader(HEADER_DG_PLAYER_LOAD_FAILED, dwHandle, 0); 
			return;
		}

		pTab = pPlayerCache->Get();

		pLD->SetPlay(true);
		memcpy(pTab->aiPremiumTimes, pLD->GetPremiumPtr(), sizeof(pTab->aiPremiumTimes));

		pPeer->EncodeHeader(HEADER_DG_PLAYER_LOAD_SUCCESS, dwHandle, sizeof(TPlayerTable));
		pPeer->Encode(pTab, sizeof(TPlayerTable));

		if (packet->player_id != pLD->GetLastPlayerID())
		{
			TPacketNeedLoginLogInfo logInfo;
			logInfo.dwPlayerID = packet->player_id;

			pLD->SetLastPlayerID(packet->player_id);

			pPeer->EncodeHeader(HEADER_DG_NEED_LOGIN_LOG, dwHandle, sizeof(TPacketNeedLoginLogInfo));
			pPeer->Encode(&logInfo, sizeof(TPacketNeedLoginLogInfo));
		}

		char szQuery[1024] = { 0, };

		TItemCacheSet* pSet = GetItemCacheSet(pTab->id);

		TraceLog("[PLAYER_LOAD] ID {} pid {} gold {} ", pTab->name, pTab->id, pTab->gold);

		//----------------------------------
		// 1) Item exists in DBCache: Retrieved from DBCache
		// 2) Item is not in DBCache: Retrieved from DB
		//----------------------------------
		
		//----------------------------------
		// 1) Item exists in DBCache: Retrieved from DBCache
		//----------------------------------
		if (pSet)
		{
			static std::vector<TPlayerItem> s_items;
			s_items.resize(pSet->size());

			uint32_t dwCount = 0;
			auto it = pSet->begin();

			while (it != pSet->end())
			{
				CItemCache* pItemCache = *it++;
				TPlayerItem* pItemTab = pItemCache->Get();

				if (pItemTab->vnum) // If there is no vnum, it is a deleted item.
					memcpy(&s_items[dwCount++], pItemTab, sizeof(TPlayerItem));
			}

			TraceLog("ITEM_CACHE: HIT! {} count: {}", pTab->name, dwCount);

			pPeer->EncodeHeader(HEADER_DG_ITEM_LOAD, dwHandle, sizeof(uint32_t) + sizeof(TPlayerItem) * dwCount);
			pPeer->EncodeDWORD(dwCount);

			if (dwCount)
				pPeer->Encode(&s_items[0], sizeof(TPlayerItem) * dwCount);

			snprintf(szQuery, sizeof(szQuery),
					"SELECT dwPID,szName,szState,lValue FROM quest%s WHERE dwPID=%d AND lValue<>0",
					GetTablePostfix(), pTab->id);
			
			CDBManager::GetInstance()->ReturnQuery(szQuery, QID_QUEST, pPeer->GetHandle(), new ClientHandleInfo(dwHandle,0,packet->account_id));

			snprintf(szQuery, sizeof(szQuery),
					"SELECT dwPID,bType,bApplyOn,lApplyValue,dwFlag,lDuration,lSPCost FROM affect%s WHERE dwPID=%d",
					GetTablePostfix(), pTab->id);
			CDBManager::GetInstance()->ReturnQuery(szQuery, QID_AFFECT, pPeer->GetHandle(), new ClientHandleInfo(dwHandle));
		}
		//----------------------------------
		// 2) Item is not in DBCache: Retrieved from DB
		//----------------------------------
		else
		{
			snprintf(szQuery, sizeof(szQuery), 
					"SELECT id,window+0,pos,count,vnum,socket0,socket1,socket2,attrtype0,attrvalue0,attrtype1,attrvalue1,attrtype2,attrvalue2,attrtype3,attrvalue3,attrtype4,attrvalue4,attrtype5,attrvalue5,attrtype6,attrvalue6 "
					"FROM item%s WHERE owner_id=%d AND (window < %d or window = %d)",
					GetTablePostfix(), pTab->id, SAFEBOX, DRAGON_SOUL_INVENTORY);

			CDBManager::GetInstance()->ReturnQuery(szQuery,
					QID_ITEM,
					pPeer->GetHandle(),
					new ClientHandleInfo(dwHandle, pTab->id));
			snprintf(szQuery, sizeof(szQuery), 
					"SELECT dwPID, szName, szState, lValue FROM quest%s WHERE dwPID=%d",
					GetTablePostfix(), pTab->id);

			CDBManager::GetInstance()->ReturnQuery(szQuery,
					QID_QUEST,
					pPeer->GetHandle(),
					new ClientHandleInfo(dwHandle, pTab->id));
			snprintf(szQuery, sizeof(szQuery), 
					"SELECT dwPID, bType, bApplyOn, lApplyValue, dwFlag, lDuration, lSPCost FROM affect%s WHERE dwPID=%d",
					GetTablePostfix(), pTab->id);

			CDBManager::GetInstance()->ReturnQuery(szQuery,
					QID_AFFECT,
					pPeer->GetHandle(),
					new ClientHandleInfo(dwHandle, pTab->id));
		}
	}
	//----------------------------------
	// 2. User information is not in DBCache: in DB
	//----------------------------------
	else
	{
		TraceLog("[PLAYER_LOAD] Load from PlayerDB pid[{}]", packet->player_id);

		char queryStr[1024];
		
		snprintf(queryStr, sizeof(queryStr),
				"SELECT "
				"id,name,job,voice,dir,x,y,z,map_index,exit_x,exit_y,exit_map_index,hp,mp,stamina,random_hp,random_sp,playtime,"
				"gold,level,level_step,st,ht,dx,iq,exp,"
				"stat_point,skill_point,sub_skill_point,stat_reset_count,part_base,part_hair,"
				"skill_level,quickslot,skill_group,alignment,horse_level,horse_riding,horse_hp,horse_hp_droptime,horse_stamina,"
				"UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(last_play),horse_skill_point FROM player%s WHERE id=%d",
				GetTablePostfix(), packet->player_id);

		ClientHandleInfo* pInfo = new ClientHandleInfo(dwHandle, packet->player_id);
		pInfo->account_id = packet->account_id;
		CDBManager::GetInstance()->ReturnQuery(queryStr, QID_PLAYER, pPeer->GetHandle(), pInfo);

		snprintf(queryStr, sizeof(queryStr),
				"SELECT id,window+0,pos,count,vnum,socket0,socket1,socket2,attrtype0,attrvalue0,attrtype1,attrvalue1,attrtype2,attrvalue2,attrtype3,attrvalue3,attrtype4,attrvalue4,attrtype5,attrvalue5,attrtype6,attrvalue6 "
				"FROM item%s WHERE owner_id=%d AND (window < %d or window = %d)",
				GetTablePostfix(), packet->player_id, SAFEBOX, DRAGON_SOUL_INVENTORY);
		CDBManager::GetInstance()->ReturnQuery(queryStr, QID_ITEM, pPeer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id));

		snprintf(queryStr, sizeof(queryStr),
				"SELECT dwPID,szName,szState,lValue FROM quest%s WHERE dwPID=%d",
				GetTablePostfix(), packet->player_id);
		CDBManager::GetInstance()->ReturnQuery(queryStr, QID_QUEST, pPeer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id,packet->account_id));

		snprintf(queryStr, sizeof(queryStr),
				"SELECT dwPID,bType,bApplyOn,lApplyValue,dwFlag,lDuration,lSPCost FROM affect%s WHERE dwPID=%d",
				GetTablePostfix(), packet->player_id);
		CDBManager::GetInstance()->ReturnQuery(queryStr, QID_AFFECT, pPeer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id));
	}
	
	
}

char* CClientManager::GetCommand(char* str)
{
	char command[20] = "";
	char* tok;

	if (str[0] == '[')
	{
		tok = strtok(str,"]");			
		strcat(command,&tok[1]);		
	}

	return command;
}

bool CreatePlayerTableFromRes(MYSQL_RES* res, TPlayerTable* pTab)
{
	if (mysql_num_rows(res) == 0)
		return false;

	memset(pTab, 0, sizeof(TPlayerTable));

	MYSQL_ROW row = mysql_fetch_row(res);

	int32_t	col = 0;

	str_to_number(pTab->id, row[col++]);
	strlcpy(pTab->name, row[col++], sizeof(pTab->name));
	str_to_number(pTab->job, row[col++]);
	str_to_number(pTab->voice, row[col++]);
	str_to_number(pTab->dir, row[col++]);
	str_to_number(pTab->x, row[col++]);
	str_to_number(pTab->y, row[col++]);
	str_to_number(pTab->z, row[col++]);
	str_to_number(pTab->lMapIndex, row[col++]);
	str_to_number(pTab->lExitX, row[col++]);
	str_to_number(pTab->lExitY, row[col++]);
	str_to_number(pTab->lExitMapIndex,  row[col++]);
	str_to_number(pTab->hp, row[col++]);
	str_to_number(pTab->sp, row[col++]);
	str_to_number(pTab->stamina, row[col++]);
	str_to_number(pTab->sRandomHP, row[col++]);
	str_to_number(pTab->sRandomSP, row[col++]);
	str_to_number(pTab->playtime, row[col++]);
	str_to_number(pTab->gold, row[col++]);
	str_to_number(pTab->level, row[col++]);
	str_to_number(pTab->level_step, row[col++]);
	str_to_number(pTab->st, row[col++]);
	str_to_number(pTab->ht, row[col++]);
	str_to_number(pTab->dx, row[col++]);
	str_to_number(pTab->iq, row[col++]);
	str_to_number(pTab->exp, row[col++]);
	str_to_number(pTab->stat_point, row[col++]);
	str_to_number(pTab->skill_point, row[col++]);
	str_to_number(pTab->sub_skill_point, row[col++]);
	str_to_number(pTab->stat_reset_count, row[col++]);
	str_to_number(pTab->part_base, row[col++]);
	str_to_number(pTab->parts[PART_HAIR], row[col++]);

	if (row[col])
		memcpy(pTab->skills, row[col], sizeof(pTab->skills));
	else
		memset(&pTab->skills, 0, sizeof(pTab->skills));

	col++;

	if (row[col])
		memcpy(pTab->quickslot, row[col], sizeof(pTab->quickslot));
	else
		memset(pTab->quickslot, 0, sizeof(pTab->quickslot));

	col++;

	str_to_number(pTab->skill_group, row[col++]);
	str_to_number(pTab->lAlignment, row[col++]);

	str_to_number(pTab->horse.bLevel, row[col++]);
	str_to_number(pTab->horse.bRiding, row[col++]);
	str_to_number(pTab->horse.sHealth, row[col++]);
	str_to_number(pTab->horse.dwHorseHealthDropTime, row[col++]);
	str_to_number(pTab->horse.sStamina, row[col++]);
	str_to_number(pTab->logoff_interval, row[col++]);
	str_to_number(pTab->horse_skill_point, row[col++]);

	{
		pTab->skills[123].bLevel = 0; // SKILL_CREATE

		if (pTab->level > 9)
		{
			int32_t max_point = pTab->level - 9;

			int32_t skill_point = 
				MIN(20, pTab->skills[121].bLevel) +	// SKILL_LEADERSHIP
				MIN(20, pTab->skills[124].bLevel) +	// SKILL_MINING
				MIN(10, pTab->skills[131].bLevel) +	// SKILL_HORSE_SUMMON
				MIN(20, pTab->skills[141].bLevel) +	// SKILL_ADD_HP
				MIN(20, pTab->skills[142].bLevel);		// SKILL_RESIST_PENETRATE

			pTab->sub_skill_point = max_point - skill_point;
		}
		else
			pTab->sub_skill_point = 0;
	}

	return true;
}

void CClientManager::RESULT_COMPOSITE_PLAYER(LPPEER pPeer, SQLMsg* pMsg, uint32_t dwQID)
{
	CQueryInfo* qi = (CQueryInfo*) pMsg->pvUserData;
	std::unique_ptr<ClientHandleInfo> info((ClientHandleInfo*) qi->pvData);
	
	MYSQL_RES* pSQLResult = pMsg->Get()->pSQLResult;
	if (!pSQLResult)
	{
		SysLog("null MYSQL_RES QID {}", dwQID);
		return;
	}

	switch (dwQID)
	{
		case QID_PLAYER:
			TraceLog("QID_PLAYER {} {}", info->dwHandle, info->player_id);
			RESULT_PLAYER_LOAD(pPeer, pSQLResult, info.get());

			break;

		case QID_ITEM:
			TraceLog("QID_ITEM {}", info->dwHandle);
			RESULT_ITEM_LOAD(pPeer, pSQLResult, info->dwHandle, info->player_id);
			break;

		case QID_QUEST:
			{
				TraceLog("QID_QUEST {}", info->dwHandle);
				RESULT_QUEST_LOAD(pPeer, pSQLResult, info->dwHandle, info->player_id);

				ClientHandleInfo*  temp1 = info.get();
				if (!temp1)
					break;
				
				CLoginData* pLoginData1 = GetLoginDataByAID(temp1->account_id);	//				
				
				if (!pLoginData1->GetAccountRef().login)
					break;

				if (!pLoginData1)
					break;
			}
			break;

		case QID_AFFECT:
			TraceLog("QID_AFFECT {}", info->dwHandle);
			RESULT_AFFECT_LOAD(pPeer, pSQLResult, info->dwHandle);
			break;
	}
	
}

void CClientManager::RESULT_PLAYER_LOAD(LPPEER pPeer, MYSQL_RES* pRes, ClientHandleInfo* pInfo)
{
	TPlayerTable tab;

	if (!CreatePlayerTableFromRes(pRes, &tab))
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_LOAD_FAILED, pInfo->dwHandle, 0); 
		return;
	}

	CLoginData* pLD = GetLoginDataByAID(pInfo->account_id);
	
	if (!pLD || pLD->IsPlay())
	{
		TraceLog("PLAYER_LOAD_ERROR: LoginData IsPlay {}", pLD ? pLD->IsPlay() : 0);
		pPeer->EncodeHeader(HEADER_DG_PLAYER_LOAD_FAILED, pInfo->dwHandle, 0); 
		return;
	}

	pLD->SetPlay(true);
	memcpy(tab.aiPremiumTimes, pLD->GetPremiumPtr(), sizeof(tab.aiPremiumTimes));

	pPeer->EncodeHeader(HEADER_DG_PLAYER_LOAD_SUCCESS, pInfo->dwHandle, sizeof(TPlayerTable));
	pPeer->Encode(&tab, sizeof(TPlayerTable));

	if (tab.id != pLD->GetLastPlayerID())
	{
		TPacketNeedLoginLogInfo logInfo;
		logInfo.dwPlayerID = tab.id;

		pLD->SetLastPlayerID(tab.id);

		pPeer->EncodeHeader(HEADER_DG_NEED_LOGIN_LOG, pInfo->dwHandle, sizeof(TPacketNeedLoginLogInfo));
		pPeer->Encode(&logInfo, sizeof(TPacketNeedLoginLogInfo));
	}
}

void CClientManager::RESULT_ITEM_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle, uint32_t dwPID)
{
	static std::vector<TPlayerItem> s_items;

	CreateItemTableFromRes(pRes, &s_items, dwPID);
	uint32_t dwCount = s_items.size();

	pPeer->EncodeHeader(HEADER_DG_ITEM_LOAD, dwHandle, sizeof(uint32_t) + sizeof(TPlayerItem) * dwCount);
	pPeer->EncodeDWORD(dwCount);

	CreateItemCacheSet(dwPID);

	TraceLog("ITEM_LOAD: count {} pid {}", dwCount, dwPID);

	if (dwCount)
	{
		pPeer->Encode(&s_items[0], sizeof(TPlayerItem) * dwCount);

		for (uint32_t i = 0; i < dwCount; ++i)
			PutItemCache(&s_items[i], true); // Since there is no need to save the loaded one, put true in the argument bSkipQuery.
	}
}

void CClientManager::RESULT_AFFECT_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle)
{
	int32_t iNumRows;

	if ((iNumRows = mysql_num_rows(pRes)) == 0)
		return;

	static std::vector<TPacketAffectElement> s_elements;
	s_elements.resize(iNumRows);

	uint32_t dwPID = 0;

	MYSQL_ROW row;

	for (int32_t i = 0; i < iNumRows; ++i)
	{
		TPacketAffectElement& r = s_elements[i];
		row = mysql_fetch_row(pRes);

		if (dwPID == 0)
			str_to_number(dwPID, row[0]);

		str_to_number(r.dwType, row[1]);
		str_to_number(r.bApplyOn, row[2]);
		str_to_number(r.lApplyValue, row[3]);
		str_to_number(r.dwFlag, row[4]);
		str_to_number(r.lDuration, row[5]);
		str_to_number(r.lSPCost, row[6]);
	}

	TraceLog("AFFECT_LOAD: count {} PID {}", s_elements.size(), dwPID);

	uint32_t dwCount = s_elements.size();

	pPeer->EncodeHeader(HEADER_DG_AFFECT_LOAD, dwHandle, sizeof(uint32_t) + sizeof(uint32_t) + sizeof(TPacketAffectElement) * dwCount);
	pPeer->Encode(&dwPID, sizeof(uint32_t));
	pPeer->Encode(&dwCount, sizeof(uint32_t));
	pPeer->Encode(&s_elements[0], sizeof(TPacketAffectElement) * dwCount);
}

void CClientManager::RESULT_QUEST_LOAD(LPPEER pPeer, MYSQL_RES* pRes, uint32_t dwHandle, uint32_t pid)
{
	int32_t iNumRows;

	if ((iNumRows = mysql_num_rows(pRes)) == 0)
	{
		uint32_t dwCount = 0; 
		pPeer->EncodeHeader(HEADER_DG_QUEST_LOAD, dwHandle, sizeof(uint32_t));
		pPeer->Encode(&dwCount, sizeof(uint32_t));
		return;
	}

	static std::vector<TQuestTable> s_table;
	s_table.resize(iNumRows);

	MYSQL_ROW row;

	for (int32_t i = 0; i < iNumRows; ++i)
	{
		TQuestTable& r = s_table[i];

		row = mysql_fetch_row(pRes);

		str_to_number(r.dwPID, row[0]);
		strlcpy(r.szName, row[1], sizeof(r.szName));
		strlcpy(r.szState, row[2], sizeof(r.szState));
		str_to_number(r.lValue, row[3]);
	}

	TraceLog("QUEST_LOAD: count {} PID {}", s_table.size(), s_table[0].dwPID);

	uint32_t dwCount = s_table.size();

	pPeer->EncodeHeader(HEADER_DG_QUEST_LOAD, dwHandle, sizeof(uint32_t) + sizeof(TQuestTable) * dwCount);
	pPeer->Encode(&dwCount, sizeof(uint32_t));
	pPeer->Encode(&s_table[0], sizeof(TQuestTable) * dwCount);
}

void CClientManager::QUERY_PLAYER_SAVE(LPPEER pPeer, uint32_t dwHandle, TPlayerTable* pTab)
{
	TraceLog("PLAYER_SAVE: {}", pTab->name);

	PutPlayerCache(pTab);
}

typedef std::map<uint32_t, time_t> time_by_id_map_t;
static time_by_id_map_t s_createTimeByAccountID;

void CClientManager::__QUERY_PLAYER_CREATE(LPPEER pPeer, uint32_t dwHandle, TPlayerCreatePacket* packet)
{
	char    queryStr[QUERY_MAX_LEN];
	int32_t		queryLen;
	int32_t		player_id;

	// You cannot create a character within X seconds for an account.
	auto it = s_createTimeByAccountID.find(packet->account_id);

	if (it != s_createTimeByAccountID.end())
	{
		time_t curtime = time(nullptr);

		if (curtime - it->second < 30)
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
			return;
		}
	}

	queryLen = snprintf(queryStr, sizeof(queryStr), 
			"SELECT pid%u FROM player_index%s WHERE id=%d", packet->account_index + 1, GetTablePostfix(), packet->account_id);

	std::unique_ptr<SQLMsg> pMsg0(CDBManager::GetInstance()->DirectQuery(queryStr));

	if (pMsg0->Get()->uiNumRows != 0)
	{
		if (!pMsg0->Get()->pSQLResult)
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg0->Get()->pSQLResult);

		uint32_t dwPID = 0; str_to_number(dwPID, row[0]);
		if (row[0] && dwPID > 0)
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_ALREADY, dwHandle, 0);
			TraceLog("ALREADY EXIST AccountChrIdx {} ID {}", packet->account_index, dwPID);
			return;
		}
	}
	else
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
		return;
	}

	snprintf(queryStr, sizeof(queryStr), 
			"SELECT COUNT(*) as count FROM player%s WHERE name='%s'", GetTablePostfix(), packet->player_table.name);

	std::unique_ptr<SQLMsg> pMsg1(CDBManager::GetInstance()->DirectQuery(queryStr));

	if (pMsg1->Get()->uiNumRows)
	{
		if (!pMsg1->Get()->pSQLResult)
		{
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg1->Get()->pSQLResult);

		if (*row[0] != '0')
		{
			TraceLog("ALREADY EXIST name {}, row[0] {} query {}", packet->player_table.name, row[0], queryStr);
			pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_ALREADY, dwHandle, 0);
			return;
		}
	}
	else
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
		return;
	}

	queryLen = snprintf(queryStr, sizeof(queryStr), 
			"INSERT INTO player%s "
			"(id, account_id, name, level, st, ht, dx, iq, "
			"job, voice, dir, x, y, z, "
			"hp, mp, random_hp, random_sp, stat_point, stamina, part_base, part_main, part_hair, gold, playtime, "
			"skill_level, quickslot) "
			"VALUES(0, %u, '%s', %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, %d, 0, %d, 0, ",
			GetTablePostfix(),
			packet->account_id, packet->player_table.name, packet->player_table.level, packet->player_table.st, packet->player_table.ht, packet->player_table.dx, packet->player_table.iq,
			packet->player_table.job, packet->player_table.voice, packet->player_table.dir, packet->player_table.x, packet->player_table.y, packet->player_table.z,
			packet->player_table.hp, packet->player_table.sp, packet->player_table.sRandomHP, packet->player_table.sRandomSP, packet->player_table.stat_point, packet->player_table.stamina, packet->player_table.part_base, packet->player_table.part_base, packet->player_table.gold);

	TraceLog("PlayerCreate accountid {} name {} level {} gold {}, st {} ht {} job {}",
			packet->account_id, 
			packet->player_table.name, 
			packet->player_table.level, 
			packet->player_table.gold, 
			packet->player_table.st, 
			packet->player_table.ht, 
			packet->player_table.job);

	static char text[4096 + 1];

	CDBManager::GetInstance()->EscapeString(text, packet->player_table.skills, sizeof(packet->player_table.skills));
	queryLen += snprintf(queryStr + queryLen, sizeof(queryStr) - queryLen, "'%s', ", text);
	TraceLog("Create_Player queryLen[{}] TEXT[{}]", queryLen, text);

	CDBManager::GetInstance()->EscapeString(text, packet->player_table.quickslot, sizeof(packet->player_table.quickslot));
	queryLen += snprintf(queryStr + queryLen, sizeof(queryStr) - queryLen, "'%s')", text);

	std::unique_ptr<SQLMsg> pMsg2(CDBManager::GetInstance()->DirectQuery(queryStr));
	TraceLog("Create_Player queryLen[{}] TEXT[{}]", queryLen, text);

	if (pMsg2->Get()->uiAffectedRows <= 0)
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_ALREADY, dwHandle, 0);
		TraceLog("ALREADY EXISTS query: {} AffectedRows {}", queryStr, pMsg2->Get()->uiAffectedRows);
		return;
	}

	player_id = pMsg2->Get()->uiInsertID;

	snprintf(queryStr, sizeof(queryStr), "UPDATE player_index%s SET pid%d=%d WHERE id=%d", 
			GetTablePostfix(), packet->account_index + 1, player_id, packet->account_id);
	std::unique_ptr<SQLMsg> pMsg3(CDBManager::GetInstance()->DirectQuery(queryStr));

	if (pMsg3->Get()->uiAffectedRows <= 0)
	{
		SysLog("QUERY_ERROR: {}", queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM player%s WHERE id=%d", GetTablePostfix(), player_id);
		CDBManager::GetInstance()->DirectQuery(queryStr);

		pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_FAILED, dwHandle, 0);
		return;
	}

	TPacketDGCreateSuccess pack;
	memset(&pack, 0, sizeof(pack));

	pack.bAccountCharacterIndex = packet->account_index;

	pack.player.dwID			= player_id;
	strlcpy(pack.player.szName, packet->player_table.name, sizeof(pack.player.szName));
	pack.player.byJob			= packet->player_table.job;
	pack.player.byLevel			= 1;
	pack.player.dwPlayMinutes	= 0;
	pack.player.byST			= packet->player_table.st;
	pack.player.byHT			= packet->player_table.ht;
	pack.player.byDX 			= packet->player_table.dx;
	pack.player.byIQ			= packet->player_table.iq;
	pack.player.wMainPart		= packet->player_table.part_base;
	pack.player.x				= packet->player_table.x;
	pack.player.y				= packet->player_table.y;

	pPeer->EncodeHeader(HEADER_DG_PLAYER_CREATE_SUCCESS, dwHandle, sizeof(TPacketDGCreateSuccess));
	pPeer->Encode(&pack, sizeof(TPacketDGCreateSuccess));

	TraceLog("7 name {} job {}", pack.player.szName, pack.player.byJob);

	s_createTimeByAccountID[packet->account_id] = time(nullptr);
}

void CClientManager::__QUERY_PLAYER_DELETE(LPPEER pPeer, uint32_t dwHandle, TPlayerDeletePacket* packet)
{
	if (!packet->login[0] || !packet->player_id || packet->account_index >= PLAYER_PER_ACCOUNT)
		return;

	CLoginData* ld = GetLoginDataByLogin(packet->login);

	if (!ld)
	{
		pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, dwHandle, 1);
		pPeer->EncodeBYTE(packet->account_index);
		return;
	}

	TAccountTable& r = ld->GetAccountRef();

	if (!IsChinaEventServer())
	{
		if (strlen(r.social_id) < 7 || strncmp(packet->private_code, r.social_id + strlen(r.social_id) - 7, 7))
		{
			TraceLog("PLAYER_DELETE FAILED len({})", strlen(r.social_id));
			pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, dwHandle, 1);
			pPeer->EncodeBYTE(packet->account_index);
			return;
		}

		CPlayerTableCache* pPlayerCache = GetPlayerCache(packet->player_id);
		if (pPlayerCache)
		{
			TPlayerTable* pTab = pPlayerCache->Get();

			if (pTab->level >= m_iPlayerDeleteLevelLimit)
			{
				TraceLog("PLAYER_DELETE FAILED LEVEL {} >= DELETE LIMIT {}", pTab->level, m_iPlayerDeleteLevelLimit);
				pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, dwHandle, 1);
				pPeer->EncodeBYTE(packet->account_index);
				return;
			}

			if (pTab->level < m_iPlayerDeleteLevelLimitLower)
			{
				TraceLog("PLAYER_DELETE FAILED LEVEL {} < DELETE LIMIT {}", pTab->level, m_iPlayerDeleteLevelLimitLower);
				pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, dwHandle, 1);
				pPeer->EncodeBYTE(packet->account_index);
				return;
			}
		}
	}

	char szQuery[128];
	snprintf(szQuery, sizeof(szQuery), "SELECT p.id, p.level, p.name FROM player_index%s AS i, player%s AS p WHERE pid%u=%u AND pid%u=p.id", 
			GetTablePostfix(), GetTablePostfix(), packet->account_index + 1, packet->player_id, packet->account_index + 1);

	ClientHandleInfo* pi = new ClientHandleInfo(dwHandle, packet->player_id);
	pi->account_index = packet->account_index;

	TraceLog("PLAYER_DELETE TRY: {} {} pid{}", packet->login, packet->player_id, packet->account_index + 1);
	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_PLAYER_DELETE, pPeer->GetHandle(), pi);
}

void CClientManager::__RESULT_PLAYER_DELETE(LPPEER pPeer, SQLMsg* msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* pi = (ClientHandleInfo*) qi->pvData;

	if (msg->Get() && msg->Get()->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		uint32_t dwPID = 0;
		str_to_number(dwPID, row[0]);

		int32_t deletedLevelLimit = 0;
		str_to_number(deletedLevelLimit, row[1]);

		char szName[64];
		strlcpy(szName, row[2], sizeof(szName));

		if (deletedLevelLimit >= m_iPlayerDeleteLevelLimit && !IsChinaEventServer())
		{
			PyLog("PLAYER_DELETE FAILED LEVEL {} >= DELETE LIMIT {}", deletedLevelLimit, m_iPlayerDeleteLevelLimit);
			pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, pi->dwHandle, 1);
			pPeer->EncodeBYTE(pi->account_index);
			return;
		}

		if (deletedLevelLimit < m_iPlayerDeleteLevelLimitLower)
		{
			PyLog("PLAYER_DELETE FAILED LEVEL {} < DELETE LIMIT {}", deletedLevelLimit, m_iPlayerDeleteLevelLimitLower);
			pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, pi->dwHandle, 1);
			pPeer->EncodeBYTE(pi->account_index);
			return;
		}

		char queryStr[128];

		snprintf(queryStr, sizeof(queryStr), "INSERT INTO player%s_deleted SELECT * FROM player%s WHERE id=%d", 
				GetTablePostfix(), GetTablePostfix(), pi->player_id);
		std::unique_ptr<SQLMsg> pIns(CDBManager::GetInstance()->DirectQuery(queryStr));

		if (pIns->Get()->uiAffectedRows == 0 || pIns->Get()->uiAffectedRows == (uint32_t)-1)
		{
			SysLog("PLAYER_DELETE FAILED {} CANNOT INSERT TO player{}_deleted", dwPID, GetTablePostfix());

			pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, pi->dwHandle, 1);
			pPeer->EncodeBYTE(pi->account_index);
			return;
		}

		PyLog("PLAYER_DELETE SUCCESS {}", dwPID);

		char account_index_string[16];

		snprintf(account_index_string, sizeof(account_index_string), "player_id%d", m_iPlayerIDStart + pi->account_index);

		// Delete the player table from the cache.
		CPlayerTableCache* pPlayerCache = GetPlayerCache(pi->player_id);

		if (pPlayerCache)
			m_map_playerCache.erase(pi->player_id);

		// Delete the items from the cache.
		TItemCacheSet* pSet = GetItemCacheSet(pi->player_id);

		if (pSet)
		{
			auto it = pSet->begin();

			while (it != pSet->end())
			{
				CItemCache* pItemCache = *it++;
				DeleteItemCache(pItemCache->Get()->id);
			}

			pSet->clear();

			m_map_pItemCacheSetPtr.erase(pi->player_id);
		}

		snprintf(queryStr, sizeof(queryStr), "UPDATE player_index%s SET pid%u=0 WHERE pid%u=%d", 
				GetTablePostfix(), 
				pi->account_index + 1, 
				pi->account_index + 1, 
				pi->player_id);

		std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(queryStr));

		if (pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1)
		{
			SysLog("PLAYER_DELETE FAIL WHEN UPDATE account table");
			pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, pi->dwHandle, 1);
			pPeer->EncodeBYTE(pi->account_index);
			return;
		}

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM player%s WHERE id=%d", GetTablePostfix(), pi->player_id);
		std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(queryStr));

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM item%s WHERE owner_id=%d AND (window < %d or window = %d)", GetTablePostfix(), pi->player_id, SAFEBOX, DRAGON_SOUL_INVENTORY);
		std::unique_ptr<SQLMsg> pSQLMsg2(CDBManager::GetInstance()->DirectQuery(queryStr));

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM quest%s WHERE dwPID=%d", GetTablePostfix(), pi->player_id);
		CDBManager::GetInstance()->AsyncQuery(queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM affect%s WHERE dwPID=%d", GetTablePostfix(), pi->player_id);
		CDBManager::GetInstance()->AsyncQuery(queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM guild_member%s WHERE pid=%d", GetTablePostfix(), pi->player_id);
		CDBManager::GetInstance()->AsyncQuery(queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM myshop_pricelist%s WHERE owner_id=%d", GetTablePostfix(), pi->player_id);
		CDBManager::GetInstance()->AsyncQuery(queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM messenger_list%s WHERE account='%s' OR companion='%s'", GetTablePostfix(), szName, szName);
		CDBManager::GetInstance()->AsyncQuery(queryStr);

		pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_SUCCESS, pi->dwHandle, 1);
		pPeer->EncodeBYTE(pi->account_index);
	}
	else
	{
		WarnLog("PLAYER_DELETE FAIL NO ROW");
		pPeer->EncodeHeader(HEADER_DG_PLAYER_DELETE_FAILED, pi->dwHandle, 1);
		pPeer->EncodeBYTE(pi->account_index);
	}
}

void CClientManager::QUERY_ADD_AFFECT(LPPEER pPeer, TPacketGDAddAffect* p)
{
	char queryStr[512];

	snprintf(queryStr, sizeof(queryStr),
			"REPLACE INTO affect%s (dwPID, bType, bApplyOn, lApplyValue, dwFlag, lDuration, lSPCost) "
			"VALUES(%u, %u, %u, %ld, %u, %ld, %ld)",
			GetTablePostfix(),
			p->dwPID,
			p->elem.dwType,
			p->elem.bApplyOn,
			p->elem.lApplyValue,
			p->elem.dwFlag,
			p->elem.lDuration,
			p->elem.lSPCost);

	CDBManager::GetInstance()->AsyncQuery(queryStr);
}

void CClientManager::QUERY_REMOVE_AFFECT(LPPEER pPeer, TPacketGDRemoveAffect* p)
{
	char queryStr[256];

	snprintf(queryStr, sizeof(queryStr),
			"DELETE FROM affect%s WHERE dwPID=%u AND bType=%u AND bApplyOn=%u",
			GetTablePostfix(), p->dwPID, p->dwType, p->bApplyOn);

	CDBManager::GetInstance()->AsyncQuery(queryStr);
}


void CClientManager::QUERY_HIGHSCORE_REGISTER(LPPEER pPeer, TPacketGDHighscore * data)
{
	char szQuery[128];
	snprintf(szQuery, sizeof(szQuery), "SELECT value FROM highscore%s WHERE board='%s' AND pid = %u", GetTablePostfix(), data->szBoard, data->dwPID);

	TraceLog("HEADER_GD_HIGHSCORE_REGISTER: PID {}", data->dwPID);

	ClientHandleInfo* pi = new ClientHandleInfo(0);
	strlcpy(pi->login, data->szBoard, sizeof(pi->login));
	pi->account_id = (uint32_t)data->lValue;
	pi->player_id = data->dwPID;
	pi->account_index = (data->cDir > 0);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_HIGHSCORE_REGISTER, pPeer->GetHandle(), pi);
}

void CClientManager::RESULT_HIGHSCORE_REGISTER(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* pi = (ClientHandleInfo*) qi->pvData;
	
	char szBoard[21];
	strlcpy(szBoard, pi->login, sizeof(szBoard));
	int32_t value = (int32_t)pi->account_id;

	SQLResult* res = msg->Get();

	if (res->uiNumRows == 0)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "INSERT INTO highscore%s VALUES('%s', %u, %d)", GetTablePostfix(), szBoard, pi->player_id, value);
		CDBManager::GetInstance()->AsyncQuery(buf);
	}
	else
	{
		if (!res->pSQLResult)
		{
			delete pi;
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(res->pSQLResult);
		if (row && row[0])
		{
			int32_t current_value = 0; str_to_number(current_value, row[0]);
			if (pi->account_index && current_value >= value || !pi->account_index && current_value <= value)
			{
				value = current_value;
			}
			else
			{
				char buf[256];
				snprintf(buf, sizeof(buf), "REPLACE INTO highscore%s VALUES('%s', %u, %d)", GetTablePostfix(), szBoard, pi->player_id, value);
				CDBManager::GetInstance()->AsyncQuery(buf);
			}
		}
		else
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "INSERT INTO highscore%s VALUES('%s', %u, %d)", GetTablePostfix(), szBoard, pi->player_id, value);
			CDBManager::GetInstance()->AsyncQuery(buf);
		}
	}
	// TODO: Check if the high score has been updated and post a notice.
	delete pi;
}

void CClientManager::InsertLogoutPlayer(uint32_t pid)
{
	auto it = m_map_logout.find(pid);

	if (it != m_map_logout.end())
	{
		TraceLog("LOGOUT: Update player time pid({})", pid);

		it->second->time = time(nullptr);
		return;
	}
		
	TLogoutPlayer* pLogout = new TLogoutPlayer;
	pLogout->pid = pid;
	pLogout->time = time(nullptr);
	m_map_logout.emplace(pid, pLogout);

	TraceLog("LOGOUT: Insert player pid({})", pid);
}

void CClientManager::DeleteLogoutPlayer(uint32_t pid)
{
	auto it = m_map_logout.find(pid);

	if (it != m_map_logout.end())
	{
		delete it->second;
		m_map_logout.erase(it);
	}
}

extern int32_t g_iLogoutSeconds;

void CClientManager::UpdateLogoutPlayer()
{
	time_t now = time(nullptr);

	auto it = m_map_logout.begin();

	while (it != m_map_logout.end())
	{
		TLogoutPlayer* pLogout = it->second;

		if (now - g_iLogoutSeconds > pLogout->time)
		{
			FlushItemCacheSet(pLogout->pid);
			FlushPlayerCacheSet(pLogout->pid);

			delete pLogout;
			m_map_logout.erase(it++);
		}
		else
			++it;
	}
}

void CClientManager::FlushPlayerCacheSet(uint32_t pid)
{
	auto it = m_map_playerCache.find(pid);

	if (it != m_map_playerCache.end())
	{
		CPlayerTableCache* pPlayerCache = it->second.get();
		pPlayerCache->Flush();

		m_map_playerCache.erase(it);
	}
}

