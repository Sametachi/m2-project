// ShadowRenderHelper.h: interface for the CShadowRenderHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHADOWRENDERHELPER_H__262445E9_97FE_41E5_92DC_406DA02D72B4__INCLUDED_)
#define AFX_SHADOWRENDERHELPER_H__262445E9_97FE_41E5_92DC_406DA02D72B4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CShadowRenderHelper : public CScreen, public CSingleton<CShadowRenderHelper>
{
public:
	CShadowRenderHelper();
	virtual ~CShadowRenderHelper();

	bool CreateTextures();
	void ReleaseTextures();

	// Assign
	void SetMapOutdoorAccssorPointer(CMapOutdoorAccessor * pMapOutdoorAccessor) { m_pMapOutdoorAccessor = pMapOutdoorAccessor;	}
	void SetTargetTerrainCoord(uint16_t wCoordX, uint16_t wCoordY) { m_wCurCoordX = wCoordX; m_wCurCoordY = wCoordY; }

	// Size
	void SetShadowMapPower(uint8_t byShadowMapPower);
	void SetIntermediateShadowMapPower(uint8_t byShadowMapPower);
	uint8_t GetShadowMapPower() { return m_byShadowMapPower; }
	uint8_t GetIntermediateShadowMapPower() { return m_byIntermediateShadowMapPower; }

	// Filter
	void SetShadowMapFilter(D3DTEXTUREFILTERTYPE eTextureFilter);
	void SetIntermediateShadowMapFilter(D3DTEXTUREFILTERTYPE eTextureFilter);

	// Output
	LPDIRECT3DTEXTURE8 GetShadowTexture()				{ return m_lpShadowMapRenderTargetTexture; }
	LPDIRECT3DTEXTURE8 GetIntermediateShadowTexture()	{ return m_lpIntermediateRenderTargetTexture; }

	// Render
	bool StartRenderingPhase(uint8_t byPhase);
	void EndRenderingPhase(uint8_t byPhase);

private:

	// Backup
	bool SaveRenderTarget();
	void RestoreRenderTarget();

	// CMapOutdoorAccessor Pointer
	CMapOutdoorAccessor *	m_pMapOutdoorAccessor;

	uint16_t					m_wCurCoordX;
	uint16_t					m_wCurCoordY;

	// Rendering Phase
	uint8_t					m_byPhase;

	// Size
	uint8_t					m_byShadowMapPower;
	const uint8_t				m_byMaxShadowMapPower;
	const uint8_t				m_byMinShadowMapPower;
	uint32_t					m_dwShadowMapSize;

	uint8_t					m_byIntermediateShadowMapPower;
	const uint8_t				m_byMaxIntermediateShadowMapPower;
	const uint8_t				m_byMinIntermediateShadowMapPower;
	uint32_t					m_dwIntermediateShadowMapSize;

	// Shadow Map
	LPDIRECT3DSURFACE8		m_lpShadowMapRenderTargetSurface;
	LPDIRECT3DSURFACE8		m_lpShadowMapDepthSurface;
	LPDIRECT3DTEXTURE8		m_lpShadowMapRenderTargetTexture;
	D3DVIEWPORT8			m_ShadowMapViewport;

	D3DTEXTUREFILTERTYPE	m_eShadowMapTextureFilter;

	// Backup
	LPDIRECT3DSURFACE8		m_lpBackupRenderTargetSurface;
	LPDIRECT3DSURFACE8		m_lpBackupDepthSurface;
	D3DVIEWPORT8			m_BackupViewport;

	LPDIRECT3DSURFACE8		m_lpIntermediateRenderTargetSurface;
	LPDIRECT3DSURFACE8		m_lpIntermediateDepthSurface;
	LPDIRECT3DTEXTURE8		m_lpIntermediateRenderTargetTexture;
	D3DVIEWPORT8			m_IntermediateViewport;
	D3DXMATRIX				m_matLightView;
	D3DXMATRIX				m_matLightProj;

	D3DTEXTUREFILTERTYPE	m_eIntermediateTextureFilter;
};

#endif // !defined(AFX_SHADOWRENDERHELPER_H__262445E9_97FE_41E5_92DC_406DA02D72B4__INCLUDED_)
