#pragma once
#include "Ref.h"
#include "Resource.h"
#include "GrpImageTexture.h"

class CGraphicImage : public CResource
{
public:
	typedef CRef<CGraphicImage> TRef;

public:
	static TType Type();

public:
	CGraphicImage(const FilenameWrapper& filename, uint32_t dwFilter = D3DX_FILTER_LINEAR);
	~CGraphicImage() = default;

	virtual bool CreateDeviceObjects();
	virtual void DestroyDeviceObjects();

	int32_t GetWidth() const;
	int32_t GetHeight() const;

	void SetWidth(long width);
	void SetHeight(long height);

	const RECT& GetRectReference() const;

	const CGraphicTexture& GetTextureReference() const;
	CGraphicTexture* GetTexturePointer();

protected:
	bool OnLoad(int32_t iSize, const void* c_pvBuf) override;

	void OnClear() override;
	bool OnIsEmpty() const override;
	bool OnIsType(TType type) override;

protected:
	CGraphicImageTexture		m_imageTexture;
	RECT						m_rect{};
	uint32_t					m_dwFilter;
};

