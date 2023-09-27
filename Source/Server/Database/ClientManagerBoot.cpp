#include "stdafx.h"
#include <Core/ProtoReader.hpp>
#include <map>
#include "ClientManager.h"
#include "Main.h"
#include "Monarch.h"
#include "CsvReader.h"
#include "ProtoReader.h"

extern std::string g_stLocaleNameColumn;
static const char* c_szItemProtoName = "item_proto.txt";
static const char* c_szItemDescName = "itemdesc.txt";
static const char* c_szMobProtoName = "mob_proto.txt";
static const char* c_szMobDescName = "mobdesc.txt";

bool CClientManager::InitializeTables()
{
	if (!InitializeMobTable())
	{
		SysLog("InitializeMobTable FAILED");
		return false;
	}

	if (!InitializeItemTable())
	{
		SysLog("InitializeItemTable FAILED");
		return false; 
	}

	if (!InitializeShopTable())
	{
		SysLog("InitializeShopTable FAILED");
		return false;
	}

	if (!InitializeSkillTable())
	{
		SysLog("InitializeSkillTable FAILED");
		return false;
	}

	if (!InitializeRefineTable())
	{
		SysLog("InitializeRefineTable FAILED");
		return false;
	}

	if (!InitializeItemAttrTable())
	{
		SysLog("InitializeItemAttrTable FAILED");
		return false;
	}

	if (!InitializeItemRareTable())
	{
		SysLog("InitializeItemRareTable FAILED");
		return false;
	}

	if (!InitializeBanwordTable())
	{
		SysLog("InitializeBanwordTable FAILED");
		return false;
	}

	if (!InitializeLandTable())
	{
		SysLog("InitializeLandTable FAILED");
		return false;
	}

	if (!InitializeObjectProto())
	{
		SysLog("InitializeObjectProto FAILED");
		return false;
	}

	if (!InitializeObjectTable())
	{
		SysLog("InitializeObjectTable FAILED");
		return false;
	}

	if (!InitializeMonarch())
	{
		SysLog("InitializeMonarch FAILED");
		return false;
	}


	return true;
}

bool CClientManager::InitializeRefineTable()
{
	char query[2048];

	snprintf(query, sizeof(query),
			"SELECT id, cost, prob, vnum0, count0, vnum1, count1, vnum2, count2,  vnum3, count3, vnum4, count4 FROM refine_proto%s",
			GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!pRes->uiNumRows)
		return true;

	if (m_pRefineTable)
	{
		TraceLog("RELOAD: refine_proto");
		delete [] m_pRefineTable;
		m_pRefineTable = nullptr;
	}

	m_iRefineTableSize = pRes->uiNumRows;

	m_pRefineTable	= new TRefineTable[m_iRefineTableSize];
	memset(m_pRefineTable, 0, sizeof(TRefineTable) * m_iRefineTableSize);

	TRefineTable* prt = m_pRefineTable;
	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		int32_t col = 0;
		//prt->src_vnum = atoi(data[col++]);
		//prt->result_vnum = atoi(data[col++]);
		str_to_number(prt->id, data[col++]);
		str_to_number(prt->cost, data[col++]);
		str_to_number(prt->prob, data[col++]);

		for (int32_t i = 0; i < ITEM::REFINE_MATERIAL_MAX_NUM; i++)
		{
			str_to_number(prt->materials[i].vnum, data[col++]);
			str_to_number(prt->materials[i].count, data[col++]);
			if (prt->materials[i].vnum == 0)
			{
				prt->material_count = i;
				break;
			}
		}

		TraceLog("REFINE: id {} cost {} prob {} mat1 {} cnt1 {}", prt->id, prt->cost, prt->prob, prt->materials[0].vnum, prt->materials[0].count);

		prt++;
	}
	return true;
}

bool CClientManager::InitializeMobTable()
{
	static std::map<uint32_t, std::string> mobNameMap;

	// Mob Names
	{
		if (!mobNameMap.empty())
		{
			TraceLog("RELOAD: mob_desc");
			mobNameMap.clear();
		}

		auto VFSFileString = CallFS().LoadFileToString(CallFS(), c_szMobDescName);
		if (!VFSFileString)
		{
			SysLog("Failed to load {0}", c_szMobDescName);
			return false;
		}

		CMemoryTextFileLoader textFileLoader;
		textFileLoader.Bind(VFSFileString.value());
		CTokenVector TokenVector;

		for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
		{
			// Ignore comments
			if (textFileLoader.GetLineString(i).find("#") == 0 ||
				textFileLoader.GetLineString(i).find("//") == 0 ||
				textFileLoader.GetLineString(i).find("--") == 0)
				continue;

			if (!textFileLoader.SplitLineByTab(i, &TokenVector))
				continue;

			if (TokenVector.size() < 2)
			{
				SysLog("Invalid token size ({}) of line {}", TokenVector.size(), i + 1);
				continue;
			}

			try
			{
				uint32_t dwVnum = std::stoi(TokenVector[0]);
				std::string strMobName = TokenVector[1];

				mobNameMap.emplace(dwVnum, strMobName);
			}
			catch (const std::invalid_argument& ia)
			{
				SysLog("Invalid token: {0}", TokenVector[0]);
				return false;
			}
			catch (const std::out_of_range& oor)
			{
				SysLog("Out of range token: {0}", TokenVector[0]);
				return false;
			}
		}
	}

	// Mob Proto
	{
		auto VFSFileString = CallFS().LoadFileToString(CallFS(), c_szMobProtoName);
		if (!VFSFileString)
		{
			SysLog("Failed to load {0}", c_szMobProtoName);
			return false;
		}

		CMemoryTextFileLoader textFileLoader;
		textFileLoader.Bind(VFSFileString.value());
		CTokenVector TokenVector;

		if (!m_vec_mobTable.empty())
		{
			TraceLog("RELOAD: mob_proto");
			m_vec_mobTable.clear();
		}

		for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
		{
			// Ignore comments
			if (textFileLoader.GetLineString(i).find("#") == 0 ||
				textFileLoader.GetLineString(i).find("//") == 0 ||
				textFileLoader.GetLineString(i).find("--") == 0)
				continue;

			if (!textFileLoader.SplitLineByTab(i, &TokenVector))
				continue;

			TMobTable table{};
			if (!ReadMobProto(TokenVector, table))
			{
				SysLog("Failed to load mob proto on line {0}", i + 1);
				continue;
			}

			// Apply locale item name from item desc file
			auto it = mobNameMap.find(table.dwVnum);
			if (it != mobNameMap.end())
				strlcpy(table.szLocaleName, it->second.c_str(), sizeof(table.szLocaleName));

			m_vec_mobTable.emplace_back(table);
		}
	}

	sort(m_vec_mobTable.begin(), m_vec_mobTable.end(), [](TMobTable& a, TMobTable& b) {return a.dwVnum < b.dwVnum; });
	return true;
}


bool CClientManager::InitializeItemTable()
{
	static std::map<uint32_t, std::string> itemNameMap;

	// Item Names
	{
		if (!itemNameMap.empty())
		{
			TraceLog("RELOAD: item_desc");
			itemNameMap.clear();
		}

		auto VFSFileString = CallFS().LoadFileToString(CallFS(), c_szItemDescName);
		if (!VFSFileString)
		{
			SysLog("Failed to load {0}", c_szItemDescName);
			return false;
		}

		CMemoryTextFileLoader textFileLoader;
		textFileLoader.Bind(VFSFileString.value());
		CTokenVector TokenVector;

		for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
		{
			// Ignore comments
			if (textFileLoader.GetLineString(i).find("#") == 0 ||
				textFileLoader.GetLineString(i).find("//") == 0 ||
				textFileLoader.GetLineString(i).find("--") == 0)
				continue;

			if (!textFileLoader.SplitLineByTab(i, &TokenVector))
				continue;

			if (TokenVector.size() < 2)
			{
				SysLog("Invalid token size ({}) of line {}", TokenVector.size(), i + 1);
				continue;
			}

			try
			{
				uint32_t dwVnum = std::stoi(TokenVector[0]);
				std::string strItemName = TokenVector[1];

				itemNameMap.emplace(dwVnum, strItemName);
			}
			catch (const std::invalid_argument& ia)
			{
				SysLog("Invalid token: {0}", TokenVector[0]);
				return false;
			}
			catch (const std::out_of_range& oor)
			{
				SysLog("Out of range token: {0}", TokenVector[0]);
				return false;
			}
		}
	}

	// Item Proto
	{
		auto VFSFileString = CallFS().LoadFileToString(CallFS(), c_szItemProtoName);
		if (!VFSFileString)
		{
			SysLog("Failed to load {0}", c_szItemProtoName);
			return false;
		}

		CMemoryTextFileLoader textFileLoader;
		textFileLoader.Bind(VFSFileString.value());
		CTokenVector TokenVector;

		if (!m_vec_itemTable.empty())
		{
			TraceLog("RELOAD: item_proto");
			m_vec_itemTable.clear();
			m_map_itemTableByVnum.clear();
		}

		for (uint32_t i = 0; i < textFileLoader.GetLineCount(); ++i)
		{
			// Ignore comments
			if (textFileLoader.GetLineString(i).find("#") == 0 ||
				textFileLoader.GetLineString(i).find("//") == 0 ||
				textFileLoader.GetLineString(i).find("--") == 0)
				continue;

			if (!textFileLoader.SplitLineByTab(i, &TokenVector))
				continue;

			TItemTable table{};
			if (!ReadItemProto(TokenVector, table))
			{
				SysLog("Failed to load item proto on line {0}", i + 1);
				continue;
			}

			// Apply locale item name from item desc file
			auto it = itemNameMap.find(table.dwVnum);
			if (it != itemNameMap.end())
				strlcpy(table.szLocaleName, it->second.c_str(), sizeof(table.szLocaleName));

			m_vec_itemTable.emplace_back(table);
		}

		auto it = m_vec_itemTable.begin();

		while (it != m_vec_itemTable.end())
		{
			TItemTable* item_table = &(*(it++));

			TraceLog("ITEM: {} {} {} VAL: {} {} {} {} {} {} WEAR {} ANTI {} IMMUNE {} REFINE {} REFINE_SET {} MAGIC_PCT {}",
				item_table->dwVnum,
				item_table->szName,
				item_table->szLocaleName,
				item_table->alValues[0],
				item_table->alValues[1],
				item_table->alValues[2],
				item_table->alValues[3],
				item_table->alValues[4],
				item_table->alValues[5],
				item_table->dwWearFlags,
				item_table->dwAntiFlags,
				item_table->dwImmuneFlag,
				item_table->dwRefinedVnum,
				item_table->wRefineSet,
				item_table->bAlterToMagicItemPct);

			m_map_itemTableByVnum.emplace(item_table->dwVnum, item_table);
		}
		sort(m_vec_itemTable.begin(), m_vec_itemTable.end(), [](TItemTable& a, TItemTable& b) {return a.dwVnum < b.dwVnum; });
		return true;
	}
}

bool CClientManager::InitializeShopTable()
{
	MYSQL_ROW	data;
	int32_t		col;

	static const char* s_szQuery = 
		"SELECT "
		"shop.vnum, "
		"shop.npc_vnum, "
		"shop_item.item_vnum, "
		"shop_item.count "
		"FROM shop LEFT JOIN shop_item "
		"ON shop.vnum = shop_item.shop_vnum ORDER BY shop.vnum, shop_item.item_vnum";

	std::unique_ptr<SQLMsg> pMsg2(CDBManager::GetInstance()->DirectQuery(s_szQuery));
	SQLResult* pRes2 = pMsg2->Get();

	if (!pRes2->uiNumRows)
	{
		WarnLog("InitializeShopTable : Table count is zero.");
		return false;
	}

	std::map<int32_t, TShopTable *> map_shop;

	if (m_pShopTable)
	{
		delete [] (m_pShopTable);
		m_pShopTable = nullptr;
	}

	TShopTable* shop_table = m_pShopTable;

	while ((data = mysql_fetch_row(pRes2->pSQLResult)))
	{
		col = 0;

		int32_t iShopVnum = 0;
		str_to_number(iShopVnum, data[col++]);

		if (map_shop.end() == map_shop.find(iShopVnum))
		{
			shop_table = new TShopTable;
			memset(shop_table, 0, sizeof(TShopTable));
			shop_table->dwVnum	= iShopVnum;

			map_shop[iShopVnum] = shop_table;
		}
		else
			shop_table = map_shop[iShopVnum];

		str_to_number(shop_table->dwNPCVnum, data[col++]);

		if (!data[col])	// If there is no item, NULL is returned.
			continue;

		TShopItemTable* pItem = &shop_table->items[shop_table->byItemCount];

		str_to_number(pItem->vnum, data[col++]);
		str_to_number(pItem->count, data[col++]);

		++shop_table->byItemCount;
	}

	m_pShopTable = new TShopTable[map_shop.size()];
	m_iShopTableSize = map_shop.size();

	auto it = map_shop.begin();

	int32_t i = 0;

	while (it != map_shop.end())
	{
		memcpy((m_pShopTable + i), (it++)->second, sizeof(TShopTable));
		TraceLog("SHOP: #{} items: {}", (m_pShopTable + i)->dwVnum, (m_pShopTable + i)->byItemCount);
		++i;
	}

	return true;
}

bool CClientManager::InitializeQuestItemTable()
{
	static const char* s_szQuery = "SELECT vnum, name, %s FROM quest_item_proto ORDER BY vnum";

	char query[1024];
	snprintf(query, sizeof(query), s_szQuery, g_stLocaleNameColumn.c_str());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!pRes->uiNumRows)
	{
		WarnLog("query error or no rows: {}", query);
		return false;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pRes->pSQLResult)))
	{
		int32_t col = 0;

		TItemTable tbl;
		memset(&tbl, 0, sizeof(tbl));

		str_to_number(tbl.dwVnum, row[col++]);

		if (row[col])
			strlcpy(tbl.szName, row[col], sizeof(tbl.szName));

		col++;

		if (row[col])
			strlcpy(tbl.szLocaleName, row[col], sizeof(tbl.szLocaleName));

		col++;

		if (m_map_itemTableByVnum.find(tbl.dwVnum) != m_map_itemTableByVnum.end())
		{
			WarnLog("QUEST_ITEM_ERROR! %lu vnum already exist! (name {})", tbl.dwVnum, tbl.szLocaleName);
			continue;
		}

		tbl.bType = ITEM::TYPE_QUEST; // Everything in the quest_item_proto table is of type ITEM_QUEST
		tbl.bSize = 1;

		m_vec_itemTable.push_back(tbl);
	}

	return true;
}

bool CClientManager::InitializeSkillTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
		"SELECT dwVnum, szName, bType, bMaxLevel, dwSplashRange, "
		"szPointOn, szPointPoly, szSPCostPoly, szDurationPoly, szDurationSPCostPoly, "
		"szCooldownPoly, szMasterBonusPoly, setFlag+0, setAffectFlag+0, "
		"szPointOn2, szPointPoly2, szDurationPoly2, setAffectFlag2+0, "
		"szPointOn3, szPointPoly3, szDurationPoly3, szGrandMasterAddSPCostPoly, "
		"bLevelStep, bLevelLimit, prerequisiteSkillVnum, prerequisiteSkillLevel, iMaxHit, szSplashAroundDamageAdjustPoly, eSkillType+0, dwTargetRange "
		"FROM skill_proto%s ORDER BY dwVnum",
		GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!pRes->uiNumRows)
	{
		WarnLog("no result from skill_proto");
		return false;
	}

	if (!m_vec_skillTable.empty())
	{
		TraceLog("RELOAD: skill_proto");
		m_vec_skillTable.clear();
	}

	m_vec_skillTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;
	int32_t		col;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TSkillTable t;
		memset(&t, 0, sizeof(t));

		col = 0;

		str_to_number(t.dwVnum, data[col++]);
		strlcpy(t.szName, data[col++], sizeof(t.szName));
		str_to_number(t.bType, data[col++]);
		str_to_number(t.bMaxLevel, data[col++]);
		str_to_number(t.dwSplashRange, data[col++]);

		strlcpy(t.szPointOn, data[col++], sizeof(t.szPointOn));
		strlcpy(t.szPointPoly, data[col++], sizeof(t.szPointPoly));
		strlcpy(t.szSPCostPoly, data[col++], sizeof(t.szSPCostPoly));
		strlcpy(t.szDurationPoly, data[col++], sizeof(t.szDurationPoly));
		strlcpy(t.szDurationSPCostPoly, data[col++], sizeof(t.szDurationSPCostPoly));
		strlcpy(t.szCooldownPoly, data[col++], sizeof(t.szCooldownPoly));
		strlcpy(t.szMasterBonusPoly, data[col++], sizeof(t.szMasterBonusPoly));

		str_to_number(t.dwFlag, data[col++]);
		str_to_number(t.dwAffectFlag, data[col++]);

		strlcpy(t.szPointOn2, data[col++], sizeof(t.szPointOn2));
		strlcpy(t.szPointPoly2, data[col++], sizeof(t.szPointPoly2));
		strlcpy(t.szDurationPoly2, data[col++], sizeof(t.szDurationPoly2));
		str_to_number(t.dwAffectFlag2, data[col++]);

		// ADD_GRANDMASTER_SKILL
		strlcpy(t.szPointOn3, data[col++], sizeof(t.szPointOn3));
		strlcpy(t.szPointPoly3, data[col++], sizeof(t.szPointPoly3));
		strlcpy(t.szDurationPoly3, data[col++], sizeof(t.szDurationPoly3));

		strlcpy(t.szGrandMasterAddSPCostPoly, data[col++], sizeof(t.szGrandMasterAddSPCostPoly));
		// END_OF_ADD_GRANDMASTER_SKILL

		str_to_number(t.bLevelStep, data[col++]);
		str_to_number(t.bLevelLimit, data[col++]);
		str_to_number(t.preSkillVnum, data[col++]);
		str_to_number(t.preSkillLevel, data[col++]);

		str_to_number(t.lMaxHit, data[col++]);

		strlcpy(t.szSplashAroundDamageAdjustPoly, data[col++], sizeof(t.szSplashAroundDamageAdjustPoly));

		str_to_number(t.bSkillAttrType, data[col++]);
		str_to_number(t.dwTargetRange, data[col++]);

		TraceLog("SKILL: {} {} flag {} point {} affect {} cooldown {}", t.dwVnum, t.szName, t.dwFlag, t.szPointOn, t.dwAffectFlag, t.szCooldownPoly);

		m_vec_skillTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeBanwordTable()
{
	m_vec_banwordTable.clear();

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery("SELECT word FROM banword"));

	SQLResult* pRes = pMsg->Get();

	if (pRes->uiNumRows == 0)
		return true;

	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TBanwordTable t;

		if (data[0])
		{
			strlcpy(t.szWord, data[0], sizeof(t.szWord));
			m_vec_banwordTable.push_back(t);
		}
	}

	TraceLog("BANWORD: total {}", m_vec_banwordTable.size());
	return true;
}

bool CClientManager::InitializeItemAttrTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear FROM item_attr%s ORDER BY apply",
			GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!pRes->uiNumRows)
	{
		WarnLog("no result from item_attr");
		return false;
	}

	if (!m_vec_itemAttrTable.empty())
	{
		TraceLog("RELOAD: item_attr");
		m_vec_itemAttrTable.clear();
	}

	m_vec_itemAttrTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		memset(&t, 0, sizeof(TItemAttrTable));

		int32_t col = 0;

		strlcpy(t.szApply, data[col++], sizeof(t.szApply));
		str_to_number(t.dwApplyIndex, data[col++]);
		str_to_number(t.dwProb, data[col++]);
		str_to_number(t.lValues[0], data[col++]);
		str_to_number(t.lValues[1], data[col++]);
		str_to_number(t.lValues[2], data[col++]);
		str_to_number(t.lValues[3], data[col++]);
		str_to_number(t.lValues[4], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_BODY], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_NECK], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_EAR], data[col++]);

		TraceLog("ITEM_ATTR: {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}",
				t.szApply,
				t.dwProb,
				t.lValues[0],
				t.lValues[1],
				t.lValues[2],
				t.lValues[3],
				t.lValues[4],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON],
				t.bMaxLevelBySet[ATTRIBUTE_SET_BODY],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST],
				t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS],
				t.bMaxLevelBySet[ATTRIBUTE_SET_NECK],
				t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_EAR]);

		m_vec_itemAttrTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeItemRareTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear FROM item_attr_rare%s ORDER BY apply",
			GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!pRes->uiNumRows)
	{
		WarnLog("no result from item_attr_rare");
		return false;
	}

	if (!m_vec_itemRareTable.empty())
	{
		TraceLog("RELOAD: item_attr_rare");
		m_vec_itemRareTable.clear();
	}

	m_vec_itemRareTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		memset(&t, 0, sizeof(TItemAttrTable));

		int32_t col = 0;

		strlcpy(t.szApply, data[col++], sizeof(t.szApply));
		str_to_number(t.dwApplyIndex, data[col++]);
		str_to_number(t.dwProb, data[col++]);
		str_to_number(t.lValues[0], data[col++]);
		str_to_number(t.lValues[1], data[col++]);
		str_to_number(t.lValues[2], data[col++]);
		str_to_number(t.lValues[3], data[col++]);
		str_to_number(t.lValues[4], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_BODY], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_NECK], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_EAR], data[col++]);

		TraceLog("ITEM_RARE: {} {} {} {} {} {} {} {} {} {} {} {} {} {} {}",
				t.szApply,
				t.dwProb,
				t.lValues[0],
				t.lValues[1],
				t.lValues[2],
				t.lValues[3],
				t.lValues[4],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON],
				t.bMaxLevelBySet[ATTRIBUTE_SET_BODY],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST],
				t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS],
				t.bMaxLevelBySet[ATTRIBUTE_SET_NECK],
				t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_EAR]);

		m_vec_itemRareTable.push_back(t);
	}

	return true;
}

bool CClientManager::InitializeLandTable()
{
	using namespace building;

	char query[4096];

	snprintf(query, sizeof(query),
		"SELECT id, map_index, x, y, width, height, guild_id, guild_level_limit, price "
		"FROM land%s WHERE enable='YES' ORDER BY id",
		GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!m_vec_kLandTable.empty())
	{
		TraceLog("RELOAD: land");
		m_vec_kLandTable.clear();
	}

	m_vec_kLandTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TLand t;

			memset(&t, 0, sizeof(t));

			int32_t col = 0;

			str_to_number(t.dwID, data[col++]);
			str_to_number(t.lMapIndex, data[col++]);
			str_to_number(t.x, data[col++]);
			str_to_number(t.y, data[col++]);
			str_to_number(t.width, data[col++]);
			str_to_number(t.height, data[col++]);
			str_to_number(t.dwGuildID, data[col++]);
			str_to_number(t.bGuildLevelLimit, data[col++]);
			str_to_number(t.dwPrice, data[col++]);

			TraceLog("LAND: {} map {} {}{} w {} h {}", t.dwID, t.lMapIndex, t.x, t.y, t.width, t.height);

			m_vec_kLandTable.push_back(t);
		}

	return true;
}

void parse_pair_number_string(const char* c_pszString, std::vector<std::pair<int32_t, int32_t> >& vec)
{
	// format: 10,1/20,3/300,50
	const char* t = c_pszString;
	const char* p = strchr(t, '/');
	std::pair<int32_t, int32_t> k;

	char szNum[32 + 1];
	char* comma;

	while (p)
	{
		if (isnhdigit(*t))
		{
			strlcpy(szNum, t, MIN(sizeof(szNum), (p-t)+1));

			comma = strchr(szNum, ',');

			if (comma)
			{
				*comma = '\0';
				str_to_number(k.second, comma+1);
			}
			else
				k.second = 0;

			str_to_number(k.first, szNum);
			vec.push_back(k);
		}

		t = p + 1;
		p = strchr(t, '/');
	}

	if (isnhdigit(*t))
	{
		strlcpy(szNum, t, sizeof(szNum));

		comma = strchr(const_cast<char*>(t), ',');

		if (comma)
		{
			*comma = '\0';
			str_to_number(k.second, comma+1);
		}
		else
			k.second = 0;

		str_to_number(k.first, szNum);
		vec.push_back(k);
	}
}

bool CClientManager::InitializeObjectProto()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT vnum, price, materials, upgrade_vnum, upgrade_limit_time, life, reg_1, reg_2, reg_3, reg_4, npc, group_vnum, dependent_group "
			"FROM object_proto%s ORDER BY vnum",
			GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!m_vec_kObjectProto.empty())
	{
		TraceLog("RELOAD: object_proto");
		m_vec_kObjectProto.clear();
	}

	m_vec_kObjectProto.reserve(MAX(0, pRes->uiNumRows));

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TObjectProto t;

			memset(&t, 0, sizeof(t));

			int32_t col = 0;

			str_to_number(t.dwVnum, data[col++]);
			str_to_number(t.dwPrice, data[col++]);

			std::vector<std::pair<int32_t, int32_t> > vec;
			parse_pair_number_string(data[col++], vec);

			for (uint32_t i = 0; i < OBJECT_MATERIAL_MAX_NUM && i < vec.size(); ++i)
			{
				std::pair<int32_t, int32_t>& r = vec[i];

				t.kMaterials[i].dwItemVnum = r.first;
				t.kMaterials[i].dwCount = r.second;
			}

			str_to_number(t.dwUpgradeVnum, data[col++]);
			str_to_number(t.dwUpgradeLimitTime, data[col++]);
			str_to_number(t.lLife, data[col++]);
			str_to_number(t.lRegion[0], data[col++]);
			str_to_number(t.lRegion[1], data[col++]);
			str_to_number(t.lRegion[2], data[col++]);
			str_to_number(t.lRegion[3], data[col++]);
			str_to_number(t.dwNPCVnum, data[col++]);
			str_to_number(t.dwGroupVnum, data[col++]);
			str_to_number(t.dwDependOnGroupVnum, data[col++]);

			t.lNPCX = 0;
			t.lNPCY = MAX(t.lRegion[1], t.lRegion[3])+300;

			TraceLog("OBJ_PROTO: vnum {} price {} mat {} {}",
					t.dwVnum, t.dwPrice, t.kMaterials[0].dwItemVnum, t.kMaterials[0].dwCount);

			m_vec_kObjectProto.push_back(t);
		}

	return true;
}

bool CClientManager::InitializeObjectTable()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query), "SELECT id, land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot, life FROM object%s ORDER BY id", GetTablePostfix());

	std::unique_ptr<SQLMsg> pMsg(CDBManager::GetInstance()->DirectQuery(query));
	SQLResult* pRes = pMsg->Get();

	if (!m_map_pObjectTable.empty())
	{
		TraceLog("RELOAD: object");
		m_map_pObjectTable.clear();
	}

	MYSQL_ROW data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TObject * k = new TObject;

			memset(k, 0, sizeof(TObject));

			int32_t col = 0;

			str_to_number(k->dwID, data[col++]);
			str_to_number(k->dwLandID, data[col++]);
			str_to_number(k->dwVnum, data[col++]);
			str_to_number(k->lMapIndex, data[col++]);
			str_to_number(k->x, data[col++]);
			str_to_number(k->y, data[col++]);
			str_to_number(k->xRot, data[col++]);
			str_to_number(k->yRot, data[col++]);
			str_to_number(k->zRot, data[col++]);
			str_to_number(k->lLife, data[col++]);

			TraceLog("OBJ: %lu vnum {} map {} {}{} life {}", 
					k->dwID, k->dwVnum, k->lMapIndex, k->x, k->y, k->lLife);

			m_map_pObjectTable.emplace(k->dwID, k);
		}

	return true;
}

bool CClientManager::InitializeMonarch()
{
	CMonarch::GetInstance()->LoadMonarch();

	return true;
}