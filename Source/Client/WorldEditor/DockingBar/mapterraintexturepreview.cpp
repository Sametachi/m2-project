// mapterraintexturepreview.cpp : implementation file
//

#include "stdafx.h"
#include "..\WorldEditor.h"
#include "mapterraintexturepreview.h"
#include "../DataCtrl/MapAccessorTerrain.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include <il/ilu.h>

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview

CMapTerrainTexturePreview::CMapTerrainTexturePreview()
{
	m_ilImage = 0;
}

CMapTerrainTexturePreview::~CMapTerrainTexturePreview()
{
	if (m_ilImage)
	{
		ilDeleteImages(1, &m_ilImage);
	}

	m_ilImage = 0;
}

BEGIN_MESSAGE_MAP(CMapTerrainTexturePreview, CWnd)
	//{{AFX_MSG_MAP(CMapTerrainTexturePreview)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview message handlers

BOOL CMapTerrainTexturePreview::Create()
{
	CRect rect;
	GetClientRect(&rect);
	if (!m_DIB.Create(NULL, rect.Width(), rect.Height()))
		return FALSE;

	ilGenImages(1, &m_ilImage);
	ilBindImage(m_ilImage);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview message handlers

void CMapTerrainTexturePreview::UpdatePreview(int32_t nTexNum)
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	CTextureSet * pTextureSet = CTerrain::GetTextureSet();

	if (nTexNum == 0)
	{
		ilClearImage();
	}
	else
	{
		CGraphicImageInstance & rImageInstance = pTextureSet->GetTexture(nTexNum).ImageInstance;
		CGraphicImage * pImage = rImageInstance.GetGraphicImagePointer();
		if (CResourceManager::Instance().IsFileExist(pImage->GetFileName())) // @fixme123
		{
			ilLoadImage((const ILstring)pImage->GetFileName());
			iluScale(m_DIB.GetWidth(), m_DIB.GetHeight(), 1);
		}
		else
			TraceError("The file [%s] doesn't exist", pImage->GetFileName());
		Invalidate(TRUE);
	}
}

void CMapTerrainTexturePreview::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	uint32_t * pdwDst = (uint32_t *)m_DIB.GetPointer();

	if (ilIsImage(m_ilImage))
	{
		uint32_t * pdwSrc = (uint32_t *)ilGetData();
		int32_t iwidth = ilGetInteger(IL_IMAGE_WIDTH);
		int32_t iheight = ilGetInteger(IL_IMAGE_HEIGHT);
		int32_t iBPP = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

		for (int32_t y = 0; y < iheight; ++y)
		{
			for (int32_t x = 0; x < iwidth; ++x)
			{
				uint32_t dwColor = pdwSrc[x+y*iwidth];
				uint8_t byr = dwColor&0xff;
				uint8_t byg = (dwColor>>8)&0xff;
				uint8_t byb = (dwColor>>16)&0xff;
				dwColor = uint32_t(0xff << 24) | uint32_t(byr << 16) | uint32_t(byg << 8) | uint32_t(byb);
				pdwDst[x+y*m_DIB.GetWidth()] = dwColor;
			}
		}
	}
	else
	{
		memset(pdwDst, 0, m_DIB.GetWidth()*m_DIB.GetHeight()*4);
	}

	m_DIB.Put(dc.GetSafeHdc(), 0, 0);
}
