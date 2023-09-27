#pragma once

#include "Resource.h"
#include "Ref.h"
#include "CollisionData.h"

typedef struct SHeightData
{
	char szName[32 + 1];
	std::vector<D3DXVECTOR3> v3VertexVector;
} THeightData;

typedef std::vector<THeightData> THeightDataVector;

class CAttributeData : public CResource
{
public:
	typedef CRef<CAttributeData> TRef;
	static TType Type();
	CAttributeData(const FilenameWrapper& filename);
	virtual ~CAttributeData() = default;

	const CStaticCollisionDataVector& GetCollisionDataVector() const
	{
		return m_StaticCollisionDataVector;
	}

	const THeightDataVector& GetHeightDataVector() const
	{
		return m_HeightDataVector;
	}

	uint32_t GetHeightDataCount() const
	{
		return static_cast<size_t>(m_HeightDataVector.size());
	}

	float GetMaximizeRadius() const
	{
		return m_fMaximizeRadius;
	}

	size_t AddCollisionData(const CStaticCollisionData& collisionData);
	bool GetHeightDataPointer(uint32_t dwIndex, const THeightData** c_ppHeightData) const;

protected:
	bool OnLoad(int32_t iSize, const void* c_pvBuf);
	void OnClear();
	bool OnIsEmpty() const;
	bool OnIsType(TType type);
	void OnSelfDestruct();

protected:
	float m_fMaximizeRadius;
	CStaticCollisionDataVector m_StaticCollisionDataVector;
	THeightDataVector m_HeightDataVector;
};
