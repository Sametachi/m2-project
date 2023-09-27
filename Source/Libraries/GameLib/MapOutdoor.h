#pragma once
#include <EterLib/SkyBox.h>
#include <EterLib/LensFlare.h>
#include <EterLib/ScreenFilter.h>
#include <PRTerrainLib/TerrainType.h>
#include <PRTerrainLib/TextureSet.h>
#include <SpeedTreeLib/SpeedTreeForest.h>
#include <SpeedTreeLib/SpeedTreeForestDirectX9.h>

#include "MapBase.h"
#include "Area.h"
#include "AreaTerrain.h"

constexpr auto LOAD_SIZE_WIDTH = 1;
constexpr auto AROUND_AREA_NUM = 1 + (LOAD_SIZE_WIDTH * 2) * (LOAD_SIZE_WIDTH * 2) * 2;
constexpr auto MAX_PREPARE_SIZE = 9;
constexpr auto MAX_MAPSIZE = 256;	// 0 ~ 255, cellsize 200 = 64km
constexpr auto TERRAINPATCH_LODMAX = 3;

typedef struct SOutdoorMapCoordinate
{
	int16_t m_sTerrainCoordX;
	int16_t m_sTerrainCoordY;
} TOutdoorMapCoordinate;

typedef std::map<const std::string, TOutdoorMapCoordinate> TOutdoorMapCoordinateMap;

class CTerrainPatchProxy;
class CTerrainQuadtreeNode;
class CSpeedTreeWrapper;

class CMapOutdoor : public CMapBase
{
public:
	enum
	{
		VIEW_NONE = 0,
		VIEW_PART,
		VIEW_ALL
	};

	enum EPart
	{
		PART_TERRAIN,
		PART_OBJECT,
		PART_CLOUD,
		PART_WATER,
		PART_TREE,
		PART_SKY,
		PART_NUM
	};

	enum ETerrainRenderSort
	{
		DISTANCE_SORT,
		TEXTURE_SORT
	};

	CMapOutdoor();
	virtual ~CMapOutdoor();
	virtual void OnBeginEnvironment();

protected:
	bool Initialize();

	virtual bool Destroy();
	virtual void OnSetEnvironmentDataPtr();
	virtual void OnResetEnvironmentDataPtr();
	virtual void OnRender();
	virtual void OnRenderShadow();
	virtual void OnRenderTreeShadow();

public:
	void SetInverseViewAndDynamicShaodwMatrices();
	virtual bool Load(float x, float y, float z);
	virtual float GetHeight(float x, float y);
	virtual bool Update(float fX, float fY, float fZ);
	virtual void UpdateAroundAmbience(float fX, float fY, float fZ);

	void Clear();
	void SetVisiblePart(int32_t ePart, bool isVisible);
	void SetSplatLimit(int32_t iSplatNum);
	std::vector<int32_t>& GetRenderedSplatNum(int32_t* piPatch, int32_t* piSplat, float* pfSplatRatio);
	bool LoadSetting(const char* c_szFileName);
	void ApplyLight(uint32_t dwVersion, const D3DLIGHT9& c_rkLight) override;

	void SetEnvironmentScreenFilter();
	void SetEnvironmentSkyBox();
	void SetEnvironmentLensFlare();

	void CreateCharacterShadowTexture();
	void ReleaseCharacterShadowTexture();
	void SetShadowTextureSize(uint16_t size);

	bool BeginRenderCharacterShadowToTexture();
	void EndRenderCharacterShadowToTexture();
	void RenderWater();
	void RenderMarkedArea();
	void RecurseRenderAttr(CTerrainQuadtreeNode* Node, bool bCullEnable = TRUE);
	void DrawPatchAttr(int32_t patchnum);
	void ClearGuildArea();
	void RegisterGuildArea(int32_t isx, int32_t isy, int32_t iex, int32_t iey);

	void VisibleMarkedArea();
	void DisableMarkedArea();

	void UpdateSky();
	void RenderCollision();
	void RenderSky();
	void RenderCloud();
	void RenderBeforeLensFlare();
	void RenderAfterLensFlare();
	void RenderScreenFiltering();
	void SetWireframe(bool bWireFrame);
	bool IsWireframe();

	bool GetPickingPointWithRay(const CRay& rRay, D3DXVECTOR3* v3IntersectPt);
	bool GetPickingPointWithRayOnlyTerrain(const CRay& rRay, D3DXVECTOR3* v3IntersectPt);
	bool GetPickingPoint(D3DXVECTOR3* v3IntersectPt);
	void GetTerrainCount(int16_t* psTerrainCountX, int16_t* psTerrainCountY)
	{
		*psTerrainCountX = m_sTerrainCountX;
		*psTerrainCountY = m_sTerrainCountY;
	}

	bool SetTerrainCount(int16_t sTerrainCountX, int16_t sTerrainCountY);

	// Shadow
	void SetDrawShadow(bool bDrawShadow);
	void SetDrawCharacterShadow(bool bDrawChrShadow);
	void SetDrawBackgroundShadow(bool bDrawBgShadow);
	uint32_t GetShadowMapColor(float fx, float fy);

protected:
	bool __PickTerrainHeight(float& fPos, const D3DXVECTOR3& v3Start, const D3DXVECTOR3& v3End, float fStep, float fRayRange, float fLimitRange, D3DXVECTOR3* pv3Pick);
	void __UpdateGarbage();
	virtual bool LoadTerrain(uint16_t wTerrainCoordX, uint16_t wTerrainCoordY);
	virtual bool LoadArea(uint16_t wAreaCoordX, uint16_t wAreaCoordY);
	virtual void UpdateAreaList(uint32_t lCenterX, uint32_t lCenterY);
	bool isTerrainLoaded(uint16_t wX, uint16_t wY);
	bool isAreaLoaded(uint16_t wX, uint16_t wY);
	void AssignTerrainPointer();
	void GetHeightMap(const uint8_t& c_rucTerrainNum, uint16_t** pwHeightMap);
	void GetNormalMap(const uint8_t& c_rucTerrainNum, char** pucNormalMap);

	// Water
	void GetWaterMap(const uint8_t& c_rucTerrainNum, uint8_t** pucWaterMap);
	void GetWaterHeight(uint8_t byTerrainNum, uint8_t byWaterNum, int32_t* plWaterHeight);

	// Terrain
	CTerrain* m_pTerrain[AROUND_AREA_NUM]{};
	CTerrainPatchProxy* m_pTerrainPatchProxyList{};  // Polygon patches that actually render when rendering terrain... Independent from CTerrain for Seamless Map...
	int32_t m_lViewRadius{};   // Viewing distance.. Cell unit..
	float m_fHeightScale{}; // Height scale... When it is 1.0, it can be expressed from 0 to 655.35 meters.
	int16_t m_sTerrainCountX{}, m_sTerrainCountY{}; // Seamless map
	TOutdoorMapCoordinate m_CurCoordinate{};
	int32_t m_lCurCoordStartX{}, m_lCurCoordStartY{};
	TOutdoorMapCoordinate m_PrevCoordinate{};
	uint16_t m_wPatchCount{};
	//TOutdoorMapCoordinateMap	m_EntryPointMap;

	virtual void DestroyTerrain();
	void CreateTerrainPatchProxyList();
	void DestroyTerrainPatchProxyList();
	void UpdateTerrain(float fX, float fY);
	void ConvertTerrainToTnL(int32_t lx, int32_t ly);
	void AssignPatch(int32_t lPatchNum, int32_t lx0, int32_t ly0, int32_t lx1, int32_t ly1);

	// Index Buffer
	uint16_t* m_pwaIndices[TERRAINPATCH_LODMAX];
	CGraphicIndexBuffer m_IndexBuffer[TERRAINPATCH_LODMAX]{};
	uint16_t m_wNumIndices[TERRAINPATCH_LODMAX]{};

	void ADDLvl1TL(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1T(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1TR(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1L(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1R(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1BL(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1B(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1BR(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl1M(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2TL(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2T(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2TR(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2L(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2R(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2BL(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2B(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2BR(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);
	void ADDLvl2M(uint16_t* pIndices, uint16_t& rwCount, const uint16_t& c_rwCurCount, const uint8_t& c_rucNumLineWarp);

public:
	bool GetTerrainPointer(uint8_t c_ucTerrainNum, CTerrain** ppTerrain);
	float GetTerrainHeight(float fx, float fy);
	bool GetWaterHeight(int32_t iX, int32_t iY, int32_t* plWaterHeight);
	bool GetNormal(int32_t ix, int32_t iy, D3DXVECTOR3* pv3Normal);

	void RenderTerrain();
	const int32_t GetViewRadius()
	{
		return m_lViewRadius;
	}
	const float GetHeightScale()
	{
		return m_fHeightScale;
	}
	const TOutdoorMapCoordinate& GetCurCoordinate()
	{
		return m_CurCoordinate;
	}
	const TOutdoorMapCoordinate& GetPrevCoordinate()
	{
		return m_PrevCoordinate;
	}

	// Area
protected:
	CArea* m_pArea[AROUND_AREA_NUM]{};
	virtual void DestroyArea();

	void UpdateArea(D3DXVECTOR3& v3Player);

	void __CollectShadowReceiver(D3DXVECTOR3& v3Target, D3DXVECTOR3& v3Light);
	void __CollectCollisionPCBlocker(D3DXVECTOR3& v3Eye, D3DXVECTOR3& v3Target, float fDistance);
	void __CollectCollisionShadowReceiver(D3DXVECTOR3& v3Target, D3DXVECTOR3& v3Light);
	void __UpdateAroundAreaList();
	bool __IsInShadowReceiverList(CGraphicObjectInstance* pkObjInstTest);
	bool __IsInPCBlockerList(CGraphicObjectInstance* pkObjInstTest);
	void ConvertToMapCoords(float fx, float fy, int32_t* iCellX, int32_t* iCellY, uint8_t* pucSubCellX, uint8_t* pucSubCellY, uint16_t* pwTerrainNumX, uint16_t* pwTerrainNumY);

public:
	bool GetAreaPointer(const uint8_t c_ucAreaNum, CArea** ppArea);
	void RenderArea(bool bRenderAmbience = true);
	void RenderAreaShadow();
	void RenderBlendArea();
	void RenderEffect();
	void RenderPCBlocker();
	void RenderTree();

	std::optional<CArea*> GetAreaPointer(uint8_t c_byAreaNum);
	bool isAttrOn(float fX, float fY, uint8_t byAttr);
	bool GetAttr(float fX, float fY, uint8_t* pbyAttr);
	bool isAttrOn(int32_t iX, int32_t iY, uint8_t byAttr);
	bool GetAttr(int32_t iX, int32_t iY, uint8_t* pbyAttr);

	bool GetTerrainNum(float fx, float fy, uint8_t* pbyTerrainNum);
	bool GetTerrainNumFromCoord(uint16_t wCoordX, uint16_t wCoordY, uint8_t* pbyTerrainNum);

protected:
	// QuadTree
	int32_t m_lCenterX, m_lCenterY;
	int32_t m_lOldReadX, m_lOldReadY;
	CTerrainQuadtreeNode* m_pRootNode;

	void BuildQuadTree();
	CTerrainQuadtreeNode* AllocQuadTreeNode(int32_t x0, int32_t y0, int32_t x1, int32_t y1);
	void SubDivideNode(CTerrainQuadtreeNode* Node);
	void UpdateQuadTreeHeights(CTerrainQuadtreeNode* Node);
	void FreeQuadTree();

	std::vector<std::pair<float, int32_t> > m_PatchVector;
	void NEW_DrawWireFrame(CTerrainPatchProxy* pTerrainPatchProxy, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType);
	void DrawWireFrame(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType);
	void DrawWater(int32_t patchnum);

	bool m_bDrawWireFrame;
	bool m_bDrawShadow;
	bool m_bDrawChrShadow;
	bool m_bDrawBgShadow;

	// Water
	void LoadWaterTexture();
	void UnloadWaterTexture();

	// Character Shadow
	IDirect3DTexture9* m_lpCharacterShadowMapTexture;
	LPDIRECT3DSURFACE9 m_lpCharacterShadowMapColorSurface;
	D3DVIEWPORT9 m_ShadowMapViewport;
	uint16_t m_wShadowMapSize;

	// Backup Device Context
	LPDIRECT3DSURFACE9 m_lpBackupRenderTargetSurface;
	LPDIRECT3DSURFACE9 m_lpBackupDepthSurface;
	D3DVIEWPORT9 m_BackupViewport;

	// View Frustum Culling
	D3DXPLANE m_plane[6];

	void BuildViewFrustum(D3DXMATRIX& mat);

	CTextureSet m_TextureSet;
	CTextureSet m_SnowTextureSet;

	CSkyBox m_SkyBox;
	CLensFlare m_LensFlare;
	CScreenFilter m_ScreenFilter;

	void SetIndexBuffer();
	void SelectIndexBuffer(uint8_t byLODLevel, uint16_t* pwPrimitiveCount, D3DPRIMITIVETYPE* pePrimitiveType);

	D3DXMATRIX m_matWorldForCommonUse;
	D3DXMATRIX m_matViewInverse;
	D3DXMATRIX m_matSplatAlpha;
	D3DXMATRIX m_matStaticShadow;
	D3DXMATRIX m_matDynamicShadow;
	D3DXMATRIX m_matDynamicShadowScale;
	D3DXMATRIX m_matLightView;

	float m_fTerrainTexCoordBase, m_fWaterTexCoordBase;
	float m_fXforDistanceCaculation, m_fYforDistanceCaculation;

	typedef std::vector< std::unique_ptr<CTerrain> > TTerrainPtrVector;
	typedef std::vector< std::unique_ptr<CArea> > TAreaPtrVector;

	TTerrainPtrVector m_TerrainVector;
	TTerrainPtrVector m_TerrainDeleteVector;
	TAreaPtrVector m_AreaVector;
	TAreaPtrVector m_AreaDeleteVector;

	enum EDeleteDir
	{
		DELETE_LEFT,
		DELETE_RIGHT,
		DELETE_TOP,
		DELETE_BOTTOM,
	};

	template <class T> struct IsUsedSectorObject
	{
		EDeleteDir m_eLRDeleteDir;
		EDeleteDir m_eTBDeleteDir;
		TOutdoorMapCoordinate m_CurCoordinate;

		IsUsedSectorObject(EDeleteDir eLRDeleteDir, EDeleteDir eTBDeleteDir, TOutdoorMapCoordinate CurCoord)
		{
			m_eLRDeleteDir = eLRDeleteDir;
			m_eTBDeleteDir = eTBDeleteDir;
			m_CurCoordinate = CurCoord;
		}

		bool operator()(std::unique_ptr<T>& p);
	};

	template <class T> void PruneSectorObjectList(T& objects, T& delQueue, EDeleteDir lr, EDeleteDir tb);

	void InitializeVisibleParts();
	bool IsVisiblePart(int32_t ePart);

	float __GetNoFogDistance();
	float __GetFogDistance();

	uint32_t m_dwVisiblePartFlags;
	int32_t m_iRenderedSplatNumSqSum;
	int32_t m_iRenderedSplatNum;
	int32_t m_iRenderedPatchNum;
	std::vector<int32_t> m_RenderedTextureNumVector;
	int32_t m_iSplatLimit;
	int32_t m_iPatchTerrainVertexCount;
	int32_t m_iPatchWaterVertexCount;
	int32_t m_iPatchTerrainVertexSize;
	int32_t m_iPatchWaterVertexSize;
	std::list<RECT> m_rkList_kGuildArea;

	void __RenderTerrain_RecurseRenderQuadTree(CTerrainQuadtreeNode* Node, bool bCullCheckNeed = true);
	int32_t	 __RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(const D3DXVECTOR3& c_v3Center, const float& c_fRadius);
	void __RenderTerrain_AppendPatch(const D3DXVECTOR3& c_rv3Center, float fDistance, int32_t lPatchNum);
	void __RenderTerrain_RenderHardwareTransformPatch();
	void __HardwareTransformPatch_RenderPatchSplat(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType);
	void __HardwareTransformPatch_RenderPatchNone(int32_t patchnum, uint16_t wPrimitiveCount, D3DPRIMITIVETYPE ePrimitiveType);

	std::vector<CGraphicObjectInstance*> m_ShadowReceiverVector;
	std::vector<CGraphicObjectInstance*> m_PCBlockerVector;
	float m_fOpaqueWaterDepth;
	CGraphicImageInstance m_WaterInstances[30];

	ETerrainRenderSort m_eTerrainRenderSort;
	CGraphicImageInstance m_attrImageInstance;
	CGraphicImageInstance m_BuildingTransparentImageInstance;
	D3DXMATRIX m_matBuildingTransparent;

public:
	float GetOpaqueWaterDepth()
	{
		return m_fOpaqueWaterDepth;
	}
	void SetOpaqueWaterDepth(float fOpaqueWaterDepth)
	{
		m_fOpaqueWaterDepth = fOpaqueWaterDepth;
	}
	void SetTerrainRenderSort(ETerrainRenderSort eTerrainRenderSort)
	{
		m_eTerrainRenderSort = eTerrainRenderSort;
	}
	ETerrainRenderSort GetTerrainRenderSort()
	{
		return m_eTerrainRenderSort;
	}

	//void GetBaseXY(uint32_t* pdwBaseX, uint32_t* pdwBaseY);
	//void SetBaseXY(uint32_t dwBaseX, uint32_t dwBaseY);

	void SetTransparentTree(bool bTransparentTree)
	{
		m_bTransparentTree = bTransparentTree;
	}
	//void EnableTerrainOnlyForHeight(bool bFlag) { m_bEnableTerrainOnlyForHeight = bFlag; }
	void EnablePortal(bool bFlag);
	bool IsEnablePortal()
	{
		return m_bEnablePortal;
	}

protected:
	//uint32_t			m_dwBaseX;
	//uint32_t			m_dwBaseY;

	D3DXVECTOR3 m_v3Player;

	bool m_bShowEntirePatchTextureCount;
	bool m_bTransparentTree;
	//bool			m_bEnableTerrainOnlyForHeight;
	bool m_bEnablePortal;

	// XMas
private:
	struct SXMasTree
	{
		CSpeedTreeWrapper* m_pkTree;
		int32_t m_iEffectID;
	} m_kXMas;

	void __XMasTree_Initialize();
	void __XMasTree_Create(float x, float y, float z, const char* c_szTreeName, const char* c_szEffName);

	typedef robin_hood::unordered_map<uint32_t, int32_t> TSpecialEffectMap;
	TSpecialEffectMap m_kMap_dwID_iEffectID;

	bool m_bSettingTerrainVisible;
	bool m_bSettingIsLavaMap;

public:
	void XMasTree_Destroy();
	void XMasTree_Set(float x, float y, float z, const char* c_szTreeName, const char* c_szEffName);
	void SpecialEffect_Create(uint32_t dwID, float x, float y, float z, const char* c_szEffName);
	void SpecialEffect_Delete(uint32_t dwID);
	void SpecialEffect_Destroy();
	void SetEnvironmentDataName(const std::string& strEnvironmentDataName);
	std::string& GetEnvironmentDataName();
	float GetShadowDistance()
	{
		return m_fShadowDistance;
	};

protected:
	std::string m_settings_envDataName;
	std::string m_envDataName;
	float m_fShadowSizeX;
	float m_fShadowSizeY;
	float m_fShadowDistance;
};

template <class T> bool CMapOutdoor::IsUsedSectorObject<T>::operator()(std::unique_ptr<T>& p)
{
	uint16_t wReferenceCoordX = m_CurCoordinate.m_sTerrainCoordX;
	uint16_t wReferenceCoordY = m_CurCoordinate.m_sTerrainCoordY;

	uint16_t wCoordX, wCoordY;
	p->GetCoordinate(&wCoordX, &wCoordY);

	switch (m_eLRDeleteDir)
	{
	case DELETE_LEFT:
		if (wCoordX < wReferenceCoordX - LOAD_SIZE_WIDTH)
			return false;
		break;
	case DELETE_RIGHT:
		if (wCoordX > wReferenceCoordX + LOAD_SIZE_WIDTH)
			return false;
		break;
	}

	switch (m_eTBDeleteDir)
	{
	case DELETE_TOP:
		if (wCoordY < wReferenceCoordY - LOAD_SIZE_WIDTH)
			return false;
		break;
	case DELETE_BOTTOM:
		if (wCoordY > wReferenceCoordY + LOAD_SIZE_WIDTH)
			return false;
		break;
	}

	return true;
}