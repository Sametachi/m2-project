#include "StdAfx.h"
#include "GrpImage.h"

CGraphicImage::CGraphicImage(const FilenameWrapper& filename, uint32_t dwFilter) : CResource(filename), m_dwFilter(dwFilter)
{
	m_rect.bottom = m_rect.right = m_rect.top = m_rect.left = 0;
}

bool CGraphicImage::CreateDeviceObjects()
{
	return m_imageTexture.CreateDeviceObjects();
}

void CGraphicImage::DestroyDeviceObjects()
{
	m_imageTexture.DestroyDeviceObjects();
}

CGraphicImage::TType CGraphicImage::Type()
{
	static TType s_type = StringToType("CGraphicImage");
	return s_type;
}

bool CGraphicImage::OnIsType(TType type)
{
	if (CGraphicImage::Type() == type)
		return true;

	return CResource::OnIsType(type);
}

void CGraphicImage::SetWidth(long width)
{
	m_rect.right = m_rect.left + width;
}

void CGraphicImage::SetHeight(long height)
{
	m_rect.bottom = m_rect.top + height;
}

int32_t CGraphicImage::GetWidth() const
{
	return m_rect.right - m_rect.left;
}

int32_t CGraphicImage::GetHeight() const
{
	return m_rect.bottom - m_rect.top;
}

const CGraphicTexture& CGraphicImage::GetTextureReference() const
{
	return m_imageTexture;
}

CGraphicTexture* CGraphicImage::GetTexturePointer()
{
	return &m_imageTexture;
}

const RECT& CGraphicImage::GetRectReference() const
{
	return m_rect;
}

bool CGraphicImage::OnLoad(int32_t iSize, const void* c_pvBuf)
{
	if (!c_pvBuf)
		return false;

	const auto& stRefResourceName = CResource::GetFileNameString();
	m_imageTexture.SetFileName(stRefResourceName.c_str());

	if (!m_imageTexture.CreateFromMemoryFile(iSize, c_pvBuf))
	{
		const auto& stRefResourceName = CResource::GetFileNameString();
		SysLog("CGraphicImage::OnLoad: CreateFromMemoryFile: texture not found {0}", stRefResourceName.c_str());
		return false;
	}

	m_rect.left = 0;
	m_rect.top = 0;
	m_rect.right = m_imageTexture.GetWidth();
	m_rect.bottom = m_imageTexture.GetHeight();
	return true;
}

void CGraphicImage::OnClear()
{
#ifdef USE_RESOURCE_DEBUG
	SysLog("Image Destroy : {0}\n", CResource::GetFileNameString());
#endif
	m_imageTexture.Destroy();
	memset(&m_rect, 0, sizeof(m_rect));
}

bool CGraphicImage::OnIsEmpty() const
{
	return m_imageTexture.IsEmpty();
}
