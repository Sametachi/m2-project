#include "StdAfx.h"
#include "TextureSet.h"
#include "../EterLib/ResourceManager.h"
#include "../EterLib/Util.h"
#include <storm/StringUtil.hpp>
#include <Basic/File_ptr.hpp>

CTextureSet::CTextureSet()
{
	Initialize();
}

CTextureSet::~CTextureSet()
{
	Clear();
}

void CTextureSet::Initialize()
{
}

void CTextureSet::Create()
{
	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>("d:/ymir work/special/error.tga");
	m_ErrorTexture.ImageInstance.SetImagePointer(static_cast<CGraphicImage*> (pResource));
	AddEmptyTexture();
}

bool CTextureSet::Load(const char* c_szTextureSetFileName, float fTerrainTexCoordBase)
{
	Clear();

	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(c_szTextureSetFileName, stTokenVectorMap))
	{
		SysLog("TextureSet::Load : cannot load {0}", c_szTextureSetFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("textureset"))
	{
		SysLog("TextureSet::Load : syntax error, TextureSet (filename: {0})", c_szTextureSetFileName);
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("texturecount"))
	{
		SysLog("TextureSet::Load : syntax error, TextureCount (filename: {0})", c_szTextureSetFileName);
		return false;
	}

	Create();

	const auto& c_rstrCount = stTokenVectorMap["texturecount"][0];

	long lCount = atol(c_rstrCount.c_str());
	char szTextureName[32 + 1];

	m_Textures.resize(lCount + 1);

	for (int32_t i = 0; i < lCount; ++i)
	{
		_snprintf_s(szTextureName, sizeof(szTextureName), "texture%03d", i + 1);

		if (stTokenVectorMap.end() == stTokenVectorMap.find(szTextureName))
			continue;

		const CTokenVector& rVector = stTokenVectorMap[szTextureName];
		const auto& c_rstrFileName = rVector[0];
		const auto& c_rstrUScale = rVector[1];
		const auto& c_rstrVScale = rVector[2];
		const auto& c_rstrUOffset = rVector[3];
		const auto& c_rstrVOffset = rVector[4];
		const auto& c_rstrbSplat = rVector[5];
		const auto& c_rstrBegin = rVector[6];
		const auto& c_rstrEnd = rVector[7];

		float fuScale, fvScale, fuOffset, fvOffset;
		uint8_t bSplat;
		unsigned short usBegin, usEnd;

		storm::ParseNumber(c_rstrUScale, fuScale);
		storm::ParseNumber(c_rstrVScale, fvScale);
		storm::ParseNumber(c_rstrUOffset, fuOffset);
		storm::ParseNumber(c_rstrVOffset, fvOffset);
		storm::ParseNumber(c_rstrbSplat, bSplat);
		usBegin = static_cast<unsigned short>(atoi(c_rstrBegin.c_str()));
		usEnd = static_cast<unsigned short>(atoi(c_rstrEnd.c_str()));

		if (!SetTexture(i + 1, c_rstrFileName.c_str(), fuScale, fvScale, fuOffset, fvOffset, bSplat, usBegin, usEnd, fTerrainTexCoordBase))
			SysLog("CTextureSet::Load : SetTexture failed : Filename: {0}", c_rstrFileName.c_str());
	}

	m_stFileName.assign(c_szTextureSetFileName);
	return true;
}

void CTextureSet::Clear()
{
	m_ErrorTexture.ImageInstance.Destroy();
	m_Textures.clear();
	Initialize();
}

void CTextureSet::AddEmptyTexture()
{
	TTerrainTexture eraser;
	m_Textures.emplace_back(eraser);
}

uint32_t CTextureSet::GetTextureCount()
{
	return m_Textures.size();
}

TTerrainTexture& CTextureSet::GetTexture(uint32_t ulIndex)
{
	if (GetTextureCount() <= ulIndex)
		return m_ErrorTexture;

	return m_Textures[ulIndex];
}

bool CTextureSet::SetTexture(uint32_t ulIndex, const char* c_szFileName, float fuScale, float fvScale, float fuOffset, float fvOffset, bool bSplat, uint16_t usBegin, uint16_t usEnd, float fTerrainTexCoordBase)
{
	if (ulIndex >= m_Textures.size())
	{
		SysLog("CTextureSet::SetTexture : Index Error : Index({0}) is Larger than TextureSet Size({1})", ulIndex, m_Textures.size());
		return false;
	}

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(c_szFileName);
	if (!pResource)
		return false;

	if (!pResource->IsType(CGraphicImage::Type()))
	{
		const auto& stRefResourceName = pResource->GetFileNameString();
		SysLog("CTerrainImpl::GenerateTexture : {0} is NOT Image File", stRefResourceName.c_str());
		return false;
	}

	if (!pResource)
		return false;

	TTerrainTexture& tex = m_Textures[ulIndex];

	tex.stFilename = c_szFileName;
	tex.UScale = fuScale;
	tex.VScale = fvScale;
	tex.UOffset = fuOffset;
	tex.VOffset = fvOffset;
	tex.bSplat = bSplat;
	tex.Begin = usBegin;
	tex.End = usEnd;
	tex.ImageInstance.SetImagePointer(static_cast<CGraphicImage*>(pResource));
	tex.pd3dTexture = tex.ImageInstance.GetTexturePointer()->GetD3DTexture();


	D3DXMatrixScaling(&tex.m_matTransform, fTerrainTexCoordBase * tex.UScale, -fTerrainTexCoordBase * tex.VScale, 0.0f);
	tex.m_matTransform._41 = tex.UOffset;
	tex.m_matTransform._42 = -tex.VOffset;
	return true;
}

void CTextureSet::Reload(float fTerrainTexCoordBase)
{
	for (uint32_t dwIndex = 1; dwIndex < GetTextureCount(); ++dwIndex)
	{
		TTerrainTexture& tex = m_Textures[dwIndex];

		tex.ImageInstance.ReloadImagePointer(CResourceManager::GetInstance()->LoadResource<CGraphicImage>(tex.stFilename));
		tex.pd3dTexture = tex.ImageInstance.GetTexturePointer()->GetD3DTexture();

		D3DXMatrixScaling(&tex.m_matTransform, fTerrainTexCoordBase * tex.UScale, -fTerrainTexCoordBase * tex.VScale, 0.0f);
		tex.m_matTransform._41 = tex.UOffset;
		tex.m_matTransform._42 = -tex.VOffset;
	}
}

bool CTextureSet::ReplaceTexture(const char* c_OldszFileName, const char* c_szFileName, float fuScale, float fvScale, float fuOffset, float fvOffset, bool bSplat, uint16_t usBegin, uint16_t usEnd, float fTerrainTexCoordBase)
{
	for (uint32_t i = 1; i < GetTextureCount(); ++i)
	{
		if (0 == m_Textures[i].stFilename.compare(c_OldszFileName))
		{
			SetTexture(i,
				c_szFileName,
				fuScale,
				fvScale,
				fuOffset,
				fvOffset,
				bSplat,
				usBegin,
				usEnd,
				fTerrainTexCoordBase);
			return true;
		}
	}
	return false;
}

bool CTextureSet::AddTexture(const char* c_szFileName, float fuScale, float fvScale, float fuOffset, float fvOffset, bool bSplat, uint16_t usBegin, uint16_t usEnd, float fTerrainTexCoordBase)
{
	if (GetTextureCount() >= 256)
	{
		SysLog("You cannot add more than 255 texture.");
		return false;
	}

	for (uint32_t i = 1; i < GetTextureCount(); ++i)
	{
		if (0 == m_Textures[i].stFilename.compare(c_szFileName))
		{
			SysLog("Texture of the same name already exists.");
			return false;
		}
	}

	CResource* pResource = CResourceManager::GetInstance()->LoadResource<CResource>(c_szFileName);
	if (!pResource)
		return false;

	if (!pResource->IsType(CGraphicImage::Type()))
	{
		const auto& stRefResourceName = pResource->GetFileNameString();
		SysLog("CTerrainImpl::GenerateTexture : It's not an image file. %s", stRefResourceName.c_str());
		return false;
	}

	if (!pResource)
		return false;

	m_Textures.reserve(m_Textures.size() + 1);

	AddEmptyTexture();
	SetTexture(m_Textures.size() - 1,
		c_szFileName,
		fuScale,
		fvScale,
		fuOffset,
		fvOffset,
		bSplat,
		usBegin,
		usEnd,
		fTerrainTexCoordBase);

	return true;
}

bool CTextureSet::RemoveTexture(uint32_t ulIndex)
{
	if (GetTextureCount() <= ulIndex)
		return false;

	auto itor = m_Textures.begin() + ulIndex;
	m_Textures.erase(itor);
	return true;
}

bool CTextureSet::Save(const char* c_pszFileName)
{
	msl::file_ptr fPtr(c_pszFileName, "w");
	if (!fPtr)
		return false;

	fprintf(fPtr.get(), "TextureSet\n");
	fprintf(fPtr.get(), "\n");

	fprintf(fPtr.get(), "TextureCount %u\n", GetTextureCount() ? (GetTextureCount() - 1) : 0);
	fprintf(fPtr.get(), "\n");

	for (uint32_t i = 1; i < GetTextureCount(); ++i)
	{
		TTerrainTexture& rTex = m_Textures[i];

		fprintf(fPtr.get(), "Start Texture%03u\n", i);
		fprintf(fPtr.get(), "    \"%s\"\n", rTex.stFilename.c_str());
		fprintf(fPtr.get(), "    %f\n", rTex.UScale);
		fprintf(fPtr.get(), "    %f\n", rTex.VScale);
		fprintf(fPtr.get(), "    %f\n", rTex.UOffset);
		fprintf(fPtr.get(), "    %f\n", rTex.VOffset);
		fprintf(fPtr.get(), "    %d\n", rTex.bSplat);
		fprintf(fPtr.get(), "    %hu\n", rTex.Begin);
		fprintf(fPtr.get(), "    %hu\n", rTex.End);
		fprintf(fPtr.get(), "End Texture%03u\n", i);
	}
	return true;
}