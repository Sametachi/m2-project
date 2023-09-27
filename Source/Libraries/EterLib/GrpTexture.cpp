#include "StdAfx.h"
#include "../eterBase/Stl.h"
#include "GrpTexture.h"
#include "StateManager.h"

void CGraphicTexture::DestroyDeviceObjects()
{
    safe_release(m_lpd3dTexture);
}

void CGraphicTexture::Destroy()
{
    DestroyDeviceObjects();

    Initialize();
}

void CGraphicTexture::Initialize()
{
    m_lpd3dTexture = nullptr;
    m_width = 0;
    m_height = 0;
    m_bEmpty = true;
}

bool CGraphicTexture::IsEmpty() const
{
    return m_bEmpty;
}

void CGraphicTexture::SetTextureStage(int32_t stage) const
{
    assert(ms_lpd3dDevice != nullptr);
    STATEMANAGER->SetTexture(stage, m_lpd3dTexture);
}

LPDIRECT3DTEXTURE9 CGraphicTexture::GetD3DTexture() const
{
    return m_lpd3dTexture;
}

int32_t CGraphicTexture::GetWidth() const
{
    return m_width;
}

int32_t CGraphicTexture::GetHeight() const
{
    return m_height;
}

void CGraphicTexture::SetWidth(int width)
{
    m_width = width;
}

void CGraphicTexture::SetHeight(int height)
{
    m_height = height;
}

bool CGraphicTexture::SetSubData(uint32_t level, uint32_t left, uint32_t top, uint32_t width, uint32_t height, uint32_t pitch, void* pData, D3DFORMAT pf)
{
    if (nullptr == m_lpd3dTexture)
    {
        return false;
    }

    IDirect3DSurface9* pSurface = 0;
    m_lpd3dTexture->GetSurfaceLevel(0, &pSurface);
    if (nullptr == pSurface)
    {
        return false;
    }

    RECT rc = { 0, 0, width, height };
    RECT drc = { left, top, width + left, height + top };
    HRESULT hr = D3DXLoadSurfaceFromMemory(pSurface, 0, &drc, pData, pf, pitch, 0, &rc, D3DX_DEFAULT, 0);
    if (FAILED(hr))
    {
        return false;
    }
    pSurface->Release();
    return false;
}

CGraphicTexture::CGraphicTexture()
{
    Initialize();
}
