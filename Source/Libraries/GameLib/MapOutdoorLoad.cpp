#include "StdAfx.h"
#include "MapOutdoor.h"
#include "AreaTerrain.h"

#include <EterLib/ResourceManager.h>

bool CMapOutdoor::Load(float x, float y, float z)
{
	Destroy();
	std::string strFileName = GetMapDataDirectory() + "\\Setting.txt";

	if (!LoadSetting(strFileName.c_str()))
	{
		SysLog("CMapOutdoor::Load : LoadSetting({0}) Failed", strFileName.c_str());
		return false;
	}

	CreateTerrainPatchProxyList();
	BuildQuadTree();
	LoadWaterTexture();
	CreateCharacterShadowTexture();
	m_lOldReadX = -1;

	CSpeedTreeForestDirectX9::GetInstance()->SetRenderingDevice();
	Update(x, y, z);
	return true;
}

std::string& CMapOutdoor::GetEnvironmentDataName()
{
	return m_envDataName;
}

bool CMapOutdoor::isTerrainLoaded(uint16_t wX, uint16_t wY)
{
	for (auto& pTerrain : m_TerrainVector)
	{
		uint16_t usCoordX, usCoordY;
		pTerrain->GetCoordinate(&usCoordX, &usCoordY);

		if (usCoordX == wX && usCoordY == wY)
			return true;
	}
	return false;
}

bool CMapOutdoor::isAreaLoaded(uint16_t wX, uint16_t wY)
{
	for (auto& pArea : m_AreaVector)
	{
		uint16_t usCoordX, usCoordY;
		pArea->GetCoordinate(&usCoordX, &usCoordY);

		if (usCoordX == wX && usCoordY == wY)
			return true;
	}
	return false;
}

void CMapOutdoor::AssignTerrainPointer()
{
	int16_t sReferenceCoordMinX, sReferenceCoordMaxX, sReferenceCoordMinY, sReferenceCoordMaxY;
	sReferenceCoordMinX = std::max(m_CurCoordinate.m_sTerrainCoordX - LOAD_SIZE_WIDTH, 0);
	sReferenceCoordMaxX = std::min(m_CurCoordinate.m_sTerrainCoordX + LOAD_SIZE_WIDTH, m_sTerrainCountX - 1);
	sReferenceCoordMinY = std::max(m_CurCoordinate.m_sTerrainCoordY - LOAD_SIZE_WIDTH, 0);
	sReferenceCoordMaxY = std::min(m_CurCoordinate.m_sTerrainCoordY + LOAD_SIZE_WIDTH, m_sTerrainCountY - 1);

	uint32_t i;
	for (i = 0; i < AROUND_AREA_NUM; ++i)
	{
		m_pArea[i] = nullptr;
		m_pTerrain[i] = nullptr;
	}

	for (i = 0; i < m_TerrainVector.size(); ++i)
	{
		auto& pTerrain = m_TerrainVector[i];
		uint16_t usCoordX, usCoordY;
		pTerrain->GetCoordinate(&usCoordX, &usCoordY);

		if (usCoordX >= sReferenceCoordMinX && usCoordX <= sReferenceCoordMaxX && usCoordY >= sReferenceCoordMinY && usCoordY <= sReferenceCoordMaxY)
		{
			m_pTerrain[(usCoordY - m_CurCoordinate.m_sTerrainCoordY + LOAD_SIZE_WIDTH) * 3 + (usCoordX - m_CurCoordinate.m_sTerrainCoordX + LOAD_SIZE_WIDTH)] = pTerrain.get();
		}
	}

	for (i = 0; i < m_AreaVector.size(); ++i)
	{
		auto& area = m_AreaVector[i];
		uint16_t usCoordX, usCoordY;
		area->GetCoordinate(&usCoordX, &usCoordY);

		if (usCoordX >= sReferenceCoordMinX && usCoordX <= sReferenceCoordMaxX && usCoordY >= sReferenceCoordMinY && usCoordY <= sReferenceCoordMaxY)
		{
			m_pArea[(usCoordY - m_CurCoordinate.m_sTerrainCoordY + LOAD_SIZE_WIDTH) * 3 + (usCoordX - m_CurCoordinate.m_sTerrainCoordX + LOAD_SIZE_WIDTH)] = area.get();
		}
	}
}

bool CMapOutdoor::LoadArea(uint16_t wAreaCoordX, uint16_t wAreaCoordY)
{
	if (isAreaLoaded(wAreaCoordX, wAreaCoordY))
		return true;

	const uint32_t ulID = static_cast<uint32_t>(wAreaCoordX) * 1000L + static_cast<uint32_t>(wAreaCoordY);
	char szAreaPathName[64 + 1];

	_snprintf(szAreaPathName, sizeof(szAreaPathName), "%s\\%06u\\", GetMapDataDirectory().c_str(), ulID);

	std::unique_ptr<CArea> area(new CArea());
	area->SetMapOutDoor(this);
	area->SetCoordinate(wAreaCoordX, wAreaCoordY);

	if (!area->Load(szAreaPathName))
		SysLog(" CMapOutdoor::LoadArea({0}, {1}) LoadShadowMap ERROR", wAreaCoordX, wAreaCoordY);

	area->EnablePortal(m_bEnablePortal);
	m_AreaVector.emplace_back(std::move(area));
	return true;
}

bool CMapOutdoor::LoadTerrain(uint16_t wTerrainCoordX, uint16_t wTerrainCoordY)
{
	if (isTerrainLoaded(wTerrainCoordX, wTerrainCoordY))
		return true;

	const uint32_t ulID = static_cast<uint32_t>(wTerrainCoordX) * 1000L + static_cast<uint32_t>(wTerrainCoordY);
	char filename[256];
	sprintf(filename, "%s\\%06lu\\AreaProperty.txt", GetMapDataDirectory().c_str(), ulID);

	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(filename, stTokenVectorMap))
	{
		SysLog("CMapOutdoor::LoadTerrain AreaProperty Read Error\n");
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("scripttype"))
	{
		SysLog("CMapOutdoor::LoadTerrain AreaProperty FileFormat Error 1\n");
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("areaname"))
	{
		SysLog("CMapOutdoor::LoadTerrain AreaProperty FileFormat Error 2\n");
		return false;
	}

	const std::string& scriptType = stTokenVectorMap["scripttype"][0];
	const std::string& areaName = stTokenVectorMap["areaname"][0];

	if (scriptType != "AreaProperty")
	{
		SysLog("CMapOutdoor::LoadTerrain AreaProperty FileFormat Error 3\n");
		return false;
	}

	std::unique_ptr<CTerrain> pTerrain(new CTerrain());
	pTerrain->Clear();
	pTerrain->SetMapOutDoor(this);
	pTerrain->SetCoordinate(wTerrainCoordX, wTerrainCoordY);
	pTerrain->CopySettingFromGlobalSetting();

	char szRawHeightFieldname[64 + 1];
	char szWaterMapName[64 + 1];
	char szAttrMapName[64 + 1];
	char szShadowTexName[64 + 1];
	char szShadowMapName[64 + 1];
	char szMiniMapTexName[64 + 1];
	char szSplatName[64 + 1];

	_snprintf(szRawHeightFieldname, sizeof(szRawHeightFieldname), "%s\\%06u\\height.raw", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szSplatName, sizeof(szSplatName), "%s\\%06u\\tile.raw", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szAttrMapName, sizeof(szAttrMapName), "%s\\%06u\\attr.atr", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szWaterMapName, sizeof(szWaterMapName), "%s\\%06u\\water.wtr", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szShadowTexName, sizeof(szShadowTexName), "%s\\%06u\\shadowmap.dds", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szShadowMapName, sizeof(szShadowMapName), "%s\\%06u\\shadowmap.raw", GetMapDataDirectory().c_str(), ulID);
	_snprintf(szMiniMapTexName, sizeof(szMiniMapTexName), "%s\\%06u\\minimap.dds", GetMapDataDirectory().c_str(), ulID);

	if (!pTerrain->LoadWaterMap(szWaterMapName))
		SysLog(" CMapOutdoor::LoadTerrain({0}, {1}) LoadWaterMap ERROR", wTerrainCoordX, wTerrainCoordY);

	if (!pTerrain->LoadHeightMap(szRawHeightFieldname))
		SysLog(" CMapOutdoor::LoadTerrain({0}, {1}) LoadHeightMap ERROR", wTerrainCoordX, wTerrainCoordY);

	if (!pTerrain->LoadAttrMap(szAttrMapName))
		SysLog(" CMapOutdoor::LoadTerrain({0}, {1}) LoadAttrMap ERROR", wTerrainCoordX, wTerrainCoordY);

	if (!pTerrain->RAW_LoadTileMap(szSplatName))
		SysLog(" CMapOutdoor::LoadTerrain({0}, {1}) RAW_LoadTileMap ERROR", wTerrainCoordX, wTerrainCoordY);

	pTerrain->LoadShadowTexture(szShadowTexName);

	if (!pTerrain->LoadShadowMap(szShadowMapName))
		SysLog(" CMapOutdoor::LoadTerrain({0}, {1}) LoadShadowMap ERROR", wTerrainCoordX, wTerrainCoordY);

	pTerrain->LoadMiniMapTexture(szMiniMapTexName);
	pTerrain->SetName(areaName);
	pTerrain->CalculateTerrainPatch();
	pTerrain->SetReady();

	m_TerrainVector.emplace_back(std::move(pTerrain));

	return true;
}

bool CMapOutdoor::LoadSetting(const char* c_szFileName)
{
	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(c_szFileName, stTokenVectorMap))
	{
		SysLog("Failed to parse config file {0}", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("scripttype"))
	{
		SysLog("{0}: Invalid resource type", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("viewradius"))
	{
		SysLog("{0}: ViewRadius == 0", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("cellscale"))
	{
		SysLog("{0}: Unsupported CellScale", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("heightscale"))
	{
		SysLog("{0}: Invalid HeightScale", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("mapsize"))
	{
		SysLog("{0}: Invalid MapSize", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("textureset"))
	{
		SysLog("{0}: Invalid Textureset", c_szFileName);
		return false;
	}

	if (stTokenVectorMap.end() != stTokenVectorMap.find("terrainvisible"))
		m_bSettingTerrainVisible = (bool)(atoi(stTokenVectorMap["terrainvisible"][0].c_str()) != 0);
	else
		m_bSettingTerrainVisible = true;

	const auto& c_rstrType = stTokenVectorMap["scripttype"][0];
	const auto& c_rstrViewRadius = stTokenVectorMap["viewradius"][0];
	const auto& c_rstrHeightScale = stTokenVectorMap["heightscale"][0];
	const auto& c_rstrMapSizeX = stTokenVectorMap["mapsize"][0];
	const auto& c_rstrMapSizeY = stTokenVectorMap["mapsize"][1];

	std::string strTextureSet;
	TTokenVector& rkVec_strToken = stTokenVectorMap["textureset"];
	if (!rkVec_strToken.empty())
		strTextureSet = rkVec_strToken[0];

	if (c_rstrType != "MapSetting")
	{
		SysLog("{0}: Invalid Map", c_szFileName);
		return false;
	}

	m_lViewRadius = atol(c_rstrViewRadius.c_str());

	if (0L >= m_lViewRadius)
	{
		SysLog("{0}: ViewRadius > 0", c_szFileName);
		return false;
	}

	m_fHeightScale = std::stof(c_rstrHeightScale.c_str());

	SetTerrainCount(atoi(c_rstrMapSizeX.c_str()), atoi(c_rstrMapSizeY.c_str()));

	m_fTerrainTexCoordBase = 1.0f / (float)(CTerrainImpl::PATCH_XSIZE * CTerrainImpl::CELLSCALE);

	//if (stTokenVectorMap.end() != stTokenVectorMap.find("baseposition"))
	//{
	//	const auto& c_rstrMapBaseX = stTokenVectorMap["baseposition"][0];
	//	const auto& c_rstrMapBaseY = stTokenVectorMap["baseposition"][1];
	//	SetBaseXY((uint32_t)atol(c_rstrMapBaseX.c_str()), (uint32_t)atol(c_rstrMapBaseY.c_str()));
	//}

	std::string stTextureSetFileName = strTextureSet;

	if (0 != stTextureSetFileName.find_first_of("textureset", 0))
		stTextureSetFileName = "textureset\\" + strTextureSet;

	if (!m_TextureSet.Load(stTextureSetFileName.c_str(), m_fTerrainTexCoordBase))
	{
		SysLog("{0}: Invalid Textureset ({0})", c_szFileName, stTextureSetFileName.c_str());
		return false;
	}

	CTerrain::SetTextureSet(&m_TextureSet);

	if (stTokenVectorMap.end() != stTokenVectorMap.find("environment"))
	{
		const CTokenVector& c_rEnvironmentVector = stTokenVectorMap["environment"];
		if (!c_rEnvironmentVector.empty())
			m_settings_envDataName = c_rEnvironmentVector[0];
		else
			SysLog("{0}: Invalid Environment", c_szFileName);
	}

	// Set environment after all the needed things are here
	std::string local_envDataName = GetMapDataDirectory() + "\\" + m_settings_envDataName;
	if (CallFS().Exists(local_envDataName))
	{
		m_envDataName = local_envDataName;
	}
	else
	{
		const auto& c_rstrEnvironmentRoot = "d:/ymir work/environment/";
		const auto& c_rstrMapName = GetName();
		m_envDataName = c_rstrEnvironmentRoot + m_settings_envDataName;

		if (c_rstrEnvironmentRoot == m_envDataName)
		{
			const auto& strAppendName = c_rstrMapName.substr(c_rstrMapName.size() - 2, 2);
			m_envDataName = c_rstrEnvironmentRoot + strAppendName + ".msenv";
		}
	}


	m_fWaterTexCoordBase = 1.0f / (float)(CTerrainImpl::CELLSCALE * 4);

	D3DXMatrixScaling(&m_matSplatAlpha, +m_fTerrainTexCoordBase * 2.0f * (float)(CTerrainImpl::PATCH_XSIZE) / (float)(CTerrainImpl::SPLATALPHA_RAW_XSIZE - 2), -m_fTerrainTexCoordBase * 2.0f * (float)(CTerrainImpl::PATCH_YSIZE) / (float)(CTerrainImpl::SPLATALPHA_RAW_XSIZE - 2), 0.0f);
	m_matSplatAlpha._41 = m_fTerrainTexCoordBase * 4.6f;
	m_matSplatAlpha._42 = m_fTerrainTexCoordBase * 4.6f;

	D3DXMatrixScaling(&m_matStaticShadow, +m_fTerrainTexCoordBase * ((float)CTerrainImpl::PATCH_XSIZE / CTerrainImpl::XSIZE), -m_fTerrainTexCoordBase * ((float)CTerrainImpl::PATCH_YSIZE / CTerrainImpl::XSIZE), 0.0f);
	m_matStaticShadow._41 = 0.0f;
	m_matStaticShadow._42 = 0.0f;

	D3DXMatrixScaling(&m_matDynamicShadowScale, 1.0f / m_fShadowSizeX, -1.0f / m_fShadowSizeY, 1.0f);
	m_matDynamicShadowScale._41 = 0.5f;
	m_matDynamicShadowScale._42 = 0.5f;

	// Transform
	D3DXMatrixScaling(&m_matBuildingTransparent, 1.0f / ((float)ms_iWidth), -1.0f / ((float)ms_iHeight), 1.0f);
	m_matBuildingTransparent._41 = 0.5f;
	m_matBuildingTransparent._42 = 0.5f;
	return true;
}