#include "stdafx.h"
#include "constants.h"
#include "sectree_manager.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "config.h"
#include "packet.h"
#include "char.h"
#include "char_manager.h"
#include "guild.h"
#include "guild_manager.h"
#include "desc.h"
#include "desc_manager.h"
#include "desc_client.h"
#include "questmanager.h"
#include "building.h"

enum
{
	// ADD_SUPPLY_BUILDING
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL = 14061,
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM = 14062,
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE = 14063,
	// END_OF_ADD_SUPPLY_BUILDING

	FLAG_VNUM = 14200,
	WALL_DOOR_VNUM	= 14201,
	WALL_BACK_VNUM	= 14202,
	WALL_LEFT_VNUM	= 14203,
	WALL_RIGHT_VNUM	= 14204,
};

using namespace building;

CObject::CObject(TObject* pData, TObjectProto* pProto)
	: m_pProto(pProto), m_dwVID(0), m_chNPC(nullptr)
{
	CEntity::Initialize(ENTITY_OBJECT);

	memcpy(&m_data, pData, sizeof(TObject));
}

CObject::~CObject()
{
	Destroy();
}

void CObject::Destroy()
{
	if (m_pProto)
	{
		SECTREE_MANAGER::GetInstance()->ForAttrRegion(GetMapIndex(),
				GetX() + m_pProto->lRegion[0],
				GetY() + m_pProto->lRegion[1],
				GetX() + m_pProto->lRegion[2],
				GetY() + m_pProto->lRegion[3],
				(int32_t)m_data.zRot, // ADD_BUILDING_ROTATION
				ATTR_OBJECT,
				ATTR_REGION_MODE_REMOVE);
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	// <Factor> NPC should be destroyed in CHARACTER_MANAGER
	// BUILDING_NPC
	/*
	if (m_chNPC) {
		M2_DESTROY_CHARACTER(m_chNPC);
	}
	*/

	RemoveSpecialEffect();
	// END_OF_BUILDING_NPC
}

// BUILDING_NPC
void CObject::Reconstruct(uint32_t dwVnum)
{
	const TMapRegion * r = SECTREE_MANAGER::GetInstance()->GetMapRegion(m_data.lMapIndex);
	if (!r)
		return;

	CLand* pLand = GetLand();
	pLand->RequestDeleteObject(GetID());
	pLand->RequestCreateObject(dwVnum, m_data.lMapIndex, m_data.x - r->sx, m_data.y - r->sy, m_data.xRot, m_data.yRot, m_data.zRot, false);
}
// END_OF_BUILDING_NPC

void CObject::EncodeInsertPacket(LPENTITY entity)
{
	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	PyLog("ObjectInsertPacket vid {} vnum {} rot {} {} {}", 
			m_dwVID, m_data.dwVnum, m_data.xRot, m_data.yRot, m_data.zRot);

	TPacketGCCharacterAdd pack;

	memset(&pack, 0, sizeof(TPacketGCCharacterAdd));

	pack.header         = HEADER_GC_CHARACTER_ADD;
	pack.dwVID          = m_dwVID;
	pack.bType          = CHAR_TYPE_BUILDING;
	pack.angle          = m_data.zRot;
	pack.x              = GetX();
	pack.y              = GetY();
	pack.z              = GetZ();
	pack.wRaceNum       = m_data.dwVnum;

	// Convert building rotation information (door position in case of wall)
	pack.dwAffectFlag[0] = unsigned(m_data.xRot);
	pack.dwAffectFlag[1] = unsigned(m_data.yRot);


	if (GetLand())
	{
		// pack.dwGuild = GetLand()->GetOwner();
	}

	d->Packet(&pack, sizeof(pack));
}

void CObject::EncodeRemovePacket(LPENTITY entity)
{
	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	PyLog("ObjectRemovePacket vid {}", m_dwVID);

	TPacketGCCharacterDelete pack;

	pack.header = HEADER_GC_CHARACTER_DEL;
	pack.id     = m_dwVID;

	d->Packet(&pack, sizeof(TPacketGCCharacterDelete));
}

void CObject::SetVID(uint32_t dwVID)
{
	m_dwVID = dwVID;
}

bool CObject::Show(int32_t lMapIndex, int32_t x, int32_t y)
{
	LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);

	if (!tree)
	{
		SysLog("cannot find sectree by {}x{} mapindex {}", x, y, lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
	}

	m_data.lMapIndex = lMapIndex;
	m_data.x = x;
	m_data.y = y;

	Save();

	SetMapIndex(lMapIndex);
	SetXYZ(x, y, 0);

	tree->InsertEntity(this);
	UpdateSectree();

	SECTREE_MANAGER::GetInstance()->ForAttrRegion(lMapIndex,
			x + m_pProto->lRegion[0],
			y + m_pProto->lRegion[1],
			x + m_pProto->lRegion[2],
			y + m_pProto->lRegion[3],
			(int32_t)m_data.zRot,
			ATTR_OBJECT,
			ATTR_REGION_MODE_SET);

	return true;
}

void CObject::Save()
{
}

void CObject::ApplySpecialEffect()
{
	if (m_pProto)
	{
		// ADD_SUPPLY_BUILDING
		if (m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL ||
				m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM ||
				m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE)
		{
			CLand* pLand = GetLand();
			uint32_t guild_id = 0;
			if (pLand)
				guild_id = pLand->GetOwner();
			CGuild* pGuild = CGuildManager::GetInstance()->FindGuild(guild_id);
			if (pGuild)
			{
				switch (m_pProto->dwVnum)
				{
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL:
						pGuild->SetMemberCountBonus(6);
						break;
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM:
						pGuild->SetMemberCountBonus(12);
						break;
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE:
						pGuild->SetMemberCountBonus(18);
						break;
				}
				if (map_allow_find(pLand->GetMapIndex()))
				{
					pGuild->BroadcastMemberCountBonus();
				}
			}
		}
		// END_OF_ADD_SUPPLY_BUILDING
	}
}

void CObject::RemoveSpecialEffect()
{
	if (m_pProto)
	{
		// ADD_SUPPLY_BUILDING
		if (m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL ||
				m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM ||
				m_pProto->dwVnum == BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE)
		{
			CLand* pLand = GetLand();
			uint32_t guild_id = 0;
			if (pLand)
				guild_id = pLand->GetOwner();
			CGuild* pGuild = CGuildManager::GetInstance()->FindGuild(guild_id);
			if (pGuild)
			{
				pGuild->SetMemberCountBonus(0);
				if (map_allow_find(pLand->GetMapIndex()))
					pGuild->BroadcastMemberCountBonus();
			}
		}
		// END_OF_ADD_SUPPLY_BUILDING
	}
}

// BUILDING_NPC
void CObject::RegenNPC()
{
	if (!m_pProto)
		return;

	if (!m_pProto->dwNPCVnum)
		return;

	if (!m_pLand)
		return;

	uint32_t dwGuildID = m_pLand->GetOwner();
	CGuild* pGuild = CGuildManager::GetInstance()->FindGuild(dwGuildID);

	if (!pGuild)
		return;

	int32_t x = m_pProto->lNPCX;
	int32_t y = m_pProto->lNPCY;
	int32_t newX, newY;

	float rot = m_data.zRot * 2.0f * M_PI / 360.0f;

	newX = (int32_t)((x * cosf(rot)) + (y * sinf(rot)));
	newY = (int32_t)((y * cosf(rot)) - (x * sinf(rot)));

	m_chNPC = CHARACTER_MANAGER::GetInstance()->SpawnMob(m_pProto->dwNPCVnum,
			GetMapIndex(),
			GetX() + newX,
			GetY() + newY,
			GetZ(),
			false,
			(int32_t)m_data.zRot);


	if (!m_chNPC)
	{
		SysLog("Cannot create guild npc");
		return;
	}

	m_chNPC->SetGuild(pGuild);

	// In the case of a temple of power, the guild level is stored in Gilma.
	if (m_pProto->dwVnum == 14061 || m_pProto->dwVnum == 14062 || m_pProto->dwVnum == 14063)
	{
		quest::PC* pPC = quest::CQuestManager::GetInstance()->GetPC(pGuild->GetMasterPID());

		if (pPC != nullptr)
		{
			pPC->SetFlag("alter_of_power.build_level", pGuild->GetLevel());
		}
	}
}
// END_OF_BUILDING_NPC

////////////////////////////////////////////////////////////////////////////////////

CLand::CLand(TLand* pData)
{
	memcpy(&m_data, pData, sizeof(TLand));
}

CLand::~CLand()
{
	Destroy();
}

void CLand::Destroy()
{
	auto it = m_map_pObject.begin();

	while (it != m_map_pObject.end())
	{
		LPOBJECT pObj = (it++)->second;
		CManager::GetInstance()->UnregisterObject(pObj);
		M2_DELETE(pObj);
	}

	m_map_pObject.clear();
	m_map_pObjectByVID.clear();
}

const TLand & CLand::GetData()
{
	return m_data;
}

void CLand::PutData(const TLand * data)
{
	memcpy(&m_data, data, sizeof(TLand));

	if (m_data.dwGuildID)
	{
		const TMapRegion * r = SECTREE_MANAGER::GetInstance()->GetMapRegion(m_data.lMapIndex);

		if (r)
		{
			CharacterVectorInteractor i;

			if (CHARACTER_MANAGER::GetInstance()->GetCharactersByRaceNum(20040, i))
			{
				CharacterVectorInteractor::iterator it = i.begin();

				while (it != i.end())
				{
					LPCHARACTER ch = *(it++);

					if (ch->GetMapIndex() != m_data.lMapIndex)
						continue;

					int32_t x = ch->GetX() - r->sx;
					int32_t y = ch->GetY() - r->sy;

					if (x > m_data.x + m_data.width || x < m_data.x)
						continue;

					if (y > m_data.y + m_data.height || y < m_data.y)
						continue;

					M2_DESTROY_CHARACTER(ch);
				}
			}
		}
	}
}

void CLand::InsertObject(LPOBJECT pObj)
{
	m_map_pObject.insert(std::make_pair(pObj->GetID(), pObj));
	m_map_pObjectByVID.insert(std::make_pair(pObj->GetVID(), pObj));

	pObj->SetLand(this);
}

LPOBJECT CLand::FindObject(uint32_t dwID)
{
	std::map<uint32_t, LPOBJECT>::iterator it = m_map_pObject.find(dwID);

	if (it == m_map_pObject.end())
		return NULL;

	return it->second;
}

LPOBJECT CLand::FindObjectByGroup(uint32_t dwGroupVnum)
{
	std::map<uint32_t, LPOBJECT>::iterator it;
	for (it = m_map_pObject.begin(); it != m_map_pObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetGroup() == dwGroupVnum)
			return pObj;
	}

	return NULL;
}

LPOBJECT CLand::FindObjectByVnum(uint32_t dwVnum)
{
	std::map<uint32_t, LPOBJECT>::iterator it;
	for (it = m_map_pObject.begin(); it != m_map_pObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetVnum() == dwVnum)
			return pObj;
	}

	return NULL;
}

// BUILDING_NPC
LPOBJECT CLand::FindObjectByNPC(LPCHARACTER npc)
{
	if (!npc)
		return NULL;

	std::map<uint32_t, LPOBJECT>::iterator it;
	for (it = m_map_pObject.begin(); it != m_map_pObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetNPC() == npc)
			return pObj;
	}

	return NULL;
}
// END_OF_BUILDING_NPC

LPOBJECT CLand::FindObjectByVID(uint32_t dwVID)
{
	std::map<uint32_t, LPOBJECT>::iterator it = m_map_pObjectByVID.find(dwVID);

	if (it == m_map_pObjectByVID.end())
		return NULL;

	return it->second;
}

void CLand::DeleteObject(uint32_t dwID)
{
	LPOBJECT pObj;

	if (!(pObj = FindObject(dwID)))
		return;

	PyLog("Land::DeleteObject {}", dwID);
	CManager::GetInstance()->UnregisterObject(pObj);
	M2_DESTROY_CHARACTER (pObj->GetNPC());

	m_map_pObject.erase(dwID);
	m_map_pObjectByVID.erase(dwID);

	M2_DELETE(pObj);
}

struct FIsIn
{
	int32_t sx, sy;
	int32_t ex, ey;
	
	bool bIn;
	FIsIn (	int32_t sx_, int32_t sy_, int32_t ex_, int32_t ey_)
		: sx(sx_), sy(sy_), ex(ex_), ey(ey_), bIn(false) 
	{}

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsMonster())
			{
				return;
			}
			if (sx <= ch->GetX() && ch->GetX() <= ex
				&& sy <= ch->GetY() && ch->GetY() <= ey)
			{
				bIn = true;
			}
		}
	}
};

bool CLand::RequestCreateObject(uint32_t dwVnum, int32_t lMapIndex, int32_t x, int32_t y, float xRot, float yRot, float zRot, bool checkAnother)
{
	auto rkSecTreeMgr = SECTREE_MANAGER::GetInstance();
	TObjectProto* pProto = CManager::GetInstance()->GetObjectProto(dwVnum);

	if (!pProto)
	{
		SysLog("Invalid Object vnum {}", dwVnum);
		return false;
	}
	const TMapRegion * r = rkSecTreeMgr->GetMapRegion(lMapIndex);
	if (!r)
		return false;

	PyLog("RequestCreateObject(vnum={}, map={}, pos=({},{}), rot=(%.1f,%.1f,%.1f) region({},{} ~ {},{})", 
			dwVnum, lMapIndex, x, y, xRot, yRot, zRot, r->sx, r->sy, r->ex, r->ey);

	x += r->sx;
	y += r->sy;

	int32_t sx = r->sx + m_data.x;
	int32_t ex = sx + m_data.width;
	int32_t sy = r->sy + m_data.y;
	int32_t ey = sy + m_data.height;

	int32_t osx = x + pProto->lRegion[0];
	int32_t osy = y + pProto->lRegion[1];
	int32_t oex = x + pProto->lRegion[2];
	int32_t oey = y + pProto->lRegion[3];

	float rad = zRot * 2.0f * M_PI / 360.0f;

	int32_t tsx = (int32_t)(pProto->lRegion[0] * cosf(rad) + pProto->lRegion[1] * sinf(rad) + x);
	int32_t tsy = (int32_t)(pProto->lRegion[0] * -sinf(rad) + pProto->lRegion[1] * cosf(rad) + y);

	int32_t tex = (int32_t)(pProto->lRegion[2] * cosf(rad) + pProto->lRegion[3] * sinf(rad) + x);
	int32_t tey = (int32_t)(pProto->lRegion[2] * -sinf(rad) + pProto->lRegion[3] * cosf(rad) + y);

	if (tsx < sx || tex > ex || tsy < sy || tey > ey)
	{
		SysLog("invalid position: object is outside of land region\nLAND: {} {} ~ {} {}\nOBJ: {} {} ~ {} {}", sx, sy, ex, ey, osx, osy, oex, oey);
		return false;
	}

	// ADD_BUILDING_ROTATION
	if (checkAnother)
	{
		if (rkSecTreeMgr->ForAttrRegion(lMapIndex, osx, osy, oex, oey, (int32_t)zRot, ATTR_OBJECT, ATTR_REGION_MODE_CHECK))
		{
			SysLog("another object already exist");
			return false;
		}
		FIsIn f (osx, osy, oex, oey);
		rkSecTreeMgr->GetMap(lMapIndex)->for_each(f);
		
		if (f.bIn)
		{
			SysLog("another object already exist");
			return false;
		}
	}
	// END_OF_BUILDING_NPC

	TPacketGDCreateObject p;

	p.dwVnum = dwVnum;
	p.dwLandID = m_data.dwID;
	p.lMapIndex = lMapIndex;
	p.x = x;
	p.y = y;
	p.xRot = xRot;
	p.yRot = yRot;
	p.zRot = zRot;

	db_clientdesc->DBPacket(HEADER_GD_CREATE_OBJECT, 0, &p, sizeof(TPacketGDCreateObject));
	return true;
}

void CLand::RequestDeleteObject(uint32_t dwID)
{
	if (!FindObject(dwID))
	{
		SysLog("no object by id {}", dwID);
		return;
	}

	db_clientdesc->DBPacket(HEADER_GD_DELETE_OBJECT, 0, &dwID, sizeof(uint32_t));
	PyLog("RequestDeleteObject id {}", dwID);
}

void CLand::RequestDeleteObjectByVID(uint32_t dwVID)
{
	LPOBJECT pObj;

	if (!(pObj = FindObjectByVID(dwVID)))
	{
		SysLog("no object by vid {}", dwVID);
		return;
	}

	uint32_t dwID = pObj->GetID();
	db_clientdesc->DBPacket(HEADER_GD_DELETE_OBJECT, 0, &dwID, sizeof(uint32_t));
	PyLog("RequestDeleteObject vid {} id {}", dwVID, dwID);
}

void CLand::SetOwner(uint32_t dwGuild)
{
	if (m_data.dwGuildID != dwGuild)
	{
		m_data.dwGuildID = dwGuild;
		RequestUpdate(dwGuild);
	}
}

void CLand::RequestUpdate(uint32_t dwGuild)
{
	uint32_t a[2];

	a[0] = GetID();
	a[1] = dwGuild;

	db_clientdesc->DBPacket(HEADER_GD_UPDATE_LAND, 0, &a[0], sizeof(uint32_t) * 2);
	PyLog("RequestUpdate id {} guild {}", a[0], a[1]);
}

////////////////////////////////////////////////////////////////////////////////////

CManager::CManager()
{
}

CManager::~CManager()
{
	Destroy();
}

void CManager::Destroy()
{
	auto it = m_map_pLand.begin();
	for (; it != m_map_pLand.end(); ++it) {
		M2_DELETE(it->second);
	}
	m_map_pLand.clear();
}

bool CManager::LoadObjectProto(const TObjectProto* pProto, int32_t size) // from DB
{
	m_vec_kObjectProto.resize(size);
	memcpy(&m_vec_kObjectProto[0], pProto, sizeof(TObjectProto) * size);

	for (int32_t i = 0; i < size; ++i)
	{
		TObjectProto& r = m_vec_kObjectProto[i];

		// BUILDING_NPC
		PyLog("ObjectProto {} price {} upgrade {} upg_limit {} life {} NPC {}",
				r.dwVnum, r.dwPrice, r.dwUpgradeVnum, r.dwUpgradeLimitTime, r.lLife, r.dwNPCVnum);
		// END_OF_BUILDING_NPC

		for (int32_t j = 0; j < OBJECT_MATERIAL_MAX_NUM; ++j)
		{
			if (!r.kMaterials[j].dwItemVnum)
				break;

			if (!ITEM_MANAGER::GetInstance()->GetTable(r.kMaterials[j].dwItemVnum))
			{
				SysLog("          mat: ERROR!! no item by vnum {}", r.kMaterials[j].dwItemVnum);
				return false;
			}

			PyLog("          mat: {} {}", r.kMaterials[j].dwItemVnum, r.kMaterials[j].dwCount);
		}

		m_map_pObjectProto.insert(std::make_pair(r.dwVnum, &m_vec_kObjectProto[i]));
	}

	return true;
}

TObjectProto * CManager::GetObjectProto(uint32_t dwVnum)
{
	auto it = m_map_pObjectProto.find(dwVnum);

	if (it == m_map_pObjectProto.end())
		return NULL;

	return it->second;
}

bool CManager::LoadLand(TLand* pTable) // from DB
{
	// Even if the land on the map is not in MapAllow, we need to load it.
	// To find out which guild the building (object) belongs to, find out which guild the building is located in.
	// If the land is not loaded, the guild building does not know which guild it belongs to.
	// Do not receive guild buffs from guild buildings.
	
	//if (!map_allow_find(pTable->lMapIndex))
	//	return false;

	CLand* pLand = M2_NEW CLand(pTable);
	m_map_pLand.insert(std::make_pair(pLand->GetID(), pLand));

	PyLog("LAND: {} map {} {}x{} w {} h {}", 
			pTable->dwID, pTable->lMapIndex, pTable->x, pTable->y, pTable->width, pTable->height);

	return true;
}

void CManager::UpdateLand(TLand* pTable)
{
	CLand* pLand = FindLand(pTable->dwID);

	if (!pLand)
	{
		SysLog("cannot find land by id {}", pTable->dwID);
		return;
	}

	pLand->PutData(pTable);

	const DESC_MANAGER::DESC_SET & cont = DESC_MANAGER::GetInstance()->GetClientSet();

	auto it = cont.begin();

	TPacketGCLandList p;

	p.header = HEADER_GC_LAND_LIST;
	p.size = sizeof(TPacketGCLandList) + sizeof(TLandPacketElement);

	TLandPacketElement e;

	e.dwID = pTable->dwID;
	e.x = pTable->x;
	e.y = pTable->y;
	e.width = pTable->width;
	e.height = pTable->height;
	e.dwGuildID = pTable->dwGuildID;

	PyLog("BUILDING: UpdateLand {} pos {}x{} guild {}", e.dwID, e.x, e.y, e.dwGuildID);

	CGuild *guild = CGuildManager::GetInstance()->FindGuild(pTable->dwGuildID);
	while (it != cont.end())
	{
		LPDESC d = *(it++);

		if (d->GetCharacter() && d->GetCharacter()->GetMapIndex() == pTable->lMapIndex)
		{
			// we must send the guild name first
			d->GetCharacter()->SendGuildName(guild);

			d->BufferedPacket(&p, sizeof(TPacketGCLandList));
			d->Packet(&e, sizeof(TLandPacketElement));
		}
	}
}

CLand * CManager::FindLand(uint32_t dwID)
{
	std::map<uint32_t, CLand *>::iterator it = m_map_pLand.find(dwID);

	if (it == m_map_pLand.end())
		return NULL;

	return it->second;
}

CLand * CManager::FindLand(int32_t lMapIndex, int32_t x, int32_t y)
{
	PyLog("BUILDING: FindLand {} {} {}", lMapIndex, x, y);

	const TMapRegion * r = SECTREE_MANAGER::GetInstance()->GetMapRegion(lMapIndex);

	if (!r)
		return NULL;

	x -= r->sx;
	y -= r->sy;

	auto it = m_map_pLand.begin();

	while (it != m_map_pLand.end())
	{
		CLand* pLand = (it++)->second;
		const TLand& r = pLand->GetData();

		if (r.lMapIndex != lMapIndex)
			continue;

		if (x < r.x || y < r.y)
			continue;

		if (x > r.x + r.width || y > r.y + r.height)
			continue;

		return pLand;
	}

	return NULL;
}

CLand * CManager::FindLandByGuild(uint32_t GID)
{
	auto it = m_map_pLand.begin();

	while (it != m_map_pLand.end())
	{
		CLand* pLand = (it++)->second;

		if (pLand->GetData().dwGuildID == GID)
			return pLand;
	}

	return NULL;
}

bool CManager::LoadObject(TObject* pTable, bool isBoot) // from DB
{
	CLand* pLand = FindLand(pTable->dwLandID);

	if (!pLand)
	{
		PyLog("Cannot find land by id {}", pTable->dwLandID);
		return false;
	}

	TObjectProto* pProto = GetObjectProto(pTable->dwVnum);

	if (!pProto)
	{
		SysLog("Cannot find object {} in prototype (id {})", pTable->dwVnum, pTable->dwID);
		return false;
	}

	PyLog("OBJ: id {} vnum {} map {} pos {}x{}", pTable->dwID, pTable->dwVnum, pTable->lMapIndex, pTable->x, pTable->y);

	LPOBJECT pObj = M2_NEW CObject(pTable, pProto);

	uint32_t dwVID = CHARACTER_MANAGER::GetInstance()->AllocVID();
	pObj->SetVID(dwVID);

	m_map_pObjByVID.insert(std::make_pair(dwVID, pObj));
	m_map_pObjByID.insert(std::make_pair(pTable->dwID, pObj));

	pLand->InsertObject(pObj);

	if (!isBoot)
		pObj->Show(pTable->lMapIndex, pTable->x, pTable->y);
	else
	{
		pObj->SetMapIndex(pTable->lMapIndex);
		pObj->SetXYZ(pTable->x, pTable->y, 0);
	}

	// BUILDING_NPC
	if (!isBoot)
	{ 
		if (pProto->dwNPCVnum)
			pObj->RegenNPC();

		pObj->ApplySpecialEffect();
	}
	// END_OF_BUILDING_NPC

	return true;
}

void CManager::FinalizeBoot()
{
	auto it = m_map_pObjByID.begin();

	while (it != m_map_pObjByID.end())
	{
		LPOBJECT pObj = (it++)->second;

		pObj->Show(pObj->GetMapIndex(), pObj->GetX(), pObj->GetY());
		// BUILDING_NPC
		pObj->RegenNPC();
		pObj->ApplySpecialEffect();
		// END_OF_BUILDING_NPC
	}

	// BUILDING_NPC
	PyLog("FinalizeBoot");
	// END_OF_BUILDING_NPC

	auto it2 = m_map_pLand.begin();

	while (it2 != m_map_pLand.end())
	{
		CLand* pLand = (it2++)->second;

		const TLand& r = pLand->GetData();

		// LAND_MASTER_LOG	
		PyLog("LandMaster map_index={} pos=({}, {})", r.lMapIndex, r.x, r.y);
		// END_OF_LAND_MASTER_LOG

		if (r.dwGuildID != 0)
			continue;

		if (!map_allow_find(r.lMapIndex))
			continue;

		const TMapRegion * region = SECTREE_MANAGER::GetInstance()->GetMapRegion(r.lMapIndex);
		if (!region)
			continue;

		CHARACTER_MANAGER::GetInstance()->SpawnMob(20040, r.lMapIndex, region->sx + r.x + (r.width / 2), region->sy + r.y + (r.height / 2), 0);
	}
}

void CManager::DeleteObject(uint32_t dwID) // from DB
{
	PyLog("OBJ_DEL: {}", dwID);

	auto it = m_map_pObjByID.find(dwID);

	if (it == m_map_pObjByID.end())
		return;

	it->second->GetLand()->DeleteObject(dwID);
}

LPOBJECT CManager::FindObjectByVID(uint32_t dwVID)
{
	auto it = m_map_pObjByVID.find(dwVID);

	if (it == m_map_pObjByVID.end())
		return NULL;

	return it->second;
}

void CManager::UnregisterObject(LPOBJECT pObj)
{
	m_map_pObjByID.erase(pObj->GetID());
	m_map_pObjByVID.erase(pObj->GetVID());
}

void CManager::SendLandList(LPDESC d, int32_t lMapIndex)
{
	TLandPacketElement e;

	TEMP_BUFFER buf;

	uint16_t wCount = 0;

	auto it = m_map_pLand.begin();

	while (it != m_map_pLand.end())
	{
		CLand* pLand = (it++)->second;
		const TLand& r = pLand->GetData();

		if (r.lMapIndex != lMapIndex)
			continue;

		//
		LPCHARACTER ch  = d->GetCharacter();
		if (ch)
		{
			CGuild *guild = CGuildManager::GetInstance()->FindGuild(r.dwGuildID);
			ch->SendGuildName(guild);
		}
		//

		e.dwID = r.dwID;
		e.x = r.x;
		e.y = r.y;
		e.width = r.width;
		e.height = r.height;
		e.dwGuildID = r.dwGuildID;

		buf.write(&e, sizeof(TLandPacketElement));
		++wCount;
	}

	PyLog("SendLandList map {} count {} elem_size: {}", lMapIndex, wCount, buf.size());

	if (wCount != 0)
	{
		TPacketGCLandList p;

		p.header = HEADER_GC_LAND_LIST;
		p.size = sizeof(TPacketGCLandList) + buf.size();

		d->BufferedPacket(&p, sizeof(TPacketGCLandList));
		d->Packet(buf.read_peek(), buf.size());
	}
}

// LAND_CLEAR
void CManager::ClearLand(uint32_t dwLandID)
{
	CLand* pLand = FindLand(dwLandID);

	if (pLand == nullptr)
	{
		PyLog("LAND_CLEAR: there is no LAND id like {}", dwLandID);
		return;
	}

	pLand->ClearLand();

	PyLog("LAND_CLEAR: request Land Clear. LandID: {}", pLand->GetID());
}

void CManager::ClearLandByGuildID(uint32_t dwGuildID)
{
	CLand* pLand = FindLandByGuild(dwGuildID);

	if (pLand == nullptr)
	{
		PyLog("LAND_CLEAR: there is no GUILD id like {}", dwGuildID);
		return;
	}

	pLand->ClearLand();

	PyLog("LAND_CLEAR: request Land Clear. LandID: {}", pLand->GetID());
}

void CLand::ClearLand()
{
	auto iter = m_map_pObject.begin();

	while (iter != m_map_pObject.end())
	{
		RequestDeleteObject(iter->second->GetID());
		iter++;
	}

	SetOwner(0);

	const TLand& r = GetData();
	const TMapRegion * region = SECTREE_MANAGER::GetInstance()->GetMapRegion(r.lMapIndex);

	CHARACTER_MANAGER::GetInstance()->SpawnMob(20040, r.lMapIndex, region->sx + r.x + (r.width / 2), region->sy + r.y + (r.height / 2), 0);
}
// END_LAND_CLEAR

// BUILD_WALL
void CLand::DrawWall(uint32_t dwVnum, int32_t nMapIndex, int32_t& x, int32_t& y, char length, float zRot)
{
	int32_t rot = (int32_t)zRot;
	rot = ((rot%360) / 90) * 90;

	int32_t dx=0, dy=0;

	switch (rot)
	{
		case 0 :
			dx = -500;
			dy = 0;
			break;

		case 90 :
			dx = 0;
			dy = 500;
			break;

		case 180 :
			dx = 500;
			dy = 0;
			break;

		case 270 :
			dx = 0;
			dy = -500;
			break;
	}

	for (int32_t i=0; i < length; i++)
	{
		this->RequestCreateObject(dwVnum, nMapIndex, x, y, 0, 0, rot, false);
		x += dx;
		y += dy;
	}
}


bool CLand::RequestCreateWall(int32_t nMapIndex, float rot)
{
	const bool 	WALL_ANOTHER_CHECKING_ENABLE = false;

	const TLand& land = GetData();

	int32_t center_x = land.x + land.width  / 2;
	int32_t center_y = land.y + land.height / 2;

	int32_t wall_x = center_x;
	int32_t wall_y = center_y;
	int32_t wall_half_w = 1000;
	int32_t wall_half_h = 1362;

	if (rot == 0.0f) 		// South door
	{
		int32_t door_x = wall_x;
		int32_t door_y = wall_y + wall_half_h;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x, wall_y + wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x, wall_y - wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x - wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x + wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}	
	else if (rot == 180.0f)		// North door
	{
		int32_t door_x = wall_x;
		int32_t door_y = wall_y - wall_half_h;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x, wall_y - wall_half_h, door_x, door_y, 180.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x, wall_y + wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x - wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x + wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}
	else if (rot == 90.0f)		// East door
	{
		int32_t door_x = wall_x + wall_half_h;
		int32_t door_y = wall_y;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x + wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x - wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x, wall_y - wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x, wall_y + wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}
	else if (rot == 270.0f)		// West door
	{
		int32_t door_x = wall_x - wall_half_h;
		int32_t door_y = wall_y;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x - wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x + wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x, wall_y - wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x, wall_y + wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}

	if (test_server)
	{
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x + 50, 			land.y + 50, 0, 0, 0.0, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x + land.width - 50,	land.y + 50, 0, 0, 90.0, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x + land.width - 50,	land.y + land.height - 50, 0, 0, 180.0, WALL_ANOTHER_CHECKING_ENABLE); 
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x + 50, 			land.y + land.height - 50, 0, 0, 270.0, WALL_ANOTHER_CHECKING_ENABLE);
	}
	return true;
}

void CLand::RequestDeleteWall()
{
	auto iter = m_map_pObject.begin();

	while (iter != m_map_pObject.end())
	{
		unsigned id   = iter->second->GetID();
		unsigned vnum = iter->second->GetVnum();

		switch (vnum)
		{
			case WALL_DOOR_VNUM:
			case WALL_BACK_VNUM:
			case WALL_LEFT_VNUM:
			case WALL_RIGHT_VNUM:
				RequestDeleteObject(id);
				break;
		}


		if (test_server)
		{
			if (FLAG_VNUM == vnum)
				RequestDeleteObject(id);

		}

		iter++;
	}
}

bool CLand::RequestCreateWallBlocks(uint32_t dwVnum, int32_t nMapIndex, char wallSize, bool doorEast, bool doorWest, bool doorSouth, bool doorNorth)
{
	const TLand& r = GetData();

	int32_t startX = r.x + (r.width  / 2) - (1300 + wallSize*500);
	int32_t startY = r.y + (r.height / 2) + (1300 + wallSize*500);

	uint32_t corner = dwVnum - 4;
	uint32_t wall   = dwVnum - 3;
	uint32_t door   = dwVnum - 1;

	bool checkAnother = false;
	int32_t* ptr = nullptr;
	int32_t delta = 1;
	int32_t rot = 270;

	bool doorOpen[4];
	doorOpen[0] = doorWest;
	doorOpen[1] = doorNorth;
	doorOpen[2] = doorEast;
	doorOpen[3] = doorSouth;

	if (wallSize > 3) wallSize = 3;
	else if (wallSize < 0) wallSize = 0;

	for (int32_t i=0; i < 4; i++, rot -= 90)
	{
		switch (i)
		{
			case 0 :
				delta = -1;
				ptr = &startY;
				break;
			case 1 :
				delta = 1;
				ptr = &startX;
				break;
			case 2 :
				ptr = &startY;
				delta = 1;
				break;
			case 3 :
				ptr = &startX;
				delta = -1;
				break;
		}

		this->RequestCreateObject(corner, nMapIndex, startX, startY, 0, 0, rot, checkAnother);

		*ptr =* ptr + (700 * delta);

		if (doorOpen[i])
		{
			this->DrawWall(wall, nMapIndex, startX, startY, wallSize, rot);

			*ptr =* ptr + (700 * delta);

			this->RequestCreateObject(door, nMapIndex, startX, startY, 0, 0, rot, checkAnother);

			*ptr =* ptr + (1300 * delta);

			this->DrawWall(wall, nMapIndex, startX, startY, wallSize, rot);
		}
		else
		{
			this->DrawWall(wall, nMapIndex, startX, startY, wallSize*2 + 4, rot);
		}

		*ptr =* ptr + (100 * delta);
	}

	return true;
}

void CLand::RequestDeleteWallBlocks(uint32_t dwID)
{
	auto iter = m_map_pObject.begin();

	uint32_t corner = dwID - 4;
	uint32_t wall = dwID - 3;
	uint32_t door = dwID - 1;
	uint32_t dwVnum = 0;

	while (iter != m_map_pObject.end())
	{
		dwVnum = iter->second->GetVnum();

		if (dwVnum == corner || dwVnum == wall || dwVnum == door)
		{
			RequestDeleteObject(iter->second->GetID());
		}
		iter++;
	}
}
// END_BUILD_WALL

