#include "StdAfx.h"
#include <EterBase/Utils.h>
#include "AttributeInstance.h"
#include "GrpMath.h"

bool CAttributeInstance::Picking(const D3DXVECTOR3& v, const D3DXVECTOR3& dir, float& out_x, float& out_y)
{
	if (IsEmpty())
		return false;

	bool bPicked = false;
	float nx = 0;
	float ny = 0;

	for (uint32_t i = 0; i < m_v3HeightDataVector.size(); ++i)
	{
		for (uint32_t j = 0; j < m_v3HeightDataVector[i].size(); j += 3)
		{
			const D3DXVECTOR3& cv0 = m_v3HeightDataVector[i][j];
			const D3DXVECTOR3& cv2 = m_v3HeightDataVector[i][static_cast<std::vector<D3DXVECTOR3, std::allocator<D3DXVECTOR3>>::size_type>(j) + 1];
			const D3DXVECTOR3& cv1 = m_v3HeightDataVector[i][static_cast<std::vector<D3DXVECTOR3, std::allocator<D3DXVECTOR3>>::size_type>(j) + 2];

			const D3DXVECTOR3& vecTemp1(cv1 - cv0);
			const D3DXVECTOR3& vecTemp2(cv2 - cv0);
			const D3DXVECTOR3& vecTemp3(v - cv0);
			const D3DXVECTOR3& vecTemp4(cv1 - cv0);
			const D3DXVECTOR3& vecTemp5(cv2 - cv1);

			D3DXVECTOR3 n;
			D3DXVec3Cross(&n, &vecTemp1, &vecTemp2);
			D3DXVECTOR3 x;
			float t;
			t = -D3DXVec3Dot(&vecTemp3, &n) / D3DXVec3Dot(&dir, &n);

			x = v + t * dir;

			const D3DXVECTOR3& vecTemp6(x - cv0);
			const D3DXVECTOR3& vecTemp7(x - cv1);

			D3DXVECTOR3 temp;
			D3DXVec3Cross(&temp, &vecTemp4, &vecTemp6);
			if (D3DXVec3Dot(&temp, &n) < 0)
			{
				continue;
			}
			D3DXVec3Cross(&temp, &vecTemp5, &vecTemp7);
			if (D3DXVec3Dot(&temp, &n) < 0)
			{
				continue;
			}

			const D3DXVECTOR3& v1 = (cv0 - cv2);
			const D3DXVECTOR3& v2 = (x - cv2);
			D3DXVec3Cross(&temp, &v1, &v2);
			if (D3DXVec3Dot(&temp, &n) < 0)
			{
				continue;
			}

			if (bPicked)
			{
				if ((v.x - x.x) * (v.x - x.x) + (v.y - x.y) * (v.y - x.y) < (v.x - nx) * (v.x - nx) + (v.y - ny) * (v.y - ny))
				{
					nx = x.x;
					ny = x.y;
				}
			}
			else
			{
				nx = x.x;
				ny = x.y;
			}
			bPicked = true;
		}
	}
	if (bPicked)
	{
		out_x = nx;
		out_y = ny;
	}
	return bPicked;
}

bool CAttributeInstance::GetHeight(float fx, float fy, float* pfHeight)
{
	if (IsEmpty())
		return false;

	fy *= -1.0f;

	if (!IsInHeight(fx, fy))
		return false;

	bool bFlag = false;

	for (uint32_t i = 0; i < m_v3HeightDataVector.size(); ++i)
	{
		for (uint32_t j = 0; j < m_v3HeightDataVector[i].size(); j += 3)
		{
			const D3DXVECTOR3& c_rv3Vertex0 = m_v3HeightDataVector[i][j];
			const D3DXVECTOR3& c_rv3Vertex1 = m_v3HeightDataVector[i][static_cast<std::vector<D3DXVECTOR3, std::allocator<D3DXVECTOR3>>::size_type>(j) + 1];
			const D3DXVECTOR3& c_rv3Vertex2 = m_v3HeightDataVector[i][static_cast<std::vector<D3DXVECTOR3, std::allocator<D3DXVECTOR3>>::size_type>(j) + 2];

			if ((fx < c_rv3Vertex0.x && fx < c_rv3Vertex1.x && fx < c_rv3Vertex2.x) ||
				(fx > c_rv3Vertex0.x && fx > c_rv3Vertex1.x && fx > c_rv3Vertex2.x) ||
				(fy < c_rv3Vertex0.y && fy < c_rv3Vertex1.y && fy < c_rv3Vertex2.y) ||
				(fy > c_rv3Vertex0.y && fy > c_rv3Vertex1.y && fy > c_rv3Vertex2.y))
				continue;

			if (IsInTriangle2D(c_rv3Vertex0.x, c_rv3Vertex0.y, c_rv3Vertex1.x, c_rv3Vertex1.y, c_rv3Vertex2.x, c_rv3Vertex2.y, fx, fy))
			{
				D3DXVECTOR3 v3Line1 = c_rv3Vertex1 - c_rv3Vertex0;
				D3DXVECTOR3 v3Line2 = c_rv3Vertex2 - c_rv3Vertex0;
				D3DXVECTOR3 v3Cross;

				D3DXVec3Cross(&v3Cross, &v3Line1, &v3Line2);
				D3DXVec3Normalize(&v3Cross, &v3Cross);

				if (0.0f != v3Cross.z)
				{
					float fd = (v3Cross.x * c_rv3Vertex0.x + v3Cross.y * c_rv3Vertex0.y + v3Cross.z * c_rv3Vertex0.z);
					float fm = (v3Cross.x * fx + v3Cross.y * fy);
					*pfHeight = fMAX((fd - fm) / v3Cross.z, *pfHeight);

					bFlag = true;
				}
			}
		}
	}

	return bFlag;
}

bool CAttributeInstance::IsInHeight(float fx, float fy)
{
	float fdx = m_matGlobal._41 - fx;
	float fdy = m_matGlobal._42 - fy;
	if (sqrtf(fdx * fdx + fdy * fdy) > m_fHeightRadius)
		return false;

	return true;
}

void CAttributeInstance::SetObjectPointer(CAttributeData* pAttributeData)
{
	Clear();
	m_roAttributeData.SetPointer(pAttributeData);
}

void CAttributeInstance::RefreshObject(const D3DXMATRIX& c_rmatGlobal)
{
	assert(!m_roAttributeData.IsNull());

	m_matGlobal = c_rmatGlobal;

	// Height
	m_fHeightRadius = m_roAttributeData->GetMaximizeRadius();

	uint32_t dwHeightDataCount = m_roAttributeData->GetHeightDataCount();
	m_v3HeightDataVector.clear();
	m_v3HeightDataVector.resize(dwHeightDataCount);
	for (uint32_t i = 0; i < dwHeightDataCount; ++i)
	{
		const THeightData* c_pHeightData;
		if (!m_roAttributeData->GetHeightDataPointer(i, &c_pHeightData))
			continue;

		uint32_t dwVertexCount = c_pHeightData->v3VertexVector.size();
		m_v3HeightDataVector[i].clear();
		m_v3HeightDataVector[i].resize(dwVertexCount);
		for (uint32_t j = 0; j < dwVertexCount; ++j)
			D3DXVec3TransformCoord(&m_v3HeightDataVector[i][j], &c_pHeightData->v3VertexVector[j], &m_matGlobal);
	}
}

bool CAttributeInstance::IsEmpty() const
{
	if (!m_v3HeightDataVector.empty())
		return false;

	return true;
}

void CAttributeInstance::Clear()
{
	m_fHeightRadius = 0.0f;
	m_fCollisionRadius = 0.0f;
	D3DXMatrixIdentity(&m_matGlobal);

	m_v3HeightDataVector.clear();
	m_roAttributeData.SetPointer(nullptr);
}

CAttributeInstance::CAttributeInstance() : m_fCollisionRadius(0.0f), m_fHeightRadius(0.0f)
{
	D3DXMatrixIdentity(&m_matGlobal);
}
