#pragma once

#include "../../../Client/gamelib/AreaTerrain.h"

class CTerrainAccessor : public CTerrain
{
	public:
		enum EBrushShape
		{
			BRUSH_SHAPE_NONE	= 0,
			BRUSH_SHAPE_CIRCLE	= 1 << 0,
			BRUSH_SHAPE_SQUARE	= 1 << 1,
			BRUSH_SHAPE_MAX		= 2,
		};

		enum EBrushType
		{
			BRUSH_TYPE_NONE			= 0,
			BRUSH_TYPE_UP			= 1 << 0,
			BRUSH_TYPE_DOWN			= 1 << 1,
			BRUSH_TYPE_PLATEAU		= 1 << 2,
			BRUSH_TYPE_NOISE		= 1 << 3,
			BRUSH_TYPE_SMOOTH		= 1 << 4,
			BRUSH_TYPE_MAX			= 5,
		};

	public:
		CTerrainAccessor();
		virtual ~CTerrainAccessor();

		virtual void	Clear();

		int32_t GetHeightPixel(int32_t lCellX, int32_t lCellY);
		void DrawHeightPixel(int32_t lHeight,
							int32_t lCellX,
							int32_t lCellY);
		void DrawHeightBrush(uint32_t dwBrushShape,
							uint32_t dwBrushType,
							int32_t lCellX,
							int32_t lCellY,
							uint8_t byBrushSize,
							uint8_t byBrushStrength);

		void DrawTextureBrush(uint32_t dwBrushShape,
			const std::vector<uint8_t> & rVectorTextureNum,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t bySubCellX,
			uint8_t bySubCellY,
			uint8_t byBrushSize,
			bool bErase,
			bool bDrawOnlyOnBlankTile);

		void DrawAttrBrush(uint32_t dwBrushShape,
						   uint8_t byAttrPowerNum,
						   int32_t lCellX,
						   int32_t lCellY,
						   uint8_t bySubCellX,
						   uint8_t bySubCellY,
						   uint8_t byBrushSize,
						   bool bErase);

		void DrawWaterBrush(uint32_t dwBrushShape,
							int32_t lCellX,
							int32_t lCellY,
							uint8_t byBrushSize,
							uint16_t wWaterHeight,
							bool bErase);

		void RAW_ResetTextures(const uint8_t * pbyTileMap = NULL);
		void RAW_RestoreMaps(const uint16_t * pHeightMap, const uint8_t * pbyTileMap, const char * pNormalMap);
		bool SaveProperty(const std::string & c_rstrMapName);

		void TerrainPutHeightmap(int32_t x, int32_t y, uint16_t val, bool bRecursive = true);

		// NewMap
		bool NewHeightMap(const std::string & c_rstrMapName);
		bool NewTileMap(const std::string & c_rstrMapName);
		bool NewAttrMap(const std::string & c_rstrMapName);

		// SaveMap
		bool SaveHeightMap(const std::string & c_rstrMapName);
		bool RAW_SaveTileMap(const std::string & c_rstrMapName);
		bool SaveAttrMap(const std::string & c_rstrMapName);
		bool SaveWaterMap(const std::string & c_rstrMapName);
		bool SaveShadowFromD3DTexture8(const std::string & c_rstrMapName, LPDIRECT3DTEXTURE8 lpShadowTexture);
		bool SaveMiniMapFromD3DTexture8(const std::string & c_rstrMapName, LPDIRECT3DTEXTURE8 lpShadowTexture);
		bool ReloadShadowTexture(const std::string & c_rstrMapName);

		//////////////////////////////////////////////////////////////////////////
		bool RAW_LoadAndSaveTileMap(const char *tilename, const std::string & c_rstrMapName, const std::vector<uint8_t> & c_rVectorBaseTexture);

		//////////////////////////////////////////////////////////////////////////
		// Attr
		void RAW_AllocateAttrSplats();
		void RAW_DeallocateAttrSplats();
		void RAW_GenerateAttrSplat();
		void RAW_UpdateAttrSplat();
		void RAW_ResetAttrSplat();

		TTerrainSplatPatch & RAW_GetAttrSplatPatch() { return m_RAWAttrSplatPatch; }
		void RAW_NotifyAttrModified();
		bool RAW_LoadAndSaveDefaultAttrMap(const std::string & c_rstrMapName);

	protected:
// 		virtual void	RAW_CountTiles();

	protected:
		void UpTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength);
		void DownTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength);
		void FlatTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength);
		void NoiseTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength);
		void SmoothTerrain(uint32_t dwBrushShape, int32_t x, int32_t y, uint8_t byBrushSize, uint8_t byBrushStrength);

//		void UpdateAttrMapFromHeightMap(int32_t lx, int32_t ly);

		void RecalculateWaterMap();

		void RecalculateTile(int32_t lX, int32_t lY, uint8_t byNewTileNum);

	protected:
		TTerrainSplatPatch	m_RAWAttrSplatPatch;
		LPDIRECT3DTEXTURE8	m_lpAttrTexture;

		BOOL m_isDestroied;
};
