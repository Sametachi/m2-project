#include "StdAfx.h"
#include <VFE/Include/VFE.hpp>
#include "GrpImageTexture.h"
#include <d3dx9tex.h>
#include "../EterImageLib/DDSTextureLoader.h"
#include "../EterImageLib/WICTextureLoader9.h"
#include "../EterImageLib/dds.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../EterImageLib/stb_image.h"

bool CGraphicImageTexture::Lock(int32_t* pRetPitch, void** ppRetPixels, int32_t level)
{
	D3DLOCKED_RECT lockedRect;
	if (FAILED(m_lpd3dTexture->LockRect(level, &lockedRect, nullptr, 0)))
		return false;

	*pRetPitch = lockedRect.Pitch;
	*ppRetPixels = (void*)lockedRect.pBits;
	return true;
}

void CGraphicImageTexture::Unlock(int32_t level)
{
	assert(m_lpd3dTexture != nullptr);
	m_lpd3dTexture->UnlockRect(level);
}

void CGraphicImageTexture::Initialize()
{
	CGraphicTexture::Initialize();

	m_stFileName = "";

	m_d3dFmt = D3DFMT_UNKNOWN;
}

void CGraphicImageTexture::Destroy()
{
	CGraphicTexture::Destroy();

	Initialize();
}

bool CGraphicImageTexture::CreateDeviceObjects()
{
	assert(ms_lpd3dDevice != nullptr);
	assert(m_lpd3dTexture == nullptr);

	if (m_stFileName.empty())
	{
		if (FAILED(ms_lpd3dDevice->CreateTexture(m_width, m_height, 1, 0, m_d3dFmt, D3DPOOL_MANAGED, &m_lpd3dTexture, nullptr)))
			return false;
	}
	else
	{
		auto vfs = CallFS().Open(m_stFileName.c_str(), Buffered);

		if (!vfs)
			return NULL;

		const uint32_t size = vfs->GetSize();

		storm::View data(storm::GetDefaultAllocator());
		vfs->GetView(0, data, size);

		return CreateFromMemoryFile(size, data.GetData());
	}

	m_bEmpty = false;
	return true;
}

bool CGraphicImageTexture::Create(uint32_t width, uint32_t height, D3DFORMAT d3dFmt)
{
	assert(ms_lpd3dDevice != nullptr);
	Destroy();

	m_width = width;
	m_height = height;
	m_d3dFmt = d3dFmt;

	return CreateDeviceObjects();
}

void CGraphicImageTexture::CreateFromTexturePointer(const CGraphicTexture* c_pSrcTexture)
{
	if (m_lpd3dTexture)
		m_lpd3dTexture->Release();

	m_width = c_pSrcTexture->GetWidth();
	m_height = c_pSrcTexture->GetHeight();
	m_lpd3dTexture = c_pSrcTexture->GetD3DTexture();

	if (m_lpd3dTexture)
		m_lpd3dTexture->AddRef();

	m_bEmpty = false;
}

bool CGraphicImageTexture::CreateWithStbImage(uint32_t bufSize, const void* c_pvBuf)
{
	struct free_deleter
	{
		void operator()(void* p) const { free(p); }
	};

	using c_ptr = std::unique_ptr<uint8_t[], free_deleter>;

	IDirect3DTexture9* ret;
	auto* data_uc = static_cast<const stbi_uc*>(c_pvBuf);

	int32_t Width{}, Height{}, comp{};
	int32_t Levels = 1;
	int32_t Usage = 0;

	c_ptr tex_mem = c_ptr{ stbi_load_from_memory(data_uc, bufSize, &Width, &Height, &comp, STBI_rgb_alpha) };

	uint8_t* tex_ptr = tex_mem.get();

	if (!(Width && Height && comp && tex_mem))
	{
		m_bEmpty = true;
		return false;
	}

	D3DFORMAT format;
	if (comp == 4)
		format = D3DFMT_A8R8G8B8;
	else
		format = D3DFMT_X8R8G8B8;

	if (FAILED(ms_lpd3dDevice->CreateTexture(Width, Height, Levels, Usage, format, D3DPOOL_MANAGED, &ret, nullptr)))
		return false;

	D3DLOCKED_RECT LockedRect;
	if (FAILED(ret->LockRect(0, &LockedRect, nullptr, 0)))
	{
		return false;
	}
	uint8_t* ptr = static_cast<uint8_t*>(LockedRect.pBits);
	for (int32_t i{}; i < Width * Height; ++i)
	{
#define SET(offset)                                                                                                    \
    *ptr = *(tex_ptr + offset);                                                                                        \
    ++ptr
		// RGBA -> BGRA
		SET(2);
		SET(1);
		SET(0);
		SET(3);
		tex_ptr += 4;
#undef SET
	}
	if (FAILED(ret->UnlockRect(0)))
	{
		m_bEmpty = true;
		return false;
	}

	m_width = Width;
	m_height = Height;
	m_d3dFmt = format;
	m_bEmpty = false;
	m_lpd3dTexture = ret;
	return true;
}

bool CGraphicImageTexture::CreateFromMemoryFile(uint32_t bufSize, const void* c_pvBuf)
{
	assert(ms_lpd3dDevice != nullptr);
	assert(m_lpd3dTexture == nullptr);

	std::string FileNameCheck = m_stFileName.c_str();

	if (!m_stFileName.empty() && CFileNameHelper::GetExtension(FileNameCheck) == "dds" || CFileNameHelper::GetExtension(FileNameCheck) == "DDS")
	{
		HRESULT hr = DirectX::CreateDDSTextureFromMemoryEx(ms_lpd3dDevice, (const uint8_t*)c_pvBuf, bufSize, 0, D3DPOOL_MANAGED, false, &m_lpd3dTexture);
		if (FAILED(hr))
		{
			D3DXIMAGE_INFO imageInfo;
			hr = D3DXCreateTextureFromFileInMemoryEx(ms_lpd3dDevice, c_pvBuf, bufSize, D3DX_DEFAULT_NONPOW2, D3DX_DEFAULT_NONPOW2, 1, 0, m_d3dFmt, D3DPOOL_MANAGED, D3DX_FILTER_NONE, D3DX_FILTER_NONE, 0xffff00ff, &imageInfo, nullptr, &m_lpd3dTexture);
			if (FAILED(hr))
			{
				SysLog("{0}: failed to create texture with {1}", m_stFileName.c_str(), (hr));
				return false;
			}
			m_bEmpty = false;

			m_width = imageInfo.Width;
			m_height = imageInfo.Height;
			return true;
		}

		D3DSURFACE_DESC desc;
		m_lpd3dTexture->GetLevelDesc(0, &desc);

		m_width = desc.Width;
		m_height = desc.Height;
		m_d3dFmt = desc.Format;
		m_bEmpty = false;
		return true;
	}
	else
	{
		if (!CreateWithStbImage(bufSize, c_pvBuf))
		{
			HRESULT hr = DirectX::CreateWICTextureFromMemoryEx(ms_lpd3dDevice, (const uint8_t*)c_pvBuf, bufSize, 0, 0, D3DPOOL_MANAGED, 0, &m_lpd3dTexture);
			if (hr == S_OK)
			{
				D3DSURFACE_DESC desc;
				m_lpd3dTexture->GetLevelDesc(0, &desc);
				m_bEmpty = false;
				m_width = desc.Width;
				m_height = desc.Height;
				m_d3dFmt = desc.Format;
				return true;
			}
			else
			{
				m_bEmpty = true;
				SysLog("{0}: failed to create texture with {1}", m_stFileName.c_str(), std::system_category().message(hr));
				return false;
			}
		}
	}

	m_bEmpty = false;
	return true;
}

void CGraphicImageTexture::SetFileName(const std::string& c_szFileName) { m_stFileName = c_szFileName; }

bool CGraphicImageTexture::CreateFromDiskFile(const char* c_szFileName, D3DFORMAT d3dFmt)
{
	Destroy();
	SetFileName(c_szFileName);

	m_d3dFmt = d3dFmt;
	return CreateDeviceObjects();
}

CGraphicImageTexture::CGraphicImageTexture() { CGraphicImageTexture::Initialize(); }
CGraphicImageTexture::~CGraphicImageTexture() { Destroy(); }
