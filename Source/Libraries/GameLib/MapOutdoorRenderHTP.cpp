#include "StdAfx.h"
#include <EterLib/StateManager.h>

#include "MapOutdoor.h"
#include <execution>

void CMapOutdoor::__RenderTerrain_RenderHardwareTransformPatch()
{
	uint32_t dwFogColor;
	float fFogFarDistance;
	float fFogNearDistance;

	if (mc_pEnvironmentData)
	{
		dwFogColor = mc_pEnvironmentData->FogColor;
		fFogNearDistance = mc_pEnvironmentData->GetFogNearDistance();
		fFogFarDistance = mc_pEnvironmentData->GetFogFarDistance();
	}
	else
	{
		dwFogColor = 0xffffffff;
		fFogNearDistance = 5000.0f;
		fFogFarDistance = 10000.0f;
	}

	//////////////////////////////////////////////////////////////////////////
	// Render State & TextureStageState	

	STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHAREF, 0x00000000);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	STATEMANAGER->SaveRenderState(D3DRS_TEXTUREFACTOR, dwFogColor);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	STATEMANAGER->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	CSpeedTreeWrapper::ms_bSelfShadowOn = true;
	STATEMANAGER->SetBestFiltering(0);
	STATEMANAGER->SetBestFiltering(1);

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER->SetTransform(D3DTS_WORLD, &m_matWorldForCommonUse);
	STATEMANAGER->SaveTransform(D3DTS_TEXTURE0, &m_matWorldForCommonUse);
	STATEMANAGER->SaveTransform(D3DTS_TEXTURE1, &m_matWorldForCommonUse);
	STATEMANAGER->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL);

	m_iRenderedSplatNumSqSum = 0;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_RenderedTextureNumVector.clear();

	std::pair<float, int32_t> fog_far(fFogFarDistance + 16000.0f, 0);
	std::pair<float, int32_t> fog_near(fFogNearDistance - 32000.0f, 0);

	auto far_it = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fog_far);
	auto near_it = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fog_near);

	uint16_t wPrimitiveCount;
	D3DPRIMITIVETYPE ePrimitiveType;

	uint8_t byCUrrentLODLevel = 0;

	float fLODLevel1Distance = __GetNoFogDistance();
	float fLODLevel2Distance = __GetFogDistance();

	SelectIndexBuffer(0, &wPrimitiveCount, &ePrimitiveType);

	uint32_t dwFogEnable = STATEMANAGER->GetRenderState(D3DRS_FOGENABLE);
	auto it = m_PatchVector.begin();

	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, FALSE);

	for (; it != near_it; ++it)
	{
		if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
		{
			byCUrrentLODLevel = 1;
			SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
		}
		else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
		{
			byCUrrentLODLevel = 2;
			SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
		}

		__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);
		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;

		if (m_bDrawWireFrame)
			DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
	}

	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, dwFogEnable);

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for (it = near_it; it != far_it; ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** __HardwareTransformPatch_RenderPatchSplat **");

	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, FALSE);
	STATEMANAGER->SetRenderState(D3DRS_LIGHTING, FALSE);
	STATEMANAGER->SetTexture(0, nullptr);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, FALSE);
	STATEMANAGER->SetTexture(1, nullptr);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, FALSE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for (it = far_it; it != m_PatchVector.end(); ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchNone(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, dwFogEnable);
	STATEMANAGER->SetRenderState(D3DRS_LIGHTING, TRUE);

	std::sort(std::execution::par, m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end());

	STATEMANAGER->RestoreRenderState(D3DRS_TEXTUREFACTOR);
	STATEMANAGER->RestoreTransform(D3DTS_TEXTURE0);
	STATEMANAGER->RestoreTransform(D3DTS_TEXTURE1);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXCOORDINDEX);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS);
	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHATESTENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHAREF);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHAFUNC);

	// Render State & TextureStageState
	D3DPERF_EndEvent();
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchSplat(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** __HardwareTransformPatch_RenderPatchSplat **");
	assert(nullptr != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchSplat");
	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	int32_t sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	uint8_t ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	CTerrain* pTerrain;
	if (!GetTerrainPointer(ucTerrainNum, &pTerrain))
		return;

	uint32_t dwFogColor;
	if (mc_pEnvironmentData)
		dwFogColor = mc_pEnvironmentData->FogColor;
	else
		dwFogColor = 0xffffffff;

	uint16_t wCoordX, wCoordY;
	pTerrain->GetCoordinate(&wCoordX, &wCoordY);

	TTerrainSplatPatch& rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();

	D3DXMATRIX matTexTransform, matSplatAlphaTexTransform, matSplatColorTexTransform;
	m_matWorldForCommonUse._41 = -(float)(wCoordX * CTerrainImpl::TERRAIN_XSIZE);
	m_matWorldForCommonUse._42 = (float)(wCoordY * CTerrainImpl::TERRAIN_YSIZE);
	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matSplatAlphaTexTransform, &matTexTransform, &m_matSplatAlpha);
	STATEMANAGER->SetTransform(D3DTS_TEXTURE1, &matSplatAlphaTexTransform);

	D3DXMATRIX matTiling;
	D3DXMatrixScaling(&matTiling, 1.0f / 640.0f, -1.0f / 640.0f, 0.0f);
	matTiling._41 = 0.0f;
	matTiling._42 = 0.0f;

	D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &matTiling);
	STATEMANAGER->SetTransform(D3DTS_TEXTURE0, &matSplatColorTexTransform);

	CGraphicVertexBuffer* pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	STATEMANAGER->SetStreamSource(0, pkVB->GetD3DVertexBuffer(), m_iPatchTerrainVertexSize);
	STATEMANAGER->SetRenderState(D3DRS_LIGHTING, FALSE);

	int32_t iPrevRenderedSplatNum = m_iRenderedSplatNum;

	bool isFirst = true;
	for (uint32_t j = 1; j < pTerrain->GetNumTextures(); ++j)
	{
		TTerainSplat& rSplat = rTerrainSplatPatch.Splats[j];

		if (!rSplat.Active)
			continue;

		if (rTerrainSplatPatch.PatchTileCount[sPatchNum][j] == 0)
			continue;

		const TTerrainTexture& rTexture = m_TextureSet.GetTexture(j);

		D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &rTexture.m_matTransform);
		STATEMANAGER->SetTransform(D3DTS_TEXTURE0, &matSplatColorTexTransform);
		if (isFirst)
		{
			STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			STATEMANAGER->SetTexture(0, rTexture.pd3dTexture);
			STATEMANAGER->SetTexture(1, rSplat.pd3dTexture);
			STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
			isFirst = false;
		}
		else
		{
			STATEMANAGER->SetTexture(0, rTexture.pd3dTexture);
			STATEMANAGER->SetTexture(1, rSplat.pd3dTexture);
			STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
		}

		auto aIterator = std::find(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end(), (int32_t)j);
		if (aIterator == m_RenderedTextureNumVector.end())
			m_RenderedTextureNumVector.emplace_back(j);
		++m_iRenderedSplatNum;
		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;

	}

	if (m_bDrawShadow)
	{
		STATEMANAGER->SetRenderState(D3DRS_LIGHTING, true);
		STATEMANAGER->SetRenderState(D3DRS_FOGCOLOR, 0xFFFFFFFF);
		STATEMANAGER->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		STATEMANAGER->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
		if (CTerrainPatch::IS_DYNAMIC_SHADOW)
		{
			STATEMANAGER->SetTexture(0, m_lpCharacterShadowMapTexture);
		}
		else
		{
			if (!m_bDrawBgShadow)
			{
				STATEMANAGER->SetTexture(0, m_lpCharacterShadowMapTexture);
			}
			else
			{
				D3DXMATRIX matShadowTexTransform;
				D3DXMatrixMultiply(&matShadowTexTransform, &matTexTransform, &m_matStaticShadow);
				STATEMANAGER->SetTransform(D3DTS_TEXTURE0, &matShadowTexTransform);
				STATEMANAGER->SetTexture(0, pTerrain->GetShadowTexture());
			}
		}
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		if (m_bDrawChrShadow)
		{
			STATEMANAGER->SetTransform(D3DTS_TEXTURE1, &m_matDynamicShadow);
			STATEMANAGER->SetTexture(1, m_lpCharacterShadowMapTexture);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
			STATEMANAGER->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			STATEMANAGER->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		}
		else
		{
			STATEMANAGER->SetTexture(1, nullptr);
		}

		ms_faceCount += wPrimitiveCount;
		STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
		++m_iRenderedSplatNum;

		if (m_bDrawChrShadow)
		{
			STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		}

		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		STATEMANAGER->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		STATEMANAGER->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		STATEMANAGER->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		STATEMANAGER->SetRenderState(D3DRS_FOGCOLOR, dwFogColor);
		STATEMANAGER->SetRenderState(D3DRS_LIGHTING, FALSE);
	}
	++m_iRenderedPatchNum;

	int32_t iCurRenderedSplatNum = m_iRenderedSplatNum - iPrevRenderedSplatNum;

	m_iRenderedSplatNumSqSum += iCurRenderedSplatNum * iCurRenderedSplatNum;
	D3DPERF_EndEvent();
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchNone(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	assert(nullptr != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchNone");
	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	CGraphicVertexBuffer* pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	STATEMANAGER->SetStreamSource(0, pkVB->GetD3DVertexBuffer(), m_iPatchTerrainVertexSize);
	STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
}
