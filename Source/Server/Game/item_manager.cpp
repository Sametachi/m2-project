#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"
#include "log.h"
#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "locale_service.h"
#include "item.h"
#include "item_manager.h"

#include <Common/VnumHelper.h>
#include "DragonSoul.h"
#include "cube.h"

ITEM_MANAGER::ITEM_MANAGER()
	: m_iTopOfTable(0), m_dwVIDCount(0), m_dwCurrentID(0)
{
	m_ItemIDRange.dwMin = m_ItemIDRange.dwMax = m_ItemIDRange.dwUsableItemIDMin = 0;
	m_ItemIDSpareRange.dwMin = m_ItemIDSpareRange.dwMax = m_ItemIDSpareRange.dwUsableItemIDMin = 0;
}

ITEM_MANAGER::~ITEM_MANAGER()
{
	Destroy();
}

void ITEM_MANAGER::Destroy()
{
	auto it = m_VIDMap.begin();
	for (; it != m_VIDMap.end(); ++it) {
		M2_DELETE(it->second);
	}
	m_VIDMap.clear();
}

void ITEM_MANAGER::GracefulShutdown()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pItemForDelayedSave.begin();

	while (it != m_set_pItemForDelayedSave.end())
		SaveSingleItem(*(it++));

	m_set_pItemForDelayedSave.clear();
}

bool ITEM_MANAGER::Initialize(TItemTable * table, int32_t size)
{
	if (!m_vec_prototype.empty())
		m_vec_prototype.clear();

	int32_t	i;

	m_vec_prototype.resize(size);
	memcpy(&m_vec_prototype[0], table, sizeof(TItemTable) * size);
	for (int32_t i = 0; i < size; i++)
	{
		if (0 != m_vec_prototype[i].dwVnumRange)
		{
			m_vec_item_vnum_range_info.push_back(&m_vec_prototype[i]);
		}
	}

	m_map_ItemRefineFrom.clear();
	for (i = 0; i < size; ++i)
	{

		if (m_vec_prototype[i].dwRefinedVnum)
			m_map_ItemRefineFrom.insert(std::make_pair(m_vec_prototype[i].dwRefinedVnum, m_vec_prototype[i].dwVnum));

		if (m_vec_prototype[i].bType == ITEM::TYPE_QUEST || IS_SET(m_vec_prototype[i].dwFlags, ITEM::FLAG_QUEST_USE | ITEM::FLAG_QUEST_USE_MULTIPLE))
			quest::CQuestManager::GetInstance()->RegisterNPCVnum(m_vec_prototype[i].dwVnum);

		m_map_vid.insert(std::map<uint32_t,TItemTable>::value_type(m_vec_prototype[i].dwVnum, m_vec_prototype[i])); 
		if (test_server)
			PyLog("ITEM_INFO {} {} ", m_vec_prototype[i].dwVnum, m_vec_prototype[i].szName);	
	}

	int32_t len = 0, len2;
	char buf[512];

	for (i = 0; i < size; ++i)
	{
		len2 = snprintf(buf + len, sizeof(buf) - len, "%5u %-16s", m_vec_prototype[i].dwVnum, m_vec_prototype[i].szLocaleName);

		if (len2 < 0 || len2 >= (int32_t) sizeof(buf) - len)
			len += (sizeof(buf) - len) - 1;
		else
			len += len2;

		if (!((i + 1) % 4))
		{
			if (!test_server)
				PyLog("{}", buf);
			len = 0;
		}
		else
		{
			buf[len++] = '\t';
			buf[len] = '\0';
		}
	}

	if ((i + 1) % 4)
	{
		if (!test_server)
			PyLog("{}", buf);
	}

	ITEM_VID_MAP::iterator it = m_VIDMap.begin();

	TraceLog("ITEM_VID_MAP {}", m_VIDMap.size());

	while (it != m_VIDMap.end())
	{
		LPITEM item = it->second;
		++it;

		const TItemTable* tableInfo = GetTable(item->GetOriginalVnum());

		if (!tableInfo)
		{
			SysLog("cannot reset item table");
			item->SetProto(nullptr);
		}

		item->SetProto(tableInfo);
	}

	return true;
}

LPITEM ITEM_MANAGER::CreateItem(uint32_t vnum, uint32_t count, uint32_t id, bool bTryMagic, int32_t iRarePct, bool bSkipSave)
{
	if (0 == vnum)
		return NULL;

	uint32_t dwMaskVnum = 0;
	if (GetMaskVnum(vnum))
	{
		dwMaskVnum = GetMaskVnum(vnum);
	}

	const TItemTable* table = GetTable(vnum);

	if (!table)
		return NULL;

	LPITEM item = nullptr;

	if (m_map_pItemByID.find(id) != m_map_pItemByID.end())
	{
		item = m_map_pItemByID[id];
		LPCHARACTER owner = item->GetOwner();
		TraceLog("ITEM_ID_DUP: {} {}", id, item->GetName());
		return NULL;
	}

	item = M2_NEW CItem(vnum);

	bool bIsNewItem = (0 == id);

	item->Initialize();
	item->SetProto(table);
	item->SetMaskVnum(dwMaskVnum);

	if (item->GetType() == ITEM::TYPE_ELK)
		item->SetSkipSave(true);

	else if (!bIsNewItem)
	{
		item->SetID(id);
		item->SetSkipSave(true);
	}
	else
	{
		item->SetID(GetNewID());

		if (item->GetType() == ITEM::TYPE_UNIQUE)
		{
			if (item->GetValue(2) == 0)
				item->SetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME, item->GetValue(0));
			else
			{
				item->SetSocket(ITEM::SOCKET_UNIQUE_REMAIN_TIME, get_global_time() + item->GetValue(0));
			}
		}
	}


	switch (item->GetVnum())
	{
		case ITEM_AUTO_HP_RECOVERY_S:
		case ITEM_AUTO_HP_RECOVERY_M:
		case ITEM_AUTO_HP_RECOVERY_L:
		case ITEM_AUTO_HP_RECOVERY_X:
		case ITEM_AUTO_SP_RECOVERY_S:
		case ITEM_AUTO_SP_RECOVERY_M:
		case ITEM_AUTO_SP_RECOVERY_L:
		case ITEM_AUTO_SP_RECOVERY_X:
		case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
		case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
		case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
		case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
		case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
		case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
			if (bIsNewItem)
				item->SetSocket(2, item->GetValue(0), true);
			else
				item->SetSocket(2, item->GetValue(0), false);
			break;
	}

	if (item->GetType() == ITEM::TYPE_ELK)
		;
	else if (item->IsStackable())
	{
		count = MINMAX(1, count, ITEM::MAX_COUNT);

		if (bTryMagic && count <= 1 && IS_SET(item->GetFlag(), ITEM::FLAG_MAKECOUNT))
			count = item->GetValue(1);
	}
	else
		count = 1;

	item->SetVID(++m_dwVIDCount);

	if (bSkipSave == false)
		m_VIDMap.insert(ITEM_VID_MAP::value_type(item->GetVID(), item));

	if (item->GetID() != 0 && bSkipSave == false)
		m_map_pItemByID.insert(std::map<uint32_t, LPITEM>::value_type(item->GetID(), item));

	if (!item->SetCount(count))
		return NULL;

	item->SetSkipSave(false);

	if (item->GetType() == ITEM::TYPE_UNIQUE && item->GetValue(2) != 0)
		item->StartUniqueExpireEvent();

	for (int32_t i=0 ; i < ITEM::LIMIT_SLOT_MAX_NUM ; i++)
	{
		if (ITEM::LIMIT_REAL_TIME == item->GetLimitType(i))
		{
			if (item->GetLimitValue(i))
			{
				item->SetSocket(0, time(0) + item->GetLimitValue(i)); 
			}
			else
			{
				item->SetSocket(0, time(0) + 60*60*24*7); 
			}

			item->StartRealTimeExpireEvent();
		}

		else if (ITEM::LIMIT_TIMER_BASED_ON_WEAR == item->GetLimitType(i))
		{
			if (item->IsEquipped())
			{
				item->StartTimerBasedOnWearExpireEvent();
			}
			else if(0 == id)
			{
				int32_t duration = item->GetSocket(0);
				if (0 == duration)
					duration = item->GetLimitValue(i);

				if (0 == duration)
					duration = 60 * 60 * 10;

				item->SetSocket(0, duration);
			}
		}
	}

	if (id == 0)
	{
		if (ITEM::TYPE_BLEND==item->GetType())
		{
			if (Blend_Item_find(item->GetVnum()))
			{
				Blend_Item_set_value(item);
				return item;
			}
		}

		if (table->sAddonType)
		{
			item->ApplyAddon(table->sAddonType);
		}

		if (bTryMagic)
		{
			if (iRarePct == -1)
				iRarePct = table->bAlterToMagicItemPct;

			if (number(1, 100) <= iRarePct)
				item->AlterToMagicItem();
		}

		if (table->bGainSocketPct)
			item->AlterToSocketItem(table->bGainSocketPct);

		if (vnum == 50300 || vnum == ITEM_SKILLFORGET_VNUM)
		{
			uint32_t dwSkillVnum;

			do
			{
				dwSkillVnum = number(1, 111);

				if (NULL != CSkillManager::GetInstance()->Get(dwSkillVnum))
					break;
			} while (true);

			item->SetSocket(0, dwSkillVnum);
		}
		else if (ITEM_SKILLFORGET2_VNUM == vnum)
		{
			uint32_t dwSkillVnum;

			do
			{
				dwSkillVnum = number(112, 119);

				if (NULL != CSkillManager::GetInstance()->Get(dwSkillVnum))
					break;
			} while (true);

			item->SetSocket(0, dwSkillVnum);
		}
	}

	if (item->GetType() == ITEM::TYPE_QUEST)
	{
		for (auto it = m_map_pQuestItemGroup.begin(); it != m_map_pQuestItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::QUEST && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}
	else if (item->GetType() == ITEM::TYPE_UNIQUE)
	{
		for (auto it = m_map_pSpecialItemGroup.begin(); it != m_map_pSpecialItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::SPECIAL && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}

	if (item->IsDragonSoul() && 0 == id)
	{
		DSManager::GetInstance()->DragonSoulItemInitialize(item);
	}
	return item;
}

void ITEM_MANAGER::DelayedSave(LPITEM item)
{
	if (item->GetID() != 0)
		m_set_pItemForDelayedSave.insert(item);
}

void ITEM_MANAGER::FlushDelayedSave(LPITEM item)
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pItemForDelayedSave.find(item);

	if (it == m_set_pItemForDelayedSave.end())
	{
		return;
	}

	m_set_pItemForDelayedSave.erase(it);
	SaveSingleItem(item);
}

void ITEM_MANAGER::SaveSingleItem(LPITEM item)
{
	if (!item->GetOwner())
	{
		uint32_t dwID = item->GetID();
		uint32_t dwOwnerID = item->GetLastOwnerPID();

		db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_DESTROY, 0, sizeof(uint32_t) + sizeof(uint32_t));
		db_clientdesc->Packet(&dwID, sizeof(uint32_t));
		db_clientdesc->Packet(&dwOwnerID, sizeof(uint32_t));

		TraceLog("ITEM_DELETE {}:{}", item->GetName(), dwID);
		return;
	}

	TraceLog("ITEM_SAVE {}:{} in {} window {}", item->GetName(), item->GetID(), item->GetOwner()->GetName(), item->GetWindow());

	TPlayerItem t;

	t.id = item->GetID();
	t.window = item->GetWindow();
	t.pos = t.window == EQUIPMENT ? item->GetCell() - INVENTORY_MAX_NUM : item->GetCell();
	t.count = item->GetCount();
	t.vnum = item->GetOriginalVnum();
	t.owner = (t.window == SAFEBOX) ? item->GetOwner()->GetDesc()->GetAccountTable().id : item->GetOwner()->GetPlayerID();
	memcpy(t.alSockets, item->GetSockets(), sizeof(t.alSockets));
	memcpy(t.aAttr, item->GetAttributes(), sizeof(t.aAttr));

	db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_SAVE, 0, sizeof(TPlayerItem));
	db_clientdesc->Packet(&t, sizeof(TPlayerItem));
}

void ITEM_MANAGER::Update()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pItemForDelayedSave.begin();
	TR1_NS::unordered_set<LPITEM>::iterator this_it;

	while (it != m_set_pItemForDelayedSave.end())
	{
		this_it = it++;
		LPITEM item = *this_it;

		if (item->GetOwner() && IS_SET(item->GetFlag(), ITEM::FLAG_SLOW_QUERY))
			continue;

		SaveSingleItem(item);

		m_set_pItemForDelayedSave.erase(this_it);
	}
}

void ITEM_MANAGER::RemoveItem(LPITEM item, const char* c_pszReason)
{
	LPCHARACTER o;

	if ((o = item->GetOwner()))
	{
		char szHint[64];
		snprintf(szHint, sizeof(szHint), "%s %u ", item->GetName(), item->GetCount());
		LogManager::GetInstance()->ItemLog(o, item, c_pszReason ? c_pszReason : "REMOVE", szHint);

		if (item->GetWindow() == SAFEBOX)
		{
			CSafebox* pSafebox = o->GetSafebox();
			if (pSafebox)
			{
				pSafebox->Remove(item->GetCell());
			}
		}
		else
		{
			o->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
			item->RemoveFromCharacter();
		}
	}

	M2_DESTROY_ITEM(item);
}

void ITEM_MANAGER::DestroyItem(LPITEM item)
{
	if (item->GetSectree())
		item->RemoveFromGround();

	if (item->GetOwner())
	{
		if (CHARACTER_MANAGER::GetInstance()->Find(item->GetOwner()->GetPlayerID()) != nullptr)
		{
			SysLog("DestroyItem: GetOwner {} {}!!", item->GetName(), item->GetOwner()->GetName());
			item->RemoveFromCharacter();
		}
		else
		{
			TraceLog("WTH! Invalid item owner. owner pointer");
		}
	}

	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pItemForDelayedSave.find(item);

	if (it != m_set_pItemForDelayedSave.end())
		m_set_pItemForDelayedSave.erase(it);

	uint32_t dwID = item->GetID();
	TraceLog("ITEM_DESTROY {}:{}", item->GetName(), dwID);

	if (!item->GetSkipSave() && dwID)
	{
		uint32_t dwOwnerID = item->GetLastOwnerPID();

		db_clientdesc->DBPacketHeader(HEADER_GD_ITEM_DESTROY, 0, sizeof(uint32_t) + sizeof(uint32_t));
		db_clientdesc->Packet(&dwID, sizeof(uint32_t));
		db_clientdesc->Packet(&dwOwnerID, sizeof(uint32_t));
	}
	else
	{
		TraceLog("ITEM_DESTROY_SKIP {}:{} (skip={})", item->GetName(), dwID, item->GetSkipSave());
	}

	if (dwID)
		m_map_pItemByID.erase(dwID);

	m_VIDMap.erase(item->GetVID());
	M2_DELETE(item);
}

LPITEM ITEM_MANAGER::Find(uint32_t id)
{
	auto it = m_map_pItemByID.find(id);
	if (it == m_map_pItemByID.end())
		return NULL;
	return it->second;
}

LPITEM ITEM_MANAGER::FindByVID(uint32_t vid)
{
	ITEM_VID_MAP::iterator it = m_VIDMap.find(vid);

	if (it == m_VIDMap.end())
		return NULL;

	return (it->second);
}

TItemTable * ITEM_MANAGER::GetTable(uint32_t vnum)
{
	int32_t rnum = RealNumber(vnum);

	if (rnum < 0)
	{
		for (int32_t i = 0; i < m_vec_item_vnum_range_info.size(); i++)
		{
			TItemTable* p = m_vec_item_vnum_range_info[i];
			if ((p->dwVnum < vnum) &&
				vnum < (p->dwVnum + p->dwVnumRange))
			{
				return p;
			}
		}
			
		return NULL;
	}

	return &m_vec_prototype[rnum];
}

int32_t ITEM_MANAGER::RealNumber(uint32_t vnum)
{
	int32_t bot, top, mid;

	bot = 0;
	top = m_vec_prototype.size();

	TItemTable* pTable = &m_vec_prototype[0];

	while (1)
	{
		mid = (bot + top) >> 1;

		if ((pTable + mid)->dwVnum == vnum)
			return (mid);

		if (bot >= top)
			return (-1);

		if ((pTable + mid)->dwVnum > vnum)
			top = mid - 1;
		else        
			bot = mid + 1;
	}
}

bool ITEM_MANAGER::GetVnum(const char* c_pszName, uint32_t& r_dwVnum)
{
	int32_t len = strlen(c_pszName);

	TItemTable* pTable = &m_vec_prototype[0];

	for (uint32_t i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->szLocaleName, len))
		{
			r_dwVnum = pTable->dwVnum;
			return true;
		}
	}

	return false;
}

bool ITEM_MANAGER::GetVnumByOriginalName(const char* c_pszName, uint32_t& r_dwVnum)
{
	int32_t len = strlen(c_pszName);

	TItemTable* pTable = &m_vec_prototype[0];

	for (uint32_t i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->szName, len))
		{
			r_dwVnum = pTable->dwVnum;
			return true;
		}
	}

	return false;
}

std::set<uint32_t> g_set_lotto;

void load_lotto()
{
	static int32_t bLoaded = false;

	if (bLoaded)
		return;

	bLoaded = true;
	FILE * fp = fopen("lotto.txt", "r");

	if (!fp)
		return;

	char buf[256];

	while (fgets(buf, 256, fp))
	{
		char* psz = strchr(buf, '\n');

		if (NULL != psz)
			*psz = '\0';

		uint32_t dw = 0;
		str_to_number(dw, buf);
		g_set_lotto.insert(dw);
	}

	fclose(fp);
}

uint32_t lotto()
{
	load_lotto();

	char szBuf[6 + 1];

	do
	{
		for (int32_t i = 0; i < 6; ++i)
			szBuf[i] = 48 + number(1, 9);

		szBuf[6] = '\0';

		uint32_t dw = 0;
		str_to_number(dw, szBuf);

		if (g_set_lotto.end() == g_set_lotto.find(dw))
		{
			FILE * fp = fopen("lotto.txt", "a+");
			if (fp)
			{
				fprintf(fp, "%u\n", dw);
				fclose(fp);
			}
			return dw;
		}
	}
	while (1);
}


class CItemDropInfo
{
	public:
		CItemDropInfo(int32_t iLevelStart, int32_t iLevelEnd, int32_t iPercent, uint32_t dwVnum) :
			m_iLevelStart(iLevelStart), m_iLevelEnd(iLevelEnd), m_iPercent(iPercent), m_dwVnum(dwVnum)
			{
			}

		int32_t	m_iLevelStart;
		int32_t	m_iLevelEnd;
		int32_t	m_iPercent;
		uint32_t	m_dwVnum;

		friend bool operator < (const CItemDropInfo & l, const CItemDropInfo& r)
		{
			return l.m_iLevelEnd < r.m_iLevelEnd;
		}
};

extern std::vector<CItemDropInfo> g_vec_pCommonDropItem[MOB::RANK_MAX_NUM];

int32_t GetDropPerKillPct(int32_t iMinimum, int32_t iDefault, int32_t iDeltaPercent, const char* c_pszFlag)
{
	int32_t iVal = 0;

	if ((iVal = quest::CQuestManager::GetInstance()->GetEventFlag(c_pszFlag)))
	{
		if (!test_server)
		{
			if (iVal < iMinimum)
				iVal = iDefault;

			if (iVal < 0)
				iVal = iDefault;
		}
	}

	if (iVal == 0)
		return 0;

	return (40000 * iDeltaPercent / iVal);
}

bool ITEM_MANAGER::GetDropPct(LPCHARACTER pChr, LPCHARACTER pKiller, OUT int32_t& iDeltaPercent, OUT int32_t& iRandRange)
{
 	if (!pChr || !pKiller)
		return false;

	int32_t iLevel = pKiller->GetLevel();
	iDeltaPercent = 100;

	if (!pChr->IsStone() && pChr->GetMobRank() >= MOB::RANK_BOSS)
		iDeltaPercent = PERCENT_LVDELTA_BOSS(pKiller->GetLevel(), pChr->GetLevel());
	else
		iDeltaPercent = PERCENT_LVDELTA(pKiller->GetLevel(), pChr->GetLevel());

	uint8_t bRank = pChr->GetMobRank();

	if (1 == number(1, 50000))
		iDeltaPercent += 1000;
	else if (1 == number(1, 10000))
		iDeltaPercent += 500;

	TraceLog("CreateDropItem for level: {} rank: {} pct: {}", iLevel, bRank, iDeltaPercent);
	iDeltaPercent = iDeltaPercent * CHARACTER_MANAGER::GetInstance()->GetMobItemRate(pKiller) / 100;

	if (pKiller->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0 ||
			pKiller->IsEquipUniqueGroup(UNIQUE_GROUP_DOUBLE_ITEM))
		iDeltaPercent += iDeltaPercent;

	iRandRange = 4000000;
	iRandRange = iRandRange * 100 / 
		(100 + 
		 CPrivManager::GetInstance()->GetPriv(pKiller, PRIV_ITEM_DROP) + 
		 pKiller->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_ITEM)?100:0);

	if (distribution_test_server) iRandRange /= 3;

	return true;
}

bool ITEM_MANAGER::CreateDropItem(LPCHARACTER pChr, LPCHARACTER pKiller, std::vector<LPITEM> & vec_item)
{
	int32_t iLevel = pKiller->GetLevel();

	int32_t iDeltaPercent, iRandRange;
	if (!GetDropPct(pChr, pKiller, iDeltaPercent, iRandRange))
		return false;

	uint8_t bRank = pChr->GetMobRank();
	LPITEM item = nullptr;

	std::vector<CItemDropInfo>::iterator it = g_vec_pCommonDropItem[bRank].begin();

	while (it != g_vec_pCommonDropItem[bRank].end())
	{
		const CItemDropInfo& c_rInfo = *(it++);

		if (iLevel < c_rInfo.m_iLevelStart || iLevel > c_rInfo.m_iLevelEnd)
			continue;

		int32_t iPercent = (c_rInfo.m_iPercent * iDeltaPercent) / 100;
		TraceLog("CreateDropItem {} ~ {} {}({})", c_rInfo.m_iLevelStart, c_rInfo.m_iLevelEnd, c_rInfo.m_dwVnum, iPercent, c_rInfo.m_iPercent);

		if (iPercent >= number(1, iRandRange))
		{
			TItemTable * table = GetTable(c_rInfo.m_dwVnum);

			if (!table)
				continue;

			item = nullptr;

			if (table->bType == ITEM::TYPE_POLYMORPH)
			{
				if (c_rInfo.m_dwVnum == pChr->GetPolymorphItemVnum())
				{
					item = CreateItem(c_rInfo.m_dwVnum, 1, 0, true);

					if (item)
						item->SetSocket(0, pChr->GetRaceNum());
				}
			}
			else
				item = CreateItem(c_rInfo.m_dwVnum, 1, 0, true);

			if (item) vec_item.push_back(item);
		}
	}

	{
		auto it = m_map_pDropItemGroup.find(pChr->GetRaceNum());

		if (it != m_map_pDropItemGroup.end())
		{
			typeof(it->second->GetVector()) v = it->second->GetVector();

			for (uint32_t i = 0; i < v.size(); ++i)
			{
				int32_t iPercent = (v[i].dwPct * iDeltaPercent) / 100;

				if (iPercent >= number(1, iRandRange))
				{
					item = CreateItem(v[i].dwVnum, v[i].iCount, 0, true);

					if (item)
					{
						if (item->GetType() == ITEM::TYPE_POLYMORPH)
						{
							if (item->GetVnum() == pChr->GetPolymorphItemVnum())
							{
								item->SetSocket(0, pChr->GetRaceNum());
							}
						}

						vec_item.push_back(item);
					}
				}
			}
		}
	}

	{
		auto it = m_map_pMobItemGroup.find(pChr->GetRaceNum());

		if (it != m_map_pMobItemGroup.end())
		{
			CMobItemGroup* pGroup = it->second;

			if (pGroup && !pGroup->IsEmpty())
			{
				int32_t iPercent = 40000 * iDeltaPercent / pGroup->GetKillPerDrop();
				if (iPercent >= number(1, iRandRange))
				{
					const CMobItemGroup::SMobItemGroupInfo& info = pGroup->GetOne();
					item = CreateItem(info.dwItemVnum, info.iCount, 0, true, info.iRarePct);

					if (item) vec_item.push_back(item);
				}
			}
		}
	}

	{
		auto it = m_map_pLevelItemGroup.find(pChr->GetRaceNum());

		if (it != m_map_pLevelItemGroup.end())
		{
			if (it->second->GetLevelLimit() <= (uint32_t)iLevel)
			{
				typeof(it->second->GetVector()) v = it->second->GetVector();

				for (uint32_t i=0; i < v.size(); i++)
				{
					if (v[i].dwPct >= (uint32_t)number(1, 1000000/*iRandRange*/))
					{
						uint32_t dwVnum = v[i].dwVNum;
						item = CreateItem(dwVnum, v[i].iCount, 0, true);
						if (item) vec_item.push_back(item);
					}
				}
			}
		}
	}
	
	{
		if (pKiller->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0 ||
				pKiller->IsEquipUniqueGroup(UNIQUE_GROUP_DOUBLE_ITEM))
		{
			auto it = m_map_pGloveItemGroup.find(pChr->GetRaceNum());

			if (it != m_map_pGloveItemGroup.end())
			{
				typeof(it->second->GetVector()) v = it->second->GetVector();

				for (uint32_t i = 0; i < v.size(); ++i)
				{
					int32_t iPercent = (v[i].dwPct * iDeltaPercent) / 100;

					if (iPercent >= number(1, iRandRange))
					{
						uint32_t dwVnum = v[i].dwVnum;
						item = CreateItem(dwVnum, v[i].iCount, 0, true);
						if (item) vec_item.push_back(item);
					}
				}
			}
		}
	}
	
	if (pChr->GetMobDropItemVnum())
	{
		auto it = m_map_dwEtcItemDropProb.find(pChr->GetMobDropItemVnum());

		if (it != m_map_dwEtcItemDropProb.end())
		{
			int32_t iPercent = (it->second * iDeltaPercent) / 100;

			if (iPercent >= number(1, iRandRange))
			{
				item = CreateItem(pChr->GetMobDropItemVnum(), 1, 0, true);
				if (item) vec_item.push_back(item);
			}
		}
	}

	if (pChr->IsStone())
	{
		if (pChr->GetDropMetinStoneVnum())
		{
			int32_t iPercent = (pChr->GetDropMetinStonePct() * iDeltaPercent) * 400;

			if (iPercent >= number(1, iRandRange))
			{
				item = CreateItem(pChr->GetDropMetinStoneVnum(), 1, 0, true);
				if (item) vec_item.push_back(item);
			}
		}
	}

	if (pKiller->IsHorseRiding() && 
			GetDropPerKillPct(1000, 1000000, iDeltaPercent, "horse_skill_book_drop") >= number(1, iRandRange))
	{
		PyLog("EVENT HORSE_SKILL_BOOK_DROP");

		if ((item = CreateItem(ITEM_HORSE_SKILL_TRAIN_BOOK, 1, 0, true)))
			vec_item.push_back(item);
	}


	if (GetDropPerKillPct(100, 1000, iDeltaPercent, "lotto_drop") >= number(1, iRandRange))
	{
		uint32_t* pdw = M2_NEW uint32_t[3];

		pdw[0] = 50001;
		pdw[1] = 1;
		pdw[2] = quest::CQuestManager::GetInstance()->GetEventFlag("lotto_round");

		DBManager::GetInstance()->ReturnQuery(QID_LOTTO, pKiller->GetPlayerID(), pdw,
				"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())",
				get_table_postfix(), pKiller->GetPlayerID());
	}

	CreateQuestDropItem(pChr, pKiller, vec_item, iDeltaPercent, iRandRange);

	for (auto it = vec_item.begin(); it != vec_item.end(); ++it)
	{
		LPITEM item = *it;
		DBManager::GetInstance()->SendMoneyLog(MONEY_LOG_DROP, item->GetVnum(), item->GetCount());
	}

	return vec_item.size();
}

int32_t GetThreeSkillLevelAdjust(int32_t level)
{
	if (level < 40)
		return 32;
	if (level < 45)
		return 16;
	if (level < 50)
		return 8;
	if (level < 55)
		return 4;
	if (level < 60)
		return 2;
	return 1;
}
static struct DropEvent_CharStone
{
	int32_t percent_lv01_10;
	int32_t percent_lv11_30;
	int32_t percent_lv31_MX;
	int32_t level_range;
	bool alive;

	DropEvent_CharStone()
	{
		percent_lv01_10 =  100;
		percent_lv11_30 =  200;
		percent_lv31_MX =  300;
		level_range = 10;
		alive = false;
	}
} gs_dropEvent_charStone;

static int32_t __DropEvent_CharStone_GetDropPercent(int32_t killer_level)
{
	int32_t killer_levelStep = (killer_level-1)/10;

	switch (killer_levelStep)
	{
		case 0:
			return gs_dropEvent_charStone.percent_lv01_10;

		case 1:
		case 2:
			return gs_dropEvent_charStone.percent_lv11_30;
	}

	return gs_dropEvent_charStone.percent_lv31_MX;
}

static void __DropEvent_CharStone_DropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr, std::vector<LPITEM>& vec_item)
{
	if (!gs_dropEvent_charStone.alive)
		return;

	int32_t killer_level = killer.GetLevel();
	int32_t dropPercent = __DropEvent_CharStone_GetDropPercent(killer_level);

	int32_t MaxRange = 10000;

	if (number(1, MaxRange) <= dropPercent)
	{
		int32_t victim_level = victim.GetLevel();
		int32_t level_diff = victim_level - killer_level;

		if (level_diff >= +gs_dropEvent_charStone.level_range || level_diff <= -gs_dropEvent_charStone.level_range)
		{
			PyLog("dropevent.drop_char_stone.level_range_over: killer({}: lv:{}), victim({}: lv:{}), level_diff({})",
			killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), level_diff);	
			
			return;
		}

		static const int32_t Stones[] = { 30210, 30211, 30212, 30213, 30214, 30215, 30216, 30217, 30218, 30219, 30258, 30259, 30260, 30261, 30262, 30263 };
		int32_t item_vnum = Stones[number(0, _countof(Stones))];

		LPITEM p_item = nullptr;

		if ((p_item = itemMgr.CreateItem(item_vnum, 1, 0, true)))
		{
			vec_item.push_back(p_item);

			PyLog("dropevent.drop_char_stone.item_drop: killer({}: lv{}), victim({}: lv:{}), item_name({})",
			killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), p_item->GetName());	
		}
	}
}

bool DropEvent_CharStone_SetValue(const std::string& name, int32_t value)
{
	if (name == "drop_char_stone")
	{
		gs_dropEvent_charStone.alive = value;

		if (value)
		{
			PyLog("dropevent.drop_char_stone = on");
		}
		else
			PyLog("dropevent.drop_char_stone = off");

	}
	else if (name == "drop_char_stone.percent_lv01_10")
		gs_dropEvent_charStone.percent_lv01_10 = value;
	else if (name == "drop_char_stone.percent_lv11_30")
		gs_dropEvent_charStone.percent_lv11_30 = value;
	else if (name == "drop_char_stone.percent_lv31_MX")
		gs_dropEvent_charStone.percent_lv31_MX = value;
	else if (name == "drop_char_stone.level_range")
		gs_dropEvent_charStone.level_range = value;
	else
		return false;

	PyLog("dropevent.drop_char_stone: {}", gs_dropEvent_charStone.alive ? true : false);
	PyLog("dropevent.drop_char_stone.percent_lv01_10: {}", gs_dropEvent_charStone.percent_lv01_10/100.0f);
	PyLog("dropevent.drop_char_stone.percent_lv11_30: {}", gs_dropEvent_charStone.percent_lv11_30/100.0f);
	PyLog("dropevent.drop_char_stone.percent_lv31_MX: {}", gs_dropEvent_charStone.percent_lv31_MX/100.0f);
	PyLog("dropevent.drop_char_stone.level_range: {}", gs_dropEvent_charStone.level_range);

	return true;
}

// fixme
// Let's subtract with the above as a quest.
// This is too dirty...
// ¡±?. I hate hardcoding.
// Start rewarding the weighing item.
// by rtsummit let's fix it

static struct DropEvent_RefineBox
{
	int32_t percent_low;
	int32_t low;
	int32_t percent_mid;
	int32_t mid;
	int32_t percent_high;
	bool alive;

	DropEvent_RefineBox()
	{
		percent_low =  100;
		low = 20;
		percent_mid =  100;
		mid = 45;
		percent_high =  100;
		alive = false;
	}
} gs_dropEvent_refineBox;

static LPITEM __DropEvent_RefineBox_GetDropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr)
{
	static const int32_t lowerBox[] = { 50197, 50198, 50199 };
	static const int32_t lowerBox_range = 3;
	static const int32_t midderBox[] = { 50203, 50204, 50205, 50206 };
	static const int32_t midderBox_range = 4;
	static const int32_t higherBox[] = { 50207, 50208, 50209, 50210, 50211 };
	static const int32_t higherBox_range = 5;

	if (victim.GetMobRank() < MOB::RANK_KNIGHT)
		return NULL;

	int32_t killer_level = killer.GetLevel();
	
	if (killer_level <= gs_dropEvent_refineBox.low)
	{
		if (number (1, gs_dropEvent_refineBox.percent_low) == 1)
		{
			return itemMgr.CreateItem(lowerBox [number (1,lowerBox_range) - 1], 1, 0, true);
		}
	}
	else if (killer_level <= gs_dropEvent_refineBox.mid)
	{
		if (number (1, gs_dropEvent_refineBox.percent_mid) == 1)
		{
			return itemMgr.CreateItem(midderBox [number (1,midderBox_range) - 1], 1, 0, true);
		}
	}
	else
	{
		if (number (1, gs_dropEvent_refineBox.percent_high) == 1)
		{
			return itemMgr.CreateItem(higherBox [number (1,higherBox_range) - 1], 1, 0, true);
		}
	}
	return NULL;
}

static void __DropEvent_RefineBox_DropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr, std::vector<LPITEM>& vec_item)
{
	if (!gs_dropEvent_refineBox.alive)
		return;

	LPITEM p_item = __DropEvent_RefineBox_GetDropItem(killer, victim, itemMgr);

	if (p_item)
	{
		vec_item.push_back(p_item);

		PyLog("dropevent.drop_refine_box.item_drop: killer({}: {}), victim({}: lv:{}), item_name({})",
		killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), p_item->GetName());	
	}
}

bool DropEvent_RefineBox_SetValue(const std::string& name, int32_t value)
{
	if (name == "refine_box_drop")
	{
		gs_dropEvent_refineBox.alive = value;

		if (value)
		{
			PyLog("refine_box_drop = on");
		}
		else
			PyLog("refine_box_drop = off");

	}
	else if (name == "refine_box_low")
		gs_dropEvent_refineBox.percent_low = value < 100 ? 100 : value;
	else if (name == "refine_box_mid")
		gs_dropEvent_refineBox.percent_mid = value < 100 ? 100 : value;
	else if (name == "refine_box_high")
		gs_dropEvent_refineBox.percent_high = value < 100 ? 100 : value;
	else
		return false;

	PyLog("refine_box_drop: {}", gs_dropEvent_refineBox.alive ? true : false);
	PyLog("refine_box_low: {}", gs_dropEvent_refineBox.percent_low);
	PyLog("refine_box_mid: {}", gs_dropEvent_refineBox.percent_mid);
	PyLog("refine_box_high: {}", gs_dropEvent_refineBox.percent_high);
	
	return true;
}


void ITEM_MANAGER::CreateQuestDropItem(LPCHARACTER pChr, LPCHARACTER pKiller, std::vector<LPITEM> & vec_item, int32_t iDeltaPercent, int32_t iRandRange)
{
	LPITEM item = nullptr;

	if (!pChr)
		return;

	if (!pKiller)
		return;

	TraceLog("CreateQuestDropItem victim({}), killer({})", pChr->GetName(), pKiller->GetName());

	__DropEvent_CharStone_DropItem(*pKiller,* pChr, *this, vec_item);
	__DropEvent_RefineBox_DropItem(*pKiller,* pChr, *this, vec_item);

	if (quest::CQuestManager::GetInstance()->GetEventFlag("xmas_sock"))
	{
		uint32_t SOCK_ITEM_VNUM = 50010;

		int32_t iDropPerKill[MOB::RANK_MAX_NUM] =
		{
			2000,
			1000,
			300,
			50,
			0,
			0,
		};

		if (iDropPerKill[pChr->GetMobRank()] != 0)
		{
			int32_t iPercent = 40000 * iDeltaPercent / iDropPerKill[pChr->GetMobRank()];

			PyLog("SOCK DROP {} {}", iPercent, iRandRange);
			if (iPercent >= number(1, iRandRange))
			{
				if ((item = CreateItem(SOCK_ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	if (quest::CQuestManager::GetInstance()->GetEventFlag("drop_moon"))
	{
		const uint32_t ITEM_VNUM = 50011;

		int32_t iDropPerKill[MOB::RANK_MAX_NUM] =
		{
			2000,
			1000,
			300,
			50,
			0,
			0,
		};

		if (iDropPerKill[pChr->GetMobRank()])
		{
			int32_t iPercent = 40000 * iDeltaPercent / iDropPerKill[pChr->GetMobRank()];

			if (iPercent >= number(1, iRandRange))
			{
				if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	if (pKiller->GetLevel() >= 15 && abs(pKiller->GetLevel() - pChr->GetLevel()) <= 5)
	{
		int32_t pct = quest::CQuestManager::GetInstance()->GetEventFlag("hc_drop");

		if (pct > 0)
		{
			const uint32_t ITEM_VNUM = 30178;

			if (number(1,100) <= pct)
			{
				if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "2006_drop") >= number(1, iRandRange))
	{
		PyLog("2006_drop DROP EVENT ");

		const static uint32_t dwVnum = 50037;

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);

	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "2007_drop") >= number(1, iRandRange))
	{
		PyLog("2007_drop DROP EVENT ");

		const static uint32_t dwVnum = 50043;

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(/* minimum */ 100, /* default */ 1000, iDeltaPercent, "newyear_fire") >= number(1, iRandRange))
	{
		const uint32_t ITEM_VNUM_FIRE = 50107;

		if ((item = CreateItem(ITEM_VNUM_FIRE, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(100, 500, iDeltaPercent, "newyear_moon") >= number(1, iRandRange))
	{
		PyLog("EVENT NEWYEAR_MOON DROP");

		const static uint32_t wonso_items[6] = { 50016, 50017, 50018, 50019, 50019, 50019, };
		uint32_t dwVnum = wonso_items[number(0,5)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(1, 2000, iDeltaPercent, "valentine_drop") >= number(1, iRandRange))
	{
		PyLog("EVENT VALENTINE_DROP");

		const static uint32_t valentine_items[2] = { 50024, 50025 };
		uint32_t dwVnum = valentine_items[number(0, 1)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "icecream_drop") >= number(1, iRandRange))
	{
		const static uint32_t icecream = 50123;

		if ((item = CreateItem(icecream, 1, 0, true)))
			vec_item.push_back(item);
	}

	if ((pKiller->CountSpecifyItem(53002) > 0) && (GetDropPerKillPct(50, 100, iDeltaPercent, "new_xmas_event") >= number(1, iRandRange)))
	{
		const static uint32_t xmas_sock = 50010;
		pKiller->AutoGiveItem (xmas_sock, 1);
	}

	if ((pKiller->CountSpecifyItem(53007) > 0) && (GetDropPerKillPct(50, 100, iDeltaPercent, "new_xmas_event") >= number(1, iRandRange)))
	{
		const static uint32_t xmas_sock = 50010;
		pKiller->AutoGiveItem (xmas_sock, 1);
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "halloween_drop") >= number(1, iRandRange))
	{
		const static uint32_t halloween_item = 30321;

		if ((item=CreateItem(halloween_item, 1, 0, true)))
			vec_item.push_back(item);
	}
	
	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "ramadan_drop") >= number(1, iRandRange))
	{
		const static uint32_t ramadan_item = 30315;

		if ((item=CreateItem(ramadan_item, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "easter_drop") >= number(1, iRandRange))
	{
		const static uint32_t easter_item_base = 50160;

		if ((item=CreateItem(easter_item_base+number(0,19), 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "football_drop") >= number(1, iRandRange))
	{
		const static uint32_t football_item = 50096;

		if ((item=CreateItem(football_item, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "whiteday_drop") >= number(1, iRandRange))
	{
		PyLog("EVENT WHITEDAY_DROP");
		const static uint32_t whiteday_items[2] = { ITEM_WHITEDAY_ROSE, ITEM_WHITEDAY_CANDY };
		uint32_t dwVnum = whiteday_items[number(0,1)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (pKiller->GetLevel()>=50)
	{
		if (GetDropPerKillPct(100, 1000, iDeltaPercent, "kids_day_drop_high") >= number(1, iRandRange))
		{
			uint32_t ITEM_QUIZ_BOX = 50034;

			if ((item = CreateItem(ITEM_QUIZ_BOX, 1, 0, true)))
				vec_item.push_back(item);
		}
	}
	else
	{
		if (GetDropPerKillPct(100, 1000, iDeltaPercent, "kids_day_drop") >= number(1, iRandRange))
		{
			uint32_t ITEM_QUIZ_BOX = 50034;

			if ((item = CreateItem(ITEM_QUIZ_BOX, 1, 0, true)))
				vec_item.push_back(item);
		}
	}

	if (pChr->GetLevel() >= 30 && GetDropPerKillPct(50, 100, iDeltaPercent, "medal_part_drop") >= number(1, iRandRange))
	{
		const static uint32_t drop_items[] = { 30265, 30266, 30267, 30268, 30269 };
		int32_t i = number (0, 4);
		item = CreateItem(drop_items[i]);
		if (item != nullptr)
			vec_item.push_back(item);
	}

	if (pChr->GetLevel() >= 40 && pChr->GetMobRank() >= MOB::RANK_BOSS && GetDropPerKillPct(/* minimum */ 1, /* default */ 1000, iDeltaPercent, "three_skill_item") / GetThreeSkillLevelAdjust(pChr->GetLevel()) >= number(1, iRandRange))
	{
		const uint32_t ITEM_VNUM = 50513;

		if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
			vec_item.push_back(item);
	}
	
	if (GetDropPerKillPct(100, 1000, iDeltaPercent, "dragon_boat_festival_drop") >= number(1, iRandRange))
	{
		const uint32_t ITEM_SEED = 50085;

		if ((item = CreateItem(ITEM_SEED, 1, 0, true)))
			vec_item.push_back(item);
	}

	if (pKiller->GetLevel() >= 15 && quest::CQuestManager::GetInstance()->GetEventFlag("mars_drop"))
	{
		const uint32_t ITEM_HANIRON = 70035;
		int32_t iDropMultiply[MOB::RANK_MAX_NUM] =
		{
			50,
			30,
			5,
			1,
			0,
			0,
		};

		if (iDropMultiply[pChr->GetMobRank()] &&
				GetDropPerKillPct(1000, 1500, iDeltaPercent, "mars_drop") >= number(1, iRandRange) * iDropMultiply[pChr->GetMobRank()])
		{
			if ((item = CreateItem(ITEM_HANIRON, 1, 0, true)))
				vec_item.push_back(item);
		}
	}
}

uint32_t ITEM_MANAGER::GetRefineFromVnum(uint32_t dwVnum)
{
	auto it = m_map_ItemRefineFrom.find(dwVnum);
	if (it != m_map_ItemRefineFrom.end())
		return it->second;
	return 0;
}

const CSpecialItemGroup* ITEM_MANAGER::GetSpecialItemGroup(uint32_t dwVnum)
{
	auto it = m_map_pSpecialItemGroup.find(dwVnum);
	if (it != m_map_pSpecialItemGroup.end())
	{
		return it->second;
	}
	return NULL;
}

const CSpecialAttrGroup* ITEM_MANAGER::GetSpecialAttrGroup(uint32_t dwVnum)
{
	auto it = m_map_pSpecialAttrGroup.find(dwVnum);
	if (it != m_map_pSpecialAttrGroup.end())
	{
		return it->second;
	}
	return NULL;
}

uint32_t ITEM_MANAGER::GetMaskVnum(uint32_t dwVnum)
{
	TMapDW2DW::iterator it = m_map_new_to_ori.find (dwVnum);
	if (it != m_map_new_to_ori.end())
	{
		return it->second;
	}
	else
		return 0;
}

void ITEM_MANAGER::CopyAllAttrTo(LPITEM pOldItem, LPITEM pNewItem)
{
	if (pOldItem->IsAccessoryForSocket())
	{
		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			pNewItem->SetSocket(i, pOldItem->GetSocket(i));
		}
	}
	else
	{
		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			if (!pOldItem->GetSocket(i))
				break;
			else
				pNewItem->SetSocket(i, 1);
		}

		int32_t slot = 0;

		for (int32_t i = 0; i < ITEM::SOCKET_MAX_NUM; ++i)
		{
			int32_t socket = pOldItem->GetSocket(i);
			const int32_t ITEM_BROKEN_METIN_VNUM = 28960;
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pNewItem->SetSocket(slot++, socket);
		}

	}

	pOldItem->CopyAttributeTo(pNewItem);
}
