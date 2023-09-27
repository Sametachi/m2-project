#pragma once
class CProperty;
#include <EterLib/SkyBox.h>

/////////////////////////////////////////////////////////////////
// Property
namespace prt
{
	enum EPropertyType
	{
		PROPERTY_TYPE_NONE,
		PROPERTY_TYPE_TREE,
		PROPERTY_TYPE_BUILDING,
		PROPERTY_TYPE_EFFECT,
		PROPERTY_TYPE_AMBIENCE,
		PROPERTY_TYPE_DUNGEON_BLOCK,
		PROPERTY_TYPE_MAX_NUM
	};

	const char c_szPropertyTypeName[PROPERTY_TYPE_MAX_NUM][32] = { "None",   "Tree",     "Building", "Effect", "Ambience", "DungeonBlock" };
	const char c_szPropertyExtension[PROPERTY_TYPE_MAX_NUM][16] = { ".pr", ".prt", ".prb", ".pre", ".pra", ".prd" };

	uint32_t GetPropertyType(const char* c_szTypeName);
	const char* GetPropertyExtension(uint32_t dwType);

	struct TPropertyTree
	{
		std::string strName;
		std::string	strFileName;

		float		fSize;
		float		fVariance;
	};

	struct TPropertyBuilding
	{
		std::string strName;
		std::string	strFileName;
		std::string	strAttributeDataFileName;
		bool isShadowFlag;
	};

	struct TPropertyEffect
	{
		std::string	strName;
		std::string	strFileName;
	};

	enum EAmbiencePlayType
	{
		AMBIENCE_PLAY_TYPE_ONCE,
		AMBIENCE_PLAY_TYPE_STEP,
		AMBIENCE_PLAY_TYPE_LOOP,
		AMBIENCE_PLAY_TYPE_MAX_NUM
	};

	const char c_szPlayTypeName[AMBIENCE_PLAY_TYPE_MAX_NUM][8] =
	{
		"ONCE",
		"STEP",
		"LOOP",
	};

	struct TPropertyAmbience
	{
		std::string strName;
		std::string strPlayType;

		float fPlayInterval;
		float fPlayIntervalVariation;
		float fMaxVolumeAreaPercentage;
		std::vector<std::string> AmbienceSoundVector;
	};

	struct TPropertyDungeonBlock
	{
		std::string strName;
		std::string	strFileName;
		std::string	strAttributeDataFileName;
	};

	bool PropertyTreeDataToString(TPropertyTree* pData, CProperty* pProperty);
	bool PropertyTreeStringToData(CProperty* pProperty, TPropertyTree* pData);
	bool PropertyBuildingDataToString(TPropertyBuilding* pData, CProperty* pProperty);
	bool PropertyBuildingStringToData(CProperty* pProperty, TPropertyBuilding* pData);
	bool PropertyEffectDataToString(TPropertyEffect* pData, CProperty* pProperty);
	bool PropertyEffectStringToData(CProperty* pProperty, TPropertyEffect* pData);
	bool PropertyAmbienceDataToString(TPropertyAmbience* pData, CProperty* pProperty);
	bool PropertyAmbienceStringToData(CProperty* pProperty, TPropertyAmbience* pData);
	bool PropertyDungeonBlockDataToString(TPropertyDungeonBlock* pData, CProperty* pProperty);
	bool PropertyDungeonBlockStringToData(CProperty* pProperty, TPropertyDungeonBlock* pData);
};

// Environment
enum
{
	ENV_DIRLIGHT_BACKGROUND,
	ENV_DIRLIGHT_CHARACTER,
	ENV_DIRLIGHT_NUM
};

typedef struct SEnvironmentData
{
	// Light
	bool bDirLightsEnable[ENV_DIRLIGHT_NUM];
	D3DLIGHT9 DirLights[ENV_DIRLIGHT_NUM];

	// Material
	D3DMATERIAL9 Material;

	// Fog
	mutable bool bFogEnable;
	bool bDensityFog;

	float m_fFogNearDistance;
	float m_fFogFarDistance;

	float GetFogNearDistance() const;
	float GetFogFarDistance() const;

	D3DXCOLOR FogColor;

	// Filtering
	bool bFilteringEnable;
	D3DXCOLOR FilteringColor;
	uint8_t byFilteringAlphaSrc;
	uint8_t byFilteringAlphaDest;

	// Wind
	float fWindStrength;
	float fWindRandom;

	// SkyBox
	D3DXVECTOR3 v3SkyBoxScale;
	bool bSkyBoxTextureRenderMode;

	uint8_t bySkyBoxGradientLevelUpper;
	uint8_t bySkyBoxGradientLevelLower;

	std::string strSkyBoxFaceFileName[6]; //order : front/back/left/right/top/bottom

	D3DXVECTOR2 v2CloudScale;
	float fCloudHeight;
	D3DXVECTOR2 v2CloudTextureScale;
	D3DXVECTOR2 v2CloudSpeed;
	std::string strCloudTextureFileName;
	TGradientColor CloudGradientColor;

	std::vector<TGradientColor> SkyBoxGradientColorVector;

	// LensFlare
	bool bLensFlareEnable;
	D3DXCOLOR LensFlareBrightnessColor;
	float fLensFlareMaxBrightness;

	bool bMainFlareEnable;
	std::string strMainFlareTextureFileName;
	float fMainFlareSize;

	bool bReserve;
} TEnvironmentData;

typedef std::map<uint32_t, TEnvironmentData*> TEnvironmentDataMap;

typedef struct SScreenPosition
{
	int32_t x;
	int32_t y;
} TScreenPosition;

typedef D3DXVECTOR3 TPixelPosition;

float SPixelPosition_CalculateDistanceSq3d(const TPixelPosition& c_rkPPosLeft, const TPixelPosition& c_rkPPosRight);

typedef struct SCellPosition
{
	SCellPosition()
	{
		this->x = 0;
		this->y = 0;
	}

	SCellPosition(int32_t x, int32_t y)
	{
		this->x = x;
		this->y = y;
	}

	int32_t x, y;
} TCellPosition;

const int32_t c_Section_xAttributeCellSize = 50;
const int32_t c_Section_yAttributeCellSize = 50;
const int32_t c_Section_xCellSize = 100;
const int32_t c_Section_yCellSize = 100;
