#pragma once
#include "Util.h"
#include "../EterLib/GrpImageInstance.h"

class CGrannyMaterial
{
public:
	enum EType
	{
		TYPE_DIFFUSE_PNT,
		TYPE_BLEND_PNT,
		TYPE_MAX_NUM
	};

	CGrannyMaterial(granny_material* material);


	bool IsIn(const char* c_szImageName, int32_t* iStage);

	void SetImagePointer(int32_t iStage, CGraphicImage* pImage);
	void SetSpecularInfo(bool bFlag, float fPower, uint8_t uSphereMapIndex);

	void ApplyRenderState();
	void RestoreRenderState();

	CGrannyMaterial::EType GetType() const;
	CGraphicImage* GetImagePointer(int32_t iStage) const;

	const CGraphicTexture* GetDiffuseTexture() const;
	const CGraphicTexture* GetOpacityTexture() const;

	LPDIRECT3DTEXTURE9 GetD3DTexture(int32_t iStage) const;

	bool IsTwoSided() const { return m_bTwoSideRender; }

	bool operator==(granny_material* material) const;

	static void CreateSphereMap(uint32_t uMapIndex, const char* c_szSphereMapImageFileName);
	static void DestroySphereMap();

	static void TranslateSpecularMatrix(float fAddX, float fAddY, float fAddZ);

protected:
	CGraphicImage* __GetImagePointer(const char* c_szFileName);

	bool __IsSpecularEnable() const;
	float __GetSpecularPower() const;

	void __ApplyDiffuseRenderState();
	void __RestoreDiffuseRenderState();
	void __ApplySpecularRenderState();
	void __RestoreSpecularRenderState();

	granny_material* m_pgrnMaterial;
	CGraphicImage::TRef m_roImage[3];
	EType m_eType;

	float m_fSpecularPower;
	bool m_bSpecularEnable;
	bool m_bTwoSideRender;
	uint32_t m_dwLastCullRenderStateForTwoSideRendering;
	uint8_t m_bSphereMapIndex;

	void (CGrannyMaterial::* m_pfnApplyRenderState)();
	void (CGrannyMaterial::* m_pfnRestoreRenderState)();

private:
	enum
	{
		SPHEREMAP_NUM = 10,
	};

	static D3DXMATRIX ms_matSpecular;
	static D3DXVECTOR3 ms_v3SpecularTrans;

	static CGraphicImageInstance ms_akSphereMapInstance[SPHEREMAP_NUM];
};

class CGrannyMaterialPalette
{
public:
	void Clear() { m_materials.clear(); }

	std::size_t RegisterMaterial(granny_material* material);

	void SetMaterialImage(const char* materialName, CGraphicImage* image);
	void SetMaterialData(const char* materialName, const SMaterialData& data);
	void SetSpecularInfo(const char* materialName, bool enable, float power);

	CGrannyMaterial& GetMaterialRef(std::size_t index);
	std::size_t GetMaterialCount() const;

protected:
	std::vector<CGrannyMaterial> m_materials;
};