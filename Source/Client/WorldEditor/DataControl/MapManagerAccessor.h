#pragma once

#include "../../../Libraries/gamelib/MapManager.h"
#include "../../../Libraries/gamelib/MonsterAreaInfo.h"
#include "../Config/Globals.h"

class CShadowRenderHelper;
class CPropertyTreeControler;
class CMapOutdoor;
class CMapOutdoorAccessor;
class CTerrainAccessor;
class CMonsterAreaInfo;

class CMapManagerAccessor : public CMapManager
{
	public:
		class CHeightObserver
		{
			public:
				CHeightObserver(CMapOutdoor * pMapOutdoor) : m_pMap(pMapOutdoor) {}
				~CHeightObserver(){}

				float GetHeight(float fx, float fy);
				CMapOutdoor * m_pMap;
		};

	public:
		CMapManagerAccessor();
		virtual ~CMapManagerAccessor();

		void				Initialize();
		void				Destroy();

		void				DestroyShadowTexture();
		void				RecreateShadowTexture();

		virtual void		Clear();
		virtual CMapBase *	AllocMap();
		virtual bool		LoadMap(const std::string & c_rstrMapName);

		void				UpdateEditing();

		void				ToggleWireframe();
		void				SetWireframe(bool isWireframe);

		void				SetAutoSave(bool bAutoSave);
		bool				IsAutoSave();
		int32_t					GetTimeSave();
		void				SetNextTimeSave();


		// Cursor
		void				SetEditingCursorPosition(const D3DXVECTOR3 & c_rv3Position);

		// Wind
#ifdef ENABLE_WIND_OPTION
		void				EnableWind(BOOL bFlag);
#endif
		void				SetWindStrength(float fStrength);
		void				SetWindRandom(float fRandom);

		// Material
		void				SetMaterialDiffuseColor(float fr, float fg, float fb);
		void				SetMaterialAmbientColor(float fr, float fg, float fb);
		void				SetMaterialEmissiveColor(float fr, float fg, float fb);

		// Light
		void				SetLightDirection(float fx, float fy, float fz);
		void				SetLightDiffuseColor(float fr, float fg, float fb);
		void				SetLightAmbientColor(float fr, float fg, float fb);
		void				EnableLight(BOOL bFlag);

		// Fog
		void				EnableFog(BOOL bFlag);
		void				SetFogColor(float fr, float fg, float fb);
		void				SetFogNearDistance(float fDistance);
		void				SetFogFarDistance(float fDistance);
		void				EnableFiltering(BOOL bFlag);

		// Filtering
		void				SetFilteringColor(float fr, float fg, float fb);
		void				SetFilteringAlpha(float fAlpha);
		void				SetFilteringAlphaSrc(uint8_t byAlphaSrc);
		void				SetFilteringAlphaDest(uint8_t byAlphaDest);

		// SkyBox
		void				SetSkyBoxTextureRenderMode(BOOL bTextureMode);
		BOOL				IsSkyBoxTextureRenderMode();

		void				SetSkyBoxFaceTexture( const char* pFileName, int32_t iFaceIndex );
		std::string		    GetSkyBoxFaceTextre( int32_t iFaceIndex );

		D3DXVECTOR3	&		GetSkyBoxScaleReference();
		D3DXVECTOR2 &		GetSkyBoxCloudScaleReference();
		D3DXVECTOR2 &		GetSkyBoxCloudTextureScaleReference();
		D3DXVECTOR2 &		GetSkyBoxCloudSpeedReference();
		float &				GetSkyBoxCloudHeightReference();
		std::string &		GetSkyBoxCloudTextureFileNameReference();
		uint8_t &				GetSkyBoxGradientUpperReference();
		uint8_t &				GetSkyBoxGradientLowerReference();
		BOOL				GetSkyBoxGradientColorPointer(uint32_t dwIndex, TGradientColor ** ppGradientColor);
		void				InsertGradientUpper();
		void				InsertGradientLower();
		void				DeleteGradient(uint32_t dwIndex);

		// LensFlare
		BOOL &				GetLensFlareEnableReference();
		D3DXCOLOR &			GetLensFlareBrightnessColorReference();
		float &				GetLensFlareMaxBrightnessReference();
		BOOL &				GetMainFlareEnableReference();
		std::string &		GetMainFlareTextureFileNameReference();
		float &				GetMainFlareSizeReference();

		////////////////////////////////////////////
		// Environment
		void				GetEnvironmentData(const TEnvironmentData ** c_ppEnvironmentData);
		void				RefreshEnvironmentData();

		void				InitializeEnvironmentData();
		void				RefreshScreenFilter();
		void				RefreshSkyBox();
		void				RefreshLensFlare();
		void				LoadEnvironmentScript(const char * c_szFileName);
		void				SaveEnvironmentScript(const char * c_szFileName);
		////////////////////////////////////////////

		////////////////////////////////////////////
		// About Brush
		// NOTE : Replace to the CSceneMap
		void				SetBrushShape(uint32_t dwShape);
		void				SetBrushType(uint32_t dwType);

		uint8_t				GetBrushSize() { return m_byBrushSize; }
		uint8_t				GetBrushSizeY() { return m_byBrushSizeY; }
		void				SetBrushSize(uint8_t ucBrushSize);
		void				SetBrushSizeY(uint8_t ucBrushSize);
		void				SetMaxBrushSize(uint8_t ucMaxBrushSize);
		uint8_t				GetBrushStrength() { return m_byBrushStrength; }
		void				SetBrushStrength(uint8_t ucBrushSize);
		void				SetMaxBrushStrength(uint8_t ucMaxBrushStrength);
 		int32_t				GetBrushWaterHeight() { return m_lBrushWaterHeight; }
 		void				SetBrushWaterHeight(int32_t wBrushWaterHeight);

		void				SetTextureBrushVector(std::vector<uint8_t> & rTextureBrushNumberVector);

		void				SetSelectedAttrFlag(uint8_t bFlag);
		uint8_t 				GetSelectedAttrFlag() { return m_bySelectedAttrFlag; }

		void				GetEditArea(int32_t * iEditX,
										int32_t * iEditY,
										uint8_t * usSubEditX,
										uint8_t * usSubEditY,
										uint16_t * usTerrainNumX,
										uint16_t * usTerrainNumY);

		uint32_t				GetBrushShape()							{ return m_dwBrushShape;		}

		void				EditingStart();
		void				EditingEnd();

		void				SetTerrainModified();

		void				SetHeightEditing(bool bOn)				{ m_bHeightEditingOn = bOn;		}
		void				SetTextureEditing(bool bOn)				{ m_bTextureEditingOn = bOn;	}
		void				SetWaterEditing(bool bOn)				{ m_bWaterEditingOn = bOn;		}
		void				SetAttrEditing(bool bOn)				{ m_bAttrEditingOn = bOn;		}
		void				SetMonsterAreaInfoEditing(bool bOn)		{ m_bMonsterAreaInfoEditingOn = bOn; }
		const bool			isHeightEditing()						{ return m_bHeightEditingOn;	}
		const bool			isTextureEditing()						{ return m_bTextureEditingOn;	}
		const bool			isWaterEditing()						{ return m_bWaterEditingOn;		}
		const bool			isAttrEditing()							{ return m_bAttrEditingOn;		}
		const bool			isMonsterAreaInfoEditing()				{ return m_bMonsterAreaInfoEditingOn; }
		////////////////////////////////////////////

		////////////////////////////////////////////
		// Texture Set
		bool	AddTerrainTexture(const char * pFilename);
		bool	RemoveTerrainTexture(int32_t lTexNum);
		void	ResetTerrainTexture();
		// Texture Set
		////////////////////////////////////////////

		////////////////////////////////////////////
		// Base Texture
		void	SetInitTextureBrushVector(std::vector<uint8_t> & rTextureBrushNumberVector);
		bool	InitBaseTexture(const char * c_szMapName = NULL);
		// Base Texture
		////////////////////////////////////////////

		////////////////////////////////////////////
		// About Object Picking & Control
		int32_t GetPickedPickedObjectIndex();
		BOOL IsSelectedObject(int32_t iIndex);
		int32_t GetSelectedObjectCount();
		void SelectObject(int32_t iIndex);
		BOOL Picking();
#ifdef CWE_AREA_ACCESSOR_MISSING_REFRESH
		void RefreshSelectedInfo();
#endif

		void RenderSelectedObject();
		void CancelSelect();
		BOOL SelectObject(float fxStart, float fyStart, float fxEnd, float fyEnd);
		void DeleteSelectedObject();
		void MoveSelectedObject(float fx, float fy);
		void MoveSelectedObjectHeight(float fHeight);
		void AddSelectedAmbienceScale(int32_t iAddScale);
		void AddSelectedAmbienceMaxVolumeAreaPercentage(float fPercentage);
		void AddSelectedObjectRotation(float fYaw, float fPitch, float fRoll);
		void SetSelectedObjectPortalNumber(int32_t iID);
		void DelSelectedObjectPortalNumber(int32_t iID);
		void CollectPortalNumber(std::set<int32_t> * pkSet_iPortalNumber);
		void EnablePortal(BOOL bFlag);
		BOOL IsSelected();

		void InsertObject(float fx, float fy, float fz, int32_t iRotation, uint32_t dwCRC);
		void InsertObject(float fx, float fy, float fz, float fYaw, float fPitch, float fRoll, uint32_t dwScale, uint32_t dwCRC);
		void RefreshObjectHeight(float fx, float fy, float fHalfSize);
		// About Object Picking
		////////////////////////////////////////////

		////////////////////////////////////////////
		// For Undo System
		void BackupObject();
		void BackupObjectCurrent();
		void BackupTerrain();
		void BackupTerrainCurrent();
		////////////////////////////////////////////

		////////////////////////////////////////////
		bool SaveMapProperty(const std::string & c_rstrFolder);
		bool SaveMapSetting(const std::string & c_rstrFolder);
		////////////////////////////////////////////

		////////////////////////////////////////////
		void InitMap();
		bool NewMap(const char * c_szMapName);
		bool SaveMap(const char * c_szMapName = NULL);
		bool SaveTerrains();
		bool SaveAreas();

		void SetNewMapName(const char * c_szNewMapName) { m_strNewMapName = c_szNewMapName;	}
		void SetNewMapSizeX(uint16_t wNewMapSizeX) { m_wNewMapSizeX = wNewMapSizeX; }
		void SetNewMapSizeY(uint16_t wNewMapSizeY) { m_wNewMapSizeY = wNewMapSizeY; }
#ifdef USE_WE_CONFIG
		void SetNewTexSetName(const char * c_szNewTexSetName) { m_strNewTexSetName = c_szNewTexSetName;	}
#endif

		bool CreateNewOutdoorMap();

		BOOL GetEditArea(CAreaAccessor ** ppAreaAccessor);
		BOOL GetArea(const uint8_t & c_ucAreaNum, CAreaAccessor ** ppAreaAccessor);

		BOOL GetEditTerrain(CTerrainAccessor ** ppTerrainAccessor);
		BOOL GetTerrain(const uint8_t & c_ucTerrainNum, CTerrainAccessor ** ppTerrainAccessor);

		const uint8_t GetTerrainNum(float fx, float fy);
		const uint8_t GetEditTerrainNum();

		void RefreshArea();
		////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Attr
		void SetEraseAttr(const bool & bErase) { m_bEraseAttr = bErase;}
		void RenderAttr();
		bool ResetToDefaultAttr();
		void RenderObjectCollision();
		// Attr
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Water
		void SetEraseWater(const bool & bErase) { m_bEraseWater = bErase;}
		void PreviewEditWater();
		// Water
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Texture
		void SetEraseTexture(const bool & bErase) { m_bEraseTexture = bErase;}
		void SetDrawOnlyOnBlankTile(const bool & bOn) { m_bDrawOnlyOnBlankTile = bOn;}
		// Texture
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// Shadow
		void UpdateTerrainShadowMap();
		void ReloadTerrainShadowTexture();
		// Shadow
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		// MiniMap
		void SaveMiniMap();
		void SaveAtlas();
		// MiniMap
		//////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////
		// Monster
		void ShowAllMonsterAreaInfo(bool bShow) { m_bShowAllMonsterAreaInfo = bShow; }
		bool ShowAllMonsterAreaInfo() { return m_bShowAllMonsterAreaInfo; }

		bool SaveMonsterAreaInfo();

		void RemoveMonsterAreaInfoPtr(CMonsterAreaInfo * pMonsterAreaInfo);

		void SetNewMonsterVID(uint32_t dwVID) { m_dwNewMonsterVID = dwVID; }
		void SetNewMonsterCount(uint32_t dwCount) { m_dwNewMonsterCount = dwCount; }
		void SetNewMonsterDir(CMonsterAreaInfo::EMonsterDir eNewMonsterDir) { m_eNewMonsterDir = eNewMonsterDir; }

		void SetNewMonsterAreaInfoType(CMonsterAreaInfo::EMonsterAreaInfoType eNewMonsterAreaInfoType) { m_eNewMonsterAreaInfoType = eNewMonsterAreaInfoType; }

		CMonsterAreaInfo * AddNewMonsterAreaInfo(int32_t lOriginX, int32_t lOriginY, int32_t lSizeX, int32_t lSizeY,
			CMonsterAreaInfo::EMonsterAreaInfoType eMonsterAreaInfoType,
			uint32_t dwVID, uint32_t dwCount, CMonsterAreaInfo::EMonsterDir eNewMonsterDir);
		void SelectMonsterAreaInfo();
		void SelectNextMonsterAreaInfo(bool bForward = true);
		CMonsterAreaInfo * GetSelectedMonsterAreaInfo();

		void SelectMonsterAreaInfoStart() { m_bSelectMonsterAreaInfoStart = true; }
		void SelectMonsterAreaInfoEnd() { m_bSelectMonsterAreaInfoStart = false; }
		bool isSelectMonsterAreaInfoStarted() { return m_bSelectMonsterAreaInfoStart; }

		uint32_t GetMonsterAreaInfoCount();
		bool GetMonsterAreaInfoFromVectorIndex(uint32_t dwMonsterAreaInfoVectorIndex, CMonsterAreaInfo ** ppMonsterAreaInfo);
		////////////////////////////////////////////

		////////////////////////////////////////////
		// ETC
		void LoadProperty(CPropertyTreeControler * pTreeControler);
		void SaveCollisionData(const char * c_szFileName);

		void UpdateHeightFieldEditingPt(D3DXVECTOR3 * v3IntersectPt);
		CHeightObserver * GetHeightObserverPointer();

		void RenderGuildArea();
		void UpdateMapInfo();
		CMapOutdoorAccessor * GetMapOutdoorPtr();
		////////////////////////////////////////////

	protected:
		void EditTerrain();
		void EditTerrainTexture();
		void EditWater();
		void EditAttr();
		void EditMonsterAreaInfo();
		void __LoadMapInfoVector();
		void __AddMapInfo();
		void __RefreshMapID(const char * c_szMapName);
	public:
		bool UpdateBaseMapInfo(uint32_t dwBaseX, uint32_t dwBaseY);
		CMapOutdoorAccessor* GetMapAccessor() { return m_pMapAccessor; };

	protected:
		uint32_t m_dwBrushShape;
		uint32_t m_dwBrushType;

		uint8_t m_byBrushSize;
		uint8_t m_byBrushSizeY;
		uint8_t m_byBrushStrength;
		int32_t m_lBrushWaterHeight;

		uint8_t m_byMAXBrushSize;
		uint8_t m_byMAXBrushStrength;

		std::vector<uint8_t> m_TextureBrushNumberVector;
		std::vector<uint8_t> m_InitTextureBrushNumberVector;

		int32_t m_ixEdit;
		int32_t m_iyEdit;

		uint8_t m_bySubCellX;
		uint8_t m_bySubCellY;
		uint16_t m_wEditTerrainNumX;
		uint16_t m_wEditTerrainNumY;

		bool m_bEditingInProgress;
		bool m_bHeightEditingOn;
		bool m_bTextureEditingOn;
		bool m_bWaterEditingOn;
		bool m_bAttrEditingOn;
		bool m_bMonsterAreaInfoEditingOn;

		CMapOutdoorAccessor *	m_pMapAccessor;
		CHeightObserver *		m_pHeightObserver;

		std::string				m_strNewMapName;
		uint16_t					m_wNewMapSizeX, m_wNewMapSizeY;
#ifdef USE_WE_CONFIG
		std::string				m_strNewTexSetName;
#endif

		uint16_t					m_wOldEditX, m_wOldEditY;

		// Attr
		uint8_t					m_bySelectedAttrFlag;
		bool					m_bEraseAttr;

		// Water
		bool					m_bEraseWater;

		// Texture
		bool					m_bEraseTexture;
		bool					m_bDrawOnlyOnBlankTile;

		// Environment
		TEnvironmentData		m_EnvironmentData;

		// CursorPosition
		D3DXVECTOR3				m_v3EditingCursorPosition;

		bool					m_bAutoSave{ false };
		int32_t						m_iTimeSave{ 0 };

	// Monster
	protected:
		bool					m_bShowAllMonsterAreaInfo;
		uint32_t					m_dwNewMonsterVID;
		uint32_t					m_dwNewMonsterCount;
		CMonsterAreaInfo::EMonsterAreaInfoType	m_eNewMonsterAreaInfoType;
		CMonsterAreaInfo::EMonsterDir			m_eNewMonsterDir;
		std::vector<CMonsterAreaInfo*>			m_pSelectedMonsterAreaInfoVector;
		std::map<std::string, int32_t>				m_kMap_strMapName_iID;
		uint32_t					m_dwSelectedMonsterAreaInfoIndex;
		bool					m_bSelectMonsterAreaInfoStart;
		int32_t					m_lOldOriginX;
		int32_t					m_lOldOriginY;
};
