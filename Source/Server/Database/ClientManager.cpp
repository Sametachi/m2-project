#include "stdafx.h"

#include <Common/building.h>
#include <Common/VnumHelper.h>
#include <Core/Grid.h>

#include "ClientManager.h"

#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "GuildManager.h"
#include "PrivManager.h"
#include "MoneyLog.h"
#include "Marriage.h"
#include "Monarch.h"
#include "ItemIDRangeManager.h"
#include "Cache.h"
extern int32_t g_iPlayerCacheFlushSeconds;
extern int32_t g_iItemCacheFlushSeconds;

extern std::string g_stLocale;
extern std::string g_stLocaleNameColumn;
bool CreateItemTableFromRes(MYSQL_RES* res, std::vector<TPlayerItem>* pVec, uint32_t dwPID);

uint32_t g_dwUsageMax = 0;
uint32_t g_dwUsageAvg = 0;

CPacketInfo g_query_info;
CPacketInfo g_item_info;

CClientManager::CClientManager() :
	m_pAuthPeer(nullptr),
	m_iPlayerIDStart(0),
	m_iPlayerDeleteLevelLimit(0),
	m_iPlayerDeleteLevelLimitLower(0),
	m_bChinaEventServer(false),
	m_iShopTableSize(0),
	m_pShopTable(nullptr),
	m_iRefineTableSize(0),
	m_pRefineTable(nullptr),
	m_bShutdowned(false),
	m_iCacheFlushCount(0),
	m_iCacheFlushCountLimit(200),
	m_bLastHeader(0),
	m_fdAccept(0),
	m_bLooping(false)
{
	m_itemRange.dwMin = 0;
	m_itemRange.dwMax = 0;
	m_itemRange.dwUsableItemIDMin = 0;

	memset(g_query_count, 0, sizeof(g_query_count));
}

CClientManager::~CClientManager()
{
	Destroy();
}

void CClientManager::SetPlayerIDStart(int32_t iIDStart)
{
	m_iPlayerIDStart = iIDStart;
}

void CClientManager::Destroy()
{
	m_mChannelStatus.clear();

	for (const auto& upPeer : m_peerList)
		upPeer->Destroy();

	m_peerList.clear();

	if (m_fdAccept > 0)
	{
		socket_close(m_fdAccept);
		m_fdAccept = -1;
	}
}

bool CClientManager::Initialize()
{
	int32_t tmpValue;

	if (!InitializeLocalization())
	{
		FatalLog("Failed Localization Information so exit");
		return false;
	}

	if (!InitializeNowItemID())
	{
		SysLog("Item range Initialize Failed. Exit DBCache Server");
		return false;
	}

	if (!InitializeTables())
	{
		SysLog("Table Initialize FAILED");
		return false;
	}

	CGuildManager::GetInstance()->BootReserveWar();

	if (!CConfig::GetInstance()->GetValue("BIND_PORT", &tmpValue))
		tmpValue = 5300;

	char szBindIP[128];

	if (!CConfig::GetInstance()->GetValue("BIND_IP", szBindIP, 128))
		strlcpy(szBindIP, "0", sizeof(szBindIP));

	m_fdAccept = socket_tcp_bind(szBindIP, tmpValue);

	if (m_fdAccept < 0)
	{
		perror("socket");
		return false;
	}

	TraceLog("ACCEPT_HANDLE: {}", m_fdAccept);
	fdwatch_add_fd(m_fdWatcher, m_fdAccept, nullptr, FDW_READ, false);

	if (!CConfig::GetInstance()->GetValue("BACKUP_LIMIT_SEC", &tmpValue))
		tmpValue = 600;

	m_bLooping = true;

	if (!CConfig::GetInstance()->GetValue("PLAYER_DELETE_LEVEL_LIMIT", &m_iPlayerDeleteLevelLimit))
	{
		SysLog("CONFIG: Cannot find PLAYER_DELETE_LEVEL_LIMIT, use default level {}", PLAYER_MAX_LEVEL_CONST + 1);
		m_iPlayerDeleteLevelLimit = PLAYER_MAX_LEVEL_CONST + 1;
	}

	if (!CConfig::GetInstance()->GetValue("PLAYER_DELETE_LEVEL_LIMIT_LOWER", &m_iPlayerDeleteLevelLimitLower))
	{
		m_iPlayerDeleteLevelLimitLower = 0;
	}

	TraceLog("PLAYER_DELETE_LEVEL_LIMIT set to {}", m_iPlayerDeleteLevelLimit);
	TraceLog("PLAYER_DELETE_LEVEL_LIMIT_LOWER set to {}", m_iPlayerDeleteLevelLimitLower);

	m_bChinaEventServer = false;

	int32_t	iChinaEventServer = 0;

	if (CConfig::GetInstance()->GetValue("CHINA_EVENT_SERVER", &iChinaEventServer))
		m_bChinaEventServer = (iChinaEventServer);

	TraceLog("CHINA_EVENT_SERVER {}", CClientManager::GetInstance()->IsChinaEventServer());


	LoadEventFlag();

	return true;
}

void CClientManager::MainLoop()
{
	SQLMsg* pSQLMsg;

	while (!m_bShutdowned)
	{
		while ((pSQLMsg = CDBManager::GetInstance()->PopResult()))
		{
			AnalyzeQueryResult(pSQLMsg);
			delete pSQLMsg;
		}

		if (!Process())
			break;
	}

	TraceLog("MainLoop exited, Starting cache flushing");

	signal_timer_disable();

	for (const auto& [dwKey, pPlayerCache] : m_map_itemCache)
		pPlayerCache->Flush();

	m_map_playerCache.clear();

	for (const auto& [dwKey, pItemCache] : m_map_itemCache)
		pItemCache->Flush();

	m_map_itemCache.clear();

	for (const auto& [dwKey, pItemPricelistCache] : m_mapItemPriceListCache)
		pItemPricelistCache->Flush();

	m_mapItemPriceListCache.clear();
}

void CClientManager::Quit()
{
	m_bShutdowned = true;
}

void CClientManager::QUERY_BOOT(LPPEER pPeer, TPacketGDBoot* p)
{
	const uint8_t bPacketVersion = 6; // Each time the BOOT packet changes, the number is incremented.

	std::vector<tAdminInfo> vAdmin;
	std::vector<std::string> vHost;

	__GetHostInfo(vHost);
	__GetAdminInfo(p->szIP, vAdmin);	

	TraceLog("QUERY_BOOT : AdminInfo (Request ServerIp {}) ", p->szIP);

	uint32_t dwPacketSize = 
		sizeof(uint32_t) +
		sizeof(uint8_t) +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TMobTable) * m_vec_mobTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TItemTable) * m_vec_itemTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TShopTable) * m_iShopTableSize +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TSkillTable) * m_vec_skillTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TRefineTable) * m_iRefineTableSize +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TItemAttrTable) * m_vec_itemAttrTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TItemAttrTable) * m_vec_itemRareTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TBanwordTable) * m_vec_banwordTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(building::TLand) * m_vec_kLandTable.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(building::TObjectProto) * m_vec_kObjectProto.size() + 
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(building::TObject) * m_map_pObjectTable.size() +
		sizeof(time_t) + 
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TItemIDRangeTable)*2 +
		sizeof(uint16_t) + sizeof(uint16_t) + 16 * vHost.size() +
		sizeof(uint16_t) + sizeof(uint16_t) +  sizeof(tAdminInfo) *  vAdmin.size() +
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(TMonarchInfo) + 
		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(MonarchCandidacy)* CMonarch::GetInstance()->MonarchCandidacySize() +
		sizeof(uint16_t); 

	pPeer->EncodeHeader(HEADER_DG_BOOT, 0, dwPacketSize);
	pPeer->Encode(&dwPacketSize, sizeof(uint32_t));
	pPeer->Encode(&bPacketVersion, sizeof(uint8_t));

	TraceLog("BOOT: PACKET: {}", dwPacketSize);
	TraceLog("BOOT: VERSION: {}", bPacketVersion);

	TraceLog("sizeof(TMobTable) = {}", sizeof(TMobTable));
	TraceLog("sizeof(TItemTable) = {}", sizeof(TItemTable));
	TraceLog("sizeof(TShopTable) = {}", sizeof(TShopTable));
	TraceLog("sizeof(TSkillTable) = {}", sizeof(TSkillTable));
	TraceLog("sizeof(TRefineTable) = {}", sizeof(TRefineTable));
	TraceLog("sizeof(TItemAttrTable) = {}", sizeof(TItemAttrTable));
	TraceLog("sizeof(TItemRareTable) = {}", sizeof(TItemAttrTable));
	TraceLog("sizeof(TBanwordTable) = {}", sizeof(TBanwordTable));
	TraceLog("sizeof(TLand) = {}", sizeof(building::TLand));
	TraceLog("sizeof(TObjectProto) = {}", sizeof(building::TObjectProto));
	TraceLog("sizeof(TObject) = {}", sizeof(building::TObject));
	TraceLog("sizeof(tAdminInfo) = {}", sizeof(tAdminInfo) * vAdmin.size());
	TraceLog("sizeof(TMonarchInfo) = {}", sizeof(TMonarchInfo));

	pPeer->EncodeWORD(sizeof(TMobTable));
	pPeer->EncodeWORD(m_vec_mobTable.size());
	if (m_vec_mobTable.size())
		pPeer->Encode(&m_vec_mobTable[0], sizeof(TMobTable) * m_vec_mobTable.size());

	pPeer->EncodeWORD(sizeof(TItemTable));
	pPeer->EncodeWORD(m_vec_itemTable.size());
	if (m_vec_itemTable.size())
		pPeer->Encode(&m_vec_itemTable[0], sizeof(TItemTable) * m_vec_itemTable.size());

	pPeer->EncodeWORD(sizeof(TShopTable));
	pPeer->EncodeWORD(m_iShopTableSize);
	if (m_iShopTableSize)
		pPeer->Encode(m_pShopTable, sizeof(TShopTable) * m_iShopTableSize);

	pPeer->EncodeWORD(sizeof(TSkillTable));
	pPeer->EncodeWORD(m_vec_skillTable.size());
	if (m_vec_skillTable.size())
		pPeer->Encode(&m_vec_skillTable[0], sizeof(TSkillTable) * m_vec_skillTable.size());

	pPeer->EncodeWORD(sizeof(TRefineTable));
	pPeer->EncodeWORD(m_iRefineTableSize);
	if (m_iRefineTableSize)
		pPeer->Encode(m_pRefineTable, sizeof(TRefineTable) * m_iRefineTableSize);

	pPeer->EncodeWORD(sizeof(TItemAttrTable));
	pPeer->EncodeWORD(m_vec_itemAttrTable.size());
	if (m_vec_itemAttrTable.size())
		pPeer->Encode(&m_vec_itemAttrTable[0], sizeof(TItemAttrTable) * m_vec_itemAttrTable.size());

	pPeer->EncodeWORD(sizeof(TItemAttrTable));
	pPeer->EncodeWORD(m_vec_itemRareTable.size());
	if (m_vec_itemRareTable.size())
		pPeer->Encode(&m_vec_itemRareTable[0], sizeof(TItemAttrTable) * m_vec_itemRareTable.size());

	pPeer->EncodeWORD(sizeof(TBanwordTable));
	pPeer->EncodeWORD(m_vec_banwordTable.size());
	if (m_vec_banwordTable.size())
		pPeer->Encode(&m_vec_banwordTable[0], sizeof(TBanwordTable) * m_vec_banwordTable.size());

	pPeer->EncodeWORD(sizeof(building::TLand));
	pPeer->EncodeWORD(m_vec_kLandTable.size());
	if (m_vec_kLandTable.size())
		pPeer->Encode(&m_vec_kLandTable[0], sizeof(building::TLand) * m_vec_kLandTable.size());

	pPeer->EncodeWORD(sizeof(building::TObjectProto));
	pPeer->EncodeWORD(m_vec_kObjectProto.size());
	pPeer->Encode(&m_vec_kObjectProto[0], sizeof(building::TObjectProto) * m_vec_kObjectProto.size());

	pPeer->EncodeWORD(sizeof(building::TObject));
	pPeer->EncodeWORD(m_map_pObjectTable.size());

	auto it = m_map_pObjectTable.begin();

	while (it != m_map_pObjectTable.end())
		pPeer->Encode((it++)->second, sizeof(building::TObject));

	time_t now = time(nullptr);
	pPeer->Encode(&now, sizeof(time_t));

	TItemIDRangeTable itemRange = CItemIDRangeManager::GetInstance()->GetRange();
	TItemIDRangeTable itemRangeSpare = CItemIDRangeManager::GetInstance()->GetRange();

	pPeer->EncodeWORD(sizeof(TItemIDRangeTable));
	pPeer->EncodeWORD(1);
	pPeer->Encode(&itemRange, sizeof(TItemIDRangeTable));
	pPeer->Encode(&itemRangeSpare, sizeof(TItemIDRangeTable));

	pPeer->SetItemIDRange(itemRange);
	pPeer->SetSpareItemIDRange(itemRangeSpare);

	pPeer->EncodeWORD(16);
	pPeer->EncodeWORD(vHost.size());

	for (const auto& strHost : vHost)
	{
		pPeer->Encode(strHost.c_str(), 16);
		TraceLog("GMHosts {}", strHost.c_str());
	}

	pPeer->EncodeWORD(sizeof(tAdminInfo));
	pPeer->EncodeWORD(vAdmin.size());

	for (const auto& TAdmin : vAdmin)
	{
		pPeer->Encode(&TAdmin, sizeof(tAdminInfo));
		TraceLog("Admin name {} ConntactIP {}", TAdmin.m_szName, TAdmin.m_szContactIP);
	}

	pPeer->EncodeWORD(sizeof(TMonarchInfo));
	pPeer->EncodeWORD(1);
	pPeer->Encode(CMonarch::GetInstance()->GetMonarch(), sizeof(TMonarchInfo));

	CMonarch::VEC_MONARCHCANDIDACY& rVecMonarchCandidacy = CMonarch::GetInstance()->GetVecMonarchCandidacy();
	
	size_t num_monarch_candidacy = CMonarch::GetInstance()->MonarchCandidacySize();
	pPeer->EncodeWORD(sizeof(MonarchCandidacy));
	pPeer->EncodeWORD(num_monarch_candidacy);
	if (num_monarch_candidacy != 0) {
		pPeer->Encode(&rVecMonarchCandidacy[0], sizeof(MonarchCandidacy) * num_monarch_candidacy);
	}

	TraceLog("MONARCHCandidacy Size {}", CMonarch::GetInstance()->MonarchCandidacySize());

	pPeer->EncodeWORD(0xffff);
}

void CClientManager::SendPartyOnSetup(LPPEER pPeer)
{
	TPartyMap& pm = m_map_pChannelParty[pPeer->GetChannel()];

	for (auto it_party = pm.begin(); it_party != pm.end(); ++it_party)
	{
		TraceLog("PARTY SendPartyOnSetup Party [{}]", it_party->first);
		pPeer->EncodeHeader(HEADER_DG_PARTY_CREATE, 0, sizeof(TPacketPartyCreate));
		pPeer->Encode(&it_party->first, sizeof(uint32_t));

		for (auto it_member = it_party->second.begin(); it_member != it_party->second.end(); ++it_member)
		{
			TraceLog("PARTY SendPartyOnSetup Party [{}] Member [{}]", it_party->first, it_member->first);
			pPeer->EncodeHeader(HEADER_DG_PARTY_ADD, 0, sizeof(TPacketPartyAdd));
			pPeer->Encode(&it_party->first, sizeof(uint32_t));
			pPeer->Encode(&it_member->first, sizeof(uint32_t));
			pPeer->Encode(&it_member->second.bRole, sizeof(uint8_t));

			pPeer->EncodeHeader(HEADER_DG_PARTY_SET_MEMBER_LEVEL, 0, sizeof(TPacketPartySetMemberLevel));
			pPeer->Encode(&it_party->first, sizeof(uint32_t));
			pPeer->Encode(&it_member->first, sizeof(uint32_t));
			pPeer->Encode(&it_member->second.bLevel, sizeof(uint8_t));
		}
	}
}

void CClientManager::QUERY_PLAYER_COUNT(LPPEER pPeer, TPlayerCountPacket* pPacket)
{
	pPeer->SetUserCount(pPacket->dwCount);
}

void CClientManager::QUERY_QUEST_SAVE(LPPEER pPeer, TQuestTable* pTable, uint32_t dwLen)
{
	if (0 != (dwLen % sizeof(TQuestTable)))
	{
		SysLog("invalid packet size {}, sizeof(TQuestTable) == {}", dwLen, sizeof(TQuestTable));
		return;
	}

	int32_t iSize = dwLen / sizeof(TQuestTable);

	char szQuery[1024];

	for (int32_t i = 0; i < iSize; ++i, ++pTable)
	{
		if (pTable->lValue == 0)
		{
			snprintf(szQuery, sizeof(szQuery),
					"DELETE FROM quest%s WHERE dwPID=%d AND szName='%s' AND szState='%s'",
					GetTablePostfix(), pTable->dwPID, pTable->szName, pTable->szState);
		}
		else
		{
			snprintf(szQuery, sizeof(szQuery),
					"REPLACE INTO quest%s (dwPID, szName, szState, lValue) VALUES(%d, '%s', '%s', %ld)",
					GetTablePostfix(), pTable->dwPID, pTable->szName, pTable->szState, pTable->lValue);
		}

		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_QUEST_SAVE, pPeer->GetHandle(), nullptr);
	}
}

void CClientManager::QUERY_SAFEBOX_LOAD(LPPEER pPeer, uint32_t dwHandle, TSafeboxLoadPacket* packet, bool bMall)
{
	ClientHandleInfo* pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, packet->szPassword, sizeof(pi->safebox_password));
	pi->account_id = packet->dwID;
	pi->account_index = 0;
	pi->ip[0] = bMall ? 1 : 0;
	strlcpy(pi->login, packet->szLogin, sizeof(pi->login));

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT account_id, size, password FROM safebox%s WHERE account_id=%u",
			GetTablePostfix(), packet->dwID);

	TraceLog("HEADER_GD_SAFEBOX_LOAD (handle: {} account.id {} is_mall {})", dwHandle, packet->dwID, bMall);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_LOAD(LPPEER pPeer, SQLMsg* msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* pi = (ClientHandleInfo*) qi->pvData;
	uint32_t dwHandle = pi->dwHandle;

	// The account_index used here refers to the query order.
	// Query to get the first password is 0
	// Query to get the second real data is 1

	if (pi->account_index == 0)
	{
		char szSafeboxPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
		strlcpy(szSafeboxPassword, pi->safebox_password, sizeof(szSafeboxPassword));

		TSafeboxTable* pSafebox = new TSafeboxTable;
		memset(pSafebox, 0, sizeof(TSafeboxTable));

		SQLResult* res = msg->Get();

		if (res->uiNumRows == 0)
		{
			if (strcmp("000000", szSafeboxPassword))
			{
				pPeer->EncodeHeader(HEADER_DG_SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}
		}
		else
		{
			MYSQL_ROW row = mysql_fetch_row(res->pSQLResult);

			// Wrong password..
			if (((!row[2] || !*row[2]) && strcmp("000000", szSafeboxPassword)) ||
				((row[2] && *row[2]) && strcmp(row[2], szSafeboxPassword)))
			{
				pPeer->EncodeHeader(HEADER_DG_SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}

			if (!row[0])
				pSafebox->dwID = 0;
			else
				str_to_number(pSafebox->dwID, row[0]);

			if (!row[1])
				pSafebox->bSize = 0;
			else
				str_to_number(pSafebox->bSize, row[1]);

			TraceLog("SAFEBOX id[{}] size[{}]", pSafebox->dwID, pSafebox->bSize);
		}

		if (0 == pSafebox->dwID)
			pSafebox->dwID = pi->account_id;

		pi->pSafebox = pSafebox;

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), 
				"SELECT id, window+0, pos, count, vnum, socket0, socket1, socket2, "
				"attrtype0, attrvalue0, "
				"attrtype1, attrvalue1, "
				"attrtype2, attrvalue2, "
				"attrtype3, attrvalue3, "
				"attrtype4, attrvalue4, "
				"attrtype5, attrvalue5, "
				"attrtype6, attrvalue6 "
				"FROM item%s WHERE owner_id=%d AND window='%s'",
				GetTablePostfix(), pi->account_id, "SAFEBOX");

		pi->account_index = 1;

		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pPeer->GetHandle(), pi);
	}
	else
	{

		if (!pi->pSafebox)
		{
			SysLog("null safebox pointer!");
			delete pi;
			return;
		}


		// There was an error in the query, so if you respond, it will be as if the warehouse is empty
		// Better not open the warehouse at all because you can see it
		if (!msg->Get()->pSQLResult)
		{
			SysLog("null safebox result");
			delete pi;
			return;
		}

		static std::vector<TPlayerItem> s_items;
		CreateItemTableFromRes(msg->Get()->pSQLResult, &s_items, pi->account_id);

		pi->pSafebox->wItemCount = s_items.size();

		pPeer->EncodeHeader(HEADER_DG_SAFEBOX_LOAD, dwHandle, sizeof(TSafeboxTable) + sizeof(TPlayerItem) * s_items.size());

		pPeer->Encode(pi->pSafebox, sizeof(TSafeboxTable));

		if (!s_items.empty())
			pPeer->Encode(&s_items[0], sizeof(TPlayerItem) * s_items.size());

		delete pi;
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_SIZE(LPPEER pPeer, uint32_t dwHandle, TSafeboxChangeSizePacket* p)
{
	ClientHandleInfo* pi = new ClientHandleInfo(dwHandle);
	pi->account_index = p->bSize;	// Temporarily use account_index as size

	char szQuery[256];

	if (p->bSize == 1)
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO safebox%s (account_id, size) VALUES(%u, %u)", GetTablePostfix(), p->dwID, p->bSize);
	else
		snprintf(szQuery, sizeof(szQuery), "UPDATE safebox%s SET size=%u WHERE account_id=%u", GetTablePostfix(), p->bSize, p->dwID);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_SIZE, pPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_SIZE(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* p = (ClientHandleInfo*) qi->pvData;
	uint32_t dwHandle = p->dwHandle;
	uint8_t bSize = p->account_index;

	delete p;

	if (msg->Get()->uiNumRows > 0)
	{
		pPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_SIZE, dwHandle, sizeof(uint8_t));
		pPeer->EncodeBYTE(bSize);
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_PASSWORD(LPPEER pPeer, uint32_t dwHandle, TSafeboxChangePasswordPacket* p)
{
	ClientHandleInfo* pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, p->szNewPassword, sizeof(pi->safebox_password));
	strlcpy(pi->login, p->szOldPassword, sizeof(pi->login));
	pi->account_id = p->dwID;

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT password FROM safebox%s WHERE account_id=%u", GetTablePostfix(), p->dwID);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD, pPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* p = (ClientHandleInfo*) qi->pvData;
	uint32_t dwHandle = p->dwHandle;

	if (msg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && *row[0] && !strcasecmp(row[0], p->login) || (!row[0] || !*row[0]) && !strcmp("000000", p->login))
		{
			char szQuery[256];
			char escape_pwd[sizeof(p->safebox_password) * 2];
			CDBManager::GetInstance()->EscapeString(escape_pwd, p->safebox_password, strlen(p->safebox_password));

			snprintf(szQuery, sizeof(szQuery), "UPDATE safebox%s SET password='%s' WHERE account_id=%u", GetTablePostfix(), escape_pwd, p->account_id);

			CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD_SECOND, pPeer->GetHandle(), p);
			return;
		}
	}

	delete p;

	// Wrong old password
	pPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER, dwHandle, sizeof(uint8_t));
	pPeer->EncodeBYTE(0);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(LPPEER pPeer, SQLMsg * msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	ClientHandleInfo* p = (ClientHandleInfo*) qi->pvData;
	uint32_t dwHandle = p->dwHandle;
	delete p;

	pPeer->EncodeHeader(HEADER_DG_SAFEBOX_CHANGE_PASSWORD_ANSWER, dwHandle, sizeof(uint8_t));
	pPeer->EncodeBYTE(1);
}

void CClientManager::RESULT_PRICELIST_LOAD(LPPEER pPeer, SQLMsg* pMsg)
{
	TItemPricelistReqInfo* pReqInfo = (TItemPricelistReqInfo*)static_cast<CQueryInfo*>(pMsg->pvUserData)->pvData;
	
	TItemPriceListTable table;
	table.dwOwnerID = pReqInfo->second;
	table.byCount = 0;
	
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		str_to_number(table.aPriceInfo[table.byCount].dwVnum, row[0]);
		str_to_number(table.aPriceInfo[table.byCount].dwPrice, row[1]);
		table.byCount++;
	}

	PutItemPriceListCache(&table);

	TPacketMyshopPricelistHeader header;

	header.dwOwnerID = pReqInfo->second;
	header.byCount = table.byCount;

	size_t sizePriceListSize = sizeof(TItemPriceInfo) * header.byCount;

	pPeer->EncodeHeader(HEADER_DG_MYSHOP_PRICELIST_RES, pReqInfo->first, sizeof(header) + sizePriceListSize);
	pPeer->Encode(&header, sizeof(header));
	pPeer->Encode(table.aPriceInfo, sizePriceListSize);

	TraceLog("Load MyShopPricelist handle[{}] pid[{}] count[{}]", pReqInfo->first, pReqInfo->second, header.byCount);

	delete pReqInfo;
}

void CClientManager::RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg)
{
	TItemPriceListTable* pUpdateTable = (TItemPriceListTable*)static_cast<CQueryInfo*>(pMsg->pvUserData)->pvData;

	TItemPriceListTable table;
	table.dwOwnerID = pUpdateTable->dwOwnerID;
	table.byCount = 0;
	
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		str_to_number(table.aPriceInfo[table.byCount].dwVnum, row[0]);
		str_to_number(table.aPriceInfo[table.byCount].dwPrice, row[1]);
		table.byCount++;
	}

	PutItemPriceListCache(&table);

	GetItemPriceListCache(pUpdateTable->dwOwnerID)->UpdateList(pUpdateTable);

	delete pUpdateTable;
}

void CClientManager::QUERY_SAFEBOX_SAVE(LPPEER pPeer, TSafeboxTable* pTable)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery),
			"UPDATE safebox%s SET gold='%u' WHERE account_id=%u", 
			GetTablePostfix(), pTable->dwGold, pTable->dwID);

	CDBManager::GetInstance()->ReturnQuery(szQuery, QID_SAFEBOX_SAVE, pPeer->GetHandle(), nullptr);
}

void CClientManager::QUERY_EMPIRE_SELECT(LPPEER pPeer, uint32_t dwHandle, TEmpireSelectPacket* p)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery), "UPDATE player_index%s SET empire=%u WHERE id=%u", GetTablePostfix(), p->bEmpire, p->dwAccountID);
	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	TraceLog("EmpireSelect: {}", szQuery);
	{
		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid1, pid2, pid3, pid4 FROM player_index%s WHERE id=%u", GetTablePostfix(), p->dwAccountID);

		std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

		SQLResult* pRes = pmsg->Get();

		if (pRes->uiNumRows)
		{
			TraceLog("EMPIRE {}", pRes->uiNumRows);

			MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
			uint32_t pids[3];

			UINT g_start_map[4] =
			{
				0,
				1,  // Red
				21, // Yellow
				41  // Blue
			};

			// TODO: Share with game
			uint32_t g_start_position[4][2]=
			{
				{      0,      0 },
				{ 469300, 964200 }, // Red
				{  55700, 157900 }, // Yellow
				{ 969600, 278400 }  // Blue
			};

			for (int32_t i = 0; i < 3; ++i)
			{
				str_to_number(pids[i], row[i]);
				TraceLog("EMPIRE PIDS[{}]", pids[i]);

				if (pids[i])
				{
					TraceLog("EMPIRE move to pid[{}] to village of {}, map_index {}", 
							pids[i], p->bEmpire, g_start_map[p->bEmpire]);

					snprintf(szQuery, sizeof(szQuery), "UPDATE player%s SET map_index=%u,x=%u,y=%u WHERE id=%u", 
							GetTablePostfix(),
							g_start_map[p->bEmpire],
							g_start_position[p->bEmpire][0],
							g_start_position[p->bEmpire][1],
							pids[i]);

					std::unique_ptr<SQLMsg> pSQLMsg2(CDBManager::GetInstance()->DirectQuery(szQuery));
				}
			}
		}
	}

	pPeer->EncodeHeader(HEADER_DG_EMPIRE_SELECT, dwHandle, sizeof(uint8_t));
	pPeer->EncodeBYTE(p->bEmpire);
}

void CClientManager::QUERY_SETUP(LPPEER pGuestPeer, uint32_t dwHandle, const char* c_pData)
{
	TPacketGDSetup* p = (TPacketGDSetup*) c_pData;
	c_pData += sizeof(TPacketGDSetup);

	if (p->bAuthServer)
	{
		m_pAuthPeer = pGuestPeer;
		return;
	}

	pGuestPeer->SetPublicIP(p->szPublicIP);
	pGuestPeer->SetChannel(p->bChannel);
	pGuestPeer->SetListenPort(p->wListenPort);
	pGuestPeer->SetP2PPort(p->wP2PPort);
	pGuestPeer->SetMaps(p->alMaps);

	// Send which map is on which server
	TMapLocation kMapLocations;

	strlcpy(kMapLocations.szHost, pGuestPeer->GetPublicIP(), sizeof(kMapLocations.szHost));
	kMapLocations.wPort = pGuestPeer->GetListenPort();
	memcpy(kMapLocations.alMaps, pGuestPeer->GetMaps(), sizeof(kMapLocations.alMaps));

	uint8_t bMapCount;

	std::vector<TMapLocation> vec_kMapLocations;

	if (pGuestPeer->GetChannel() == 1)
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (pPeer == pGuestPeer)
				continue;

			if (!pPeer->GetChannel())
				continue;

			if (pPeer->GetChannel() == GUILD_WARP_WAR_CHANNEL || pPeer->GetChannel() == pGuestPeer->GetChannel())
			{
				TMapLocation kMapLocation2;
				strlcpy(kMapLocation2.szHost, pPeer->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = pPeer->GetListenPort();
				memcpy(kMapLocation2.alMaps, pPeer->GetMaps(), sizeof(kMapLocation2.alMaps));
				vec_kMapLocations.push_back(kMapLocation2);

				pPeer->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(uint8_t) + sizeof(TMapLocation));
				bMapCount = 1;
				pPeer->EncodeBYTE(bMapCount);
				pPeer->Encode(&kMapLocations, sizeof(TMapLocation));
			}
		}
	}
	else if (pGuestPeer->GetChannel() == GUILD_WARP_WAR_CHANNEL)
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (pPeer == pGuestPeer)
				continue;

			if (!pPeer->GetChannel())
				continue;

			if (pPeer->GetChannel() == 1 || pPeer->GetChannel() == pGuestPeer->GetChannel())
			{
				TMapLocation kMapLocation2;
				strlcpy(kMapLocation2.szHost, pPeer->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = pPeer->GetListenPort();
				memcpy(kMapLocation2.alMaps, pPeer->GetMaps(), sizeof(kMapLocation2.alMaps));
				vec_kMapLocations.push_back(kMapLocation2);
			}

			pPeer->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(uint8_t) + sizeof(TMapLocation));
			bMapCount = 1;
			pPeer->EncodeBYTE(bMapCount);
			pPeer->Encode(&kMapLocations, sizeof(TMapLocation));
		}
	}
	else
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (pPeer == pGuestPeer)
				continue;

			if (!pPeer->GetChannel())
				continue;

			if (pPeer->GetChannel() == GUILD_WARP_WAR_CHANNEL || pPeer->GetChannel() == pGuestPeer->GetChannel())
			{
				TMapLocation kMapLocation2;

				strlcpy(kMapLocation2.szHost, pPeer->GetPublicIP(), sizeof(kMapLocation2.szHost));
				kMapLocation2.wPort = pPeer->GetListenPort();
				memcpy(kMapLocation2.alMaps, pPeer->GetMaps(), sizeof(kMapLocation2.alMaps));

				vec_kMapLocations.push_back(kMapLocation2);
			}

			if (pPeer->GetChannel() == pGuestPeer->GetChannel())
			{
				pPeer->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(uint8_t) + sizeof(TMapLocation));
				bMapCount = 1;
				pPeer->EncodeBYTE(bMapCount);
				pPeer->Encode(&kMapLocations, sizeof(TMapLocation));
			}
		}
	}

	vec_kMapLocations.push_back(kMapLocations);

	pGuestPeer->EncodeHeader(HEADER_DG_MAP_LOCATIONS, 0, sizeof(uint8_t) + sizeof(TMapLocation) * vec_kMapLocations.size());
	bMapCount = vec_kMapLocations.size();
	pGuestPeer->EncodeBYTE(bMapCount);
	pGuestPeer->Encode(&vec_kMapLocations[0], sizeof(TMapLocation) * vec_kMapLocations.size());
	
	// Setup: Make other pPeers connect to the connected pPeer. (Create P2P connection)
	PyLog("SETUP: channel {} listen {} p2p {} count {}", pGuestPeer->GetChannel(), p->wListenPort, p->wP2PPort, bMapCount);

	TPacketDGP2P p2pSetupPacket;
	p2pSetupPacket.wPort = pGuestPeer->GetP2PPort();
	p2pSetupPacket.bChannel = pGuestPeer->GetChannel();
	strlcpy(p2pSetupPacket.szHost, pGuestPeer->GetPublicIP(), sizeof(p2pSetupPacket.szHost));

	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (pPeer == pGuestPeer)
			continue;

		// If the channel is 0, it can be considered as a pPeer or auth that has not yet received a SETUP packet.
		if (0 == pPeer->GetChannel())
			continue;

		pPeer->EncodeHeader(HEADER_DG_P2P, 0, sizeof(TPacketDGP2P));
		pPeer->Encode(&p2pSetupPacket, sizeof(TPacketDGP2P));
	}

	// Login
	TPacketLoginOnSetup* pck = (TPacketLoginOnSetup*) c_pData;;

	for (uint32_t c = 0; c < p->dwLoginCount; ++c, ++pck)
	{
		CLoginData* pLD = new CLoginData;

		pLD->SetKey(pck->dwLoginKey);
		pLD->SetClientKey(pck->adwClientKey);
		pLD->SetIP(pck->szHost);

		TAccountTable& r = pLD->GetAccountRef();

		r.id = pck->dwID;
		trim_and_lower(pck->szLogin, r.login, sizeof(r.login));
		strlcpy(r.social_id, pck->szSocialID, sizeof(r.social_id));
		strlcpy(r.passwd, "TEMP", sizeof(r.passwd));

		InsertLoginData(pLD);

		if (InsertLogonAccount(pck->szLogin, pGuestPeer->GetHandle(), pck->szHost))
		{
			PyLog("SETUP: login {} {} login_key {} host {}", pck->dwID, pck->szLogin, pck->dwLoginKey, pck->szHost);
			pLD->SetPlay(true);
		}
		else
		{
			PyLog("SETUP: login_fail {} {} login_key {}", pck->dwID, pck->szLogin, pck->dwLoginKey);
		}
	}

	SendPartyOnSetup(pGuestPeer);
	CGuildManager::GetInstance()->OnSetup(pGuestPeer);
	CPrivManager::GetInstance()->SendPrivOnSetup(pGuestPeer);
	SendEventFlagsOnSetup(pGuestPeer);
	CMarriageManager::GetInstance()->OnSetup(pGuestPeer);
}

void CClientManager::QUERY_ITEM_FLUSH(LPPEER pPeer, const char* c_pData)
{
	uint32_t dwID = *(uint32_t*) c_pData;

	TraceLog("HEADER_GD_ITEM_FLUSH: {}", dwID);

	CItemCache* c = GetItemCache(dwID);

	if (c)
		c->Flush();
}

void CClientManager::QUERY_ITEM_SAVE(LPPEER pPeer, const char* c_pData)
{
	TPlayerItem* p = (TPlayerItem*) c_pData;

	// If it is a warehouse, it is not cached, and what was in the cache should be discarded.
	// auction must not take this route. You have to ride EnrollInAuction.

	if (p->window == SAFEBOX || p->window == MALL)
	{
		CItemCache* pItemCache = GetItemCache(p->id);

		if (pItemCache)
		{
			auto it = m_map_pItemCacheSetPtr.find(pItemCache->Get()->owner);

			if (it != m_map_pItemCacheSetPtr.end())
			{
				TraceLog("ITEM_CACHE: safe-box owner {} id {}", pItemCache->Get()->owner, pItemCache->Get()->id);

				it->second->erase(pItemCache);
			}

			m_map_itemCache.erase(p->id);
		}
		char szQuery[512];

		snprintf(szQuery, sizeof(szQuery), 
			"REPLACE INTO item%s (id, owner_id, window, pos, count, vnum, socket0, socket1, socket2, "
			"attrtype0, attrvalue0, "
			"attrtype1, attrvalue1, "
			"attrtype2, attrvalue2, "
			"attrtype3, attrvalue3, "
			"attrtype4, attrvalue4, "
			"attrtype5, attrvalue5, "
			"attrtype6, attrvalue6) "
			"VALUES(%u, %u, %d, %d, %u, %u, %ld, %ld, %ld, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
			GetTablePostfix(),
			p->id,
			p->owner,
			p->window,
			p->pos,
			p->count,
			p->vnum,
			p->alSockets[0],
			p->alSockets[1],
			p->alSockets[2],
			p->aAttr[0].bType, p->aAttr[0].sValue,
			p->aAttr[1].bType, p->aAttr[1].sValue,
			p->aAttr[2].bType, p->aAttr[2].sValue,
			p->aAttr[3].bType, p->aAttr[3].sValue,
			p->aAttr[4].bType, p->aAttr[4].sValue,
			p->aAttr[5].bType, p->aAttr[5].sValue,
			p->aAttr[6].bType, p->aAttr[6].sValue);

		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEM_SAVE, pPeer->GetHandle(), nullptr);
	}
	else
	{
		TraceLog("QUERY_ITEM_SAVE => PutItemCache() owner {} id {} vnum {} ", p->owner, p->id, p->vnum);

		PutItemCache(p);
	}
}

CClientManager::TItemCacheSet * CClientManager::GetItemCacheSet(uint32_t pid)
{
	auto it = m_map_pItemCacheSetPtr.find(pid);

	if (it == m_map_pItemCacheSetPtr.end())
		return nullptr;

	return it->second.get();
}

void CClientManager::CreateItemCacheSet(uint32_t pid)
{
	if (m_map_pItemCacheSetPtr.find(pid) != m_map_pItemCacheSetPtr.end())
		return;

	std::unique_ptr<TItemCacheSet> upItemCacheSet = std::make_unique<TItemCacheSet>();
	m_map_pItemCacheSetPtr.emplace(pid, std::move(upItemCacheSet));

	TraceLog("ITEM_CACHE: new cache {}", pid);
}

void CClientManager::FlushItemCacheSet(uint32_t pid)
{
	auto it = m_map_pItemCacheSetPtr.find(pid);

	if (it == m_map_pItemCacheSetPtr.end())
	{
		WarnLog("FLUSH_ITEMCACHESET : No ItemCacheSet pid({})", pid);
		return;
	}

	TItemCacheSet* pItemCacheSet = it->second.get();
	auto it_set = pItemCacheSet->begin();

	while (it_set != pItemCacheSet->end())
	{
		CItemCache* pItemCache = *it_set++;
		pItemCache->Flush();

		m_map_itemCache.erase(pItemCache->Get()->id);
	}

	pItemCacheSet->clear();

	m_map_pItemCacheSetPtr.erase(it);

	TraceLog("FLUSH_ITEMCACHESET : Deleted pid({})", pid);
}

CItemCache * CClientManager::GetItemCache(uint32_t id)
{
	auto it = m_map_itemCache.find(id);

	if (it == m_map_itemCache.end())
		return nullptr;

	return it->second.get();
}

void CClientManager::PutItemCache(TPlayerItem* pNew, bool bSkipQuery)
{       
	CItemCache* pItemCache;     

	pItemCache = GetItemCache(pNew->id);
	
	if (!pItemCache)
	{
		TraceLog("ITEM_CACHE: PutItemCache ==> New CItemCache id{} vnum{} new owner{}", pNew->id, pNew->vnum, pNew->owner);

		std::unique_ptr<CItemCache> upItemCache = std::make_unique<CItemCache>();
		pItemCache = upItemCache.get();

		m_map_itemCache.emplace(pNew->id, std::move(upItemCache));
	}
	else
	{
		TraceLog("ITEM_CACHE: PutItemCache ==> Have Cache");

		if (pNew->owner != pItemCache->Get()->owner)
		{
			// Delete an item from a user who already has this item.
			auto it = m_map_pItemCacheSetPtr.find(pItemCache->Get()->owner);

			if (it != m_map_pItemCacheSetPtr.end())
			{
				TraceLog("ITEM_CACHE: delete owner {} id {} new owner {}", pItemCache->Get()->owner, pItemCache->Get()->id, pNew->owner);
				it->second->erase(pItemCache);
			}
		}
	}

	pItemCache->Put(pNew, bSkipQuery);
	
	auto it = m_map_pItemCacheSetPtr.find(pItemCache->Get()->owner);

	if (it != m_map_pItemCacheSetPtr.end())
	{
		TraceLog("ITEM_CACHE: save {} id {}", pItemCache->Get()->owner, pItemCache->Get()->id);
		it->second->insert(pItemCache);
	}
	else
	{
		// Since there is no current owner, it must be saved immediately by querying SQL when the next connection comes.
		// Since we can receive it, we save it immediately.
		TraceLog("ITEM_CACHE: direct save {} id {}", pItemCache->Get()->owner, pItemCache->Get()->id);

		pItemCache->OnFlush();
	}
}

bool CClientManager::DeleteItemCache(uint32_t dwID)
{
	CItemCache* c = GetItemCache(dwID);

	if (!c)
		return false;

	c->Delete();
	return true;
}

CItemPriceListTableCache* CClientManager::GetItemPriceListCache(uint32_t dwID)
{
	auto it = m_mapItemPriceListCache.find(dwID);

	if (it == m_mapItemPriceListCache.end())
		return nullptr;

	return it->second.get();
}

void CClientManager::PutItemPriceListCache(const TItemPriceListTable* pItemPriceList)
{
	CItemPriceListTableCache* pItemPricelistCache = GetItemPriceListCache(pItemPriceList->dwOwnerID);

	if (!pItemPricelistCache)
	{
		std::unique_ptr<CItemPriceListTableCache> upItemPricelistCache = std::make_unique<CItemPriceListTableCache>();
		pItemPricelistCache = upItemPricelistCache.get();

		m_mapItemPriceListCache.emplace(pItemPriceList->dwOwnerID, std::move(upItemPricelistCache));
	}

	pItemPricelistCache->Put(const_cast<TItemPriceListTable*>(pItemPriceList), true);
}

void CClientManager::UpdatePlayerCache()
{
	auto it = m_map_playerCache.begin();

	while (it != m_map_playerCache.end())
	{
		CPlayerTableCache* pPlayerCache = (it++)->second.get();

		if (pPlayerCache->CheckTimeout())
		{
			TraceLog("UPDATE : UpdatePlayerCache() ==> FlushPlayerCache {} {} ", pPlayerCache->Get(false)->id, pPlayerCache->Get(false)->name);

			pPlayerCache->Flush();

			UpdateItemCacheSet(pPlayerCache->Get()->id);
		}
		else if (pPlayerCache->CheckFlushTimeout())
			pPlayerCache->Flush();
	}
}

void CClientManager::SetCacheFlushCountLimit(int32_t iLimit)
{
	m_iCacheFlushCountLimit = MAX(10, iLimit);
	TraceLog("CACHE_FLUSH_LIMIT_PER_SECOND: {}", m_iCacheFlushCountLimit);
}

void CClientManager::UpdateItemCache()
{
	if (m_iCacheFlushCount >= m_iCacheFlushCountLimit)
		return;

	auto it = m_map_itemCache.begin();

	while (it != m_map_itemCache.end())
	{
		CItemCache* pItemCache = (it++)->second.get();

		if (pItemCache->CheckFlushTimeout())
		{
			TraceLog("UpdateItemCache ==> Flush() vnum {} id owner {}", pItemCache->Get()->vnum, pItemCache->Get()->id, pItemCache->Get()->owner);

			pItemCache->Flush();

			if (++m_iCacheFlushCount >= m_iCacheFlushCountLimit)
				break;
		}
	}
}

void CClientManager::UpdateItemPriceListCache()
{
	auto it = m_mapItemPriceListCache.begin();

	while (it != m_mapItemPriceListCache.end())
	{
		CItemPriceListTableCache* pItemPricelistCache = it->second.get();

		if (pItemPricelistCache->CheckFlushTimeout())
		{
			pItemPricelistCache->Flush();
			m_mapItemPriceListCache.erase(it++);
		}
		else
			++it;
	}
}

void CClientManager::QUERY_ITEM_DESTROY(LPPEER pPeer, const char* c_pData)
{
	uint32_t dwID = *(uint32_t*) c_pData;
	c_pData += sizeof(uint32_t);

	uint32_t dwPID = *(uint32_t*) c_pData;

	if (!DeleteItemCache(dwID))
	{
		char szQuery[64];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), dwID);

		TraceLog("HEADER_GD_ITEM_DESTROY: PID {} ID {}", dwPID, dwID);

		if (dwPID == 0)
			CDBManager::GetInstance()->AsyncQuery(szQuery);
		else
			CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEM_DESTROY, pPeer->GetHandle(), nullptr);
	}
}

void CClientManager::QUERY_FLUSH_CACHE(LPPEER pPeer, const char* c_pData)
{
	uint32_t dwPID = *(uint32_t*) c_pData;

	CPlayerTableCache* pPlayerCache = GetPlayerCache(dwPID);

	if (!pPlayerCache)
		return;

	TraceLog("FLUSH_CACHE: {}", dwPID);

	pPlayerCache->Flush();
	FlushItemCacheSet(dwPID);

	m_map_playerCache.erase(dwPID);
}

void CClientManager::QUERY_RELOAD_PROTO()
{
	if (!InitializeTables())
	{
		SysLog("QUERY_RELOAD_PROTO: cannot load tables");
		return;
	}

	for (const auto& upPeer: m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (!pPeer->GetChannel())
			continue;

		pPeer->EncodeHeader(HEADER_DG_RELOAD_PROTO, 0,
				sizeof(uint16_t) + sizeof(TSkillTable) * m_vec_skillTable.size() +
				sizeof(uint16_t) + sizeof(TBanwordTable) * m_vec_banwordTable.size() +
				sizeof(uint16_t) + sizeof(TItemTable) * m_vec_itemTable.size() +
				sizeof(uint16_t) + sizeof(TMobTable) * m_vec_mobTable.size());

		pPeer->EncodeWORD(m_vec_skillTable.size());
		if (m_vec_skillTable.size())
			pPeer->Encode(&m_vec_skillTable[0], sizeof(TSkillTable) * m_vec_skillTable.size());

		pPeer->EncodeWORD(m_vec_banwordTable.size());
		if (m_vec_banwordTable.size())
			pPeer->Encode(&m_vec_banwordTable[0], sizeof(TBanwordTable) * m_vec_banwordTable.size());

		pPeer->EncodeWORD(m_vec_itemTable.size());
		if (m_vec_itemTable.size())
			pPeer->Encode(&m_vec_itemTable[0], sizeof(TItemTable) * m_vec_itemTable.size());

		pPeer->EncodeWORD(m_vec_mobTable.size());
		if (m_vec_mobTable.size())
			pPeer->Encode(&m_vec_mobTable[0], sizeof(TMobTable) * m_vec_mobTable.size());
	}
}

void CClientManager::AddGuildPriv(TPacketGiveGuildPriv* p)
{
	CPrivManager::GetInstance()->AddGuildPriv(p->guild_id, p->type, p->value, p->duration_sec);
}

void CClientManager::AddEmpirePriv(TPacketGiveEmpirePriv* p)
{
	CPrivManager::GetInstance()->AddEmpirePriv(p->empire, p->type, p->value, p->duration_sec);
}

void CClientManager::AddCharacterPriv(TPacketGiveCharacterPriv* p)
{
	CPrivManager::GetInstance()->AddCharPriv(p->pid, p->type, p->value);
}

void CClientManager::MoneyLog(TPacketMoneyLog* p)
{
	CMoneyLog::GetInstance()->AddLog(p->type, p->vnum, p->gold);
}

CLoginData * CClientManager::GetLoginData(uint32_t dwKey)
{
	auto it = m_map_pLoginData.find(dwKey);

	if (it == m_map_pLoginData.end())
		return nullptr;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByLogin(const char* c_pszLogin)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	auto it = m_map_pLoginDataByLogin.find(szLogin);

	if (it == m_map_pLoginDataByLogin.end())
		return nullptr;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByAID(uint32_t dwAID)
{
	auto it = m_map_pLoginDataByAID.find(dwAID);

	if (it == m_map_pLoginDataByAID.end())
		return nullptr;

	return it->second;
}

void CClientManager::InsertLoginData(CLoginData* pLD)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(pLD->GetAccountRef().login, szLogin, sizeof(szLogin));

	m_map_pLoginData.emplace(pLD->GetKey(), pLD);
	m_map_pLoginDataByLogin.emplace(szLogin, pLD);
	m_map_pLoginDataByAID.emplace(pLD->GetAccountRef().id, pLD);
}

void CClientManager::DeleteLoginData(CLoginData* pLD)
{
	m_map_pLoginData.erase(pLD->GetKey());
	m_map_pLoginDataByLogin.erase(pLD->GetAccountRef().login);
	m_map_pLoginDataByAID.erase(pLD->GetAccountRef().id);

	if (m_map_kLogonAccount.find(pLD->GetAccountRef().login) == m_map_kLogonAccount.end())
		delete pLD;
	else
		pLD->SetDeleted(true);
}

void CClientManager::QUERY_AUTH_LOGIN(LPPEER pPeer, uint32_t dwHandle, TPacketGDAuthLogin* p)
{
	TraceLog("QUERY_AUTH_LOGIN {} {} {}", p->dwID, p->dwLoginKey, p->szLogin);
	CLoginData* pLD = GetLoginDataByLogin(p->szLogin);

	if (pLD)
	{
		DeleteLoginData(pLD);
	}

	uint8_t bResult;

	if (GetLoginData(p->dwLoginKey))
	{
		SysLog("LoginData already exist key {} login {}", p->dwLoginKey, p->szLogin);
		bResult = 0;

		pPeer->EncodeHeader(HEADER_DG_AUTH_LOGIN, dwHandle, sizeof(uint8_t));
		pPeer->EncodeBYTE(bResult);
	}
	else
	{
		CLoginData* pLD = new CLoginData;

		pLD->SetKey(p->dwLoginKey);
		pLD->SetClientKey(p->adwClientKey);
		pLD->SetBillType(p->bBillType);
		pLD->SetBillID(p->dwBillID);
		pLD->SetPremium(p->iPremiumTimes);

		TAccountTable& r = pLD->GetAccountRef();

		r.id = p->dwID;
		trim_and_lower(p->szLogin, r.login, sizeof(r.login));
		strlcpy(r.social_id, p->szSocialID, sizeof(r.social_id));
		strlcpy(r.passwd, "TEMP", sizeof(r.passwd));

		TraceLog("AUTH_LOGIN id({}) login({}) social_id({}) login_key({}), client_key({} {} {} {})",
				p->dwID, p->szLogin, p->szSocialID, p->dwLoginKey,
				p->adwClientKey[0], p->adwClientKey[1], p->adwClientKey[2], p->adwClientKey[3]);

		bResult = 1;

		InsertLoginData(pLD);

		pPeer->EncodeHeader(HEADER_DG_AUTH_LOGIN, dwHandle, sizeof(uint8_t));
		pPeer->EncodeBYTE(bResult);
	}
}

void CClientManager::GuildDepositMoney(TPacketGDGuildMoney* p)
{
	CGuildManager::GetInstance()->DepositMoney(p->dwGuild, p->iGold);
}

void CClientManager::GuildWithdrawMoney(LPPEER pPeer, TPacketGDGuildMoney* p)
{
	CGuildManager::GetInstance()->WithdrawMoney(pPeer, p->dwGuild, p->iGold);
}

void CClientManager::GuildWithdrawMoneyGiveReply(TPacketGDGuildMoneyWithdrawGiveReply* p)
{
	CGuildManager::GetInstance()->WithdrawMoneyReply(p->dwGuild, p->bGiveSuccess, p->iChangeGold);
}

void CClientManager::GuildWarBet(TPacketGDGuildWarBet* p)
{
	CGuildManager::GetInstance()->Bet(p->dwWarID, p->szLogin, p->dwGold, p->dwGuild);
}

void CClientManager::CreateObject(TPacketGDCreateObject* p)
{
	using namespace building;

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery),
			"INSERT INTO object%s (land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot) VALUES(%u, %u, %d, %d, %d, %f, %f, %f)",
			GetTablePostfix(), p->dwLandID, p->dwVnum, p->lMapIndex, p->x, p->y, p->xRot, p->yRot, p->zRot);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiInsertID == 0)
	{
		SysLog("cannot insert object");
		return;
	}

	TObject* pObj = new TObject;

	memset(pObj, 0, sizeof(TObject));

	pObj->dwID = pmsg->Get()->uiInsertID;
	pObj->dwVnum = p->dwVnum;
	pObj->dwLandID = p->dwLandID;
	pObj->lMapIndex = p->lMapIndex;
	pObj->x = p->x;
	pObj->y = p->y;
	pObj->xRot = p->xRot;
	pObj->yRot = p->yRot;
	pObj->zRot = p->zRot;
	pObj->lLife = 0;

	ForwardPacket(HEADER_DG_CREATE_OBJECT, pObj, sizeof(TObject));

	m_map_pObjectTable.emplace(pObj->dwID, pObj);
}

void CClientManager::DeleteObject(uint32_t dwID)
{
	char szQuery[128];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM object%s WHERE id=%u", GetTablePostfix(), dwID);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		SysLog("no object by id {}", dwID);
		return;
	}

	auto it = m_map_pObjectTable.find(dwID);

	if (it != m_map_pObjectTable.end())
	{
		delete it->second;
		m_map_pObjectTable.erase(it);
	}

	ForwardPacket(HEADER_DG_DELETE_OBJECT, &dwID, sizeof(uint32_t));
}

void CClientManager::UpdateLand(uint32_t* pdw)
{
	uint32_t dwID = pdw[0];
	uint32_t dwGuild = pdw[1];

	building::TLand* p = &m_vec_kLandTable[0];

	uint32_t i;

	for (i = 0; i < m_vec_kLandTable.size(); ++i, ++p)
	{
		if (p->dwID == dwID)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "UPDATE land%s SET guild_id=%u WHERE id=%u", GetTablePostfix(), dwGuild, dwID);
			CDBManager::GetInstance()->AsyncQuery(buf);

			p->dwGuildID = dwGuild;
			break;
		}
	}

	if (i < m_vec_kLandTable.size())
		ForwardPacket(HEADER_DG_UPDATE_LAND, p, sizeof(building::TLand));
}

void CClientManager::BlockChat(TPacketBlockChat* p)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player%s WHERE name = '%s'", GetTablePostfix(), p->szName);

	std::unique_ptr<SQLMsg> pmsg(CDBManager::GetInstance()->DirectQuery(szQuery));
	SQLResult* pRes = pmsg->Get();

	if (pRes->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
		uint32_t pid = strtoul(row[0], nullptr, 10);

		TPacketGDAddAffect pa;
		pa.dwPID = pid;
		pa.elem.dwType = 223;
		pa.elem.bApplyOn = 0;
		pa.elem.lApplyValue = 0;
		pa.elem.dwFlag = 0;
		pa.elem.lDuration = p->lDuration;
		pa.elem.lSPCost = 0;
		QUERY_ADD_AFFECT(nullptr, &pa);
	}
	else
	{
		// Cannot find user with that name
	}
}

void CClientManager::MarriageAdd(TPacketMarriageAdd* p)
{
	TraceLog("MarriageAdd {} {} {} {}", p->dwPID1, p->dwPID2, p->szName1, p->szName2);
	CMarriageManager::GetInstance()->Add(p->dwPID1, p->dwPID2, p->szName1, p->szName2);
}

void CClientManager::MarriageUpdate(TPacketMarriageUpdate* p)
{
	TraceLog("MarriageUpdate PID:{} {} LP:{} ST:{}", p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
	CMarriageManager::GetInstance()->Update(p->dwPID1, p->dwPID2, p->iLovePoint, p->byMarried);
}

void CClientManager::MarriageRemove(TPacketMarriageRemove* p)
{
	TraceLog("MarriageRemove {} {}", p->dwPID1, p->dwPID2);
	CMarriageManager::GetInstance()->Remove(p->dwPID1, p->dwPID2);
}

void CClientManager::WeddingRequest(TPacketWeddingRequest* p)
{
	TraceLog("WeddingRequest {} {}", p->dwPID1, p->dwPID2);
	ForwardPacket(HEADER_DG_WEDDING_REQUEST, p, sizeof(TPacketWeddingRequest));
}

void CClientManager::WeddingReady(TPacketWeddingReady* p)
{
	TraceLog("WeddingReady {} {}", p->dwPID1, p->dwPID2);
	ForwardPacket(HEADER_DG_WEDDING_READY, p, sizeof(TPacketWeddingReady));
	CMarriageManager::GetInstance()->ReadyWedding(p->dwMapIndex, p->dwPID1, p->dwPID2);
}

void CClientManager::WeddingEnd(TPacketWeddingEnd* p)
{
	TraceLog("WeddingEnd {} {}", p->dwPID1, p->dwPID2);
	CMarriageManager::GetInstance()->EndWedding(p->dwPID1, p->dwPID2);
}

// If there is price information in the cache, update the cache, if there is no price information in the cache
// First, after loading the existing data, create a cache with the existing information and update the newly received price information.
void CClientManager::MyshopPricelistUpdate(const TPacketMyshopPricelistHeader* pPacket)
{
	if (pPacket->byCount > SHOP_PRICELIST_MAX_NUM)
	{
		SysLog("count overflow!");
		return;
	}

	CItemPriceListTableCache* pCache = GetItemPriceListCache(pPacket->dwOwnerID);

	if (pCache)
	{
		TItemPriceListTable table;

		table.dwOwnerID = pPacket->dwOwnerID;
		table.byCount = pPacket->byCount;

		const TItemPriceInfo* pInfo = reinterpret_cast<const TItemPriceInfo*>(pPacket + sizeof(TPacketMyshopPricelistHeader));
		memcpy(table.aPriceInfo, pInfo, sizeof(TItemPriceInfo)* pPacket->byCount);

		pCache->UpdateList(&table);
	}
	else
	{
		TItemPriceListTable* pUpdateTable = new TItemPriceListTable;

		pUpdateTable->dwOwnerID = pPacket->dwOwnerID;
		pUpdateTable->byCount = pPacket->byCount;

		const TItemPriceInfo* pInfo = reinterpret_cast<const TItemPriceInfo*>(pPacket + sizeof(TPacketMyshopPricelistHeader));
		memcpy(pUpdateTable->aPriceInfo, pInfo, sizeof(TItemPriceInfo)* pPacket->byCount);

		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT item_vnum, price FROM myshop_pricelist%s WHERE owner_id=%u", GetTablePostfix(), pPacket->dwOwnerID);
		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEMPRICE_LOAD_FOR_UPDATE, 0, pUpdateTable);
	}
}

// If there is cached price information, it reads the cache and sends it immediately. 
// If there is no information in the cache, it queries the DB.
void CClientManager::MyshopPricelistRequest(LPPEER pPeer, uint32_t dwHandle, uint32_t dwPlayerID)
{
	if (CItemPriceListTableCache* pItemPricelistCache = GetItemPriceListCache(dwPlayerID))
	{
		TraceLog("Cache MyShopPricelist handle[{}] pid[{}]", dwHandle, dwPlayerID);

		TItemPriceListTable* pTable = pItemPricelistCache->Get(false);

		TPacketMyshopPricelistHeader header =
		{
			pTable->dwOwnerID,
			pTable->byCount
		};

		size_t sizePriceListSize = sizeof(TItemPriceInfo)* pTable->byCount;

		pPeer->EncodeHeader(HEADER_DG_MYSHOP_PRICELIST_RES, dwHandle, sizeof(header) + sizePriceListSize);
		pPeer->Encode(&header, sizeof(header));
		pPeer->Encode(pTable->aPriceInfo, sizePriceListSize);

	}
	else
	{
		TraceLog("Query MyShopPricelist handle[{}] pid[{}]", dwHandle, dwPlayerID);

		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT item_vnum, price FROM myshop_pricelist%s WHERE owner_id=%u", GetTablePostfix(), dwPlayerID);
		CDBManager::GetInstance()->ReturnQuery(szQuery, QID_ITEMPRICE_LOAD, pPeer->GetHandle(), new TItemPricelistReqInfo(dwHandle, dwPlayerID));
	}
}

void CPacketInfo::Add(int32_t header)
{
	auto it = m_map_info.find(header);

	if (it == m_map_info.end())
		m_map_info.emplace(header, 1);
	else
		++it->second;
}

void CPacketInfo::Reset()
{
	m_map_info.clear();
}

void CClientManager::ProcessPackets(LPPEER pPeer)
{
	uint8_t		header;
	uint32_t		dwHandle;
	uint32_t		dwLength;
	const char* data = nullptr;
	int32_t			i = 0;
	int32_t			iCount = 0;

	while (pPeer->PeekPacket(i, header, dwHandle, dwLength, &data))
	{
		m_bLastHeader = header;
		++iCount;


		if (header != 10)
		{
			TraceLog(" ProcessPacket Header [{}] Handle[{}] Length[{}] iCount[{}]", header, dwHandle, dwLength, iCount);
		}

		switch (header)
		{
			case HEADER_GD_BOOT:
				QUERY_BOOT(pPeer, (TPacketGDBoot*) data);
				break;

			case HEADER_GD_HAMMER_OF_TOR:
				break;

			case HEADER_GD_LOGIN_BY_KEY:
				QUERY_LOGIN_BY_KEY(pPeer, dwHandle, (TPacketGDLoginByKey*) data);
				break;

			case HEADER_GD_LOGOUT:
				QUERY_LOGOUT(pPeer, dwHandle, data);
				break;

			case HEADER_GD_PLAYER_LOAD:
				TraceLog("HEADER_GD_PLAYER_LOAD (handle: {} length: {})", dwHandle, dwLength);
				QUERY_PLAYER_LOAD(pPeer, dwHandle, (TPlayerLoadPacket*) data);
				break;

			case HEADER_GD_PLAYER_SAVE:
				TraceLog("HEADER_GD_PLAYER_SAVE (handle: {} length: {})", dwHandle, dwLength);
				QUERY_PLAYER_SAVE(pPeer, dwHandle, (TPlayerTable*) data);
				break;

			case HEADER_GD_PLAYER_CREATE:
				TraceLog("HEADER_GD_PLAYER_CREATE (handle: {} length: {})", dwHandle, dwLength);
				__QUERY_PLAYER_CREATE(pPeer, dwHandle, (TPlayerCreatePacket*) data);
				break;

			case HEADER_GD_PLAYER_DELETE:
				TraceLog("HEADER_GD_PLAYER_DELETE (handle: {} length: {})", dwHandle, dwLength);
				__QUERY_PLAYER_DELETE(pPeer, dwHandle, (TPlayerDeletePacket*) data);
				break;

			case HEADER_GD_PLAYER_COUNT:
				QUERY_PLAYER_COUNT(pPeer, (TPlayerCountPacket*) data);
				break;

			case HEADER_GD_QUEST_SAVE:
				TraceLog("HEADER_GD_QUEST_SAVE (handle: {} length: {})", dwHandle, dwLength);
				QUERY_QUEST_SAVE(pPeer, (TQuestTable*) data, dwLength);
				break;

			case HEADER_GD_SAFEBOX_LOAD:
				QUERY_SAFEBOX_LOAD(pPeer, dwHandle, (TSafeboxLoadPacket*) data, false);
				break;

			case HEADER_GD_SAFEBOX_SAVE:
				TraceLog("HEADER_GD_SAFEBOX_SAVE (handle: {} length: {})", dwHandle, dwLength);
				QUERY_SAFEBOX_SAVE(pPeer, (TSafeboxTable*) data);
				break;

			case HEADER_GD_SAFEBOX_CHANGE_SIZE:
				QUERY_SAFEBOX_CHANGE_SIZE(pPeer, dwHandle, (TSafeboxChangeSizePacket*) data);
				break;

			case HEADER_GD_SAFEBOX_CHANGE_PASSWORD:
				QUERY_SAFEBOX_CHANGE_PASSWORD(pPeer, dwHandle, (TSafeboxChangePasswordPacket*) data);
				break;

			case HEADER_GD_MALL_LOAD:
				QUERY_SAFEBOX_LOAD(pPeer, dwHandle, (TSafeboxLoadPacket*) data, true);
				break;

			case HEADER_GD_EMPIRE_SELECT:
				QUERY_EMPIRE_SELECT(pPeer, dwHandle, (TEmpireSelectPacket*) data);
				break;

			case HEADER_GD_SETUP:
				QUERY_SETUP(pPeer, dwHandle, data);
				break;

			case HEADER_GD_GUILD_CREATE:
				GuildCreate(pPeer, *(uint32_t*) data);
				break;

			case HEADER_GD_GUILD_SKILL_UPDATE:
				GuildSkillUpdate(pPeer, (TPacketGuildSkillUpdate*) data);		
				break;

			case HEADER_GD_GUILD_EXP_UPDATE:
				GuildExpUpdate(pPeer, (TPacketGuildExpUpdate*) data);
				break;

			case HEADER_GD_GUILD_ADD_MEMBER:
				GuildAddMember(pPeer, (TPacketGDGuildAddMember*) data);
				break;

			case HEADER_GD_GUILD_REMOVE_MEMBER:
				GuildRemoveMember(pPeer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_GRADE:
				GuildChangeGrade(pPeer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_MEMBER_DATA:
				GuildChangeMemberData(pPeer, (TPacketGuildChangeMemberData*) data);
				break;

			case HEADER_GD_GUILD_DISBAND:
				GuildDisband(pPeer, (TPacketGuild*) data);
				break;

			case HEADER_GD_GUILD_WAR:
				GuildWar(pPeer, (TPacketGuildWar*) data);
				break;

			case HEADER_GD_GUILD_WAR_SCORE:
				GuildWarScore(pPeer, (TPacketGuildWarScore*) data);
				break;

			case HEADER_GD_GUILD_CHANGE_LADDER_POINT:
				GuildChangeLadderPoint((TPacketGuildLadderPoint*) data);
				break;

			case HEADER_GD_GUILD_USE_SKILL:
				GuildUseSkill((TPacketGuildUseSkill*) data);
				break;

			case HEADER_GD_FLUSH_CACHE:
				QUERY_FLUSH_CACHE(pPeer, data);
				break;

			case HEADER_GD_ITEM_SAVE:
				QUERY_ITEM_SAVE(pPeer, data);
				break;

			case HEADER_GD_ITEM_DESTROY:
				QUERY_ITEM_DESTROY(pPeer, data);
				break;

			case HEADER_GD_ITEM_FLUSH:
				QUERY_ITEM_FLUSH(pPeer, data);
				break;

			case HEADER_GD_ADD_AFFECT:
				TraceLog("HEADER_GD_ADD_AFFECT");
				QUERY_ADD_AFFECT(pPeer, (TPacketGDAddAffect*) data);
				break;

			case HEADER_GD_REMOVE_AFFECT:
				TraceLog("HEADER_GD_REMOVE_AFFECT");
				QUERY_REMOVE_AFFECT(pPeer, (TPacketGDRemoveAffect*) data);
				break;

			case HEADER_GD_HIGHSCORE_REGISTER:
				QUERY_HIGHSCORE_REGISTER(pPeer, (TPacketGDHighscore*) data);
				break;

			case HEADER_GD_PARTY_CREATE:
				QUERY_PARTY_CREATE(pPeer, (TPacketPartyCreate*) data);
				break;

			case HEADER_GD_PARTY_DELETE:
				QUERY_PARTY_DELETE(pPeer, (TPacketPartyDelete*) data);
				break;

			case HEADER_GD_PARTY_ADD:
				QUERY_PARTY_ADD(pPeer, (TPacketPartyAdd*) data);
				break;

			case HEADER_GD_PARTY_REMOVE:
				QUERY_PARTY_REMOVE(pPeer, (TPacketPartyRemove*) data);
				break;

			case HEADER_GD_PARTY_STATE_CHANGE:
				QUERY_PARTY_STATE_CHANGE(pPeer, (TPacketPartyStateChange*) data);
				break;

			case HEADER_GD_PARTY_SET_MEMBER_LEVEL:
				QUERY_PARTY_SET_MEMBER_LEVEL(pPeer, (TPacketPartySetMemberLevel*) data);
				break;

			case HEADER_GD_RELOAD_PROTO:
				QUERY_RELOAD_PROTO();
				break;

			case HEADER_GD_CHANGE_NAME:
				QUERY_CHANGE_NAME(pPeer, dwHandle, (TPacketGDChangeName*) data);
				break;

			case HEADER_GD_AUTH_LOGIN:
				QUERY_AUTH_LOGIN(pPeer, dwHandle, (TPacketGDAuthLogin*) data);
				break;

			case HEADER_GD_REQUEST_GUILD_PRIV:
				AddGuildPriv((TPacketGiveGuildPriv*)data);
				break;

			case HEADER_GD_REQUEST_EMPIRE_PRIV:
				AddEmpirePriv((TPacketGiveEmpirePriv*)data);
				break;

			case HEADER_GD_REQUEST_CHARACTER_PRIV:
				AddCharacterPriv((TPacketGiveCharacterPriv*) data);
				break;

			case HEADER_GD_MONEY_LOG:
				MoneyLog((TPacketMoneyLog*)data);
				break;

			case HEADER_GD_GUILD_DEPOSIT_MONEY:
				GuildDepositMoney((TPacketGDGuildMoney*)data);
				break;

			case HEADER_GD_GUILD_WITHDRAW_MONEY:
				GuildWithdrawMoney(pPeer, (TPacketGDGuildMoney*)data);
				break;

			case HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY:
				GuildWithdrawMoneyGiveReply((TPacketGDGuildMoneyWithdrawGiveReply*)data);
				break;

			case HEADER_GD_GUILD_WAR_BET:
				GuildWarBet((TPacketGDGuildWarBet*) data);
				break;

			case HEADER_GD_SET_EVENT_FLAG:
				SetEventFlag((TPacketSetEventFlag*) data);
				break;

			case HEADER_GD_CREATE_OBJECT:
				CreateObject((TPacketGDCreateObject*) data);
				break;

			case HEADER_GD_DELETE_OBJECT:
				DeleteObject(*(uint32_t*) data);
				break;

			case HEADER_GD_UPDATE_LAND:
				UpdateLand((uint32_t*) data);
				break;

			case HEADER_GD_MARRIAGE_ADD:
				MarriageAdd((TPacketMarriageAdd*) data);
				break;

			case HEADER_GD_MARRIAGE_UPDATE:
				MarriageUpdate((TPacketMarriageUpdate*) data);
				break;

			case HEADER_GD_MARRIAGE_REMOVE:
				MarriageRemove((TPacketMarriageRemove*) data);
				break;

			case HEADER_GD_WEDDING_REQUEST:
				WeddingRequest((TPacketWeddingRequest*) data);
				break;

			case HEADER_GD_WEDDING_READY:
				WeddingReady((TPacketWeddingReady*) data);
				break;

			case HEADER_GD_WEDDING_END:
				WeddingEnd((TPacketWeddingEnd*) data);
				break;

			case HEADER_GD_BLOCK_CHAT:
				BlockChat((TPacketBlockChat*) data);
				break;

			case HEADER_GD_MYSHOP_PRICELIST_UPDATE:
				MyshopPricelistUpdate((TPacketMyshopPricelistHeader*)data);
				break;

			case HEADER_GD_MYSHOP_PRICELIST_REQ:
				MyshopPricelistRequest(pPeer, dwHandle, *(uint32_t*)data);
				break;

			case HEADER_GD_RELOAD_ADMIN:
				ReloadAdmin(pPeer, (TPacketReloadAdmin*)data);
				break;

			case HEADER_GD_BREAK_MARRIAGE:
				BreakMarriage(pPeer, data);
				break;

			case HEADER_GD_ELECT_MONARCH:
				Election(pPeer, dwHandle, data);
				break;

			case HEADER_GD_CANDIDACY:
				Candidacy(pPeer, dwHandle, data);
				break;

			case HEADER_GD_ADD_MONARCH_MONEY:
				AddMonarchMoney(pPeer, dwHandle, data);
				break;

			case HEADER_GD_DEC_MONARCH_MONEY:
				DecMonarchMoney(pPeer, dwHandle, data);
				break;

			case HEADER_GD_TAKE_MONARCH_MONEY:
				TakeMonarchMoney(pPeer, dwHandle, data);
				break;

			case HEADER_GD_COME_TO_VOTE:
				ComeToVote(pPeer, dwHandle, data);
				break;

			case HEADER_GD_RMCANDIDACY:
				RMCandidacy(pPeer, dwHandle, data);
				break;

			case HEADER_GD_SETMONARCH:
				SetMonarch(pPeer, dwHandle, data);
				break;

			case HEADER_GD_RMMONARCH:
				RMMonarch(pPeer, dwHandle, data);
				break;

			case HEADER_GD_CHANGE_MONARCH_LORD :
				ChangeMonarchLord(pPeer, dwHandle, (TPacketChangeMonarchLord*)data);
				break;

			case HEADER_GD_REQ_SPARE_ITEM_ID_RANGE :
				SendSpareItemIDRange(pPeer);
				break;

			case HEADER_GD_REQ_CHANGE_GUILD_MASTER :
				GuildChangeMaster((TPacketChangeGuildMaster*) data);
				break;

			case HEADER_GD_UPDATE_HORSE_NAME :
				UpdateHorseName((TPacketUpdateHorseName*) data, pPeer);
				break;

			case HEADER_GD_REQ_HORSE_NAME :
				AckHorseName(*(uint32_t*)data, pPeer);
				break;

			case HEADER_GD_DC:
				DeleteLoginKey((TPacketDC*) data);
				break;

			case HEADER_GD_VALID_LOGOUT:
				ResetLastPlayerID((TPacketNeedLoginLogInfo*)data);
				break;

			case HEADER_GD_REQUEST_CHARGE_CASH:
				ChargeCash((TRequestChargeCash*)data);
				break;

			case HEADER_GD_UPDATE_CHANNELSTATUS:
				UpdateChannelStatus((SChannelStatus*) data);
				break;
			case HEADER_GD_REQUEST_CHANNELSTATUS:
				RequestChannelStatus(pPeer, dwHandle);
				break;

			default:					
				SysLog("Unknown header (header: {} handle: {} length: {})", header, dwHandle, dwLength);
				break;
		}
	}

	pPeer->RecvEnd(i);
}

void CClientManager::AddPeer(socket_t fd)
{
	std::unique_ptr<CPeer> upPeer = std::make_unique<CPeer>();

	if (upPeer->Accept(fd))
		m_peerList.push_front(std::move(upPeer));
}

void CClientManager::RemovePeer(LPPEER pPeer)
{
	if (m_pAuthPeer == pPeer)
	{
		m_pAuthPeer = nullptr;
	}
	else
	{
		auto it = m_map_kLogonAccount.begin();

		while (it != m_map_kLogonAccount.end())
		{
			CLoginData* pLD = it->second;

			if (pLD->GetConnectedPeerHandle() == pPeer->GetHandle())
			{
				if (pLD->IsPlay())
				{
					pLD->SetPlay(false);
				}

				if (pLD->IsDeleted())
				{
					TraceLog("DELETING LoginData");
					delete pLD;
				}

				m_map_kLogonAccount.erase(it++);
			}
			else
				++it;
		}
	}
	
	for (const auto& upPeer : m_peerList)
	{
		if (upPeer.get() == pPeer)
		{
			m_peerList.remove(upPeer);
			break;
		}
	}
}

LPPEER CClientManager::GetPeer(IDENT ident)
{
	for (const auto& upPeer : m_peerList)
	{
		if (upPeer->GetHandle() == ident)
			return upPeer.get();
	}

	return nullptr;
}

LPPEER CClientManager::GetAnyPeer()
{
	if (m_peerList.empty())
		return nullptr;

	return m_peerList.front().get();
}

// Process the result received from the db manager.
int32_t CClientManager::AnalyzeQueryResult(SQLMsg* msg)
{
	CQueryInfo* qi = (CQueryInfo*) msg->pvUserData;
	LPPEER pPeer = GetPeer(qi->dwIdent);

	switch (qi->iType)
	{
		case QID_GUILD_RANKING:
			CGuildManager::GetInstance()->ResultRanking(msg->Get()->pSQLResult);
			break;
			
		case QID_ITEMPRICE_LOAD_FOR_UPDATE:
			RESULT_PRICELIST_LOAD_FOR_UPDATE(msg);
			break;
	}

	if (!pPeer)
	{	
		// (Soni) TODO: Replace such queries with AsyncQuery
		SysLog("CClientManager::AnalyzeQueryResult: Unknown peer (ident: {}, type: {}, str: {})", qi->dwIdent, qi->iType, msg->stQuery);
		delete qi;
		return true;
	}

	switch (qi->iType)
	{
		case QID_PLAYER:
		case QID_ITEM:
		case QID_QUEST:
		case QID_AFFECT:
			RESULT_COMPOSITE_PLAYER(pPeer, msg, qi->iType);
			break;

		case QID_LOGIN:
			RESULT_LOGIN(pPeer, msg);
			break;

		case QID_SAFEBOX_LOAD:
			TraceLog("QUERY_RESULT: HEADER_GD_SAFEBOX_LOAD");
			RESULT_SAFEBOX_LOAD(pPeer, msg);
			break;

		case QID_SAFEBOX_CHANGE_SIZE:
			TraceLog("QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_SIZE");
			RESULT_SAFEBOX_CHANGE_SIZE(pPeer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD:
			TraceLog("QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD");
			RESULT_SAFEBOX_CHANGE_PASSWORD(pPeer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD_SECOND:
			TraceLog("QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD");
			RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(pPeer, msg);
			break;

		case QID_HIGHSCORE_REGISTER:
			TraceLog("QUERY_RESULT: HEADER_GD_HIGHSCORE_REGISTER");
			RESULT_HIGHSCORE_REGISTER(pPeer, msg);
			break;

		case QID_SAFEBOX_SAVE:
		case QID_ITEM_SAVE:
		case QID_ITEM_DESTROY:
		case QID_QUEST_SAVE:
		case QID_PLAYER_SAVE:
			break;

		case QID_PLAYER_INDEX_CREATE:
			RESULT_PLAYER_INDEX_CREATE(pPeer, msg);
			break;

		case QID_PLAYER_DELETE:
			__RESULT_PLAYER_DELETE(pPeer, msg);
			break;

		case QID_LOGIN_BY_KEY:
			RESULT_LOGIN_BY_KEY(pPeer, msg);
			break;

		case QID_ITEMPRICE_LOAD:
			RESULT_PRICELIST_LOAD(pPeer, msg);
			break;

		default:
			WarnLog("CClientManager::AnalyzeQueryResult unknown query result type: {}, str: {}", qi->iType, msg->stQuery.c_str());
			break;
	}

	delete qi;
	return true;
}

void UsageLog()
{   
	FILE* fp = nullptr;

	time_t      ct;
	char        *time_s;
	struct tm   lt;

	int32_t         avg = g_dwUsageAvg / 3600;

	fp = fopen("usage.txt", "a+");

	if (!fp)
		return;

	ct = time(nullptr);
	lt = *localtime(&ct);
	time_s = asctime(&lt);

	time_s[strlen(time_s) - 1] = '\0';

	fprintf(fp, "| %4d %-15.15s | %5d | %5u |", lt.tm_year + 1900, time_s + 4, avg, g_dwUsageMax);

	fprintf(fp, "\n");
	fclose(fp);

	g_dwUsageMax = g_dwUsageAvg = 0;
}

int32_t CClientManager::Process()
{
	int32_t pulses;

	if (!(pulses = thecore_idle()))
		return 0;

	while (pulses--)
	{
		++thecore_heart->pulse;

		if (!(thecore_heart->pulse % thecore_heart->passes_per_sec))
		{
			CDBManager::GetInstance()->ResetCounter();

			uint32_t dwCount = CClientManager::GetInstance()->GetUserCount();

			g_dwUsageAvg += dwCount;
			g_dwUsageMax = MAX(g_dwUsageMax, dwCount);

			memset(&thecore_profiler[0], 0, sizeof(thecore_profiler));

			if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 3600)))
				UsageLog();

			m_iCacheFlushCount = 0;

			UpdatePlayerCache();
			UpdateItemCache();
			UpdateLogoutPlayer();
			UpdateItemPriceListCache();

			CGuildManager::GetInstance()->Update();
			CPrivManager::GetInstance()->Update();
			CMarriageManager::GetInstance()->Update();
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 10)))
		{
			TraceLog("QUERY: MAIN[%d] ASYNC[%d]", g_query_count[0], g_query_count[1]);
			g_query_count[0] = 0;
			g_query_count[1] = 0;

			TraceLog("ITEM:%d\n", g_item_count);
			g_item_count = 0;
		}

		// Spend time for unique items.
		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 60)))
			CClientManager::GetInstance()->SendTime();

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 3600)))
			CMoneyLog::GetInstance()->Save();
	}

	int32_t num_events = fdwatch(m_fdWatcher, nullptr);
	int32_t idx;
	LPPEER pPeer;

	for (idx = 0; idx < num_events; ++idx)
	{
		pPeer = (LPPEER) fdwatch_get_client_data(m_fdWatcher, idx);

		if (!pPeer)
		{
			if (fdwatch_check_event(m_fdWatcher, m_fdAccept, idx) == FDW_READ)
			{
				AddPeer(m_fdAccept);
				fdwatch_clear_event(m_fdWatcher, m_fdAccept, idx);
			}
			else
			{
				SysLog("FDWATCH: pPeer null in event: ident {}", fdwatch_get_ident(m_fdWatcher, idx));
			}

			continue;
		}

		switch (fdwatch_check_event(m_fdWatcher, pPeer->GetFd(), idx))
		{
			case FDW_READ:
				if (pPeer->Recv() < 0)
				{
					SysLog("Recv failed");
					RemovePeer(pPeer);
				}
				else
				{
					if (pPeer == m_pAuthPeer)
					{
						TraceLog("AUTH_PEER_READ: size {}", pPeer->GetRecvLength());
					}

					ProcessPackets(pPeer);
				}
				break;

			case FDW_WRITE:
				if (pPeer == m_pAuthPeer)
					TraceLog("AUTH_PEER_WRITE: size {}", pPeer->GetSendLength());

				if (pPeer->Send() < 0)
				{
					SysLog("Send failed");
					RemovePeer(pPeer);
				}

				break;

			case FDW_EOF:
				RemovePeer(pPeer);
				break;

			default:
				SysLog("fdwatch_check_fd returned unknown result");
				RemovePeer(pPeer);
				break;
		}
	}
	return 1;
}

uint32_t CClientManager::GetUserCount()
{
	return m_map_kLogonAccount.size();
}

void CClientManager::SendAllGuildSkillRechargePacket()
{
	ForwardPacket(HEADER_DG_GUILD_SKILL_RECHARGE, nullptr, 0);
}

void CClientManager::SendTime()
{
	time_t now = GetCurrentTime();
	ForwardPacket(HEADER_DG_TIME, &now, sizeof(time_t));
}

void CClientManager::ForwardPacket(uint8_t header, const void* data, int32_t size, uint8_t bChannel, LPPEER except)
{
	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (pPeer == except)
			continue;

		if (!pPeer->GetChannel())
			continue;

		if (bChannel && pPeer->GetChannel() != bChannel)
			continue;

		pPeer->EncodeHeader(header, 0, size);

		if (size > 0 && data)
			pPeer->Encode(data, size);
	}
}

void CClientManager::SendNotice(const char* c_pszFormat, ...)
{
	char szBuf[255+1];
	va_list args;

	va_start(args, c_pszFormat);
	int32_t len = vsnprintf(szBuf, sizeof(szBuf), c_pszFormat, args);
	va_end(args);
	szBuf[len] = '\0';

	ForwardPacket(HEADER_DG_NOTICE, szBuf, len + 1);
}

time_t CClientManager::GetCurrentTime()
{
	return time(nullptr);
}

bool CClientManager::InitializeNowItemID()
{
	uint32_t dwMin, dwMax;

	// Initialize the item ID.
	if (!CConfig::GetInstance()->GetTwoValue("ITEM_ID_RANGE", &dwMin, &dwMax))
	{
		SysLog("CONFIG: Cannot find ITEM_ID_RANGE [start_item_id] [end_item_id]");
		return false;
	}

	TraceLog("ItemRange From File {} ~ {} ", dwMin, dwMax);
	
	if (CItemIDRangeManager::GetInstance()->BuildRange(dwMin, dwMax, m_itemRange) == false)
	{
		SysLog("Can not build ITEM_ID_RANGE");
		return false;
	}
	
	TraceLog(" Init Success Start {} End {} Now {}\n", m_itemRange.dwMin, m_itemRange.dwMax, m_itemRange.dwUsableItemIDMin);

	return true;
}

uint32_t CClientManager::GainItemID()
{
	return m_itemRange.dwUsableItemIDMin++;
}

uint32_t CClientManager::GetItemID()
{
	return m_itemRange.dwUsableItemIDMin;
}

bool CClientManager::InitializeLocalization() 
{
	char szQuery[512];	
	snprintf(szQuery, sizeof(szQuery), "SELECT mValue, mKey FROM locale");
	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_COMMON));

	if (pSQLMsg->Get()->uiNumRows == 0)
	{
		SysLog("InitializeLocalization() ==> DirectQuery failed({})", szQuery);
		return false;
	}

	TraceLog("InitializeLocalization() - LoadLocaleTable(count:{})", pSQLMsg->Get()->uiNumRows);

	m_vec_Locale.clear();

	MYSQL_ROW row = nullptr;

	for (int32_t n = 0; (row = mysql_fetch_row(pSQLMsg->Get()->pSQLResult)) != nullptr; ++n)
	{
		int32_t col = 0;
		tLocale locale;

		strlcpy(locale.szValue, row[col++], sizeof(locale.szValue));
		strlcpy(locale.szKey, row[col++], sizeof(locale.szKey));
	
		if (strcmp(locale.szKey, "LOCALE") == 0)
		{
			if (strcmp(locale.szValue, "uk") == 0)
			{
				TraceLog("locale[LOCALE] = {}", locale.szValue);

				if (g_stLocale != locale.szValue)
				{
					TraceLog("Changed g_stLocale {} to {}", g_stLocale, "euckr");
				}

				g_stLocale = "latin1";
				g_stLocaleNameColumn = "locale_name";
			}
			else
			{
				FatalLog("locale[LOCALE] = UNKNOWN({})", locale.szValue);
				exit(0);
			}

			CDBManager::GetInstance()->SetLocale(g_stLocale.c_str());
		}
		else if (strcmp(locale.szKey, "DB_NAME_COLUMN") == 0)
		{
			TraceLog("locale[DB_NAME_COLUMN] = {}", locale.szValue);
			g_stLocaleNameColumn = locale.szValue;	
		}
		else
		{
			TraceLog("locale[UNKNOWN_KEY({})] = {}", locale.szKey, locale.szValue);
		}
		m_vec_Locale.push_back(locale);
	}

	return true;
}

bool CClientManager::__GetAdminInfo(const char* szIP, std::vector<tAdminInfo>& rAdminVec)
{
	// If szIP == nullptr, all servers have operator rights.
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT mID,mAccount,mName,mContactIP,mServerIP,mAuthority FROM gmlist WHERE mServerIP='ALL' or mServerIP='%s'",
		   	szIP ? szIP : "ALL");

	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_COMMON));

	if (pSQLMsg->Get()->uiNumRows == 0)
	{
		SysLog("__GetAdminInfo() ==> DirectQuery failed({})", szQuery);
		return false;
	}

	MYSQL_ROW row;
	rAdminVec.reserve(pSQLMsg->Get()->uiNumRows);

	while ((row = mysql_fetch_row(pSQLMsg->Get()->pSQLResult)))
	{
		int32_t idx = 0;
		tAdminInfo Info;

		str_to_number(Info.m_ID, row[idx++]);
		trim_and_lower(row[idx++], Info.m_szAccount, sizeof(Info.m_szAccount));
		strlcpy(Info.m_szName, row[idx++], sizeof(Info.m_szName));
		strlcpy(Info.m_szContactIP, row[idx++], sizeof(Info.m_szContactIP));
		strlcpy(Info.m_szServerIP, row[idx++], sizeof(Info.m_szServerIP));
		std::string stAuth = row[idx++];

		if (!stAuth.compare("IMPLEMENTOR"))
			Info.m_Authority = GM_IMPLEMENTOR;
		else if (!stAuth.compare("GOD"))
			Info.m_Authority = GM_GOD; 
		else if (!stAuth.compare("HIGH_WIZARD"))
			Info.m_Authority = GM_HIGH_WIZARD;
		else if (!stAuth.compare("LOW_WIZARD")) 
			Info.m_Authority = GM_LOW_WIZARD;
		else if (!stAuth.compare("WIZARD"))
			Info.m_Authority = GM_WIZARD;
		else 
			continue;

		rAdminVec.push_back(Info);

		TraceLog("GM: PID {} Login {} Character {} ContactIP {} ServerIP {} Authority {}[{}]",
			   	Info.m_ID, Info.m_szAccount, Info.m_szName, Info.m_szContactIP, Info.m_szServerIP, Info.m_Authority, stAuth.c_str());
	}

	return true;
}

bool CClientManager::__GetHostInfo(std::vector<std::string>& rIPVec)
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "SELECT mIP FROM gmhost");
	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_COMMON));

	if (pSQLMsg->Get()->uiNumRows == 0)
	{
		SysLog("__GetHostInfo() ==> DirectQuery failed({})", szQuery);
		return false;
	}

	rIPVec.reserve(pSQLMsg->Get()->uiNumRows);

	MYSQL_ROW row; 

	while ((row = mysql_fetch_row(pSQLMsg->Get()->pSQLResult)))
	{
		if (row[0] && *row[0])
		{
			rIPVec.emplace_back(row[0]);
			TraceLog("GMHOST: {}", row[0]);
		}
	}

	return true;
}

void CClientManager::ReloadAdmin(LPPEER pGuestPeer, TPacketReloadAdmin* p)
{
	std::vector<tAdminInfo> vAdmin;
	std::vector<std::string> vHost;
	
	__GetHostInfo(vHost);
	__GetAdminInfo(p->szIP, vAdmin);

	uint32_t dwPacketSize = sizeof(uint16_t) + sizeof (uint16_t) + sizeof(tAdminInfo) * vAdmin.size() + 
		  sizeof(uint16_t) + sizeof(uint16_t) + 16 * vHost.size();	

	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (!pPeer->GetChannel())
			continue;

		pPeer->EncodeHeader(HEADER_DG_RELOAD_ADMIN, 0, dwPacketSize);

		pPeer->EncodeWORD(16);
		pPeer->EncodeWORD(vHost.size());

		for (const auto& strHost : vHost)
			pPeer->Encode(strHost.c_str(), 16);

		pPeer->EncodeWORD(sizeof(tAdminInfo));
		pPeer->EncodeWORD(vAdmin.size());

		for (const auto& TAdmin : vAdmin)
			pPeer->Encode(&TAdmin, sizeof(tAdminInfo));
	}

	TraceLog("ReloadAdmin End {}", p->szIP);
}

void CClientManager::BreakMarriage(LPPEER pPeer, const char* data)
{
	uint32_t pid1, pid2;

	pid1 = *(int32_t*) data;
	data += sizeof(int32_t);

	pid2 = *(int32_t*) data;
	data += sizeof(int32_t);

	TraceLog("Breaking off a marriage engagement! pid {} and pid {}", pid1, pid2);
	CMarriageManager::GetInstance()->Remove(pid1, pid2);
}

void CClientManager::UpdateItemCacheSet(uint32_t pid)
{
	auto it = m_map_pItemCacheSetPtr.find(pid);

	if (it == m_map_pItemCacheSetPtr.end())
	{
		TraceLog("UPDATE_ITEMCACHESET : UpdateItemCacheSet ==> No ItemCacheSet pid({})", pid);
		return;
	}

	TItemCacheSet* pItemSet = it->second.get();
	auto it_set = pItemSet->begin();

	while (it_set != pItemSet->end())
	{
		CItemCache* pItemCache = *it_set++;
		pItemCache->Flush();
	}

	TraceLog("UPDATE_ITEMCACHESET : UpdateItemCachsSet pid({})", pid);
}

void CClientManager::Election(LPPEER pPeer, uint32_t dwHandle, const char* data)
{
	uint32_t idx;
	uint32_t selectingpid;

	idx = *(uint32_t*) data;
	data += sizeof(uint32_t);

	selectingpid = *(uint32_t*) data;
	data += sizeof(uint32_t);

	int32_t Success = 0;

	if (!(Success = CMonarch::GetInstance()->VoteMonarch(selectingpid, idx)))
	{
		TraceLog("[MONARCH_VOTE] Failed {} {}", idx, selectingpid);
		pPeer->EncodeHeader(HEADER_DG_ELECT_MONARCH, dwHandle, sizeof(int32_t));
		pPeer->Encode(&Success, sizeof(int32_t));
		return;
	}
	else
	{
		TraceLog("[MONARCH_VOTE] Success {} {}", idx, selectingpid);
		pPeer->EncodeHeader(HEADER_DG_ELECT_MONARCH, dwHandle, sizeof(int32_t));
		pPeer->Encode(&Success, sizeof(int32_t));
		return;
	}

}
void CClientManager::Candidacy(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	uint32_t pid;

	pid = *(uint32_t*) data;
	data += sizeof(uint32_t);

	if (!CMonarch::GetInstance()->AddCandidacy(pid, data))
	{
		TraceLog("[MONARCH_CANDIDACY] Failed {} {}", pid, data);

		pTargetPeer->EncodeHeader(HEADER_DG_CANDIDACY, dwHandle, sizeof(int32_t) + 32);
		pTargetPeer->Encode(nullptr, sizeof(int32_t));
		pTargetPeer->Encode(data, 32);
		return;
	}
	else
	{
		TraceLog("[MONARCH_CANDIDACY] Success {} {}", pid, data);

		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (!pPeer->GetChannel())
				continue;

			if (pPeer->GetChannel() != 0)
				continue;

			if (pPeer == pTargetPeer)
			{	
				pPeer->EncodeHeader(HEADER_DG_CANDIDACY, dwHandle, sizeof(int32_t) + 32);
				pPeer->Encode(&pid, sizeof(int32_t));
				pPeer->Encode(data, 32);
			}
			else
			{
				pPeer->EncodeHeader(HEADER_DG_CANDIDACY, 0, sizeof(int32_t) + 32);
				pPeer->Encode(&pid, sizeof(int32_t));
				pPeer->Encode(data, 32);
			}
		}
	}
}

void CClientManager::AddMonarchMoney(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);

	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);

	TraceLog("[MONARCH] Add money Empire({}) Money({})", Empire, Money);

	CMonarch::GetInstance()->AddMoney(Empire, Money);
	
	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (!pPeer->GetChannel())
			continue;

		if (pPeer == pTargetPeer)
		{	
			pPeer->EncodeHeader(HEADER_DG_ADD_MONARCH_MONEY, dwHandle, sizeof(int32_t) + sizeof(int32_t));
			pPeer->Encode(&Empire, sizeof(int32_t));
			pPeer->Encode(&Money, sizeof(int32_t));
		}
		else
		{
			pPeer->EncodeHeader(HEADER_DG_ADD_MONARCH_MONEY, 0, sizeof(int32_t) + sizeof(int32_t));
			pPeer->Encode(&Empire, sizeof(int32_t));
			pPeer->Encode(&Money, sizeof(int32_t));
		}

	}
}
void CClientManager::DecMonarchMoney(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);

	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);
		
	TraceLog("[MONARCH] Dec money Empire({}) Money({})", Empire, Money);

	CMonarch::GetInstance()->DecMoney(Empire, Money);
	
	for (const auto& upPeer : m_peerList)
	{
		LPPEER pPeer = upPeer.get();

		if (!pPeer->GetChannel())
			continue;

		if (pPeer == pTargetPeer)
		{	
			pPeer->EncodeHeader(HEADER_DG_DEC_MONARCH_MONEY, dwHandle, sizeof(int32_t) + sizeof(int32_t));
			pPeer->Encode(&Empire, sizeof(int32_t));
			pPeer->Encode(&Money, sizeof(int32_t));
		}
		else
		{
			pPeer->EncodeHeader(HEADER_DG_DEC_MONARCH_MONEY, 0, sizeof(int32_t) + sizeof(int32_t));
			pPeer->Encode(&Empire, sizeof(int32_t));
			pPeer->Encode(&Money, sizeof(int32_t));
		}
	}
}

void CClientManager::TakeMonarchMoney(LPPEER pPeer, uint32_t dwHandle, const char* data)
{
	int32_t Empire = *(int32_t*) data;
	data += sizeof(int32_t);

	uint32_t pid = *(uint32_t*) data;
	data += sizeof(int32_t);

	int32_t Money = *(int32_t*) data;
	data += sizeof(int32_t);

	TraceLog("[MONARCH] Take money Empire({}) Money({})", Empire, Money);

	if (CMonarch::GetInstance()->TakeMoney(Empire, pid, Money))
	{
		pPeer->EncodeHeader(HEADER_DG_TAKE_MONARCH_MONEY, dwHandle, sizeof(int32_t) + sizeof(int32_t));
		pPeer->Encode(&Empire, sizeof(int32_t));
		pPeer->Encode(&Money, sizeof(int32_t));
	}
	else
	{
		Money = 0;
		pPeer->EncodeHeader(HEADER_DG_TAKE_MONARCH_MONEY, dwHandle, sizeof(int32_t) + sizeof(int32_t));
		pPeer->Encode(&Empire, sizeof(int32_t));
		pPeer->Encode(&Money, sizeof(int32_t));
	}
}

void CClientManager::ComeToVote(LPPEER pPeer, uint32_t dwHandle, const char* data)
{
	CMonarch::GetInstance()->ElectMonarch();	
}

void CClientManager::RMCandidacy(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	char szName[32];
	strlcpy(szName, data, sizeof(szName));

	TraceLog("[MONARCH_GM] Remove candidacy name({})", szName);

	int32_t iRet = CMonarch::GetInstance()->DelCandidacy(szName) ? 1 : 0;

	if (iRet)
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (!pPeer->GetChannel())
				continue;

			if (pPeer == pTargetPeer)
			{
				pPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
			else
			{
				pPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		pTargetPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
		pTargetPeer->Encode(&iRet, sizeof(int32_t));
		pTargetPeer->Encode(szName, sizeof(szName));
	}
}

void CClientManager::SetMonarch(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	char szName[32];
	strlcpy(szName, data, sizeof(szName));

	TraceLog("[MONARCH_GM] Set Monarch name({})", szName);
	
	int32_t iRet = CMonarch::GetInstance()->SetMonarch(szName) ? 1 : 0;

	if (iRet)
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (!pPeer->GetChannel())
				continue;

			if (pPeer == pTargetPeer)
			{
				pPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
			else
			{
				pPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		pTargetPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
		pTargetPeer->Encode(&iRet, sizeof(int32_t));
		pTargetPeer->Encode(szName, sizeof(szName));
	}
}

void CClientManager::RMMonarch(LPPEER pTargetPeer, uint32_t dwHandle, const char* data)
{
	char szName[32];
	strlcpy(szName, data, sizeof(szName));
	
	TraceLog("[MONARCH_GM] Remove Monarch name({})", szName);
	
	CMonarch::GetInstance()->DelMonarch(szName);
	
	int32_t iRet = CMonarch::GetInstance()->DelMonarch(szName) ? 1 : 0;

	if (iRet)
	{
		for (const auto& upPeer : m_peerList)
		{
			LPPEER pPeer = upPeer.get();

			if (!pPeer->GetChannel())
				continue;

			if (pPeer == pTargetPeer)
			{
				pPeer->EncodeHeader(HEADER_DG_RMMONARCH, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
			else
			{
				pPeer->EncodeHeader(HEADER_DG_RMMONARCH, dwHandle, sizeof(int32_t) + sizeof(szName));
				pPeer->Encode(&iRet, sizeof(int32_t));
				pPeer->Encode(szName, sizeof(szName));
			}
		}
	}
	else
	{
		pTargetPeer->EncodeHeader(HEADER_DG_RMCANDIDACY, dwHandle, sizeof(int32_t) + sizeof(szName));
		pTargetPeer->Encode(&iRet, sizeof(int32_t));
		pTargetPeer->Encode(szName, sizeof(szName));
	}
}

void CClientManager::ChangeMonarchLord(LPPEER pPeer, uint32_t dwHandle, TPacketChangeMonarchLord* info)
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), 
			"SELECT a.name, NOW() FROM player%s AS a, player_index%s AS b WHERE (a.account_id=b.id AND a.id=%u AND b.empire=%u) AND "
		    "(b.pid1=%u OR b.pid2=%u OR b.pid3=%u OR b.pid4=%u)", 
			GetTablePostfix(), GetTablePostfix(), info->dwPID, info->bEmpire,
		   	info->dwPID, info->dwPID, info->dwPID, info->dwPID);

	std::unique_ptr<SQLMsg> pSQLMsg(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_PLAYER));

	if (pSQLMsg->Get()->uiNumRows != 0)
	{
		TPacketChangeMonarchLordACK ack;
		ack.bEmpire = info->bEmpire;
		ack.dwPID = info->dwPID;
		
		MYSQL_ROW row = mysql_fetch_row(pSQLMsg->Get()->pSQLResult);
		strlcpy(ack.szName, row[0], sizeof(ack.szName));
		strlcpy(ack.szDate, row[1], sizeof(ack.szDate));
		
		snprintf(szQuery, sizeof(szQuery), "UPDATE monarch SET pid=%u, windate=NOW() WHERE empire=%d", ack.dwPID, ack.bEmpire);
		std::unique_ptr<SQLMsg> pSQLMsg2(CDBManager::GetInstance()->DirectQuery(szQuery, SQL_PLAYER));

		if (pSQLMsg2->Get()->uiAffectedRows > 0)
		{
			CMonarch::GetInstance()->LoadMonarch();

			TMonarchInfo* newInfo = CMonarch::GetInstance()->GetMonarch();

			for (const auto& upPeer : m_peerList)
			{
				LPPEER pPeer = upPeer.get();

				pPeer->EncodeHeader(HEADER_DG_CHANGE_MONARCH_LORD_ACK, 0, sizeof(TPacketChangeMonarchLordACK));
				pPeer->Encode(&ack, sizeof(TPacketChangeMonarchLordACK));

				pPeer->EncodeHeader(HEADER_DG_UPDATE_MONARCH_INFO, 0, sizeof(TMonarchInfo));
				pPeer->Encode(newInfo, sizeof(TMonarchInfo));
			}
		}
	}
}

void CClientManager::SendSpareItemIDRange(LPPEER pPeer)
{
	pPeer->SendSpareItemIDRange();
}

void CClientManager::DeleteLoginKey(TPacketDC* data)
{
	char login[LOGIN_MAX_LEN+1] = {0};
	trim_and_lower(data->login, login, sizeof(login));

	CLoginData* pLD = GetLoginDataByLogin(login);

	if (pLD)
	{
		auto it = m_map_pLoginData.find(pLD->GetKey());

		if (it != m_map_pLoginData.end())
			m_map_pLoginData.erase(it);
	}
}

void CClientManager::UpdateChannelStatus(TChannelStatus* pData)
{
	auto it = m_mChannelStatus.find(pData->nPort);

	if (it != m_mChannelStatus.end()) 
		it->second = pData->bStatus;
	else
		m_mChannelStatus.emplace(pData->nPort, pData->bStatus);
}

void CClientManager::RequestChannelStatus(LPPEER pPeer, uint32_t dwHandle)
{
	const int32_t nSize = m_mChannelStatus.size();
	pPeer->EncodeHeader(HEADER_DG_RESPOND_CHANNELSTATUS, dwHandle, sizeof(TChannelStatus)*nSize+sizeof(int32_t));
	pPeer->Encode(&nSize, sizeof(int32_t));

	for (const auto& [sPort, bStatus] : m_mChannelStatus)
	{
		pPeer->Encode(&sPort, sizeof(int16_t));
		pPeer->Encode(&bStatus, sizeof(uint8_t));
	}
}

void CClientManager::ResetLastPlayerID(const TPacketNeedLoginLogInfo* data)
{
	CLoginData* pLD = GetLoginDataByAID(data->dwPlayerID);

	if (pLD)
		pLD->SetLastPlayerID(0);
}

void CClientManager::ChargeCash(const TRequestChargeCash* packet)
{
	char szQuery[512];

	if (ERequestCharge_Cash == packet->eChargeType)
		sprintf(szQuery, "update account set `cash` = `cash` + %d where id = %d limit 1", packet->dwAmount, packet->dwAID);
	else if (ERequestCharge_Mileage == packet->eChargeType)
		sprintf(szQuery, "update account set `mileage` = `mileage` + %d where id = %d limit 1", packet->dwAmount, packet->dwAID);
	else
	{
		SysLog("Invalid request charge type (type : {}, amount : {}, aid : {})", packet->eChargeType, packet->dwAmount, packet->dwAID);
		return;
	}

	SysLog ("Request Charge (type : {}, amount : {}, aid : {})", packet->eChargeType, packet->dwAmount, packet->dwAID);

	CDBManager::GetInstance()->AsyncQuery(szQuery, SQL_ACCOUNT);
}