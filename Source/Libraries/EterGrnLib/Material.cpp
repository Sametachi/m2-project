#include "StdAfx.h"
#include "Material.h"
#include <utility>
#include "Mesh.h"
#include <Eterlib/ResourceManager.h>
#include <Eterlib/StateManager.h>
#include <Eterlib/GrpScreen.h>

CGraphicImageInstance CGrannyMaterial::ms_akSphereMapInstance[SPHEREMAP_NUM];

D3DXVECTOR3	CGrannyMaterial::ms_v3SpecularTrans(0.0f, 0.0f, 0.0f);
D3DXMATRIX	CGrannyMaterial::ms_matSpecular;
D3DXCOLOR g_fSpecularColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);

CGrannyMaterial::CGrannyMaterial(granny_material* material)
	: m_pgrnMaterial(material)
	, m_fSpecularPower(0.0f)
	, m_bSpecularEnable(false)
	, m_dwLastCullRenderStateForTwoSideRendering(D3DCULL_CW)
	, m_bSphereMapIndex(0)
	, m_pfnApplyRenderState(&CGrannyMaterial::__ApplyDiffuseRenderState)
	, m_pfnRestoreRenderState(&CGrannyMaterial::__RestoreDiffuseRenderState)
{
	granny_texture* diffuseTexture = nullptr;
	granny_texture* opacityTexture = nullptr;

	if (material)
	{
		if (material->MapCount > 1 && !_strnicmp(material->Name, "Blend", 5))
		{
			diffuseTexture = GrannyGetMaterialTextureByType(material->Maps[0].Material, GrannyDiffuseColorTexture);
			opacityTexture = GrannyGetMaterialTextureByType(material->Maps[1].Material, GrannyDiffuseColorTexture);
		}
		else
		{
			diffuseTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyDiffuseColorTexture);
			opacityTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyOpacityTexture);
		}

		{
			granny_int32 twoSided = 0;
			granny_data_type_definition TwoSidedFieldType[] =
			{
				{GrannyInt32Member, "Two-sided"},
				{GrannyEndMember},
			};

			granny_variant twoSideResult;
			const bool ok = GrannyFindMatchingMember(material->ExtendedData.Type, material->ExtendedData.Object, "Two-sided", &twoSideResult);

			if (ok && twoSideResult.Type) 
			{
				GrannyConvertSingleObject(twoSideResult.Type, twoSideResult.Object, TwoSidedFieldType, &twoSided, NULL);

				m_bTwoSideRender = 1 == twoSided;
			}
			else 
			{
				m_bTwoSideRender = false;
			}
		}
	}

	if (diffuseTexture)
		m_roImage[0] = __GetImagePointer(diffuseTexture->FromFileName);

	if (opacityTexture)
		m_roImage[1] = __GetImagePointer(opacityTexture->FromFileName);

	if (!m_roImage[1].IsNull())
		m_eType = TYPE_BLEND_PNT;
	else
		m_eType = TYPE_DIFFUSE_PNT;
}

bool CGrannyMaterial::IsIn(const char* c_szImageName, int32_t* piStage)
{
	std::string strImageName = c_szImageName;
	CFileNameHelper::StringPath(strImageName);

	granny_texture* pgrnDiffuseTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyDiffuseColorTexture);
	if (pgrnDiffuseTexture)
	{
		std::string strDiffuseFileName = pgrnDiffuseTexture->FromFileName;
		CFileNameHelper::StringPath(strDiffuseFileName);
		if (strDiffuseFileName == strImageName)
		{
			*piStage = 0;
			return true;
		}
	}

	granny_texture* pgrnOpacityTexture = GrannyGetMaterialTextureByType(m_pgrnMaterial, GrannyOpacityTexture);
	if (pgrnOpacityTexture)
	{
		std::string strOpacityFileName = pgrnOpacityTexture->FromFileName;
		CFileNameHelper::StringPath(strOpacityFileName);
		if (strOpacityFileName == strImageName)
		{
			*piStage = 1;
			return true;
		}
	}

	return false;
}

void CGrannyMaterial::SetImagePointer(int32_t iStage, CGraphicImage* pImage)
{
	assert(iStage < 2 && "CGrannyMaterial::SetImagePointer");
	m_roImage[iStage] = std::move(pImage);
}

void CGrannyMaterial::SetSpecularInfo(bool bFlag, float fPower, uint8_t uSphereMapIndex)
{
	m_fSpecularPower = fPower;
	m_bSphereMapIndex = uSphereMapIndex;
	m_bSpecularEnable = bFlag;

	if (bFlag)
	{
		m_pfnApplyRenderState = &CGrannyMaterial::__ApplySpecularRenderState;
		m_pfnRestoreRenderState = &CGrannyMaterial::__RestoreSpecularRenderState;
	}
	else
	{
		m_pfnApplyRenderState = &CGrannyMaterial::__ApplyDiffuseRenderState;
		m_pfnRestoreRenderState = &CGrannyMaterial::__RestoreDiffuseRenderState;
	}
}

void CGrannyMaterial::ApplyRenderState()
{
	assert(m_pfnApplyRenderState != nullptr && "CGrannyMaterial::SaveRenderState");
	(this->*m_pfnApplyRenderState)();
}

void CGrannyMaterial::RestoreRenderState()
{
	assert(m_pfnRestoreRenderState != nullptr && "CGrannyMaterial::RestoreRenderState");
	(this->*m_pfnRestoreRenderState)();
}

CGrannyMaterial::EType CGrannyMaterial::GetType() const
{
	return m_eType;
}

CGraphicImage* CGrannyMaterial::GetImagePointer(int32_t iStage) const
{
	const CGraphicImage::TRef& ratImage = m_roImage[iStage];

	if (ratImage.IsNull())
		return nullptr;

	CGraphicImage* pImage = ratImage.GetPointer();
	return pImage;
}

const CGraphicTexture* CGrannyMaterial::GetDiffuseTexture() const
{
	if (m_roImage[0].IsNull())
		return nullptr;

	return m_roImage[0].GetPointer()->GetTexturePointer();
}

const CGraphicTexture* CGrannyMaterial::GetOpacityTexture() const
{
	if (m_roImage[1].IsNull())
		return nullptr;

	return m_roImage[1].GetPointer()->GetTexturePointer();
}

LPDIRECT3DTEXTURE9 CGrannyMaterial::GetD3DTexture(int32_t iStage) const
{
	const CGraphicImage::TRef& ratImage = m_roImage[iStage];
	if (ratImage.IsNull())
		return nullptr;

	CGraphicImage* pImage = ratImage.GetPointer();
	const CGraphicTexture* pTexture = pImage->GetTexturePointer();
	return pTexture->GetD3DTexture();
}

bool CGrannyMaterial::operator==(granny_material* material) const
{
	return m_pgrnMaterial == material;
}

bool CGrannyMaterial::__IsSpecularEnable() const
{
	return m_bSpecularEnable;
}

float CGrannyMaterial::__GetSpecularPower() const
{
	return m_fSpecularPower;
}

extern const std::string& GetModelLocalPath();

CGraphicImage* CGrannyMaterial::__GetImagePointer(const char* fileName)
{
	assert(*fileName != '\0');

	CResourceManager* rkResMgr = CResourceManager::GetInstance();

	// SUPPORT_LOCAL_TEXTURE
	int32_t fileName_len = strlen(fileName);
	if (fileName_len > 2 && fileName[1] != ':')
	{
		char localFileName[256];
		const std::string& modelLocalPath = GetModelLocalPath();

		int32_t localFileName_len = modelLocalPath.length() + 1 + fileName_len;
		if (localFileName_len < sizeof(localFileName) - 1)
		{
			_snprintf_s(localFileName, sizeof(localFileName), "%s%s", GetModelLocalPath().c_str(), fileName);
			return rkResMgr->LoadResource<CGraphicImage>(localFileName);
		}
	}
	// END_OF_SUPPORT_LOCAL_TEXTURE

	return rkResMgr->LoadResource<CGraphicImage>(fileName);
}

void CGrannyMaterial::__ApplyDiffuseRenderState()
{
	STATEMANAGER->SetTexture(0, GetD3DTexture(0));

	if (m_bTwoSideRender)
	{
		// -_- The rendering process is a little ugly ... If you Save & Restore, it gets a little twisted due to the order. It's annoying, so save it separately instead of Save & Restore.
		m_dwLastCullRenderStateForTwoSideRendering = STATEMANAGER->GetRenderState(D3DRS_CULLMODE);
		STATEMANAGER->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}
}

void CGrannyMaterial::__RestoreDiffuseRenderState()
{
	if (m_bTwoSideRender)
	{
		STATEMANAGER->SetRenderState(D3DRS_CULLMODE, m_dwLastCullRenderStateForTwoSideRendering);
	}
}

void CGrannyMaterial::__ApplySpecularRenderState()
{
	if (STATEMANAGER->GetRenderState(D3DRS_ALPHABLENDENABLE))
	{
		__ApplyDiffuseRenderState();
		return;
	}

	CGraphicTexture* pkTexture = ms_akSphereMapInstance[m_bSphereMapIndex].GetTexturePointer();

	STATEMANAGER->SetTexture(0, GetD3DTexture(0));

	if (pkTexture)
		STATEMANAGER->SetTexture(1, pkTexture->GetD3DTexture());
	else
		STATEMANAGER->SetTexture(1, nullptr);

	STATEMANAGER->SetRenderState(D3DRS_TEXTUREFACTOR, D3DXCOLOR(g_fSpecularColor.r, g_fSpecularColor.g, g_fSpecularColor.b, __GetSpecularPower()));
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
	STATEMANAGER->SaveTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATEALPHA_ADDCOLOR);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	STATEMANAGER->SetTransform(D3DTS_TEXTURE1, &ms_matSpecular);
	STATEMANAGER->SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	STATEMANAGER->SaveSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
}

void CGrannyMaterial::__RestoreSpecularRenderState()
{
	if (STATEMANAGER->GetRenderState(D3DRS_ALPHABLENDENABLE))
	{
		__RestoreDiffuseRenderState();
		return;
	}

	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSU);
	STATEMANAGER->RestoreSamplerState(1, D3DSAMP_ADDRESSV);
	STATEMANAGER->RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	STATEMANAGER->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_COLORARG1);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_COLORARG2);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_COLOROP);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_ALPHAARG1);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_ALPHAARG2);
	STATEMANAGER->RestoreTextureStageState(0, D3DTSS_ALPHAOP);
}

void CGrannyMaterial::CreateSphereMap(uint32_t uMapIndex, const char* c_szSphereMapImageFileName)
{
	CResourceManager* rkResMgr = CResourceManager::GetInstance();
	CGraphicImage* r = rkResMgr->LoadResource<CGraphicImage>(c_szSphereMapImageFileName);
	ms_akSphereMapInstance[uMapIndex].SetImagePointer(r);
}

void CGrannyMaterial::DestroySphereMap()
{
	for (uint32_t uMapIndex = 0; uMapIndex < SPHEREMAP_NUM; ++uMapIndex)
		ms_akSphereMapInstance[uMapIndex].Destroy();
}

void CGrannyMaterial::TranslateSpecularMatrix(float fAddX, float fAddY, float fAddZ)
{
	static float SPECULAR_TRANSLATE_MAX = 1000000.0f;

	ms_v3SpecularTrans.x += fAddX;
	ms_v3SpecularTrans.y += fAddY;
	ms_v3SpecularTrans.z += fAddZ;

	if (ms_v3SpecularTrans.x >= SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.x = 0.0f;

	if (ms_v3SpecularTrans.y >= SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.y = 0.0f;

	if (ms_v3SpecularTrans.z >= SPECULAR_TRANSLATE_MAX)
		ms_v3SpecularTrans.z = 0.0f;

	D3DXMatrixTranslation(&ms_matSpecular, ms_v3SpecularTrans.x, ms_v3SpecularTrans.y, ms_v3SpecularTrans.z);
}

/*
		CGrannyMaterialPalette
*/

void CGrannyMaterialPalette::SetMaterialImage(const char* materialName, CGraphicImage* image)
{
	for (CGrannyMaterial& material : m_materials)
	{
		int32_t stage;
		if (!material.IsIn(materialName, &stage))
			continue;

		material.SetImagePointer(stage, image);
		break;
	}
}

void CGrannyMaterialPalette::SetMaterialData(const char* materialName, const SMaterialData& data)
{
	if (materialName) 
	{
		for (CGrannyMaterial& material : m_materials) 
		{
			int32_t stage;
			if (!material.IsIn(materialName, &stage))
				continue;

			material.SetImagePointer(stage, data.pImage);
			material.SetSpecularInfo(data.isSpecularEnable, data.fSpecularPower, data.bSphereMapIndex);
			break;
		}
	}
	else 
	{
		for (CGrannyMaterial& material : m_materials) 
		{
			material.SetSpecularInfo(data.isSpecularEnable, data.fSpecularPower, data.bSphereMapIndex);
			break;
		}
	}
}

void CGrannyMaterialPalette::SetSpecularInfo(const char* materialName, bool enable, float power)
{
	if (materialName)
	{
		for (CGrannyMaterial& material : m_materials)
		{
			int32_t stage;
			if (!material.IsIn(materialName, &stage))
				continue;

			material.SetSpecularInfo(enable, power, 0);
			break;
		}
	}
	else 
	{
		for (CGrannyMaterial& material : m_materials)
		{
			material.SetSpecularInfo(enable, power, 0);
			break;
		}
	}
}

std::size_t CGrannyMaterialPalette::RegisterMaterial(granny_material* material)
{
	std::size_t size = m_materials.size();
	for (std::size_t i = 0; i != size; ++i) 
	{
		if (m_materials[i] == material)
			return i;
	}

	m_materials.emplace_back(material);
	return size;
}

CGrannyMaterial& CGrannyMaterialPalette::GetMaterialRef(std::size_t index)
{
	assert(index < m_materials.size());
	return m_materials[index];
}

std::size_t CGrannyMaterialPalette::GetMaterialCount() const
{
	return m_materials.size();
}


