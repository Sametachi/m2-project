#include "StdAfx.h"
#include "../WorldEditor.h"
#include "MapAccessorTerrain.h"
#include "MapAccessorOutdoor.h"
#include <fcntl.h>
#include <il/il.h>
#include <il/ilu.h>
#include "../Config/Globals.h"

//////////////////////////////////////////////////////////////////////////
// CTerrainAccessor
//////////////////////////////////////////////////////////////////////////
void CTerrainAccessor::RAW_ResetTextures(const uint8_t * pbyTileMap)
{
	if (pbyTileMap)
	{
		for (int32_t ly = 0; ly < TILEMAP_RAW_YSIZE; ++ly)
		{
			for (int32_t lx = 0; lx < TILEMAP_RAW_XSIZE; ++lx)
			{
				int32_t lOffset = ly * TILEMAP_RAW_XSIZE + lx;
				uint8_t byOldRawtileNum = m_abyTileMap[lOffset];
				uint8_t byNewRawtileNum = pbyTileMap[lOffset];
				if (byOldRawtileNum != byNewRawtileNum)
					RecalculateTile(lx, ly, byNewRawtileNum);
			}
		}
	}
	else
	{
		for (int32_t y = 0; y < TILEMAP_RAW_YSIZE; ++y)
			for (int32_t x = 0; x < TILEMAP_RAW_XSIZE; ++x)
				RecalculateTile(x, y, m_abyTileMap[y * TILEMAP_RAW_XSIZE + x]);
	}
}

int32_t CTerrainAccessor::GetHeightPixel(int32_t lCellX, int32_t lCellY)
{
	return (int32_t)GetHeightMapValue(lCellX, lCellY);
}

void CTerrainAccessor::DrawHeightPixel(int32_t lHeight,
									int32_t lCellX,
									int32_t lCellY)
{
	TerrainPutHeightmap(lCellX, lCellY, lHeight, false);
	RAW_UpdateAttrSplat();
}

void CTerrainAccessor::DrawHeightBrush(uint32_t dwBrushShape,
									uint32_t dwBrushType,
									int32_t lCellX,
									int32_t lCellY,
									uint8_t byBrushSize,
									uint8_t byBrushStrength)
{
	switch (dwBrushType)
	{
		case BRUSH_TYPE_UP:
			UpTerrain(dwBrushShape, lCellX, lCellY, byBrushSize, byBrushStrength);
			break;

		case BRUSH_TYPE_DOWN:
			DownTerrain(dwBrushShape, lCellX, lCellY, byBrushSize, byBrushStrength);
			break;

		case BRUSH_TYPE_PLATEAU:
			FlatTerrain(dwBrushShape, lCellX, lCellY, byBrushSize, byBrushStrength);
			break;

		case BRUSH_TYPE_NOISE:
			NoiseTerrain(dwBrushShape, lCellX, lCellY, byBrushSize, byBrushStrength);
			break;

		case BRUSH_TYPE_SMOOTH:
			SmoothTerrain(dwBrushShape, lCellX, lCellY, byBrushSize, byBrushStrength);
			break;

		default:
			return;
	}
	RAW_UpdateAttrSplat();
}

void CTerrainAccessor::DrawTextureBrush(uint32_t dwBrushShape,
										const std::vector<uint8_t> & rVectorTextureNum,
										int32_t lCellX,
										int32_t lCellY,
										uint8_t bySubCellX,
										uint8_t bySubCellY,
										uint8_t byBrushSize,
										bool bErase,
										bool bDrawOnlyOnBlankTile)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;
	uint8_t origtilenum, newtilenum;

	int32_t Left, Top;

	/* Center location */
	cx = lCellX * HEIGHT_TILE_XRATIO + bySubCellX;
	cy = lCellY * HEIGHT_TILE_YRATIO + bySubCellY;

	/* Move to upper left */
	Left = cx - byBrushSize * HEIGHT_TILE_XRATIO;
	Top = cy - byBrushSize * HEIGHT_TILE_YRATIO;

	uint8_t byTextureMax = rVectorTextureNum.size();

	if (0 == byTextureMax)
		return;

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < max(2 * byBrushSize * HEIGHT_TILE_YRATIO, 1); j++)
		{
			for (i = 0; i < max(2 * byBrushSize * HEIGHT_TILE_XRATIO, 1); i++)
			{
				x2 = Left + i;
				y2 = Top + j;
				if (x2 < -1 || x2 >= TILEMAP_RAW_XSIZE-1 || y2 < -1 || y2 >= TILEMAP_RAW_YSIZE-1)
					continue;
				float xf = (float) (x2) + 0.5f;
				float yf = (float) (y2) + 0.5f;

				dist = sqrtf( ( xf - (float)cx) * ( xf - (float)cx) + ( yf - (float)cy) * ( yf - (float)cy));

				if (dist < max(byBrushSize * HEIGHT_TILE_XRATIO, 1))
				{
					origtilenum = m_abyTileMap[(y2+1) * TILEMAP_RAW_XSIZE + (x2+1)];

					if (bDrawOnlyOnBlankTile)
					{
						if (0 == origtilenum)
						{
							newtilenum = rVectorTextureNum[random_range(0, byTextureMax - 1)];
							RecalculateTile(x2+1, y2+1, newtilenum);
						}
					}
					else
					{
						if (bErase)
						{
							bool bFoundNErased = false;
							for (uint32_t dwi = 0; dwi < rVectorTextureNum.size(); ++dwi)
							{
								if (origtilenum == rVectorTextureNum[dwi])
								{
									bFoundNErased = true;
									newtilenum = 0;
								}
							}
							if (!bFoundNErased)
								newtilenum = origtilenum;
						}
						else
							newtilenum = rVectorTextureNum[random_range(0, byTextureMax - 1)];
						RecalculateTile(x2+1, y2+1, newtilenum);
					}
				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 0; j < max(2 * byBrushSize * HEIGHT_TILE_YRATIO, 1); j++)
		{
			for (i = 0; i < max(2 * byBrushSize * HEIGHT_TILE_XRATIO, 1); i++)
			{
				x2 = Left + i;
				y2 = Top + j;
				if (x2 < -1 || x2 >= TILEMAP_RAW_XSIZE-1 || y2 < -1 || y2 >= TILEMAP_RAW_YSIZE-1)
					continue;

				origtilenum = m_abyTileMap[(y2+1) * TILEMAP_RAW_XSIZE + (x2+1)];

				if (bDrawOnlyOnBlankTile)
				{
					if (0 == origtilenum)
					{
						newtilenum = rVectorTextureNum[random_range(0, byTextureMax - 1)];
						RecalculateTile(x2+1, y2+1, newtilenum);
					}
				}
				else
				{
					if (bErase)
					{
						bool bFoundNErased = false;
						for (uint32_t dwi = 0; dwi < rVectorTextureNum.size(); ++dwi)
						{
							if (origtilenum == rVectorTextureNum[dwi])
							{
								bFoundNErased = true;
								newtilenum = 0;
							}
						}
						if (!bFoundNErased)
							newtilenum = origtilenum;
					}
					else
						newtilenum = rVectorTextureNum[random_range(0, byTextureMax - 1)];
					RecalculateTile(x2+1, y2+1, newtilenum);
				}
			}
		}
	}
}

void CTerrainAccessor::DrawAttrBrush(uint32_t dwBrushShape,
									 uint8_t byAttrFlag,
									 int32_t lCellX,
									 int32_t lCellY,
									 uint8_t bySubCellX,
									 uint8_t bySubCellY,
									 uint8_t byBrushSize,
									 bool bErase)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;

	int32_t Left, Top;

	float fAttrHeightRatio = ((float)ATTRMAP_XSIZE) / ((float)XSIZE);
	float fAttrTileRatio = ((float)ATTRMAP_XSIZE) / ((float)TILEMAP_XSIZE);

	/* Center location */
	cx = lCellX * fAttrHeightRatio + bySubCellX * fAttrTileRatio;
	cy = lCellY * fAttrHeightRatio + bySubCellY * fAttrTileRatio;

	/* Move to upper left */
	Left = cx - byBrushSize * fAttrHeightRatio;
	Top = cy - byBrushSize * fAttrHeightRatio;

	if (bErase)
	{
		if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
		{
			for (j = 0; j < max(2 * byBrushSize * fAttrHeightRatio, 1); ++j)
			{
				for (i = 0; i < max(2 * byBrushSize * fAttrHeightRatio, 1); ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= ATTRMAP_XSIZE || y2 < 0 || y2 >= ATTRMAP_YSIZE)
						continue;
					float xf = (float) (Left + i) + 0.5f;
					float yf= (float) (Top + j) + 0.5f;

					dist = sqrtf( ( xf - (float)cx) * ( xf - (float)cx) + ( yf - (float)cy) * ( yf - (float)cy));

					if (dist < max(byBrushSize * fAttrHeightRatio, 1))
						m_abyAttrMap[y2 * ATTRMAP_XSIZE + x2] &= ~(byAttrFlag);
				}
			}
		}
		else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
		{
			for (j = 0; j < max(2 * byBrushSize * fAttrHeightRatio, 1); ++j)
			{
				for (i = 0; i < max(2 * byBrushSize * fAttrHeightRatio, 1); ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= ATTRMAP_XSIZE || y2 < 0 || y2 >= ATTRMAP_XSIZE)
						continue;
					m_abyAttrMap[y2 * ATTRMAP_XSIZE + x2] &= ~(byAttrFlag);
				}
			}
		}
	}
	else
	{
		if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
		{
			for (j = 0; j < max(2 * byBrushSize * fAttrHeightRatio, 1); ++j)
			{
				for (i = 0; i < max(2 * byBrushSize * fAttrHeightRatio, 1); ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= ATTRMAP_XSIZE || y2 < 0 || y2 >= ATTRMAP_XSIZE)
						continue;
					float xf = (float) (Left + i) + 0.5f;
					float yf= (float) (Top + j) + 0.5f;

					dist = sqrtf((xf - (float)cx) * (xf - (float)cx) + ( yf - (float)cy) * ( yf - (float)cy));

					if (dist < max(byBrushSize * fAttrHeightRatio, 1))
						m_abyAttrMap[y2 * ATTRMAP_XSIZE + x2] |= (byAttrFlag);
				}
			}
		}
		else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
		{
			for (j = 0; j < max(2 * byBrushSize * fAttrHeightRatio, 1); ++j)
			{
				for (i = 0; i < max(2 * byBrushSize * fAttrHeightRatio, 1); ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= ATTRMAP_XSIZE || y2 < 0 || y2 >= ATTRMAP_XSIZE)
						continue;
					m_abyAttrMap[y2 * ATTRMAP_XSIZE + x2] |= (byAttrFlag);
				}
			}
		}
	}
	RAW_NotifyAttrModified();
}

void CTerrainAccessor::DrawWaterBrush(uint32_t dwBrushShape,
										int32_t lCellX,
										int32_t lCellY,
										uint8_t byBrushSize,
										uint16_t wWaterHeight,
										bool bErase)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;
	int32_t offset;

	int32_t Left, Top;

	/* Center location */
	cx = lCellX;
	cy = lCellY;

	/* Move to upper left */
	Left = lCellX - byBrushSize;
	Top = lCellY - byBrushSize;

	uint8_t byWaterID;

	if (MAX_WATER_NUM <= m_byNumWater)
	{
		Tracef("You cannot put more water here. (256 kinds of water with different height?)\n");
		return;
	}
	else
	{
		// @fixme109
		bool bNotUpdate = false;
		uint8_t byi;
		for (byi = 0; byi < MAX_WATER_NUM; byi++)
		{
			if (-1 == m_lWaterHeight[byi])
				break;
			else if (wWaterHeight == m_lWaterHeight[byi])
			{
				bNotUpdate = true;
				break;
			}
		}
		byWaterID = byi;
		m_lWaterHeight[byi] = wWaterHeight;
		if (!bNotUpdate)
			++m_byNumWater;
	}

	// Tracef("%d max water, %d waterid, %d waterheight\n", m_byNumWater, byWaterID, m_lWaterHeight);
	if (bErase)
	{
		if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
		{
			for (j = 0; j < 2 * byBrushSize; ++j)
			{
				for (i = 0; i < 2 * byBrushSize; ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= WATERMAP_XSIZE || y2 < 0 || y2 >= WATERMAP_YSIZE)
						continue;
					float xf = (float) (Left + i) + 0.5f;
					float yf= (float) (Top + j) + 0.5f;

					dist = sqrtf( ( xf - (float)cx) * ( xf - (float)cx) + ( yf - (float)cy) * ( yf - (float)cy));

					if (dist < byBrushSize)
					{
						offset = y2 * WATERMAP_XSIZE + x2;
						if (0xFF != m_abyWaterMap[offset])
							m_abyWaterMap[offset] = 0xFF;

						uint8_t byPatchNumX = x2 / PATCH_XSIZE;
						uint8_t byPatchNumY = y2 / PATCH_YSIZE;
						m_TerrainPatchList[byPatchNumY * PATCH_XCOUNT + byPatchNumX].NeedUpdate(true);

						uint32_t dwRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
						m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio] &= ~(ATTRIBUTE_WATER);
						m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio + 1] &= ~(ATTRIBUTE_WATER);
						m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio] &= ~(ATTRIBUTE_WATER);
						m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio + 1] &= ~(ATTRIBUTE_WATER);
					}
				}
			}
		}
		else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
		{
			for (j = 0; j < 2 * byBrushSize; ++j)
			{
				for (i = 0; i < 2 * byBrushSize; ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= WATERMAP_XSIZE || y2 < 0 || y2 >= WATERMAP_YSIZE)
						continue;
					offset = y2 * WATERMAP_XSIZE + x2;
					if (0xFF != m_abyWaterMap[offset])
						m_abyWaterMap[offset] = 0xFF;

					uint8_t byPatchNumX = x2 / PATCH_XSIZE;
					uint8_t byPatchNumY = y2 / PATCH_YSIZE;
					m_TerrainPatchList[byPatchNumY * PATCH_XCOUNT + byPatchNumX].NeedUpdate(true);

					uint32_t dwRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio] &= ~(ATTRIBUTE_WATER);
					m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio + 1] &= ~(ATTRIBUTE_WATER);
					m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio] &= ~(ATTRIBUTE_WATER);
					m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio + 1] &= ~(ATTRIBUTE_WATER);
				}
			}
		}
	}
	else
	{
		if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
		{
			for (j = 0; j < 2 * byBrushSize; ++j)
			{
				for (i = 0; i < 2 * byBrushSize; ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= WATERMAP_XSIZE || y2 < 0 || y2 >= WATERMAP_YSIZE)
						continue;
					float xf = (float) (Left + i) + 0.5f;
					float yf= (float) (Top + j) + 0.5f;

					dist = sqrtf( ( xf - (float)cx) * ( xf - (float)cx) + ( yf - (float)cy) * ( yf - (float)cy));

					if (dist < byBrushSize)
					{
						m_abyWaterMap[y2 * WATERMAP_XSIZE + x2] = byWaterID;

						uint8_t byPatchNumX = x2 / PATCH_XSIZE;
						uint8_t byPatchNumY = y2 / PATCH_YSIZE;
						m_TerrainPatchList[byPatchNumY * PATCH_XCOUNT + byPatchNumX].NeedUpdate(true);

						uint32_t dwRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
						m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio] |= (ATTRIBUTE_WATER);
						m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio + 1] |= (ATTRIBUTE_WATER);
						m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio] |= (ATTRIBUTE_WATER);
						m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio + 1] |= (ATTRIBUTE_WATER);
					}
				}
			}
		}
		else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
		{
			for (j = 0; j < 2 * byBrushSize; ++j)
			{
				for (i = 0; i < 2 * byBrushSize; ++i)
				{
					x2 = Left + i;
					y2 = Top + j;
					if (x2 < 0 || x2 >= WATERMAP_XSIZE || y2 < 0 || y2 >= WATERMAP_YSIZE)
						continue;
					m_abyWaterMap[y2 * WATERMAP_XSIZE + x2] = byWaterID;

					uint8_t byPatchNumX = x2 / PATCH_XSIZE;
					uint8_t byPatchNumY = y2 / PATCH_YSIZE;
					m_TerrainPatchList[byPatchNumY * PATCH_XCOUNT + byPatchNumX].NeedUpdate(true);

					uint32_t dwRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio] |= (ATTRIBUTE_WATER);
					m_abyAttrMap[y2 * dwRatio * ATTRMAP_XSIZE + x2 * dwRatio + 1] |= (ATTRIBUTE_WATER);
					m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio] |= (ATTRIBUTE_WATER);
					m_abyAttrMap[(y2 * dwRatio + 1) * ATTRMAP_XSIZE + x2 * dwRatio + 1] |= (ATTRIBUTE_WATER);
				}
			}
		}
	}

	RAW_UpdateAttrSplat();
}

void CTerrainAccessor::UpTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	int32_t hgt, delta;
	int32_t Left, Top;
	float dist;

	/* Center location */
	cx = x;
	cy = y;

	/* Move to upper left */
	Left = x - byBrushSize;
	Top = y - byBrushSize;

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; ++j)
		{
			for (i = 0; i < 2 * byBrushSize; ++i)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				dist = sqrtf( ( (float)x2 - (float)cx) * ( (float)x2 - (float)cx) + ( (float)y2 - (float)cy) * ( (float)y2 - (float)cy));

				if (dist < byBrushSize)
				{
					delta = (int32_t) (((float) byBrushSize * (float) byBrushSize - dist * dist) * (float) byBrushStrength / 16.0f);
					if (delta <= 0)
						delta = 0;

					hgt = (int32_t)GetHeightMapValue(x2, y2);
					hgt += delta;

					if (hgt < 0)
						hgt = 0;
					if (hgt > 65535)
						hgt = 65535;
					TerrainPutHeightmap(x2, y2, hgt, false);
				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; ++j)
		{
			for (i = 0; i < 2 * byBrushSize; ++i)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				delta = (int32_t) ((float) byBrushSize * (float) byBrushStrength / 16.0f);
				if (delta <= 0)
					delta = 0;

				hgt = (int32_t)GetHeightMapValue(x2, y2);
				hgt += delta;

				if (hgt < 0)
					hgt = 0;
				if (hgt > 65535)
					hgt = 65535;
				TerrainPutHeightmap(x2, y2, hgt, false);
			}
		}
	}
}

void CTerrainAccessor::DownTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	int32_t hgt, delta;
	int32_t Left, Top;
	float dist;

	/* Center location */
	cx = x;
	cy = y;

	/* Move to upper left */
	Left = x - byBrushSize;
	Top = y - byBrushSize;

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; j++)
		{
			for (i = 0; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				dist = sqrtf( ( (float)x2 - (float)cx) * ( (float)x2 - (float)cx) + ( (float)y2 - (float)cy) * ( (float)y2 - (float)cy));

				if (dist < byBrushSize)
				{
					delta = (int32_t) (((float) byBrushSize * (float) byBrushSize - dist * dist) * (float) byBrushStrength / 16.0f);
					if (delta <= 0)
						delta = 0;

					hgt = (int32_t)GetHeightMapValue(x2, y2);
					hgt -= delta;

					if (hgt < 0)
						hgt = 0;
					if (hgt > 65535)
						hgt = 65535;
					TerrainPutHeightmap(x2, y2, hgt, false);
	//				UpdateAttrMapFromHeightMap(x2, y2);
				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; j++)
		{
			for (i = 0; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				delta = (int32_t) ((float) byBrushSize * (float) byBrushStrength / 16.0f);
				if (delta <= 0)
					delta = 0;

				hgt = (int32_t)GetHeightMapValue(x2, y2);
				hgt -= delta;

				if (hgt < 0)
					hgt = 0;
				if (hgt > 65535)
					hgt = 65535;
				TerrainPutHeightmap(x2, y2, hgt, false);
			}
		}
	}
}

void CTerrainAccessor::FlatTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;
	int32_t TargetHeight, hgt, delta;
	int32_t Left, Top;

	/* Center location */
	cx = x;
	cy = y;

	/* Move to upper left */
	Left = x - byBrushSize;
	Top = y - byBrushSize;

	if ((GetAsyncKeyState(VK_LSHIFT) & 0x80) == 0x80)
	{
		TargetHeight = 32767;
	}
	else
	{
		uint8_t byMyTerrainNum;
		if (!m_pOwnerOutdoorMap->GetTerrainNumFromCoord(m_wX, m_wY, &byMyTerrainNum))
			return;

		CTerrain * pTerrain;
		if (y < 0)
		{
			if (x < 0)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum - 4, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x + XSIZE , y + YSIZE);
			}
			else if (x > XSIZE)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum - 2, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x - XSIZE , y + YSIZE);
			}
			else
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum - 3, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x, y + YSIZE);
			}
		}
		else if (y > YSIZE)
		{
			if (x < 0)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum + 2, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x + XSIZE , y - YSIZE);
			}
			else if (x > XSIZE)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum + 4, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x - XSIZE , y - YSIZE);
			}
			else
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum + 3, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x, y - YSIZE);
			}
		}
		else
		{
			if (x < 0)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum - 1, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x + XSIZE , y);
			}
			else if (x > XSIZE)
			{
				if (!m_pOwnerOutdoorMap->GetTerrainPointer(byMyTerrainNum + 1, &pTerrain))
					return;
				TargetHeight = (int32_t) pTerrain->WE_GetHeightMapValue(x - XSIZE , y);
			}
			else
			{
				TargetHeight = (int32_t)WE_GetHeightMapValue(x, y);
			}
		}

		if (TargetHeight < 0)
			TargetHeight = 0;
		if (TargetHeight > 65535)
			TargetHeight = 65535;
	}

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; j++)
		{
			for (i = 0; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				dist = sqrtf( ( (float)x2 - (float)cx) * ( (float)x2 - (float)cx) + ( (float)y2 - (float)cy) * ( (float)y2 - (float)cy));

				if (dist < byBrushSize)
				{
					hgt = (int32_t)GetHeightMapValue(x2, y2);

					delta = ( TargetHeight - hgt) * byBrushStrength / dynamic_cast<CMapOutdoorAccessor *>(m_pOwnerOutdoorMap)->GetMaxBrushStrength();
					hgt += delta;

					if (hgt < 0)
						hgt = 0;
					if (hgt > 65535)
						hgt = 65535;
					TerrainPutHeightmap(x2, y2, hgt);
//					UpdateAttrMapFromHeightMap(x2, y2);
 				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 1; j < 2 * byBrushSize; j++)
		{
			for (i = 1; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				hgt = (int32_t)GetHeightMapValue(x2, y2);

				delta = ( TargetHeight - hgt) * byBrushStrength / dynamic_cast<CMapOutdoorAccessor *>(m_pOwnerOutdoorMap)->GetMaxBrushStrength();
				hgt += delta;

				if (hgt < 0)
					hgt = 0;
				if (hgt > 65535)
					hgt = 65535;
				TerrainPutHeightmap(x2, y2, hgt);
//				UpdateAttrMapFromHeightMap(x2, y2);
			}
		}
	}
}

void CTerrainAccessor::NoiseTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;
	int32_t hgt, delta;
	int32_t Left, Top;

	/* Center location */
	cx = x;
	cy = y;

	/* Move to upper left */
	Left = x - byBrushSize;
	Top = y - byBrushSize;

	uint8_t myTerrainNum;
	m_pOwnerOutdoorMap->GetTerrainNumFromCoord(m_wX, m_wY, &myTerrainNum);
	CTerrainAccessor * pTerrainAccessor = NULL;

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; j++)
		{
			for (i = 0; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				/* Find distance from center of brush */
				dist = sqrtf(((float) x2 - (float) cx) * ((float)x2 - (float)cx) + ((float)y2 - (float)cy) * ((float)y2 - (float)cy));

				if (dist < byBrushSize)
				{
					delta = (int32_t) (random() % byBrushStrength - (byBrushStrength / 2));

					hgt = (int32_t)GetHeightMapValue(x2, y2);
					hgt += delta;

					if (hgt < 0)
						hgt = 0;
					if (hgt > 65535)
						hgt = 65535;
					TerrainPutHeightmap(x2, y2, hgt);
				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 1; j < 2 * byBrushSize; j++)
		{
			for (i = 1; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				delta = (int32_t) (random() % byBrushStrength - byBrushStrength / 2);

				hgt = (int32_t)GetHeightMapValue(x2, y2);
				hgt += delta;

				if (hgt < 0)
					hgt = 0;
				if (hgt > 65535)
					hgt = 65535;
				TerrainPutHeightmap(x2, y2, hgt);
			}
		}
	}
}

void CTerrainAccessor::SmoothTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength)
{
	int32_t cx, cy;
	int32_t i, j;
	int32_t x2, y2;
	float dist;
	int32_t hgt, delta;
	int32_t Left, Top;
	int32_t xt, xb, xl, xr, yt, yb, yl, yr, zt, zb, zl, zr;

	/* Center location */
	cx = x;
	cy = y;

	/* Move to upper left */
	Left = x - byBrushSize;
	Top = y - byBrushSize;

	uint8_t myTerrainNum;
	m_pOwnerOutdoorMap->GetTerrainNumFromCoord(m_wX, m_wY, &myTerrainNum);
	CTerrainAccessor * pTerrainAccessor = NULL;

	if (BRUSH_SHAPE_CIRCLE == dwBrushShape)
	{
		for (j = 0; j < 2 * byBrushSize; j++)
		{
			for (i = 0; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				xt = xb = x2;
				yl = yr = y2;
				xl = x2 - 1;
				xr = x2 + 1;
				yt = y2 - 1;
				yb = y2 + 1;

				dist = sqrtf( ( (float)x2 - (float)cx) * ( (float)x2 - (float)cx) + ( (float)y2 - (float)cy) * ( (float)y2 - (float)cy));

				if (dist < byBrushSize)
				{
					/* Find distance from center of brush */
					zt = (int32_t)WE_GetHeightMapValue(xt, yt);
					zb = (int32_t)WE_GetHeightMapValue(xb, yb);
					zl = (int32_t)WE_GetHeightMapValue(xl, yl);
					zr = (int32_t)WE_GetHeightMapValue(xr, yr);

					hgt = (int32_t)GetHeightMapValue(x2, y2);

					delta = (zt + zb + zl + zr - 4 * hgt) / 4 * byBrushStrength / dynamic_cast<CMapOutdoorAccessor *>(m_pOwnerOutdoorMap)->GetMaxBrushStrength();

					hgt += delta;

					if (hgt < 0)
						hgt = 0;
					if (hgt > 65535)
						hgt = 65535;
					TerrainPutHeightmap(x2, y2, hgt);
				}
			}
		}
	}
	else if (BRUSH_SHAPE_SQUARE == dwBrushShape)
	{
		for (j = 1; j < 2 * byBrushSize; j++)
		{
			for (i = 1; i < 2 * byBrushSize; i++)
			{
				x2 = Left + i;
				y2 = Top + j;

				if (x2 < -1 || x2 >= HEIGHTMAP_RAW_XSIZE - 1 || y2 < -1 || y2 >= HEIGHTMAP_RAW_YSIZE -1 )
					continue;

				xt = xb = x2;
				yl = yr = y2;
				xl = x2 - 1;
				xr = x2 + 1;
				yt = y2 - 1;
				yb = y2 + 1;

				zt = (int32_t)WE_GetHeightMapValue(xt, yt);
				zb = (int32_t)WE_GetHeightMapValue(xb, yb);
				zl = (int32_t)WE_GetHeightMapValue(xl, yl);
				zr = (int32_t)WE_GetHeightMapValue(xr, yr);

				hgt = (int32_t)GetHeightMapValue(x2, y2);

				delta = (zt + zb + zl + zr - 4 * hgt) / 4 * byBrushStrength / dynamic_cast<CMapOutdoorAccessor *>(m_pOwnerOutdoorMap)->GetMaxBrushStrength();

				hgt += delta;

				if (hgt < 0)
					hgt = 0;
				if (hgt > 65535)
					hgt = 65535;
				TerrainPutHeightmap(x2, y2, hgt);
			}
		}
	}
}

void CTerrainAccessor::RAW_RestoreMaps(const uint16_t * pHeightMap, const uint8_t * pbyTileMap, const char * pNormalMap)
{
	memcpy(m_awRawHeightMap, pHeightMap, sizeof(uint16_t) * HEIGHTMAP_RAW_YSIZE * HEIGHTMAP_RAW_XSIZE);
	memcpy(m_acNormalMap, pNormalMap, sizeof(char) * NORMALMAP_YSIZE * NORMALMAP_XSIZE * 3);

	for (uint8_t byPatchNumY = 0; byPatchNumY < PATCH_YCOUNT; ++byPatchNumY)
		for (uint8_t byPatchNumX = 0; byPatchNumX < PATCH_XCOUNT; ++byPatchNumX)
			m_TerrainPatchList[byPatchNumY * PATCH_XCOUNT + byPatchNumX].NeedUpdate(true);
	RAW_ResetTextures(pbyTileMap);
}

bool CTerrainAccessor::SaveProperty(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\AreaProperty.txt", c_rstrMapName.c_str(), ulID);
	FILE * File = fopen(szFileName, "w");

	if (!File)
		return false;

	fprintf(File, "ScriptType AreaProperty\n");
	fprintf(File, "\n");

	fprintf(File, "AreaName \"%s\"\n", m_strName.c_str());
	fprintf(File, "\n");

	fprintf(File, "NumWater %u\n", m_byNumWater);
	fprintf(File, "\n");

	fclose(File);

	return true;
}

bool CTerrainAccessor::NewHeightMap(const std::string & c_rstrMapName)
{
	uint16_t map[HEIGHTMAP_RAW_YSIZE*HEIGHTMAP_RAW_XSIZE];
	uint16_t * pmap;
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\height.raw", c_rstrMapName.c_str(), ulID);

	int32_t x,y;
	pmap = map;
	for(y=0;y<HEIGHTMAP_RAW_YSIZE;y++)
		for(x=0;x<HEIGHTMAP_RAW_XSIZE;x++)
			*pmap++ = 0x7fff;
	FILE* fp;
	fp = fopen(szFileName,"wb");
	fwrite(map,sizeof(uint16_t),HEIGHTMAP_RAW_YSIZE*HEIGHTMAP_RAW_XSIZE,fp);
	fclose(fp);
	return true;
}

bool CTerrainAccessor::SaveHeightMap(const std::string & c_rstrMapName)
{
	//char szFileName[256];
	char szRawFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);

	FILE * fp;
	sprintf(szRawFileName, "%s\\%06u\\height.raw", c_rstrMapName.c_str(), ulID);
	fp = fopen(szRawFileName,"wb");
	fwrite(	m_awRawHeightMap,sizeof(uint16_t), HEIGHTMAP_RAW_XSIZE*HEIGHTMAP_RAW_YSIZE, fp);
	fclose(fp);

	return true;
}

bool CTerrainAccessor::NewTileMap(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\tile.raw", c_rstrMapName.c_str(), ulID);

    FILE *fp = fopen(szFileName, "wb");				/* open the file */
	if (!fp)
		return false;

	int32_t size = TILEMAP_RAW_XSIZE * TILEMAP_RAW_YSIZE * sizeof(uint8_t);
	uint8_t * byTileMap = new uint8_t[size];
	memset(byTileMap, 0, size);
	fwrite(byTileMap, size, 1, fp);
	delete [] byTileMap;

    fclose(fp);
	return true;
}

bool CTerrainAccessor::RAW_SaveTileMap(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\tile.raw", c_rstrMapName.c_str(), ulID);

	FILE *fp = fopen(szFileName, "wb");

	if (!fp)
		return false;

	int32_t size = TILEMAP_RAW_XSIZE * TILEMAP_RAW_YSIZE * sizeof(uint8_t);	/* find size */
	fwrite(m_abyTileMap, size, 1, fp);			/* write size bytes of data from map ptr */

    fclose(fp);
	return true;
}

bool CTerrainAccessor::SaveAttrMap(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\attr.atr", c_rstrMapName.c_str(), ulID);

    FILE *fp = fopen(szFileName, "wb");				/* open the file */

	if (!fp)
		return false;

    const uint16_t mapver = 2634;
    fwrite(&mapver, sizeof(uint16_t), 1, fp);		/* write the magic number */

	const uint16_t wWidth = ATTRMAP_XSIZE;
	const uint16_t wHeight = ATTRMAP_YSIZE;
	fwrite(&wWidth, sizeof(uint16_t), 1, fp);	/* write dimensions */
	fwrite(&wHeight, sizeof(uint16_t), 1, fp);

	int32_t size = wWidth * wHeight * sizeof(uint8_t);	/* find size */
	fwrite(m_abyAttrMap, size, 1, fp);			/* write size bytes of data from map ptr */
    fclose(fp);
	return true;
}

bool CTerrainAccessor::NewAttrMap(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\attr.atr", c_rstrMapName.c_str(), ulID);

    FILE *fp = fopen(szFileName, "wb");				/* open the file */
	if (!fp)
		return false;

    const uint16_t mapver = 2634;
    fwrite(&mapver, sizeof(uint16_t), 1, fp);		/* write the magic number */

	const uint16_t wWidth = ATTRMAP_XSIZE;
	const uint16_t wHeight = ATTRMAP_YSIZE;
	fwrite(&wWidth, sizeof(uint16_t), 1, fp);	/* write dimensions */
	fwrite(&wHeight, sizeof(uint16_t), 1, fp);

	int32_t size = wWidth * wHeight * sizeof(uint8_t);	/* find size */
	uint8_t * pbyAttrMap = new uint8_t[size];			/* write size bytes of data from map ptr */
	memset(pbyAttrMap, 0, size);
	fwrite(pbyAttrMap, size, 1, fp);

    fclose(fp);
	return true;
}

void CTerrainAccessor::RecalculateWaterMap()
{
	uint8_t byNumWaterFirst, byNumWaterSecond;
	uint16_t wWidth, wHeight;
	for (byNumWaterFirst = 0; byNumWaterFirst < m_byNumWater - 1; ++byNumWaterFirst)
	{
		if (-1 == m_lWaterHeight[byNumWaterFirst])
			continue;
		for (byNumWaterSecond = byNumWaterFirst + 1; byNumWaterSecond < m_byNumWater; ++byNumWaterSecond)
		{
			if (-1 == m_lWaterHeight[byNumWaterSecond])
				continue;
			if (m_lWaterHeight[byNumWaterSecond] == m_lWaterHeight[byNumWaterFirst])
			{
				for (wWidth = 0; wWidth < WATERMAP_YSIZE; ++wWidth)
				{
					for (wHeight = 0; wHeight < WATERMAP_XSIZE; ++wHeight)
					{
						if (byNumWaterSecond == m_abyWaterMap[wHeight * WATERMAP_YSIZE + wWidth])
							m_abyWaterMap[wHeight * WATERMAP_XSIZE + wWidth] = byNumWaterFirst;
					}
				}
				m_lWaterHeight[byNumWaterSecond] = -1;
			}
		}
	}

	uint32_t dwNumWater[MAX_WATER_NUM];
	memset ( dwNumWater, 0, sizeof(dwNumWater));

	for (wWidth = 0; wWidth < WATERMAP_YSIZE; ++wWidth)
	{
		for (wHeight = 0; wHeight < WATERMAP_XSIZE; ++wHeight)
		{
			uint8_t byNumWater = m_abyWaterMap[wHeight * WATERMAP_XSIZE + wWidth];
			if (0xFF != byNumWater)
				++dwNumWater[byNumWater];
		}
	}

	uint8_t byNumWaterAfterRecalculate = 0;

	for (byNumWaterFirst = 0; byNumWaterFirst < MAX_WATER_NUM - 1; ++byNumWaterFirst)
	{
		if (0 == dwNumWater[byNumWaterFirst])
		{
			bool bWaterFound = false;
			for (byNumWaterSecond = byNumWaterFirst + 1; byNumWaterSecond < MAX_WATER_NUM; ++byNumWaterSecond)
			{
				if (0 != dwNumWater[byNumWaterSecond])
				{
					bWaterFound = true;
					break;
				}
				else
					m_lWaterHeight[byNumWaterSecond] = -1;
			}
			if (!bWaterFound)
			{
				m_lWaterHeight[byNumWaterFirst] = -1;
				break;
			}
			for (uint16_t wWidth = 0; wWidth < WATERMAP_YSIZE; ++wWidth)
			{
				for (uint16_t wHeight = 0; wHeight < WATERMAP_XSIZE; ++wHeight)
				{
					if (byNumWaterSecond == m_abyWaterMap[wHeight * WATERMAP_XSIZE + wWidth])
						m_abyWaterMap[wHeight * WATERMAP_XSIZE + wWidth] = byNumWaterFirst;
				}
			}
			m_lWaterHeight[byNumWaterFirst] = m_lWaterHeight[byNumWaterSecond];
			m_lWaterHeight[byNumWaterSecond] = -1;
		}
		else
		{
			++byNumWaterAfterRecalculate;
		}
	}

	m_byNumWater = byNumWaterAfterRecalculate;
}

bool CTerrainAccessor::SaveWaterMap(const std::string & c_rstrMapName)
{
	RecalculateWaterMap();
	char szFileName[256];
	uint32_t ulID = (uint32_t) (m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\water.wtr", c_rstrMapName.c_str(), ulID);

    FILE *fp;
    int32_t size;

    fp = fopen(szFileName, "wb");

	if (!fp)
		return false;

    const uint16_t mapver = 5426;
    fwrite(&mapver, sizeof(uint16_t), 1, fp);

	uint16_t wSizeX = WATERMAP_XSIZE, wSizeY = WATERMAP_YSIZE;
	fwrite(&wSizeX, sizeof(uint16_t), 1, fp);
	fwrite(&wSizeY, sizeof(uint16_t), 1, fp);
	fwrite(&m_byNumWater, sizeof(uint8_t), 1, fp);

	size = WATERMAP_XSIZE * WATERMAP_YSIZE * sizeof(uint8_t);
	fwrite(m_abyWaterMap, size, 1, fp);

	if (m_byNumWater > 0)
	{
		size = m_byNumWater * sizeof(int32_t);
		fwrite(m_lWaterHeight, size, 1, fp);
	}

    fclose(fp);
	return true;
}

static HFILE gs_fileout = NULL;

void WriteDTXnFile(uint32_t count, void *buffer)
{
	if (!gs_fileout)
	{
		TraceError("WriteDTXnFile: no file handle");
		return;
	}

	_write(gs_fileout, buffer, count);
}

void ReadDTXnFile(uint32_t count, void *buffer)
{
	return;
}

bool CTerrainAccessor::SaveShadowFromD3DTexture8(const std::string & c_rstrMapName, LPDIRECT3DTEXTURE8 lpShadowTexture)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t) (m_wX) * 1000L + (uint32_t)(m_wY);

	sprintf(szFileName, "%s\\%06u\\shadowmap.tga", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);

	sprintf(szFileName, "%s\\%06u\\shadowmap.raw", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);

	sprintf(szFileName, "%s\\%06u\\shadowmap.bmp", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);

	// DDS fix
	char szDDSFileName[MAX_PATH + 1];
	_snprintf(szDDSFileName, MAX_PATH, "%s\\%06u\\shadowmap.dds", c_rstrMapName.c_str(), ulID);
	DeleteFile(szDDSFileName);

	sprintf(szFileName, "%s\\%06u\\shadowmap.bmp", c_rstrMapName.c_str(), ulID);
	D3DXSaveTextureToFile(szFileName, D3DXIFF_BMP, lpShadowTexture, NULL);

	{
		//////////////////////////////////////////////////////////////////////////
		ilInit();
		ilEnable(IL_FILE_OVERWRITE);

		ILuint image;
		ilGenImages(1, &image);
		ilBindImage(image);

		ilLoadImage(szFileName);
		iluScale(256, 256, 1);

		ilSaveImage(szFileName);
		ilSaveImage(szDDSFileName); // @fixme103
		ilLoadImage(szFileName);

		ilConvertImage(IL_RGBA, IL_BYTE);
		iluFlipImage();

		ILubyte * pRawData = ilGetData();

#ifdef SHADOWMAP_TO_TGA
		sprintf(szFileName, "%s\\%06u\\shadowmap.tga", c_rstrMapName.c_str(), ulID);
#else
		sprintf(szFileName, "%s\\%06u\\shadowmap.raw", c_rstrMapName.c_str(), ulID);
#endif
		FILE * fp = fopen(szFileName, "w");

		if (fp)
		{
#ifdef SHADOWMAP_TO_TGA
			TGA_HEADER header;

			memset(&header, 0, sizeof(TGA_HEADER));

			header.imgType		= 2;
			header.width		= ilGetInteger(IL_IMAGE_WIDTH);
			header.height		= ilGetInteger(IL_IMAGE_WIDTH);
			header.colorBits	= 16;
			header.desc			= IMAGEDESC_TOPLEFT;

			fwrite(&header, sizeof(TGA_HEADER), 1, fp);
#endif
			uint8_t * pbData = (uint8_t *) pRawData;

			for (int32_t h = 0; h < ilGetInteger(IL_IMAGE_HEIGHT); ++h)
			{
				for (int32_t w = 0; w < ilGetInteger(IL_IMAGE_WIDTH); ++w)
				{
					uint8_t r = *(pbData++);
					uint8_t g = *(pbData++);
					uint8_t b = *(pbData++);
					uint8_t a = *(pbData++);

#ifdef SHADOWMAP_TO_TGA
					// Targa = B5 G5 R5
					uint16_t wColor = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
#else
					// Raw565 = R5 G6 B5
					uint16_t wColor = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
#endif
					fwrite(&wColor, sizeof(uint16_t), 1, fp);
				}
			}

			fclose(fp);
		}

		ilDeleteImages(1, &image);
	}

	sprintf(szFileName, "%s\\%06u\\shadowmap.bmp", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);

	return true;
}

bool CTerrainAccessor::SaveMiniMapFromD3DTexture8(const std::string & c_rstrMapName, LPDIRECT3DTEXTURE8 lpMiniMapTexture)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t) (m_wX) * 1000L + (uint32_t)(m_wY);

	sprintf(szFileName, "%s\\%06u\\minimap.bmp", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);
	D3DXSaveTextureToFile(szFileName, D3DXIFF_BMP, lpMiniMapTexture, NULL);

	{
		char szDDSFileName[MAX_PATH + 1];
		_snprintf(szDDSFileName, MAX_PATH, "%s\\%06u\\minimap.dds", c_rstrMapName.c_str(), ulID);

		//////////////////////////////////////////////////////////////////////////
#ifdef USE_WE_CONFIG
		if (globals::dft::NOMINIMAP_RAWALPHA)
		{
			ilInit();
			ilEnable(IL_FILE_OVERWRITE);

			ILuint image;
			ilGenImages(1, &image);
			ilBindImage(image);

			ilLoadImage(szFileName);
			ilSaveImage(szDDSFileName);

			ilConvertImage(IL_RGBA, IL_BYTE);
			iluFlipImage();

			ILubyte * pRawData = ilGetData();
			ilDeleteImages(1, &image);
		}
		else
#endif
		{
			DeleteFile(szDDSFileName);
			D3DXSaveTextureToFile(szDDSFileName, D3DXIFF_DDS, lpMiniMapTexture, NULL);
		}

	}

	sprintf(szFileName, "%s\\%06u\\minimap.bmp", c_rstrMapName.c_str(), ulID);
	DeleteFile(szFileName);

	return true;
}

bool CTerrainAccessor::ReloadShadowTexture(const std::string & c_rstrMapName)
{
	char szFileName[256];
	uint32_t ulID = (uint32_t) (m_wX) * 1000L + (uint32_t)(m_wY);
	sprintf(szFileName, "%s\\%06u\\shadowmap.dds", c_rstrMapName.c_str(), ulID);

	CGraphicImage * pImage = (CGraphicImage *) CResourceManager::Instance().GetResourcePointer(szFileName);
	m_ShadowGraphicImageInstance.ReloadImagePointer(pImage);

	if (!m_ShadowGraphicImageInstance.GetTexturePointer()->IsEmpty())
		m_lpShadowTexture = m_ShadowGraphicImageInstance.GetTexturePointer()->GetD3DTexture();
	else
		m_lpShadowTexture = NULL;

	return true;
}

//////////////////////////////////////////////////////////////////////////

bool CTerrainAccessor::RAW_LoadAndSaveTileMap(const char *rawtilename, const std::string & c_rstrMapName, const std::vector<uint8_t> & c_rVectorBaseTexture)
{
	if (!RAW_LoadTileMap(rawtilename))
	{
		LogBoxf("TileMap failed to load in %d, %d", m_wX, m_wY);
		return false;
	}

	// @fixme111
	uint8_t byNumTexture = c_rVectorBaseTexture.size();
	for (int32_t ix = 0; ix < TILEMAP_RAW_YSIZE; ++ix)
		for (int32_t iy = 0; iy < TILEMAP_RAW_XSIZE; ++iy)
			m_abyTileMap[iy * TILEMAP_RAW_XSIZE + ix] = c_rVectorBaseTexture[random_range(0, byNumTexture - 1)];

	if (!RAW_SaveTileMap(c_rstrMapName))
	{
		LogBoxf("TileMap failed to save in %d, %d", m_wX, m_wY);
		return false;
	}

	return true;
}
// Utility
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Attr

void CTerrainAccessor::RAW_DeallocateAttrSplats()
{
	TTerainSplat & rSplat = m_RAWAttrSplatPatch.Splats[0];
	if (m_lpAttrTexture)
	{
		ULONG ulRef;
		do
		{
			ulRef = m_lpAttrTexture->Release();
		} while(ulRef > 0);
	}
	rSplat.pd3dTexture = m_lpAttrTexture = NULL;
}

void CTerrainAccessor::RAW_AllocateAttrSplats()
{
	RAW_DeallocateAttrSplats();
	m_RAWAttrSplatPatch.m_bNeedsUpdate = true;
	m_RAWAttrSplatPatch.Splats[0].NeedsUpdate = true;
	RAW_GenerateAttrSplat();
}

void CTerrainAccessor::RAW_ResetAttrSplat()
{
	m_RAWAttrSplatPatch.m_bNeedsUpdate = true;
	m_RAWAttrSplatPatch.Splats[0].NeedsUpdate = true;
	RAW_GenerateAttrSplat();
}

void CTerrainAccessor::RAW_UpdateAttrSplat()
{
	RAW_GenerateAttrSplat();
}

void CTerrainAccessor::RAW_GenerateAttrSplat()
{
	if (!m_RAWAttrSplatPatch.m_bNeedsUpdate)
		return;

	m_RAWAttrSplatPatch.m_bNeedsUpdate = false;

	uint8_t abyAlphaMap[ATTRMAP_XSIZE * ATTRMAP_YSIZE];

	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	uint8_t bySelectedAttrFlag = pMapManagerAccessor->GetSelectedAttrFlag();

	TTerainSplat & rAttrSplat = m_RAWAttrSplatPatch.Splats[0];
	if (rAttrSplat.NeedsUpdate)
	{
		rAttrSplat.NeedsUpdate = 0;

		// make alpha texture
		if (m_lpAttrTexture)
		{
			ULONG ulRef;
			do
			{
				ulRef = m_lpAttrTexture->Release();
			} while(ulRef > 0);
		}
		rAttrSplat.pd3dTexture = m_lpAttrTexture = NULL;

		uint8_t * aptr = abyAlphaMap;

		for (int32_t y = 0; y < ATTRMAP_YSIZE; ++y)
		{
			for (int32_t x = 0; x < ATTRMAP_XSIZE; ++x)
			{
				if (isAttrOn(x, y, bySelectedAttrFlag))
					*aptr = 0x60;
				else
					*aptr = 0x00;

				++aptr;
			}
		}
		D3DLOCKED_RECT  d3dlr;

		HRESULT hr;
		do
		{
			hr = ms_lpd3dDevice->CreateTexture(ATTRMAP_XSIZE, ATTRMAP_YSIZE, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &m_lpAttrTexture);
		} while(FAILED(hr));
		do
		{
			hr = m_lpAttrTexture->LockRect(0, &d3dlr, 0, 0);
		} while(FAILED(hr));
		PutImage32(abyAlphaMap, (uint8_t*) d3dlr.pBits, ATTRMAP_XSIZE, d3dlr.Pitch, ATTRMAP_XSIZE, ATTRMAP_YSIZE);
		do
		{
			hr = m_lpAttrTexture->UnlockRect(0);
		} while(FAILED(hr));
		rAttrSplat.pd3dTexture = m_lpAttrTexture;
	}
}

void CTerrainAccessor::RAW_NotifyAttrModified()
{
	m_RAWAttrSplatPatch.m_bNeedsUpdate = true;
	m_RAWAttrSplatPatch.Splats[0].NeedsUpdate = true;
}

bool CTerrainAccessor::RAW_LoadAndSaveDefaultAttrMap(const std::string & c_rstrMapName)
{
	char szAttrMapName[256];
	uint32_t ulID = (uint32_t)(m_wX) * 1000L + (uint32_t)(m_wY);

	sprintf(szAttrMapName, "%s\\%06u\\attr.atr", c_rstrMapName.c_str(), ulID);

	LoadAttrMap(szAttrMapName);

	for (uint32_t dwY = 0; dwY < ATTRMAP_YSIZE; ++dwY)
	{
		for (uint32_t dwX = 0; dwX < ATTRMAP_XSIZE; ++dwX)
		{
			m_abyAttrMap[dwY * ATTRMAP_XSIZE + dwX] &= ~0xF0;
		}
	}

	SaveAttrMap(c_rstrMapName);
	return true;
}
// Attr
//////////////////////////////////////////////////////////////////////////

void CTerrainAccessor::RecalculateTile(int32_t lX, int32_t lY, uint8_t byNewTileNum)
{
	if (lX < 0 || lY < 0 || lX >= TILEMAP_RAW_XSIZE || lY >= TILEMAP_RAW_YSIZE)
		return;

	uint32_t dwTileOffset = lY * TILEMAP_RAW_XSIZE + lX;
	uint8_t byOrigTilelNum = m_abyTileMap[dwTileOffset];
	m_abyTileMap[dwTileOffset] = byNewTileNum;

	m_TerrainSplatPatch.m_bNeedsUpdate = true;

	if (m_TerrainSplatPatch.TileCount[byOrigTilelNum] > 0)
		--m_TerrainSplatPatch.TileCount[byOrigTilelNum];

	++m_TerrainSplatPatch.TileCount[byNewTileNum];

	for (int32_t inum = min(byOrigTilelNum, byNewTileNum); inum <= max(byOrigTilelNum, byNewTileNum); ++inum)
		m_TerrainSplatPatch.Splats[inum].NeedsUpdate = 1;

	int32_t lPatchIndexX = min(max((lX-1)/PATCH_TILE_XSIZE, 0), PATCH_XCOUNT - 1);
	int32_t lPatchIndexY = min(max((lY-1)/PATCH_TILE_YSIZE, 0), PATCH_YCOUNT - 1);
	int32_t lPatchNum = lPatchIndexY * PATCH_XCOUNT + lPatchIndexX;

	if ( m_TerrainSplatPatch.PatchTileCount[lPatchNum][byOrigTilelNum] > 0)
		--m_TerrainSplatPatch.PatchTileCount[lPatchNum][byOrigTilelNum];

	++m_TerrainSplatPatch.PatchTileCount[lPatchNum][byNewTileNum];

	if ( 0 == lY % PATCH_TILE_YSIZE && 0 != lY && (TILEMAP_RAW_YSIZE - 2) != lY)
	{
		++m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + lPatchIndexX][byNewTileNum];
		--m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + lPatchIndexX][byOrigTilelNum];
		if ( 0 == lX % PATCH_TILE_XSIZE && 0 != lX && (TILEMAP_RAW_YSIZE - 2) != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byOrigTilelNum];
			++m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byOrigTilelNum];
		}
		else if ( 1 == lX % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE -1) != lX && 1 != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byOrigTilelNum];
			++m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[min(PATCH_YCOUNT - 1, lPatchIndexY + 1) * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byOrigTilelNum];
		}
	}
	else if ( 1 == lY % PATCH_TILE_YSIZE && (TILEMAP_RAW_YSIZE -1) != lY && 1 != lY)
	{
		++m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + lPatchIndexX][byNewTileNum];
		--m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + lPatchIndexX][byOrigTilelNum];
		if ( 0 == lX % PATCH_TILE_XSIZE && 0 != lX && (TILEMAP_RAW_YSIZE - 2) != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byOrigTilelNum];
			++m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byOrigTilelNum];
		}
		else if ( 1 == lX % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE -1) !=lX && 1 != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byOrigTilelNum];
			++m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[max(0, lPatchIndexY - 1) * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byOrigTilelNum];
		}
	}
	else
	{
		if ( 0 == lX % PATCH_TILE_XSIZE && 0 != lX && (TILEMAP_RAW_YSIZE - 2) != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + min(PATCH_XCOUNT - 1, lPatchIndexX + 1)][byOrigTilelNum];
		}
		else if ( 1 == lX % PATCH_TILE_XSIZE && (TILEMAP_RAW_XSIZE -1) != lX && 1 != lX)
		{
			++m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byNewTileNum];
			--m_TerrainSplatPatch.PatchTileCount[lPatchIndexY * PATCH_XCOUNT + max(0, lPatchIndexX - 1)][byOrigTilelNum];
		}
	}
}

void CTerrainAccessor::Clear()
{
	memset(&m_abyAttrMap, 0, sizeof(ATTRMAP_YSIZE*ATTRMAP_XSIZE));
	memset(&m_RAWAttrSplatPatch, 0, sizeof(m_RAWAttrSplatPatch));
	CTerrain::Clear();
}

void CTerrainAccessor::TerrainPutHeightmap(int32_t x, int32_t y, uint16_t val, bool bRecursive)
{
	int32_t iPos = (y + 1) * HEIGHTMAP_RAW_XSIZE + (x+1);
	if (iPos >= 0)
	if (iPos < HEIGHTMAP_RAW_YSIZE*HEIGHTMAP_RAW_XSIZE)
		m_awRawHeightMap[iPos] = val;

	if ( x >= 0 && y >= 0 && x < NORMALMAP_XSIZE && y < NORMALMAP_YSIZE)
		CalculateNormal(x, y);
	int32_t x2 = x - 1;
	int32_t y2 = y;
	if ( x2 >= 0 && y2 >= 0 && x2 < NORMALMAP_XSIZE && y2 < NORMALMAP_YSIZE)
		CalculateNormal(x2, y2);
	x2 = x + 1;
	y2 = y;
	if ( x2 >= 0 && y2 >= 0 && x2 < NORMALMAP_XSIZE && y2 < NORMALMAP_YSIZE)
		CalculateNormal(x2, y2);
	x2 = x;
	y2 = y - 1;
	if ( x2 >= 0 && y2 >= 0 && x2 < NORMALMAP_XSIZE && y2 < NORMALMAP_YSIZE)
		CalculateNormal(x2, y2);
	x2 = x;
	y2 = y + 1;
	if ( x2 >= 0 && y2 >= 0 && x2 < NORMALMAP_XSIZE && y2 < NORMALMAP_YSIZE)
		CalculateNormal(x2, y2);

	uint8_t byPatchNumX, byPatchNumY;
	byPatchNumX = max(x, 0) / PATCH_XSIZE;
	byPatchNumY = max(y, 0) / PATCH_YSIZE;
	int32_t iPatchPos = byPatchNumY * PATCH_XCOUNT + byPatchNumX;
	if (iPatchPos >= 0)
	if (iPatchPos < PATCH_XCOUNT * PATCH_YCOUNT)
		m_TerrainPatchList[iPatchPos].NeedUpdate(true);
	if ( y % PATCH_YSIZE == 0)
	{
		if ( x % PATCH_YSIZE == 0)
		{
			int32_t iPatchPos = (byPatchNumY - 1) * PATCH_XCOUNT + (byPatchNumX - 1);
			if (iPatchPos >= 0)
			if (iPatchPos < PATCH_XCOUNT * PATCH_YCOUNT)
				m_TerrainPatchList[iPatchPos].NeedUpdate(true);
		}
		else
		{
			int32_t iPatchPos = (byPatchNumY - 1) * PATCH_XCOUNT + byPatchNumX;
			if (iPatchPos >= 0)
			if (iPatchPos < PATCH_XCOUNT * PATCH_YCOUNT)
				m_TerrainPatchList[iPatchPos].NeedUpdate(true);
		}
	}
	else if ( x % PATCH_YSIZE == 0)
	{
		int32_t iPatchPos = byPatchNumY * PATCH_XCOUNT + (byPatchNumX - 1);
		if (iPatchPos >= 0)
		if (iPatchPos < PATCH_XCOUNT * PATCH_YCOUNT)
			m_TerrainPatchList[iPatchPos].NeedUpdate(true);
	}

	if (!bRecursive)
		return;

	bool bWrongPut= false;
	int32_t i,j;

	uint8_t byTerrainNum;
	if ( !m_pOwnerOutdoorMap->GetTerrainNumFromCoord(m_wX, m_wY, &byTerrainNum) )
	{
		Tracef("CTerrainAccessor::TerrainPutHeightmap : Can't Get TerrainNum from Coord %d, %d", m_wX, m_wY);
		byTerrainNum = 4;
	}

	if (( x >= 0 && x < HEIGHTMAP_XSIZE && y >= 0 && y < HEIGHTMAP_YSIZE ) && ((x<=1 || x>=HEIGHTMAP_XSIZE-2) || (y<=1 || y>=HEIGHTMAP_YSIZE-2)))
	{
		for(i=-1;i<=1;i++)
		{
			for(j=-1;j<=1;j++)
			{
				if (i==j && i==0)
					continue;
				else
				{
					int32_t nx, ny;
					nx = x-j*YSIZE;
					ny = y-i*XSIZE;
					if ( (nx<=0 || ny<=0 || nx >= HEIGHTMAP_RAW_XSIZE-3 || ny >= HEIGHTMAP_RAW_YSIZE-3)&&
						nx>=-1 && nx<HEIGHTMAP_RAW_XSIZE-1 && ny>=-1 && ny<HEIGHTMAP_RAW_YSIZE-1)
					{
						CTerrain* pTerrain;
						if (!m_pOwnerOutdoorMap->GetTerrainPointer(byTerrainNum+i*3+j,&pTerrain))
							bWrongPut = true;
						else
							((CTerrainAccessor*)pTerrain)->TerrainPutHeightmap(nx,ny,val,false);
					}
				}
			}
		}
	}
	if (bWrongPut)
	{
		Tracef("It can't be created. Wrong Height which affects the maps next it. TerrainNum(%d), x(%d), y(%d)\n", byTerrainNum, x, y);
	}
}

CTerrainAccessor::CTerrainAccessor() : m_lpAttrTexture(NULL)
{
	m_isDestroied = FALSE;
}

CTerrainAccessor::~CTerrainAccessor()
{
	assert(!m_isDestroied);
	RAW_DeallocateAttrSplats();
	Clear();
	m_isDestroied = TRUE;
}
