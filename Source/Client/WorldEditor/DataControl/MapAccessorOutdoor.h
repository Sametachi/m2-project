#pragma once

#include "../../../Client/gamelib/MapOutdoor.h"
#include "../Config/Globals.h"

class CTerrainAccessor;
class CMapArrangeHeightProgress;

class CMapOutdoorAccessor : public CMapOutdoor
{
	public:
		///////////////////////////////////////////////////////////////////////////////////////////
		// Undo
		class CObjectUndoData : public CUndoBuffer::IUndoData
		{
			public:
				CObjectUndoData(CMapOutdoorAccessor * pOutdoorAccessor);
				virtual ~CObjectUndoData();

				void Backup();
				void Restore();

				void BackupStatement();
				void RestoreStatement();

			protected:
				CAreaAccessor::TObjectDataVector	m_backupObjectDataVector[AROUND_AREA_NUM];
				CAreaAccessor *						m_pbackupArea[AROUND_AREA_NUM];

				int16_t								m_sCenterCoordX, m_sCenterCoordY;
				float								m_fCameraX, m_fCameraY;

			private:
				CMapOutdoorAccessor *				m_pOwner;
		};

		class CTerrainUndoData : public CUndoBuffer::IUndoData
		{
			public:
				class CTerrainUndoDataSet
				{
				public:
					CTerrainUndoDataSet();
					virtual ~CTerrainUndoDataSet();

					void DeleteMaps();

					int16_t						m_sCenterCoordX, m_sCenterCoordY;
					float						m_fCameraX, m_fCameraY;
					int16_t						m_sCoordX, m_sCoordY;

					uint16_t *						m_pRawHeightMap;
					uint8_t *						m_pbyTileMap;
					char *						m_pNormalMap;
				};

				typedef std::vector<CTerrainUndoDataSet *> TTerrainUndoDataSetPtrVector;
				typedef TTerrainUndoDataSetPtrVector::iterator TTerrainUndoDataSetPtrVectorIterator;

			public:
				CTerrainUndoData(CMapOutdoorAccessor * pOwner);
				virtual ~CTerrainUndoData();

				void Clear();

				void AddTerrainUndoDataSets();

				void Backup();
				void Restore();

				void BackupStatement();
				void RestoreStatement();

			protected:
 				void AddTerrainUndoDataSet(uint8_t byTerrainNum);

				float						m_fCameraX, m_fCameraY;
				int16_t						m_sCoordX, m_sCoordY;

			private:
				TTerrainUndoDataSetPtrVector m_TerrainUndoDataSetPtrVector;
				CMapOutdoorAccessor * m_pOwner;
		};
		// Undo
		///////////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// For Attr
		enum
		{
			RENDER_SHADOW,
			RENDER_ATTR,
		};

		//////////////////////////////////////////////////////////////////////////
		// Portal
		typedef std::vector<int32_t> TPortalNumberVector;

	public:
		CMapOutdoorAccessor();
		virtual ~CMapOutdoorAccessor();

		virtual void OnPreAssignTerrainPtr();
		virtual bool Load(float x, float y, float z);

		bool SaveProperty(const std::string & c_rstrFolder);
		bool SaveSetting(const std::string & c_rstrFolder);

		bool SaveTerrains();
		bool SaveAreas();

		int32_t GetHeightPixel(
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY);
		void DrawHeightPixel(int32_t lHeight,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY);
		void DrawHeightBrush(uint32_t c_rdwBrushShape,
			uint32_t c_rdwBrushType,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t byBrushSize,
			uint8_t c_rucBrushStrength);

		void DrawTextureBrush(uint32_t dwBrushShape,
			std::vector<uint8_t> & rVectorTextureNum,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t bySubCellX,
			uint8_t bySubCellY,
			uint8_t byBrushSize,
			bool bErase = false,
			bool bDrawOnlyOnBlankTile = false);

		void DrawWaterBrush(uint32_t c_rdwBrushShape,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t byBrushSize,
			uint16_t wWaterHeight,
			bool bErase = false);

		void PreviewWaterBrush(uint32_t c_rdwBrushShape,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t byBrushSize,
			uint16_t wWaterHeight,
			bool bErase = false);

		void DrawAttrBrush(uint32_t dwBrushShape,
			uint8_t byAttrFlag,
			uint16_t wTerrainNumX,
			uint16_t wTerrainNumY,
			int32_t lCellX,
			int32_t lCellY,
			uint8_t bySubCellX,
			uint8_t bySubCellY,
			uint8_t byBrushSize,
			bool bErase = false);

		void ResetTextures();
		void ResetAttrSplats();

		int32_t GetMapID();
		void SetMapID(int32_t iID);
		void SetTerrainModified();

		bool CreateNewTerrainFiles(uint16_t wCoordX, uint16_t wCoordY);

		//////////////////////////////////////////////////////////////////////////
		void SetMaxBrushStrength(uint8_t byMaxBrushStrength) { m_byMAXBrushStrength = byMaxBrushStrength;}
		uint8_t GetMaxBrushStrength() { return m_byMAXBrushStrength;}

		BOOL GetAreaAccessor(uint32_t dwIndex, CAreaAccessor ** ppAreaAccessor);

		bool GetPickingCoordinate(D3DXVECTOR3 * v3IntersectPt, int32_t * piCellX, int32_t * piCellY, uint8_t * pbySubCellX, uint8_t * pbySubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY);
		bool GetPickingCoordinateWithRay(const CRay & rRay, D3DXVECTOR3 * v3IntersectPt, int32_t * piCellX, int32_t * piCellY, uint8_t * pbySubCellX, uint8_t * pbySubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY);
		////////////////////////////////////////////
		// For Undo System
		void BackupObject();
		void BackupObjectCurrent();
		void BackupTerrain();
		void BackupTerrainCurrent();
		////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Utility
		bool RAW_InitBaseTexture(const std::vector<uint8_t> & c_rVectorBaseTexture);
		void ReloadBuildingTexture();

		//////////////////////////////////////////////////////////////////////////
		// ATTR
		void RenderAccessorTerrain(uint8_t byRenderMode, uint8_t byAttrFlag);
		bool ResetToDefaultAttr();
		void RenderObjectCollision();

		// ShadowMap
		void RenderToShadowMap();
		void RenderShadow();

		// MiniMap
		void RenderMiniMap();

		// Reload
		void ReloadTerrainTextures();

		// System Value
		float GetSplatValue();
		void SetSplatValue(float fValue);

		// ETC
		void ArrangeTerrainHeight();

		// Guild Area List
		bool LoadGuildAreaList(const char * c_szFileName);

		// Selected Object
		void SetSelectedObjectName(const char * szName);
		void ClearSelectedPortalNumber();
		void AddSelectedObjectPortalNumber(int32_t iNum);
		const TPortalNumberVector & GetSelectedObjectPortalVectorRef();
		const char * GetSelectedObjectName();

	protected:
		virtual void	__ClearGarvage();
		virtual void	__UpdateGarvage();

		virtual bool	LoadTerrain(uint16_t wTerrainCoordX, uint16_t wTerrainCoordY, uint16_t wCellCoordX, uint16_t wCellCoordY);
		virtual bool	LoadArea(uint16_t wAreaCoordX, uint16_t wAreaCoordY, uint16_t wCellCoordX, uint16_t wCellCoordY);
		virtual void	UpdateAreaList(int32_t lCenterX, int32_t lCenterY);
		virtual bool	Destroy();

		static void		main_ArrangeTerrainHeight(void* pv);
		void			__CreateProgressDialog();
		void			__HideProgressDialog();
		void			__DestroyProgressDialog();
		void			__ACCESS_ConvertToMapCoords(float fx, float fy, int32_t *iCellX, int32_t *iCellY, uint8_t * pucSubCellX, uint8_t * pucSubCellY, uint16_t * pwTerrainNumX, uint16_t * pwTerrainNumY);

	private:
		void RecurseRenderAccessorTerrain(CTerrainQuadtreeNode *Node, uint8_t byRenderMode, uint8_t byAttrFlag, bool bCullEnable = true);
		void DrawPatchAttr(int32_t patchnum, uint8_t byAttrFlag);

		//////////////////////////////////////////////////////////////////////////
		// For ShadowRender
		void DrawMeshOnly(int32_t patchnum);
		//////////////////////////////////////////////////////////////////////////

		void ResetTerrainPatchVertexBuffer();

		uint8_t			m_byMAXBrushStrength;
		bool					m_bLoadTextureAttr;

		float					m_fSplatValue;

		int32_t						m_iMapID;

		std::string				m_strSelectedObjectName;
		TPortalNumberVector		m_kVec_iPortalNumber;

		static CMapOutdoorAccessor *			ms_pThis;
		static CMapArrangeHeightProgress *		ms_pkProgressDialog;

	public:
		void SetMonsterNames();
		void RemoveMonsterAreaInfoPtr(CMonsterAreaInfo * pMonsterAreaInfo);
		bool SaveMonsterAreaInfo();

		BOOL m_bNowAccessGarvage;

#ifdef USE_WE_CONFIG
	private:
		std::string m_strNewTexSetName;
	public:
		void SetNewTexSetName(const std::string & strNewTexSetName) { m_strNewTexSetName = strNewTexSetName; }
		const std::string & GetNewTexSetName() { return m_strNewTexSetName; }
#endif
};
