#include "StdAfx.h"
#include "GraphCtrl.h"

void CGraphCtrl::Update()
{
}

void CGraphCtrl::GraphBegin()
{
	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_PROJECTION, &matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_VIEW, &matIdentity);
	STATEMANAGER.SaveTransform(D3DTS_WORLD, &matIdentity);
	SetOrtho2D(1024.0f, 768.0f, 400.0f);
	SetClearColor(1.0f, 1.0f, 1.0f);
	Clear();
	Begin();
}

void CGraphCtrl::GraphEnd(RECT * pRect, HWND hWnd)
{
	End();
	Show(pRect, hWnd);
	STATEMANAGER.RestoreTransform(D3DTS_WORLD);
	STATEMANAGER.RestoreTransform(D3DTS_VIEW);
	STATEMANAGER.RestoreTransform(D3DTS_PROJECTION);
}

void CGraphCtrl::Render()
{
	RenderGrid();
	RenderGraph();
}

void CGraphCtrl::RenderGraph()
{
	if (!m_pAccessor)
		return;

	uint32_t dwCount = m_pAccessor->GetValueCount();
	m_PointVector.clear();
	m_PointVector.resize(dwCount);

	uint32_t i;
	float fTime;
	uint32_t dwValue;

	int32_t ix, iy;
	int32_t iBoxHalfSize = 3;

	SetDiffuseColor(1.0f, 0.0f, 0.0f);
	for (i = 0; i < dwCount; ++i)
	{
		m_pAccessor->GetTime(i, &fTime);
		m_pAccessor->GetValue(i, &dwValue);
		TimeToScreen(fTime, &ix);
		iy = dwValue;

		if (ix >= m_ixTemporary)
		if (ix <= m_iWidth)
		{
			RenderBox2d(ix - iBoxHalfSize, iy - iBoxHalfSize,
						ix + iBoxHalfSize, iy + iBoxHalfSize);

			if (m_dwSelectedIndex == i || m_dwGrippedIndex == i)
			{
				RenderBar2d(ix - iBoxHalfSize, iy - iBoxHalfSize,
							ix + iBoxHalfSize, iy + iBoxHalfSize);
			}
		}

		m_PointVector[i].ix = ix;
		m_PointVector[i].iy = iy;
	}

	// Render Line
	SetDiffuseColor(0.0f, 0.0f, 0.0f);
	if (dwCount)
	switch (m_iGraphType)
	{
		case GRAPH_TYPE_BRIDGE:
			if (dwCount > 1)
			for (i = 0; i < dwCount-1; i+=2)
			{
				ConnectPoint(m_PointVector[i], m_PointVector[i+1]);
			}
			break;

		case GRAPH_TYPE_LINEAR:
			RenderLine2d(m_ixTemporary, m_PointVector[0].iy, m_PointVector[0].ix, m_PointVector[0].iy);
			for (i = 0; i < dwCount-1; ++i)
			{
				ConnectPoint(m_PointVector[i], m_PointVector[i+1]);
			}
			if (dwCount > 1)
				RenderLine2d(m_PointVector[dwCount-1].ix, m_PointVector[dwCount-1].iy, m_iWidth, m_PointVector[dwCount-1].iy);
			break;

		case GRAPH_TYPE_BLOCK:
			RenderLine2d(m_ixTemporary, m_PointVector[0].iy, m_PointVector[0].ix, m_PointVector[0].iy);
			for (i = 0; i < dwCount-1; ++i)
			{
				TPoint vec = m_PointVector[i+1];
				vec.iy = m_PointVector[i].iy;
				ConnectPoint(m_PointVector[i], vec);
				ConnectPoint(m_PointVector[i+1], vec);
			}
			if (dwCount > 1)
				RenderLine2d(m_PointVector[dwCount-1].ix, m_PointVector[dwCount-1].iy, m_iWidth, m_PointVector[dwCount-1].iy);

			break;
	}
}

void CGraphCtrl::RenderTimeLine(float fTime)
{
	int32_t ix;
	TimeToScreen(fTime, &ix);

	SetDiffuseColor(0.3f, 0.7f, 0.3f);
	RenderBox2d(ix-2, 0, ix+2, m_iHeight - m_iyTemporary + 5);
}

void CGraphCtrl::RenderEndLine(float fTime)
{
	int32_t ix;
	TimeToScreen(fTime, &ix);

	SetDiffuseColor(0.7f, 0.3f, 0.3f);
	RenderLine2d(ix-1, 20, ix-1, m_iHeight - m_iyTemporary - 20);
	RenderLine2d(ix+1, 20, ix+1, m_iHeight - m_iyTemporary - 20);
}

void CGraphCtrl::RenderGrid()
{
	SetDiffuseColor(0.8f, 0.8f, 0.8f);
	for (int32_t x = 0; x < m_ixGridCount; ++x)
	{
		int32_t xPos = m_ixTemporary + x * m_ixGridStep;
		RenderLine2d(xPos, 0, xPos, m_iHeight - m_iyTemporary + 5);
	}
	for (int32_t y = 0; y < m_iyGridCount; ++y)
	{
		int32_t yPos = m_iHeight - (m_iyGridCount - y - 1) * m_iyGridStep - m_iyTemporary;
		RenderLine2d(m_ixTemporary - 5, yPos, m_iWidth, yPos);
	}

	for (uint32_t dwTimeCount = 0; dwTimeCount < m_HorizontalTextLine.size(); ++dwTimeCount)
	{
		CGraphicTextInstance * pInstance = m_HorizontalTextLine[dwTimeCount];

		//pInstance->Render();
	}
}

void CGraphCtrl::ConnectPoint(TPoint & rLeft, TPoint & rRight)
{
	if (rRight.ix < m_ixTemporary)
		return;
	if (rLeft.ix > m_iWidth)
		return;

	RenderLine2d(max(m_ixTemporary, rLeft.ix), rLeft.iy,
				 min(m_iWidth, rRight.ix), rRight.iy);
}

void CGraphCtrl::SetSize(int32_t iWidth, int32_t iHeight)
{
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	m_ixTemporary = 30;
	m_iyTemporary = 20;
	m_ixGridStep = 25;
	m_iyGridStep = 13;
	m_ixGridCount = m_iWidth / m_ixGridStep;
	m_iyGridCount = m_iHeight / m_iyGridStep;

	BuildHorizontalLine(m_ixGridCount);
	UpdateHorizontalLine(0, m_iHorizontalZoom);
}

void CGraphCtrl::Move(float fx, float fy)
{
	m_fxPosition += fx;
	m_fyPosition += fy;

	m_fxPosition = fMAX(0, m_fxPosition);

	UpdateHorizontalLine(int32_t(m_fxPosition), m_iHorizontalZoom);
}

void CGraphCtrl::UpdateHorizontalLine(int32_t iStartPosition, int32_t iZoom)
{
	char szTime[16+1];
	for (uint32_t i = 0; i < m_HorizontalTextLine.size(); ++i)
	{
		CGraphicTextInstance * pInstance = m_HorizontalTextLine[i];

		int32_t ix = m_ixTemporary + i*m_ixGridStep;
		int32_t iy = m_iHeight - 13;

		_snprintf(szTime, 16, "%d", (iStartPosition + i) * iZoom);
		pInstance->SetValue(szTime);
		pInstance->SetPosition(ix, iy);
		pInstance->Update();
	}
}

void CGraphCtrl::BuildHorizontalLine(int32_t iCount)
{
	for (uint32_t k = 0; k < m_HorizontalTextLine.size(); ++k)
	{
		m_textInstancePool.Free(m_HorizontalTextLine[k]);
	}

	m_HorizontalTextLine.clear();
	m_HorizontalTextLine.resize(iCount);

	CResource* pResource = CResourceManager::Instance().GetTypeResourcePointer("Tahoma:11.fnt");

	for (int32_t i = 0; i < iCount; ++i)
	{
		CGraphicTextInstance * pInstance = m_textInstancePool.Alloc();

		pInstance->SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_CENTER);
		pInstance->SetTextPointer(static_cast<CGraphicText*>(pResource));
		pInstance->SetColor(0.0f, 0.0f, 0.0f);

		m_HorizontalTextLine[i] = pInstance;
	}
}

void CGraphCtrl::TimeToScreen(float fTime, int32_t * px)
{
	float fStartTime = floor(m_fxPosition) / 10.0f * m_iHorizontalZoom;
	*px = (fTime - fStartTime) * 1000.0f / float(m_iHorizontalZoom * 100) * float(m_ixGridStep) + m_ixTemporary;
}

void CGraphCtrl::ScreenToTime(uint32_t ix, float * pTime)
{
	int32_t ixReal = ix + floor(m_fxPosition) * float(m_ixGridStep);
	*pTime = float(ixReal - m_ixTemporary) / float(m_ixGridStep) * float(m_iHorizontalZoom * 100) / 1000.0f;
}

void CGraphCtrl::SetGraphType(int32_t iType)
{
	m_iGraphType = iType;
}
void CGraphCtrl::SetValueType(int32_t iType)
{
	m_iValueType = iType;
}

void CGraphCtrl::SetAccessorPointer(IValueAccessor * pAccessor)
{
	m_dwSelectedIndex = POINT_NONE;
	m_pAccessor = pAccessor;
}

void CGraphCtrl::ZoomInHorizon()
{
	if (m_iHorizontalZoom > HORIZON_ZOOMING_MIN)
		m_iHorizontalZoom -= 1;

	UpdateHorizontalLine(int32_t(m_fxPosition), m_iHorizontalZoom);
}
void CGraphCtrl::ZoomOutHorizon()
{
	if (m_iHorizontalZoom < HORIZON_ZOOMING_MAX)
		m_iHorizontalZoom += 1;

	UpdateHorizontalLine(int32_t(m_fxPosition), m_iHorizontalZoom);
}

void CGraphCtrl::OnMouseMove(int32_t ix, int32_t iy)
{
	// Check Selecting
	//m_dwSelectedIndex = POINT_NONE;

	for (uint32_t i = 0; i < m_PointVector.size(); ++i)
	{
		TPoint & rPoint = m_PointVector[i];

		if (abs(ix - rPoint.ix) < 5)
		if (abs(iy - rPoint.iy) < 5)
		{
			m_dwSelectedIndex = i;
		}
	}

	// Move gripped point
	float fTime;

	if (POINT_NONE != m_dwGrippedIndex)
	if (m_dwGrippedIndex < m_PointVector.size())
	{
		TPoint & rPoint = m_PointVector[m_dwGrippedIndex];

		if (m_dwGrippedIndex > 0)
		{
			if (m_PointVector[m_dwGrippedIndex-1].ix + 10 > ix)
				ix = m_PointVector[m_dwGrippedIndex-1].ix + 10;
		}
		if (m_dwGrippedIndex < m_PointVector.size()-1)
		{
			if (m_PointVector[m_dwGrippedIndex+1].ix - 10 < ix)
				ix = m_PointVector[m_dwGrippedIndex+1].ix - 10;
		}

		ScreenToTime(ix, &fTime);

		if (fTime < 0.0f)
			fTime = 0.0f;

		uint32_t fValue;
		m_pAccessor->GetValue(m_dwGrippedIndex, &fValue);

		m_pAccessor->SetTime(m_dwGrippedIndex, fTime);
		m_pAccessor->SetValue(m_dwGrippedIndex, iy<0?0:iy);
	}
}

void CGraphCtrl::OnLButtonDown(int32_t ix, int32_t iy)
{
	/*
	if (POINT_NONE != m_dwSelectedIndex)
	{
		m_dwGrippedIndex = m_dwSelectedIndex;
	}
	*/

	m_dwGrippedIndex = POINT_NONE;

	for (uint32_t i = 0; i < m_PointVector.size(); ++i)
	{
		TPoint & rPoint = m_PointVector[i];

		if (abs(ix - rPoint.ix) < 5)
		if (abs(iy - rPoint.iy) < 5)
		{
			m_dwGrippedIndex = i;
		}
	}

	if (m_dwGrippedIndex==POINT_NONE && isCreatingMode())
	{
		float fTime;
		ScreenToTime(ix, &fTime);

		m_pAccessor->Insert(fTime, 0);

		for (uint32_t i = 0; i < m_pAccessor->GetValueCount(); ++i)
		{
			float time;
			m_pAccessor->GetTime(i,&time);

			if (fabs(time-fTime)<=0.001f)
			{
				m_dwGrippedIndex = i;
			}
		}

	}
}
void CGraphCtrl::OnLButtonUp(int32_t ix, int32_t iy)
{
	m_dwGrippedIndex = -1;
}
void CGraphCtrl::OnRButtonDown(int32_t ix, int32_t iy)
{
	if (isCreatingMode())
	if (POINT_NONE != m_dwSelectedIndex)
	{
		m_pAccessor->Delete(m_dwSelectedIndex);
		m_dwSelectedIndex = POINT_NONE;
	}
}
void CGraphCtrl::OnRButtonUp(int32_t ix, int32_t iy)
{
}

void CGraphCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch(nChar) {
	case VK_LEFT:
		if (m_dwSelectedIndex!=POINT_NONE && m_dwSelectedIndex>0)
			m_dwSelectedIndex--;
		break;
	case VK_RIGHT:
		if (m_dwSelectedIndex!=POINT_NONE && m_dwSelectedIndex+1<m_PointVector.size())
			m_dwSelectedIndex++;
		break;
	default:
		break;
	}
}


bool CGraphCtrl::isCreatingMode()
{
	return (GetAsyncKeyState(VK_LCONTROL) & (int16_t)(1<<15)) != 0 ? true : false;
}

void CGraphCtrl::Initialize()
{
	CScreen::Clear();
	m_dwSelectedIndex = POINT_NONE;
}

CGraphCtrl::CGraphCtrl()
{
	m_iGraphType = GRAPH_TYPE_LINEAR;
	m_iValueType = VALUE_TYPE_CENTER;

	m_pAccessor = NULL;

	m_dwSelectedIndex = POINT_NONE;
	m_dwGrippedIndex = POINT_NONE;

	m_PointVector.clear();

	m_iWidth = 0;
	m_iHeight = 0;

	m_fxPosition = 0.0f;
	m_fyPosition = 0.0f;
	m_iHorizontalZoom = 2;

	m_ixTemporary = 0;
	m_iyTemporary = 0;
	m_ixGridStep = 0;
	m_iyGridStep = 0;
	m_ixGridCount = 0;
	m_iyGridCount = 0;
}
CGraphCtrl::~CGraphCtrl()
{
}


/*
	for (uint32_t j = 0; j < m_VerticalTextLine.size(); ++j)
	{
		CGraphicTextInstance * pInstance = m_VerticalTextLine[j];

		int32_t xPos = 0;
		int32_t yPos = m_iHeight - (j * m_iyGridStep) - m_iyTemporary - 5;

		if (j < 10)
			xPos = 17;
		else if(j < 100)
			xPos = 12;
		else if(j < 1000)
			xPos = 7;
		else if(j < 10000)
			xPos = 2;

		pInstance->SetPosition(xPos, yPos, 0.0f);
		pInstance->Update();
	}
*/

uint32_t CGraphCtrl::GetSelectedIndex()
{
	return m_dwSelectedIndex;
}

void CGraphCtrl::SetSelectedIndex(uint32_t dwSelectedIndex)
{
	m_dwSelectedIndex = dwSelectedIndex;
	if (m_dwSelectedIndex>=m_PointVector.size())
		m_dwSelectedIndex = m_PointVector.size()-1;
}
