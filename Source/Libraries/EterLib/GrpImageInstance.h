#pragma once
#include "GrpImage.h"
#include "GrpIndexBuffer.h"
#include "Pool.h"

class CGraphicImageInstance
{
public:
	static uint32_t Type();
	BOOL IsType(uint32_t dwType);

public:
	CGraphicImageInstance();
	virtual ~CGraphicImageInstance();

	virtual void Destroy();

	void Render();

	void SetDiffuseColor(float fr, float fg, float fb, float fa);
	void SetScale(float sx, float sy);
	void SetPosition(float fx, float fy);
	void SetInverse();

	void SetImagePointer(CGraphicImage* pImage);
	void ReloadImagePointer(CGraphicImage* pImage);
	bool IsEmpty() const;

	int32_t GetWidth();
	int32_t GetHeight();

	void SetSize(int32_t width, int32_t height);

	void GetPositon(D3DXVECTOR2& pos);

	CGraphicTexture* GetTexturePointer();
	const CGraphicTexture& GetTextureReference() const;
	CGraphicImage* GetGraphicImagePointer();

	bool operator == (const CGraphicImageInstance& rhs) const;
	D3DXCOLOR GetPixelColor(int32_t x, int32_t y);

protected:
	void Initialize();

	virtual void OnRender();
	virtual void OnSetImagePointer();

	virtual BOOL OnIsType(uint32_t dwType);

protected:
	D3DXCOLOR m_DiffuseColor;
	D3DXVECTOR2 m_v2Position;
	D3DXVECTOR2 m_v2Scale;
	bool m_bIsInverse{};

	CGraphicImage::TRef m_roImage;

public:
	static void CreateSystem(uint32_t uCapacity);
	static void DestroySystem();

	static CGraphicImageInstance* New();
	static void Delete(CGraphicImageInstance* pkImgInst);

	static CDynamicPool<CGraphicImageInstance>		ms_kPool;
};
