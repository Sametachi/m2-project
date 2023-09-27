#include "StdAfx.h"
#include "SceneMap.h"

// Test Code
#include "../WorldEditor.h"
#include "../WorldEditorDoc.h"
#include "../WorldEditorView.h"
#include "../MainFrm.h"
#include "../DataCtrl/ShadowRenderHelper.h"
#include "../DataCtrl/MapAccessorOutdoor.h"
#include "../../../Client/eterlib/Camera.h"
#include "../DataCtrl/MapAccessorTerrain.h"
#include "../DataCtrl/NonPlayerCharacterInfo.h"
#include "../../../Client/milesLib/SoundManager.h"
#include "../DataCtrl/MiniMapRenderHelper.h"

#include "../Config/Globals.h"
struct FGetPortalID
{
	float m_fRequestX, m_fRequestY;
	std::set<int32_t> m_kSet_iPortalID;
	FGetPortalID(float fRequestX, float fRequestY)
	{
		m_fRequestX=fRequestX;
		m_fRequestY=fRequestY;
	}
	void operator () (CGraphicObjectInstance * pObject)
	{
		for (int32_t i = 0; i < PORTAL_ID_MAX_NUM; ++i)
		{
			int32_t iID = pObject->GetPortal(i);
			if (0 == iID)
				break;

			m_kSet_iPortalID.insert(iID);
		}
	}
};

void CSceneMap::OnUpdate()
{
	if (globals::dft::DETECT_MDATR_HEIGHT)
	{
		CCullingManager::Instance().Process();
		CCullingManager::Instance().Update();
	}

	CEffectManager::Instance().Update();
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	if (m_pMapManagerAccessor->IsAutoSave())
	{
		if (m_pMapManagerAccessor->GetTimeSave() <= time(0))
		{
			m_pMapManagerAccessor->SetNextTimeSave();
			m_pMapManagerAccessor->SaveMap();
		}
	}

	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	m_CursorRenderer.SetCenterPosition(pv3Position);
	m_CursorRenderer.SetCursorPosition(m_vecMouseMapIntersectPosition);

	m_pMapManagerAccessor->UpdateEditing();
 	m_pMapManagerAccessor->UpdateMap(pv3Position.x, pv3Position.y, pv3Position.z);
	m_pMapManagerAccessor->UpdateAroundAmbience(pv3Position.x, -pv3Position.y, pv3Position.z);

	const TOutdoorMapCoordinate & c_rCurCoordinate = m_pMapAccessor->GetCurCoordinate();
	if (c_rCurCoordinate.m_sTerrainCoordX != m_kPrevCoordinate.m_sTerrainCoordX ||
		c_rCurCoordinate.m_sTerrainCoordY != m_kPrevCoordinate.m_sTerrainCoordY)
	{
		m_pMapAccessor->VisibleMarkedArea();
		m_kPrevCoordinate = c_rCurCoordinate;
	}

	CCamera * pMainCamera = CCameraManager::Instance().GetCurrentCamera();
	const D3DXVECTOR3 & c_rv3CameraDirection = pMainCamera->GetView();
	const D3DXVECTOR3 & c_rv3CameraUp = pMainCamera->GetUp();
	CSoundManager::Instance().SetPosition(pv3Position.x, pv3Position.y, pv3Position.z);
	CSoundManager::Instance().SetDirection(c_rv3CameraDirection.x, c_rv3CameraDirection.y, c_rv3CameraDirection.z, c_rv3CameraUp.x, c_rv3CameraUp.y, c_rv3CameraUp.z);
	CSoundManager::Instance().Update();

	// TerrainDecal
	{
		static float s_fWidth = 500.0f;
		static float s_fHeight = 500.0f;

		D3DXVECTOR3 v3Tangent(0.0f, 1.0f, 0.0f);

		m_aConpasTerrainDecal.SetMapOutdoor(&m_pMapManagerAccessor->GetMapOutdoorRef());
		m_aConpasTerrainDecal.Make(D3DXVECTOR3(pv3Position.x, pv3Position.y, m_pMapManagerAccessor->GetHeight(pv3Position.x, fabs(pv3Position.y)) + 0.25f), D3DXVECTOR3(0.0f, 0.0f, 1.0f), v3Tangent, s_fWidth, s_fHeight, 1000.0f);
	}
/*
	{
		CCullingManager & rkCullingMgr = CCullingManager::Instance();
		FGetPortalID kGetPortalID(pv3Position.x, pv3Position.y);

		Vector3d aVector3d;
		aVector3d.Set(pv3Position.x, pv3Position.y, 0.0f);

		Vector3d toTop;
		toTop.Set(0, 0, 25000.0f);

		rkCullingMgr.ForInRay(aVector3d, toTop, &kGetPortalID);

		std::set<int32_t>::iterator itor = kGetPortalID.m_kSet_iPortalID.begin();
		for (; itor != kGetPortalID.m_kSet_iPortalID.end(); ++itor)
		{
			int32_t iID = *itor;
		}
	}

*/
}

void CSceneMap::OnRender(BOOL bClear)
{
	if (m_bTerrainShadowMapUpdateNeeded)
	{
		// CCameraManager::Instance().GetCurrentCamera()->SetEye(D3DXVECTOR3(2 * CTerrainImpl::TERRAIN_XSIZE, -CTerrainImpl::TERRAIN_YSIZE * 2, 100000));
		// CCameraManager::Instance().GetCurrentCamera()->SetTarget(D3DXVECTOR3(2 * CTerrainImpl::TERRAIN_XSIZE, -CTerrainImpl::TERRAIN_YSIZE * 2, 20000));

		CTerrainAccessor * pTerrainAccessor;
		if (!m_pMapAccessor->GetTerrainPointer(4, (CTerrain **) &pTerrainAccessor))
			return;


		int16_t wX, wY;
		m_pMapAccessor->GetTerrainCount(&wX, &wY);

		// uint16_t wOx, wOy;
		// pTerrainAccessor->GetCoordinate(&wOx, &wOy);
#ifdef USE_WE_CONFIG
		D3DXVECTOR3 v3GoBack = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
#endif
		for(int16_t x = 0; x < wX; x++)
		{
			for(int16_t y = 0; y < wY; y++)
			{
				// CCameraManager::Instance().GetCurrentCamera()->SetEye(D3DXVECTOR3(x * CTerrainImpl::TERRAIN_XSIZE + 1, -CTerrainImpl::TERRAIN_YSIZE * y - 1, 100000));
				// CCameraManager::Instance().GetCurrentCamera()->SetTarget(D3DXVECTOR3(x * CTerrainImpl::TERRAIN_XSIZE + 1, -CTerrainImpl::TERRAIN_YSIZE * y - 1, 20000));
				CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
				CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();
				D3DXVECTOR3 v3Target = CCameraManager::Instance().GetCurrentCamera()->GetTarget();

				pView->UpdateTargetPosition(- v3Target.x + (float)(x * CTerrainImpl::TERRAIN_XSIZE), - v3Target.y + - (float)(y * CTerrainImpl::TERRAIN_YSIZE));
				D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();
				m_pMapManagerAccessor->SetTerrainModified();
				m_pMapManagerAccessor->UpdateMap(pv3Position.x, pv3Position.y, 0.0f); //pv3Position.z

				m_pMapManagerAccessor->UpdateTerrainShadowMap();
				m_pMapManagerAccessor->ReloadTerrainShadowTexture();
				m_pMapManagerAccessor->SaveMiniMap();
#ifdef USE_WE_CONFIG
				if (!globals::dft::NOMAI_ATLAS_DUMP)
#endif
				SaveMiniMapWithMonsterAreaInfo();
			}
		}
#ifdef USE_WE_CONFIG
		if (globals::dft::NO_GOTO_AFTER_INSERT)
		{
			CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
			CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();
			D3DXVECTOR3 v3WhereIAm = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
			MovePosition(-v3WhereIAm.x +v3GoBack.x, -v3WhereIAm.y +v3GoBack.y);
		}
#endif
		//pTerrainAccessor->SetCoordinate(wOx, wOy);

		m_bTerrainShadowMapUpdateNeeded = false;
	}

	if(bClear)
	{
		CScreen::SetClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b);
		CScreen::Clear();
	}

	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	if (m_bShadowRenderingOn)
	{
		m_pMapAccessor->BeginRenderCharacterShadowToTexture();

		if (m_bCharacterRenderingOn)
			OnRenderCharacter();

		m_pMapAccessor->EndRenderCharacterShadowToTexture();
	}


	///////////////////////////////////////////////////////////////
	// Render Map
	m_pMapAccessor->RenderSky();
	m_pMapAccessor->RenderBeforeLensFlare();
	m_pMapAccessor->RenderCloud();

	m_pMapManagerAccessor->RefreshEnvironmentData();
	m_pMapManagerAccessor->BeginEnvironment();
	{
		m_pMapAccessor->SetInverseViewAndDynamicShaodwMatrices();

		SetDiffuseOperation();
		if (m_bObjectRenderingOn)
		{
			m_pMapAccessor->RenderArea();
			CSpeedTreeForestDirectX8::Instance().Render();
		}

		if (m_bTerrainRenderingOn)
   			m_pMapAccessor->RenderTerrain();

		if (m_bGuildAreaRenderingOn)
			m_pMapAccessor->RenderMarkedArea();

		if (m_bCharacterRenderingOn)
			OnRenderCharacter();

		if (m_bWaterRenderingOn)
			m_pMapAccessor->RenderWater();

		if (m_bObjectRenderingOn)
		{
			// Start Dungeon Rendering - Without Environment
			SetDiffuseOperation();
			STATEMANAGER.SetTextureStageState(1, D3DTSS_COLORARG1,	D3DTA_TEXTURE);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_COLORARG2,	D3DTA_CURRENT);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_MODULATE);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAARG2,	D3DTA_CURRENT);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_MODULATE);
			m_pMapAccessor->RenderDungeon();
			STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_DISABLE);
			STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);
			STATEMANAGER.SetTexture(0, NULL);
			STATEMANAGER.SetTexture(1, NULL);
			// End Dungeon Rendering - Without Environment
		}

		m_pMapAccessor->RenderEffect();
	}

	m_pMapManagerAccessor->EndEnvironment();

	if (EDITING_MODE_ATTRIBUTE == m_iEditingMode)
		OnRenderSceneAttribute();

	if (m_bObjectCollisionRenderingOn)
		OnRenderObjectCollision();

	m_pMapAccessor->RenderAfterLensFlare();
	m_pMapAccessor->RenderScreenFiltering();

	CEffectManager::Instance().Render();

	//////////////////////////////////////
	switch (m_iEditingMode)
	{
		case EDITING_MODE_TERRAIN:
		case EDITING_MODE_ATTRIBUTE:
			OnRenderTerrainEditingArea();
			break;

		case EDITING_MODE_OBJECT:
			OnRenderSelectedObject();
			OnRenderObjectSettingArea();
			break;
	}

	SetColorOperation();
	OnRenderCenterCursor();
	RenderSelectedObject();

	static bool s_bSnow = FALSE;
	if (!s_bSnow)
	{
		m_kSnowEnvironment.Create();
		s_bSnow = TRUE;
	}

	m_kSnowEnvironment.Update(ms_Camera->GetTarget());

	if (m_bCompassOn)
		OnRenderCompass();

	if (m_bMeterGridOn)
		OnRenderMeterGrid();

	if (m_bMapBoundGridOn)
		OnRenderMapBoundGrid();

	if (m_bPatchGridOn)
		OnRenderPatchGrid();
	//////////////////////////////////////

	CEffectInstance::ResetRenderingEffectCount();
}

void CSceneMap::OnRenderUI(float fx, float fy)
{
	int32_t iRenderedPatchNum, iRenderedSplatNum;
	float fSplatRatio;
	std::vector<int32_t> & aVector = m_pMapManagerAccessor->GetRenderedSplatNum(&iRenderedPatchNum, &iRenderedSplatNum, &fSplatRatio);

	std::ostringstream ostr;
	std::copy(aVector.begin(),aVector.end(),std::ostream_iterator<int32_t>(ostr," "));

	char szMsg[128+1];

	_snprintf(szMsg, 128, "Texture Count: %d", iRenderedSplatNum);
	m_textInstanceSplatTextureCount.SetValue(szMsg, strlen(szMsg));
	_snprintf(szMsg, 128, "Splat Count: %d", iRenderedPatchNum);
	m_textInstanceSplatMeshCount.SetValue(szMsg, strlen(szMsg));
	_snprintf(szMsg, 128, "Texture Mesh Ratio: %f", fSplatRatio);
	m_textInstanceSplatMeshPercentage.SetValue(szMsg, strlen(szMsg));

	D3DXVECTOR3 v3Target = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
	uint32_t dwCoordX, dwCoordY, wTerrainCoordX, wTerrainCoordY;;
	PR_FLOAT_TO_INT(v3Target.x, dwCoordX);
	PR_FLOAT_TO_INT(-v3Target.y, dwCoordY);

	wTerrainCoordX = dwCoordX / CTerrainImpl::TERRAIN_XSIZE;
	wTerrainCoordY = dwCoordY / CTerrainImpl::TERRAIN_YSIZE;

	uint8_t byTerrainNum;
	m_pMapAccessor->GetTerrainNumFromCoord(wTerrainCoordX, wTerrainCoordY, &byTerrainNum);
	CTerrain * pTerrain;
	m_pMapAccessor->GetTerrainPointer(byTerrainNum, &pTerrain);

	if (!pTerrain)
		return;

	TTerrainSplatPatch & rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();

	dwCoordX -= wTerrainCoordX * CTerrainImpl::TERRAIN_XSIZE;
	dwCoordY -= wTerrainCoordY * CTerrainImpl::TERRAIN_YSIZE;

	dwCoordX = dwCoordX/(CTerrainImpl::PATCH_XSIZE * CTerrainImpl::CELLSCALE);
	dwCoordY = dwCoordY/(CTerrainImpl::PATCH_YSIZE * CTerrainImpl::CELLSCALE);

	uint16_t wPatchNum = dwCoordY * CTerrainImpl::PATCH_XCOUNT + dwCoordX;

	std::ostringstream aStream;
	std::vector<std::pair<uint32_t, int32_t> > aTilecountVector;

	for (uint8_t byTextureIndex = 0; byTextureIndex <= pTerrain->GetTextureSet()->GetTextureCount(); ++byTextureIndex)
	{
		uint32_t dwTileCount = rTerrainSplatPatch.PatchTileCount[wPatchNum][byTextureIndex];
		if (dwTileCount > 0)
			aTilecountVector.push_back(std::vector<std::pair<uint32_t, uint8_t> >::value_type(dwTileCount, (int32_t)byTextureIndex));
	}
	std::sort(aTilecountVector.begin(), aTilecountVector.end());

	aStream << "( Full " << aTilecountVector.size() << " Field ) : ";
	std::vector<std::pair<uint32_t, int32_t> >::reverse_iterator aIterator = aTilecountVector.rbegin();
	while(aTilecountVector.rend() != aIterator)
	{
		std::pair<uint32_t, int32_t> aPair = *aIterator;

		aStream << aPair.second <<"("<< aPair.first << "), ";
		++aIterator;
	}

	// _snprintf(szMsg, 128, "Current Patch Texture usage: %s", aStream.str().c_str());
	// m_textInstancePatchSplatTileCount.SetValue(szMsg, strlen(szMsg));
	// @fixme105
	m_textInstancePatchSplatTileCount.SetValueString(std::string("Current Patch Texture usage: ") + aStream.str());

	std::ostringstream aStream2, aStream3;

	for (uint32_t dwPatchCoordY = 0; dwPatchCoordY < CTerrainImpl::PATCH_YCOUNT; ++dwPatchCoordY)
	{
		for (uint32_t dwPatchCoordX = 0; dwPatchCoordX < CTerrainImpl::PATCH_XCOUNT; ++dwPatchCoordX)
		{
			uint16_t wPatchIndex = dwPatchCoordY * CTerrainImpl::PATCH_XCOUNT + dwPatchCoordX;
			if (rTerrainSplatPatch.PatchTileCount[wPatchIndex][0] > 0)
				aStream2 << " : ( " <<
				dwPatchCoordX * CTerrainImpl::PATCH_XSIZE * 2 + wTerrainCoordX * CTerrainImpl::XSIZE * 2<< ", " <<
				dwPatchCoordY * CTerrainImpl::PATCH_YSIZE * 2 + wTerrainCoordY * CTerrainImpl::YSIZE * 2<< " )";
			for (uint32_t dwi = 1; dwi < pTerrain->GetTextureSet()->GetTextureCount(); ++dwi)
			{
				uint32_t dwCount = rTerrainSplatPatch.PatchTileCount[wPatchIndex][dwi];
				if ( dwCount > 0 && dwCount < 51)
					aStream3 << " : ( " <<
					dwPatchCoordX * CTerrainImpl::PATCH_XSIZE * 2 + wTerrainCoordX * CTerrainImpl::XSIZE * 2<< ", " <<
					dwPatchCoordY * CTerrainImpl::PATCH_YSIZE * 2 + wTerrainCoordY * CTerrainImpl::YSIZE * 2<< " )";
			}
		}
	}


	CScreen::SetDiffuseColor(0.0f, 0.0f, 0.0f, 0.3f);
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	CScreen::RenderBar2d(fx, fy, fx + 500.0f, fy + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_ELEMENTS_END));
#else
	CScreen::RenderBar2d(fx, fy, fx + 500.0f, fy + 140.0f);
#endif

#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceSplatTextureCount.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_TEXTURE_COUNT));
#else
	m_textInstanceSplatTextureCount.SetPosition(fx + 10.0f, fy + 10.0f);
#endif
	m_textInstanceSplatTextureCount.Update();
	m_textInstanceSplatTextureCount.Render();
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceSplatMeshCount.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_SPLAT_COUNT));
#else
	m_textInstanceSplatMeshCount.SetPosition(fx + 10.0f, fy + 30.0f);
#endif
	m_textInstanceSplatMeshCount.Update();
	m_textInstanceSplatMeshCount.Render();
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceSplatMeshPercentage.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_TEXTURE_MESH_RATIO));
#else
	m_textInstanceSplatMeshPercentage.SetPosition(fx + 10.0f, fy + 50.0f);
#endif
	m_textInstanceSplatMeshPercentage.Update();
	m_textInstanceSplatMeshPercentage.Render();
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstancePatchSplatTileCount.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_CURRENT_PATCH_TEXTURE_USAGE));
#else
	m_textInstancePatchSplatTileCount.SetPosition(fx + 10.0f, fy + 70.0f);
#endif
	m_textInstancePatchSplatTileCount.Update();
	m_textInstancePatchSplatTileCount.Render();

	_snprintf(szMsg, 128, "0 Patch Location %s", aStream2.str().c_str());
	m_textInstanceTexture0Count.SetValue(szMsg, strlen(szMsg));
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceTexture0Count.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_PATCH_LOCATION));
#else
	m_textInstanceTexture0Count.SetPosition(fx + 10.0f, fy + 90.0f);
#endif
	m_textInstanceTexture0Count.Update();
	m_textInstanceTexture0Count.Render();

	// _snprintf(szMsg, 128, "Texture patch location less than 51 it is: %s", aStream3.str().c_str());
	// _snprintf(szMsg, 128, "Texture patch loc. < 51: %s", aStream3.str().c_str());
	// m_textInstanceTexture0Count.SetValue(szMsg, strlen(szMsg));
	// @fixme105
	m_textInstanceTexture0Count.SetValueString(std::string("Texture patch loc. < 51: ") + aStream3.str());
#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceTexture0Count.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_TEXTURE_PATCH_LOCATION));
#else
	m_textInstanceTexture0Count.SetPosition(fx + 10.0f, fy + 110.0f);
#endif
	m_textInstanceTexture0Count.Update();
	m_textInstanceTexture0Count.Render();

#ifdef CWE_INFO_BOARD_POSITION
	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();
	_snprintf(szMsg, 128, "Local Position: %.0f, %.0f", fabsf(pv3Position.x / 100), fabsf(pv3Position.y / 100));
	m_textInstanceLocalPosition.SetValue(szMsg, strlen(szMsg));
	#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceLocalPosition.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_LOCAL_POSITION));
	#else
	m_textInstanceLocalPosition.SetPosition(fx + 10.0f, fy + 190.0f);
	#endif
	m_textInstanceLocalPosition.Update();
	m_textInstanceLocalPosition.Render();

	uint32_t dwBaseX, dwBaseY;
	CMapOutdoor& rMapOutdoor = m_pMapManagerAccessor->GetMapOutdoorRef();
	rMapOutdoor.GetBaseXY(&dwBaseX, &dwBaseY);
	_snprintf(szMsg, 128, "Global Position: %.0f, %.0f", fabsf(pv3Position.x / 100) + dwBaseX, fabsf(pv3Position.y / 100) + dwBaseY);
	m_textInstanceGlobalPosition.SetValue(szMsg, strlen(szMsg));
	#ifdef CWE_AUTO_INFO_BOARD_SIZE
	m_textInstanceGlobalPosition.SetPosition(fx + 10.0f, fy + 10.0f + (CSceneMap::fInfoBoard * CSceneMap::Information_Board::INFORMATION_BOARD_GLOBAL_POSITION));
	#else
	m_textInstanceGlobalPosition.SetPosition(fx + 10.0f, fy + 220.0f);
	#endif
	m_textInstanceGlobalPosition.Update();
	m_textInstanceGlobalPosition.Render();
#endif
}

void CSceneMap::OnRenderLightDirection()
{
	const TEnvironmentData * c_pEnvironmentData;
	m_pMapManagerAccessor->GetEnvironmentData(&c_pEnvironmentData);

	SetDiffuseColor(1.0f, 0.0f, 0.0f);
	D3DXVECTOR3 v3Target = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
	RenderLine3d(v3Target.x, v3Target.y, v3Target.z,
		v3Target.x - c_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.x * 100000.0f,
		v3Target.y - c_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.y * 100000.0f,
		v3Target.z - c_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.z * 100000.0f);
}

void CSceneMap::OnRenderMonsterAreaInfo(CMonsterAreaInfo * pMonsterAreaInfo)
{
	int32_t lOriginX, lOriginY;
	pMonsterAreaInfo->GetOrigin(&lOriginX, &lOriginY);
	float fOriginX, fOriginY;
	fOriginX = (float)lOriginX * 100.0f;
	fOriginY = (float)lOriginY * 100.0f;

	float fLeft = (float)pMonsterAreaInfo->GetLeft() * 100.0f;
	float fRight = (float)pMonsterAreaInfo->GetRight() * 100.0f;
	float fTop = -(float)pMonsterAreaInfo->GetTop() * 100.0f;
	float fBottom = -(float)pMonsterAreaInfo->GetBottom() * 100.0f;

	float fHeightScale;
	fHeightScale = m_pMapAccessor->GetHeightScale();

	D3DXVECTOR3 v3Origin(fOriginX, -fOriginY, m_pMapManagerAccessor->GetHeight(fOriginX, fOriginY) + 10.0f);

	RenderLine3d(v3Origin.x, v3Origin.y, v3Origin.z - 100.0f, v3Origin.x, v3Origin.y, v3Origin.z + 100.0f);
	RenderLine3d(v3Origin.x - 100.0f, v3Origin.y, v3Origin.z, v3Origin.x + 100.0f, v3Origin.y, v3Origin.z);
	RenderLine3d(v3Origin.x, v3Origin.y - 100.0f, v3Origin.z, v3Origin.x, v3Origin.y + 100.0f, v3Origin.z);

	float fx, fy, fcurz, fnextz;

	float fGridWidth = (float)CTerrainImpl::CELLSCALE;

	for (fy = fBottom; fy < fTop; fy += fGridWidth)
	{
		fcurz = m_pMapAccessor->GetHeight(fLeft, fy) + 10.0f;
		fnextz = m_pMapAccessor->GetHeight(fLeft, fy + fGridWidth) + 10.0f;
		RenderLine3d(fLeft, fy, fcurz, fLeft, fy + fGridWidth, fnextz);
		fcurz = m_pMapAccessor->GetHeight(fRight, fy) + 10.0f;
		fnextz = m_pMapAccessor->GetHeight(fRight, fy + fGridWidth) + 10.0f;
		RenderLine3d(fRight, fy, fcurz, fRight, fy + fGridWidth, fnextz);
	}
	for (fx = fLeft; fx < fRight; fx += fGridWidth)
	{
		fcurz = m_pMapAccessor->GetHeight(fx, fTop) + 10.0f;
		fnextz = m_pMapAccessor->GetHeight(fx + fGridWidth, fTop) + 10.0f;
		RenderLine3d(fx, fTop, fcurz, fx + fGridWidth, fTop, fnextz);
		fcurz = m_pMapAccessor->GetHeight(fx, fBottom) + 10.0f;
		fnextz = m_pMapAccessor->GetHeight(fx + fGridWidth, fBottom) + 10.0f;
		RenderLine3d(fx, fBottom, fcurz, fx + fGridWidth, fBottom, fnextz);
	}

	CNonPlayerCharacterInfo & rNPCInfo = CNonPlayerCharacterInfo::Instance();
	std::string strMonsterName = "Noname";
	if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_MONSTER == pMonsterAreaInfo->GetMonsterAreaInfoType())
	{
		strMonsterName = pMonsterAreaInfo->GetMonsterName();
		if (0 == strMonsterName.compare("Noname"))
		{
			uint32_t dwMonsterVID = pMonsterAreaInfo->GetMonsterVID();
			if (dwMonsterVID != 0)
			{
				strMonsterName = rNPCInfo.GetNameByVID(dwMonsterVID);
				if (0 != strMonsterName.compare("Noname"))
					pMonsterAreaInfo->SetMonsterName(strMonsterName);
			}
		}
	}
	else if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_GROUP == pMonsterAreaInfo->GetMonsterAreaInfoType())
	{
		char szFollowerCount[32+1];
		_snprintf(szFollowerCount, 32, "%d", pMonsterAreaInfo->GetMonsterGroupFollowerCount());
		strMonsterName = pMonsterAreaInfo->GetMonsterGroupName() +
			"(Leader:" +
			pMonsterAreaInfo->GetMonsterGroupLeaderName() +
			", Follower:" +
			szFollowerCount +
			")";
	}
	uint32_t dwMonsterCount = pMonsterAreaInfo->GetMonsterCount();
	D3DXVECTOR2 v2Direction = pMonsterAreaInfo->GetMonsterDirVector();

	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);

	std::vector<D3DXVECTOR2> aMonsterNameRenderPositionVector;
	for (uint32_t dwCount = 0; dwCount < dwMonsterCount; ++dwCount)
	{
		D3DXVECTOR2 v2Position = pMonsterAreaInfo->GetTempMonsterPos(dwCount) * 100.0f;

		RenderLine3d(v2Position.x, -v2Position.y, m_pMapAccessor->GetHeight(v2Position.x, v2Position.y),
			v2Position.x, -v2Position.y, m_pMapAccessor->GetHeight(v2Position.x, v2Position.y) + 100.0f);
		RenderLine3d(v2Position.x,
			-v2Position.y,
			m_pMapAccessor->GetHeight(v2Position.x, v2Position.y) + 10.0f,
			v2Position.x + v2Direction.x * 50.0f,
			-v2Position.y + v2Direction.y * 50.0f,
			m_pMapAccessor->GetHeight(v2Position.x + v2Direction.x * 50.0f, v2Position.y - v2Direction.y * 50.0f) + 10.0f);
		D3DXVECTOR2 v2NameRenderPos;
		ProjectPosition(v2Position.x, -v2Position.y, m_pMapAccessor->GetHeight(v2Position.x, v2Position.y) + 100.0f, &v2NameRenderPos.x, &v2NameRenderPos.y);
		aMonsterNameRenderPositionVector.push_back(v2NameRenderPos);
	}

	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();

	CRect Rect;
	pView->GetClientRect(&Rect);

	STATEMANAGER.SaveTransform(D3DTS_VIEW, &matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_WORLD, &matIdentity);
	SetOrtho2D(Rect.Width(), Rect.Height(), 400.0f);
	char szMsg[128+1];

	_snprintf(szMsg, 128, "%s", strMonsterName.c_str());
	m_textInstanceMonsterInfo.SetValue(szMsg, strlen(szMsg));
	for (uint32_t dwMonCount = 0; dwMonCount < dwMonsterCount; ++dwMonCount)
	{
		D3DXVECTOR2 v2NameRenderPos = aMonsterNameRenderPositionVector[dwMonCount];
		m_textInstanceMonsterInfo.SetPosition(v2NameRenderPos.x, v2NameRenderPos.y);
		m_textInstanceMonsterInfo.Update();
		m_textInstanceMonsterInfo.Render();

	}
	SetPerspective(ms_fFieldOfView, ms_fAspect, ms_fNearY, ms_fFarY);
	STATEMANAGER.RestoreTransform(D3DTS_VIEW);
	STATEMANAGER.RestoreTransform(D3DTS_WORLD);
}

void CSceneMap::OnRenderSceneAttribute()
{
	STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	STATEMANAGER.SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_pMapManagerAccessor->RenderAttr();
	STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER.RestoreRenderState(D3DRS_SRCBLEND);
	STATEMANAGER.RestoreRenderState(D3DRS_DESTBLEND);

	SetColorOperation();
	SetDiffuseColor(1.0f, 1.0f, 1.0f);
	CMonsterAreaInfo * pMonsterAreaInfo;
	if (m_pMapManagerAccessor->ShowAllMonsterAreaInfo())
	{
		uint32_t dwMonsterAreaCount = m_pMapManagerAccessor->GetMonsterAreaInfoCount();
		for (uint32_t dwIndex = 0; dwIndex < dwMonsterAreaCount; ++dwIndex)
		{
			if (!m_pMapManagerAccessor->GetMonsterAreaInfoFromVectorIndex(dwIndex, &pMonsterAreaInfo))
				continue;
			if (pMonsterAreaInfo == m_pMapManagerAccessor->GetSelectedMonsterAreaInfo())
				continue;
			OnRenderMonsterAreaInfo(pMonsterAreaInfo);
		}
	}
	if (m_pMapManagerAccessor->GetSelectedMonsterAreaInfo())
	{
		SetDiffuseColor(1.0f, 0.0f, 1.0f);
		m_textInstanceMonsterInfo.SetColor(1.0f, 0.0f, 1.0f);
		OnRenderMonsterAreaInfo(m_pMapManagerAccessor->GetSelectedMonsterAreaInfo());
		SetDiffuseColor(1.0f, 1.0f, 1.0f);
		m_textInstanceMonsterInfo.SetColor(1.0f, 1.0f, 1.0f);
	}
}

void CSceneMap::OnRenderEnvironmentMap()
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	// Save transformation matrices of the device.
	D3DXMATRIX matProj, matView;
	STATEMANAGER.SaveTransform(D3DTS_VIEW, &matView);
	STATEMANAGER.SaveTransform(D3DTS_PROJECTION, &matProj);

	// Store the current back buffer and z-buffer.
	LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
	ms_lpd3dDevice->GetRenderTarget( &pBackBuffer );
	ms_lpd3dDevice->GetDepthStencilSurface( &pZBuffer );

	// Use 90-degree field of view in the projection.
	D3DXMatrixPerspectiveFovRH( &matProj, D3DX_PI/2, 1.0f, 0.0f, 10000.0f );
	STATEMANAGER.SetTransform( D3DTS_PROJECTION, &matProj );

	// Loop through the six faces of the cube map.
	for (uint32_t i = 0; i < 6; i++)
	{
		// Standard view that will be overridden below.
		D3DXVECTOR3 vEnvEyePt = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
		D3DXVECTOR3 vLookatPt, vUpVec;

		switch (i)
		{
			case D3DCUBEMAP_FACE_POSITIVE_X:
				vLookatPt = D3DXVECTOR3( 1.0f, 0.0f, 0.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
				break;
			case D3DCUBEMAP_FACE_NEGATIVE_X:
				vLookatPt = D3DXVECTOR3( -1.0f, 0.0f, 0.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
				break;
			case D3DCUBEMAP_FACE_POSITIVE_Y:
				vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 1.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, -1.0f, 0.0f );
				break;
			case D3DCUBEMAP_FACE_NEGATIVE_Y:
				vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, -1.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
				break;
			case D3DCUBEMAP_FACE_POSITIVE_Z:
				vLookatPt = D3DXVECTOR3( 0.0f, 1.0f, 0.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
				break;
			case D3DCUBEMAP_FACE_NEGATIVE_Z:
				vLookatPt = D3DXVECTOR3( 0.0f, -1.0f, 0.0f) + vEnvEyePt;
				vUpVec    = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
				break;
		}

		D3DXMatrixLookAtRH( &matView, &vEnvEyePt, &vLookatPt, &vUpVec );
		STATEMANAGER.SetTransform(D3DTS_VIEW, &matView);

		//Get pointer to surface in order to render to it.
		LPDIRECT3DSURFACE8 pFace = NULL;
		ms_lpd3dDevice->SetRenderTarget(pFace, pZBuffer);

		if (FAILED(ms_lpd3dDevice->Clear(0L,
										 NULL,
										 D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0)))
		{
		}

		pFace->Release();

		ms_lpd3dDevice->BeginScene();

		//////////////////////////////////////////////////////////////////////////

		m_pMapAccessor->RenderSky();
		m_pMapAccessor->RenderBeforeLensFlare();
		m_pMapAccessor->RenderArea();
		m_pMapAccessor->RenderTerrain();

		if (m_bCharacterRenderingOn)
			OnRenderCharacter();

		m_pMapAccessor->RenderAfterLensFlare();
		//////////////////////////////////////////////////////////////////////////

		ms_lpd3dDevice->EndScene();
	}

	ms_lpd3dDevice->SetRenderTarget( pBackBuffer, pZBuffer );
	pBackBuffer->Release();
	pZBuffer->Release();

	// Restore the original transformation matrices.
	STATEMANAGER.RestoreTransform(D3DTS_VIEW);
	STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);
}

void CSceneMap::OnRenderMeterGrid()
{
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	SetDiffuseOperation();
	SetDiffuseColor(1.0f, 1.0f, 1.0f);

	STATEMANAGER.SetRenderState(D3DRS_FOGENABLE, FALSE);
	STATEMANAGER.SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1,	D3DTA_DIFFUSE);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_SELECTARG1);
	STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);
	STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_DISABLE);
	STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);

	int32_t viewradius;
	float fHeightScale;

	viewradius			= m_pMapAccessor->GetViewRadius();
	fHeightScale		= m_pMapAccessor->GetHeightScale();

	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	float GridInterval = 1000;

	int32_t GridMinX = (((int32_t)(pv3Position.x) / CTerrainImpl::CELLSCALE) - viewradius / 2) * CTerrainImpl::CELLSCALE / GridInterval;
	int32_t GridMinY = (((int32_t)(-pv3Position.y) / CTerrainImpl::CELLSCALE) - viewradius / 2) * CTerrainImpl::CELLSCALE / GridInterval;
	int32_t GridMaxX = (((int32_t)(pv3Position.x) / CTerrainImpl::CELLSCALE) + viewradius / 2) * CTerrainImpl::CELLSCALE / GridInterval;
	int32_t GridMaxY = (((int32_t)(-pv3Position.y) / CTerrainImpl::CELLSCALE) + viewradius / 2) * CTerrainImpl::CELLSCALE / GridInterval;

	float fx, fy, fcurz, fnextz;
	int32_t i, j;
	for (i = GridMinX; i <= GridMaxX; ++i)
	{
		fx = (float)i * GridInterval;
		for (j = GridMinY; j < GridMaxY; ++j)
		{
			fy = (float)j * GridInterval;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx, fy + GridInterval) + 10.0f;
			if (0 == i%10)
			{
				SetDiffuseColor(1.0f, 1.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx, -fy - GridInterval, fnextz);
				SetDiffuseColor(1.0f, 1.0f, 1.0f);
			}
			else if (0 == i%5)
			{
				SetDiffuseColor(1.0f, 0.0f, 1.0f);
				RenderLine3d(fx, -fy, fcurz, fx, -fy - GridInterval, fnextz);
				SetDiffuseColor(1.0f, 1.0f, 1.0f);
			}
			else
			{
				RenderLine3d(fx, -fy, fcurz, fx, -fy - GridInterval, fnextz);
			}
		}
	}

	for (j = GridMinY; j <= GridMaxY; ++j)
	{
		fy = (float) j * GridInterval;

		for (i = GridMinX; i < GridMaxX; ++i)
		{
			fx = (float)i * GridInterval;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx + GridInterval, fy) + 10.0f;
			if (0 == j%10)
			{
				SetDiffuseColor(1.0f, 1.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx + GridInterval, -fy, fnextz);
				SetDiffuseColor(1.0f, 1.0f, 1.0f);
			}
			else if (0 == j%5)
			{
				SetDiffuseColor(1.0f, 0.0f, 1.0f);
				RenderLine3d(fx, -fy, fcurz, fx + GridInterval, -fy, fnextz);
				SetDiffuseColor(1.0f, 1.0f, 1.0f);
			}
			else
			{
				RenderLine3d(fx, -fy, fcurz, fx + GridInterval, -fy, fnextz);
			}
		}
	}
}

void CSceneMap::OnRenderPatchGrid()
{
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	SetDiffuseOperation();
	SetDiffuseColor(1.0f, 0.0f, 1.0f);

	int32_t viewradius;
	float fHeightScale;
	viewradius			= m_pMapAccessor->GetViewRadius();
	fHeightScale		= m_pMapAccessor->GetHeightScale();

	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	float fx, fy, fcurz, fnextz;
	int32_t i, j;

	uint16_t wGridWidth = CTerrainImpl::PATCH_XSIZE;
	float fGridWidth = (float)(wGridWidth * CTerrainImpl::CELLSCALE);

	int32_t lMapGridMinX = (((int32_t)pv3Position.x) / CTerrainImpl::CELLSCALE - viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMaxX = (((int32_t)pv3Position.x) / CTerrainImpl::CELLSCALE + viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMinY = (-((int32_t)pv3Position.y) / CTerrainImpl::CELLSCALE - viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMaxY = (-((int32_t)pv3Position.y) / CTerrainImpl::CELLSCALE + viewradius) / wGridWidth * wGridWidth;

	for (i = lMapGridMinX; i <= lMapGridMaxX; i += wGridWidth)
	{
		fx = (float)i * CTerrainImpl::CELLSCALE;
		for (j = lMapGridMinY; j <= lMapGridMaxY; j += wGridWidth)
		{
			fy = (float)j * CTerrainImpl::CELLSCALE;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx, fy + fGridWidth) + 10.0f;
			RenderLine3d(fx, -fy, fcurz, fx, -fy - fGridWidth, fnextz);
		}
	}
	for (j = lMapGridMinY; j <= lMapGridMaxY; j += wGridWidth)
	{
		fy = (float)j * CTerrainImpl::CELLSCALE;
		for (i = lMapGridMinX; i <= lMapGridMaxX; i += wGridWidth)
		{
			fx = (float)i * CTerrainImpl::CELLSCALE;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx + fGridWidth, fy) + 10.0f;
			RenderLine3d(fx, -fy, fcurz, fx + fGridWidth, -fy, fnextz);
		}
	}
}

void CSceneMap::OnRenderMapBoundGrid()
{
	SetDiffuseOperation();
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	SetDiffuseColor(0.0f, 0.0f, 0.0f);

	STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);

	int32_t viewradius;
	float fHeightScale;
	viewradius			= m_pMapAccessor->GetViewRadius();
	fHeightScale		= m_pMapAccessor->GetHeightScale();

	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	float fx, fy, fcurz, fnextz;
	int32_t i, j;

	uint16_t wGridWidth = CTerrainImpl::XSIZE / 4;
	float fGridWidth = (float)(wGridWidth * CTerrainImpl::CELLSCALE);

	int32_t lMapGridMinX = (((int32_t)pv3Position.x) / CTerrainImpl::CELLSCALE - viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMaxX = (((int32_t)pv3Position.x) / CTerrainImpl::CELLSCALE + viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMinY = (-((int32_t)pv3Position.y) / CTerrainImpl::CELLSCALE - viewradius) / wGridWidth * wGridWidth;
	int32_t lMapGridMaxY = (-((int32_t)pv3Position.y) / CTerrainImpl::CELLSCALE + viewradius) / wGridWidth * wGridWidth;

	for (i = lMapGridMinX; i <= lMapGridMaxX; i += wGridWidth)
	{
		fx = (float)i * CTerrainImpl::CELLSCALE;
		for (j = lMapGridMinY; j <= lMapGridMaxY; j += wGridWidth)
		{
			fy = (float)j * CTerrainImpl::CELLSCALE;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx, fy + fGridWidth) + 10.0f;
			if (0 == i%CTerrainImpl::XSIZE)
			{
				RenderLine3d(fx, -fy, fcurz, fx, -fy - fGridWidth, fnextz);
			}
			else if (0 == i%(CTerrainImpl::XSIZE / 2))
			{
				SetDiffuseColor(1.0f, 1.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx, -fy - fGridWidth, fnextz);
				SetDiffuseColor(0.0f, 0.0f, 0.0f);
			}
			else if (0 == i%(CTerrainImpl::XSIZE / 4))
			{
				SetDiffuseColor(1.0f, 0.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx, -fy - fGridWidth, fnextz);
				SetDiffuseColor(0.0f, 0.0f, 0.0f);
			}
		}
	}
	for (j = lMapGridMinY; j <= lMapGridMaxY; j += wGridWidth)
	{
		fy = (float)j * CTerrainImpl::CELLSCALE;
		for (i = lMapGridMinX; i <= lMapGridMaxX; i += wGridWidth)
		{
			fx = (float)i * CTerrainImpl::CELLSCALE;
			fcurz = m_pMapAccessor->GetHeight(fx, fy) + 10.0f;
			fnextz = m_pMapAccessor->GetHeight(fx + fGridWidth, fy) + 10.0f;
			if (0 == j%CTerrainImpl::YSIZE)
			{
				RenderLine3d(fx, -fy, fcurz, fx + fGridWidth, -fy, fnextz);
			}
			else if (0 == j%(CTerrainImpl::YSIZE / 2))
			{
				SetDiffuseColor(1.0f, 1.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx + fGridWidth, -fy, fnextz);
				SetDiffuseColor(0.0f, 0.0f, 0.0f);
			}
			else if (0 == j%(CTerrainImpl::YSIZE / 4))
			{
				SetDiffuseColor(1.0f, 0.0f, 0.0f);
				RenderLine3d(fx, -fy, fcurz, fx + fGridWidth, -fy, fnextz);
				SetDiffuseColor(0.0f, 0.0f, 0.0f);
			}
		}
	}
}

void CSceneMap::OnRenderCharacter()
{
	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	SetDiffuseOperation();
	RenderBackGroundCharacter(pv3Position.x, pv3Position.y, pv3Position.z - 100.0f);
}

void CSceneMap::OnRenderObjectCollision()
{
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	m_pMapManagerAccessor->RenderObjectCollision();
}

void CSceneMap::OnRenderCenterCursor()
{
	SetDiffuseColor(m_PickingPointColor.r, m_PickingPointColor.g, m_PickingPointColor.b);
	RenderLine3d(
		m_vecMouseMapIntersectPosition.x, m_vecMouseMapIntersectPosition.y, m_vecMouseMapIntersectPosition.z - 100.0f,
		m_vecMouseMapIntersectPosition.x, m_vecMouseMapIntersectPosition.y, m_vecMouseMapIntersectPosition.z + 100.0f);
	RenderLine3d(
		m_vecMouseMapIntersectPosition.x - 100.0f, m_vecMouseMapIntersectPosition.y, m_vecMouseMapIntersectPosition.z + 1.0f,
		m_vecMouseMapIntersectPosition.x + 100.0f, m_vecMouseMapIntersectPosition.y, m_vecMouseMapIntersectPosition.z + 1.0f);
	RenderLine3d(
		m_vecMouseMapIntersectPosition.x, m_vecMouseMapIntersectPosition.y - 100.0f, m_vecMouseMapIntersectPosition.z + 1.0f,
		m_vecMouseMapIntersectPosition.x, m_vecMouseMapIntersectPosition.y + 100.0f, m_vecMouseMapIntersectPosition.z + 1.0);
}

void CSceneMap::OnRenderCompass()
{
	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	SetDiffuseOperation();
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	STATEMANAGER.SaveRenderState(D3DRS_ZENABLE, FALSE);

	D3DXVECTOR3 arrowpartx1(-10.0f, 0.0f, -10.0f);
	D3DXVECTOR3 arrowpartx2(-10.0f, 0.0f, 10.0f);
	D3DXMATRIX matPitch, matRoll;

	D3DXMatrixRotationAxis(&matPitch, &D3DXVECTOR3(1.0f, 0.0f, 0.0f), -D3DXToRadian(ms_Camera->GetPitch()));
	D3DXVec3TransformCoord(&arrowpartx1, &arrowpartx1, &matPitch);
	D3DXVec3TransformCoord(&arrowpartx2, &arrowpartx2, &matPitch);
	SetDiffuseColor(1.0f, 0.0f, 0.0f);
	RenderLine3d(pv3Position.x, pv3Position.y, pv3Position.z - 100.0f,
				 pv3Position.x+100.0f, pv3Position.y, pv3Position.z - 100.0f);
	RenderLine3d(pv3Position.x+100.0f, pv3Position.y, pv3Position.z - 100.0f,
				 pv3Position.x+100.0f+arrowpartx1.x, pv3Position.y+arrowpartx1.y, pv3Position.z - 100.0f+arrowpartx1.z);
	RenderLine3d(pv3Position.x+100.0f, pv3Position.y, pv3Position.z - 100.0f,
				 pv3Position.x+100.0f+arrowpartx2.x, pv3Position.y+arrowpartx2.y, pv3Position.z - 100.0f+arrowpartx2.z);

	D3DXVECTOR3 arrowparty1(0.0f, -10.0f, -10.0f);
	D3DXVECTOR3 arrowparty2(0.0f, -10.0f, 10.0f);
	D3DXMatrixRotationAxis(&matPitch, &D3DXVECTOR3(0.0f, 1.0f, 0.0f), -D3DXToRadian(ms_Camera->GetPitch()));
	D3DXVec3TransformCoord(&arrowparty1, &arrowparty1, &matPitch);
	D3DXVec3TransformCoord(&arrowparty2, &arrowparty2, &matPitch);
	SetDiffuseColor(0.0f, 0.5f, 0.0f);
	RenderLine3d(pv3Position.x, pv3Position.y, pv3Position.z - 100.0f,
				 pv3Position.x, pv3Position.y+100.0f, pv3Position.z - 100.0f);
	RenderLine3d(pv3Position.x, pv3Position.y+100.0f, pv3Position.z - 100.0f,
				 pv3Position.x+arrowparty1.x, pv3Position.y+100.0f+arrowparty1.y, pv3Position.z - 100.0f+arrowparty1.z);
	RenderLine3d(pv3Position.x, pv3Position.y+100.0f, pv3Position.z - 100.0f,
				 pv3Position.x+arrowparty2.x, pv3Position.y+100.0f+arrowparty2.y, pv3Position.z - 100.0f+arrowparty2.z);

	D3DXVECTOR3 arrowpartz1(-10.0f, 0.0f, -10.0f);
	D3DXVECTOR3 arrowpartz2(10.0f, 0.0f, -10.0f);
	D3DXMatrixRotationZ(&matRoll, -D3DXToRadian(ms_Camera->GetRoll()));
	D3DXVec3TransformCoord(&arrowpartz1, &arrowpartz1, &matRoll);
	D3DXVec3TransformCoord(&arrowpartz2, &arrowpartz2, &matRoll);
	SetDiffuseColor(0.0f, 0.0f, 1.0f);
	RenderLine3d(pv3Position.x, pv3Position.y, pv3Position.z - 100.0f,
				 pv3Position.x, pv3Position.y, pv3Position.z);
	RenderLine3d(pv3Position.x, pv3Position.y, pv3Position.z,
				 pv3Position.x+arrowpartz1.x, pv3Position.y+arrowpartz1.y, pv3Position.z+arrowpartz1.z);
	RenderLine3d(pv3Position.x, pv3Position.y, pv3Position.z,
				 pv3Position.x+arrowpartz2.x, pv3Position.y+arrowpartz2.y, pv3Position.z+arrowpartz2.z);

	float fx, fy;
	ProjectPosition(pv3Position.x+100.0f, pv3Position.y, pv3Position.z - 100.0f, &fx, &fy);
	m_TextInstance[0].SetPosition(fx+10.0f, fy);
	ProjectPosition(pv3Position.x, pv3Position.y+100.0f, pv3Position.z - 100.0f, &fx, &fy);
	m_TextInstance[1].SetPosition(fx+10.0f, fy);
	ProjectPosition(pv3Position.x, pv3Position.y, pv3Position.z, &fx, &fy);
	m_TextInstance[2].SetPosition(fx+10.0f, fy);

	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_PROJECTION, &matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_VIEW, &matIdentity);
	SetOrtho2D(1024.0f, 768.0f, 400.0f);
	SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	for (int32_t i = 0; i < 3; ++i)
	{
		m_TextInstance[i].Update();
		m_TextInstance[i].Render();
	}
	STATEMANAGER.RestoreTransform(D3DTS_VIEW);
	STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);

	STATEMANAGER.SetTexture(0, m_pCompasGraphicImageInstance.GetTexturePointer()->GetD3DTexture());
	m_aConpasTerrainDecal.Render();

	STATEMANAGER.RestoreRenderState(D3DRS_ZENABLE);
}

void CSceneMap::OnRenderTerrainEditingArea()
{
	int32_t EditX, EditY;
	uint8_t ucSubEditX, ucSubEditY;
	uint16_t usEditTerrainX, usEditTerrainY;
	m_pMapManagerAccessor->GetEditArea(&EditX, &EditY, &ucSubEditX, &ucSubEditY, &usEditTerrainX, &usEditTerrainY);

	uint8_t ucBrushSize = m_pMapManagerAccessor->GetBrushSize();

	float fheightscale, fx, fy, fz, fLeft, fTop, fRight, fBottom;
	fheightscale		= m_pMapAccessor->GetHeightScale();

	uint32_t dwBrushShape = m_pMapManagerAccessor->GetBrushShape();

	if (EDITING_MODE_ATTRIBUTE == m_iEditingMode)
	{
		if (m_pMapManagerAccessor->isAttrEditing())
		{
			fx = (float)(EditX * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fy = (float)(EditY * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fz = m_pMapAccessor->GetHeight((float)fx, (float)fy) + 10.0f;

			fLeft	= (float)((EditX - ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fTop	= (float)((EditY - ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fRight	= (float)((EditX + ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fBottom	= (float)((EditY + ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
		}
		else if (m_pMapManagerAccessor->isMonsterAreaInfoEditing())
		{
			uint8_t ucBrushSizeY = m_pMapManagerAccessor->GetBrushSizeY();
			fx = (float)(EditX * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fy = (float)(EditY * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fz = m_pMapAccessor->GetHeight((float)fx, (float)fy) + 10.0f;

			fLeft	= (float)((EditX - ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fTop	= (float)((EditY - ucBrushSizeY) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fRight	= (float)((EditX + ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fBottom	= (float)((EditY + ucBrushSizeY) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
		}
		else
			return;
	}
	else
	{
		if (m_pMapManagerAccessor->isWaterEditing())
		{
			m_pMapManagerAccessor->PreviewEditWater();
			return;
		}
		else if (m_pMapManagerAccessor->isTextureEditing())
		{
			fx = (float)(EditX * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fy = (float)(EditY * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fz = m_pMapAccessor->GetHeight((float)fx, (float)fy) + 10.0f;

			fLeft	= (float)((EditX - ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fTop	= (float)((EditY - ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fRight	= (float)((EditX + ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditX * CTerrainImpl::CELLSCALE / 2 + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fBottom	= (float)((EditY + ucBrushSize) * CTerrainImpl::CELLSCALE + ucSubEditY * CTerrainImpl::CELLSCALE / 2 + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
		}
		else
		{
			fx = (float)(EditX * CTerrainImpl::CELLSCALE + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fy = (float)(EditY * CTerrainImpl::CELLSCALE + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fz = m_pMapAccessor->GetHeight((float)fx, (float)fy) + 10.0f;

			fLeft	= (float)((EditX - ucBrushSize) * CTerrainImpl::CELLSCALE + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fTop	= (float)((EditY - ucBrushSize) * CTerrainImpl::CELLSCALE + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
			fRight	= (float)((EditX + ucBrushSize) * CTerrainImpl::CELLSCALE + usEditTerrainX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
			fBottom	= (float)((EditY + ucBrushSize) * CTerrainImpl::CELLSCALE + usEditTerrainY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);
		}
	}

	SetDiffuseColor(m_EditCenterColor.r, m_EditCenterColor.g, m_EditCenterColor.b);
	RenderLine3d( fx, -fy, fz - 30.0f, fx, -fy, fz + 30.0f);
	RenderLine3d( fx - 30.0f, -fy, fz, fx + 30.0f, -fy, fz);
	RenderLine3d( fx, -fy - 30.0f, fz, fx, -fy + 30.0f, fz);

	SetDiffuseColor(m_EditablePointColor.r, m_EditablePointColor.g, m_EditablePointColor.b);

	SetDiffuseOperation();
	STATEMANAGER.SetTransform(D3DTS_WORLD, &GetIdentityMatrix());
	STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);
	SetDiffuseColor(globals::dft::RENDER_CURSOR_COLOR_R,globals::dft::RENDER_CURSOR_COLOR_G,globals::dft::RENDER_CURSOR_COLOR_B);

	if (CTerrainAccessor::BRUSH_SHAPE_CIRCLE == dwBrushShape)
		m_CursorRenderer.RenderCursorCircle(fx, fy, 0.0f, float(ucBrushSize * CTerrainImpl::CELLSCALE));
	else if (CTerrainAccessor::BRUSH_SHAPE_SQUARE == dwBrushShape)
		m_CursorRenderer.RenderCursorSquare( fLeft,	fTop, fRight, fBottom, 2 * ucBrushSize);
}

void CSceneMap::OnRenderSelectedObject()
{
	m_pMapManagerAccessor->RenderSelectedObject();
}

void CSceneMap::OnRenderObjectSettingArea()
{
	m_CursorRenderer.Update();
	SetColorOperation();
	m_CursorRenderer.RenderCursorArea();
	SetDiffuseOperation();
	m_CursorRenderer.Render();
}

void CSceneMap::SaveMiniMapWithMonsterAreaInfo()
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	CTerrainAccessor * pTerrainAccessor;
	if (!m_pMapAccessor->GetTerrainPointer(4, (CTerrain **) &pTerrainAccessor))
		return;

	CMiniMapRenderHelper::Instance().SetMiniMapPower(10);
	CMiniMapRenderHelper::Instance().CreateTextures();

	CMiniMapRenderHelper::Instance().SetMapOutdoorAccssorPointer(m_pMapAccessor);

	uint16_t wCoordX, wCoordY;
	pTerrainAccessor->GetCoordinate(&wCoordX, &wCoordY);
	CMiniMapRenderHelper::Instance().SetTargetTerrainCoord(wCoordX, wCoordY);

	if (CMiniMapRenderHelper::Instance().StartRendering())
	{
		m_pMapAccessor->RenderMiniMap();
		SetColorOperation();
		SetDiffuseColor(1.0f, 1.0f, 1.0f);
		CMonsterAreaInfo * pMonsterAreaInfo;
		uint32_t dwMonsterAreaCount = m_pMapManagerAccessor->GetMonsterAreaInfoCount();
		for (uint32_t dwIndex = 0; dwIndex < dwMonsterAreaCount; ++dwIndex)
		{
			if (!m_pMapManagerAccessor->GetMonsterAreaInfoFromVectorIndex(dwIndex, &pMonsterAreaInfo))
				continue;
			OnRenderMonsterAreaInfo(pMonsterAreaInfo);
		}
	}
	else
		Trace("CSceneMap::SaveMiniMap() Rendering Failed...\n");

	CMiniMapRenderHelper::Instance().EndRendering();

	std::string strFolderName = "MAI" + m_pMapAccessor->GetName();
	CreateDirectory(strFolderName.c_str(), NULL);

	char szFileName[256];
	uint16_t wX, wY;
	pTerrainAccessor->GetCoordinate(&wX, &wY);
	uint32_t ulID = (uint32_t) (wX) * 1000L + (uint32_t)(wY);

	sprintf(szFileName, "%s\\%06u_minimap.bmp", strFolderName.c_str(), ulID);
	DeleteFile(szFileName);
	D3DXSaveTextureToFile(szFileName, D3DXIFF_BMP, CMiniMapRenderHelper::Instance().GetMiniMapTexture(), NULL);

	CMiniMapRenderHelper::Instance().ReleaseTextures();
	CMiniMapRenderHelper::Instance().SetMiniMapPower(8);
}

void CSceneMap::OnLightMove(const int32_t & c_rlx, const int32_t & c_rly)
{
	int32_t xMove = m_loldX - c_rlx;
	int32_t yMove = m_loldY - c_rly;

	const TEnvironmentData * pEnvironmentData;
	m_pMapManagerAccessor->GetEnvironmentData(&pEnvironmentData);

	D3DXVECTOR3 v3DirLight;

	v3DirLight.x = pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.x;
	v3DirLight.y = pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.y;
	v3DirLight.z = pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction.z;

	float fRoll, fPitch;
	fRoll = (float)xMove * 0.01f;
	fPitch = (float)yMove * 0.01f;

	D3DXMATRIX matTranform, udRotation, lrRotation;
	D3DXMatrixRotationX(&udRotation, fPitch);
	D3DXMatrixRotationZ(&lrRotation, fRoll);

	matTranform=lrRotation*udRotation;

	D3DXVec3TransformCoord(&v3DirLight, &v3DirLight, &matTranform);
	D3DXVec3Normalize(&v3DirLight, &v3DirLight);

	m_pMapManagerAccessor->SetLightDirection(v3DirLight.x, v3DirLight.y, v3DirLight.z);

	m_loldX = c_rlx;
	m_loldY = c_rly;
}

void CSceneMap::SetObjectShadowRendering(bool bOn)
{
	m_pMapAccessor->SetDrawShadow(bOn);
	if (globals::dft::VIEW_OBJECT_LIGHTING)
		m_pMapAccessor->SetDrawCharacterShadow(bOn); // @fixme122
	m_bShadowRenderingOn = bOn;
}

void CSceneMap::SetGuildAreaRendering(bool bOn)
{
	m_bGuildAreaRenderingOn = bOn;
	if (bOn)
		m_pMapAccessor->VisibleMarkedArea();
	else
		m_pMapAccessor->DisableMarkedArea();
}

void CSceneMap::SetEditingMode(int32_t iMode)
{
	if (m_iEditingMode!=iMode)
	{
		//m_CursorRenderer.ClearCursor();
		m_iEditingMode = iMode;
	}
}

void CSceneMap::RefreshArea()
{
	m_pMapManagerAccessor->RefreshArea();
}

CCursorRenderer * CSceneMap::GetCursorRenererPointer()
{
	return & m_CursorRenderer;
}

void CSceneMap::SetMapManagerAccessor(CMapManagerAccessor * pMapManagerAccessor)
{
	m_pMapManagerAccessor	= pMapManagerAccessor;
	m_pMapAccessor			= (CMapOutdoorAccessor*) &pMapManagerAccessor->GetMapOutdoorRef();
	m_pHeightObserver		= pMapManagerAccessor->GetHeightObserverPointer();
	m_CursorRenderer.SetHeightObserver(m_pHeightObserver);
}

#ifdef USE_WE_CONFIG
typedef struct keyStruct_s
{
	int32_t key;
	float x;
	float y;
} keyStruct_t;

static keyStruct_t keyStruct_a[] = {
	{VK_W, 0.0f, globals::dft::WASD_MINIMAL_MOVE},
	{VK_A, -globals::dft::WASD_MINIMAL_MOVE, 0.0f},
	{VK_S, 0.0f, -globals::dft::WASD_MINIMAL_MOVE},
	{VK_D, globals::dft::WASD_MINIMAL_MOVE, 0.0f},
	{VK_UP, 0.0f, globals::dft::WASD_MINIMAL_MOVE*10},
	{VK_LEFT, -globals::dft::WASD_MINIMAL_MOVE*10, 0.0f},
	{VK_DOWN, 0.0f, -globals::dft::WASD_MINIMAL_MOVE*10},
	{VK_RIGHT, globals::dft::WASD_MINIMAL_MOVE*10, 0.0f},
};

void preWASD(CSceneMap* pScene, int32_t idx)
{
	pScene->MovePosition(keyStruct_a[idx].x, keyStruct_a[idx].y);
}

void recWASD(CSceneMap* pScene, int32_t iChar)
{
	if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000))
		return;
	for (int32_t i=0; i<sizeof(keyStruct_a); i++)
	{
		if (keyStruct_a[i].key==iChar || GetAsyncKeyState(keyStruct_a[i].key)&0x8000)
			preWASD(pScene, i);
	}
}
#endif

void CSceneMap::OnKeyDown(int32_t iChar)
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
	switch (iChar)
	{
		case VK_Z:
			m_pMapAccessor->SetSplatValue(m_pMapAccessor->GetSplatValue() - 0.1f);
			break;

		case VK_X:
			m_pMapAccessor->SetSplatValue(m_pMapAccessor->GetSplatValue() + 0.1f);
			break;

#ifdef USE_WE_CONFIG
		case VK_W:
		case VK_A:
		case VK_S: // ctrl+save OnKeyUp
		case VK_D:

		case VK_UP:
		case VK_LEFT:
		case VK_DOWN:
		case VK_RIGHT:
			recWASD(this, iChar);
			break;
#endif

		case VK_F3:
			SetMapBoundGrid(!GetMapBoundGrid());
			break;

		case VK_ESCAPE:
			__ClearCursor();
			break;

		case VK_DELETE:
			if (EDITING_MODE_OBJECT == m_iEditingMode)
			{
				m_pMapManagerAccessor->BackupObject();
				m_pMapManagerAccessor->DeleteSelectedObject();
				m_pMapManagerAccessor->BackupObjectCurrent();
			}
			else if (EDITING_MODE_ATTRIBUTE == m_iEditingMode)
			{
				m_pMapManagerAccessor->RemoveMonsterAreaInfoPtr(m_pMapManagerAccessor->GetSelectedMonsterAreaInfo());
			}
			pFrame->UpdateMapControlBar();
			break;

		case VK_R:
			m_pMapManagerAccessor->ReloadTerrainShadowTexture();
			m_pMapAccessor->ResetTextures();
			m_pMapAccessor->ReloadBuildingTexture();
			break;

		case VK_PRIOR:
			CShadowRenderHelper::Instance().SetShadowMapPower(CShadowRenderHelper::Instance().GetShadowMapPower() + 1);
			break;

		case VK_NEXT:
			CShadowRenderHelper::Instance().SetShadowMapPower(CShadowRenderHelper::Instance().GetShadowMapPower() - 1);
			break;

		case VK_HOME:
			CShadowRenderHelper::Instance().SetIntermediateShadowMapPower(CShadowRenderHelper::Instance().GetIntermediateShadowMapPower()+1);
			break;

		case VK_END:
			CShadowRenderHelper::Instance().SetIntermediateShadowMapPower(CShadowRenderHelper::Instance().GetIntermediateShadowMapPower()-1);
			break;
	}
}

void __MdatrHeightRefresh()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	pMapManagerAccessor->RefreshArea();
}

void CSceneMap::OnKeyUp(int32_t iChar)
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	CCamera * pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
	D3DXVECTOR3 v3Target = pCurrentCamera->GetTarget();
	switch (iChar)
	{
		case VK_F6:
		case VK_INSERT:
			m_bTerrainShadowMapUpdateNeeded = true;
			break;
		case VK_S:
			if (GetAsyncKeyState(VK_LCONTROL)&0x8000)
				m_pMapManagerAccessor->SaveMap();
			break;
		case VK_T:
			if (GetAsyncKeyState(VK_LCONTROL)&0x8000)
			{
				printf("distance(%f), pitch(%f), roll(%f)\n", pCurrentCamera->GetDistance(), pCurrentCamera->GetPitch(), pCurrentCamera->GetRoll());
				break;
			}
			pCurrentCamera->SetDistance(32271.91f);
			pCurrentCamera->RotateEyeAroundTarget(-pCurrentCamera->GetPitch()+15.20f, -pCurrentCamera->GetRoll()+135.30f);
			MovePosition(-v3Target.x, -v3Target.y);
			break;
		case VK_H:
			__MdatrHeightRefresh();
			break;
	}
}

void CSceneMap::OnMouseMove(int32_t x, int32_t y)
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();

	CRect Rect;
	pView->GetWindowRect(&Rect);
 	SetPerspective(40.0f, float(Rect.Width()) / float(Rect.Height()), 100.0f, 50000.0f);
	SetCursorPosition((int32_t) x, (int32_t) y, Rect.Width(), Rect.Height());
	GetCursorPosition(&ms_vecMousePosition.x, &ms_vecMousePosition.y, &ms_vecMousePosition.z);
	m_pMapManagerAccessor->UpdateHeightFieldEditingPt(&m_vecMouseMapIntersectPosition);
	m_pMapManagerAccessor->SetEditingCursorPosition(m_CursorRenderer.GetCursorPosition());

	switch(m_iEditingMode)
	{
		case EDITING_MODE_OBJECT:

			m_bObjectIsMoved = TRUE;

			if (m_CursorRenderer.IsPicking())
			{
				const D3DXVECTOR3 & c_rv3PickedPosition = m_CursorRenderer.GetPickedPosition();
				const D3DXVECTOR3 & c_rv3CurrentPosition = m_CursorRenderer.GetCursorPosition();
				D3DXVECTOR3 v3Movement = c_rv3CurrentPosition - c_rv3PickedPosition;
				m_CursorRenderer.UpdatePickedPosition();

				m_pMapManagerAccessor->MoveSelectedObject(v3Movement.x, v3Movement.y);
			}
			else if (m_CursorRenderer.IsSelecting())
			{
				float fxStart, fyStart;
				float fxEnd, fyEnd;

				m_CursorRenderer.GetSelectArea(&fxStart, &fyStart, &fxEnd, &fyEnd);
				BOOL bNeedChange = m_pMapManagerAccessor->SelectObject(fxStart, fyStart, fxEnd, fyEnd);
				if (bNeedChange)
					pFrame->UpdateMapControlBar();
			}
			break;
		case EDITING_MODE_ENVIRONMENT:
			if (m_bLightPositionEditingOn && m_bLightPositionEditingInProgress)
				OnLightMove(x, y);
			break;
	}
}

void CSceneMap::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bCursorYawPitchChange=false;
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	switch (m_iEditingMode)
	{
		case EDITING_MODE_TERRAIN:
			m_pMapManagerAccessor->EditingStart();
			m_pMapManagerAccessor->BackupTerrain();
			m_pMapManagerAccessor->SetEditingCursorPosition(m_CursorRenderer.GetCursorPosition());
			break;
		case EDITING_MODE_OBJECT:
		{
			if (0 == m_CursorRenderer.GetCurrentCRC())
			{
				m_pMapManagerAccessor->BackupObject();
				m_iInsertedObjectIndex = -1;
				m_bObjectIsMoved = FALSE;

				int32_t iPickedObjectIndex = m_pMapManagerAccessor->GetPickedPickedObjectIndex();

				if (-1 == iPickedObjectIndex)
				{
					m_pMapManagerAccessor->CancelSelect();
					m_CursorRenderer.SelectStart();
				}
				else
				{
					if (0 == (GetAsyncKeyState(VK_LCONTROL) & 0x8000))
					{
						if (!m_pMapManagerAccessor->IsSelectedObject(iPickedObjectIndex))
							m_pMapManagerAccessor->CancelSelect();

						m_CursorRenderer.PickStart();
					}
					else
					{
						m_iInsertedObjectIndex = iPickedObjectIndex;
					}

					m_pMapManagerAccessor->SelectObject(iPickedObjectIndex);
				}

				CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
				pFrame->UpdateMapControlBar();
			}
			else
			{
				m_pMapManagerAccessor->BackupObject();

				uint32_t dwObjectCRC = m_CursorRenderer.GetCurrentCRC();

				float fRoll = m_CursorRenderer.GetCursorRoll();

				if (m_isRandomRotation)
					fRoll = random() % 360;

				int32_t ix, iy;
				for (uint32_t i = 0; i < m_CursorRenderer.GetSettingCursorCount(); ++i)
				{
					const D3DXVECTOR3 & c_rv3CursorPosition = m_CursorRenderer.GetCursorPosition();
					m_CursorRenderer.GetSettingCursorPosition(i, &ix, &iy);
					m_pMapManagerAccessor->InsertObject(c_rv3CursorPosition.x + ix,
											 c_rv3CursorPosition.y + iy,
											 m_CursorRenderer.GetObjectHeight(),
											 m_CursorRenderer.GetCursorYaw(),
											 m_CursorRenderer.GetCursorPitch(),
											 fRoll,
											 m_CursorRenderer.GetCursorScale(),
											 dwObjectCRC);
				}

				m_pMapManagerAccessor->BackupObjectCurrent();
			}
			break;
		}
		case EDITING_MODE_ENVIRONMENT:
			if (0 == (GetAsyncKeyState(VK_LCONTROL) & 0x8000))
			{
				m_ptClick = point;
				m_loldX = m_ptClick.x;
				m_loldY = m_ptClick.y;
				LightPositionEditingStart();
			}
			break;
		case EDITING_MODE_ATTRIBUTE:
			if (0 == (GetAsyncKeyState(VK_LCONTROL) & 0x8000))
				m_pMapManagerAccessor->EditingStart();
			break;
	}
}

void CSceneMap::OnLButtonUp()
{
	m_CursorRenderer.PickEnd();
	m_CursorRenderer.SelectEnd();
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	m_pMapManagerAccessor->EditingEnd();
	m_pMapManagerAccessor->BackupTerrainCurrent();
	if (EDITING_MODE_ENVIRONMENT == m_iEditingMode)
	{
		LightPositionEditingEnd();
		m_ptClick = CPoint(-1, -1);
	}
	if (EDITING_MODE_OBJECT == m_iEditingMode)
	{
		int32_t iPickedObjectIndex = m_pMapManagerAccessor->GetPickedPickedObjectIndex();

		if (iPickedObjectIndex != m_iInsertedObjectIndex)
			if (m_pMapManagerAccessor->GetSelectedObjectCount() > 1)
				if (m_pMapManagerAccessor->IsSelectedObject(iPickedObjectIndex))
					if (!m_bObjectIsMoved)
					{
						m_pMapManagerAccessor->CancelSelect();
						m_pMapManagerAccessor->SelectObject(iPickedObjectIndex);
					}

		if (m_bObjectIsMoved)
		{
			m_pMapManagerAccessor->BackupObjectCurrent();
		}

		m_bObjectIsMoved = FALSE;
	}
	m_bCursorYawPitchChange = false;
	m_CursorRenderer.SetCursor(m_dwCursorObjectCRC);
#ifdef CWE_AREA_ACCESSOR_MISSING_REFRESH
	m_pMapManagerAccessor->RefreshSelectedInfo();
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	pFrame->UpdateMapControlBar();
#endif
}

void CSceneMap::OnRButtonDown()
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

	switch (m_iEditingMode)
	{
		// case EDITING_MODE_OBJECT:
			// __ClearCursor();
			// break;
		case EDITING_MODE_ATTRIBUTE:
		{
			m_pMapManagerAccessor->SelectMonsterAreaInfoStart();
			break;
		}
	}
}

void CSceneMap::OnRButtonUp()
{

	if (!m_pMapManagerAccessor->IsMapReady())
		return;
	if (EDITING_MODE_ATTRIBUTE == m_iEditingMode)
	{
		m_pMapManagerAccessor->SelectMonsterAreaInfoEnd();
	}
}

BOOL CSceneMap::OnMouseWheel(int16_t zDelta)
{
	if (m_iEditingMode == EDITING_MODE_OBJECT)
	{
		if (GetAsyncKeyState(VK_1)&0x8000)
		{
			int32_t icurRotation = int32_t(m_CursorRenderer.GetCursorYaw()) / CArea::YAW_STEP_AMOUNT;
			int32_t iNewRotation = (icurRotation + int32_t(zDelta / 120)) * CArea::YAW_STEP_AMOUNT;
			iNewRotation = (iNewRotation + 360) % 360;
			m_CursorRenderer.SetCursorYaw(iNewRotation);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_2)&0x8000)
		{
			int32_t icurRotation = int32_t(m_CursorRenderer.GetCursorPitch()) / CArea::PITCH_STEP_AMOUNT;
			int32_t iNewRotation = (icurRotation - int32_t(zDelta / 120)) * CArea::PITCH_STEP_AMOUNT;
			iNewRotation = (iNewRotation + 360) % 360;
			m_CursorRenderer.SetCursorPitch(iNewRotation);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_3)&0x8000)
		{
			int32_t icurRotation = int32_t(m_CursorRenderer.GetCursorRoll()) / CArea::ROTATION_STEP_AMOUNT;
			int32_t iNewRotation = (icurRotation + int32_t(zDelta / 120)) * CArea::ROTATION_STEP_AMOUNT;
			iNewRotation = (iNewRotation + 360) % 360;
			m_CursorRenderer.SetCursorRoll(iNewRotation);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_4)&0x8000)
		{
			float fNewHeight = m_CursorRenderer.GetObjectHeight() + float(zDelta / 120) * 100.0f;
			m_CursorRenderer.SetObjectHeight(fNewHeight);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_5)&0x8000)
		{
			float fNewHeight = m_CursorRenderer.GetObjectHeight() + float(zDelta / 120) * 5.0f;
			m_CursorRenderer.SetObjectHeight(fNewHeight);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_6)&0x8000)
		{
			float fNewHeight = m_CursorRenderer.GetObjectHeight() + float(zDelta / 120) * 5.0f;
			m_CursorRenderer.SetObjectHeight(fNewHeight);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_7)&0x8000)
		{
			uint32_t dwScale = m_CursorRenderer.GetCursorScale() + uint32_t(float(zDelta / 120) * 100.0f);
			m_CursorRenderer.SetCursorScale(dwScale);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_Q)&0x8000)
		{
			float fMovement = float(zDelta / 120) * 50.0f;
			m_pMapManagerAccessor->MoveSelectedObjectHeight(fMovement);
			return TRUE;
		}

		if ((GetAsyncKeyState(VK_9)&0x8000) || (GetAsyncKeyState(VK_0)&0x8000))
		{
			float fMovement = float(zDelta / 120);
			m_bObjectIsMoved = TRUE;
			m_CursorRenderer.UpdatePickedPosition();

			if (GetAsyncKeyState(VK_RSHIFT)&0x8000)
				fMovement*=10;
			if (GetAsyncKeyState(VK_RCONTROL)&0x8000)
				fMovement*=100;

			if (GetAsyncKeyState(VK_9)&0x8000)
				m_pMapManagerAccessor->MoveSelectedObject(fMovement, 0.0f);
			if (GetAsyncKeyState(VK_0)&0x8000)
				m_pMapManagerAccessor->MoveSelectedObject(0.0f, fMovement);
			return TRUE;
		}

		if (GetAsyncKeyState(VK_A)&0x8000)
		{
			int32_t iAddScale = -int32_t(float(zDelta / 120) * 50.0f);
			m_pMapManagerAccessor->AddSelectedAmbienceScale(iAddScale);
			return TRUE;
		}

		if (GetAsyncKeyState(VK_S)&0x8000)
		{
			float fPercentage = -(float(zDelta) / 120.0f) / 50.0f;
			m_pMapManagerAccessor->AddSelectedAmbienceMaxVolumeAreaPercentage(fPercentage);
			return TRUE;
		}
	}
	else if (m_iEditingMode == EDITING_MODE_TERRAIN)
	{
		CWorldEditorApp * pApplication = (CWorldEditorApp*) AfxGetApp();
		CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
		if (GetAsyncKeyState(VK_4)&0x8000)
		{
			float fNewHeight = pMapManagerAccessor->GetBrushWaterHeight() + float(zDelta / 120) * 100.0f;
			pMapManagerAccessor->SetBrushWaterHeight(fNewHeight);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}

		if (GetAsyncKeyState(VK_5)&0x8000)
		{
			float fNewHeight = pMapManagerAccessor->GetBrushWaterHeight() + float(zDelta / 120) * 5.0f;
			pMapManagerAccessor->SetBrushWaterHeight(fNewHeight);

			CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
			pFrame->UpdateMapControlBar();

			return TRUE;
		}
	}
	else if (EDITING_MODE_ATTRIBUTE == m_iEditingMode)
	{
		if (m_pMapManagerAccessor->isSelectMonsterAreaInfoStarted())
		{
			if (zDelta > 0)
				m_pMapManagerAccessor->SelectNextMonsterAreaInfo();
			else
				m_pMapManagerAccessor->SelectNextMonsterAreaInfo(false);

			return TRUE;
		}
	}
	return FALSE;
}

void CSceneMap::OnMovePosition(float fx, float fy)
{
	if (!m_pMapManagerAccessor->IsMapReady())
		return;

 	D3DXVECTOR3 pv3Position = ms_Camera->GetTarget();

	int16_t sTerrainCountX, sTerrainCountY;
	m_pMapAccessor->GetTerrainCount(&sTerrainCountX, &sTerrainCountY);

	if (pv3Position.x + fx <= 0)
		fx = -pv3Position.x;
	else if (pv3Position.x + fx >= (float)(sTerrainCountX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE))
		fx = -pv3Position.x + (float)(sTerrainCountX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
	if ( pv3Position.y + fy >= 0)
		fy = -pv3Position.y;
	else if (pv3Position.y + fy <= - (float)(sTerrainCountY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE))
		fy = -pv3Position.y - (float)(sTerrainCountY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);

	float fz = m_pMapAccessor->GetHeight(pv3Position.x + fx, - pv3Position.y - fy) - pv3Position.z + 100.0f;
 	CCameraManager::Instance().GetCurrentCamera()->Move(D3DXVECTOR3(fx, fy, fz));
}

D3DXVECTOR3 CSceneMap::GetMouseMapIntersect()
{
	return m_vecMouseMapIntersectPosition;
}

//////////////////////////////////////////////////////////////////////////////
void CSceneMap::SetObjectBrushType(int32_t iType)
{
	m_CursorRenderer.SetObjectBrushType(iType);
}
void CSceneMap::SetObjectBrushScale(int32_t iScale)
{
	m_CursorRenderer.SetObjectAreaSize(iScale);
}

void CSceneMap::SetObjectBrushDensity(int32_t iDensity)
{
	m_CursorRenderer.SetObjectAreaDensity(iDensity);
}

void CSceneMap::SetObjectBrushRandom(int32_t iRandom)
{
	m_CursorRenderer.SetObjectAreaRandom(iRandom);
}

void CSceneMap::SetCheckingNearObject(bool bFlag)
{
	m_isCheckingNearObject = bFlag;
}

void CSceneMap::SetRandomRotation(bool bFlag)
{
	m_isRandomRotation = bFlag;
}

void CSceneMap::SetCursorYaw(float fYaw)
{
	m_pMapManagerAccessor->AddSelectedObjectRotation(fYaw - m_CursorRenderer.GetCursorYaw(), 0.0f, 0.0f);
	m_CursorRenderer.SetCursorYaw(fYaw);
}
void CSceneMap::SetCursorPitch(float fPitch)
{
	m_pMapManagerAccessor->AddSelectedObjectRotation(0.0f, fPitch - m_CursorRenderer.GetCursorPitch(), 0.0f);
	m_CursorRenderer.SetCursorPitch(fPitch);
}
void CSceneMap::SetCursorRoll(float fRoll)
{
	m_pMapManagerAccessor->AddSelectedObjectRotation(0.0f, 0.0f, fRoll - m_CursorRenderer.GetCursorRoll());
	m_CursorRenderer.SetCursorRoll(fRoll);
}
void CSceneMap::SetCursorScale(uint32_t dwScale)
{
	m_CursorRenderer.SetCursorScale(dwScale);
}

void CSceneMap::RefreshCursor()
{
	m_CursorRenderer.RefreshCursor();
}

void CSceneMap::ClearCursor()
{
	m_CursorRenderer.ClearCursor();
}

void CSceneMap::ChangeCursor(uint32_t dwCRC)
{
	m_dwCursorObjectCRC = dwCRC;
	m_CursorRenderer.SetCursor(dwCRC);
}

void CSceneMap::SetGridMode(uint8_t byGridMode)
{
	m_CursorRenderer.SetGridMode(byGridMode);
}

void CSceneMap::SetGridDistance(float fDistance)
{
	m_CursorRenderer.SetGridDistance(fDistance);
}

void CSceneMap::SetObjectHeight(float fHeight)
{
	m_pMapManagerAccessor->MoveSelectedObjectHeight(fHeight - m_CursorRenderer.GetObjectHeight());
	m_CursorRenderer.SetObjectHeight(fHeight);
}

float CSceneMap::GetCursorYaw()
{
	return m_CursorRenderer.GetCursorYaw();
}

float CSceneMap::GetCursorRoll()
{
	return m_CursorRenderer.GetCursorRoll();
}

float CSceneMap::GetCursorPitch()
{
	return m_CursorRenderer.GetCursorPitch();
}

float CSceneMap::GetCursorObjectHeight()
{
	return m_CursorRenderer.GetObjectHeight();
}

uint32_t CSceneMap::GetCursorScale()
{
	return m_CursorRenderer.GetCursorScale();
}

void CSceneMap::UpdateSelecting()
{
}

void CSceneMap::RenderSelectedObject()
{
}

void CSceneMap::__ClearCursor()
{
	m_CursorRenderer.SetCursor(0);
}

void CSceneMap::CreateEnvironment()
{
}

void CSceneMap::Initialize()
{
	CResource * pResource = CResourceManager::Instance().GetTypeResourcePointer("Tahoma:20.fnt");
	CGraphicText * pText = static_cast<CGraphicText*>(pResource);

	m_TextInstance[0].SetTextPointer(pText);
	m_TextInstance[1].SetTextPointer(pText);
	m_TextInstance[2].SetTextPointer(pText);

	m_TextInstance[0].SetValue("X", 1);
	m_TextInstance[1].SetValue("Y", 1);
	m_TextInstance[2].SetValue("Z", 1);

	m_TextInstance[0].Update();
	m_TextInstance[1].Update();
	m_TextInstance[2].Update();

	m_TextInstance[0].SetColor(1.0f, 0.0f, 0.0f);
	m_TextInstance[1].SetColor(0.0f, 0.5f, 0.0f);
	m_TextInstance[2].SetColor(0.0f, 0.0f, 1.0f);

	pResource = CResourceManager::Instance().GetResourcePointer("D:\\Ymir Work\\special\\compas.dds");
	m_pCompasGraphicImageInstance.SetImagePointer(static_cast<CGraphicImage *>(pResource));

	///////////////////////////////////////////////////////////////////////////////////////////

	CResource * pResourceOrtho = CResourceManager::Instance().GetTypeResourcePointer("Tahoma:15.fnt");
	CGraphicText * pTextOrtho = static_cast<CGraphicText*>(pResourceOrtho);
	m_textInstanceSplatTextureCount.SetTextPointer(pTextOrtho);
	m_textInstanceSplatTextureCount.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceSplatTextureCount.SetPosition(0.0f, 0.0f);
	m_textInstanceSplatMeshCount.SetTextPointer(pTextOrtho);
	m_textInstanceSplatMeshCount.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceSplatMeshCount.SetPosition(0.0f, 0.0f);
	m_textInstanceSplatMeshPercentage.SetTextPointer(pTextOrtho);
	m_textInstanceSplatMeshPercentage.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceSplatMeshPercentage.SetPosition(0.0f, 0.0f);
	m_textInstancePatchSplatTileCount.SetTextPointer(pTextOrtho);
	m_textInstancePatchSplatTileCount.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstancePatchSplatTileCount.SetPosition(0.0f, 0.0f);
	m_textInstanceTexture0Count.SetTextPointer(pTextOrtho);
	m_textInstanceTexture0Count.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceTexture0Count.SetPosition(0.0f, 0.0f);
#ifdef CWE_INFO_BOARD_POSITION
	m_textInstanceLocalPosition.SetTextPointer(pTextOrtho);
	m_textInstanceLocalPosition.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceLocalPosition.SetPosition(0.0f, 0.0f);
	m_textInstanceGlobalPosition.SetTextPointer(pTextOrtho);
	m_textInstanceGlobalPosition.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceGlobalPosition.SetPosition(0.0f, 0.0f);
#endif
	m_textInstanceMonsterInfo.SetTextPointer(pTextOrtho);
	m_textInstanceMonsterInfo.SetColor(0.8f, 0.8f, 0.8f);
	m_textInstanceMonsterInfo.SetPosition(0.0f, 0.0f);
	m_textInstanceMonsterInfo.SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_CENTER);
}

CSceneMap::CSceneMap()
{
	m_pMapManagerAccessor = NULL;
	m_pMapAccessor = NULL;
	m_pHeightObserver = NULL;
	m_iEditingMode = EDITING_MODE_TERRAIN;

	m_vecMouseMapIntersectPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_ClearColor = D3DXCOLOR(0.4882f, 0.4882f, 0.4882f, 1.0f);
	m_EditCenterColor = D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
	m_EditablePointColor = D3DXCOLOR(1.0f, 0.25f, 0.25f, 1.0f);
	m_PickingPointColor = D3DXCOLOR(0.0f, 1.0f, 1.0f, 1.0f);

	m_isCheckingNearObject = false;
	m_isRandomRotation = false;

	m_bCompassOn = false;
	m_bMeterGridOn = false;
	m_bPatchGridOn = false;
	m_bMapBoundGridOn = false;
	m_bCharacterRenderingOn = false;
	m_bWaterRenderingOn = false;
	m_bObjectRenderingOn = true;
	m_bObjectCollisionRenderingOn = false;
	m_bTerrainRenderingOn = true;
	m_bShadowRenderingOn = true;
	m_bGuildAreaRenderingOn = false;

	m_bLightPositionEditingInProgress = false;
	m_bLightPositionEditingOn = true;

	m_bCursorYawPitchChange = false;

	m_bTerrainShadowMapUpdateNeeded = false;

	m_kPrevCoordinate.m_sTerrainCoordX = -1;
	m_kPrevCoordinate.m_sTerrainCoordY = -1;

	m_iInsertedObjectIndex = -1;
	m_bObjectIsMoved = FALSE;
	m_dwCursorObjectCRC = 0;
}

CSceneMap::~CSceneMap()
{
	m_kSnowEnvironment.Destroy();
}
