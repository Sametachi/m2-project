#include "stdafx.h"
#include <sstream>
#include <Core/Attribute.h>
#include "config.h"
#include "utils.h"
#include "sectree_manager.h"
#include "regen.h"
#include "lzo_manager.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "start_position.h"

uint16_t SECTREE_MANAGER::current_sectree_version = MAKEWORD(0, 3);

SECTREE_MAP::SECTREE_MAP()
{
	memset(&m_setting, 0, sizeof(m_setting));
}

SECTREE_MAP::~SECTREE_MAP()
{
	MapType::iterator it = map_.begin();

	while (it != map_.end()) {
		LPSECTREE sectree = (it++)->second;
		M2_DELETE(sectree);
	}

	map_.clear();
}

SECTREE_MAP::SECTREE_MAP(SECTREE_MAP& r)
{
	m_setting = r.m_setting;

	MapType::iterator it = r.map_.begin();

	while (it != r.map_.end())
	{
		LPSECTREE tree = M2_NEW SECTREE;

		tree->m_id.coord = it->second->m_id.coord;
		tree->CloneAttribute(it->second);

		map_.insert(MapType::value_type(it->first, tree));
		++it;
	}

	Build();
}

LPSECTREE SECTREE_MAP::Find(uint32_t dwPackage)
{
	MapType::iterator it = map_.find(dwPackage);

	if (it == map_.end())
		return NULL;

	return it->second;
}

LPSECTREE SECTREE_MAP::Find(uint32_t x, uint32_t y)
{
	SECTREEID id;
	id.coord.x = x / SECTREE_SIZE;
	id.coord.y = y / SECTREE_SIZE;
	return Find(id.package);
}

void SECTREE_MAP::Build()
{
    // To give the client the information of the character with a radius of 150m
     // Expand the surrounding sectree to 3x3 squares -> 5x5 squares (Korea)
	struct neighbor_coord_s
	{
		int32_t x;
		int32_t y;
	} neighbor_coord[8] = {
		{ -SECTREE_SIZE,	0		},
		{  SECTREE_SIZE,	0		},
		{ 0	       ,	-SECTREE_SIZE	},
		{ 0	       ,	 SECTREE_SIZE	},
		{ -SECTREE_SIZE,	 SECTREE_SIZE	},
		{  SECTREE_SIZE,	-SECTREE_SIZE	},
		{ -SECTREE_SIZE,	-SECTREE_SIZE	},
		{  SECTREE_SIZE,	 SECTREE_SIZE	},
	};

	//
	// Create a list of surrounding sectrees for all sectrees.
	//
	MapType::iterator it = map_.begin();

	while (it != map_.end())
	{
		LPSECTREE tree = it->second;

		tree->m_neighbor_list.push_back(tree);

		TraceLog("{}x{}", (int32_t)tree->m_id.coord.x, (int32_t)tree->m_id.coord.y);

		int32_t x = tree->m_id.coord.x * SECTREE_SIZE;
		int32_t y = tree->m_id.coord.y * SECTREE_SIZE;

		for (uint32_t i = 0; i < 8; ++i)
		{
			LPSECTREE tree2 = Find(x + neighbor_coord[i].x, y + neighbor_coord[i].y);

			if (tree2)
			{
				TraceLog("   {} {}x{}", i, (int32_t)tree2->m_id.coord.x, (int32_t)tree2->m_id.coord.y);
				tree->m_neighbor_list.push_back(tree2);
			}
		}

		++it;
	}
}

SECTREE_MANAGER::SECTREE_MANAGER()
{
}

SECTREE_MANAGER::~SECTREE_MANAGER()
{
	/*
	   std::map<uint32_t, LPSECTREE_MAP>::iterator it = m_map_pSectree.begin();

	   while (it != m_map_pSectree.end())
	   {
	   M2_DELETE(it->second);
	   ++it;
	   }
	 */
}

LPSECTREE_MAP SECTREE_MANAGER::GetMap(int32_t lMapIndex)
{
	std::map<uint32_t, LPSECTREE_MAP>::iterator it = m_map_pSectree.find(lMapIndex);

	if (it == m_map_pSectree.end())
		return NULL;

	return it->second;
}

LPSECTREE SECTREE_MANAGER::Get(uint32_t dwIndex, uint32_t package)
{
	LPSECTREE_MAP pSectreeMap = GetMap(dwIndex);

	if (!pSectreeMap)
		return NULL;

	return pSectreeMap->Find(package);
}

LPSECTREE SECTREE_MANAGER::Get(uint32_t dwIndex, uint32_t x, uint32_t y)
{
	SECTREEID id;
	id.coord.x = x / SECTREE_SIZE;
	id.coord.y = y / SECTREE_SIZE;
	return Get(dwIndex, id.package);
}

// -----------------------------------------------------------------------------
// Setting.txt Create a SECTREE from
// -----------------------------------------------------------------------------
int32_t SECTREE_MANAGER::LoadSettingFile(int32_t lMapIndex, const char* c_pszSettingFileName, TMapSetting& r_setting)
{
	memset(&r_setting, 0, sizeof(TMapSetting));

	FILE * fp = fopen(c_pszSettingFileName, "r");

	if (!fp)
	{
		SysLog("cannot open file: {}", c_pszSettingFileName);
		return 0;
	}

	char buf[256], cmd[256];
	int32_t iWidth = 0, iHeight = 0;

	while (fgets(buf, 256, fp))
	{
		sscanf(buf, " %s ", cmd);

		if (!strcasecmp(cmd, "MapSize"))
		{
			sscanf(buf, " %s %d %d ", cmd, &iWidth, &iHeight);
		}
		else if (!strcasecmp(cmd, "BasePosition"))
		{
			sscanf(buf, " %s %d %d", cmd, &r_setting.iBaseX, &r_setting.iBaseY);
		}
		else if (!strcasecmp(cmd, "CellScale"))
		{
			sscanf(buf, " %s %d ", cmd, &r_setting.iCellScale);
		}
	}

	fclose(fp);

	if ((iWidth == 0 && iHeight == 0) || r_setting.iCellScale == 0)
	{
		SysLog("Invalid Settings file: {}", c_pszSettingFileName);
		return 0;
	}

	r_setting.iIndex = lMapIndex;
	r_setting.iWidth = (r_setting.iCellScale * 128 * iWidth);
	r_setting.iHeight = (r_setting.iCellScale * 128 * iHeight);
	return 1;
}

LPSECTREE_MAP SECTREE_MANAGER::BuildSectreeFromSetting(TMapSetting& r_setting)
{
	LPSECTREE_MAP pMapSectree = M2_NEW SECTREE_MAP;

	pMapSectree->m_setting = r_setting;

	int32_t x, y;
	LPSECTREE tree;

	for (x = r_setting.iBaseX; x < r_setting.iBaseX + r_setting.iWidth; x += SECTREE_SIZE)
	{
		for (y = r_setting.iBaseY; y < r_setting.iBaseY + r_setting.iHeight; y += SECTREE_SIZE)
		{
			tree = M2_NEW SECTREE;
			tree->m_id.coord.x = x / SECTREE_SIZE;
			tree->m_id.coord.y = y / SECTREE_SIZE;
			pMapSectree->Add(tree->m_id.package, tree);
			TraceLog("new sectree {} x {}", (int32_t)tree->m_id.coord.x, (int32_t)tree->m_id.coord.y);
		}
	}

	if ((r_setting.iBaseX + r_setting.iWidth) % SECTREE_SIZE)
	{
		tree = M2_NEW SECTREE;
		tree->m_id.coord.x = ((r_setting.iBaseX + r_setting.iWidth) / SECTREE_SIZE) + 1;
		tree->m_id.coord.y = ((r_setting.iBaseY + r_setting.iHeight) / SECTREE_SIZE);
		pMapSectree->Add(tree->m_id.package, tree);
	}

	if ((r_setting.iBaseY + r_setting.iHeight) % SECTREE_SIZE)
	{
		tree = M2_NEW SECTREE;
		tree->m_id.coord.x = ((r_setting.iBaseX + r_setting.iWidth) / SECTREE_SIZE);
		tree->m_id.coord.y = ((r_setting.iBaseX + r_setting.iHeight) / SECTREE_SIZE) + 1;
		pMapSectree->Add(tree->m_id.package, tree);
	}

	return pMapSectree;
}

void SECTREE_MANAGER::LoadDungeon(int32_t iIndex, const char* c_pszFileName)
{
	FILE* fp = fopen(c_pszFileName, "r");

	if (!fp)
		return;

	int32_t count = 0; // for debug

	while (!feof(fp))
	{
		char buf[1024];

		if (!fgets(buf, 1024, fp))
			break;

		if (buf[0] == '#' || buf[0] == '/' && buf[1] == '/')
			continue;

		std::istringstream ins(buf, std::ios_base::in);
		std::string position_name;
		int32_t x, y, sx, sy, dir;

		ins >> position_name >> x >> y >> sx >> sy >> dir;

		if (ins.fail())
			continue;

		x -= sx;
		y -= sy;
		sx *= 2;
		sy *= 2;
		sx += x;
		sy += y;

		m_map_pArea[iIndex].insert(std::make_pair(position_name, TAreaInfo(x, y, sx, sy, dir)));

		count++;
	}

	fclose(fp);

	PyLog("Dungeon Position Load [%3d]{} count {}", iIndex, c_pszFileName, count);
}

// Fix me
// Because we just receive x, y from the current Town.txt and add the base coordinates to it in this code
// You can never move to a town on another map.
// If there is a map or other identifier in front of it,
// Let's make it possible to move to a town on another map.
// by rtsummit
bool SECTREE_MANAGER::LoadMapRegion(const char* c_pszFileName, TMapSetting& r_setting, const char* c_pszMapName)
{
	FILE * fp = fopen(c_pszFileName, "r");

	if (test_server)
		PyLog("[LoadMapRegion] file({})", c_pszFileName);

	if (!fp)
		return false;

	int32_t iX=0, iY=0;
	PIXEL_POSITION pos[3] = { {0,0,0}, {0,0,0}, {0,0,0} };

	fscanf(fp, " %d %d ", &iX, &iY);

	int32_t iEmpirePositionCount = fscanf(fp, " %d %d %d %d %d %d ", 
			&pos[0].x, &pos[0].y,
			&pos[1].x, &pos[1].y,
			&pos[2].x, &pos[2].y);

	fclose(fp);

	if(iEmpirePositionCount == 6)
	{
		for (int32_t n = 0; n < 3; ++n)
			PyLog("LoadMapRegion {} {} ", pos[n].x, pos[n].y);
	}
	else
	{
		PyLog("LoadMapRegion no empire specific start point");
	}

	TMapRegion region;

	region.index = r_setting.iIndex;
	region.sx = r_setting.iBaseX;
	region.sy = r_setting.iBaseY;
	region.ex = r_setting.iBaseX + r_setting.iWidth;
	region.ey = r_setting.iBaseY + r_setting.iHeight;

	region.strMapName = c_pszMapName;

	region.posSpawn.x = r_setting.iBaseX + (iX * 100);
	region.posSpawn.y = r_setting.iBaseY + (iY * 100); 

	r_setting.posSpawn = region.posSpawn;

	PyLog("LoadMapRegion {} x {} ~ {} y {} ~ {}, town {} {}", 
			region.index,
			region.sx,
			region.ex,
			region.sy,
			region.ey,
			region.posSpawn.x,
			region.posSpawn.y);

	if (iEmpirePositionCount == 6)
	{
		region.bEmpireSpawnDifferent = true;

		for (int32_t i = 0; i < 3; i++)
		{
			region.posEmpire[i].x = r_setting.iBaseX + (pos[i].x * 100);
			region.posEmpire[i].y = r_setting.iBaseY + (pos[i].y * 100);
		}
	}
	else
	{
		region.bEmpireSpawnDifferent = false;
	}

	m_vec_mapRegion.push_back(region);

	PyLog("LoadMapRegion {} End", region.index);
	return true;
}

bool SECTREE_MANAGER::LoadAttribute(LPSECTREE_MAP pMapSectree, const char* c_pszFileName, TMapSetting& r_setting)
{
	FILE * fp = fopen(c_pszFileName, "rb");

	if (!fp)
	{
		SysLog("SECTREE_MANAGER::LoadAttribute : cannot open {}", c_pszFileName);
		return false;
	}

	int32_t iWidth, iHeight;

	fread(&iWidth, sizeof(int32_t), 1, fp);
	fread(&iHeight, sizeof(int32_t), 1, fp);

	int32_t maxMemSize = LZOManager::GetInstance()->GetMaxCompressedSize(sizeof(uint32_t) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE));

	uint32_t uiSize;
	lzo_uint uiDestSize;

#ifndef _MSC_VER
	uint8_t abComp[maxMemSize];
#else
	uint8_t* abComp = M2_NEW uint8_t[maxMemSize];
#endif
	uint32_t* attr = M2_NEW uint32_t[maxMemSize];

	for (int32_t y = 0; y < iHeight; ++y)
		for (int32_t x = 0; x < iWidth; ++x)
		{
			// Use the uint32_t value created by combining the coordinates with UNION as the ID.
			SECTREEID id;
			id.coord.x = (r_setting.iBaseX / SECTREE_SIZE) + x;
			id.coord.y = (r_setting.iBaseY / SECTREE_SIZE) + y;

			LPSECTREE tree = pMapSectree->Find(id.package);

			// SERVER_ATTR_LOAD_ERROR
			if (tree == nullptr)
			{
				SysLog("FATAL ERROR! LoadAttribute({}) - cannot find sectree(package={}, coord=({}, {}), map_index={}, map_base=({}, {}))", 
						c_pszFileName, id.package, (int32_t)id.coord.x, (int32_t)id.coord.y, r_setting.iIndex, r_setting.iBaseX, r_setting.iBaseY);
				SysLog("ERROR_ATTR_POS({}, {}) attr_size({}, {})", x, y, iWidth, iHeight);
				SysLog("CHECK! 'Setting.txt' and 'server_attr' MAP_SIZE!!");

				pMapSectree->DumpAllToSysErr();
				abort();

				M2_DELETE_ARRAY(attr);
#ifdef _MSC_VER
				M2_DELETE_ARRAY(abComp);
#endif
				return false;
			}
			// END_OF_SERVER_ATTR_LOAD_ERROR

			if (tree->m_id.package != id.package)
			{
				SysLog("returned tree id mismatch! return {}, request {}", 
						tree->m_id.package, id.package);
				fclose(fp);

				M2_DELETE_ARRAY(attr);
#ifdef _MSC_VER
				M2_DELETE_ARRAY(abComp);
#endif
				return false;
			}

			fread(&uiSize, sizeof(int32_t), 1, fp);
			fread(abComp, sizeof(char), uiSize, fp);

			//LZOManager::GetInstance()->Decompress(abComp, uiSize, (uint8_t*) tree->GetAttributePointer(), &uiDestSize);
			uiDestSize = sizeof(uint32_t) * maxMemSize;
			LZOManager::GetInstance()->Decompress(abComp, uiSize, (uint8_t*) attr, &uiDestSize);

			if (uiDestSize != sizeof(uint32_t) * (SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE))
			{
				SysLog("SECTREE_MANAGER::LoadAttribte : {} : {} {} size mismatch! {}",
						c_pszFileName, (int32_t)tree->m_id.coord.x, (int32_t)tree->m_id.coord.y, uiDestSize);
				fclose(fp);

				M2_DELETE_ARRAY(attr);
#ifdef _MSC_VER
				M2_DELETE_ARRAY(abComp);
#endif
				return false;
			}

			tree->BindAttribute(M2_NEW CAttribute(attr, SECTREE_SIZE / CELL_SIZE, SECTREE_SIZE / CELL_SIZE));
		}

	fclose(fp);

	M2_DELETE_ARRAY(attr);
#ifdef _MSC_VER
	M2_DELETE_ARRAY(abComp);
#endif
	return true;
}

bool SECTREE_MANAGER::GetRecallPositionByEmpire(int32_t iMapIndex, uint8_t bEmpire, PIXEL_POSITION& r_pos)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	// Maps over 10000 are limited to instance dungeons.
	if (iMapIndex >= 10000)
	{
		iMapIndex /= 10000;
	}

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (rRegion.index == iMapIndex)
		{
			if (rRegion.bEmpireSpawnDifferent && bEmpire >= 1 && bEmpire <= 3)
				r_pos = rRegion.posEmpire[bEmpire - 1];
			else
				r_pos = rRegion.posSpawn;

			return true;
		}
	}

	return false;
}

bool SECTREE_MANAGER::GetCenterPositionOfMap(int32_t lMapIndex, PIXEL_POSITION& r_pos)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (rRegion.index == lMapIndex)
		{
			r_pos.x = rRegion.sx + (rRegion.ex - rRegion.sx) / 2;
			r_pos.y = rRegion.sy + (rRegion.ey - rRegion.sy) / 2;
			r_pos.z = 0;
			return true;
		}
	}

	return false;
}

bool SECTREE_MANAGER::GetSpawnPositionByMapIndex(int32_t lMapIndex, PIXEL_POSITION& r_pos)
{
	if (lMapIndex> 10000) lMapIndex /= 10000;
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (lMapIndex == rRegion.index)
		{
			r_pos = rRegion.posSpawn;
			return true;
		}
	}

	return false;
}

bool SECTREE_MANAGER::GetSpawnPosition(int32_t x, int32_t y, PIXEL_POSITION& r_pos)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (x >= rRegion.sx && y >= rRegion.sy && x < rRegion.ex && y < rRegion.ey)
		{
			r_pos = rRegion.posSpawn;
			return true;
		}
	}

	return false;
}

bool SECTREE_MANAGER::GetMapBasePositionByMapIndex(int32_t lMapIndex, PIXEL_POSITION& r_pos)
{
	if (lMapIndex> 10000) lMapIndex /= 10000;
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		//if (x >= rRegion.sx && y >= rRegion.sy && x < rRegion.ex && y < rRegion.ey)
		if (lMapIndex == rRegion.index)
		{
			r_pos.x = rRegion.sx;
			r_pos.y = rRegion.sy;
			r_pos.z = 0;
			return true;
		}
	}

	return false;
}

bool SECTREE_MANAGER::GetMapBasePosition(int32_t x, int32_t y, PIXEL_POSITION& r_pos)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (x >= rRegion.sx && y >= rRegion.sy && x < rRegion.ex && y < rRegion.ey)
		{
			r_pos.x = rRegion.sx;
			r_pos.y = rRegion.sy;
			r_pos.z = 0;
			return true;
		}
	}

	return false;
}

const TMapRegion * SECTREE_MANAGER::FindRegionByPartialName(const char* szMapName)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		//if (rRegion.index == lMapIndex)
		//return &rRegion;
		if (rRegion.strMapName.find(szMapName))
			return &rRegion;
	}

	return NULL;
}

const TMapRegion * SECTREE_MANAGER::GetMapRegion(int32_t lMapIndex)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (rRegion.index == lMapIndex)
			return &rRegion;
	}

	return NULL;
}

int32_t SECTREE_MANAGER::GetMapIndex(int32_t x, int32_t y)
{
	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (x >= rRegion.sx && y >= rRegion.sy && x < rRegion.ex && y < rRegion.ey)
			return rRegion.index;
	}

	PyLog("SECTREE_MANAGER::GetMapIndex({}, {})", x, y);

	std::vector<TMapRegion>::iterator i;
	for (i = m_vec_mapRegion.begin(); i !=m_vec_mapRegion.end(); ++i)
	{
		TMapRegion& rRegion = *i;
		PyLog("{}: ({}, {}) ~ ({}, {})", rRegion.index, rRegion.sx, rRegion.sy, rRegion.ex, rRegion.ey);
	}

	return 0;
}

int32_t SECTREE_MANAGER::Build(const char* c_pszListFileName, const char* c_pszMapBasePath)
{
	if (test_server)
	{
		PyLog("[BUILD] Build {} {} ", c_pszListFileName, c_pszMapBasePath);
	}

	FILE* fp = fopen(c_pszListFileName, "r");

	if (!fp)
		return 0;

	char buf[256 + 1];
	char szFilename[256];
	char szMapName[256];
	int32_t iIndex;

	while (fgets(buf, 256, fp))
	{
		*strrchr(buf, '\n') = '\0';

		if (!strncmp(buf, "//", 2) || *buf == '#')
			continue;

		sscanf(buf, " %d %s ", &iIndex, szMapName);

		snprintf(szFilename, sizeof(szFilename), "%s/%s/Setting.txt", c_pszMapBasePath, szMapName);

		TMapSetting setting;
		setting.iIndex = iIndex;

		if (!LoadSettingFile(iIndex, szFilename, setting))
		{
			SysLog("can't load file {} in LoadSettingFile", szFilename);
			fclose(fp);
			return 0;
		}

		snprintf(szFilename, sizeof(szFilename), "%s/%s/Town.txt", c_pszMapBasePath, szMapName);

		if (!LoadMapRegion(szFilename, setting, szMapName))
		{
			SysLog("can't load file {} in LoadMapRegion", szFilename);
			fclose(fp);
			return 0;
		}

		if (test_server)
			PyLog("[BUILD] Build {} {} {} ",c_pszMapBasePath, szMapName, iIndex);

		// First, check if this server needs to spawn a monster in this map.
		if (map_allow_find(iIndex))
		{
			LPSECTREE_MAP pMapSectree = BuildSectreeFromSetting(setting);
			m_map_pSectree.insert(std::map<uint32_t, LPSECTREE_MAP>::value_type(iIndex, pMapSectree));

			snprintf(szFilename, sizeof(szFilename), "%s/%s/server_attr", c_pszMapBasePath, szMapName);
			LoadAttribute(pMapSectree, szFilename, setting);

			snprintf(szFilename, sizeof(szFilename), "%s/%s/regen.txt", c_pszMapBasePath, szMapName);
			regen_load(szFilename, setting.iIndex, setting.iBaseX, setting.iBaseY);

			snprintf(szFilename, sizeof(szFilename), "%s/%s/npc.txt", c_pszMapBasePath, szMapName);
			regen_load(szFilename, setting.iIndex, setting.iBaseX, setting.iBaseY);

			snprintf(szFilename, sizeof(szFilename), "%s/%s/boss.txt", c_pszMapBasePath, szMapName);
			regen_load(szFilename, setting.iIndex, setting.iBaseX, setting.iBaseY);

			snprintf(szFilename, sizeof(szFilename), "%s/%s/stone.txt", c_pszMapBasePath, szMapName);
			regen_load(szFilename, setting.iIndex, setting.iBaseX, setting.iBaseY);

			snprintf(szFilename, sizeof(szFilename), "%s/%s/dungeon.txt", c_pszMapBasePath, szMapName);
			LoadDungeon(iIndex, szFilename);

			pMapSectree->Build();
		}
	}

	fclose(fp);

	return 1;
}

bool SECTREE_MANAGER::IsMovablePosition(int32_t lMapIndex, int32_t x, int32_t y)
{
	LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, x, y);

	if (!tree)
		return false;

	return (!tree->IsAttr(x, y, ATTR_BLOCK | ATTR_OBJECT));
}

bool SECTREE_MANAGER::GetMovablePosition(int32_t lMapIndex, int32_t x, int32_t y, PIXEL_POSITION & pos)
{
	int32_t i = 0;

	do
	{
		int32_t dx = x + aArroundCoords[i].x;
		int32_t dy = y + aArroundCoords[i].y;

		LPSECTREE tree = SECTREE_MANAGER::GetInstance()->Get(lMapIndex, dx, dy);

		if (!tree)
			continue;

		if (!tree->IsAttr(dx, dy, ATTR_BLOCK | ATTR_OBJECT))
		{
			pos.x = dx;
			pos.y = dy;
			return true;
		}
	} while (++i < ARROUND_COORD_MAX_NUM);

	pos.x = x;
	pos.y = y;
	return false;
}

bool SECTREE_MANAGER::GetValidLocation(int32_t lMapIndex, int32_t x, int32_t y, int32_t& r_lValidMapIndex, PIXEL_POSITION& r_pos, uint8_t empire)
{
	LPSECTREE_MAP pSectreeMap = GetMap(lMapIndex);

	if (!pSectreeMap)
	{
		if (lMapIndex >= 10000)
		{
/*			int32_t m = lMapIndex / 10000;
			if (m == 216)
			{
				if (GetRecallPositionByEmpire (m, empire, r_pos))
				{
					r_lValidMapIndex = m;
					return true;
				}
				else 
					return false;
			}*/
			return GetValidLocation(lMapIndex / 10000, x, y, r_lValidMapIndex, r_pos);
		}
		else
		{
			SysLog("cannot find sectree_map by map index {}", lMapIndex);
			return false;
		}
	}

	int32_t lRealMapIndex = lMapIndex;

	if (lRealMapIndex >= 10000)
		lRealMapIndex = lRealMapIndex / 10000;

	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (rRegion.index == lRealMapIndex)
		{
			LPSECTREE tree = pSectreeMap->Find(x, y);

			if (!tree)
			{
				SysLog("cannot find tree by {} {} (map index {})", x, y, lMapIndex);
				return false;
			}

			r_lValidMapIndex = lMapIndex;
			r_pos.x = x;
			r_pos.y = y;
			return true;
		}
	}

	SysLog("invalid location (map index {} {} x {})", lRealMapIndex, x, y);
	return false;
}

bool SECTREE_MANAGER::GetRandomLocation(int32_t lMapIndex, PIXEL_POSITION& r_pos, uint32_t dwCurrentX, uint32_t dwCurrentY, int32_t iMaxDistance)
{
	LPSECTREE_MAP pSectreeMap = GetMap(lMapIndex);

	if (!pSectreeMap)
		return false;

	uint32_t x, y;

	std::vector<TMapRegion>::iterator it = m_vec_mapRegion.begin();

	while (it != m_vec_mapRegion.end())
	{
		TMapRegion& rRegion = *(it++);

		if (rRegion.index != lMapIndex)
			continue;

		int32_t i = 0;

		while (i++ < 100)
		{
			x = number(rRegion.sx + 50, rRegion.ex - 50);
			y = number(rRegion.sy + 50, rRegion.ey - 50);

			if (iMaxDistance != 0)
			{
				int32_t d;

				d = abs((float)dwCurrentX - x);

				if (d > iMaxDistance)
				{
					if (x < dwCurrentX)
						x = dwCurrentX - iMaxDistance;
					else
						x = dwCurrentX + iMaxDistance;
				}

				d = abs((float)dwCurrentY - y);

				if (d > iMaxDistance)
				{
					if (y < dwCurrentY)
						y = dwCurrentY - iMaxDistance;
					else
						y = dwCurrentY + iMaxDistance;
				}
			}

			LPSECTREE tree = pSectreeMap->Find(x, y);

			if (!tree)
				continue;

			if (tree->IsAttr(x, y, ATTR_BLOCK | ATTR_OBJECT))
				continue;

			r_pos.x = x;
			r_pos.y = y;
			return true;
		}
	}

	return false;
}

int32_t SECTREE_MANAGER::CreatePrivateMap(int32_t lMapIndex)
{
	if (lMapIndex >= 10000) // There is no map more than 10000. (or already private)
		return 0;

	LPSECTREE_MAP pMapSectree = GetMap(lMapIndex);

	if (!pMapSectree)
	{
		SysLog("Cannot find map index {}", lMapIndex);
		return 0;
	}

	// <Factor> Circular private map indexing
	int32_t base = lMapIndex * 10000;
	int32_t index_cap = 10000;
	if (lMapIndex == 107 || lMapIndex == 108 || lMapIndex == 109) {
		index_cap = (test_server ? 1 : 51);
	}
	PrivateIndexMapType::iterator it = next_private_index_map_.find(lMapIndex);
	if (it == next_private_index_map_.end()) {
		it = next_private_index_map_.insert(PrivateIndexMapType::value_type(lMapIndex, 0)).first;
	}
	int32_t i, next_index = it->second;
	for (i = 0; i < index_cap; ++i) {
		if (GetMap(base + next_index) == nullptr) {
			break; // available
		}
		if (++next_index >= index_cap) {
			next_index = 0;
		}
	}
	if (i == index_cap) {
		// No available index
		return 0;
	}
	int32_t lNewMapIndex = base + next_index;
	if (++next_index >= index_cap) {
		next_index = 0;
	}
	it->second = next_index;

	/*
	int32_t i;

	for (i = 0; i < 10000; ++i)
	{
		if (!GetMap((lMapIndex * 10000) + i))
			break;
	}
	
	if (test_server)
		PyLog("Create Dungeon : OrginalMapindex {} NewMapindex {}", lMapIndex, i);
	
	if (lMapIndex == 107 || lMapIndex == 108 || lMapIndex == 109)
	{
		if (test_server)
		{
			if (i > 0)
				return NULL;
		}
		else
		{
			if (i > 50)
				return NULL;
			
		}
	}

	if (i == 10000)
	{
		SysLog("not enough private map index (map_index {})", lMapIndex);
		return 0;
	}

	int32_t lNewMapIndex = lMapIndex * 10000 + i;
	*/

	pMapSectree = M2_NEW SECTREE_MAP(*pMapSectree);
	m_map_pSectree.insert(std::map<uint32_t, LPSECTREE_MAP>::value_type(lNewMapIndex, pMapSectree));

	PyLog("PRIVATE_MAP: {} created (original {})", lNewMapIndex, lMapIndex);
	return lNewMapIndex;
}

struct FDestroyPrivateMapEntity
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			PyLog("PRIVAE_MAP: removing character {}", ch->GetName());

			if (ch->GetDesc())
				DESC_MANAGER::GetInstance()->DestroyDesc(ch->GetDesc());
			else
				M2_DESTROY_CHARACTER(ch);
		}
		else if (ent->IsType(ENTITY_ITEM))
		{
			LPITEM item = (LPITEM) ent;
			PyLog("PRIVATE_MAP: removing item {}", item->GetName());

			M2_DESTROY_ITEM(item);
		}
		else
			SysLog("PRIVAE_MAP: trying to remove unknown entity {}", ent->GetType());
	}
};

void SECTREE_MANAGER::DestroyPrivateMap(int32_t lMapIndex)
{
	if (lMapIndex < 10000) // The private map has an index of 10000 or higher.
		return;

	LPSECTREE_MAP pMapSectree = GetMap(lMapIndex);

	if (!pMapSectree)
		return;

	// Destroy everything currently on this map.
	// WARNING:
	// May be in this map but not in any Sectree
	// So we can't delete here, so the pointer might be broken.
	// need to be handled separately
	FDestroyPrivateMapEntity f;
	pMapSectree->for_each(f);

	m_map_pSectree.erase(lMapIndex);
	M2_DELETE(pMapSectree);

	PyLog("PRIVATE_MAP: {} destroyed", lMapIndex);
}

TAreaMap& SECTREE_MANAGER::GetDungeonArea(int32_t lMapIndex)
{
	auto it = m_map_pArea.find(lMapIndex);

	if (it == m_map_pArea.end())
	{
		return m_map_pArea[-1];
	}
	return it->second;
}

void SECTREE_MANAGER::SendNPCPosition(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();
	if (!d)
		return;

	int32_t lMapIndex = ch->GetMapIndex();

	if (m_mapNPCPosition[lMapIndex].empty())
		return;

	TEMP_BUFFER buf;
	TPacketGCNPCPosition p;
	p.header = HEADER_GC_NPC_POSITION;
	p.count = m_mapNPCPosition[lMapIndex].size();

	TNPCPosition np;

	// Send TODO m_mapNPCPosition[lMapIndex]
	for (auto it = m_mapNPCPosition[lMapIndex].begin(); it != m_mapNPCPosition[lMapIndex].end(); ++it)
	{
		np.bType = it->bType;
		strlcpy(np.name, it->name, sizeof(np.name));
		np.x = it->x;
		np.y = it->y;
		buf.write(&np, sizeof(np));
	}

	p.size = sizeof(p) + buf.size();

	if (buf.size())
	{
		d->BufferedPacket(&p, sizeof(TPacketGCNPCPosition));
		d->Packet(buf.read_peek(), buf.size());
	}
	else
		d->Packet(&p, sizeof(TPacketGCNPCPosition));
}

void SECTREE_MANAGER::InsertNPCPosition(int32_t lMapIndex, uint8_t bType, const char* szName, int32_t x, int32_t y)
{
	m_mapNPCPosition[lMapIndex].push_back(npc_info(bType, szName, x, y));
}

uint8_t SECTREE_MANAGER::GetEmpireFromMapIndex(int32_t lMapIndex)
{
	if (lMapIndex >= 1 && lMapIndex <= 20)
		return 1;

	if (lMapIndex >= 21 && lMapIndex <= 40)
		return 2;

	if (lMapIndex >= 41 && lMapIndex <= 60)
		return 3;

	if (lMapIndex == 184 || lMapIndex == 185)
		return 1;
	
	if (lMapIndex == 186 || lMapIndex == 187)
		return 2;
	
	if (lMapIndex == 188 || lMapIndex == 189)
		return 3;

	switch (lMapIndex)
	{
		case 190 :
			return 1;
		case 191 :
			return 2;
		case 192 :
			return 3;
	}
	
	return 0;
}

class FRemoveIfAttr
{
	public:
		FRemoveIfAttr(LPSECTREE pTree, uint32_t dwAttr) : m_pTree(pTree), m_dwCheckAttr(dwAttr)
		{
		}

		void operator () (LPENTITY entity)
		{
			if (!m_pTree->IsAttr(entity->GetX(), entity->GetY(), m_dwCheckAttr))
				return;

			if (entity->IsType(ENTITY_ITEM))
			{
				M2_DESTROY_ITEM((LPITEM) entity);
			}
			else if (entity->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) entity;

				if (ch->IsPC())
				{
					PIXEL_POSITION pos;

					if (SECTREE_MANAGER::GetInstance()->GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
						ch->WarpSet(pos.x, pos.y);
					else
						ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
				}
				else
					ch->Dead();
			}
		}

		LPSECTREE m_pTree;
		uint32_t m_dwCheckAttr;
};

bool SECTREE_MANAGER::ForAttrRegionCell(int32_t lMapIndex, int32_t lCX, int32_t lCY, uint32_t dwAttr, EAttrRegionMode mode)
{
	SECTREEID id;

	id.coord.x = lCX / (SECTREE_SIZE / CELL_SIZE);
	id.coord.y = lCY / (SECTREE_SIZE / CELL_SIZE);

	int32_t lTreeCX = id.coord.x * (SECTREE_SIZE / CELL_SIZE);
	int32_t lTreeCY = id.coord.y * (SECTREE_SIZE / CELL_SIZE);

	LPSECTREE pSec = Get(lMapIndex, id.package);
	if (!pSec)
		return false;

	switch (mode)
	{
		case ATTR_REGION_MODE_SET:
			pSec->SetAttribute(lCX - lTreeCX, lCY - lTreeCY, dwAttr);
			break;

		case ATTR_REGION_MODE_REMOVE:
			pSec->RemoveAttribute(lCX - lTreeCX, lCY - lTreeCY, dwAttr);
			break;

		case ATTR_REGION_MODE_CHECK:
			if (pSec->IsAttr(lCX * CELL_SIZE, lCY * CELL_SIZE, ATTR_OBJECT))
				return true;
			break;

		default:
			SysLog("Unknown region mode {}", mode);
			break;
	}

	return false;
}

bool SECTREE_MANAGER::ForAttrRegionRightAngle(int32_t lMapIndex, int32_t lCX, int32_t lCY, int32_t lCW, int32_t lCH, int32_t lRotate, uint32_t dwAttr, EAttrRegionMode mode)
{
	if (1 == lRotate/90 || 3 == lRotate/90)
	{
		for (int32_t x = 0; x < lCH; ++x)
			for (int32_t y = 0; y < lCW; ++y)
			{
				if (ForAttrRegionCell(lMapIndex, lCX + x, lCY + y, dwAttr, mode))
					return true;
			}
	}
	if (0 == lRotate/90 || 2 == lRotate/90)
	{
		for (int32_t x = 0; x < lCW; ++x)
			for (int32_t y = 0; y < lCH; ++y)
			{
				if (ForAttrRegionCell(lMapIndex, lCX + x, lCY + y, dwAttr, mode))
					return true;
			}
	}

	return mode == ATTR_REGION_MODE_CHECK ? false : true;
}

#define min(l, r)	((l) < (r) ? (l) : (r))
#define max(l, r)	((l) < (r) ? (r) : (l))

bool SECTREE_MANAGER::ForAttrRegionFreeAngle(int32_t lMapIndex, int32_t lCX, int32_t lCY, int32_t lCW, int32_t lCH, int32_t lRotate, uint32_t dwAttr, EAttrRegionMode mode)
{
	float fx1 = (-lCW/2) * sinf(float(lRotate)/180.0f*3.14f) + (-lCH/2) * cosf(float(lRotate)/180.0f*3.14f);
	float fy1 = (-lCW/2) * cosf(float(lRotate)/180.0f*3.14f) - (-lCH/2) * sinf(float(lRotate)/180.0f*3.14f);

	float fx2 = (+lCW/2) * sinf(float(lRotate)/180.0f*3.14f) + (-lCH/2) * cosf(float(lRotate)/180.0f*3.14f);
	float fy2 = (+lCW/2) * cosf(float(lRotate)/180.0f*3.14f) - (-lCH/2) * sinf(float(lRotate)/180.0f*3.14f);

	float fx3 = (-lCW/2) * sinf(float(lRotate)/180.0f*3.14f) + (+lCH/2) * cosf(float(lRotate)/180.0f*3.14f);
	float fy3 = (-lCW/2) * cosf(float(lRotate)/180.0f*3.14f) - (+lCH/2) * sinf(float(lRotate)/180.0f*3.14f);

	float fx4 = (+lCW/2) * sinf(float(lRotate)/180.0f*3.14f) + (+lCH/2) * cosf(float(lRotate)/180.0f*3.14f);
	float fy4 = (+lCW/2) * cosf(float(lRotate)/180.0f*3.14f) - (+lCH/2) * sinf(float(lRotate)/180.0f*3.14f);

	float fdx1 = fx2 - fx1;
	float fdy1 = fy2 - fy1;
	float fdx2 = fx1 - fx3;
	float fdy2 = fy1 - fy3;

	if (0 == fdx1 || 0 == fdx2)
	{
		SysLog("SECTREE_MANAGER::ForAttrRegion - Unhandled exception. MapIndex: {}", lMapIndex);
		return false;
	}

	float fTilt1 = float(fdy1) / float(fdx1);
	float fTilt2 = float(fdy2) / float(fdx2);
	float fb1 = fy1 - fTilt1*fx1;
	float fb2 = fy1 - fTilt2*fx1;
	float fb3 = fy4 - fTilt1*fx4;
	float fb4 = fy4 - fTilt2*fx4;

	float fxMin = min(fx1, min(fx2, min(fx3, fx4)));
	float fxMax = max(fx1, max(fx2, max(fx3, fx4)));
	for (int32_t i = int32_t(fxMin); i < int32_t(fxMax); ++i)
	{
		float fyValue1 = fTilt1*i + min(fb1, fb3);
		float fyValue2 = fTilt2*i + min(fb2, fb4);

		float fyValue3 = fTilt1*i + max(fb1, fb3);
		float fyValue4 = fTilt2*i + max(fb2, fb4);

		float fMinValue;
		float fMaxValue;
		if (abs(int32_t(fyValue1)) < abs(int32_t(fyValue2)))
			fMaxValue = fyValue1;
		else
			fMaxValue = fyValue2;
		if (abs(int32_t(fyValue3)) < abs(int32_t(fyValue4)))
			fMinValue = fyValue3;
		else
			fMinValue = fyValue4;

		for (int32_t j = int32_t(min(fMinValue, fMaxValue)); j < int32_t(max(fMinValue, fMaxValue)); ++j) {
			if (ForAttrRegionCell(lMapIndex, lCX + (lCW / 2) + i, lCY + (lCH / 2) + j, dwAttr, mode))
				return true;
		}
	}

	return mode == ATTR_REGION_MODE_CHECK ? false : true;
}

bool SECTREE_MANAGER::ForAttrRegion(int32_t lMapIndex, int32_t lStartX, int32_t lStartY, int32_t lEndX, int32_t lEndY, int32_t lRotate, uint32_t dwAttr, EAttrRegionMode mode)
{
	LPSECTREE_MAP pMapSectree = GetMap(lMapIndex);

	if (!pMapSectree)
	{
		SysLog("Cannot find SECTREE_MAP by map index {}", lMapIndex);
		return mode == ATTR_REGION_MODE_CHECK ? true : false;
	}

	//
	// Extend the coordinates of the area according to the size of the Cell.
	//

	lStartX	-= lStartX % CELL_SIZE;
	lStartY	-= lStartY % CELL_SIZE;
	lEndX	+= lEndX % CELL_SIZE;
	lEndY	+= lEndY % CELL_SIZE;

	//
	// Get the cell coordinates.
	//

	int32_t lCX = lStartX / CELL_SIZE;
	int32_t lCY = lStartY / CELL_SIZE;
	int32_t lCW = (lEndX - lStartX) / CELL_SIZE;
	int32_t lCH = (lEndY - lStartY) / CELL_SIZE;

	TraceLog("ForAttrRegion {} {} ~ {} {}", lStartX, lStartY, lEndX, lEndY);

	lRotate = lRotate % 360;

	if (0 == lRotate % 90)
		return ForAttrRegionRightAngle(lMapIndex, lCX, lCY, lCW, lCH, lRotate, dwAttr, mode);

	return ForAttrRegionFreeAngle(lMapIndex, lCX, lCY, lCW, lCH, lRotate, dwAttr, mode);
}

struct FPurgeMonsters
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER lpChar = (LPCHARACTER)ent;

			if (lpChar->IsMonster() && !lpChar->IsPet())
			{
				M2_DESTROY_CHARACTER(lpChar);
			}
		}
	}
};

void SECTREE_MANAGER::PurgeMonstersInMap(int32_t lMapIndex)
{
	LPSECTREE_MAP sectree = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (sectree != nullptr)
	{
		struct FPurgeMonsters f;

		sectree->for_each(f);
	}
}

struct FPurgeStones
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER lpChar = (LPCHARACTER)ent;

			if (lpChar->IsStone())
			{
				M2_DESTROY_CHARACTER(lpChar);
			}
		}
	}
};

void SECTREE_MANAGER::PurgeStonesInMap(int32_t lMapIndex)
{
	LPSECTREE_MAP sectree = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (sectree != nullptr)
	{
		struct FPurgeStones f;

		sectree->for_each(f);
	}
}

struct FPurgeNPCs
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER lpChar = (LPCHARACTER)ent;

			if (lpChar->IsNPC() && !lpChar->IsPet())
			{
				M2_DESTROY_CHARACTER(lpChar);
			}
		}
	}
};

void SECTREE_MANAGER::PurgeNPCsInMap(int32_t lMapIndex)
{
	LPSECTREE_MAP sectree = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (sectree != nullptr)
	{
		struct FPurgeNPCs f;

		sectree->for_each(f);
	}
}

struct FCountMonsters
{
	std::map<VID, VID> m_map_Monsters;

	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER lpChar = (LPCHARACTER)ent;

			if (lpChar->IsMonster())
			{
				m_map_Monsters[lpChar->GetVID()] = lpChar->GetVID();
			}
		}
	}
};

size_t SECTREE_MANAGER::GetMonsterCountInMap(int32_t lMapIndex)
{
	LPSECTREE_MAP sectree = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (sectree != nullptr)
	{
		struct FCountMonsters f;

		sectree->for_each(f);

		return f.m_map_Monsters.size();
	}

	return 0;
}

struct FCountSpecifiedMonster
{
	uint32_t SpecifiedVnum;
	size_t cnt;

	FCountSpecifiedMonster(uint32_t id)
		: SpecifiedVnum(id), cnt(0)
	{}

	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsStone())
			{
				if (pChar->GetMobTable().dwVnum == SpecifiedVnum)
					cnt++;
			}
		}
	}
};

size_t SECTREE_MANAGER::GetMonsterCountInMap(int32_t lMapIndex, uint32_t dwVnum)
{
	LPSECTREE_MAP sectree = SECTREE_MANAGER::GetInstance()->GetMap(lMapIndex);

	if (NULL != sectree)
	{
		struct FCountSpecifiedMonster f(dwVnum);

		sectree->for_each(f);

		return f.cnt;
	}

	return 0;
}


