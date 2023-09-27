#include "StdAfx.h"
#include <EterLib/StateManager.h>

#include "MapManager.h"
#include "MapOutdoor.h"
#include "PropertyLoader.h"

bool CMapManager::IsMapOutdoor()
{
	if (m_pkMap)
		return true;

	return false;
}

CMapOutdoor& CMapManager::GetMapOutdoorRef()
{
	assert(nullptr != m_pkMap);
	return *m_pkMap;
}

CMapManager::CMapManager() : mc_pcurEnvironmentData(nullptr)
{
	m_pkMap = nullptr;
}

CMapManager::~CMapManager()
{
	Destroy();
}

void CMapManager::Initialize()
{
	mc_pcurEnvironmentData = nullptr;
	__LoadMapInfoVector();
}

void CMapManager::Create()
{
	if (m_pkMap)
	{
		m_pkMap->Clear();
		return;
	}

	m_pkMap = (CMapOutdoor*)AllocMap();
	assert(nullptr != m_pkMap && "CMapManager::Create MAP is nullptr");
}

void CMapManager::Destroy()
{
	stl_wipe_second(m_EnvironmentDataMap);

	if (m_pkMap)
	{
		delete m_pkMap;
		m_pkMap = nullptr;
	}
}

void CMapManager::Clear()
{
	if (m_pkMap)
		m_pkMap->Clear();

	m_Forest.Clear();
}

CMapBase* CMapManager::AllocMap()
{
	return new CMapOutdoor;
}

// Map
void CMapManager::LoadProperty()
{
	CPropertyLoader PropertyLoader;
	PropertyLoader.SetPropertyManager(&m_PropertyManager);
	PropertyLoader.Create("*.*", "Property");
}

bool CMapManager::LoadMap(const std::string& c_rstrMapName, float x, float y, float z)
{
	Clear();
	m_pkMap->Leave();
	m_pkMap->SetName(c_rstrMapName);
	m_pkMap->LoadProperty();

	if (CMapBase::MAPTYPE_INDOOR == m_pkMap->GetType())
	{
		SysLog("CMapManager::LoadMap() Indoor Map Load Failed");
		return false;
	}
	else if (CMapBase::MAPTYPE_OUTDOOR == m_pkMap->GetType())
	{
		if (!m_pkMap->Load(x, y, z))
		{
			SysLog("CMapManager::LoadMap() Outdoor Map Load Failed");
			return false;
		}

		RegisterEnvironmentData(0, m_pkMap->GetEnvironmentDataName().c_str());
		SetEnvironmentData(0);
	}
	else
	{
		SysLog("CMapManager::LoadMap() Invalid Map Type");
		return false;
	}

	m_pkMap->Enter();
	return true;
}

bool CMapManager::IsMapReady()
{
	if (!m_pkMap)
		return false;

	return m_pkMap->IsReady();
}

bool CMapManager::UnloadMap(const std::string& c_strMapName)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	if (c_strMapName != rkMap.GetName() && !rkMap.GetName().empty())
	{
		return false;
	}

	Clear();
	return true;
}

bool CMapManager::UpdateMap(float fx, float fy, float fz)
{
	if (!m_pkMap)
		return false;

	return m_pkMap->Update(fx, -fy, fz);
}

void CMapManager::UpdateAroundAmbience(float fx, float fy, float fz)
{
	if (!m_pkMap)
		return;

	m_pkMap->UpdateAroundAmbience(fx, -fy, fz);
}

float CMapManager::GetHeight(float fx, float fy)
{
	if (!m_pkMap)
	{
		return 0.0f;
	}

	return m_pkMap->GetHeight(fx, fy);
}

float CMapManager::GetTerrainHeight(float fx, float fy)
{
	if (!m_pkMap)
	{
		SysLog("CMapManager::GetTerrainHeight({0}, {1}) - Invalid map", fx, fy);
		return 0.0f;
	}

	return m_pkMap->GetTerrainHeight(fx, fy);
}

bool CMapManager::GetWaterHeight(int32_t iX, int32_t iY, int32_t* plWaterHeight)
{
	if (!m_pkMap)
	{
		SysLog("CMapManager::GetTerrainHeight({0}, {1}) - Invalid map", iX, iY);
		return false;
	}

	return m_pkMap->GetWaterHeight(iX, iY, plWaterHeight);
}

// Environment
void CMapManager::BeginEnvironment()
{
	if (!m_pkMap)
		return;

	if (!mc_pcurEnvironmentData)
		return;

	// Light always on
	STATEMANAGER->SaveRenderState(D3DRS_LIGHTING, TRUE);

	// Fog
	STATEMANAGER->SaveRenderState(D3DRS_FOGENABLE, mc_pcurEnvironmentData->bFogEnable);

	// Material
	STATEMANAGER->SetMaterial(&mc_pcurEnvironmentData->Material);

	// Directional Light
	if (mc_pcurEnvironmentData->bDirLightsEnable[ENV_DIRLIGHT_BACKGROUND])
	{
		ms_lpd3dDevice->LightEnable(0, TRUE);

		m_pkMap->ApplyLight((uint32_t)mc_pcurEnvironmentData, mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND]);
	}
	else
		ms_lpd3dDevice->LightEnable(0, FALSE);

	if (mc_pcurEnvironmentData->bFogEnable)
	{
		uint32_t dwFogColor = mc_pcurEnvironmentData->FogColor;
		STATEMANAGER->SetRenderState(D3DRS_FOGCOLOR, dwFogColor);

		if (mc_pcurEnvironmentData->bDensityFog)
		{
			float fDensity = 0.00015f;
			STATEMANAGER->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_EXP);					// pixel fog
			STATEMANAGER->SetRenderState(D3DRS_FOGDENSITY, *((uint32_t*)&fDensity));			// vertex fog
		}
		else
		{
			auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
			rkForest->SetFog(
				mc_pcurEnvironmentData->GetFogNearDistance(),
				mc_pcurEnvironmentData->GetFogFarDistance()
			);

			float fFogNear = mc_pcurEnvironmentData->GetFogNearDistance();
			float fFogFar = mc_pcurEnvironmentData->GetFogFarDistance();
			STATEMANAGER->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);								// vertex fox
			STATEMANAGER->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);											// vertex fox
			STATEMANAGER->SetRenderState(D3DRS_FOGSTART, *((uint32_t*)&fFogNear));	// USED BY D3DFOG_LINEAR
			STATEMANAGER->SetRenderState(D3DRS_FOGEND, *((uint32_t*)&fFogFar));		// USED BY D3DFOG_LINEAR
		}
	}

	m_pkMap->OnBeginEnvironment();
}

void CMapManager::EndEnvironment()
{
	if (!mc_pcurEnvironmentData)
		return;

	STATEMANAGER->RestoreRenderState(D3DRS_LIGHTING);
	STATEMANAGER->RestoreRenderState(D3DRS_FOGENABLE);
}

void CMapManager::SetEnvironmentData(int32_t nEnvDataIndex)
{
	const TEnvironmentData* c_pEnvironmenData;

	if (GetEnvironmentData(nEnvDataIndex, &c_pEnvironmenData))
		SetEnvironmentDataPtr(c_pEnvironmenData);
}

void CMapManager::SetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData)
{
	if (!m_pkMap)
		return;

	if (!c_pEnvironmentData)
	{
		assert(!"null environment data");
		SysLog("Not set environment data");
		return;
	}

	mc_pcurEnvironmentData = c_pEnvironmentData;
	m_pkMap->SetEnvironmentDataPtr(mc_pcurEnvironmentData);
}

void CMapManager::ResetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData)
{
	if (!m_pkMap)
		return;

	if (!c_pEnvironmentData)
	{
		assert(!"null environment data");
		SysLog("Not set environment data");
		return;
	}

	mc_pcurEnvironmentData = c_pEnvironmentData;
	m_pkMap->ResetEnvironmentDataPtr(mc_pcurEnvironmentData);
}

void CMapManager::BlendEnvironmentData(const TEnvironmentData* c_pEnvironmentData, int32_t iTransitionTime)
{
}

bool CMapManager::RegisterEnvironmentData(uint32_t dwIndex, const char* c_szFileName)
{
	TEnvironmentData* pEnvironmentData = AllocEnvironmentData();

	if (!LoadEnvironmentData(c_szFileName, pEnvironmentData))
	{
		DeleteEnvironmentData(pEnvironmentData);
		return false;
	}

	auto f = m_EnvironmentDataMap.find(dwIndex);
	if (m_EnvironmentDataMap.end() == f)
		m_EnvironmentDataMap.emplace(dwIndex, pEnvironmentData);
	else
	{
		delete f->second;
		f->second = pEnvironmentData;
	}
	return true;
}

void CMapManager::GetCurrentEnvironmentData(const TEnvironmentData** c_ppEnvironmentData)
{
	*c_ppEnvironmentData = mc_pcurEnvironmentData;
}

bool CMapManager::GetEnvironmentData(uint32_t dwIndex, const TEnvironmentData** c_ppEnvironmentData)
{
	auto itor = m_EnvironmentDataMap.find(dwIndex);

	if (m_EnvironmentDataMap.end() == itor)
	{
		*c_ppEnvironmentData = nullptr;
		return false;
	}

	*c_ppEnvironmentData = itor->second;
	return true;
}

void CMapManager::RefreshPortal()
{
	if (!IsMapReady())
		return;

	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		auto pArea = m_pkMap->GetAreaPointer(i);
		if (!pArea)
			continue;

		pArea.value()->RefreshPortal();
	}
}

void CMapManager::ClearPortal()
{
	if (!IsMapReady())
		return;

	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		auto pArea = m_pkMap->GetAreaPointer(i);
		if (!pArea)
			continue;

		pArea.value()->ClearPortal();
	}
}

void CMapManager::AddShowingPortalID(int32_t iID)
{
	if (!IsMapReady())
		return;

	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		auto pArea = m_pkMap->GetAreaPointer(i);
		if (!pArea)
			continue;

		pArea.value()->AddShowingPortalID(iID);
	}
}

TEnvironmentData* CMapManager::AllocEnvironmentData()
{
	auto* pEnvironmentData = new TEnvironmentData;
	Environment_Init(*pEnvironmentData);
	return pEnvironmentData;
}

void CMapManager::DeleteEnvironmentData(TEnvironmentData* pEnvironmentData)
{
	delete pEnvironmentData;
	pEnvironmentData = nullptr;
}

bool CMapManager::LoadEnvironmentData(const char* c_szFileName, TEnvironmentData* pEnvironmentData)
{
	if (!pEnvironmentData)
		return false;

	return (bool)Environment_Load(*pEnvironmentData, c_szFileName);
}

uint32_t CMapManager::GetShadowMapColor(float fx, float fy)
{
	if (!IsMapReady())
		return 0xFFFFFFFF;

	return m_pkMap->GetShadowMapColor(fx, fy);
}

std::vector<int32_t>& CMapManager::GetRenderedSplatNum(int32_t* piPatch, int32_t* piSplat, float* pfSplatRatio)
{
	if (!m_pkMap)
	{
		static std::vector<int32_t> s_emptyVector;
		*piPatch = 0;
		*piSplat = 0;
		return s_emptyVector;
	}

	return m_pkMap->GetRenderedSplatNum(piPatch, piSplat, pfSplatRatio);
}

bool CMapManager::GetNormal(int32_t ix, int32_t iy, D3DXVECTOR3* pv3Normal)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->GetNormal(ix, iy, pv3Normal);
}

bool CMapManager::isPhysicalCollision(const D3DXVECTOR3& c_rvCheckPosition)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->isAttrOn(c_rvCheckPosition.x, -c_rvCheckPosition.y, CTerrainImpl::ATTRIBUTE_BLOCK);
}

bool CMapManager::isAttrOn(float fX, float fY, uint8_t byAttr)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->isAttrOn(fX, fY, byAttr);
}

bool CMapManager::GetAttr(float fX, float fY, uint8_t* pbyAttr)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->GetAttr(fX, fY, pbyAttr);
}

bool CMapManager::isAttrOn(int32_t iX, int32_t iY, uint8_t byAttr)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->isAttrOn(iX, iY, byAttr);
}

bool CMapManager::GetAttr(int32_t iX, int32_t iY, uint8_t* pbyAttr)
{
	if (!IsMapReady())
		return false;

	return m_pkMap->GetAttr(iX, iY, pbyAttr);
}

void CMapManager::SetTerrainRenderSort(CMapOutdoor::ETerrainRenderSort eTerrainRenderSort)
{
	if (!IsMapReady())
		return;

	m_pkMap->SetTerrainRenderSort(eTerrainRenderSort);
}

void CMapManager::SetTransparentTree(bool bTransparenTree)
{
	if (!IsMapReady())
		return;

	m_pkMap->SetTransparentTree(bTransparenTree);
}

CMapOutdoor::ETerrainRenderSort CMapManager::GetTerrainRenderSort()
{
	if (!IsMapReady())
		return CMapOutdoor::DISTANCE_SORT;

	return m_pkMap->GetTerrainRenderSort();
}

void CMapManager::__LoadMapInfoVector()
{
	auto vfs_string = CallFS().LoadFileToString(CallFS(), m_stAtlasInfoFileName);
	if (!vfs_string)
	{
		SysLog("Failed to load {0}", m_stAtlasInfoFileName);
		return;
	}

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(vfs_string.value());

	char szMapName[256];
	char szMapLocaleName[256]{};
	int32_t x, y;
	int32_t width, height;
	for (uint32_t uLineIndex = 0; uLineIndex < textFileLoader.GetLineCount(); ++uLineIndex)
	{
		const std::string& c_rstLine = textFileLoader.GetLineString(uLineIndex);
		sscanf(c_rstLine.c_str(), "%s %d %d %d %d", szMapName, &x, &y, &width, &height);

		if ('\0' == szMapName[0])
			continue;

		TMapInfo kMapInfo;
		kMapInfo.m_strName = szMapName;
		kMapInfo.m_strLocaleName = szMapName;
		kMapInfo.m_dwBaseX = x;
		kMapInfo.m_dwBaseY = y;

		kMapInfo.m_dwSizeX = width;
		kMapInfo.m_dwSizeY = height;

		kMapInfo.m_dwEndX = kMapInfo.m_dwBaseX + kMapInfo.m_dwSizeX * CTerrainImpl::TERRAIN_XSIZE;
		kMapInfo.m_dwEndY = kMapInfo.m_dwBaseY + kMapInfo.m_dwSizeY * CTerrainImpl::TERRAIN_YSIZE;

		m_kVct_kMapInfo.emplace_back(kMapInfo);
	}
}

void CMapManager::SetEnvironmentFog(bool flag)
{
	if (!IsMapReady())
		return;

	if (mc_pcurEnvironmentData)
	{
		mc_pcurEnvironmentData->bFogEnable = flag;
		ResetEnvironmentDataPtr(mc_pcurEnvironmentData);
	}
}
