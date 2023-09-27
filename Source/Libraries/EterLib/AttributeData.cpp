#include "StdAfx.h"
#include <EterBase/Utils.h>
#include "AttributeData.h"

const char c_szAttributeDataFileHeader[] = "AttributeData";
const int32_t c_iAttributeDataFileHeaderLength = 13;

bool CAttributeData::GetHeightDataPointer(uint32_t dwIndex, const THeightData** c_ppHeightData) const
{
	if (dwIndex >= GetHeightDataCount())
		return false;

	*c_ppHeightData = &m_HeightDataVector[dwIndex];

	return true;
}

size_t CAttributeData::AddCollisionData(const CStaticCollisionData& data)
{
	m_StaticCollisionDataVector.emplace_back(data);
	return m_StaticCollisionDataVector.size();
}

bool CAttributeData::OnLoad(int32_t iSize, const void* c_pvBuf)
{
	if (!c_pvBuf)
		return true;

	const uint8_t* c_pbBuf = static_cast<const uint8_t*>(c_pvBuf);

	char szHeader[c_iAttributeDataFileHeaderLength + 1] = { 0 };
	std::memcpy(szHeader, c_pbBuf, static_cast<size_t>(c_iAttributeDataFileHeaderLength) + 1);
	c_pbBuf += c_iAttributeDataFileHeaderLength + 1;
	if (_strcmpi(szHeader, c_szAttributeDataFileHeader))
		return false;

	uint32_t dwCollisionDataCount;
	uint32_t dwHeightDataCount;
	std::memcpy(&dwCollisionDataCount, c_pbBuf, sizeof(uint32_t));
	c_pbBuf += sizeof(uint32_t);
	std::memcpy(&dwHeightDataCount, c_pbBuf, sizeof(uint32_t));
	c_pbBuf += sizeof(uint32_t);

	m_StaticCollisionDataVector.clear();
	m_StaticCollisionDataVector.resize(dwCollisionDataCount);
	m_HeightDataVector.clear();
	m_HeightDataVector.resize(dwHeightDataCount);

	for (uint32_t i = 0; i < dwCollisionDataCount; ++i)
	{
		CStaticCollisionData& rCollisionData = m_StaticCollisionDataVector[i];
		std::memcpy(&rCollisionData.dwType, c_pbBuf, sizeof(uint32_t));
		c_pbBuf += sizeof(uint32_t);
		std::memcpy(rCollisionData.szName, c_pbBuf, 32);
		c_pbBuf += 32;
		std::memcpy(&rCollisionData.v3Position, c_pbBuf, sizeof(D3DXVECTOR3));
		c_pbBuf += sizeof(D3DXVECTOR3);

		switch (rCollisionData.dwType)
		{
		case COLLISION_TYPE_PLANE:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, 2 * sizeof(float));
			c_pbBuf += 2 * sizeof(float);
			break;

		case COLLISION_TYPE_BOX:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, 3 * sizeof(float));
			c_pbBuf += 3 * sizeof(float);
			break;

		case COLLISION_TYPE_SPHERE:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, sizeof(float));
			c_pbBuf += sizeof(float);
			break;

		case COLLISION_TYPE_CYLINDER:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, 2 * sizeof(float));
			c_pbBuf += 2 * sizeof(float);
			break;

		case COLLISION_TYPE_AABB:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, 3 * sizeof(float));
			c_pbBuf += 3 * sizeof(float);
			break;

		case COLLISION_TYPE_OBB:
			std::memcpy(rCollisionData.fDimensions, c_pbBuf, 3 * sizeof(float));
			c_pbBuf += 3 * sizeof(float);
			break;
		}

		std::memcpy(rCollisionData.quatRotation, c_pbBuf, sizeof(D3DXQUATERNION));
		c_pbBuf += sizeof(D3DXQUATERNION);
	}

	for (uint32_t j = 0; j < dwHeightDataCount; ++j)
	{
		THeightData& rHeightData = m_HeightDataVector[j];
		std::memcpy(rHeightData.szName, c_pbBuf, 32);
		c_pbBuf += 32;

		uint32_t dwPrimitiveCount;
		std::memcpy(&dwPrimitiveCount, c_pbBuf, sizeof(uint32_t));
		c_pbBuf += sizeof(uint32_t);

		rHeightData.v3VertexVector.clear();
		rHeightData.v3VertexVector.resize(dwPrimitiveCount);
		std::memcpy(&rHeightData.v3VertexVector[0], c_pbBuf, dwPrimitiveCount * sizeof(D3DXVECTOR3));
		c_pbBuf += dwPrimitiveCount * sizeof(D3DXVECTOR3);

		// Getting Maximize Radius
		for (uint32_t k = 0; k < rHeightData.v3VertexVector.size(); ++k)
		{
			m_fMaximizeRadius = std::max(m_fMaximizeRadius, fabs(rHeightData.v3VertexVector[k].x) + 50.0f);
			m_fMaximizeRadius = std::max(m_fMaximizeRadius, fabs(rHeightData.v3VertexVector[k].y) + 50.0f);
			m_fMaximizeRadius = std::max(m_fMaximizeRadius, fabs(rHeightData.v3VertexVector[k].z) + 50.0f);
		}
		// Getting Maximize Radius
	}

	return true;
}

void CAttributeData::OnClear()
{
	m_StaticCollisionDataVector.clear();
	m_HeightDataVector.clear();
}

bool CAttributeData::OnIsEmpty() const
{
	if (!m_StaticCollisionDataVector.empty())
		return false;
	if (!m_HeightDataVector.empty())
		return false;

	return true;
}

bool CAttributeData::OnIsType(TType type)
{
	if (CAttributeData::Type() == type)
		return true;

	return CResource::OnIsType(type);
}

CAttributeData::TType CAttributeData::Type()
{
	static TType s_type = StringToType("CAttributeData");
	return s_type;
}

void CAttributeData::OnSelfDestruct()
{
	Clear();
}

CAttributeData::CAttributeData(const FilenameWrapper& filename) : CResource(filename)
{
	m_fMaximizeRadius = 0.0f;
}
