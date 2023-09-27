#pragma once

#include "GrpBase.h"

class CGraphicTexture : public CGraphicBase
{
public:
	virtual bool IsEmpty() const;

	int32_t GetWidth() const;
	int32_t GetHeight() const;

	void SetWidth(int width);
	void SetHeight(int height);

	void SetTextureStage(int32_t stage) const;
	LPDIRECT3DTEXTURE9 GetD3DTexture() const;

	bool SetSubData(uint32_t level, uint32_t  left, uint32_t top, uint32_t width, uint32_t height, uint32_t pitch, void* pData, D3DFORMAT pf);
	void DestroyDeviceObjects();

protected:
	CGraphicTexture();
	~CGraphicTexture() override = default;

	void Destroy();
	void Initialize();

protected:
	bool m_bEmpty;

	int32_t m_width;
	int32_t m_height;

	LPDIRECT3DTEXTURE9 m_lpd3dTexture;
};
