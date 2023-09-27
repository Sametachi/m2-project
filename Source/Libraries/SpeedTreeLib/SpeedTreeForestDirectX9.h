#pragma once
#define SPEEDTREE_DATA_FORMAT_DIRECTX
#include "SpeedTreeForest.h"
#include "SpeedTreeMaterial.h"

class CSpeedTreeForestDirectX9 : public CSpeedTreeForest, public CGraphicBase, public Singleton<CSpeedTreeForestDirectX9>
{
public:
	CSpeedTreeForestDirectX9();
	virtual ~CSpeedTreeForestDirectX9();

	void UploadWindMatrix(uint32_t uiLocation, const float* pMatrix) const;
	void UpdateCompundMatrix(const D3DXVECTOR3& c_rEyeVec, const D3DXMATRIX& c_rmatView, const D3DXMATRIX& c_rmatProj);

	void Render(uint32_t ulRenderBitVector = Forest_RenderAll);
	void RenderShadows(uint32_t ulRenderBitVector = Forest_RenderAll);
	bool SetRenderingDevice();

private:
	bool InitVertexShaders();
	LPDIRECT3DVERTEXSHADER9 m_dwBranchVertexShader;
	LPDIRECT3DVERTEXSHADER9 m_dwLeafVertexShader;
};
