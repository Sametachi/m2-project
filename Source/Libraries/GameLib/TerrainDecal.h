#pragma once
#include <EterLib/Decal.h>

class CMapOutdoor;

class CTerrainDecal : public CDecal
{
public:
	enum
	{
		MAX_SEARCH_VERTICES = 1024
	};

	CTerrainDecal(CMapOutdoor* pMapOutdoor = nullptr);
	virtual ~CTerrainDecal();

	virtual void Make(D3DXVECTOR3 v3Center, D3DXVECTOR3 v3Normal, D3DXVECTOR3 v3Tangent, float fWidth, float fHeight, float fDepth);
	// 	virtual void Update();
	virtual void Render();

	void SetMapOutdoor(CMapOutdoor* pMapOutdoor) { m_pMapOutdoor = pMapOutdoor; }

protected:

	void SearchAffectedTerrainMesh(float fMinX,
		float fMaxX,
		float fMinY,
		float fMaxY,
		uint32_t* pdwAffectedPrimitiveCount,
		D3DXVECTOR3* pv3AffectedVertex,
		D3DXVECTOR3* pv3AffectedNormal);

	CMapOutdoor* m_pMapOutdoor;
};
