#include "StdAfx.h"
#include "MapOutdoor.h"
#include "TerrainPatch.h"
#include "AreaTerrain.h"
#include "TerrainQuadtree.h"

#include <EterLib/Camera.h>
#include <EterLib/StateManager.h>
#include <execution>

//#define MAX_RENDER_SPALT 150

void CMapOutdoor::RenderTerrain()
{
	if (!m_pTerrainPatchProxyList)
		return;

	if (!IsVisiblePart(PART_TERRAIN))
		return;

	if (!m_bSettingTerrainVisible)
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderTerrain **");

	CCamera* pCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	if (!pCamera)
		return;

	D3DXMATRIX matrix = ms_matView * ms_matProj;
	BuildViewFrustum(matrix);

	D3DXVECTOR3 v3Eye = pCamera->GetEye();
	m_fXforDistanceCaculation = -v3Eye.x;
	m_fYforDistanceCaculation = -v3Eye.y;

	m_PatchVector.clear();
	__RenderTerrain_RecurseRenderQuadTree(m_pRootNode);
	std::sort(std::execution::par, m_PatchVector.begin(), m_PatchVector.end());

	__RenderTerrain_RenderHardwareTransformPatch();
	D3DPERF_EndEvent();
}

void CMapOutdoor::__RenderTerrain_RecurseRenderQuadTree(CTerrainQuadtreeNode* Node, bool bCullCheckNeed)
{
	if (bCullCheckNeed)
	{
		switch (__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(Node->center, Node->radius))
		{
		case VIEW_ALL:
			// all child nodes need not cull check
			bCullCheckNeed = false;
			break;
		case VIEW_PART:
			break;
		case VIEW_NONE:
			// no need to render
			return;
		}
		// if no need cull check more
		// -> bCullCheckNeed = false;
	}

	if (Node->Size == 1)
	{
		D3DXVECTOR3 v3Center = Node->center;
		float fDistance = std::max(fabs(v3Center.x + m_fXforDistanceCaculation), fabs(-v3Center.y + m_fYforDistanceCaculation));
		__RenderTerrain_AppendPatch(v3Center, fDistance, Node->PatchNum);
	}
	else
	{
		if (Node->NW_Node != nullptr)
			__RenderTerrain_RecurseRenderQuadTree(Node->NW_Node, bCullCheckNeed);
		if (Node->NE_Node != nullptr)
			__RenderTerrain_RecurseRenderQuadTree(Node->NE_Node, bCullCheckNeed);
		if (Node->SW_Node != nullptr)
			__RenderTerrain_RecurseRenderQuadTree(Node->SW_Node, bCullCheckNeed);
		if (Node->SE_Node != nullptr)
			__RenderTerrain_RecurseRenderQuadTree(Node->SE_Node, bCullCheckNeed);
	}
}

int32_t	CMapOutdoor::__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(const D3DXVECTOR3& c_v3Center, const float& c_fRadius)
{
	const int32_t count = 6;

	D3DXVECTOR3 center = c_v3Center;
	center.y = -center.y;

	float distance[count]{};
	for (int32_t i = 0; i < count; ++i)
	{
		distance[i] = D3DXPlaneDotCoord(&m_plane[i], &center);
		if (distance[i] <= -c_fRadius)
			return VIEW_NONE;
	}

	for (int32_t i = 0; i < count; ++i)
	{
		if (distance[i] <= c_fRadius)
			return VIEW_PART;
	}

	return VIEW_ALL;
}

void CMapOutdoor::__RenderTerrain_AppendPatch(const D3DXVECTOR3& c_rv3Center, float fDistance, int32_t lPatchNum)
{
	assert(nullptr != m_pTerrainPatchProxyList && "CMapOutdoor::__RenderTerrain_AppendPatch");
	if (!m_pTerrainPatchProxyList[lPatchNum].isUsed())
		return;

	m_pTerrainPatchProxyList[lPatchNum].SetCenterPosition(c_rv3Center);
	m_PatchVector.emplace_back(fDistance, lPatchNum);
}

void CMapOutdoor::ApplyLight(uint32_t dwVersion, const D3DLIGHT9& c_rkLight)
{
	STATEMANAGER->SetLight(0, &c_rkLight);
}

void CMapOutdoor::InitializeVisibleParts()
{
	m_dwVisiblePartFlags = 0xffffffff;
}

void CMapOutdoor::SetVisiblePart(int32_t ePart, bool isVisible)
{
	uint32_t dwMask = (1 << ePart);
	if (isVisible)
	{
		m_dwVisiblePartFlags |= dwMask;
	}
	else
	{
		uint32_t dwReverseMask = ~dwMask;
		m_dwVisiblePartFlags &= dwReverseMask;
	}
}

bool CMapOutdoor::IsVisiblePart(int32_t ePart)
{
	uint32_t dwMask = (1 << ePart);
	if (dwMask & m_dwVisiblePartFlags)
		return true;

	return false;
}

void CMapOutdoor::SetSplatLimit(int32_t iSplatNum)
{
	m_iSplatLimit = iSplatNum;
}

std::vector<int32_t>& CMapOutdoor::GetRenderedSplatNum(int32_t* piPatch, int32_t* piSplat, float* pfSplatRatio)
{
	*piPatch = m_iRenderedPatchNum;
	*piSplat = m_iRenderedSplatNum;
	*pfSplatRatio = m_iRenderedSplatNumSqSum / float(m_iRenderedPatchNum);

	return m_RenderedTextureNumVector;
}

void CMapOutdoor::RenderBeforeLensFlare()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderBeforeLensFlare **");
	m_LensFlare.DrawBeforeFlare();

	if (!mc_pEnvironmentData)
	{
		SysLog("CMapOutdoor::RenderBeforeLensFlare mc_pEnvironmentData is nullptr");
		return;
	}

	m_LensFlare.Compute(mc_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction);
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderAfterLensFlare()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderAfterLensFlare **");
	m_LensFlare.AdjustBrightness();
	m_LensFlare.DrawFlare();
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderCollision()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderCollision **");
	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
			pArea->RenderCollision();
	}
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderScreenFiltering()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderScreenFiltering **");
	m_ScreenFilter.Render();
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderSky()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderSky **");
	if (IsVisiblePart(PART_SKY))
		m_SkyBox.Render();
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderCloud()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderCloud **");
	if (IsVisiblePart(PART_CLOUD))
		m_SkyBox.RenderCloud();
	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderTree()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** RenderTree **");
	if (IsVisiblePart(PART_TREE))
		CSpeedTreeForestDirectX9::GetInstance()->Render();

	D3DPERF_EndEvent();
}

void CMapOutdoor::SetInverseViewAndDynamicShaodwMatrices()
{
	CCamera* pCamera = CCameraManager::GetInstance()->GetCurrentCamera();

	if (!pCamera)
		return;

	m_matViewInverse = pCamera->GetInverseViewMatrix();

	D3DXVECTOR3 v3Target = pCamera->GetTarget();

	D3DXVECTOR3 v3LightEye(v3Target.x - 1.732f * 1250.0f,
		v3Target.y - 1250.0f,
		v3Target.z + 2.0f * 1.732f * 1250.0f);

	const D3DXVECTOR3 cCalc = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
	D3DXMatrixLookAtRH(&m_matLightView, &v3LightEye, &v3Target, &cCalc);
	m_matDynamicShadow = m_matViewInverse * m_matLightView * m_matDynamicShadowScale;
}

void CMapOutdoor::OnRender()
{
	SetInverseViewAndDynamicShaodwMatrices();
	SetBlendOperation();
	RenderArea();
	RenderTree();
	RenderTerrain();
	RenderBlendArea();
}

void CMapOutdoor::OnRenderShadow()
{
	SetInverseViewAndDynamicShaodwMatrices();
	RenderAreaShadow();
}

void CMapOutdoor::OnRenderTreeShadow()
{
	if (!m_bDrawBgShadow)
		return;

	CSpeedTreeForestDirectX9::GetInstance()->RenderShadows();
}

struct FAreaRenderShadow
{
	void operator () (CGraphicObjectInstance* pInstance)
	{
		pInstance->RenderShadow();
		//pInstance->Hide();
	}
};

struct FPCBlockerHide
{
	void operator () (CGraphicObjectInstance* pInstance)
	{
		pInstance->Hide();
	}
};

struct FRenderPCBlocker
{
	void operator () (CGraphicObjectInstance* pInstance)
	{
		pInstance->Show();
		auto* pThingInstance = dynamic_cast<CGraphicThingInstance*>(pInstance); // the only dynamic cast required!
		if (pThingInstance != nullptr)
		{
			if (pThingInstance->HaveBlendThing())
			{
				STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				pThingInstance->BlendRender();
				return;
			}
		}
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		pInstance->RenderPCBlocker();
	}
};

void CMapOutdoor::RenderEffect()
{
	if (!IsVisiblePart(PART_OBJECT))
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CMapOutdoor::RenderEffect **");

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
			pArea->RenderEffect();
	}

	D3DPERF_EndEvent();
}

struct CMapOutdoor_LessThingInstancePtrRenderOrder
{
	bool operator() (CGraphicThingInstance* pkLeft, CGraphicThingInstance* pkRight)
	{
		CCamera* pCurrentCamera = CCameraManager::GetInstance()->GetCurrentCamera();
		const D3DXVECTOR3& c_rv3CameraPos = pCurrentCamera->GetEye();
		const D3DXVECTOR3& c_v3LeftPos = pkLeft->GetPosition();
		const D3DXVECTOR3& c_v3RightPos = pkRight->GetPosition();

		const D3DXVECTOR3 calc1 = D3DXVECTOR3(c_rv3CameraPos - c_v3RightPos);
		const D3DXVECTOR3 calc2 = D3DXVECTOR3(c_rv3CameraPos - c_v3LeftPos);

		return D3DXVec3LengthSq(&calc2) < D3DXVec3LengthSq(&calc1);
	}
};

struct CMapOutdoor_FOpaqueThingInstanceRender
{
	inline void operator () (CGraphicThingInstance* pkThingInst)
	{
		pkThingInst->Render();
	}
};
struct CMapOutdoor_FBlendThingInstanceRender
{
	inline void operator () (CGraphicThingInstance* pkThingInst)
	{
		pkThingInst->Render();
		pkThingInst->BlendRender();
	}
};

void CMapOutdoor::RenderArea(bool bRenderAmbience)
{
	if (!IsVisiblePart(PART_OBJECT))
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** CMapOutdoor::RenderArea **");
	for (int32_t j = 0; j < AROUND_AREA_NUM; ++j)
	{
		CArea* pArea;
		if (GetAreaPointer(j, &pArea))
			pArea->RenderDungeon();
	}

	bool bRenderShadow = true;
	if (GetName().find("metin2_map_orchideout") != std::string::npos)
		bRenderShadow = false;

	// PCBlocker
	std::for_each(m_PCBlockerVector.begin(), m_PCBlockerVector.end(), FPCBlockerHide());

	// Shadow Receiver
	if (m_bDrawShadow && m_bDrawChrShadow && bRenderShadow)
	{
		if (mc_pEnvironmentData != nullptr)
			STATEMANAGER->SetRenderState(D3DRS_FOGCOLOR, 0xFFFFFFFF);

		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
		STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

		// Transform
		STATEMANAGER->SaveTransform(D3DTS_TEXTURE1, &m_matDynamicShadow);
		STATEMANAGER->SetTexture(1, m_lpCharacterShadowMapTexture);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
		STATEMANAGER->SaveSamplerState(1, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);

		std::for_each(m_ShadowReceiverVector.begin(), m_ShadowReceiverVector.end(), FAreaRenderShadow());

		STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
		STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
		STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSU);
		STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSV);
		STATEMANAGER->RestoreSamplerState(1, D3DSAMP_BORDERCOLOR);
		STATEMANAGER->RestoreTransform(D3DTS_TEXTURE1);

		if (mc_pEnvironmentData != nullptr)
			STATEMANAGER->SetRenderState(D3DRS_FOGCOLOR, mc_pEnvironmentData->FogColor);
	}

	STATEMANAGER->SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);

	static std::vector<CGraphicThingInstance*> s_kVct_pkOpaqueThingInstSort;
	s_kVct_pkOpaqueThingInstSort.clear();

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
		{
			pArea->CollectRenderingObject(s_kVct_pkOpaqueThingInstSort);
		}

	}

	std::sort(s_kVct_pkOpaqueThingInstSort.begin(), s_kVct_pkOpaqueThingInstSort.end(), CMapOutdoor_LessThingInstancePtrRenderOrder());
	std::for_each(s_kVct_pkOpaqueThingInstSort.begin(), s_kVct_pkOpaqueThingInstSort.end(), CMapOutdoor_FOpaqueThingInstanceRender());

	STATEMANAGER->RestoreRenderState(D3DRS_ZWRITEENABLE);

	// Shadow Receiver
	if (m_bDrawShadow && m_bDrawChrShadow && bRenderShadow)
	{
		for (const auto& shadowReceiver : m_ShadowReceiverVector)
			shadowReceiver->Show();
	}

	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderBlendArea()
{
	if (!IsVisiblePart(PART_OBJECT))
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 255, 0, 0), L"** CMapOutdoor::RenderBlendArea **");

	static std::vector<CGraphicThingInstance*> s_kVct_pkBlendThingInstSort;
	s_kVct_pkBlendThingInstSort.clear();

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
			pArea->CollectBlendRenderingObject(s_kVct_pkBlendThingInstSort);
	}

	if (!s_kVct_pkBlendThingInstSort.empty())
	{
		std::sort(std::execution::par, s_kVct_pkBlendThingInstSort.begin(), s_kVct_pkBlendThingInstSort.end(), CMapOutdoor_LessThingInstancePtrRenderOrder());

		STATEMANAGER->SaveRenderState(D3DRS_ZWRITEENABLE, TRUE);
		STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

		std::for_each(std::execution::par, s_kVct_pkBlendThingInstSort.begin(), s_kVct_pkBlendThingInstSort.end(), CMapOutdoor_FBlendThingInstanceRender());

		STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
		STATEMANAGER->RestoreRenderState(D3DRS_SRCBLEND);
		STATEMANAGER->RestoreRenderState(D3DRS_DESTBLEND);
		STATEMANAGER->RestoreRenderState(D3DRS_ZWRITEENABLE);
	}

	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderAreaShadow()
{
	if (!m_bDrawBgShadow)
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CMapOutdoor::RenderAreaShadow **");

	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTA_TEXTURE);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	STATEMANAGER->SaveTransform(D3DTS_TEXTURE1, &m_matDynamicShadow);
	STATEMANAGER->SetTexture(1, m_lpCharacterShadowMapTexture);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
		{
			pArea->Render();

		}
	}

	D3DPERF_EndEvent();
}

void CMapOutdoor::RenderPCBlocker()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CMapOutdoor::RenderPCBlocker **");
	// PCBlocker
	if (!m_PCBlockerVector.empty())
	{
		STATEMANAGER->SetTexture(0, nullptr);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
		STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		STATEMANAGER->SaveTransform(D3DTS_TEXTURE1, &m_matBuildingTransparent);
		STATEMANAGER->SetTexture(1, m_BuildingTransparentImageInstance.GetTexturePointer()->GetD3DTexture());

		std::for_each(m_PCBlockerVector.begin(), m_PCBlockerVector.end(), FRenderPCBlocker());

		STATEMANAGER->SetTexture(1, nullptr);
		STATEMANAGER->RestoreTransform(D3DTS_TEXTURE1);
		STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
		STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSU);
		STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSV);
		STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	}

	D3DPERF_EndEvent();
}

void CMapOutdoor::SelectIndexBuffer(uint8_t byLODLevel, uint16_t* pwPrimitiveCount, D3DPRIMITIVETYPE* pePrimitiveType)
{
	if (0 == byLODLevel)
	{
		*pwPrimitiveCount = m_wNumIndices[0] - 2;
		*pePrimitiveType = D3DPT_TRIANGLESTRIP;
	}
	else
	{
		*pwPrimitiveCount = m_wNumIndices[byLODLevel] / 3;
		*pePrimitiveType = D3DPT_TRIANGLELIST;
	}

	STATEMANAGER->SetIndices(m_IndexBuffer[byLODLevel].GetD3DIndexBuffer());
}

float CMapOutdoor::__GetNoFogDistance()
{
	return static_cast<float>(CTerrainImpl::CELLSCALE * m_lViewRadius) * 0.5f;
}

float CMapOutdoor::__GetFogDistance()
{
	return static_cast<float>(CTerrainImpl::CELLSCALE * m_lViewRadius) * 0.75f;
}

struct FPatchNumMatch
{
	int32_t m_lPatchNumToCheck;
	FPatchNumMatch(int32_t lPatchNum)
	{
		m_lPatchNumToCheck = lPatchNum;
	}
	bool operator() (std::pair<int32_t, uint8_t>& aPair)
	{
		return m_lPatchNumToCheck == aPair.first;
	}
};

void CMapOutdoor::NEW_DrawWireFrame(CTerrainPatchProxy* pTerrainPatchProxy, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	uint32_t dwFillMode = STATEMANAGER->GetRenderState(D3DRS_FILLMODE);
	STATEMANAGER->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	uint32_t dwFogEnable = STATEMANAGER->GetRenderState(D3DRS_FOGENABLE);

	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, FALSE);
	STATEMANAGER->SetTexture(0, nullptr);
	STATEMANAGER->SetTexture(1, nullptr);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
	STATEMANAGER->SetRenderState(D3DRS_FILLMODE, dwFillMode);
	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, dwFogEnable);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}

void CMapOutdoor::DrawWireFrame(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType)
{
	assert(nullptr != m_pTerrainPatchProxyList && "CMapOutdoor::DrawWireFrame");

	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	int32_t sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;
	uint8_t ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	uint32_t dwFillMode = STATEMANAGER->GetRenderState(D3DRS_FILLMODE);
	STATEMANAGER->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	uint32_t dwFogEnable = STATEMANAGER->GetRenderState(D3DRS_FOGENABLE);

	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, FALSE);
	STATEMANAGER->SetTexture(0, nullptr);
	STATEMANAGER->SetTexture(1, nullptr);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->DrawIndexedPrimitive(ePrimitiveType, 0, m_iPatchTerrainVertexCount, 0, wPrimitiveCount);
	STATEMANAGER->SetRenderState(D3DRS_FILLMODE, dwFillMode);
	STATEMANAGER->SetRenderState(D3DRS_FOGENABLE, dwFogEnable);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}

// Attr
void CMapOutdoor::RenderMarkedArea()
{
	if (!m_pTerrainPatchProxyList)
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CMapOutdoor::RenderMarkedArea **");

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER->SetTransform(D3DTS_WORLD, &m_matWorldForCommonUse);

	uint16_t wPrimitiveCount;
	D3DPRIMITIVETYPE eType;
	SelectIndexBuffer(0, &wPrimitiveCount, &eType);

	D3DXMATRIX matTexTransform;

	D3DXMatrixScaling(&matTexTransform, m_fTerrainTexCoordBase * 32.0f, -m_fTerrainTexCoordBase * 32.0f, 0.0f);
	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &matTexTransform);
	STATEMANAGER->SaveTransform(D3DTS_TEXTURE0, &matTexTransform);
	STATEMANAGER->SaveTransform(D3DTS_TEXTURE1, &matTexTransform);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	static int32_t lStartTime = timeGetTime();
	float fTime = static_cast<float>((timeGetTime() - lStartTime) % 3000) / 3000.0f;
	float fAlpha = fabs(fTime - 0.5f) / 2.0f + 0.1f;
	STATEMANAGER->SetRenderState(D3DRS_TEXTUREFACTOR, D3DXCOLOR(1.0f, 1.0f, 1.0f, fAlpha));
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG2);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	STATEMANAGER->SetTexture(0, m_attrImageInstance.GetTexturePointer()->GetD3DTexture());

	RecurseRenderAttr(m_pRootNode);

	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXCOORDINDEX);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS);
	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_MINFILTER);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_MAGFILTER);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_MIPFILTER);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSU);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSV);
	STATEMANAGER->RestoreTransform(D3DTS_TEXTURE0);
	STATEMANAGER->RestoreTransform(D3DTS_TEXTURE1);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_SRCBLEND);
	STATEMANAGER->RestoreRenderState(D3DRS_DESTBLEND);
	D3DPERF_EndEvent();
}

void CMapOutdoor::RecurseRenderAttr(CTerrainQuadtreeNode* Node, bool bCullEnable)
{
	if (bCullEnable)
	{
		if (__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(Node->center, Node->radius) == VIEW_NONE)
			return;
	}
	if (Node->Size == 1)
	{
		DrawPatchAttr(Node->PatchNum);
	}
	else
	{
		if (Node->NW_Node != nullptr)
			RecurseRenderAttr(Node->NW_Node, bCullEnable);
		if (Node->NE_Node != nullptr)
			RecurseRenderAttr(Node->NE_Node, bCullEnable);
		if (Node->SW_Node != nullptr)
			RecurseRenderAttr(Node->SW_Node, bCullEnable);
		if (Node->SE_Node != nullptr)
			RecurseRenderAttr(Node->SE_Node, bCullEnable);
	}
}

void CMapOutdoor::DrawPatchAttr(int32_t patchnum)
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(251, 50, 50, 0), L" CMapOutdoor::DrawPatchAttr");

	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	if (!pTerrainPatchProxy->isUsed())
		return;

	int32_t sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	uint8_t ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	// Deal with this material buffer
	CTerrain* pTerrain;
	if (!GetTerrainPointer(ucTerrainNum, &pTerrain))
		return;

	if (!pTerrain->IsMarked())
		return;

	uint16_t wCoordX, wCoordY;
	pTerrain->GetCoordinate(&wCoordX, &wCoordY);

	m_matWorldForCommonUse._41 = -static_cast<float>(wCoordX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
	m_matWorldForCommonUse._42 = static_cast<float>(wCoordY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);

	D3DXMATRIX matTexTransform;
	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matTexTransform, &matTexTransform, &m_matStaticShadow);
	STATEMANAGER->SetTransform(D3DTS_TEXTURE1, &matTexTransform);

	TTerrainSplatPatch& rAttrSplatPatch = pTerrain->GetMarkedSplatPatch();
	STATEMANAGER->SetTexture(1, rAttrSplatPatch.Splats[0].pd3dTexture);
	STATEMANAGER->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL);
	STATEMANAGER->SetStreamSource(0, pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr()->GetD3DVertexBuffer(), m_iPatchTerrainVertexSize);
	STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_iPatchTerrainVertexCount, 0, m_wNumIndices[0] - 2);

	D3DPERF_EndEvent();
}
