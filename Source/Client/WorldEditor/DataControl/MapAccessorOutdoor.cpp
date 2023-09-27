#include "StdAfx.h"
#include "MapAccessorOutdoor.h"
#include "MapAccessorTerrain.h"
#include "NonPlayerCharacterInfo.h"
#include "../Dialog/MapArrangeHeightProgressDialog.h"
#include "../MainFrm.h"
#include "../WorldEditorDoc.h"
#include "../WorldEditorView.h"
#include "../../../Client/eterlib/Camera.h"
#include "../../../Client/gamelib/TerrainPatch.h"
#include "../../../Client/gamelib/TerrainQuadtree.h"

#include "../Config/Globals.h"

CMapOutdoorAccessor *			CMapOutdoorAccessor::ms_pThis = NULL;
CMapArrangeHeightProgress *		CMapOutdoorAccessor::ms_pkProgressDialog = NULL;

//////////////////////////////////////////////////////////////////////////
// CMapOutdoorAccessor
//////////////////////////////////////////////////////////////////////////

CMapOutdoorAccessor::CMapOutdoorAccessor()
{
	m_iMapID = 0;
	m_fSplatValue = 4.6f;
	ms_pkProgressDialog = NULL;
	ms_pThis = this;
	m_bNowAccessGarvage = FALSE;
#ifdef USE_WE_CONFIG
	m_strNewTexSetName = "";
#endif
}

CMapOutdoorAccessor::~CMapOutdoorAccessor()
{
	Destroy();
}

float CMapOutdoorAccessor::GetSplatValue()
{
	return m_fSplatValue;
}

void CMapOutdoorAccessor::SetSplatValue(float fValue)
{
	m_fSplatValue = fValue;

	m_matSplatAlpha._41 = m_fTerrainTexCoordBase * m_fSplatValue;
	m_matSplatAlpha._42 = m_fTerrainTexCoordBase * m_fSplatValue;
}

void CMapOutdoorAccessor::OnPreAssignTerrainPtr()
{
	if (!m_bReady)
		return;
	SaveTerrains();
	SaveAreas();
	SaveMonsterAreaInfo();
	CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
	pFrame->UpdateMapControlBar();
}

bool CMapOutdoorAccessor::SaveProperty(const std::string & c_rstrFolder)
{
	std::string strfileName = c_rstrFolder;
	strfileName += "\\MapProperty.txt";
	FILE * File = fopen(strfileName.c_str(), "w");

	if (!File)
		return false;

	fprintf(File, "ScriptType MapProperty\n");
	fprintf(File, "\n");

	if (CMapBase::MAPTYPE_INDOOR == GetType())
		fprintf(File, "MapType \"Indoor\"\n");
	else if (CMapBase::MAPTYPE_OUTDOOR == GetType())
		fprintf(File, "MapType \"Outdoor\"\n");
	else
		fprintf(File, "MapType \"Invalid\"\n");
	fprintf(File, "\n");

//	fprintf(File, "MapSetting \"Setting.txt\"\n", c_rstrFolder.c_str());
//	fprintf(File, "\n");

	fclose(File);
	return true;
}

#define TexSetInit(texset, tertex, skipsave)	\
	if (!m_TextureSet.Load(texset, tertex))\
	{\
		skipsave = (m_TextureSet.Save(texset) && m_TextureSet.Load(texset, tertex));\
		if (skipsave)\
			Tracef("[%s] TextureSet successfully created.\n", texset);\
		else\
			LogBoxf("[%s] TextureSet can't be created. It will remain blank.", texset);\
	}\
	else\
		Tracef("[%s] TextureSet successfully loaded.\n", GetNewTexSetName().c_str());

bool CMapOutdoorAccessor::SaveSetting(const std::string & c_rstrFolder)
{
	if (_access("textureset", 0) != 0) // @fixme120 create folder if missing
		CreateDirectoryA("textureset", nullptr);
	bool skipTSSave = false;
	if (m_TextureSet.GetFileName()[0]=='\0')
	{
#ifdef USE_WE_CONFIG
		float fTerrainTexCoordBase = 1.0f / (float) (CTerrainImpl::PATCH_XSIZE * CTerrainImpl::CELLSCALE);
		if (!GetNewTexSetName().empty())
		{
			TexSetInit(GetNewTexSetName().c_str(), fTerrainTexCoordBase, skipTSSave);
		}

		if (!skipTSSave && globals::dft::NEWMAP_TEXTURESETSAVEASMAPNAME)
		{
			std::string newTextureSet = (std::string("textureset")+"\\"+c_rstrFolder+".txt");
			TexSetInit(newTextureSet.c_str(), fTerrainTexCoordBase, skipTSSave);
		}
		else if (!skipTSSave)
#endif
		{
			skipTSSave = true;
			LogBox("TextureSet is empty! TextureSet.Save skipped! (note: OK when creating new maps!)", "Info");
		}
	}
	// if (globals::dft::NEWMAP_TEXTURESETSAVEASMAPNAME)
	// {
		// m_TextureSet.SetFileName((std::string("textureset")+"\\"+c_rstrFolder+".txt").c_str());
	// }

	std::string strfileName = c_rstrFolder;
	strfileName += "\\Setting.txt";

	FILE * File = fopen(strfileName.c_str(), "w");

	if (!File)
		return false;

	fprintf(File, "ScriptType\tMapSetting\n");
	fprintf(File, "\n");

	fprintf(File, "CellScale\t%ld\n", CTerrainImpl::CELLSCALE);
	fprintf(File, "HeightScale\t%f\n", 0.5f); // @fixme102 GetHeightScale() -> 0.5f
	fprintf(File, "\n");

	fprintf(File, "ViewRadius\t%ld\n", 128); // @fixme102 GetViewRadius() -> 128
	fprintf(File, "\n");

	int16_t sTerrainCountX, sTerrainCountY;
	GetTerrainCount(&sTerrainCountX, &sTerrainCountY);

	if (0 != sTerrainCountX && 0 != sTerrainCountY)
		fprintf(File, "MapSize\t%u\t%u\n", sTerrainCountX, sTerrainCountY);
	else
		fprintf(File, "MapSize\t%u\t%u\n", 1, 1);

	fprintf(File, "BasePosition\t%u\t%u\n", m_dwBaseX, m_dwBaseY);
	fprintf(File, "TextureSet\t%s\n", m_TextureSet.GetFileName());
	fprintf(File, "Environment\t%s\n", GetEnvironmentDataName().c_str());
	fprintf(File, "\n");

#ifdef ENABLE_MAP_ENCRYPTION
	if (IsEncryptionSet())
	{
		fprintf(File, "EncryptionSet\t%u\n", GetEncryptionSet());
		fprintf(File, "\n");
	}
#endif

#ifdef ENABLE_SPECIAL_WATER
	if (IsSpecialWater())
	{
		fprintf(File, "SpecialWaterPath\t\"%s\"\n", GetSpecialWaterPath());
		fprintf(File, "SpecialWaterCount\t%u\n", GetSpecialWaterCount());
	fprintf(File, "\n");
	}
#endif

	fclose(File);

	// @fixme101
	if (!skipTSSave)
	{
		if(!(m_TextureSet.Save(m_TextureSet.GetFileName())))
			return false;
	}

	return true;
}

bool CMapOutdoorAccessor::SaveTerrains()
{
	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CTerrainAccessor * pTerrainAccessor;

		if (!GetTerrainPointer(i, (CTerrain **) &pTerrainAccessor))
			continue;

		if (NULL != pTerrainAccessor)
		{
			uint16_t usCoordX, usCoordY;
  			pTerrainAccessor->GetCoordinate(&usCoordX, &usCoordY);

			// Save Property
 			if (!pTerrainAccessor->SaveProperty(m_strName))
			{
				LogBoxf("[%s] failed to save Property at X:%d Y:%d", m_strName.c_str(), usCoordX, usCoordY);
				return false;
			}

			// Save HeightFieldMap
			if (!pTerrainAccessor->SaveHeightMap(m_strName))
			{
				LogBoxf("Failed to save HeightMap X:%d  Y:%d", usCoordX, usCoordY);
				return false;
			}

			// Save TileMap
			if (!pTerrainAccessor->RAW_SaveTileMap(m_strName))
			{
				LogBoxf("Failed to save TileMap X:%d Y:%d", usCoordX, usCoordY);
				return false;
			}

			if (!pTerrainAccessor->SaveWaterMap(m_strName))
			{
				LogBoxf("Failed to save WaterMap X:%d Y:%d", usCoordX, usCoordY);
				return false;
			}

			if (!pTerrainAccessor->SaveAttrMap(m_strName))
			{
				LogBoxf("Failed to save AttrMap X:%d Y:%d", usCoordX, usCoordY);
				return false;
			}
		}
	}
	return true;
}

bool CMapOutdoorAccessor::SaveAreas()
{
	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CAreaAccessor * pAreaAccessor;

		if (GetAreaPointer(i, (CArea **) &pAreaAccessor))
		{
			uint16_t usCoordX, usCoordY;
			pAreaAccessor->GetCoordinate(&usCoordX, &usCoordY);

			// Save HeightFieldMap
			if (!pAreaAccessor->Save(m_strName))
			{
				LogBoxf("Failed to save AreaData X:%d Y:%d", usCoordX, usCoordY);
				return false;
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
int32_t CMapOutdoorAccessor::GetHeightPixel(
	uint16_t wTerrainNumX,
	uint16_t wTerrainNumY,
	int32_t lCellX,
	int32_t lCellY)
{
	uint8_t ucTerrainNum;
	if (!GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &ucTerrainNum))
		return -1;

	CTerrainAccessor* pTerrainAccessor = NULL;
	if (GetTerrainPointer(ucTerrainNum, (CTerrain**)&pTerrainAccessor))
		return pTerrainAccessor->GetHeightPixel(lCellX, lCellY);

	return -1;
}

void CMapOutdoorAccessor::DrawHeightPixel(int32_t lHeight,
	uint16_t wTerrainNumX,
	uint16_t wTerrainNumY,
	int32_t lCellX,
	int32_t lCellY)
{
	uint8_t ucTerrainNum;
	if (!GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &ucTerrainNum))
		return;

	CTerrainAccessor* pTerrainAccessor = NULL;
	if (GetTerrainPointer(ucTerrainNum, (CTerrain**)&pTerrainAccessor))
		pTerrainAccessor->DrawHeightPixel(lHeight, lCellX, lCellY);

	SetTerrainModified();
}

void CMapOutdoorAccessor::DrawHeightBrush(uint32_t dwBrushShape,
					 uint32_t dwBrushType,
					 uint16_t wTerrainNumX,
					 uint16_t wTerrainNumY,
					 int32_t lCellX,
					 int32_t lCellY,
					 uint8_t byBrushSize,
					 uint8_t byBrushStrength)
{
	uint8_t ucTerrainNum;
	if ( !GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &ucTerrainNum) )
		return;

	CTerrainAccessor * pTerrainAccessor = NULL;


	if (lCellY < byBrushSize)
	{
		if (lCellX < byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum - 4, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX + CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum - 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY + CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum - 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX + CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
		}
		else if (lCellX > CTerrainImpl::XSIZE - 1 - byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum - 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY + CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum - 2, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX - CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX - CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
		}
		else
		{
			if ( GetTerrainPointer(ucTerrainNum - 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY + CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
		}
	}
	else if (lCellY > (CTerrainImpl::XSIZE - 1) - byBrushSize)
	{
		if (lCellX < byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum - 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX + CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 2, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX + CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY - CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX - CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY - CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 4, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX - CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
		}
		else
		{
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 3, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY - CTerrainImpl::YSIZE, byBrushSize, byBrushStrength);
		}
	}
	else
	{
		if (lCellX < byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum - 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX + CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
			if ( GetTerrainPointer(ucTerrainNum + 1, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX - CTerrainImpl::XSIZE, lCellY, byBrushSize, byBrushStrength);
		}
		else
		{
			if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
				pTerrainAccessor->DrawHeightBrush(dwBrushShape, dwBrushType, lCellX, lCellY, byBrushSize, byBrushStrength);
		}
	}

	SetTerrainModified();
}

void CMapOutdoorAccessor::DrawTextureBrush(uint32_t dwBrushShape,
										   std::vector<uint8_t> & rVectorTextureNum,
										   uint16_t wTerrainNumX,
										   uint16_t wTerrainNumY,
										   int32_t lCellX,
										   int32_t lCellY,
										   uint8_t bySubCellX,
										   uint8_t bySubCellY,
										   uint8_t byBrushSize,
										   bool bErase,
										   bool bDrawOnlyOnBlankTile)
{
	CTerrainAccessor * pTerrainAccessor = NULL;
	uint8_t byTerrainNum;
	if(!GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &byTerrainNum))
		return;

	if (GetTerrainPointer(byTerrainNum, (CTerrain **) &pTerrainAccessor))
		pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);

	if (lCellX < byBrushSize + 1)
	{
		if (GetTerrainPointer(byTerrainNum - 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX + CTerrainImpl::XSIZE, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
	}
	else if (lCellX > (CTerrainImpl::XSIZE - 2) - byBrushSize)
	{
		if (GetTerrainPointer(byTerrainNum + 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX - CTerrainImpl::XSIZE, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
	}

	if (lCellY < byBrushSize + 1)
	{
		if (GetTerrainPointer(byTerrainNum - 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);

		if (lCellX < byBrushSize + 1)
		{
			if (GetTerrainPointer(byTerrainNum - 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX + CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 2) - byBrushSize)
		{
			if (GetTerrainPointer(byTerrainNum - 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX - CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
		}
	}
	else if (lCellY > (CTerrainImpl::YSIZE - 2) - byBrushSize)
	{
		if (GetTerrainPointer(byTerrainNum + 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);

		if (lCellX < byBrushSize + 1)
		{
			if (GetTerrainPointer(byTerrainNum + 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX + CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 2) - byBrushSize)
		{
			if (GetTerrainPointer(byTerrainNum + 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawTextureBrush(dwBrushShape, rVectorTextureNum, lCellX - CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase, bDrawOnlyOnBlankTile);
		}
	}

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		if (!GetTerrainPointer(i, (CTerrain **) &pTerrainAccessor))
			continue;

		pTerrainAccessor->RAW_GenerateSplat();
	}
}

void CMapOutdoorAccessor::ResetAttrSplats()
{
	CTerrainAccessor * pTerrainAccessor = NULL;

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		if (!GetTerrainPointer(i, (CTerrain **) &pTerrainAccessor))
			continue;

		pTerrainAccessor->RAW_ResetAttrSplat();
	}
}

void CMapOutdoorAccessor::DrawAttrBrush(uint32_t dwBrushShape,
										uint8_t byAttrFlag,
										uint16_t wTerrainNumX,
										uint16_t wTerrainNumY,
										int32_t lCellX,
										int32_t lCellY,
										uint8_t bySubCellX,
										uint8_t bySubCellY,
										uint8_t byBrushSize,
										bool bErase)
{
	CTerrainAccessor * pTerrainAccessor = NULL;
	uint8_t ucTerrainNum = (wTerrainNumY - m_CurCoordinate.m_sTerrainCoordY + 1) * 3 + (wTerrainNumX - m_CurCoordinate.m_sTerrainCoordX + 1);

	if ( GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor) )
		pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase);

	if (lCellY < byBrushSize)
	{
		if (wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		if (lCellX < byBrushSize)
		{
			if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX + CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX - CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		}
	}
	else if (lCellY > (CTerrainImpl::YSIZE - 1) - byBrushSize)
	{
		if (wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		if (lCellX < byBrushSize)
		{
			if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX + CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX - CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, bySubCellX, bySubCellY, byBrushSize, bErase);
		}
	}

	if (lCellX < byBrushSize)
	{
		if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && GetTerrainPointer(ucTerrainNum - 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX + CTerrainImpl::XSIZE, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase);
	}
	else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
	{
		if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && GetTerrainPointer(ucTerrainNum + 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawAttrBrush(dwBrushShape, byAttrFlag, lCellX - CTerrainImpl::XSIZE, lCellY, bySubCellX, bySubCellY, byBrushSize, bErase);
	}

	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		if (!GetTerrainPointer(i, (CTerrain **) &pTerrainAccessor))
			continue;
		pTerrainAccessor->RAW_UpdateAttrSplat();
	}
}

void CMapOutdoorAccessor::PreviewWaterBrush(uint32_t dwBrushShape,
											uint16_t wTerrainNumX,
											uint16_t wTerrainNumY,
											int32_t lCellX,
											int32_t lCellY,
											uint8_t byBrushSize,
											uint16_t wWaterHeight,
											bool bErase)
{
	float fx, fy, fz, fLeft, fTop, fRight, fBottom;

	fx = (float)(lCellX * CTerrainImpl::CELLSCALE + wTerrainNumX * CTerrainImpl::TERRAIN_XSIZE);
	fy = (float)(lCellY * CTerrainImpl::CELLSCALE + wTerrainNumY * CTerrainImpl::TERRAIN_YSIZE);
	fz = wWaterHeight * m_fHeightScale;

	fLeft	= (float)((lCellX - byBrushSize) * CTerrainImpl::CELLSCALE + wTerrainNumX * CTerrainImpl::TERRAIN_XSIZE);
	fTop	= -(float)((lCellY - byBrushSize) * CTerrainImpl::CELLSCALE + wTerrainNumY * CTerrainImpl::TERRAIN_YSIZE);
	fRight	= (float)((lCellX + byBrushSize) * CTerrainImpl::CELLSCALE + wTerrainNumX * CTerrainImpl::TERRAIN_XSIZE);
	fBottom	= -(float)((lCellY + byBrushSize) * CTerrainImpl::CELLSCALE + wTerrainNumY * CTerrainImpl::TERRAIN_YSIZE);

	SetDiffuseColor(1.0f, 1.0f, 1.0f);
	RenderLine3d( fx, -fy, fz - 30.0f, fx, -fy, fz + 30.0f);
	RenderLine3d( fx - 30.0f, -fy, fz, fx + 30.0f, -fy, fz);
	RenderLine3d( fx, -fy - 30.0f, fz, fx, -fy + 30.0f, fz);

	RenderLine3d(fLeft, fTop, fz, fLeft, fBottom, fz);
	RenderLine3d(fRight, fTop, fz, fRight, fBottom, fz);
	RenderLine3d(fLeft, fTop, fz, fRight, fTop, fz);
	RenderLine3d(fLeft, fBottom, fz, fRight, fBottom, fz);

	int32_t count, iStep;
	float theta, delta;
	float x, y, z, fRadius;
	std::vector<D3DXVECTOR3> pts;

	fRadius = float(byBrushSize * CTerrainImpl::CELLSCALE);
	iStep = 50;

	pts.clear();
	pts.resize(iStep);

	theta = 0.0;
	delta = 2 * D3DX_PI / float(iStep);

	for (count = 0; count < iStep; ++count)
	{
		x = fx + fRadius * cosf(theta);
		y = fy + fRadius * sinf(theta);
		z = fz;

		pts[count] = D3DXVECTOR3(x, -y, z);

		theta += delta;
	}

	for (count = 0; count < iStep - 1; ++count)
	{
		RenderLine3d(pts[count].x, pts[count].y, pts[count].z, pts[count + 1].x, pts[count + 1].y, pts[count + 1].z);
	}

	RenderLine3d(pts[iStep - 1].x, pts[iStep - 1].y, pts[iStep - 1].z, pts[0].x, pts[0].y, pts[0].z);
}

void CMapOutdoorAccessor::DrawWaterBrush(uint32_t dwBrushShape,
										 uint16_t wTerrainNumX,
										 uint16_t wTerrainNumY,
										 int32_t lCellX,
										 int32_t lCellY,
										 uint8_t byBrushSize,
										 uint16_t wWaterHeight,
										 bool bErase)
{
	CTerrainAccessor * pTerrainAccessor = NULL;
	uint8_t ucTerrainNum = (wTerrainNumY - m_CurCoordinate.m_sTerrainCoordY + 1) * 3 + (wTerrainNumX - m_CurCoordinate.m_sTerrainCoordX + 1);


	if (GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor))
		pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX, lCellY, byBrushSize, wWaterHeight, bErase);

	if (lCellY < byBrushSize)
	{
		if (wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX, lCellY + CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		if (lCellX < byBrushSize)
		{
			if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX + CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY >= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum - 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX - CTerrainImpl::XSIZE, lCellY + CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		}
	}
	else if (lCellY > (CTerrainImpl::YSIZE - 1) - byBrushSize)
	{
		if (wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 3, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX, lCellY - CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		if (lCellX < byBrushSize)
		{
			if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 2, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX + CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		}
		else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
		{
			if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && wTerrainNumY <= m_CurCoordinate.m_sTerrainCoordY && GetTerrainPointer(ucTerrainNum + 4, (CTerrain **) &pTerrainAccessor))
				pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX - CTerrainImpl::XSIZE, lCellY - CTerrainImpl::YSIZE, byBrushSize, wWaterHeight, bErase);
		}
	}

	if (lCellX < byBrushSize)
	{
		if (wTerrainNumX >= m_CurCoordinate.m_sTerrainCoordX && GetTerrainPointer(ucTerrainNum - 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX + CTerrainImpl::XSIZE, lCellY, byBrushSize, wWaterHeight, bErase);
	}
	else if (lCellX > (CTerrainImpl::XSIZE - 1) - byBrushSize)
	{
		if (wTerrainNumX <= m_CurCoordinate.m_sTerrainCoordX && GetTerrainPointer(ucTerrainNum + 1, (CTerrain **) &pTerrainAccessor))
			pTerrainAccessor->DrawWaterBrush(dwBrushShape, lCellX - CTerrainImpl::XSIZE, lCellY, byBrushSize, wWaterHeight, bErase);
	}

	SetTerrainModified();
}

//////////////////////////////////////////////////////////////////////////

int32_t CMapOutdoorAccessor::GetMapID()
{
	return m_iMapID;
}

void CMapOutdoorAccessor::SetMapID(int32_t iID)
{
	m_iMapID = iID;
}

void CMapOutdoorAccessor::SetTerrainModified()
{
	ResetTerrainPatchVertexBuffer();
	m_lOldReadX = m_lOldReadY = -1;
}

bool CMapOutdoorAccessor::CreateNewTerrainFiles(uint16_t wCoordX, uint16_t wCoordY)
{
	CTerrainAccessor * pTerrainAccessor = new CTerrainAccessor();

	pTerrainAccessor->SetMapOutDoor(this);
	pTerrainAccessor->SetCoordinate(wCoordX, wCoordY);

	char szTerrainFolder[256];
	uint32_t ulID = (uint32_t)(wCoordX) * 1000L + (uint32_t)(wCoordY);
	sprintf(szTerrainFolder, "%s\\%06u", m_strName.c_str(), ulID);

	struct _stat sb;

	if (0 != _stat(szTerrainFolder, &sb))
	{
		// Make The Directory
		if (!CreateDirectory(szTerrainFolder, NULL))
		{
			LogBoxf("Save Failed : [%s] Directory cannot be saved", szTerrainFolder);
			return false;
		}
	}

	if (0 != _stat(szTerrainFolder, &sb))
	{
		LogBoxf("Save Failed : Disk Error", szTerrainFolder);
		return false;
	}

	if (!(sb.st_mode & _S_IFDIR))
	{
		LogBoxf("Save Failed : [%s] File with the same name", szTerrainFolder);
		return false;
	}

	// Save Property
	if (!pTerrainAccessor->SaveProperty(m_strName))
	{
		LogBoxf("Property failed to save in %d, %d", wCoordX, wCoordY);
		return false;
	}

	// Save HeightFieldMap
	if (!pTerrainAccessor->NewHeightMap(m_strName))
	{
		LogBoxf("HeightFieldMap failed to save in %d, %d", wCoordY, wCoordY);
		return false;
	}

	// Save TileMap
 	if (!pTerrainAccessor->NewTileMap(m_strName))
	{
		LogBoxf("TileMap failed to save in %d, %d", wCoordY, wCoordY);
		return false;
	}

	// Save AttrMap
 	if (!pTerrainAccessor->NewAttrMap(m_strName))
	{
		LogBoxf("AttrMap failed to save in %d, %d", wCoordY, wCoordY);
		return false;
	}

	delete pTerrainAccessor;
	pTerrainAccessor = NULL;
	return true;
}

BOOL CMapOutdoorAccessor::GetAreaAccessor(uint32_t dwIndex, CAreaAccessor ** ppAreaAccessor)
{
	if (dwIndex >= AROUND_AREA_NUM)
	{
		*ppAreaAccessor = NULL;
		return FALSE;
	}

	CArea * pArea;

	if (!GetAreaPointer(dwIndex, &pArea))
	{
		*ppAreaAccessor = NULL;
		return FALSE;
	}

	*ppAreaAccessor = (CAreaAccessor *) pArea;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
// Utility
bool CMapOutdoorAccessor::RAW_InitBaseTexture(const std::vector<uint8_t> & c_rVectorBaseTexture)
{
	CTerrainAccessor * pTerrainAccessor = new CTerrainAccessor();
	pTerrainAccessor->SetMapOutDoor(this);

	for (uint16_t usY = 0; usY < m_sTerrainCountY; ++usY)
	{
		for (uint16_t usX = 0; usX < m_sTerrainCountX; ++usX)
		{
			pTerrainAccessor->SetCoordinate(usX, usY);

			char szTerrainFolder[256], szRAWTileMapName[256];
			uint32_t ulID = (uint32_t)(usX) * 1000L + (uint32_t)(usY);
			sprintf(szTerrainFolder, "%s\\%06u", m_strName.c_str(), ulID);

			struct _stat sb;

			if (0 != _stat(szTerrainFolder, &sb))
			{
				LogBoxf("Initialization Failed : [%s] is not a directory", szTerrainFolder);
				return false;
			}

			if (!(sb.st_mode & _S_IFDIR))
			{
				LogBoxf("Initialization Failed : [%s] File with the same name", szTerrainFolder);
				return false;
			}

			sprintf(szRAWTileMapName, "%s\\tile.raw", szTerrainFolder);

			if (!pTerrainAccessor->RAW_LoadAndSaveTileMap(szRAWTileMapName, m_strName, c_rVectorBaseTexture))
			{
				LogBoxf("ShadowMap failed to load and save in %d, %d", usX, usY);
				return false;
			}
		}
	}
	delete pTerrainAccessor;
	pTerrainAccessor = NULL;
	return true;
}
//////////////////////////////////////////////////////////////////////////

void CMapOutdoorAccessor::RenderAccessorTerrain(uint8_t byRenderMode, uint8_t byAttrFlag)
{
	if (!m_pTerrainPatchProxyList)
		return;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.SetTransform(D3DTS_WORLD, &m_matWorldForCommonUse);

	uint16_t wPrimitiveCount;
	D3DPRIMITIVETYPE eType;
	SelectIndexBuffer(0, &wPrimitiveCount, &eType);

	switch (byRenderMode)
	{
		case RENDER_SHADOW:
			{
				STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

				RecurseRenderAccessorTerrain(m_pRootNode, byRenderMode, byAttrFlag, false);
			}
			break;

		case RENDER_ATTR:
			{
				D3DXMATRIX matTexTransform, matTexTransformTemp;

				D3DXMatrixScaling(&matTexTransform, m_fTerrainTexCoordBase * 32.0f, -m_fTerrainTexCoordBase * 32.0f, 0.0f);
				D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &matTexTransform);
				STATEMANAGER.SaveTransform(D3DTS_TEXTURE0, &matTexTransform);
				STATEMANAGER.SaveTransform(D3DTS_TEXTURE1, &matTexTransform);

				STATEMANAGER.SetTexture(0, m_attrImageInstance.GetTexturePointer()->GetD3DTexture());

				STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
				STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
				STATEMANAGER.SaveTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
				STATEMANAGER.SaveTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

				STATEMANAGER.SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
				STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
				STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
				STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
				STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_MINFILTER,	D3DTEXF_POINT);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_MAGFILTER,	D3DTEXF_POINT);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_MIPFILTER,	D3DTEXF_POINT);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_ADDRESSU,	D3DTADDRESS_CLAMP);
				STATEMANAGER.SaveTextureStageState(1, D3DTSS_ADDRESSV,	D3DTADDRESS_CLAMP);

				RecurseRenderAccessorTerrain(m_pRootNode, byRenderMode, byAttrFlag);

				STATEMANAGER.RestoreTextureStageState(0, D3DTSS_TEXCOORDINDEX);
				STATEMANAGER.RestoreTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_TEXCOORDINDEX);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_MINFILTER);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_MAGFILTER);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_MIPFILTER);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_ADDRESSU);
				STATEMANAGER.RestoreTextureStageState(1, D3DTSS_ADDRESSV);

				STATEMANAGER.RestoreTransform(D3DTS_TEXTURE0);
				STATEMANAGER.RestoreTransform(D3DTS_TEXTURE1);
			}
			break;
	}
}

void CMapOutdoorAccessor::RecurseRenderAccessorTerrain(CTerrainQuadtreeNode *Node, uint8_t byRenderMode, uint8_t byAttrFlag, bool bCullEnable)
{
	if (bCullEnable)
	{
		if (__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(Node->center, Node->radius)==VIEW_NONE)
			return;
	}
	{
		if (Node->Size == 1)
		{
			switch(byRenderMode)
			{
				case RENDER_SHADOW:
					if (4 == m_pTerrainPatchProxyList[Node->PatchNum].GetTerrainNum())
						DrawMeshOnly(Node->PatchNum);
					break;

				case RENDER_ATTR:
					DrawPatchAttr(Node->PatchNum, byAttrFlag);
					if (m_bDrawWireFrame)
	 					DrawWireFrame(Node->PatchNum, m_wNumIndices - 2, D3DPT_TRIANGLESTRIP);
					break;
			}
		}
		else
		{
			if (Node->NW_Node != NULL)
				RecurseRenderAccessorTerrain(Node->NW_Node, byRenderMode, byAttrFlag, bCullEnable);
			if (Node->NE_Node != NULL)
				RecurseRenderAccessorTerrain(Node->NE_Node, byRenderMode, byAttrFlag, bCullEnable);
			if (Node->SW_Node != NULL)
				RecurseRenderAccessorTerrain(Node->SW_Node, byRenderMode, byAttrFlag, bCullEnable);
			if (Node->SE_Node != NULL)
				RecurseRenderAccessorTerrain(Node->SE_Node, byRenderMode, byAttrFlag, bCullEnable);
		}
 	}
}

void CMapOutdoorAccessor::DrawMeshOnly(int32_t patchnum)
{
	CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	if (!pTerrainPatchProxy->isUsed())
		return;

	int32_t sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;
	uint8_t ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	STATEMANAGER.SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL);
	STATEMANAGER.SetStreamSource(0, pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr()->GetD3DVertexBuffer(), m_iPatchTerrainVertexSize);
	STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_iPatchTerrainVertexCount, 0, m_wNumIndices -2);
}

void CMapOutdoorAccessor::RenderShadow()
{
	RenderAccessorTerrain(RENDER_SHADOW, 0);
}

void CMapOutdoorAccessor::DrawPatchAttr(int32_t patchnum, uint8_t byAttrFlag)
{
	CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	if (!pTerrainPatchProxy->isUsed())
		return;

	int32_t sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	uint8_t ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	// Deal with this material buffer
	CTerrainAccessor * pTerrainAccessor;
	if (!GetTerrainPointer(ucTerrainNum, (CTerrain **) &pTerrainAccessor))
		return;

	uint16_t wCoordX, wCoordY;
	pTerrainAccessor->GetCoordinate(&wCoordX, &wCoordY);

	m_matWorldForCommonUse._41 = -(float) (wCoordX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
	m_matWorldForCommonUse._42 = (float) (wCoordY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);

	D3DXMATRIX matTexTransform, matTexTransformTemp;
	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matTexTransform, &matTexTransform, &m_matStaticShadow);
	STATEMANAGER.SetTransform(D3DTS_TEXTURE1, &matTexTransform);

	TTerrainSplatPatch & rAttrSplatPatch = pTerrainAccessor->RAW_GetAttrSplatPatch();
 	STATEMANAGER.SetTexture(1, rAttrSplatPatch.Splats[0].pd3dTexture);

	STATEMANAGER.SetVertexShader(D3DFVF_XYZ | D3DFVF_NORMAL);
	STATEMANAGER.SetStreamSource(0, pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr()->GetD3DVertexBuffer(), m_iPatchTerrainVertexSize);
	STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, m_iPatchTerrainVertexCount, 0, m_wNumIndices -2);
}

bool CMapOutdoorAccessor::ResetToDefaultAttr()
{
	CTerrainAccessor * pTerrainAccessor = new CTerrainAccessor();
	pTerrainAccessor->SetMapOutDoor(this);
	pTerrainAccessor->CopySettingFromGlobalSetting();

	for (uint16_t usY = 0; usY < m_sTerrainCountY; ++usY)
	{
		for (uint16_t usX = 0; usX < m_sTerrainCountX; ++usX)
		{
			pTerrainAccessor->SetCoordinate(usX, usY);

			char szTerrainFolder[256];
			uint32_t ulID = (uint32_t)(usX) * 1000L + (uint32_t)(usY);
			sprintf(szTerrainFolder, "%s\\%06u", m_strName.c_str(), ulID);

			struct _stat sb;

			if (0 != _stat(szTerrainFolder, &sb))
			{
				LogBoxf("Initialization Failed : [%s] is not a directory", szTerrainFolder);
				return false;
			}

			if (!(sb.st_mode & _S_IFDIR))
			{
				LogBoxf("Initialization Failed : [%s] File with the same name", szTerrainFolder);
				return false;
			}

			if(!pTerrainAccessor->RAW_LoadAndSaveDefaultAttrMap(m_strName))
			{
				LogBoxf("Attr failed to load and save in %d, %d", usX, usY);
				return false;
			}
		}
	}
	delete pTerrainAccessor;
	pTerrainAccessor = NULL;
	return true;
}

bool CMapOutdoorAccessor::Destroy()
{
	__DestroyProgressDialog();

	stl_wipe(m_TerrainVector);
	stl_wipe(m_TerrainDeleteVector);
	stl_wipe(m_AreaVector);
	stl_wipe(m_AreaDeleteVector);

	CMapOutdoor::Destroy();
	return true;
}

bool CMapOutdoorAccessor::Load(float x, float y, float z)
{
	Destroy();

	std::string strFileName = GetName() + "\\Setting.txt";
	if (!LoadSetting(strFileName.c_str()))
	{
		LogBoxf("CMapOutdoorAccessor::Load : Failed to read file %s", strFileName.c_str());
		return false;
	}

	LoadMonsterAreaInfo();
	CreateTerrainPatchProxyList();
	BuildQuadTree();
	LoadWaterTexture();
	CreateCharacterShadowTexture();

	LoadGuildAreaList("GuildAreaList.txt");

	m_lOldReadX = -1;

	CSpeedTreeForestDirectX8::Instance().SetRenderingDevice(ms_lpd3dDevice);

	Update(x, y, z);

	m_envDataName = m_settings_envDataName;

	return true;
}

bool CMapOutdoorAccessor::LoadGuildAreaList(const char * c_szFileName)
{
	FILE * file = fopen(c_szFileName, "r");
	if (!file)
		return false;

	char szLine[1024+1];
	while (fgets(szLine, 1024, file))
	{
		int32_t iID;
		int32_t iMapIndex;
		uint32_t dwsx;
		uint32_t dwsy;
		uint32_t dwWidth;
		uint32_t dwHeight;
		uint32_t dwGuildID;
		uint32_t dwGuildLevelLimit;
		uint32_t dwPrice;
		sscanf(szLine, "%d %d %d %d %d %d %d %d %d", &iID, &iMapIndex, &dwsx, &dwsy, &dwWidth, &dwHeight, &dwGuildID, &dwGuildLevelLimit, &dwPrice);

		if (iMapIndex == GetMapID())
		{
			RegisterGuildArea(dwsx, dwsy, dwsx+dwWidth, dwsy+dwHeight);
		}
	}

	fclose(file);

	VisibleMarkedArea();
	return true;
}

void CMapOutdoorAccessor::RenderObjectCollision()
{
	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CAreaAccessor * pAreaAccessor;
		if (!GetAreaPointer(i, (CArea **) &pAreaAccessor) )
			continue;
		pAreaAccessor->RenderCollision();
	}
}

//////////////////////////////////////////////////////////////////////////
// Shadow Map
void CMapOutdoorAccessor::RenderToShadowMap()
{
	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CAreaAccessor * pAreaAccessor;

		if (GetAreaPointer(i, (CArea **) &pAreaAccessor))
		{
			pAreaAccessor->RenderToShadowMap();
		}
	}
}

bool CMapOutdoorAccessor::LoadArea(uint16_t wAreaCoordX, uint16_t wAreaCoordY, uint16_t wCellCoordX, uint16_t wCellCoordY)
{
	if (isAreaLoaded(wAreaCoordX, wAreaCoordY))
		return true;

	uint32_t dwStartTime = timeGetTime();

	uint32_t ulID = (uint32_t) (wAreaCoordX) * 1000L + (uint32_t) (wAreaCoordY);
	char szAreaDataName[64];
	_snprintf(szAreaDataName,	64, "%s\\%06u\\", m_strName.c_str(), ulID);

	CAreaAccessor * pAreaAccessor = new CAreaAccessor;

	pAreaAccessor->SetMapOutDoor(this);
	pAreaAccessor->SetCoordinate(wAreaCoordX, wAreaCoordY);
	pAreaAccessor->Load(szAreaDataName);

	m_AreaVector.push_back(pAreaAccessor);

	Tracef("CMapOutdoor::LoadArea %d\n", timeGetTime() - dwStartTime);

	return true;
}

void CMapOutdoorAccessor::__ClearGarvage()
{
	if (m_bNowAccessGarvage)
	{
		return;
	}

	m_bNowAccessGarvage = TRUE;
	stl_wipe(m_TerrainDeleteVector);
	stl_wipe(m_AreaDeleteVector);
	m_bNowAccessGarvage = FALSE;
}

void CMapOutdoorAccessor::__UpdateGarvage()
{
	if (m_bNowAccessGarvage)
	{
		return;
	}

	m_bNowAccessGarvage = TRUE;

	const uint32_t dwTerrainEraseInterval = 1000;
	static uint32_t dwEraseTime = timeGetTime();
	if (!m_TerrainDeleteVector.empty())
	{
		if (timeGetTime() - dwEraseTime <= dwTerrainEraseInterval)
			return;

		TTerrainPtrVector::iterator itor = m_TerrainDeleteVector.begin();
		CTerrainAccessor * pTerrainAccessor = (CTerrainAccessor *) *itor;
		m_TerrainDeleteVector.erase(itor);

		delete pTerrainAccessor;
		pTerrainAccessor = NULL;

		// @fixme113
		Tracef("Accessor Delete Terrain / Left Count : %d ms / %d\n", timeGetTime() - dwEraseTime, m_TerrainDeleteVector.size());
		dwEraseTime = timeGetTime();
		return;
	}

	if (!m_AreaDeleteVector.empty())
	{
		if (timeGetTime() - dwEraseTime <= dwTerrainEraseInterval)
			return;

		TAreaPtrVector::iterator itor = m_AreaDeleteVector.begin();
		CAreaAccessor * pAreaAccessor = (CAreaAccessor *) *m_AreaDeleteVector.begin();
		m_AreaDeleteVector.erase(itor);

		delete pAreaAccessor;
		pAreaAccessor = NULL;

		Tracef("Delete Area [%d]\n", timeGetTime() - dwEraseTime);
		dwEraseTime = timeGetTime();
		return;
	}

	m_bNowAccessGarvage = FALSE;
}

bool CMapOutdoorAccessor::LoadTerrain(uint16_t wX, uint16_t wY, uint16_t wCellCoordX, uint16_t wCellCoordY)
{
	if (isTerrainLoaded(wX, wY))
		return true;


	uint32_t ulID = (uint32_t) (wX) * 1000L + (uint32_t) (wY);
	char filename[256];
	sprintf(filename, "%s\\%06u\\AreaProperty.txt", m_strName.c_str(), ulID);

	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(filename, stTokenVectorMap))
		return false;

	if (stTokenVectorMap.end() == stTokenVectorMap.find("scripttype"))
		return false;

	if (stTokenVectorMap.end() == stTokenVectorMap.find("areaname"))
		return false;

	const std::string & c_rstrType = stTokenVectorMap["scripttype"][0];
	const std::string & c_rstrAreaName = stTokenVectorMap["areaname"][0];

	if (c_rstrType != "AreaProperty")
		return false;

	CTerrainAccessor * pTerrainAccessor = new CTerrainAccessor();
	pTerrainAccessor->SetMapOutDoor(this);

	char szTileMapName[64];
	char szRAWTileMapName[64];
	char szWaterMapName[64];
	char szAttrMapName[64];
	char szShadowTexName[64];
	char szShadowMapName[64];
	char szRawHeightFilename[64];

	_snprintf(szRawHeightFilename, 64, "%s\\%06u\\height.raw", m_strName.c_str(), ulID);
	_snprintf(szRAWTileMapName,	64, "%s\\%06u\\tile.raw", m_strName.c_str(), ulID);
	_snprintf(szTileMapName,	64, "%s\\%06u\\color.cmp", m_strName.c_str(), ulID);
	_snprintf(szAttrMapName,	64, "%s\\%06u\\attr.atr", m_strName.c_str(), ulID);
	_snprintf(szWaterMapName,	64, "%s\\%06u\\water.wtr", m_strName.c_str(), ulID);
	_snprintf(szShadowTexName,	64, "%s\\%06u\\shadowmap.dds", m_strName.c_str(), ulID);
	_snprintf(szShadowMapName,	64, "%s\\%06u\\shadowmap.raw", m_strName.c_str(), ulID);

	pTerrainAccessor->Clear();

	pTerrainAccessor->SetCoordinate(wX, wY);

	pTerrainAccessor->CopySettingFromGlobalSetting();

	pTerrainAccessor->LoadWaterMap(szWaterMapName);
	pTerrainAccessor->LoadHeightMap(szRawHeightFilename);
	pTerrainAccessor->LoadAttrMap(szAttrMapName);
	pTerrainAccessor->RAW_LoadTileMap(szRAWTileMapName);
	pTerrainAccessor->LoadShadowTexture(szShadowTexName);
	pTerrainAccessor->LoadShadowMap(szShadowMapName);
	pTerrainAccessor->SetName(c_rstrAreaName.c_str());
	pTerrainAccessor->CalculateTerrainPatch();

	// Attribute Map
	pTerrainAccessor->RAW_AllocateAttrSplats();

	pTerrainAccessor->SetReady();

	m_TerrainVector.push_back(pTerrainAccessor);

	return true;
}

void CMapOutdoorAccessor::ResetTerrainPatchVertexBuffer()
{
	for (int32_t i = 0; i < m_wPatchCount * m_wPatchCount; ++i)
		m_pTerrainPatchProxyList[i].SetTerrainPatch(NULL);
	for (uint8_t byTerrainNum = 0; byTerrainNum < AROUND_AREA_NUM; ++byTerrainNum)
	{
		CTerrain * pTerrain;
		if (GetTerrainPointer(byTerrainNum, &pTerrain))
			pTerrain->CalculateTerrainPatch();
	}
}

void CMapOutdoorAccessor::ResetTextures()
{
	m_TextureSet.Reload(m_fTerrainTexCoordBase);
}

void CMapOutdoorAccessor::UpdateAreaList(int32_t lCenterX, int32_t lCenterY)
{
	__ClearGarvage();

	if (m_TerrainVector.size() <= AROUND_AREA_NUM && m_AreaVector.size() <= AROUND_AREA_NUM)
		return;

	FPushToDeleteVector::EDeleteDir eDeleteLRDir, eDeleteTBDir;

	if (lCenterX > CTerrainImpl::XSIZE / 2)
		eDeleteLRDir = FPushToDeleteVector::DELETE_LEFT;
	else
		eDeleteLRDir = FPushToDeleteVector::DELETE_RIGHT;
	if (lCenterY > CTerrainImpl::YSIZE / 2)
		eDeleteTBDir = FPushToDeleteVector::DELETE_TOP;
	else
		eDeleteTBDir = FPushToDeleteVector::DELETE_BOTTOM;

	FPushTerrainToDeleteVector &rPushTerrainToDeleteVector = std::for_each(m_TerrainVector.begin(), m_TerrainVector.end(),
		FPushTerrainToDeleteVector(eDeleteLRDir, eDeleteTBDir, m_CurCoordinate));
	FPushAreaToDeleteVector &rPushAreaToDeleteVector = std::for_each(m_AreaVector.begin(), m_AreaVector.end(),
		FPushAreaToDeleteVector(eDeleteLRDir, eDeleteTBDir, m_CurCoordinate));

	if (!rPushTerrainToDeleteVector.m_ReturnTerrainVector.empty())
	{
		m_TerrainDeleteVector.resize(rPushTerrainToDeleteVector.m_ReturnTerrainVector.size());
		std::copy(rPushTerrainToDeleteVector.m_ReturnTerrainVector.begin(), rPushTerrainToDeleteVector.m_ReturnTerrainVector.end(), m_TerrainDeleteVector.begin());

		for (uint32_t dwIndex = 0; dwIndex < rPushTerrainToDeleteVector.m_ReturnTerrainVector.size(); ++dwIndex)
		{
			TTerrainPtrVectorIterator aTerrainPtrItertor = m_TerrainVector.begin();
			while(aTerrainPtrItertor != m_TerrainVector.end())
			{
				CTerrainAccessor * pTerrainAccessor = (CTerrainAccessor *) *aTerrainPtrItertor;
				if (pTerrainAccessor == (CTerrainAccessor *) rPushTerrainToDeleteVector.m_ReturnTerrainVector[dwIndex])
					aTerrainPtrItertor = m_TerrainVector.erase(aTerrainPtrItertor);
				else
					++aTerrainPtrItertor;
			}
		}
	}
	if (!rPushAreaToDeleteVector.m_ReturnAreaVector.empty())
	{
		m_AreaDeleteVector.resize(rPushAreaToDeleteVector.m_ReturnAreaVector.size());
		std::copy(rPushAreaToDeleteVector.m_ReturnAreaVector.begin(), rPushAreaToDeleteVector.m_ReturnAreaVector.end(), m_AreaDeleteVector.begin());

		for (uint32_t dwIndex = 0; dwIndex < rPushAreaToDeleteVector.m_ReturnAreaVector.size(); ++dwIndex)
		{
			TAreaPtrVectorIterator aAreaPtrItertor = m_AreaVector.begin();
			while(aAreaPtrItertor != m_AreaVector.end())
			{
				CAreaAccessor * pAreaAccessor = (CAreaAccessor *) *aAreaPtrItertor;
				if (pAreaAccessor == (CAreaAccessor *) rPushAreaToDeleteVector.m_ReturnAreaVector[dwIndex])
					aAreaPtrItertor = m_AreaVector.erase(aAreaPtrItertor);
				else
					++aAreaPtrItertor;
			}
		}
	}
}

void CMapOutdoorAccessor::RenderMiniMap()
{
	SetInverseViewAndDynamicShaodwMatrices();
	SetDiffuseOperation();
	RenderArea(false);
	RenderDungeon();
	if (IsVisiblePart(PART_TREE))
		CSpeedTreeForestDirectX8::Instance().Render(Forest_RenderAll | Forest_RenderToMiniMap);
 	RenderTerrain();
	RenderWater();
}

bool CMapOutdoorAccessor::GetPickingCoordinate(D3DXVECTOR3 * v3IntersectPt, int32_t * piCellX, int32_t * piCellY, uint8_t * pbySubCellX, uint8_t * pbySubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY)
{
	return GetPickingCoordinateWithRay(ms_Ray, v3IntersectPt, piCellX, piCellY, pbySubCellX, pbySubCellY, pwTerrainNumX, pwTerrainNumY);
}

bool CMapOutdoorAccessor::GetPickingCoordinateWithRay(const CRay & rRay, D3DXVECTOR3 * v3IntersectPt, int32_t * piCellX, int32_t * piCellY, uint8_t * pbySubCellX, uint8_t * pbySubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY)
{
	CTerrain * pTerrain;

	D3DXVECTOR3 v3Start, v3End, v3CurPos;

	rRay.GetStartPoint(&v3Start);
	rRay.GetEndPoint(&v3End);

	float fAdd = 1.0f / (float)(CTerrainImpl::TERRAIN_XSIZE);

	float ft = 0.0f;
	while (ft < 1.0f)
	{
		D3DXVec3Lerp(&v3CurPos, &v3Start, &v3End, ft);
		uint8_t byTerrainNum;
		float fMultiplier = 1.0f;
		if (GetTerrainNum(v3CurPos.x, v3CurPos.y, &byTerrainNum))
		{
			if (GetTerrainPointer(byTerrainNum, &pTerrain))
			{
				int32_t ix, iy;
				PR_FLOAT_TO_INT(v3CurPos.x, ix);
				PR_FLOAT_TO_INT(fabs(v3CurPos.y), iy);
				float fMapHeight = pTerrain->GetHeight(ix, iy);
				if ( fMapHeight >= v3CurPos.z)
				{
					__ACCESS_ConvertToMapCoords(v3CurPos.x, v3CurPos.y, piCellX, piCellY, pbySubCellX, pbySubCellY, pwTerrainNumX, pwTerrainNumY);
					*v3IntersectPt = v3CurPos;
					return true;
				}
				else
					fMultiplier = fMAX(1.0f, 0.01f * ( v3CurPos.z - fMapHeight ) );
			}
		}
		ft += fAdd * fMultiplier;
	}

	return false;
}

void CMapOutdoorAccessor::__ACCESS_ConvertToMapCoords(float fx, float fy, int32_t *iCellX, int32_t *iCellY, uint8_t * pucSubCellX, uint8_t * pucSubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY)
{
	if ( fy < 0 )
		fy = -fy;

	int32_t ix, iy;
	PR_FLOAT_TO_INT(fx, ix);
	PR_FLOAT_TO_INT(fy, iy);

	*pwTerrainNumX = ix / (CTerrainImpl::TERRAIN_XSIZE);
	*pwTerrainNumY = iy / (CTerrainImpl::TERRAIN_YSIZE);

	float maxx = (float) CTerrainImpl::TERRAIN_XSIZE;
	float maxy = (float) CTerrainImpl::TERRAIN_YSIZE;

	while (fx < 0)
		fx += maxx;

	while (fy < 0)
		fy += maxy;

	while (fx >= maxx)
		fx -= maxx;

	while (fy >= maxy)
		fy -= maxy;

	float fooscale = 1.0f / (float)(CTerrainImpl::CELLSCALE);

	float fCellX, fCellY;

	fCellX = fx * fooscale;
	fCellY = fy * fooscale;

	PR_FLOAT_TO_INT(fCellX, *iCellX);
	PR_FLOAT_TO_INT(fCellY, *iCellY);

	float fRatioooscale = ((float)CTerrainImpl::HEIGHT_TILE_XRATIO) * fooscale;

	float fSubcellX, fSubcellY;
	fSubcellX = fx * fRatioooscale;
	fSubcellY = fy * fRatioooscale;

	PR_FLOAT_TO_INT(fSubcellX, *pucSubCellX);
	PR_FLOAT_TO_INT(fSubcellY, *pucSubCellY);
	*pucSubCellX = (*pucSubCellX) % CTerrainImpl::HEIGHT_TILE_XRATIO;
	*pucSubCellY = (*pucSubCellY) % CTerrainImpl::HEIGHT_TILE_YRATIO;
}

//////////////////////////////////////////////////////////////////////////
// MonsterAreaInfo

void CMapOutdoorAccessor::RemoveMonsterAreaInfoPtr(CMonsterAreaInfo * pMonsterAreaInfo)
{
	m_MonsterAreaInfoPtrVectorIterator = std::find(m_MonsterAreaInfoPtrVector.begin(), m_MonsterAreaInfoPtrVector.end(), pMonsterAreaInfo);
	if (m_MonsterAreaInfoPtrVectorIterator != m_MonsterAreaInfoPtrVector.end())
	{
		pMonsterAreaInfo->Clear();
		m_kPool_kMonsterAreaInfo.Free(pMonsterAreaInfo);
		m_MonsterAreaInfoPtrVectorIterator = m_MonsterAreaInfoPtrVector.erase(m_MonsterAreaInfoPtrVectorIterator);
	}
}

void CMapOutdoorAccessor::SetMonsterNames()
{
	CNonPlayerCharacterInfo & rNonPlayerCharacterInfo = CNonPlayerCharacterInfo::Instance();

	m_MonsterAreaInfoPtrVectorIterator = m_MonsterAreaInfoPtrVector.begin();
	while (m_MonsterAreaInfoPtrVectorIterator != m_MonsterAreaInfoPtrVector.end())
	{
		CMonsterAreaInfo * pMonsterAreaInfo = *m_MonsterAreaInfoPtrVectorIterator;
		if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_MONSTER == pMonsterAreaInfo->GetMonsterAreaInfoType())
		{
			std::string strMonsterName = rNonPlayerCharacterInfo.GetNameByVID(pMonsterAreaInfo->GetMonsterVID());
			pMonsterAreaInfo->SetMonsterName(strMonsterName);
		}
		else if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_GROUP == pMonsterAreaInfo->GetMonsterAreaInfoType())
		{
			std::string strMonsterGroupName = rNonPlayerCharacterInfo.GetNPCGroupNameByGroupID(pMonsterAreaInfo->GetMonsterGroupID());
			std::string strMonsterLeaderGroupName = rNonPlayerCharacterInfo.GetNPCGroupLeaderNameByGroupID(pMonsterAreaInfo->GetMonsterGroupID());
			uint32_t dwFollowerCount = rNonPlayerCharacterInfo.GetNPCGroupFollowerCountByGroupID(pMonsterAreaInfo->GetMonsterGroupID());
			pMonsterAreaInfo->SetMonsterGroupName(strMonsterGroupName);
			pMonsterAreaInfo->SetMonsterGroupLeaderName(strMonsterLeaderGroupName);
			pMonsterAreaInfo->SetMonsterGroupFollowerCount(dwFollowerCount);
		}
		++m_MonsterAreaInfoPtrVectorIterator;
	}
}

bool CMapOutdoorAccessor::SaveMonsterAreaInfo()
{
	if (m_MonsterAreaInfoPtrVector.empty())
		return true;

	char szFileName[256];
	sprintf(szFileName, "%s\\regen.txt", m_strName.c_str());
	FILE * File = fopen(szFileName, "w");

	if (!File)
		return false;

	std::set<uint32_t> MonsterVnumSet;
	MonsterVnumSet.clear();

	fprintf(File, "//type\tcx\tcy\tsx\tsy\tz\tdir\ttime\tpercent\tcount\tvnum\n");
	fprintf(File, "//-----------------------------------------------------------------------------------\n");

	for (uint32_t dwIndex = 0; dwIndex < m_MonsterAreaInfoPtrVector.size(); ++dwIndex)
	{
		CMonsterAreaInfo * pMonsterAreaInfo;
		if (GetMonsterAreaInfoFromVectorIndex(dwIndex, &pMonsterAreaInfo))
		{
			int32_t lOriginX, lOriginY, lSizeX, lSizeY, lZ, lTime, lPercent;
			pMonsterAreaInfo->GetOrigin(&lOriginX, &lOriginY);
			pMonsterAreaInfo->GetSize(&lSizeX, &lSizeY);
			CMonsterAreaInfo::EMonsterDir eMonsterDir = pMonsterAreaInfo->GetMonsterDir();
			CMonsterAreaInfo::EMonsterAreaInfoType eMonsterAreaInfoType = pMonsterAreaInfo->GetMonsterAreaInfoType();
			std::string strMonsterType = "i"; // Invalid;

			uint32_t dwMonsterCount = pMonsterAreaInfo->GetMonsterCount();

			lZ = 0;
			lTime = 1;
			lPercent = 100;

			uint32_t dwMonsterVNum = 0;

			if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_GROUP == eMonsterAreaInfoType)
			{
				strMonsterType = "g";
				dwMonsterVNum = pMonsterAreaInfo->GetMonsterGroupID();
			}
			else if (CMonsterAreaInfo::MONSTERAREAINFOTYPE_MONSTER == eMonsterAreaInfoType)
			{
				strMonsterType = "m";
				dwMonsterVNum = pMonsterAreaInfo->GetMonsterVID();
			}

			fprintf(File, "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%dm\t%d\t%d\t%d\n",
				strMonsterType.c_str(), lOriginX, lOriginY, lSizeX, lSizeY, lZ, eMonsterDir, lTime, lPercent, dwMonsterCount, dwMonsterVNum);

			MonsterVnumSet.insert(dwMonsterVNum);
		}
	}

	fclose(File);

	/////

	char szMonsterArrangeFileName[256];
	sprintf(szMonsterArrangeFileName, "%s\\MonsterArrange.txt", m_strName.c_str());
	FILE * MonsterArrangeFile = fopen(szMonsterArrangeFileName, "w");

	if (!MonsterArrangeFile)
		return false;

	std::set<uint32_t>::iterator itor = MonsterVnumSet.begin();
	for (; itor != MonsterVnumSet.end(); ++itor)
	{
		fprintf(MonsterArrangeFile, "%d\n", *itor);
	}

	fclose(MonsterArrangeFile);

	return true;
}

void CMapOutdoorAccessor::ReloadBuildingTexture()
{
	for (uint8_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CAreaAccessor * pAreaAccessor;
		if (GetAreaPointer(i, (CArea **) &pAreaAccessor))
			pAreaAccessor->ReloadBuildingTexture();
	}
}

void CMapOutdoorAccessor::SetSelectedObjectName(const char * szName)
{
	m_strSelectedObjectName.assign(szName);
}

void CMapOutdoorAccessor::ClearSelectedPortalNumber()
{
	m_kVec_iPortalNumber.clear();
}

void CMapOutdoorAccessor::AddSelectedObjectPortalNumber(int32_t iNum)
{
	m_kVec_iPortalNumber.push_back(iNum);
}

const CMapOutdoorAccessor::TPortalNumberVector & CMapOutdoorAccessor::GetSelectedObjectPortalVectorRef()
{
	return m_kVec_iPortalNumber;
}

const char * CMapOutdoorAccessor::GetSelectedObjectName()
{
	return m_strSelectedObjectName.c_str();
}

struct FGetDungeonObjectHeight
{
	bool m_bHeightFound;
	float m_fReturnHeight;
	float m_fRequestX, m_fRequestY;
	FGetDungeonObjectHeight(float fRequestX, float fRequestY)
	{
		m_fRequestX=fRequestX;
		m_fRequestY=fRequestY;
		m_bHeightFound=false;
		m_fReturnHeight=0.0f;
	}
	void operator () (CGraphicObjectInstance * pObject)
	{
		if (pObject->GetObjectHeight(m_fRequestX, m_fRequestY, &m_fReturnHeight))
		{
			m_bHeightFound = true;
		}
	}
};

void CMapOutdoorAccessor::ArrangeTerrainHeight()
{
	int32_t iRet = ::MessageBox(NULL, "When working on the terrain, changes cannot be reversed\nDo you want to continue?\nThis will set the terrain height as the same level of the buildings' collision height!", "Info", MB_YESNO);
	if (6 != iRet)
		return;

	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();
	pView->Lock();

	__CreateProgressDialog();
	_beginthread(CMapOutdoorAccessor::main_ArrangeTerrainHeight, 0, pView);
	// CMapOutdoorAccessor::main_ArrangeTerrainHeight(pView);
}

void CMapOutdoorAccessor::__CreateProgressDialog()
{
	__DestroyProgressDialog();

	ms_pkProgressDialog = new CMapArrangeHeightProgress;
	ms_pkProgressDialog->Create(CMapArrangeHeightProgress::IDD);
	ms_pkProgressDialog->Init();
	ms_pkProgressDialog->CenterWindow();
	ms_pkProgressDialog->ShowWindow(TRUE);
}

void CMapOutdoorAccessor::__HideProgressDialog()
{
	if (!ms_pkProgressDialog)
		return;

	ms_pkProgressDialog->ShowWindow(SW_HIDE);
}

void CMapOutdoorAccessor::__DestroyProgressDialog()
{
	if (ms_pkProgressDialog)
	{
		delete ms_pkProgressDialog;
	}
	ms_pkProgressDialog = NULL;
}

void CMapOutdoorAccessor::main_ArrangeTerrainHeight(void* pv)
{
	if (!ms_pkProgressDialog)
		return;
	for (int32_t i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CTerrainAccessor * pTerrainAccessor;
		if (!ms_pThis->GetTerrainPointer(i, (CTerrain **) &pTerrainAccessor))
		{
			ms_pkProgressDialog->AddLine("%d/%d Skipped\n", i+1, AROUND_AREA_NUM);
			continue;
		}

		uint16_t wx, wy;
		pTerrainAccessor->GetCoordinate(&wx, &wy);

		int32_t x, y;
		float afHeightMap[CTerrain::HEIGHTMAP_RAW_XSIZE*CTerrain::HEIGHTMAP_RAW_YSIZE];
		BOOL abObjectHeightFound[CTerrain::HEIGHTMAP_RAW_XSIZE*CTerrain::HEIGHTMAP_RAW_YSIZE];

		for (x = 0; x < CTerrain::HEIGHTMAP_RAW_XSIZE; ++x)
		for (y = 0; y < CTerrain::HEIGHTMAP_RAW_YSIZE; ++y)
		{
			float fxLocalPos = float(x*CTerrain::CELLSCALE);
			float fyLocalPos = float(y*CTerrain::CELLSCALE);
			float fxGlobalPos = (wx*CTerrain::TERRAIN_XSIZE) + fxLocalPos;
			float fyGlobalPos = (wy*CTerrain::TERRAIN_YSIZE) + fyLocalPos;

			afHeightMap[x+y*CTerrain::HEIGHTMAP_RAW_XSIZE] = ms_pThis->GetHeight(fxGlobalPos, fyGlobalPos);

			CCullingManager & rkCullingMgr = CCullingManager::Instance();
			Vector3d aVector3d(fxGlobalPos, -fyGlobalPos, 0.0f);
			FGetDungeonObjectHeight kGetObjHeight(fxGlobalPos, fyGlobalPos);
			RangeTester<FGetDungeonObjectHeight> kRangeTester_kGetObjHeight(&kGetObjHeight);

			rkCullingMgr.PointTest2d(aVector3d, &kRangeTester_kGetObjHeight);
			abObjectHeightFound[x+y*CTerrain::HEIGHTMAP_RAW_XSIZE] = kGetObjHeight.m_bHeightFound;
		}

		for (x = 0; x < CTerrain::HEIGHTMAP_RAW_XSIZE; ++x)
		for (y = 0; y < CTerrain::HEIGHTMAP_RAW_YSIZE; ++y)
		{
			float fChangingHeight = 0.0f;
			if (abObjectHeightFound[x+y*CTerrain::HEIGHTMAP_RAW_XSIZE])
			{
				fChangingHeight = afHeightMap[x+y*CTerrain::HEIGHTMAP_RAW_XSIZE];
			}
			else
			{
				for (int32_t i = 0; i < 3; ++i)
				for (int32_t j = 0; j < 3; ++j)
				{
					if (x+i <= 0 || y+j-1 <= 0)
						continue;
					if (x+i >= CTerrain::HEIGHTMAP_RAW_XSIZE || y+j >= CTerrain::HEIGHTMAP_RAW_YSIZE)
						continue;

					if (abObjectHeightFound[(x+i)+(y+j)*CTerrain::HEIGHTMAP_RAW_XSIZE])
					{
						fChangingHeight = max(fChangingHeight, afHeightMap[(x+i)+(y+j)*CTerrain::HEIGHTMAP_RAW_XSIZE]);
					}
				}
			}

			if (0.0f != fChangingHeight)
			{
				float fxLocalPos = float(x*CTerrain::CELLSCALE);
				float fyLocalPos = float(y*CTerrain::CELLSCALE);
				pTerrainAccessor->TerrainPutHeightmap(int32_t(fxLocalPos/200.0f)+1,
													  int32_t(fyLocalPos/200.0f)+1,
													  32767 + (fChangingHeight-16382)*2,
													  false);
			}
		}

		ms_pkProgressDialog->AddLine("%d/%d successfully completed\n", i+1, AROUND_AREA_NUM);
		ms_pkProgressDialog->SetProgress(float(i)/float(AROUND_AREA_NUM) * 100.0f);
	}
	ms_pkProgressDialog->AddLine("End-Of-Job\n");
	LogBox("Terrain height adjustments have been shut down");

	ms_pThis->SetTerrainModified(); // crash if not locked
	ms_pThis->__HideProgressDialog();

	if (pv!=NULL)
		((CWorldEditorView *)pv)->Unlock(); // should be passed by main thread, otherwise crash when pView is get
}

