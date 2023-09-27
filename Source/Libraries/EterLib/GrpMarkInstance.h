#pragma once
#include "GrpImage.h"
#include "Pool.h"

class CGraphicMarkInstance
{
public:
	static uint32_t Type();
	BOOL IsType(uint32_t dwType);

	void SetImageFileName(const char* c_szFileName);
	const std::string& GetImageFileName();

public:
	CGraphicMarkInstance() = default;
	virtual ~CGraphicMarkInstance();

	void Destroy();

	void Render();

	void SetDiffuseColor(float fr, float fg, float fb, float fa);
	void SetPosition(float fx, float fy, float fz = 0.0f);
	void SetIndex(uint32_t uIndex);
	void SetScale(float fScale);

	void Load();
	bool IsEmpty() const;

	int32_t GetWidth();
	int32_t GetHeight();

	CGraphicTexture* GetTexturePointer();
	const CGraphicTexture& GetTextureReference() const;
	CGraphicImage* GetGraphicImagePointer();

	bool operator == (const CGraphicMarkInstance& rhs) const;

protected:
	enum
	{
		MARK_WIDTH = 16,
		MARK_HEIGHT = 12
	};
	virtual void OnRender();
	virtual void OnSetImagePointer();

	virtual BOOL OnIsType(uint32_t dwType);

	void SetImagePointer(CGraphicImage* pImage);

protected:
	D3DXCOLOR m_DiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	D3DXVECTOR3 m_v3Position = { 0.0f, 0.0f, 0.0f };

	uint32_t m_uIndex = 0;

	float m_fScale = 1.0f;
	float m_fDepth;

	CGraphicImage::TRef m_roImage;
	std::string m_stImageFileName;
};
