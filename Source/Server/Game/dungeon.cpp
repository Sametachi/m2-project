#include "stdafx.h"
#include <Core/Net/PacketsCG.hpp>
#include "dungeon.h"
#include "char.h"
#include "char_manager.h"
#include "party.h"
#include "affect.h"
#include "packet.h"
#include "desc.h"
#include "config.h"
#include "regen.h"
#include "start_position.h"
#include "item.h"
#include "item_manager.h"
#include "utils.h"
#include "questmanager.h"

CDungeon::CDungeon(IdType id, int32_t lOriginalMapIndex, int32_t lMapIndex)
	: m_id(id),
	m_lOrigMapIndex(lOriginalMapIndex),
	m_lMapIndex(lMapIndex),
	m_map_Area(SECTREE_MANAGER::GetInstance()->GetDungeonArea(lOriginalMapIndex))
{
	Initialize();
}

CDungeon::~CDungeon()
{
	if (m_pParty != nullptr)
	{
		m_pParty->SetDungeon_for_Only_party (nullptr);
	}
	ClearRegen();
	event_cancel(&deadEvent);
	event_cancel(&exit_all_event_);
	event_cancel(&jump_to_event_);
}

void CDungeon::Initialize()
{
	deadEvent = nullptr;
	exit_all_event_ = nullptr;
	jump_to_event_ = nullptr;
	regen_id_ = 0;

	m_iMobKill = 0;
	m_iStoneKill = 0;
	m_bUsePotion = false;
	m_bUseRevive = false;

	m_iMonsterCount = 0;

	m_bExitAllAtEliminate = false;
	m_bWarpAtEliminate = false;

	m_iWarpDelay = 0;
	m_lWarpMapIndex = 0;
	m_lWarpX = 0;
	m_lWarpY = 0;

	m_stRegenFile = "";

	m_pParty = nullptr;
}

void CDungeon::SetFlag(std::string name, int32_t value)
{
	auto it =  m_map_Flag.find(name);
	if (it != m_map_Flag.end())
		it->second = value;
	else
		m_map_Flag.insert(make_pair(name, value));
}

int32_t CDungeon::GetFlag(std::string name)
{
	auto it =  m_map_Flag.find(name);
	if (it != m_map_Flag.end())
		return it->second;
	else
		return 0;
}

struct FSendDestPosition
{
	FSendDestPosition(int32_t x, int32_t y)
	{
		p1.bHeader = HEADER_GC_DUNGEON;
		p1.subheader = DUNGEON_SUBHEADER_GC_DESTINATION_POSITION;
		p2.x = x;
		p2.y = y;
		p1.size = sizeof(p1)+sizeof(p2);
	}

	void operator()(LPCHARACTER ch)
	{
		ch->GetDesc()->BufferedPacket(&p1, sizeof(TPacketGCDungeon));
		ch->GetDesc()->Packet(&p2, sizeof(TPacketGCDungeonDestPosition));
	}

	TPacketGCDungeon p1;
	TPacketGCDungeonDestPosition p2;
};

void CDungeon::SendDestPositionToParty(LPPARTY pParty, int32_t x, int32_t y)
{
	if (m_map_pParty.find(pParty) == m_map_pParty.end())
	{
		SysLog("PARTY {} not in DUNGEON {}", pParty->GetLeaderPID(), m_lMapIndex);
		return;
	}

	FSendDestPosition f(x, y);
	pParty->ForEachNearMember(f);
}

struct FWarpToDungeon
{
	FWarpToDungeon(int32_t lMapIndex, LPDUNGEON d)
		: m_lMapIndex(lMapIndex), m_pDungeon(d)
		{
			LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);
			m_x = pSectreeMap->m_setting.posSpawn.x;
			m_y = pSectreeMap->m_setting.posSpawn.y; 
		}

	void operator () (LPCHARACTER ch)
	{
		ch->SaveExitLocation();
		ch->WarpSet(m_x, m_y, m_lMapIndex);
	}

	int32_t m_lMapIndex;
	int32_t m_x;
	int32_t m_y;
	LPDUNGEON m_pDungeon;
};

void CDungeon::Join(LPCHARACTER ch)
{
	if (SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex) == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	FWarpToDungeon(m_lMapIndex, this) (ch);
}

void CDungeon::JoinParty(LPPARTY pParty)
{
	pParty->SetDungeon(this);
	m_map_pParty.insert(std::make_pair(pParty,0));

	if (SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex) == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	FWarpToDungeon f(m_lMapIndex, this);
	pParty->ForEachOnlineMember(f);
}

void CDungeon::QuitParty(LPPARTY pParty)
{
	pParty->SetDungeon(nullptr);
	TPartyMap::iterator it = m_map_pParty.find(pParty);

	if (it != m_map_pParty.end())
		m_map_pParty.erase(it);
}

EVENTINFO(dungeon_id_info)
{
	CDungeon::IdType dungeon_id;

	dungeon_id_info() 
	: dungeon_id(0)
	{
	}
};

EVENTFUNC(dungeon_dead_event)
{
	dungeon_id_info* info = dynamic_cast<dungeon_id_info*>(event->info);
	
	if (info == nullptr)
	{
		SysLog("dungeon_dead_event> <Factor> Null pointer");
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Find(info->dungeon_id);
	if (pDungeon == nullptr) {
		return 0;
	}

	pDungeon->deadEvent = nullptr;

	CDungeonManager::GetInstance()->Destroy(info->dungeon_id);
	return 0;
}

void CDungeon::IncMember(LPCHARACTER ch)
{
	if (m_set_pCharacter.find(ch) == m_set_pCharacter.end())
		m_set_pCharacter.insert(ch);

	event_cancel(&deadEvent);
}

void CDungeon::DecMember(LPCHARACTER ch)
{
	auto it = m_set_pCharacter.find(ch);

	if (it == m_set_pCharacter.end()) {
		return;
	}

	m_set_pCharacter.erase(it);

	if (m_set_pCharacter.empty())
	{
		dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
		info->dungeon_id = m_id;

		event_cancel(&deadEvent);
		deadEvent = event_create(dungeon_dead_event, info, PASSES_PER_SEC(10));
	}
}

void CDungeon::IncPartyMember(LPPARTY pParty, LPCHARACTER ch)
{
	TPartyMap::iterator it = m_map_pParty.find(pParty);

	if (it != m_map_pParty.end())
		it->second++;
	else
		m_map_pParty.insert(std::make_pair(pParty,1));

	IncMember(ch);
}

void CDungeon::DecPartyMember(LPPARTY pParty, LPCHARACTER ch)
{
	TPartyMap::iterator it = m_map_pParty.find(pParty);

	if (it == m_map_pParty.end())
	{
		SysLog("cannot find party");
	}
	else
	{
		it->second--;

		if (it->second == 0)
			QuitParty(pParty);
	}

	DecMember(ch);
}

struct FWarpToPosition
{
	int32_t lMapIndex;
	int32_t x;
	int32_t y;
	FWarpToPosition(int32_t lMapIndex, int32_t x, int32_t y)
		: lMapIndex(lMapIndex), x(x), y(y)
		{}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER)) {
			return;
		}
		LPCHARACTER ch = (LPCHARACTER)ent;
		if (!ch->IsPC()) {
			return;
		}
		if (ch->GetMapIndex() == lMapIndex)
		{
			ch->Show(lMapIndex, x, y, 0);
			ch->Stop();
		}
		else
		{
			ch->WarpSet(x,y,lMapIndex);
		}
	}
};

struct FWarpToPositionForce
{
	int32_t lMapIndex;
	int32_t x;
	int32_t y;
	FWarpToPositionForce(int32_t lMapIndex, int32_t x, int32_t y)
		: lMapIndex(lMapIndex), x(x), y(y)
		{}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER)) {
			return;
		}
		LPCHARACTER ch = (LPCHARACTER)ent;
		if (!ch->IsPC()) {
			return;
		}
		ch->WarpSet(x,y,lMapIndex);
	}
};

void CDungeon::JumpAll(int32_t lFromMapIndex, int32_t x, int32_t y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(lFromMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", lFromMapIndex);
		return;
	}

	FWarpToPosition f(m_lMapIndex, x, y);

	pMap->for_each(f);
}

void CDungeon::WarpAll(int32_t lFromMapIndex, int32_t x, int32_t y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(lFromMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", lFromMapIndex);
		return;
	}

	FWarpToPositionForce f(m_lMapIndex, x, y);

	pMap->for_each(f);
}

void CDungeon::JumpParty(LPPARTY pParty, int32_t lFromMapIndex, int32_t x, int32_t y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(lFromMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", lFromMapIndex);
		return;
	}

	if (pParty->GetDungeon_for_Only_party() == nullptr)
	{
		if (m_pParty == nullptr)
		{
			m_pParty = pParty;
		}
		else if (m_pParty != pParty)
		{
			SysLog("Dungeon already has party. Another party cannot jump in dungeon : index {}", GetMapIndex());
			return;
		}
		pParty->SetDungeon_for_Only_party (this);
	}

	FWarpToPosition f(m_lMapIndex, x, y);

	pParty->ForEachOnMapMember(f, lFromMapIndex);
}

void CDungeon::SetPartyNull()
{
	m_pParty = nullptr;
}


void CDungeonManager::Destroy(CDungeon::IdType dungeon_id)
{
	PyLog("DUNGEON destroy : map index {}", dungeon_id);
	LPDUNGEON pDungeon = Find(dungeon_id);
	if (pDungeon == nullptr) {
		return;
	}
	m_map_pDungeon.erase(dungeon_id);

	int32_t lMapIndex = pDungeon->m_lMapIndex;
	m_map_pMapDungeon.erase(lMapIndex);

	uint32_t server_timer_arg = lMapIndex;
	quest::CQuestManager::GetInstance()->CancelServerTimers(server_timer_arg);

	SECTREE_MANAGER::GetInstance()->DestroyPrivateMap(lMapIndex);
	M2_DELETE(pDungeon);
}

LPDUNGEON CDungeonManager::Find(CDungeon::IdType dungeon_id)
{
	auto it = m_map_pDungeon.find(dungeon_id);
	if (it != m_map_pDungeon.end())
		return it->second;
	return NULL;
}

LPDUNGEON CDungeonManager::FindByMapIndex(int32_t lMapIndex)
{
	auto it = m_map_pMapDungeon.find(lMapIndex);
	if (it != m_map_pMapDungeon.end()) {
		return it->second;
	}
	return NULL;
}

LPDUNGEON CDungeonManager::Create(int32_t lOriginalMapIndex)
{
	uint32_t lMapIndex = SECTREE_MANAGER::GetInstance()->CreatePrivateMap(lOriginalMapIndex);

	if (!lMapIndex) 
	{
		PyLog("Fail to Create Dungeon : OrginalMapindex {} NewMapindex {}", lOriginalMapIndex, lMapIndex);
		return NULL;
	}

	CDungeon::IdType id = next_id_++;
	while (Find(id) != nullptr) {
		id = next_id_++;
	}

	LPDUNGEON pDungeon = M2_NEW CDungeon(id, lOriginalMapIndex, lMapIndex);
	if (!pDungeon)
	{
		SysLog("M2_NEW CDungeon failed");
		return NULL;
	}
	m_map_pDungeon.insert(std::make_pair(id, pDungeon));
	m_map_pMapDungeon.insert(std::make_pair(lMapIndex, pDungeon));

	return pDungeon;
}

CDungeonManager::CDungeonManager()
	: next_id_(0)
{
}

CDungeonManager::~CDungeonManager()
{
}

void CDungeon::UniqueSetMaxHP(const std::string& key, int32_t iMaxHP)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key : {}", key.c_str());
		return;
	}
	it->second->SetMaxHP(iMaxHP);
}

void CDungeon::UniqueSetHP(const std::string& key, int32_t iHP)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key : {}", key.c_str());
		return;
	}
	it->second->SetHP(iHP);
}

void CDungeon::UniqueSetDefGrade(const std::string& key, int32_t iGrade)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key : {}", key.c_str());
		return;
	}
	it->second->PointChange(POINT_DEF_GRADE,iGrade - it->second->GetPoint(POINT_DEF_GRADE));
}

void CDungeon::SpawnMoveUnique(const char* key, uint32_t vnum, const char* pos_from, const char* pos_to)
{
	TAreaMap::iterator it_to = m_map_Area.find(pos_to);
	if (it_to == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos_to);
		return;
	}

	TAreaMap::iterator it_from = m_map_Area.find(pos_from);
	if (it_from == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos_from);
		return;
	}

	TAreaInfo & ai = it_from->second;
	TAreaInfo & ai_to = it_to->second;
	int32_t dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	for (int32_t i=0;i<100;i++)
	{
		int32_t dx = number(ai.sx, ai.ex);
		int32_t dy = number(ai.sy, ai.ey);
		int32_t tx = number(ai_to.sx, ai_to.ex);
		int32_t ty = number(ai_to.sy, ai_to.ey);

		LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+dx, pSectreeMap->m_setting.iBaseY+dy, 0, false, dir);

		if (ch)
		{
			m_map_UniqueMob.insert(make_pair(std::string(key), ch));
			ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
			ch->SetDungeon(this);

			if (ch->Goto(pSectreeMap->m_setting.iBaseX+tx, pSectreeMap->m_setting.iBaseY+ty))
				ch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
		}
		else
		{
			SysLog("Cannot spawn at {} {}", pSectreeMap->m_setting.iBaseX+((ai.sx+ai.ex)>>1), pSectreeMap->m_setting.iBaseY+((ai.sy+ai.ey)>>1));
		}
	}

}

void CDungeon::SpawnUnique(const char* key, uint32_t vnum, const char* pos)
{
	TAreaMap::iterator it = m_map_Area.find(pos);
	if (it == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos);
		return;
	}

	TAreaInfo & ai = it->second;
	int32_t dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	for (int32_t i=0;i<100;i++)
	{
		int32_t dx = number(ai.sx, ai.ex);
		int32_t dy = number(ai.sy, ai.ey);

		LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+dx, pSectreeMap->m_setting.iBaseY+dy, 0, false, dir);

		if (ch)
		{
			m_map_UniqueMob.insert(make_pair(std::string(key), ch));
			ch->SetDungeon(this);
			ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
			break;
		}
		else
		{
			SysLog("Cannot spawn at {} {}", pSectreeMap->m_setting.iBaseX+((ai.sx+ai.ex)>>1), pSectreeMap->m_setting.iBaseY+((ai.sy+ai.ey)>>1));
		}
	}
}

void CDungeon::SetUnique(const char* key, uint32_t vid)
{
	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->Find(vid);
	if (ch)
	{
		m_map_UniqueMob.insert(make_pair(std::string(key), ch));
		ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	}
}

void CDungeon::SpawnStoneDoor(const char* key, const char* pos) 
{
	SpawnUnique(key, 13001, pos);
}

void CDungeon::SpawnWoodenDoor(const char* key, const char* pos)
{
	SpawnUnique(key, 13000, pos);
	UniqueSetMaxHP(key, 10000);
	UniqueSetHP(key, 10000);
	UniqueSetDefGrade(key, 300);
}

void CDungeon::PurgeUnique(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key or Dead: {}", key.c_str());
		return;
	}
	LPCHARACTER ch = it->second;
	m_map_UniqueMob.erase(it);
	M2_DESTROY_CHARACTER(ch);
}

void CDungeon::KillUnique(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key or Dead: {}", key.c_str());
		return;
	}
	LPCHARACTER ch = it->second;
	m_map_UniqueMob.erase(it);
	ch->Dead();
}

uint32_t CDungeon::GetUniqueVid(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key or Dead: {}", key.c_str());
		return 0;
	}
	LPCHARACTER ch = it->second;
	return ch->GetVID();
}

float CDungeon::GetUniqueHpPerc(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key : {}", key.c_str());
		return false;
	}
	return (100.f*it->second->GetHP())/it->second->GetMaxHP();
}

void CDungeon::DeadCharacter(LPCHARACTER ch)
{
	if (!ch->IsPC())
	{
		TUniqueMobMap::iterator it = m_map_UniqueMob.begin();
		while (it!=m_map_UniqueMob.end())
		{
			if (it->second == ch)
			{
				m_map_UniqueMob.erase(it);
				break;
			}
			++it;
		}
	}
}

bool CDungeon::IsUniqueDead(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);

	if (it == m_map_UniqueMob.end())
	{
		SysLog("Unknown Key or Dead : {}", key.c_str());
		return true;
	}

	return it->second->IsDead();
}

void CDungeon::Spawn(uint32_t vnum, const char* pos)
{
	TAreaMap::iterator it = m_map_Area.find(pos);

	if (it == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos);
		return;
	}

	TAreaInfo & ai = it->second;
	int32_t dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr)
	{
		SysLog("cannot find map by index {}", m_lMapIndex);
		return;
	}
	int32_t dx = number(ai.sx, ai.ex);
	int32_t dy = number(ai.sy, ai.ey);

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+dx, pSectreeMap->m_setting.iBaseY+dy, 0, false, dir);
	if (ch)
		ch->SetDungeon(this);
}

LPCHARACTER CDungeon::SpawnMob(uint32_t vnum, int32_t x, int32_t y, int32_t dir)
{
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return NULL;
	}
	PyLog("CDungeon::SpawnMob {} {} {}", vnum, x,  y);

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+x*100, pSectreeMap->m_setting.iBaseY+y*100, 0, false, dir == 0 ? -1 : (dir - 1) * 45);

	if (ch)
	{
		ch->SetDungeon(this);
		PyLog("CDungeon::SpawnMob name {}", ch->GetName());
	}

	return ch;
}

LPCHARACTER CDungeon::SpawnMob_ac_dir(uint32_t vnum, int32_t x, int32_t y, int32_t dir)
{
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return NULL;
	}
	PyLog("CDungeon::SpawnMob {} {} {}", vnum, x,  y);

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+x*100, pSectreeMap->m_setting.iBaseY+y*100, 0, false, dir);

	if (ch)
	{
		ch->SetDungeon(this);
		PyLog("CDungeon::SpawnMob name {}", ch->GetName());
	}

	return ch;
}

void CDungeon::SpawnNameMob(uint32_t vnum, int32_t x, int32_t y, const char* name)
{
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+x, pSectreeMap->m_setting.iBaseY+y, 0, false, -1);
	if (ch)
	{
		ch->SetName(name);
		ch->SetDungeon(this);
	}
}

void CDungeon::SpawnGotoMob(int32_t lFromX, int32_t lFromY, int32_t lToX, int32_t lToY)
{
	const int32_t MOB_GOTO_VNUM = 20039;

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}

	PyLog("SpawnGotoMob {} {} to {} {}", lFromX, lFromY, lToX, lToY);

	lFromX = pSectreeMap->m_setting.iBaseX+lFromX*100;
	lFromY = pSectreeMap->m_setting.iBaseY+lFromY*100;

	LPCHARACTER ch = CHARACTER_MANAGER::GetInstance()->SpawnMob(MOB_GOTO_VNUM, m_lMapIndex, lFromX, lFromY, 0, false, -1);

	if (ch)
	{
		char buf[30+1];
		snprintf(buf, sizeof(buf), ". %ld %ld", lToX, lToY);

		ch->SetName(buf);
		ch->SetDungeon(this);
	}
}

LPCHARACTER CDungeon::SpawnGroup(uint32_t vnum, int32_t x, int32_t y, float radius, bool bAggressive, int32_t count)
{
	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return NULL;
	}

	int32_t iRadius = (int32_t) radius;

	int32_t sx = pSectreeMap->m_setting.iBaseX + x - iRadius;
	int32_t sy = pSectreeMap->m_setting.iBaseY + y - iRadius;
	int32_t ex = sx + iRadius;
	int32_t ey = sy + iRadius;

	LPCHARACTER ch = nullptr;

	while (count--)
	{
		LPCHARACTER chLeader = CHARACTER_MANAGER::GetInstance()->SpawnGroup(vnum, m_lMapIndex, sx, sy, ex, ey, NULL, bAggressive, this);
		if (chLeader && !ch)
			ch = chLeader;
	}

	return ch;
}

void CDungeon::SpawnRegen(const char* filename, bool bOnce)
{
	if (!filename)
	{
		SysLog("CDungeon::SpawnRegen(filename=NULL, bOnce={}) - m_lMapIndex[{}]", bOnce, m_lMapIndex); 
		return;
	}

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (!pSectreeMap)
	{
		SysLog("CDungeon::SpawnRegen(filename={}, bOnce={}) - m_lMapIndex[{}]", filename, bOnce, m_lMapIndex); 
		return;
	}
	regen_do(filename, m_lMapIndex, pSectreeMap->m_setting.iBaseX, pSectreeMap->m_setting.iBaseY, this, bOnce);
}

void CDungeon::AddRegen(LPREGEN regen)
{
	regen->id = regen_id_++;
	m_regen.push_back(regen);
}

void CDungeon::ClearRegen()
{
	for (auto it = m_regen.begin(); it != m_regen.end(); ++it)
	{
		LPREGEN regen = *it;

		event_cancel(&regen->event);
		M2_DELETE(regen);
	}
	m_regen.clear();
}

bool CDungeon::IsValidRegen(LPREGEN regen, size_t regen_id) {
	auto it = std::find(m_regen.begin(), m_regen.end(), regen);
	if (it == m_regen.end()) {
		return false;
	}
	LPREGEN found = *it;
	return (found->id == regen_id);
}

void CDungeon::SpawnMoveGroup(uint32_t vnum, const char* pos_from, const char* pos_to, int32_t count)
{
	TAreaMap::iterator it_to = m_map_Area.find(pos_to);

	if (it_to == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos_to);
		return;
	}

	TAreaMap::iterator it_from = m_map_Area.find(pos_from);

	if (it_from == m_map_Area.end())
	{
		SysLog("Wrong position string : {}", pos_from);
		return;
	}

	TAreaInfo & ai = it_from->second;
	TAreaInfo & ai_to = it_to->second;
	int32_t dir = ai.dir;

	if (dir == -1)
		dir = number(0,359);

	LPSECTREE_MAP pSectreeMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pSectreeMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}

	while (count--)
	{
		int32_t tx = number(ai_to.sx, ai_to.ex)+pSectreeMap->m_setting.iBaseX;
		int32_t ty = number(ai_to.sy, ai_to.ey)+pSectreeMap->m_setting.iBaseY;
		CHARACTER_MANAGER::GetInstance()->SpawnMoveGroup(vnum, m_lMapIndex, pSectreeMap->m_setting.iBaseX+ai.sx, pSectreeMap->m_setting.iBaseY+ai.sy, pSectreeMap->m_setting.iBaseX+ai.ex, pSectreeMap->m_setting.iBaseY+ai.ey, tx, ty, NULL, true);
	}
}

namespace
{
	struct FKillSectree
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC() && !ch->IsPet())
					ch->Dead();
			}
		}
	};

	struct FPurgeSectree
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC() && !ch->IsPet()) {
					M2_DESTROY_CHARACTER(ch);
				}
			}
			else if (ent->IsType(ENTITY_ITEM))
			{
				LPITEM item = (LPITEM) ent;
				M2_DESTROY_ITEM(item);
			}
			else
				SysLog("unknown entity type {} is in dungeon", ent->GetType());
		}
	};
}

// DUNGEON_KILL_ALL_BUG_FIX
void CDungeon::KillAll()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	FKillSectree f;
	pMap->for_each(f);
}
// END_OF_DUNGEON_KILL_ALL_BUG_FIX

void CDungeon::Purge()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);
	if (pMap == nullptr) {
		SysLog("CDungeon: SECTREE_MAP not found for #{}", m_lMapIndex);
		return;
	}
	FPurgeSectree f;
	pMap->for_each(f);
}

void CDungeon::IncKillCount(LPCHARACTER pKiller, LPCHARACTER pVictim)
{
	if (pVictim->IsStone())
		m_iStoneKill ++;
	else
		m_iMobKill ++;
}

void CDungeon::UsePotion(LPCHARACTER ch)
{
	m_bUsePotion = true;
}

void CDungeon::UseRevive(LPCHARACTER ch)
{
	m_bUseRevive = true;
}

bool CDungeon::IsUsePotion()
{
	return m_bUsePotion;
}

bool CDungeon::IsUseRevive()
{
	return m_bUseRevive;
}

int32_t CDungeon::GetKillMobCount()
{
	return m_iMobKill;
}
int32_t CDungeon::GetKillStoneCount()
{
	return m_iStoneKill;
}

struct FCountMonster
{
	int32_t n;
	FCountMonster() : n(0) {};
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (!ch->IsPC())
				n++;
		}
	}
};

int32_t CDungeon::CountRealMonster()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lOrigMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", m_lOrigMapIndex);
		return 0;
	}

	FCountMonster f;

	pMap->for_each(f);
	return f.n;
}

struct FExitDungeon
{
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
				ch->ExitToSavedLocation();
		}
	}
};

void CDungeon::ExitAll()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", m_lMapIndex);
		return;
	}

	FExitDungeon f;

	pMap->for_each(f);
}

// DUNGEON_NOTICE
namespace
{
	struct FNotice
	{
		FNotice(const char* psz) : m_psz(psz)
		{
		}

		void operator() (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
				ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
			}
		}

		const char* m_psz;
	};
}

void CDungeon::Notice(const char* msg)
{
	PyLog("XXX Dungeon {} Notice {}", m_id, msg);
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", m_lMapIndex);
		return;
	}

	FNotice f(msg);
	pMap->for_each(f);
}
// END_OF_DUNGEON_NOTICE

struct FExitDungeonToStartPosition
{
	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
			{
				PIXEL_POSITION posWarp;

				if (SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(g_start_map[ch->GetEmpire()], ch->GetEmpire(), posWarp))
					ch->WarpSet(posWarp.x, posWarp.y);
				else
					ch->ExitToSavedLocation();
			}
		}
	}
};

void CDungeon::ExitAllToStartPosition()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", m_lMapIndex);
		return;
	}

	FExitDungeonToStartPosition f;

	pMap->for_each(f);
}

EVENTFUNC(dungeon_jump_to_event)
{
	dungeon_id_info * info = dynamic_cast<dungeon_id_info *>(event->info);

	if (info == nullptr)
	{
		SysLog("dungeon_jump_to_event> <Factor> Null pointer");
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Find(info->dungeon_id);
	pDungeon->jump_to_event_ = nullptr;

	if (pDungeon)
		pDungeon->JumpToEliminateLocation();
	else
		SysLog("cannot find dungeon with map index {}", info->dungeon_id);

	return 0;
}

EVENTFUNC(dungeon_exit_all_event)
{
	dungeon_id_info * info = dynamic_cast<dungeon_id_info *>(event->info);

	if (info == nullptr)
	{
		SysLog("dungeon_exit_all_event> <Factor> Null pointer");
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::GetInstance()->Find(info->dungeon_id);
	pDungeon->exit_all_event_ = nullptr;

	if (pDungeon)
		pDungeon->ExitAll();

	return 0;
}

void CDungeon::CheckEliminated()
{
	if (m_iMonsterCount > 0)
		return;

	if (m_bExitAllAtEliminate)
	{
		PyLog("CheckEliminated: exit");
		m_bExitAllAtEliminate = false;

		if (m_iWarpDelay)
		{
			dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
			info->dungeon_id = m_id;

			event_cancel(&exit_all_event_);
			exit_all_event_ = event_create(dungeon_exit_all_event, info, PASSES_PER_SEC(m_iWarpDelay));
		}
		else
		{
			ExitAll();
		}
	}
	else if (m_bWarpAtEliminate)
	{
		PyLog("CheckEliminated: warp");
		m_bWarpAtEliminate = false;

		if (m_iWarpDelay)
		{
			dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
			info->dungeon_id = m_id;

			event_cancel(&jump_to_event_);
			jump_to_event_ = event_create(dungeon_jump_to_event, info, PASSES_PER_SEC(m_iWarpDelay));
		}
		else
		{
			JumpToEliminateLocation();
		}
	}
	else
		PyLog("CheckEliminated: none");
}

void CDungeon::SetExitAllAtEliminate(int32_t time)
{
	PyLog("SetExitAllAtEliminate: time {}", time);
	m_bExitAllAtEliminate = true;
	m_iWarpDelay = time;
}

void CDungeon::SetWarpAtEliminate(int32_t time, int32_t lMapIndex, int32_t x, int32_t y, const char* regen_file)
{
	m_bWarpAtEliminate = true;
	m_iWarpDelay = time;
	m_lWarpMapIndex = lMapIndex;
	m_lWarpX = x;
	m_lWarpY = y;

	if (!regen_file || !*regen_file)
		m_stRegenFile.clear();
	else
		m_stRegenFile = regen_file;

	PyLog("SetWarpAtEliminate: time {} map {} {}x{} regenfile {}", time, lMapIndex, x, y, m_stRegenFile.c_str());
}

void CDungeon::JumpToEliminateLocation()
{
	LPDUNGEON pDungeon = CDungeonManager::GetInstance()->FindByMapIndex(m_lWarpMapIndex);

	if (pDungeon)
	{
		pDungeon->JumpAll(m_lMapIndex, m_lWarpX, m_lWarpY);

		if (!m_stRegenFile.empty())
		{
			pDungeon->SpawnRegen(m_stRegenFile.c_str());
			m_stRegenFile.clear();
		}
	}
	else
	{
		LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);

		if (!pMap)
		{
			SysLog("no map by index {}", m_lMapIndex);
			return;
		}

		FWarpToPosition f(m_lWarpMapIndex, m_lWarpX * 100, m_lWarpY * 100);

		pMap->for_each(f);
	}
}

struct FNearPosition
{
	int32_t x;
	int32_t y;
	int32_t dist;
	bool ret;

	FNearPosition(int32_t x, int32_t y, int32_t d) :
		x(x), y(y), dist(d), ret(true)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (ret == false)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
			{
				if (DISTANCE_APPROX(ch->GetX() - x * 100, ch->GetY() - y * 100) > dist * 100)
					ret = false;
			}
		}
	}
};

bool CDungeon::IsAllPCNearTo(int32_t x, int32_t y, int32_t dist)
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::GetInstance()->GetMap(m_lMapIndex);

	if (!pMap)
	{
		SysLog("cannot find map by index {}", m_lMapIndex);
		return false;
	}

	FNearPosition f(x, y, dist);

	pMap->for_each(f);

	return f.ret;
}

void CDungeon::CreateItemGroup (std::string& group_name, ItemGroup& item_group)
{
	m_map_ItemGroup.insert (ItemGroupMap::value_type (group_name, item_group));
}

const CDungeon::ItemGroup* CDungeon::GetItemGroup (std::string& group_name)
{
	ItemGroupMap::iterator it = m_map_ItemGroup.find (group_name);
	if (it != m_map_ItemGroup.end())
		return &(it->second);
	else
		return NULL;
}
