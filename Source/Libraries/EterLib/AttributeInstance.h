#pragma once

#include <vector>
#include "AttributeData.h"
#include "Pool.h"

class CAttributeInstance
{
public:
	CAttributeInstance();
	virtual ~CAttributeInstance() = default;

	void Clear();
	bool IsEmpty() const;

	void SetObjectPointer(CAttributeData* pAttributeData);
	void RefreshObject(const D3DXMATRIX& c_rmatGlobal);

	const char* GetDataFileName() const
	{
		return m_roAttributeData->GetFileNameString().c_str();
	}

	CAttributeData* GetObjectPointer() const
	{
		return m_roAttributeData.GetPointer();
	}

	bool Picking(const D3DXVECTOR3& v, const D3DXVECTOR3& dir, float& out_x, float& out_y);
	bool IsInHeight(float fx, float fy);
	bool GetHeight(float fx, float fy, float* pfHeight);

protected:
	float m_fCollisionRadius;
	float m_fHeightRadius;
	D3DXMATRIX m_matGlobal;
	std::vector<std::vector<D3DXVECTOR3>> m_v3HeightDataVector;
	CAttributeData::TRef					m_roAttributeData;
};
