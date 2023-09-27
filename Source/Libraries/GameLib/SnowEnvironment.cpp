#include "StdAfx.h"
#include <EterLib/StateManager.h>
#include <EterLib/Camera.h>
#include <EterLib/ResourceManager.h>

#include "SnowEnvironment.h"
#include "SnowParticle.h"

static const std::string snow_resource_filename = "d:/ymir work/special/snow.dds";

void CSnowEnvironment::Enable()
{
	if (!m_bSnowEnable)
		Create();

	m_bSnowEnable = true;
	//m_lastTime = DX::StepTimer::Instance().GetTotalSeconds();
}

void CSnowEnvironment::Disable()
{
	m_bSnowEnable = false;
}

void CSnowEnvironment::Update(const D3DXVECTOR3& c_rv3Pos)
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	m_v3Center = c_rv3Pos;
}

void CSnowEnvironment::Deform()
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	const D3DXVECTOR3& c_rv3Pos = m_v3Center;

	static int32_t s_lLastTime = CTimer::GetInstance()->GetCurrentMillisecond();
	int32_t lcurTime = CTimer::GetInstance()->GetCurrentMillisecond();
	float fElapsedTime = float(lcurTime - s_lLastTime) / 1000.0f;
	s_lLastTime = lcurTime;

	CCamera* pCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	if (!pCamera)
		return;

	const D3DXVECTOR3& c_rv3View = pCamera->GetView();

	D3DXVECTOR3 v3ChangedPos = c_rv3View * 3500.0f + c_rv3Pos;
	v3ChangedPos.z = c_rv3Pos.z;

	auto itor = m_kVct_pkParticleSnow.begin();
	for (; itor != m_kVct_pkParticleSnow.end();)
	{
		auto pSnow = (*itor).get();
		pSnow->Update(fElapsedTime, v3ChangedPos);

		if (!pSnow->IsActivate())
		{
			itor = m_kVct_pkParticleSnow.erase(itor);
		}
		else
		{
			++itor;
		}
	}

	if (m_bSnowEnable)
	{
		for (uint32_t p = 0; p < std::min<uint32_t>(10, m_dwParticleMaxNum - m_kVct_pkParticleSnow.size()); ++p)
		{
			auto pSnowParticle = std::make_unique<CSnowParticle>();
			pSnowParticle->Init(v3ChangedPos);
			m_kVct_pkParticleSnow.emplace_back(std::move(pSnowParticle));
		}
	}
}

void CSnowEnvironment::__BeginBlur()
{
	if (!m_bBlurEnable)
		return;

	ms_lpd3dDevice->GetRenderTarget(0, &m_lpOldSurface);
	ms_lpd3dDevice->GetDepthStencilSurface(&m_lpOldDepthStencilSurface);
	ms_lpd3dDevice->SetRenderTarget(0, m_lpSnowRenderTargetSurface);
	ms_lpd3dDevice->SetDepthStencilSurface(m_lpSnowDepthSurface);
	ms_lpd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0L);

	STATEMANAGER->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	STATEMANAGER->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	STATEMANAGER->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTALPHA);
}

void CSnowEnvironment::__ApplyBlur()
{
	if (!m_bBlurEnable)
		return;

	ms_lpd3dDevice->SetRenderTarget(0, m_lpOldSurface);
	ms_lpd3dDevice->SetDepthStencilSurface(m_lpOldDepthStencilSurface);

	STATEMANAGER->SetTexture(0, m_lpSnowTexture);
	STATEMANAGER->SetRenderState(D3DRS_ALPHABLENDENABLE, true);

	D3DSURFACE_DESC	desc;
	m_lpOldSurface->GetDesc(&desc);
	float sx = (float)desc.Width;
	float sy = (float)desc.Height;
	SAFE_RELEASE(m_lpOldSurface);
	SAFE_RELEASE(m_lpOldDepthStencilSurface);

	BlurVertex V[4] = { BlurVertex(D3DXVECTOR3(0.0f,0.0f,0.0f),1.0f	,0xFFFFFF, 0,0) ,
						BlurVertex(D3DXVECTOR3(sx,0.0f,0.0f),1.0f	,0xFFFFFF, 1,0) ,
						BlurVertex(D3DXVECTOR3(0.0f,sy,0.0f),1.0f	,0xFFFFFF, 0,1) ,
						BlurVertex(D3DXVECTOR3(sx,sy,0.0f),1.0f		,0xFFFFFF, 1,1) };

	STATEMANAGER->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	STATEMANAGER->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(BlurVertex));
}

void CSnowEnvironment::Render()
{
	if (!m_bSnowEnable)
	{
		if (m_kVct_pkParticleSnow.empty())
			return;
	}

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CSnowEnvironment::__BeginBlur **");
	__BeginBlur();
	D3DPERF_EndEvent();

	uint32_t dwParticleCount = std::min<uint32_t>(m_dwParticleMaxNum, m_kVct_pkParticleSnow.size());

	CCamera* pCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	if (!pCamera)
		return;

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CSnowEnvironment::Render **");

	const D3DXVECTOR3& c_rv3Up = pCamera->GetUp();
	const D3DXVECTOR3& c_rv3Cross = pCamera->GetCross();

	SParticleVertex* pv3Verticies = nullptr;
	if (SUCCEEDED(m_pVB->Lock(0, sizeof(SParticleVertex) * dwParticleCount * 4, (void**)&pv3Verticies, D3DLOCK_DISCARD)))
	{
		for (int32_t i = 0; i < m_kVct_pkParticleSnow.size(); i++)
		{
			auto* pSnow = m_kVct_pkParticleSnow[i].get();
			if (pSnow)
			{
				pSnow->SetCameraVertex(c_rv3Up, c_rv3Cross);
				pSnow->GetVerticies(pv3Verticies[i * 4 + 0], pv3Verticies[i * 4 + 1], pv3Verticies[i * 4 + 2], pv3Verticies[i * 4 + 3]);
			}
		}
		m_pVB->Unlock();
	}

	STATEMANAGER->SaveRenderState(D3DRS_ZWRITEENABLE, FALSE);
	STATEMANAGER->SaveRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	STATEMANAGER->SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	STATEMANAGER->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	STATEMANAGER->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTexture(1, nullptr);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	m_pImageInstance->GetGraphicImagePointer()->GetTextureReference().SetTextureStage(0);
	STATEMANAGER->SetIndices(m_pIB);
	STATEMANAGER->SetStreamSource(0, m_pVB, sizeof(SParticleVertex));
	STATEMANAGER->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
	STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, dwParticleCount * 4, 0, dwParticleCount * 2);
	STATEMANAGER->RestoreRenderState(D3DRS_ALPHABLENDENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_ZWRITEENABLE);
	STATEMANAGER->RestoreRenderState(D3DRS_CULLMODE);
	D3DPERF_EndEvent();

	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 50, 50, 0), L"** CSnowEnvironment::__ApplyBlur **");
	__ApplyBlur();
	D3DPERF_EndEvent();
}

bool CSnowEnvironment::__CreateBlurTexture()
{
	if (!m_bBlurEnable)
		return true;

	if (FAILED(ms_lpd3dDevice->CreateTexture(m_wBlurTextureSize, m_wBlurTextureSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_lpSnowTexture, nullptr)))
		return false;

	if (FAILED(m_lpSnowTexture->GetSurfaceLevel(0, &m_lpSnowRenderTargetSurface)))
		return false;

	if (FAILED(ms_lpd3dDevice->CreateDepthStencilSurface(m_wBlurTextureSize, m_wBlurTextureSize, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, false, &m_lpSnowDepthSurface, nullptr)))
		return false;

	if (FAILED(ms_lpd3dDevice->CreateTexture(m_wBlurTextureSize, m_wBlurTextureSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &m_lpAccumTexture, nullptr)))
		return false;

	if (FAILED(m_lpAccumTexture->GetSurfaceLevel(0, &m_lpAccumRenderTargetSurface)))
		return false;

	if (FAILED(ms_lpd3dDevice->CreateDepthStencilSurface(m_wBlurTextureSize, m_wBlurTextureSize, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, false, &m_lpAccumDepthSurface, nullptr)))
		return false;

	return true;
}

bool CSnowEnvironment::__CreateGeometry()
{
	if (FAILED(ms_lpd3dDevice->CreateVertexBuffer(sizeof(SParticleVertex) * m_dwParticleMaxNum * 4, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_XYZ | D3DFVF_TEX1,
		D3DPOOL_SYSTEMMEM, &m_pVB, 0)))
		return false;

	if (FAILED(ms_lpd3dDevice->CreateIndexBuffer(sizeof(uint16_t) * m_dwParticleMaxNum * 6, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &m_pIB, 0)))
		return false;

	uint16_t* dstIndices = nullptr;
	if (FAILED(m_pIB->Lock(0, sizeof(uint16_t) * m_dwParticleMaxNum * 6, (void**)&dstIndices, 0)))
		return false;

	const uint16_t c_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1 };
	for (uint32_t i = 0; i < m_dwParticleMaxNum; ++i)
	{
		for (int32_t j = 0; j < 6; ++j)
			dstIndices[i * 6 + j] = i * 4 + c_awFillRectIndices[j];
	}

	m_pIB->Unlock();
	return true;
}

bool CSnowEnvironment::Create()
{
	Destroy();

	if (!__CreateBlurTexture())
		return false;

	if (!__CreateGeometry())
		return false;

	CGraphicImage* pImage = CResourceManager::GetInstance()->LoadResource<CGraphicImage>(snow_resource_filename);
	m_pImageInstance = CGraphicImageInstance::New();
	m_pImageInstance->SetImagePointer(pImage);

	return true;
}

void CSnowEnvironment::Destroy()
{
	SAFE_RELEASE(m_lpSnowTexture);
	SAFE_RELEASE(m_lpOldDepthStencilSurface);
	SAFE_RELEASE(m_lpSnowRenderTargetSurface);
	SAFE_RELEASE(m_lpSnowDepthSurface);
	SAFE_RELEASE(m_lpAccumTexture);
	SAFE_RELEASE(m_lpAccumRenderTargetSurface);
	SAFE_RELEASE(m_lpAccumDepthSurface);
	SAFE_RELEASE(m_pVB);
	SAFE_RELEASE(m_pIB);

	m_kVct_pkParticleSnow.clear();

	if (m_pImageInstance)
	{
		CGraphicImageInstance::Delete(m_pImageInstance);
		m_pImageInstance = nullptr;
	}

	__Initialize();
}

void CSnowEnvironment::__Initialize()
{
	m_bSnowEnable = false;
	m_lpSnowTexture = nullptr;
	m_lpOldDepthStencilSurface = nullptr;
	m_lpSnowRenderTargetSurface = nullptr;
	m_lpSnowDepthSurface = nullptr;
	m_lpAccumTexture = nullptr;
	m_lpAccumRenderTargetSurface = nullptr;
	m_lpAccumDepthSurface = nullptr;
	m_pVB = nullptr;
	m_pIB = nullptr;
	m_pImageInstance = nullptr;

	m_kVct_pkParticleSnow.reserve(m_dwParticleMaxNum);
}

CSnowEnvironment::CSnowEnvironment()
{
	m_bBlurEnable = false;
	m_dwParticleMaxNum = 3000;
	m_wBlurTextureSize = 512;

	__Initialize();
}
CSnowEnvironment::~CSnowEnvironment()
{
	Destroy();
}
