#include "stdafx.h"
#include <Eterlib/CullingManager.h>
#include <Eterlib/Camera.h>
#include <Gamelib/MapOutDoor.h>
#include <Gamelib/PropertyLoader.h>

#include "PythonBackground.h"
#include "PythonCharacterManager.h"
#include "PythonNetworkStream.h"
#include "PythonMiniMap.h"
#include "PythonSystem.h"

std::string g_strEffectName = "d:/ymir work/effect/etc/direction/direction_land.mse";

uint32_t CPythonBackground::GetRenderShadowTime()
{
	return m_dwRenderShadowTime;
}

bool CPythonBackground::SetVisiblePart(int32_t eMapOutDoorPart, bool isVisible)
{
	if (!m_pkMap)
		return false;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.SetVisiblePart(eMapOutDoorPart, isVisible);
	return true;
}

void CPythonBackground::EnableTerrainOnlyForHeight()
{
	if (!m_pkMap)
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	//rkMap.EnableTerrainOnlyForHeight(true);
}

bool CPythonBackground::SetSplatLimit(int32_t iSplatNum)
{
	if (!m_pkMap)
		return false;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.SetSplatLimit(iSplatNum);
	return true;
}

void CPythonBackground::CreateCharacterShadowTexture()
{
	if (!m_pkMap)
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.CreateCharacterShadowTexture();
}

void CPythonBackground::ReleaseCharacterShadowTexture()
{
	if (!m_pkMap)
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.ReleaseCharacterShadowTexture();
}

float CPythonBackground::GetShadowDistance()
{
	if (!m_pkMap)
		return 0.0f;

	return m_pkMap->GetShadowDistance();
}

void CPythonBackground::RefreshShadowTargetLevel()
{
	SetShadowTargetLevel(ClientConfig::GetInstance()->GetShadowTargetLevel());
}

bool CPythonBackground::SetShadowTargetLevel(int eLevel)
{
	if (!m_pkMap)
		return false;

	m_eShadowTargetLevel = eLevel;

	switch (m_eShadowTargetLevel)
	{
	case SHADOW_NONE:
		m_pkMap->SetDrawShadow(false);
		m_pkMap->SetDrawCharacterShadow(false);
		m_pkMap->SetDrawBackgroundShadow(false);
		break;

	case SHADOW_SELF_ONLY:
		m_pkMap->SetDrawShadow(true);
		m_pkMap->SetDrawCharacterShadow(true);
		m_pkMap->SetDrawBackgroundShadow(false);
		break;

	case SHADOW_ALL:
		m_pkMap->SetDrawShadow(true);
		m_pkMap->SetDrawCharacterShadow(true);
		m_pkMap->SetDrawBackgroundShadow(true);
		break;
	}

	return true;
}

void CPythonBackground::RefreshShadowQualityLevel()
{
	SetShadowQualityLevel(ClientConfig::GetInstance()->GetShadowQualityLevel());
}

bool CPythonBackground::SetShadowQualityLevel(int eLevel)
{
	if (!m_pkMap)
		return false;

	m_eShadowQualityLevel = eLevel;

	switch (m_eShadowQualityLevel)
	{
	case SHADOW_AVERAGE:
		m_pkMap->SetShadowTextureSize(2048);
		break;

	case SHADOW_GOOD:
		m_pkMap->SetShadowTextureSize(4096);
		break;

	case SHADOW_EPIC:
		m_pkMap->SetShadowTextureSize(8192);
		break;

	default:
		m_pkMap->SetShadowTextureSize(1024);
		break;
	}

	return true;
}

void CPythonBackground::SelectViewDistanceNum(int32_t eNum)
{
	if (!m_pkMap)
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();

	if (!mc_pcurEnvironmentData)
	{
		SysLog("CPythonBackground::SelectViewDistanceNum(int32_t eNum={0}) mc_pcurEnvironmentData is nullptr", eNum);
		return;
	}

	m_eViewDistanceNum = eNum;

	TEnvironmentData* env = (const_cast<TEnvironmentData*>(mc_pcurEnvironmentData));

	// It can change the atmosphere of the game, so if it is reserved, it will not be changed.
	if (env->bReserve)
	{
		env->m_fFogNearDistance = m_ViewDistanceSet[m_eViewDistanceNum].m_fFogStart;
		env->m_fFogFarDistance = m_ViewDistanceSet[m_eViewDistanceNum].m_fFogEnd;
		env->v3SkyBoxScale = m_ViewDistanceSet[m_eViewDistanceNum].m_v3SkyBoxScale;
		rkMap.SetEnvironmentSkyBox();
	}
}

void CPythonBackground::SetViewDistanceSet(int32_t eNum, float fFarClip)
{
	if (!m_pkMap)
		return;

	m_ViewDistanceSet[eNum].m_fFogStart = fFarClip * 0.5f;//0.3333333f;
	m_ViewDistanceSet[eNum].m_fFogEnd = fFarClip * 0.7f;//0.6666667f;

	float fSkyBoxScale = fFarClip * 0.6f;//0.5773502f;
	m_ViewDistanceSet[eNum].m_v3SkyBoxScale = D3DXVECTOR3(fSkyBoxScale, fSkyBoxScale, fSkyBoxScale);
	m_ViewDistanceSet[eNum].m_fFarClip = fFarClip;

	if (eNum == m_eViewDistanceNum)
		SelectViewDistanceNum(eNum);
}

float CPythonBackground::GetFarClip()
{
	if (!m_pkMap)
		return 50000.0f;

	if (m_ViewDistanceSet[m_eViewDistanceNum].m_fFarClip == 0.0f)
	{
		SysLog("CPythonBackground::GetFarClip m_eViewDistanceNum={0}", m_eViewDistanceNum);
		m_ViewDistanceSet[m_eViewDistanceNum].m_fFarClip = 25600.0f;
	}

	return m_ViewDistanceSet[m_eViewDistanceNum].m_fFarClip;
}

void CPythonBackground::GetDistanceSetInfo(int32_t* peNum, float* pfStart, float* pfEnd, float* pfFarClip)
{
	if (!m_pkMap)
	{
		*peNum = 4;
		*pfStart = 10000.0f;
		*pfEnd = 15000.0f;
		*pfFarClip = 50000.0f;
		return;
	}
	*peNum = m_eViewDistanceNum;
	*pfStart = m_ViewDistanceSet[m_eViewDistanceNum].m_fFogStart;
	*pfEnd = m_ViewDistanceSet[m_eViewDistanceNum].m_fFogEnd;
	*pfFarClip = m_ViewDistanceSet[m_eViewDistanceNum].m_fFarClip;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPythonBackground::CPythonBackground()
{
	m_dwRenderShadowTime = 0;
	m_eViewDistanceNum = 0;
	m_eViewDistanceNum = 0;
	m_eViewDistanceNum = 0;
	m_eShadowTargetLevel = ClientConfig::GetInstance()->GetShadowTargetLevel();
	m_eShadowQualityLevel = ClientConfig::GetInstance()->GetShadowQualityLevel();
	m_dwBaseX = 0;
	m_dwBaseY = 0;
	m_strMapName = "";
	m_iDayMode = DAY_MODE_LIGHT;
	m_iXMasTreeGrade = 0;
	m_bVisibleGuildArea = FALSE;

	SetViewDistanceSet(4, 25600.0f);
	SetViewDistanceSet(3, 25600.0f);
	SetViewDistanceSet(2, 25600.0f);
	SetViewDistanceSet(1, 25600.0f);
	SetViewDistanceSet(0, 25600.0f);
	Initialize();
}

CPythonBackground::~CPythonBackground()
{
	ConsoleLog("CPythonBackground Clear");
}

void CPythonBackground::Initialize()
{
	SetAtlasInfoFileName("Globals/AtlasInfo.txt");
	CMapManager::Initialize();
}

void CPythonBackground::CreateProperty()
{
	m_PropertyManager.Initialize();
}

//////////////////////////////////////////////////////////////////////
// Normal Functions
//////////////////////////////////////////////////////////////////////

bool CPythonBackground::GetPickingPoint(D3DXVECTOR3* v3IntersectPt)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	return rkMap.GetPickingPoint(v3IntersectPt);
}

bool CPythonBackground::GetPickingPointWithRay(const CRay& rRay, D3DXVECTOR3* v3IntersectPt)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	return rkMap.GetPickingPointWithRay(rRay, v3IntersectPt);
}

bool CPythonBackground::GetPickingPointWithRayOnlyTerrain(const CRay& rRay, D3DXVECTOR3* v3IntersectPt)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	return rkMap.GetPickingPointWithRayOnlyTerrain(rRay, v3IntersectPt);
}

BOOL CPythonBackground::GetLightDirection(D3DXVECTOR3& rv3LightDirection)
{
	if (!mc_pcurEnvironmentData)
		return FALSE;

	rv3LightDirection.x = mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.x;
	rv3LightDirection.y = mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.y;
	rv3LightDirection.z = mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.z;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CPythonBackground::Destroy()
{
	CMapManager::Destroy();
	m_SnowEnvironment.Destroy();
	m_bVisibleGuildArea = FALSE;
}

void CPythonBackground::Create()
{
	CMapManager::Create();
	m_SnowEnvironment.Create();
}

struct FGetPortalID
{
	float m_fRequestX, m_fRequestY;
	std::set<int32_t> m_kSet_iPortalID;
	FGetPortalID(float fRequestX, float fRequestY)
	{
		m_fRequestX = fRequestX;
		m_fRequestY = fRequestY;
	}
	void operator () (CGraphicObjectInstance* pObject)
	{
		for (int32_t i = 0; i < PORTAL_ID_MAX_NUM; ++i)
		{
			int32_t iID = pObject->GetPortal(i);
			if (0 == iID)
				break;

			m_kSet_iPortalID.emplace(iID);
		}
	}
};

void CPythonBackground::Update(float fCenterX, float fCenterY, float fCenterZ)
{
	if (!IsMapReady())
		return;

	UpdateMap(fCenterX, fCenterY, fCenterZ);
	UpdateAroundAmbience(fCenterX, fCenterY, fCenterZ);
	m_SnowEnvironment.Update(D3DXVECTOR3(fCenterX, -fCenterY, fCenterZ));

	// Portal Process
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	if (rkMap.IsEnablePortal())
	{
		auto rkCullingMgr = CCullingManager::GetInstance();
		FGetPortalID kGetPortalID(fCenterX, -fCenterY);

		Vector3d aVector3d;
		aVector3d.Set(fCenterX, -fCenterY, fCenterZ);

		Vector3d toTop;
		toTop.Set(0, 0, 25000.0f);

		rkCullingMgr->ForInRay(aVector3d, toTop, &kGetPortalID);

		if (!__IsSame(kGetPortalID.m_kSet_iPortalID, m_kSet_iShowingPortalID))
		{
			ClearPortal();
			auto itor = kGetPortalID.m_kSet_iPortalID.begin();
			for (; itor != kGetPortalID.m_kSet_iPortalID.end(); ++itor)
				AddShowingPortalID(*itor);
			RefreshPortal();

			m_kSet_iShowingPortalID = kGetPortalID.m_kSet_iPortalID;
		}
	}

	// Target Effect Process
	{
		auto itor = m_kMap_dwTargetID_dwChrID.begin();
		for (; itor != m_kMap_dwTargetID_dwChrID.end(); ++itor)
		{
			uint32_t dwTargetID = itor->first;
			uint32_t dwChrID = itor->second;

			CInstanceBase* pInstance = CPythonCharacterManager::GetInstance()->GetInstancePtr(dwChrID);

			if (!pInstance)
				continue;

			TPixelPosition kPixelPosition;
			pInstance->NEW_GetPixelPosition(&kPixelPosition);

			CreateSpecialEffect(dwTargetID,
				+kPixelPosition.x,
				-kPixelPosition.y,
				+kPixelPosition.z,
				g_strEffectName.c_str());
		}
	}

	// Reserve Target Effect
	{
		auto itor = m_kMap_dwID_kReserveTargetEffect.begin();
		for (; itor != m_kMap_dwID_kReserveTargetEffect.end();)
		{
			uint32_t dwID = itor->first;
			SReserveTargetEffect& rReserveTargetEffect = itor->second;

			auto ilx = float(rReserveTargetEffect.ilx);
			auto ily = float(rReserveTargetEffect.ily);

			float fHeight = rkMap.GetHeight(ilx, ily);
			if (0.0f == fHeight)
			{
				++itor;
				continue;
			}

			CreateSpecialEffect(dwID, ilx, ily, fHeight, g_strEffectName.c_str());

			itor = m_kMap_dwID_kReserveTargetEffect.erase(itor);
		}
	}
}

bool CPythonBackground::__IsSame(std::set<int32_t>& rleft, std::set<int32_t>& rright)
{
	for (auto& itor_l : rleft)
	{
		if (rright.end() == rright.find(itor_l))
			return false;
	}

	for (auto& itor_r : rright)
	{
		if (rleft.end() == rleft.find(itor_r))
			return false;
	}

	return true;
}

void CPythonBackground::Render()
{
	if (!IsMapReady())
		return;

	m_SnowEnvironment.Deform();

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.Render();
	if (m_bVisibleGuildArea)
		rkMap.RenderMarkedArea();
}

void CPythonBackground::RenderSnow()
{
	m_SnowEnvironment.Render();
}

void CPythonBackground::RenderPCBlocker()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderPCBlocker();
}

void CPythonBackground::RenderCollision()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderCollision();
}

void CPythonBackground::RenderShadow()
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderShadow();
	if (m_bVisibleGuildArea)
		rkMap.RenderMarkedArea();
}

void CPythonBackground::RenderTreeShadow()
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderTreeShadow();
	if (m_bVisibleGuildArea)
		rkMap.RenderMarkedArea();
}

void CPythonBackground::RenderCharacterShadowToTexture()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	uint32_t t1 = ELTimer_GetMSec();

	if (m_eShadowTargetLevel == SHADOW_ALL || m_eShadowTargetLevel == SHADOW_SELF_ONLY)
	{
		D3DXMATRIX matWorld;
		STATEMANAGER->GetTransform(D3DTS_WORLD, &matWorld);

		bool canRender = rkMap.BeginRenderCharacterShadowToTexture();
		if (canRender)
		{
			auto rkChrMgr = CPythonCharacterManager::GetInstance();

			rkChrMgr->RenderShadowAllInstances();
			if (ClientConfig::GetInstance()->IsDynamicShadow())
			{
				RenderShadow();
				RenderTreeShadow();
			}
		}
		rkMap.EndRenderCharacterShadowToTexture();

		STATEMANAGER->SetTransform(D3DTS_WORLD, &matWorld);
	}

	uint32_t t2 = ELTimer_GetMSec();

	m_dwRenderShadowTime = t2 - t1;
}

inline float Interpolate(float fStart, float fEnd, float fPercent)
{
	return fStart + (fEnd - fStart) * fPercent;
}

struct CollisionChecker
{
	bool isBlocked;
	CInstanceBase* pInstance;
	CollisionChecker(CInstanceBase* pInstance) : isBlocked(false), pInstance(pInstance) {}
	void operator () (CGraphicObjectInstance* pOpponent)
	{
		if (isBlocked)
			return;

		if (!pOpponent)
			return;

		if (pInstance->IsBlockObject(*pOpponent))
			isBlocked = true;
	}
};

struct CollisionAdjustChecker
{
	bool isBlocked;
	CInstanceBase* pInstance;
	CollisionAdjustChecker(CInstanceBase* pInstance) : isBlocked(false), pInstance(pInstance) {}
	void operator () (CGraphicObjectInstance* pOpponent)
	{
		if (!pOpponent)
			return;

		if (pInstance->AvoidObject(*pOpponent))
			isBlocked = true;
	}
};

bool CPythonBackground::CheckAdvancing(CInstanceBase* pInstance)
{
	if (!IsMapReady())
		return true;

	Vector3d center;
	float radius;
	pInstance->GetGraphicThingInstanceRef().GetBoundingSphere(center, radius);

	auto rkCullingMgr = CCullingManager::GetInstance();

	CollisionAdjustChecker kCollisionAdjustChecker(pInstance);
	rkCullingMgr->ForInRange(center, radius, &kCollisionAdjustChecker);
	if (kCollisionAdjustChecker.isBlocked)
	{
		CollisionChecker kCollisionChecker(pInstance);
		rkCullingMgr->ForInRange(center, radius, &kCollisionChecker);
		if (kCollisionChecker.isBlocked)
		{
			pInstance->BlockMovement();
			return true;
		}
		pInstance->NEW_MoveToDestPixelPositionDirection(pInstance->NEW_GetDstPixelPositionRef());
		return false;
	}
	return false;
}

void CPythonBackground::RenderSky()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderSky();
}

void CPythonBackground::RenderCloud()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderCloud();
}

void CPythonBackground::RenderWater()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderWater();
}

void CPythonBackground::RenderEffect()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderEffect();
}

void CPythonBackground::RenderBeforeLensFlare()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderBeforeLensFlare();
}

void CPythonBackground::RenderAfterLensFlare()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RenderAfterLensFlare();
}

void CPythonBackground::ClearGuildArea()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.ClearGuildArea();
}

void CPythonBackground::RegisterGuildArea(int32_t isx, int32_t isy, int32_t iex, int32_t iey)
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.RegisterGuildArea(isx, isy, iex, iey);
}

void CPythonBackground::SetCharacterDirLight()
{
	if (!IsMapReady())
		return;

	if (!mc_pcurEnvironmentData)
		return;

	STATEMANAGER->SetLight(0, &mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_CHARACTER]);
}

void CPythonBackground::SetBackgroundDirLight()
{
	if (!IsMapReady())
		return;
	if (!mc_pcurEnvironmentData)
		return;

	STATEMANAGER->SetLight(0, &mc_pcurEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND]);
}

void CPythonBackground::GlobalPositionToLocalPosition(int32_t& rGlobalX, int32_t& rGlobalY)
{
	rGlobalX -= m_dwBaseX;
	rGlobalY -= m_dwBaseY;
}

void CPythonBackground::LocalPositionToGlobalPosition(int32_t& rLocalX, int32_t& rLocalY)
{
	rLocalX += m_dwBaseX;
	rLocalY += m_dwBaseY;
}

void CPythonBackground::RegisterDungeonMapName(const char* c_szMapName)
{
	m_kSet_strDungeonMapName.emplace(c_szMapName);
}

CPythonBackground::TMapInfo* CPythonBackground::GlobalPositionToMapInfo(uint32_t dwGlobalX, uint32_t dwGlobalY)
{
	auto f = std::find_if(m_kVct_kMapInfo.begin(), m_kVct_kMapInfo.end(), FFindWarpMapName(dwGlobalX, dwGlobalY));
	if (f == m_kVct_kMapInfo.end())
		return nullptr;

	return &(*f);
}

void CPythonBackground::Warp(uint32_t dwX, uint32_t dwY)
{
	TMapInfo* pkMapInfo = GlobalPositionToMapInfo(dwX, dwY);
	if (!pkMapInfo)
	{
		SysLog("NOT_FOUND_GLOBAL_POSITION({0}, {1})", dwX, dwY);
		return;
	}

	RefreshShadowTargetLevel();
	RefreshShadowQualityLevel();

	TMapInfo& rMapInfo = *pkMapInfo;
	assert((dwX >= rMapInfo.m_dwBaseX) && (dwY >= rMapInfo.m_dwBaseY));

	if (!LoadMap(rMapInfo.m_strName, float(dwX - rMapInfo.m_dwBaseX), float(dwY - rMapInfo.m_dwBaseY), 0))
	{
		PostQuitMessage(0);
		return;
	}

	CPythonMiniMap::GetInstance()->LoadAtlas();

	m_dwBaseX = rMapInfo.m_dwBaseX;
	m_dwBaseY = rMapInfo.m_dwBaseY;

	m_strMapName = rMapInfo.m_strName;

	SetXMaxTree(m_iXMasTreeGrade);

	if (m_kSet_strDungeonMapName.end() != m_kSet_strDungeonMapName.find(m_strMapName))
	{
		CMapOutdoor& rkMap = GetMapOutdoorRef();
		rkMap.EnablePortal(TRUE);
	}

	m_kSet_iShowingPortalID.clear();
	m_kMap_dwTargetID_dwChrID.clear();
	m_kMap_dwID_kReserveTargetEffect.clear();
}

void CPythonBackground::VisibleGuildArea()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.VisibleMarkedArea();

	m_bVisibleGuildArea = TRUE;
}

void CPythonBackground::DisableGuildArea()
{
	if (!IsMapReady())
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.DisableMarkedArea();

	m_bVisibleGuildArea = FALSE;
}

const char* CPythonBackground::GetWarpMapName()
{
	return m_strMapName.c_str();
}

std::string CPythonBackground::GetMapName(uint32_t dwX, uint32_t dwY)
{
	TMapInfo* pkMapInfo = GlobalPositionToMapInfo(dwX, dwY);
	if (!pkMapInfo)
	{
		SysLog("NOT_FOUND_GLOBAL_POSITION({0}, {1})", dwX, dwY);
		return "";
	}
	return pkMapInfo->m_strName;
}

void CPythonBackground::ChangeToDay()
{
	m_iDayMode = DAY_MODE_LIGHT;
}

void CPythonBackground::ChangeToNight()
{
	m_iDayMode = DAY_MODE_DARK;
}

void CPythonBackground::EnableSnowEnvironment()
{
	m_SnowEnvironment.Enable();
}

void CPythonBackground::DisableSnowEnvironment()
{
	m_SnowEnvironment.Disable();
}

const D3DXVECTOR3 c_v3TreePos = D3DXVECTOR3(76500.0f, -60900.0f, 20215.0f);

void CPythonBackground::SetXMaxTree(int32_t iGrade)
{
	if (!m_pkMap)
		return;

	assert(iGrade >= 0 && iGrade <= 3);
	m_iXMasTreeGrade = iGrade;

	CMapOutdoor& rkMap = GetMapOutdoorRef();

	if ("map_n_snowm_01" != m_strMapName)
	{
		rkMap.XMasTree_Destroy();
		return;
	}

	if (0 == iGrade)
	{
		rkMap.XMasTree_Destroy();
		return;
	}

	//////////////////////////////////////////////////////////////////////

	iGrade -= 1;
	iGrade = std::max(iGrade, 0);
	iGrade = std::min(iGrade, 2);

	static std::string s_strTreeName[3] = {
		"d:/ymir work/tree/christmastree1.spt",
		"d:/ymir work/tree/christmastree2.spt",
		"d:/ymir work/tree/christmastree3.spt"
	};
	static std::string s_strEffectName[3] = {
		"d:/ymir work/effect/etc/christmas_tree/tree_1s.mse",
		"d:/ymir work/effect/etc/christmas_tree/tree_2s.mse",
		"d:/ymir work/effect/etc/christmas_tree/tree_3s.mse",
	};
	rkMap.XMasTree_Set(c_v3TreePos.x, c_v3TreePos.y, c_v3TreePos.z, s_strTreeName[iGrade].c_str(), s_strEffectName[iGrade].c_str());
}

void CPythonBackground::CreateTargetEffect(uint32_t dwID, uint32_t dwChrVID)
{
	m_kMap_dwTargetID_dwChrID.emplace(dwID, dwChrVID);
}

void CPythonBackground::CreateTargetEffect(uint32_t dwID, int32_t lx, int32_t ly)
{
	if (m_kMap_dwTargetID_dwChrID.end() != m_kMap_dwTargetID_dwChrID.find(dwID))
		return;

	CMapOutdoor& rkMap = GetMapOutdoorRef();

	float fHeight = m_pkMap->GetHeight(float(lx), float(ly));

	if (0.0f == fHeight)
	{
		SReserveTargetEffect ReserveTargetEffect{};
		ReserveTargetEffect.ilx = lx;
		ReserveTargetEffect.ily = ly;
		m_kMap_dwID_kReserveTargetEffect.emplace(dwID, ReserveTargetEffect);
		return;
	}

	CreateSpecialEffect(dwID, lx, ly, fHeight, g_strEffectName.c_str());
}

void CPythonBackground::DeleteTargetEffect(uint32_t dwID)
{
	if (m_kMap_dwID_kReserveTargetEffect.end() != m_kMap_dwID_kReserveTargetEffect.find(dwID))
		m_kMap_dwID_kReserveTargetEffect.erase(dwID);
	if (m_kMap_dwTargetID_dwChrID.end() != m_kMap_dwTargetID_dwChrID.find(dwID))
		m_kMap_dwTargetID_dwChrID.erase(dwID);

	DeleteSpecialEffect(dwID);
}

void CPythonBackground::CreateSpecialEffect(uint32_t dwID, float fx, float fy, float fz, const char* c_szFileName)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.SpecialEffect_Create(dwID, fx, fy, fz, c_szFileName);
}

void CPythonBackground::DeleteSpecialEffect(uint32_t dwID)
{
	CMapOutdoor& rkMap = GetMapOutdoorRef();
	rkMap.SpecialEffect_Delete(dwID);
}
