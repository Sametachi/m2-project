#pragma once
#include <SpeedTreeLib/SpeedTreeForest.h>
#include <SpeedTreeLib/SpeedTreeForestDirectX9.h>
#include "MapOutdoor.h"
#include "PropertyManager.h"
#include "PhysicsObject.h"
#include <unordered_map>
class CMapBase;

class CMapManager : public CScreen, public IPhysicsWorld
{
public:
	CMapManager();
	virtual ~CMapManager();
	bool IsMapOutdoor();
	CMapOutdoor& GetMapOutdoorRef();

	void Initialize();
	void Destroy();
	void Create();
	virtual void Clear();
	virtual CMapBase* AllocMap();

	// Map 
	bool IsMapReady();
	virtual bool LoadMap(const std::string& c_rstrMapName, float x, float y, float z);
	bool UnloadMap(const std::string& c_strMapName);

	bool UpdateMap(float fx, float fy, float fz);
	void UpdateAroundAmbience(float fx, float fy, float fz);
	float GetHeight(float fx, float fy);
	float GetTerrainHeight(float fx, float fy);
	bool GetWaterHeight(int32_t iX, int32_t iY, int32_t* plWaterHeight);
	bool GetNormal(int32_t ix, int32_t iy, D3DXVECTOR3* pv3Normal);

	// Environment
	void SetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData);
	void ResetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData);
	void SetEnvironmentData(int32_t nEnvDataIndex);
	void BeginEnvironment();
	void EndEnvironment();
	void BlendEnvironmentData(const TEnvironmentData* c_pEnvironmentData, int32_t iTransitionTime);
	void GetCurrentEnvironmentData(const TEnvironmentData** c_ppEnvironmentData);
	bool RegisterEnvironmentData(uint32_t dwIndex, const char* c_szFileName);
	bool GetEnvironmentData(uint32_t dwIndex, const TEnvironmentData** c_ppEnvironmentData);

	// Portal
	void RefreshPortal();
	void ClearPortal();
	void AddShowingPortalID(int32_t iID);

	// External interface
	void LoadProperty();
	uint32_t GetShadowMapColor(float fx, float fy);
	virtual bool isPhysicalCollision(const D3DXVECTOR3& c_rvCheckPosition);

	bool isAttrOn(float fX, float fY, uint8_t byAttr);
	bool GetAttr(float fX, float fY, uint8_t* pbyAttr);
	bool isAttrOn(int32_t iX, int32_t iY, uint8_t byAttr);
	bool GetAttr(int32_t iX, int32_t iY, uint8_t* pbyAttr);

	std::vector<int32_t>& GetRenderedSplatNum(int32_t* piPatch, int32_t* piSplat, float* pfSplatRatio);
	void SetEnvironmentFog(bool flag);

protected:
	TEnvironmentData* AllocEnvironmentData();
	void DeleteEnvironmentData(TEnvironmentData* pEnvironmentData);
	bool LoadEnvironmentData(const char* c_szFileName, TEnvironmentData* pEnvironmentData);

	CPropertyManager m_PropertyManager;
	TEnvironmentDataMap m_EnvironmentDataMap;
	const TEnvironmentData* mc_pcurEnvironmentData;
	CMapOutdoor* m_pkMap;
	CSpeedTreeForestDirectX9 m_Forest;

public:
	void SetTerrainRenderSort(CMapOutdoor::ETerrainRenderSort eTerrainRenderSort);
	CMapOutdoor::ETerrainRenderSort	GetTerrainRenderSort();
	void SetTransparentTree(bool bTransparenTree);

public:
	typedef struct MapInfo
	{
		std::string	m_strName;
		std::string	m_strLocaleName;
		uint32_t m_dwBaseX{};
		uint32_t m_dwBaseY{};
		uint32_t m_dwSizeX{};
		uint32_t m_dwSizeY{};
		uint32_t m_dwEndX{};
		uint32_t m_dwEndY{};
	} TMapInfo;
	typedef std::vector<TMapInfo>		TMapInfoVector;
	typedef TMapInfoVector::iterator	TMapInfoVectorIterator;

	const TMapInfoVector& GetMapInfoVector()
	{
		return m_kVct_kMapInfo;
	}

protected:
	TMapInfoVector m_kVct_kMapInfo;
	void	__LoadMapInfoVector();

	struct FFindMapName
	{
		std::string strNametoFind;
		FFindMapName(const std::string& c_rMapName)
		{
			strNametoFind = c_rMapName;
			stl_lowers(strNametoFind);
		}
		bool operator() (TMapInfo& rMapInfo)
		{
			return rMapInfo.m_strName == strNametoFind;
		}
	};
public:
	void SetAtlasInfoFileName(const char* filename)
	{
		m_stAtlasInfoFileName = filename;
	}
private:
	std::string m_stAtlasInfoFileName;
};
