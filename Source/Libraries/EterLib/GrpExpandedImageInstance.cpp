#include "StdAfx.h"
#include "GrpExpandedImageInstance.h"
#include "StateManager.h"

void CGraphicExpandedImageInstance::OnRender()
{
	D3DPERF_BeginEvent(D3DCOLOR_ARGB(255, 0, 255, 0), L"** CGraphicExpandedImageInstance::OnRender **");
	CGraphicImage* pImage = m_roImage.GetPointer();
	CGraphicTexture* pTexture = pImage->GetTexturePointer();

	const RECT& c_rRect = pImage->GetRectReference();
	float texReverseWidth = 1.0f / float(pTexture->GetWidth());
	float texReverseHeight = 1.0f / float(pTexture->GetHeight());
	float su = (c_rRect.left - m_RenderingRect.left) * texReverseWidth;
	float sv = (c_rRect.top - m_RenderingRect.top) * texReverseHeight;
	float eu = (c_rRect.left + m_RenderingRect.right + (c_rRect.right - c_rRect.left)) * texReverseWidth;
	float ev = (c_rRect.top + m_RenderingRect.bottom + (c_rRect.bottom - c_rRect.top)) * texReverseHeight;

	TPDTVertex vertices[4];
	vertices[0].position.x = m_v2Position.x - 0.5f;
	vertices[0].position.y = m_v2Position.y - 0.5f;
	vertices[0].position.z = m_fDepth;
	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[0].diffuse = m_DiffuseColor;

	vertices[1].position.x = m_v2Position.x - 0.5f;
	vertices[1].position.y = m_v2Position.y - 0.5f;
	vertices[1].position.z = m_fDepth;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[1].diffuse = m_DiffuseColor;

	vertices[2].position.x = m_v2Position.x - 0.5f;
	vertices[2].position.y = m_v2Position.y - 0.5f;
	vertices[2].position.z = m_fDepth;
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[2].diffuse = m_DiffuseColor;

	vertices[3].position.x = m_v2Position.x - 0.5f;
	vertices[3].position.y = m_v2Position.y - 0.5f;
	vertices[3].position.z = m_fDepth;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);
	vertices[3].diffuse = m_DiffuseColor;

	if (0.0f == m_fRotation)
	{
		float fimgWidth = float(pImage->GetWidth()) * m_v2Scale.x;
		float fimgHeight = float(pImage->GetHeight()) * m_v2Scale.y;

		vertices[0].position.x -= m_RenderingRect.left;
		vertices[0].position.y -= m_RenderingRect.top;
		vertices[1].position.x += fimgWidth + m_RenderingRect.right;
		vertices[1].position.y -= m_RenderingRect.top;
		vertices[2].position.x -= m_RenderingRect.left;
		vertices[2].position.y += fimgHeight + m_RenderingRect.bottom;
		vertices[3].position.x += fimgWidth + m_RenderingRect.right;
		vertices[3].position.y += fimgHeight + m_RenderingRect.bottom;
		if ((0.0f < m_v2Scale.x && 0.0f > m_v2Scale.y) || (0.0f > m_v2Scale.x && 0.0f < m_v2Scale.y))
		{
			STATEMANAGER->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		}
	}
	else
	{
		float fimgHalfWidth = float(pImage->GetWidth()) / 2.0f * m_v2Scale.x;
		float fimgHalfHeight = float(pImage->GetHeight()) / 2.0f * m_v2Scale.y;

		for (int32_t i = 0; i < 4; ++i)
		{
			vertices[i].position.x += m_v2Origin.x;
			vertices[i].position.y += m_v2Origin.y;
		}

		float fRadian = D3DXToRadian(m_fRotation);
		vertices[0].position.x += (-fimgHalfWidth * cosf(fRadian)) - (-fimgHalfHeight * sinf(fRadian));
		vertices[0].position.y += (-fimgHalfWidth * sinf(fRadian)) + (-fimgHalfHeight * cosf(fRadian));
		vertices[1].position.x += (+fimgHalfWidth * cosf(fRadian)) - (-fimgHalfHeight * sinf(fRadian));
		vertices[1].position.y += (+fimgHalfWidth * sinf(fRadian)) + (-fimgHalfHeight * cosf(fRadian));
		vertices[2].position.x += (-fimgHalfWidth * cosf(fRadian)) - (+fimgHalfHeight * sinf(fRadian));
		vertices[2].position.y += (-fimgHalfWidth * sinf(fRadian)) + (+fimgHalfHeight * cosf(fRadian));
		vertices[3].position.x += (+fimgHalfWidth * cosf(fRadian)) - (+fimgHalfHeight * sinf(fRadian));
		vertices[3].position.y += (+fimgHalfWidth * sinf(fRadian)) + (+fimgHalfHeight * cosf(fRadian));
	}

	switch (m_iRenderingMode)
	{
	case RENDERING_MODE_SCREEN:
	case RENDERING_MODE_COLOR_DODGE:
		STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_INVDESTCOLOR);
		STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
		break;
	case RENDERING_MODE_MODULATE:
		STATEMANAGER->SaveRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
		STATEMANAGER->SaveRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
		break;
	}

	STATEMANAGER->SaveSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	STATEMANAGER->SaveSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER->SetTexture(0, pTexture->GetD3DTexture());
		STATEMANAGER->SetTexture(1, nullptr);
		STATEMANAGER->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
		STATEMANAGER->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
	}

	STATEMANAGER->RestoreSamplerState(0, D3DSAMP_MINFILTER);
	STATEMANAGER->RestoreSamplerState(0, D3DSAMP_MAGFILTER);

	switch (m_iRenderingMode)
	{
	case RENDERING_MODE_SCREEN:
	case RENDERING_MODE_COLOR_DODGE:
	case RENDERING_MODE_MODULATE:
		STATEMANAGER->RestoreRenderState(D3DRS_SRCBLEND);
		STATEMANAGER->RestoreRenderState(D3DRS_DESTBLEND);
		break;
	}
	STATEMANAGER->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	D3DPERF_EndEvent();
}

void CGraphicExpandedImageInstance::SetDepth(float fDepth)
{
	m_fDepth = fDepth;
}

void CGraphicExpandedImageInstance::SetOrigin()
{
	SetOrigin(float(GetWidth()) / 2.0f, float(GetHeight()) / 2.0f);
}

void CGraphicExpandedImageInstance::SetSize(long width, long height)
{
	CGraphicImage* pImage = m_roImage.GetPointer();
	CGraphicTexture* pTexture = pImage->GetTexturePointer();

	if (pTexture)
	{
		pTexture->SetWidth(width);
		pTexture->SetHeight(height);
	}

	if (pImage)
	{
		pImage->SetWidth(width);
		pImage->SetHeight(height);
	}
}

void CGraphicExpandedImageInstance::SetOrigin(float fx, float fy)
{
	m_v2Origin.x = fx;
	m_v2Origin.y = fy;
}

void CGraphicExpandedImageInstance::SetRotation(float fRotation)
{
	m_fRotation = fRotation;
}

void CGraphicExpandedImageInstance::SetScale(float fx, float fy)
{
	m_v2Scale.x = fx;
	m_v2Scale.y = fy;
}

void CGraphicExpandedImageInstance::SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
{
	if (IsEmpty())
		return;

	auto fWidth = float(GetWidth());
	auto fHeight = float(GetHeight());

	m_RenderingRect.left = fWidth * fLeft;
	m_RenderingRect.top = fHeight * fTop;
	m_RenderingRect.right = fWidth * fRight;
	m_RenderingRect.bottom = fHeight * fBottom;
}

void CGraphicExpandedImageInstance::SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom)
{
	if (IsEmpty())
	{
		return;
	}

	float fWidth = float(GetWidth()) * m_v2Scale.x;
	float fHeight = float(GetHeight()) * m_v2Scale.y;

	m_RenderingRect.left = fWidth * fLeft;
	m_RenderingRect.top = fHeight * fTop;
	m_RenderingRect.right = fWidth * fRight;
	m_RenderingRect.bottom = fHeight * fBottom;
}

void CGraphicExpandedImageInstance::SetRenderingMode(int32_t iMode)
{
	m_iRenderingMode = iMode;
}

uint32_t CGraphicExpandedImageInstance::Type()
{
	static uint32_t s_dwType = GetCRC32("CGraphicExpandedImageInstance", strlen("CGraphicExpandedImageInstance"));
	return (s_dwType);
}

void CGraphicExpandedImageInstance::OnSetImagePointer()
{
	if (IsEmpty())
		return;

	SetOrigin(float(GetWidth()) / 2.0f, float(GetHeight()) / 2.0f);
}

BOOL CGraphicExpandedImageInstance::OnIsType(uint32_t dwType)
{
	if (CGraphicExpandedImageInstance::Type() == dwType)
		return true;

	return CGraphicImageInstance::IsType(dwType);
}

void CGraphicExpandedImageInstance::Destroy()
{
	CGraphicImageInstance::Destroy();
}

CGraphicExpandedImageInstance::~CGraphicExpandedImageInstance()
{
	CGraphicExpandedImageInstance::Destroy();
}
