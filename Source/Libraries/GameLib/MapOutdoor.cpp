#include "StdAfx.h"
#include <EterLib/StateManager.h>
#include <EterLib/Camera.h>
#include <EterLib/CullingManager.h>
#include <EffectLib/EffectManager.h>
#include <PRTerrainLib/StdAfx.h>

#include "MapOutdoor.h"
#include "TerrainPatch.h"
#include "AreaTerrain.h"

struct FGetObjectHeight
{
	bool m_bHeightFound;
	float m_fReturnHeight;
	float m_fRequestX, m_fRequestY;

	FGetObjectHeight(float fRequestX, float fRequestY)
	{
		m_fRequestX = fRequestX;
		m_fRequestY = fRequestY;
		m_bHeightFound = false;
		m_fReturnHeight = 0.0f;
	}

	void operator()(CGraphicObjectInstance* pObject)
	{
		if (pObject->GetObjectHeight(m_fRequestX, m_fRequestY, &m_fReturnHeight))
		{
			m_bHeightFound = true;
		}
	}
};

struct FGetPickingPoint
{
	D3DXVECTOR3 m_v3Start;
	D3DXVECTOR3 m_v3Dir;
	D3DXVECTOR3 m_v3PickingPoint;
	bool m_bPicked;

	FGetPickingPoint(D3DXVECTOR3& v3Start, D3DXVECTOR3& v3Dir) : m_v3Start(v3Start), m_v3Dir(v3Dir), m_bPicked(false) {}
	void operator() (CGraphicObjectInstance* pInstance)
	{
		if (pInstance && pInstance->GetType() == CGraphicThingInstance::ID)
		{
			CGraphicThingInstance* pThing = (CGraphicThingInstance*)pInstance;
			if (!pThing->IsObjectHeight())
				return;

			float fX, fY, fZ;
			if (pThing->Picking(m_v3Start, m_v3Dir, fX, fY))
			{
				if (pThing->GetObjectHeight(fX, -fY, &fZ))
				{
					m_v3PickingPoint.x = fX;
					m_v3PickingPoint.y = fY;
					m_v3PickingPoint.z = fZ;
					m_bPicked = true;
				}
			}
		}
	}
};

CMapOutdoor::CMapOutdoor()
{
	CGraphicImage* pAttrImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>("d:/ymir work/special/white.dds");
	CGraphicImage* pBuildTransparentImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>("d:/ymir Work/special/PCBlockerAlpha.dds");
	m_attrImageInstance.SetImagePointer(pAttrImage);
	m_BuildingTransparentImageInstance.SetImagePointer(pBuildTransparentImage);
	CMapOutdoor::Initialize();
}

CMapOutdoor::~CMapOutdoor()
{
	CMapOutdoor::Destroy();
}

bool CMapOutdoor::Initialize()
{
	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		m_pArea[i] = nullptr;
		m_pTerrain[i] = nullptr;
	}

	m_pTerrainPatchProxyList = nullptr;
	m_lViewRadius = 0L;
	m_fHeightScale = 0.0f;
	m_sTerrainCountX = m_sTerrainCountY = 0;
	m_CurCoordinate.m_sTerrainCoordX = -1;
	m_CurCoordinate.m_sTerrainCoordY = -1;
	m_PrevCoordinate.m_sTerrainCoordX = -1;
	m_PrevCoordinate.m_sTerrainCoordY = -1;
	m_lCenterX = m_lCenterY = 0;
	m_lOldReadX = m_lOldReadY = -1;

	std::memset(m_pwaIndices, 0, sizeof(m_pwaIndices));
	for (auto i = 0; i < TERRAINPATCH_LODMAX; ++i)
		m_IndexBuffer[i].Destroy();

	m_bSettingTerrainVisible = false;
	m_bSettingIsLavaMap = false;
	m_bDrawWireFrame = false;
	m_bDrawShadow = false;
	m_bDrawChrShadow = false;

	m_iSplatLimit = 50000;
	m_wPatchCount = 0;
	m_pRootNode = nullptr;
	m_lpCharacterShadowMapTexture = nullptr;
	m_lpCharacterShadowMapColorSurface = nullptr;
	m_lpBackupRenderTargetSurface = nullptr;
	m_lpBackupDepthSurface = nullptr;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_fOpaqueWaterDepth = 400.0f;

	m_TerrainVector.clear();
	m_TerrainDeleteVector.clear();
	m_AreaVector.clear();
	m_AreaDeleteVector.clear();
	m_PatchVector.clear();

	m_eTerrainRenderSort = DISTANCE_SORT;
	D3DXMatrixIdentity(&m_matWorldForCommonUse);
	InitializeVisibleParts();

	//m_dwBaseX = 0;
	//m_dwBaseY = 0;

	m_settings_envDataName.clear();
	m_bShowEntirePatchTextureCount = false;
	m_bTransparentTree = true;
	CMapBase::Clear();

	__XMasTree_Initialize();
	SpecialEffect_Destroy();

	//m_bEnableTerrainOnlyForHeight = FALSE;
	m_bEnablePortal = false;

	m_wShadowMapSize = 512;
	m_fShadowSizeX = 11000.0f;
	m_fShadowSizeY = 11000.0f;
	m_fShadowDistance = 7000.0f;

	return true;
}


bool CMapOutdoor::Destroy()
{
	//m_bEnableTerrainOnlyForHeight = FALSE;
	m_bEnablePortal = false;

	XMasTree_Destroy();
	DestroyTerrain();
	DestroyArea();
	DestroyTerrainPatchProxyList();
	FreeQuadTree();
	ReleaseCharacterShadowTexture();
	m_settings_envDataName.clear();

	m_rkList_kGuildArea.clear();
	CSpeedTreeForestDirectX9::GetInstance()->Clear();

	return true;
}

void CMapOutdoor::Clear()
{
	UnloadWaterTexture();
	Destroy();
	Initialize();
}

bool CMapOutdoor::SetTerrainCount(int16_t sTerrainCountX, int16_t sTerrainCountY)
{
	if (0 == sTerrainCountX || MAX_MAPSIZE < sTerrainCountX)
		return false;

	if (0 == sTerrainCountY || MAX_MAPSIZE < sTerrainCountY)
		return false;

	m_sTerrainCountX = sTerrainCountX;
	m_sTerrainCountY = sTerrainCountY;
	return true;
}

void CMapOutdoor::OnBeginEnvironment()
{
	if (!mc_pEnvironmentData)
		return;

	auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
	rkForest->SetFog(mc_pEnvironmentData->GetFogNearDistance(), mc_pEnvironmentData->GetFogFarDistance());

	const D3DLIGHT9& c_rkLight = mc_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND];
	rkForest->SetLight((const float*)&c_rkLight.Direction, (const float*)&c_rkLight.Ambient, (const float*)&c_rkLight.Diffuse);

	// We dont even support wind strength! Yet!
	//rkForest.SetWindStrength(mc_pEnvironmentData->fWindStrength);
}

void CMapOutdoor::OnSetEnvironmentDataPtr()
{
	SetEnvironmentScreenFilter();
	SetEnvironmentSkyBox();
	SetEnvironmentLensFlare();
}

void CMapOutdoor::OnResetEnvironmentDataPtr()
{
	m_SkyBox.Unload();
	SetEnvironmentScreenFilter();
	SetEnvironmentSkyBox();
	SetEnvironmentLensFlare();
}

void CMapOutdoor::SetEnvironmentScreenFilter()
{
	if (!mc_pEnvironmentData)
		return;

	m_ScreenFilter.SetEnable(mc_pEnvironmentData->bFilteringEnable);
	m_ScreenFilter.SetBlendType(mc_pEnvironmentData->byFilteringAlphaSrc, mc_pEnvironmentData->byFilteringAlphaDest);
	m_ScreenFilter.SetColor(mc_pEnvironmentData->FilteringColor);
}

void CMapOutdoor::SetEnvironmentSkyBox()
{
	if (!mc_pEnvironmentData)
		return;

	m_SkyBox.SetSkyBoxScale(mc_pEnvironmentData->v3SkyBoxScale);
	m_SkyBox.SetGradientLevel(mc_pEnvironmentData->bySkyBoxGradientLevelUpper, mc_pEnvironmentData->bySkyBoxGradientLevelLower);
	m_SkyBox.SetRenderMode((mc_pEnvironmentData->bSkyBoxTextureRenderMode != FALSE) ? CSkyObject::SKY_RENDER_MODE_TEXTURE : CSkyObject::SKY_RENDER_MODE_DIFFUSE);

	for (int32_t i = 0; i < 6; ++i)
	{
		if (!mc_pEnvironmentData->strSkyBoxFaceFileName[i].empty())
			m_SkyBox.SetFaceTexture(mc_pEnvironmentData->strSkyBoxFaceFileName[i].c_str(), i);
	}

	if (!mc_pEnvironmentData->strCloudTextureFileName.empty())
		m_SkyBox.SetCloudTexture(mc_pEnvironmentData->strCloudTextureFileName.c_str());

	m_SkyBox.SetCloudScale(mc_pEnvironmentData->v2CloudScale);
	m_SkyBox.SetCloudHeight(mc_pEnvironmentData->fCloudHeight);
	m_SkyBox.SetCloudTextureScale(mc_pEnvironmentData->v2CloudTextureScale);
	m_SkyBox.SetCloudScrollSpeed(mc_pEnvironmentData->v2CloudSpeed);
	m_SkyBox.Refresh();

	// Temporary
	m_SkyBox.SetCloudColor(mc_pEnvironmentData->CloudGradientColor, mc_pEnvironmentData->CloudGradientColor, 1);

	if (!mc_pEnvironmentData->SkyBoxGradientColorVector.empty())
		m_SkyBox.SetSkyColor(mc_pEnvironmentData->SkyBoxGradientColorVector, mc_pEnvironmentData->SkyBoxGradientColorVector, 1);
	// Temporary

	m_SkyBox.StartTransition();
}

void CMapOutdoor::SetEnvironmentLensFlare()
{
	if (!mc_pEnvironmentData)
		return;

	m_LensFlare.CharacterizeFlare(mc_pEnvironmentData->bLensFlareEnable != FALSE ? true : false,
		mc_pEnvironmentData->bMainFlareEnable != FALSE ? true : false,
		mc_pEnvironmentData->fLensFlareMaxBrightness,
		mc_pEnvironmentData->LensFlareBrightnessColor);

	m_LensFlare.Initialize("d:/ymir work/environment");

	if (!mc_pEnvironmentData->strMainFlareTextureFileName.empty())
		m_LensFlare.SetMainFlare(mc_pEnvironmentData->strMainFlareTextureFileName.c_str(),
			mc_pEnvironmentData->fMainFlareSize);
}

void CMapOutdoor::SetWireframe(bool bWireFrame)
{
	m_bDrawWireFrame = bWireFrame;
}

bool CMapOutdoor::IsWireframe()
{
	return m_bDrawWireFrame;
}

// TerrainPatchList
void CMapOutdoor::CreateTerrainPatchProxyList()
{
	m_wPatchCount = ((m_lViewRadius * 2) / TERRAIN_PATCHSIZE) + 2;

	m_pTerrainPatchProxyList = new CTerrainPatchProxy[m_wPatchCount * m_wPatchCount];

	m_iPatchTerrainVertexCount = (TERRAIN_PATCHSIZE + 1) * (TERRAIN_PATCHSIZE + 1);
	m_iPatchWaterVertexCount = TERRAIN_PATCHSIZE * TERRAIN_PATCHSIZE * 6;
	m_iPatchTerrainVertexSize = 24;
	m_iPatchWaterVertexSize = 16;

	SetIndexBuffer();
}

void CMapOutdoor::DestroyTerrainPatchProxyList()
{
	if (m_pTerrainPatchProxyList)
	{
		delete[] m_pTerrainPatchProxyList;
		m_pTerrainPatchProxyList = nullptr;
	}

	for (int32_t i = 0; i < TERRAINPATCH_LODMAX; ++i)
		m_IndexBuffer[i].Destroy();
}

// Area
void CMapOutdoor::EnablePortal(bool bFlag)
{
	m_bEnablePortal = bFlag;

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
		if (m_pArea[i])
			m_pArea[i]->EnablePortal(bFlag);
}

void CMapOutdoor::DestroyArea()
{
	m_AreaVector.clear();
	m_AreaDeleteVector.clear();

	for (auto& i : m_pArea)
		i = nullptr;
}

// Terrain
void CMapOutdoor::DestroyTerrain()
{
	m_TerrainVector.clear();
	m_TerrainDeleteVector.clear();

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
		m_pTerrain[i] = nullptr;
}

bool CMapOutdoor::GetTerrainNum(float fx, float fy, uint8_t* pbyTerrainNum)
{
	if (fy < 0)
		fy = -fy;

	int32_t ix, iy;

	PR_FLOAT_TO_INT(fx, ix);
	PR_FLOAT_TO_INT(fy, iy);

	uint16_t wTerrainNumX = ix / (CTerrainImpl::TERRAIN_XSIZE);
	uint16_t wTerrainNumY = iy / (CTerrainImpl::TERRAIN_YSIZE);

	return GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, pbyTerrainNum);
}

bool CMapOutdoor::GetPickingPoint(D3DXVECTOR3* v3IntersectPt)
{
	return GetPickingPointWithRay(ms_Ray, v3IntersectPt);
}

bool CMapOutdoor::__PickTerrainHeight(float& fPos, const D3DXVECTOR3& v3Start, const D3DXVECTOR3& v3End, float fStep, float fRayRange, float fLimitRange, D3DXVECTOR3* pv3Pick)
{
	CTerrain* pTerrain;

	D3DXVECTOR3 v3CurPos;

	float fRayRangeInv = 1.0f / fRayRange;
	while (fPos < fRayRange && fPos < fLimitRange)
	{
		D3DXVec3Lerp(&v3CurPos, &v3Start, &v3End, fPos * fRayRangeInv);
		uint8_t byTerrainNum;
		float fMultiplier = 1.0f;
		if (GetTerrainNum(v3CurPos.x, v3CurPos.y, &byTerrainNum))
		{
			if (GetTerrainPointer(byTerrainNum, &pTerrain))
			{
				int32_t ix, iy;
				PR_FLOAT_TO_INT(v3CurPos.x, ix);
				PR_FLOAT_TO_INT(fabs(v3CurPos.y), iy);
				float fMapHeight = pTerrain->GetHeight(ix, iy);
				if (fMapHeight >= v3CurPos.z)
				{
					*pv3Pick = v3CurPos;
					return true;
				}
				else
				{
					fMultiplier = fMAX(1.0f, 0.01f * (v3CurPos.z - fMapHeight));
				}
			}
		}
		fPos += fStep * fMultiplier;
	}

	return false;
}

bool CMapOutdoor::GetPickingPointWithRay(const CRay& rRay, D3DXVECTOR3* v3IntersectPt)
{
	bool bObjectPick = false;
	bool bTerrainPick = false;
	D3DXVECTOR3 v3ObjectPick, v3TerrainPick;

	D3DXVECTOR3 v3Start, v3End, v3Dir;
	float fRayRange;
	rRay.GetStartPoint(&v3Start);
	rRay.GetDirection(&v3Dir, &fRayRange);
	rRay.GetEndPoint(&v3End);

	Vector3d v3dStart, v3dEnd;
	v3dStart.Set(v3Start.x, v3Start.y, v3Start.z);
	v3dEnd.Set(v3End.x - v3Start.x, v3End.y - v3Start.y, v3End.z - v3Start.z);

	if (IsVisiblePart(PART_TERRAIN))
	{
		auto rkCullingMgr = CCullingManager::GetInstance();
		FGetPickingPoint kGetPickingPoint(v3Start, v3Dir);
		rkCullingMgr->ForInRange2d(v3dStart, &kGetPickingPoint);

		if (kGetPickingPoint.m_bPicked)
		{
			bObjectPick = true;
			v3ObjectPick = kGetPickingPoint.m_v3PickingPoint;
		}
	}

	float fPos = 0.0f;

	bTerrainPick = true;
	if (!__PickTerrainHeight(fPos, v3Start, v3End, 5.0f, fRayRange, 5000.0f, &v3TerrainPick))
		if (!__PickTerrainHeight(fPos, v3Start, v3End, 10.0f, fRayRange, 10000.0f, &v3TerrainPick))
			if (!__PickTerrainHeight(fPos, v3Start, v3End, 100.0f, fRayRange, 100000.0f, &v3TerrainPick))
				bTerrainPick = false;


	if (bObjectPick && bTerrainPick)
	{
		const D3DXVECTOR3 cCalc0 = (v3TerrainPick - v3Start);
		const D3DXVECTOR3 cCalc1 = (v3ObjectPick - v3Start);
		if (D3DXVec3Length(&cCalc1) >= D3DXVec3Length(&cCalc0))
			*v3IntersectPt = v3TerrainPick;
		else
			*v3IntersectPt = v3ObjectPick;
		return true;
	}
	else if (bObjectPick)
	{
		*v3IntersectPt = v3ObjectPick;
		return true;
	}
	else if (bTerrainPick)
	{
		*v3IntersectPt = v3TerrainPick;
		return true;
	}

	return false;
}

bool CMapOutdoor::GetPickingPointWithRayOnlyTerrain(const CRay& rRay, D3DXVECTOR3* v3IntersectPt)
{
	D3DXVECTOR3 v3TerrainPick;

	D3DXVECTOR3 v3Start, v3End, v3Dir;
	float fRayRange;
	rRay.GetStartPoint(&v3Start);
	rRay.GetDirection(&v3Dir, &fRayRange);
	rRay.GetEndPoint(&v3End);

	Vector3d v3dStart, v3dEnd;
	v3dStart.Set(v3Start.x, v3Start.y, v3Start.z);
	v3dEnd.Set(v3End.x - v3Start.x, v3End.y - v3Start.y, v3End.z - v3Start.z);

	float fPos = 0.0f;
	bool bTerrainPick = true;
	if (!__PickTerrainHeight(fPos, v3Start, v3End, 5.0f, fRayRange, 5000.0f, &v3TerrainPick))
		if (!__PickTerrainHeight(fPos, v3Start, v3End, 10.0f, fRayRange, 10000.0f, &v3TerrainPick))
			if (!__PickTerrainHeight(fPos, v3Start, v3End, 100.0f, fRayRange, 100000.0f, &v3TerrainPick))
				bTerrainPick = false;

	if (bTerrainPick)
	{
		*v3IntersectPt = v3TerrainPick;
		return true;
	}

	return false;
}

void CMapOutdoor::GetHeightMap(const uint8_t& c_rucTerrainNum, uint16_t** pwHeightMap)
{
	if (c_rucTerrainNum < 0 || c_rucTerrainNum > AROUND_AREA_NUM - 1 || !m_pTerrain[c_rucTerrainNum])
	{
		*pwHeightMap = nullptr;
		return;
	}

	*pwHeightMap = m_pTerrain[c_rucTerrainNum]->GetHeightMap();
}

void CMapOutdoor::GetNormalMap(const uint8_t& c_rucTerrainNum, char** pucNormalMap)
{
	if (c_rucTerrainNum < 0 || c_rucTerrainNum > AROUND_AREA_NUM - 1 || !m_pTerrain[c_rucTerrainNum])
	{
		*pucNormalMap = nullptr;
		return;
	}

	*pucNormalMap = m_pTerrain[c_rucTerrainNum]->GetNormalMap();
}

void CMapOutdoor::GetWaterMap(const uint8_t& c_rucTerrainNum, uint8_t** pucWaterMap)
{
	if (c_rucTerrainNum < 0 || c_rucTerrainNum > AROUND_AREA_NUM - 1 || !m_pTerrain[c_rucTerrainNum])
	{
		*pucWaterMap = nullptr;
		return;
	}

	*pucWaterMap = m_pTerrain[c_rucTerrainNum]->GetWaterMap();
}

void CMapOutdoor::GetWaterHeight(uint8_t byTerrainNum, uint8_t byWaterNum, int32_t* plWaterHeight)
{
	if (byTerrainNum < 0 || byTerrainNum > AROUND_AREA_NUM - 1 || !m_pTerrain[byTerrainNum])
	{
		*plWaterHeight = -1;
		return;
	}

	m_pTerrain[byTerrainNum]->GetWaterHeight(byWaterNum, plWaterHeight);
}

bool CMapOutdoor::GetWaterHeight(int32_t iX, int32_t iY, int32_t* plWaterHeight)
{
	if (iX < 0 || iY < 0 || iX > m_sTerrainCountX * CTerrainImpl::TERRAIN_XSIZE || iY > m_sTerrainCountY * CTerrainImpl::TERRAIN_YSIZE)
		return false;

	uint16_t wTerrainCoordX, wTerrainCoordY;
	wTerrainCoordX = iX / CTerrainImpl::TERRAIN_XSIZE;
	wTerrainCoordY = iY / CTerrainImpl::TERRAIN_YSIZE;

	uint8_t byTerrainNum;
	if (!GetTerrainNumFromCoord(wTerrainCoordX, wTerrainCoordY, &byTerrainNum))
		return false;
	CTerrain* pTerrain;
	if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		return false;

	uint16_t wLocalX, wLocalY;
	wLocalX = (iX - wTerrainCoordX * CTerrainImpl::TERRAIN_XSIZE) / (CTerrainImpl::WATERMAP_XSIZE);
	wLocalY = (iY - wTerrainCoordY * CTerrainImpl::TERRAIN_YSIZE) / (CTerrainImpl::WATERMAP_YSIZE);

	return pTerrain->GetWaterHeight(wLocalX, wLocalY, plWaterHeight);
}

// Update
bool CMapOutdoor::GetTerrainNumFromCoord(uint16_t wCoordX, uint16_t wCoordY, uint8_t* pbyTerrainNum)
{
	*pbyTerrainNum = (wCoordY - m_CurCoordinate.m_sTerrainCoordY + LOAD_SIZE_WIDTH) * 3 + (wCoordX - m_CurCoordinate.m_sTerrainCoordX + LOAD_SIZE_WIDTH);

	if (*pbyTerrainNum < 0 || *pbyTerrainNum > AROUND_AREA_NUM)
		return false;

	return true;
}

void CMapOutdoor::BuildViewFrustum(D3DXMATRIX& mat)
{
	m_plane[0] = D3DXPLANE(mat._13, mat._23, mat._33, mat._43);		// Near
	m_plane[1] = D3DXPLANE(mat._14 - mat._13, mat._24 - mat._23, mat._34 - mat._33, mat._44 - mat._43);		// Far
	m_plane[2] = D3DXPLANE(mat._14 + mat._11, mat._24 + mat._21, mat._34 + mat._31, mat._44 + mat._41);		// Left
	m_plane[3] = D3DXPLANE(mat._14 - mat._11, mat._24 - mat._21, mat._34 - mat._31, mat._44 - mat._41);		// Right
	m_plane[4] = D3DXPLANE(mat._14 + mat._12, mat._24 + mat._22, mat._34 + mat._32, mat._44 + mat._42);		// Bottom
	m_plane[5] = D3DXPLANE(mat._14 - mat._12, mat._24 - mat._22, mat._34 - mat._32, mat._44 - mat._42);		// Top

	for (int32_t i = 0; i < 6; ++i)
		D3DXPlaneNormalize(&m_plane[i], &m_plane[i]);
}

float CMapOutdoor::GetHeight(float fx, float fy)
{
	float fTerrainHeight = GetTerrainHeight(fx, fy);

	if (IsVisiblePart(PART_TERRAIN))
	{
		auto rkCullingMgr = CCullingManager::GetInstance();

		float CHECK_HEIGHT = 25000.0f;
		float fObjectHeight = -CHECK_HEIGHT;

		Vector3d aVector3d;
		aVector3d.Set(fx, -fy, fTerrainHeight);

		FGetObjectHeight kGetObjHeight(fx, fy);

		RangeTester<FGetObjectHeight> kRangeTester_kGetObjHeight(&kGetObjHeight);
		rkCullingMgr->PointTest2d(aVector3d, &kRangeTester_kGetObjHeight);

		if (kGetObjHeight.m_bHeightFound)
			fObjectHeight = kGetObjHeight.m_fReturnHeight;

		return std::max(fObjectHeight, fTerrainHeight);
	}

	return fTerrainHeight;
}

bool CMapOutdoor::GetNormal(int32_t ix, int32_t iy, D3DXVECTOR3* pv3Normal)
{
	if (ix <= 0)
		ix = 0;
	else if (ix >= m_sTerrainCountX * CTerrainImpl::TERRAIN_XSIZE)
		ix = m_sTerrainCountX * CTerrainImpl::TERRAIN_XSIZE;

	if (iy <= 0)
		iy = 0;
	else if (iy >= m_sTerrainCountY * CTerrainImpl::TERRAIN_YSIZE)
		iy = m_sTerrainCountY * CTerrainImpl::TERRAIN_YSIZE;

	uint16_t usCoordX, usCoordY;

	usCoordX = (uint16_t)(ix / (CTerrainImpl::TERRAIN_XSIZE));
	usCoordY = (uint16_t)(iy / (CTerrainImpl::TERRAIN_YSIZE));

	if (usCoordX >= m_sTerrainCountX - 1)
		usCoordX = m_sTerrainCountX - 1;

	if (usCoordY >= m_sTerrainCountY - 1)
		usCoordY = m_sTerrainCountY - 1;

	uint8_t byTerrainNum;
	if (!GetTerrainNumFromCoord(usCoordX, usCoordY, &byTerrainNum))
		return false;

	CTerrain* pTerrain;

	if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		return false;

	while (ix >= CTerrainImpl::TERRAIN_XSIZE)
		ix -= CTerrainImpl::TERRAIN_XSIZE;

	while (iy >= CTerrainImpl::TERRAIN_YSIZE)
		iy -= CTerrainImpl::TERRAIN_YSIZE;

	return pTerrain->GetNormal(ix, iy, pv3Normal);
}

float CMapOutdoor::GetTerrainHeight(float fx, float fy)
{
	if (fy < 0)
		fy = -fy;
	int32_t lx, ly;
	PR_FLOAT_TO_INT(fx, lx);
	PR_FLOAT_TO_INT(fy, ly);

	uint16_t usCoordX, usCoordY;

	usCoordX = (uint16_t)(lx / CTerrainImpl::TERRAIN_XSIZE);
	usCoordY = (uint16_t)(ly / CTerrainImpl::TERRAIN_YSIZE);

	uint8_t byTerrainNum;
	if (!GetTerrainNumFromCoord(usCoordX, usCoordY, &byTerrainNum))
		return 0.0f;

	CTerrain* pTerrain;

	if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		return 0.0f;

	return pTerrain->GetHeight(lx, ly);
}

std::optional<CArea*> CMapOutdoor::GetAreaPointer(const uint8_t c_byAreaNum)
{
	if (c_byAreaNum >= AROUND_AREA_NUM)
		return std::nullopt;

	if (nullptr == m_pArea[c_byAreaNum])
		return std::nullopt;

	return m_pArea[c_byAreaNum];
}

bool CMapOutdoor::GetAreaPointer(const uint8_t c_byAreaNum, CArea** ppArea)
{
	if (c_byAreaNum >= AROUND_AREA_NUM)
	{
		*ppArea = nullptr;
		return false;
	}

	if (nullptr == m_pArea[c_byAreaNum])
	{
		*ppArea = nullptr;
		return false;
	}

	*ppArea = m_pArea[c_byAreaNum];
	return true;
}

bool CMapOutdoor::GetTerrainPointer(const uint8_t c_byTerrainNum, CTerrain** ppTerrain)
{
	if (c_byTerrainNum >= AROUND_AREA_NUM)
	{
		*ppTerrain = nullptr;
		return false;
	}

	if (nullptr == m_pTerrain[c_byTerrainNum])
	{
		*ppTerrain = nullptr;
		return false;
	}

	*ppTerrain = m_pTerrain[c_byTerrainNum];
	return true;
}

void CMapOutdoor::SetDrawShadow(bool bDrawShadow)
{
	m_bDrawShadow = bDrawShadow;
}

void CMapOutdoor::SetDrawCharacterShadow(bool bDrawChrShadow)
{
	m_bDrawChrShadow = bDrawChrShadow;
}

void CMapOutdoor::SetDrawBackgroundShadow(bool bDrawBgShadow)
{
	m_bDrawBgShadow = bDrawBgShadow;
}

uint32_t CMapOutdoor::GetShadowMapColor(float fx, float fy)
{
	if (fy < 0)
		fy = -fy;

	auto fTerrainSize = (float)(CTerrainImpl::TERRAIN_XSIZE);
	float fXRef = fx - (float)(m_lCurCoordStartX);
	float fYRef = fy - (float)(m_lCurCoordStartY);

	CTerrain* pTerrain;

	if (fYRef < -fTerrainSize)
		return 0xFFFFFFFF;
	if (fYRef >= -fTerrainSize && fYRef < 0.0f)
	{
		if (fXRef < -fTerrainSize)
			return 0xFFFFFFFF;
		if (fXRef >= -fTerrainSize && fXRef < 0.0f)
		{
			if (GetTerrainPointer(0, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef + fTerrainSize, fYRef + fTerrainSize);
			return 0xFFFFFFFF;
		}
		if (fXRef >= 0.0f && fXRef < fTerrainSize)
		{
			if (GetTerrainPointer(1, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef, fYRef + fTerrainSize);
			return 0xFFFFFFFF;
		}
		if (fXRef >= fTerrainSize && fXRef < 2.0f * fTerrainSize)
		{
			if (GetTerrainPointer(2, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef - fTerrainSize, fYRef + fTerrainSize);
			return 0xFFFFFFFF;
		}
		return 0xFFFFFFFF;
	}
	if (fYRef >= 0.0f && fYRef < fTerrainSize)
	{
		if (fXRef < -fTerrainSize)
			return 0xFFFFFFFF;
		if (fXRef >= -fTerrainSize && fXRef < 0.0f)
		{
			if (GetTerrainPointer(3, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef + fTerrainSize, fYRef);
			return 0xFFFFFFFF;
		}
		if (fXRef >= 0.0f && fXRef < fTerrainSize)
		{
			if (GetTerrainPointer(4, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef, fYRef);
			return 0xFFFFFFFF;
		}
		if (fXRef >= fTerrainSize && fXRef < 2.0f * fTerrainSize)
		{
			if (GetTerrainPointer(5, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef - fTerrainSize, fYRef);
			return 0xFFFFFFFF;
		}
		return 0xFFFFFFFF;
	}
	if (fYRef >= fTerrainSize && fYRef < 2.0f * fTerrainSize)
	{
		if (fXRef < -fTerrainSize)
			return 0xFFFFFFFF;
		if (fXRef >= -fTerrainSize && fXRef < 0.0f)
		{
			if (GetTerrainPointer(6, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef + fTerrainSize, fYRef - fTerrainSize);
			return 0xFFFFFFFF;
		}
		if (fXRef >= 0.0f && fXRef < fTerrainSize)
		{
			if (GetTerrainPointer(7, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef, fYRef - fTerrainSize);
			return 0xFFFFFFFF;
		}
		if (fXRef >= fTerrainSize && fXRef < 2.0f * fTerrainSize)
		{
			if (GetTerrainPointer(8, &pTerrain))
				return pTerrain->GetShadowMapColor(fXRef - fTerrainSize, fYRef - fTerrainSize);
			return 0xFFFFFFFF;
		}
		return 0xFFFFFFFF;
	}
	return 0xFFFFFFFF;
}

bool CMapOutdoor::isAttrOn(float fX, float fY, uint8_t byAttr)
{
	int32_t iX, iY;
	PR_FLOAT_TO_INT(fX, iX);
	PR_FLOAT_TO_INT(fY, iY);

	return isAttrOn(iX, iY, byAttr);
}

bool CMapOutdoor::GetAttr(float fX, float fY, uint8_t* pbyAttr)
{
	int32_t iX, iY;
	PR_FLOAT_TO_INT(fX, iX);
	PR_FLOAT_TO_INT(fY, iY);

	return GetAttr(iX, iY, pbyAttr);
}

bool CMapOutdoor::isAttrOn(int32_t iX, int32_t iY, uint8_t byAttr)
{
	if (iX < 0 || iY < 0 || iX > m_sTerrainCountX * CTerrainImpl::TERRAIN_XSIZE || iY > m_sTerrainCountY * CTerrainImpl::TERRAIN_YSIZE)
		return false;

	uint16_t wTerrainCoordX, wTerrainCoordY;
	wTerrainCoordX = iX / CTerrainImpl::TERRAIN_XSIZE;
	wTerrainCoordY = iY / CTerrainImpl::TERRAIN_YSIZE;

	uint8_t byTerrainNum;
	if (!GetTerrainNumFromCoord(wTerrainCoordX, wTerrainCoordY, &byTerrainNum))
		return false;
	CTerrain* pTerrain;
	if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		return false;

	uint16_t wLocalX, wLocalY;
	wLocalX = (iX - wTerrainCoordX * CTerrainImpl::TERRAIN_XSIZE) / (CTerrainImpl::HALF_CELLSCALE);
	wLocalY = (iY - wTerrainCoordY * CTerrainImpl::TERRAIN_YSIZE) / (CTerrainImpl::HALF_CELLSCALE);

	return pTerrain->isAttrOn(wLocalX, wLocalY, byAttr);
}

bool CMapOutdoor::GetAttr(int32_t iX, int32_t iY, uint8_t* pbyAttr)
{
	if (iX < 0 || iY < 0 || iX > m_sTerrainCountX * CTerrainImpl::TERRAIN_XSIZE || iY > m_sTerrainCountY * CTerrainImpl::TERRAIN_YSIZE)
		return false;

	uint16_t wTerrainCoordX, wTerrainCoordY;
	wTerrainCoordX = iX / CTerrainImpl::TERRAIN_XSIZE;
	wTerrainCoordY = iY / CTerrainImpl::TERRAIN_YSIZE;

	uint8_t byTerrainNum;
	if (!GetTerrainNumFromCoord(wTerrainCoordX, wTerrainCoordY, &byTerrainNum))
		return false;
	CTerrain* pTerrain;
	if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		return false;

	uint16_t wLocalX, wLocalY;
	wLocalX = (uint16_t)(iX - wTerrainCoordX * CTerrainImpl::TERRAIN_XSIZE) / (CTerrainImpl::HALF_CELLSCALE);
	wLocalY = (uint16_t)(iY - wTerrainCoordY * CTerrainImpl::TERRAIN_YSIZE) / (CTerrainImpl::HALF_CELLSCALE);

	uint8_t byAttr = pTerrain->GetAttr(wLocalX, wLocalY);

	*pbyAttr = byAttr;

	return true;
}

void CMapOutdoor::SetEnvironmentDataName(const std::string& strEnvironmentDataName)
{
	m_envDataName = strEnvironmentDataName;
}

void CMapOutdoor::__XMasTree_Initialize()
{
	m_kXMas.m_pkTree = nullptr;
	m_kXMas.m_iEffectID = -1;
}

void CMapOutdoor::XMasTree_Destroy()
{
	if (m_kXMas.m_pkTree)
	{
		auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
		m_kXMas.m_pkTree->Clear();
		rkForest->DeleteInstance(m_kXMas.m_pkTree);
		m_kXMas.m_pkTree = nullptr;
	}
	if (-1 != m_kXMas.m_iEffectID)
	{
		auto rkEffMgr = CEffectManager::GetInstance();
		rkEffMgr->DestroyEffectInstance(m_kXMas.m_iEffectID);
		m_kXMas.m_iEffectID = -1;
	}
}

void CMapOutdoor::__XMasTree_Create(float x, float y, float z, const char* c_szTreeName, const char* c_szEffName)
{
	assert(nullptr == m_kXMas.m_pkTree);
	assert(-1 == m_kXMas.m_iEffectID);

	auto rkForest = CSpeedTreeForestDirectX9::GetInstance();
	uint32_t dwCRC32 = GetCaseCRC32(c_szTreeName, strlen(c_szTreeName));
	m_kXMas.m_pkTree = rkForest->CreateInstance(x, y, z, dwCRC32, c_szTreeName);

	auto rkEffMgr = CEffectManager::GetInstance();
	rkEffMgr->RegisterEffect(c_szEffName);
	m_kXMas.m_iEffectID = rkEffMgr->CreateEffect(c_szEffName,
		D3DXVECTOR3(x, y, z),
		D3DXVECTOR3(0.0f, 0.0f, 0.0f));
}

void CMapOutdoor::XMasTree_Set(float x, float y, float z, const char* c_szTreeName, const char* c_szEffName)
{
	XMasTree_Destroy();
	__XMasTree_Create(x, y, z, c_szTreeName, c_szEffName);
}

void CMapOutdoor::SpecialEffect_Create(uint32_t dwID, float x, float y, float z, const char* c_szEffName)
{
	auto rkEffMgr = CEffectManager::GetInstance();

	auto itor = m_kMap_dwID_iEffectID.find(dwID);
	if (m_kMap_dwID_iEffectID.end() != itor)
	{
		uint32_t dwEffectID = itor->second;
		if (rkEffMgr->SelectEffectInstance(dwEffectID))
		{
			D3DXMATRIX mat;
			D3DXMatrixIdentity(&mat);
			mat._41 = x;
			mat._42 = y;
			mat._43 = z;
			rkEffMgr->SetEffectInstanceGlobalMatrix(mat);
			return;
		}
	}

	rkEffMgr->RegisterEffect(c_szEffName);
	uint32_t dwEffectID = rkEffMgr->CreateEffect(c_szEffName,
		D3DXVECTOR3(x, y, z),
		D3DXVECTOR3(0.0f, 0.0f, 0.0f));
	m_kMap_dwID_iEffectID.emplace(dwID, dwEffectID);
}

void CMapOutdoor::SpecialEffect_Delete(uint32_t dwID)
{
	auto itor = m_kMap_dwID_iEffectID.find(dwID);

	if (m_kMap_dwID_iEffectID.end() == itor)
		return;

	auto rkEffMgr = CEffectManager::GetInstance();
	int32_t iEffectID = itor->second;
	rkEffMgr->DestroyEffectInstance(iEffectID);
}

void CMapOutdoor::SpecialEffect_Destroy()
{
	auto rkEffMgr = CEffectManager::GetInstance();

	auto itor = m_kMap_dwID_iEffectID.begin();
	for (; itor != m_kMap_dwID_iEffectID.end(); ++itor)
	{
		int32_t iEffectID = itor->second;
		rkEffMgr->DestroyEffectInstance(iEffectID);
	}
}

void CMapOutdoor::ClearGuildArea()
{
	m_rkList_kGuildArea.clear();
}

void CMapOutdoor::RegisterGuildArea(int32_t isx, int32_t isy, int32_t iex, int32_t iey)
{
	RECT rect{};
	rect.left = isx;
	rect.top = isy;
	rect.right = iex;
	rect.bottom = iey;
	m_rkList_kGuildArea.emplace_back(rect);
}

void CMapOutdoor::VisibleMarkedArea()
{
	static const std::size_t kBufferSize = CTerrain::ATTRMAP_XSIZE * CTerrain::ATTRMAP_YSIZE;
	std::unordered_map<int, std::unique_ptr<uint8_t[]>> attrMaps;

	auto itorRect = m_rkList_kGuildArea.begin();
	for (; itorRect != m_rkList_kGuildArea.end(); ++itorRect)
	{
		const RECT& rkRect = *itorRect;

		int32_t ix1Cell;
		int32_t iy1Cell;
		uint8_t byx1SubCell;
		uint8_t byy1SubCell;
		uint16_t wx1TerrainNum;
		uint16_t wy1TerrainNum;

		int32_t ix2Cell;
		int32_t iy2Cell;
		uint8_t byx2SubCell;
		uint8_t byy2SubCell;
		uint16_t wx2TerrainNum;
		uint16_t wy2TerrainNum;

		ConvertToMapCoords(float(rkRect.left), float(rkRect.top), &ix1Cell, &iy1Cell, &byx1SubCell, &byy1SubCell, &wx1TerrainNum, &wy1TerrainNum);
		ConvertToMapCoords(float(rkRect.right), float(rkRect.bottom), &ix2Cell, &iy2Cell, &byx2SubCell, &byy2SubCell, &wx2TerrainNum, &wy2TerrainNum);

		ix1Cell = ix1Cell + wx1TerrainNum * CTerrain::ATTRMAP_XSIZE;
		iy1Cell = iy1Cell + wy1TerrainNum * CTerrain::ATTRMAP_YSIZE;
		ix2Cell = ix2Cell + wx2TerrainNum * CTerrain::ATTRMAP_XSIZE;
		iy2Cell = iy2Cell + wy2TerrainNum * CTerrain::ATTRMAP_YSIZE;

		for (int32_t ixCell = ix1Cell; ixCell <= ix2Cell; ++ixCell)
		{
			for (int32_t iyCell = iy1Cell; iyCell <= iy2Cell; ++iyCell)
			{
				int32_t ixLocalCell = ixCell % CTerrain::ATTRMAP_XSIZE;
				int32_t iyLocalCell = iyCell % CTerrain::ATTRMAP_YSIZE;
				int32_t ixTerrain = ixCell / CTerrain::ATTRMAP_XSIZE;
				int32_t iyTerrain = iyCell / CTerrain::ATTRMAP_YSIZE;
				int32_t iTerrainNum = ixTerrain + iyTerrain * 100;

				uint8_t byTerrainNum;
				if (!GetTerrainNumFromCoord(ixTerrain, iyTerrain, &byTerrainNum))
					continue;
				CTerrain* pTerrain;
				if (!GetTerrainPointer(byTerrainNum, &pTerrain))
					continue;

				if (attrMaps.end() == attrMaps.find(iTerrainNum))
				{
					std::unique_ptr<uint8_t[]> buf(new uint8_t[kBufferSize]);
					std::memset(buf.get(), 0, kBufferSize);
					attrMaps[iTerrainNum] = std::move(buf);
				}

				uint8_t* buf = attrMaps[iTerrainNum].get();
				buf[ixLocalCell + iyLocalCell * CTerrain::ATTRMAP_XSIZE] = 0xff;
			}
		}
	}

	for (const auto& p : attrMaps)
	{
		int32_t ixTerrain = p.first % 100;
		int32_t iyTerrain = p.first / 100;

		uint8_t byTerrainNum;
		if (!GetTerrainNumFromCoord(ixTerrain, iyTerrain, &byTerrainNum))
			continue;

		CTerrain* pTerrain;
		if (!GetTerrainPointer(byTerrainNum, &pTerrain))
			continue;

		pTerrain->AllocateMarkedSplats(p.second.get());
	}
}

void CMapOutdoor::DisableMarkedArea()
{
	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		if (!m_pTerrain[i])
			continue;

		m_pTerrain[i]->DeallocateMarkedSplats();
	}
}

void CMapOutdoor::ConvertToMapCoords(float fx, float fy, int32_t* iCellX, int32_t* iCellY, uint8_t* pucSubCellX, uint8_t* pucSubCellY, uint16_t* pwTerrainNumX, uint16_t* pwTerrainNumY)
{
	if (fy < 0)
		fy = -fy;

	int32_t ix, iy;
	PR_FLOAT_TO_INT(fx, ix);
	PR_FLOAT_TO_INT(fy, iy);

	*pwTerrainNumX = ix / (CTerrainImpl::TERRAIN_XSIZE);
	*pwTerrainNumY = iy / (CTerrainImpl::TERRAIN_YSIZE);

	float maxx = (float)CTerrainImpl::TERRAIN_XSIZE;
	float maxy = (float)CTerrainImpl::TERRAIN_YSIZE;

	while (fx < 0)
		fx += maxx;

	while (fy < 0)
		fy += maxy;

	while (fx >= maxx)
		fx -= maxx;

	while (fy >= maxy)
		fy -= maxy;

	float fooscale = 1.0f / (float)(CTerrainImpl::HALF_CELLSCALE);

	float fCellX, fCellY;

	fCellX = fx * fooscale;
	fCellY = fy * fooscale;

	PR_FLOAT_TO_INT(fCellX, *iCellX);
	PR_FLOAT_TO_INT(fCellY, *iCellY);

	float fRatioooscale = ((float)CTerrainImpl::HEIGHT_TILE_XRATIO) * fooscale;

	float fSubcellX, fSubcellY;
	fSubcellX = fx * fRatioooscale;
	fSubcellY = fy * fRatioooscale;

	PR_FLOAT_TO_INT(fSubcellX, *pucSubCellX);
	PR_FLOAT_TO_INT(fSubcellY, *pucSubCellY);
	*pucSubCellX = (*pucSubCellX) % CTerrainImpl::HEIGHT_TILE_XRATIO;
	*pucSubCellY = (*pucSubCellY) % CTerrainImpl::HEIGHT_TILE_YRATIO;
}
