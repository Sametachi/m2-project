#pragma once
#include "GrpTexture.h"

class CGraphicImageTexture : public CGraphicTexture
{
public:
	CGraphicImageTexture();
	virtual ~CGraphicImageTexture();

	void Destroy();
	bool Create(uint32_t width, uint32_t height, D3DFORMAT d3dFmt);
	bool CreateDeviceObjects();

	void CreateFromTexturePointer(const CGraphicTexture* c_pSrcTexture);
	bool CreateWithStbImage(uint32_t bufSize, const void* c_pvBuf);
	bool CreateFromDiskFile(const char* c_szFileName, D3DFORMAT d3dFmt);
	bool CreateFromMemoryFile(uint32_t bufSize, const void* c_pvBuf);

	void SetFileName(const std::string& c_szFileName);
	bool Lock(int32_t* pRetPitch, void** ppRetPixels, int32_t level = 0);
	void Unlock(int32_t level = 0);

protected:
	void Initialize();

	D3DFORMAT m_d3dFmt;
	std::string m_stFileName;
};
