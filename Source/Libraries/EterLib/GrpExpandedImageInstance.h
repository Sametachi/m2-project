#pragma once
#include "GrpImageInstance.h"

class CGraphicExpandedImageInstance : public CGraphicImageInstance
{
public:
	static uint32_t Type();
	enum ERenderingMode
	{
		RENDERING_MODE_NORMAL,
		RENDERING_MODE_SCREEN,
		RENDERING_MODE_COLOR_DODGE,
		RENDERING_MODE_MODULATE
	};

public:
	CGraphicExpandedImageInstance() = default;
	virtual ~CGraphicExpandedImageInstance();

	void Destroy() override;

	void SetDepth(float fDepth);
	void SetSize(long width, long height);
	void SetOrigin();
	void SetOrigin(float fx, float fy);
	void SetRotation(float fRotation);
	void SetScale(float fx, float fy);
	void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
	void SetRenderingRectWithScale(float fLeft, float fTop, float fRight, float fBottom);
	void SetRenderingMode(int32_t iMode);

protected:
	void OnRender() override;
	void OnSetImagePointer() override;
	BOOL OnIsType(uint32_t dwType) override;

	float m_fDepth{ 0.0f };
	D3DXVECTOR2 m_v2Origin{ 0.0f, 0.0f };
	D3DXVECTOR2 m_v2Scale{ 1.0f, 1.0f };
	float m_fRotation{ 0.0f };
	RECT m_RenderingRect{};
	int32_t m_iRenderingMode{ RENDERING_MODE_NORMAL };
};
