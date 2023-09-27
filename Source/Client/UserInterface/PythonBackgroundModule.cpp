#include "StdAfx.h"
#include "PythonSystem.h"
#include "PythonBackground.h"
#include <EterLib/StateManager.h>
#include <GameLib/MapOutDoor.h>


static void backgroundEnableSnow(bool nIsEnable)
{
	auto rkBG=CPythonBackground::GetInstance();
	if (nIsEnable)
		rkBG->EnableSnowEnvironment();
	else
		rkBG->DisableSnowEnvironment();
}

static std::tuple<int32_t, int32_t> backgroundGlobalPositionToLocalPosition(int32_t iX, int32_t iY)
{
	int32_t lX=iX;
	int32_t lY=iY;	
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->GlobalPositionToLocalPosition(lX, lY);

	return std::make_tuple( lX, lY);
}

static std::tuple<std::string, uint32_t, uint32_t> backgroundGlobalPositionToMapInfo(int32_t iX, int32_t iY)
{
	auto rkBG=CPythonBackground::GetInstance();
	CPythonBackground::TMapInfo* pkMapInfo=rkBG->GlobalPositionToMapInfo(iX, iY);
	
	if (pkMapInfo)
		return std::make_tuple( pkMapInfo->m_strName, pkMapInfo->m_dwBaseX, pkMapInfo->m_dwBaseY);	else
	
	return std::make_tuple( "", 0U, 0U);
}

static uint32_t backgroundGetRenderShadowTime()
{
	auto rkBG=CPythonBackground::GetInstance();
	return  rkBG->GetRenderShadowTime();
}

static void backgroundLoadMap(std::string pszMapPathName, float x, float y, float z)
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->LoadMap(pszMapPathName, x, y, z);
}

static void backgroundDestroy()
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->SetShadowTargetLevel(CPythonBackground::SHADOW_NONE);
	rkBG->SetShadowQualityLevel(CPythonBackground::SHADOW_NONE);
	rkBG->Destroy();
}

static void backgroundRegisterEnvironmentData(uint32_t iIndex, std::string pszEnvironmentFileName)
{
	auto rkBG=CPythonBackground::GetInstance();
 	if (!rkBG->RegisterEnvironmentData(iIndex, pszEnvironmentFileName.c_str()))
	{
		TraceLog("background.RegisterEnvironmentData(iIndex={}, szEnvironmentFileName={})", iIndex, pszEnvironmentFileName);
	}
}

static void backgroundSetEnvironmentData(uint32_t iIndex)
{
	const TEnvironmentData * c_pEnvironmenData;

	auto rkBG=CPythonBackground::GetInstance();
	if (rkBG->GetEnvironmentData(iIndex, &c_pEnvironmenData))
		rkBG->ResetEnvironmentDataPtr(c_pEnvironmenData);
}

static std::string backgroundGetCurrentMapName()
{
	auto rkBG=CPythonBackground::GetInstance();
	return  rkBG->GetWarpMapName();
}

static std::tuple<float,float,float> backgroundGetPickingPoint()
{
	auto rkBG=CPythonBackground::GetInstance();
	TPixelPosition kPPosPicked(0.0f, 0.0f, 0.0f);
	if (rkBG->GetPickingPoint(&kPPosPicked))
	{
		kPPosPicked.y=-kPPosPicked.y;
	}
	return std::make_tuple( kPPosPicked.x, kPPosPicked.y, kPPosPicked.z);
}

static void backgroundBeginEnvironment()
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->BeginEnvironment();
}

static void backgroundEndEnvironment()
{
	auto rkBG = CPythonBackground::GetInstance();
	rkBG->EndEnvironment();
}

static void backgroundSetCharacterDirLight()
{
	auto rkBG = CPythonBackground::GetInstance();
	rkBG->SetCharacterDirLight();
}

static void backgroundSetBackgroundDirLight()
{
	auto rkBG = CPythonBackground::GetInstance();
	rkBG->SetBackgroundDirLight();
}

static void backgroundInitialize()
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->Create();
}

static void backgroundUpdate(float fCameraX, float fCameraY, float fCameraZ)
{
	CPythonBackground::GetInstance()->Update(fCameraX, fCameraY, fCameraZ);
}

static void backgroundRender()
{
	CPythonBackground::GetInstance()->Render();
}

static void backgroundRenderPCBlocker()
{
	CPythonBackground::GetInstance()->RenderPCBlocker();
}

static void backgroundRenderCollision()
{
	CPythonBackground::GetInstance()->RenderCollision();
}

static void backgroundRenderSky()
{
	CPythonBackground::GetInstance()->RenderSky();
}

static void backgroundRenderCloud()
{
	CPythonBackground::GetInstance()->RenderCloud();
}

static void backgroundRenderWater()
{
	CPythonBackground::GetInstance()->RenderWater();
}

static void backgroundRenderEffect()
{
	CPythonBackground::GetInstance()->RenderEffect();
}

static void backgroundRenderBeforeLensFlare()
{
	CPythonBackground::GetInstance()->RenderBeforeLensFlare();
}

static void backgroundRenderAfterLensFlare()
{
	CPythonBackground::GetInstance()->RenderAfterLensFlare();
}

static void backgroundRenderCharacterShadowToTexture()
{
	CPythonBackground::GetInstance()->RenderCharacterShadowToTexture();
}

static void backgroundRenderDungeon()
{
	assert(!"background.RenderDungeon() - ������� �ʴ� �Լ��Դϴ� - [levites]");
}

static float backgroundGetHeight(float fx, float fy)
{
	return CPythonBackground::GetInstance()->GetHeight(fx, fy);
}

static void backgroundSetVisiblePart(int32_t ePart, bool isVisible)
{
	if (ePart>=CMapOutdoor::PART_NUM)
		throw std::runtime_error("ePart(" + std::to_string(ePart) + std::string(")<background.PART_NUM(") + std::to_string(CMapOutdoor::PART_NUM) + std::string(")"));

	auto rkBG=CPythonBackground::GetInstance();
	rkBG->SetVisiblePart(ePart, isVisible);
}

static uint32_t backgroundGetShadowMapColor(float fx, float fy)
{
	return CPythonBackground::GetInstance()->GetShadowMapColor(fx, fy);
}

static void backgroundSetSplatLimit(int32_t iSplatNum)
{
	if (iSplatNum < 0)
		throw std::runtime_error("background.SetSplatLimit(iSplatNum(" + std::to_string(iSplatNum) + std::string(")>=0)"));

	auto rkBG = CPythonBackground::GetInstance();
	rkBG->SetSplatLimit(iSplatNum);
}

static std::tuple<int32_t, int32_t,float,std::string> backgroundGetRenderedSplatNum()
{
	int32_t iPatch;
	int32_t iSplat;
	float fSplatRatio;

	std::vector<int32_t> & aTextureNumVector = CPythonBackground::GetInstance()->GetRenderedSplatNum(&iPatch, &iSplat, &fSplatRatio);

	char szOutput[MAX_PATH] = "";
	int32_t iOutput = 0;
	for( std::vector<int32_t>::iterator it = aTextureNumVector.begin(); it != aTextureNumVector.end(); it++ ) {
		iOutput += snprintf(szOutput + iOutput, sizeof(szOutput) - iOutput, "%d ", *it);
	}

	return std::make_tuple( iPatch, iSplat, fSplatRatio, szOutput);
}

static void backgroundSelectViewDistanceNum(int32_t iNum)
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->SelectViewDistanceNum(iNum);
}

static void backgroundSetViewDistanceSet(int32_t iNum, float fFarClip)
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->SetViewDistanceSet(iNum, fFarClip);
}

static float backgroundGetFarClip()
{
	return CPythonBackground::GetInstance()->GetFarClip();
}

static std::tuple<int32_t,float,float,float> backgroundGetDistanceSetInfo()
{
	int32_t iNum;
	float fStart, fEnd, fFarClip;
	CPythonBackground::GetInstance()->GetDistanceSetInfo(&iNum, &fStart, &fEnd, &fFarClip);
	return std::make_tuple( iNum, fStart, fEnd, fFarClip);
}

static void backgroundSetRenderSort(int32_t eSort)
{
	CPythonBackground::GetInstance()->SetTerrainRenderSort(CMapOutdoor::ETerrainRenderSort(eSort));
}

static void backgroundSetTransparentTree(bool bTransparent)
{
	CPythonBackground::GetInstance()->SetTransparentTree(bTransparent);
}

static void backgroundSetXMasTree(int32_t iGrade)
{
	CPythonBackground::GetInstance()->SetXMaxTree(iGrade);
}

static void backgroundRegisterDungeonMapName(std::string szName)
{
	CPythonBackground::GetInstance()->RegisterDungeonMapName(szName.c_str());
}

static void backgroundVisibleGuildArea()
{
	CPythonBackground::GetInstance()->VisibleGuildArea();
}

static void backgroundDisableGuildArea()
{
	CPythonBackground::GetInstance()->DisableGuildArea();
}

static void backgroundWarpTest(uint32_t iX, uint32_t iY)
{
	CPythonBackground::GetInstance()->Warp(iX * 100U , iY * 100U);
}

static void backgroundSetShadowTargetLevel(int32_t level)
{
	CPythonBackground::GetInstance()->SetShadowTargetLevel(level);
}

static void backgroundSetShadowQualityLevel(int32_t quality)
{
	CPythonBackground::GetInstance()->SetShadowQualityLevel(quality);
}

PYBIND11_EMBEDDED_MODULE(background, m)
{
	m.def("EnableSnow",	backgroundEnableSnow);
	m.def("GlobalPositionToLocalPosition",	backgroundGlobalPositionToLocalPosition);
	m.def("GlobalPositionToMapInfo",	backgroundGlobalPositionToMapInfo);
	m.def("GetRenderShadowTime",	backgroundGetRenderShadowTime);
	m.def("LoadMap",	backgroundLoadMap);
	m.def("Destroy",	backgroundDestroy);
	m.def("RegisterEnvironmentData",	backgroundRegisterEnvironmentData);
	m.def("SetEnvironmentData",	backgroundSetEnvironmentData);
	m.def("GetCurrentMapName",	backgroundGetCurrentMapName);
	m.def("GetPickingPoint",	backgroundGetPickingPoint);
	m.def("BeginEnvironment",	backgroundBeginEnvironment);
	m.def("EndEnvironment",	backgroundEndEnvironment);
	m.def("SetCharacterDirLight",	backgroundSetCharacterDirLight);
	m.def("SetBackgroundDirLight",	backgroundSetBackgroundDirLight);
	m.def("Initialize",	backgroundInitialize);
	m.def("Update",	backgroundUpdate);
	m.def("Render",	backgroundRender);
	m.def("RenderPCBlocker",	backgroundRenderPCBlocker);
	m.def("RenderCollision",	backgroundRenderCollision);
	m.def("RenderSky",	backgroundRenderSky);
	m.def("RenderCloud",	backgroundRenderCloud);
	m.def("RenderWater",	backgroundRenderWater);
	m.def("RenderEffect",	backgroundRenderEffect);
	m.def("RenderBeforeLensFlare",	backgroundRenderBeforeLensFlare);
	m.def("RenderAfterLensFlare",	backgroundRenderAfterLensFlare);
	m.def("RenderCharacterShadowToTexture",	backgroundRenderCharacterShadowToTexture);
	m.def("RenderDungeon",	backgroundRenderDungeon);
	m.def("GetHeight",	backgroundGetHeight);
	m.def("SetVisiblePart",	backgroundSetVisiblePart);
	m.def("GetShadowMapColor",	backgroundGetShadowMapColor);
	m.def("SetSplatLimit",	backgroundSetSplatLimit);
	m.def("GetRenderedSplatNum",	backgroundGetRenderedSplatNum);
	m.def("SelectViewDistanceNum",	backgroundSelectViewDistanceNum);
	m.def("SetViewDistanceSet",	backgroundSetViewDistanceSet);
	m.def("GetFarClip",	backgroundGetFarClip);
	m.def("GetDistanceSetInfo",	backgroundGetDistanceSetInfo);
	m.def("SetRenderSort",	backgroundSetRenderSort);
	m.def("SetTransparentTree",	backgroundSetTransparentTree);
	m.def("SetXMasTree",	backgroundSetXMasTree);
	m.def("RegisterDungeonMapName",	backgroundRegisterDungeonMapName);
	m.def("VisibleGuildArea",	backgroundVisibleGuildArea);
	m.def("DisableGuildArea",	backgroundDisableGuildArea);
	m.def("WarpTest",	backgroundWarpTest);
	m.def("SetShadowTargetLevel", backgroundSetShadowTargetLevel);
	m.def("SetShadowQualityLevel", backgroundSetShadowQualityLevel);

	m.attr("PART_SKY") = int32_t(CMapOutdoor::PART_SKY);
	m.attr("PART_TREE") = int32_t(CMapOutdoor::PART_TREE);
	m.attr("PART_CLOUD") = int32_t(CMapOutdoor::PART_CLOUD);
	m.attr("PART_WATER") = int32_t(CMapOutdoor::PART_WATER);
	m.attr("PART_OBJECT") = int32_t(CMapOutdoor::PART_OBJECT);
	m.attr("PART_TERRAIN") = int32_t(CMapOutdoor::PART_TERRAIN);
	m.attr("SKY_RENDER_MODE_DEFAULT") = int32_t(CSkyObject::SKY_RENDER_MODE_DEFAULT);
	m.attr("SKY_RENDER_MODE_DIFFUSE") = int32_t(CSkyObject::SKY_RENDER_MODE_DIFFUSE);
	m.attr("SKY_RENDER_MODE_TEXTURE") = int32_t(CSkyObject::SKY_RENDER_MODE_TEXTURE);
	m.attr("SKY_RENDER_MODE_MODULATE") = int32_t(CSkyObject::SKY_RENDER_MODE_MODULATE);
	m.attr("SKY_RENDER_MODE_MODULATE2X") = int32_t(CSkyObject::SKY_RENDER_MODE_MODULATE2X);
	m.attr("SKY_RENDER_MODE_MODULATE4X") = int32_t(CSkyObject::SKY_RENDER_MODE_MODULATE4X);

	m.attr("SHADOW_NONE") = int32_t(CPythonBackground::SHADOW_NONE);
	m.attr("SHADOW_SELF") = int32_t(CPythonBackground::SHADOW_SELF_ONLY);
	m.attr("SHADOW_ALL") = int32_t(CPythonBackground::SHADOW_ALL);

	m.attr("SHADOW_BAD") = int32_t(CPythonBackground::SHADOW_BAD);
	m.attr("SHADOW_AVERAGE") = int32_t(CPythonBackground::SHADOW_AVERAGE);
	m.attr("SHADOW_GOOD") = int32_t(CPythonBackground::SHADOW_GOOD);
	m.attr("SHADOW_EPIC") = int32_t(CPythonBackground::SHADOW_EPIC);

	m.attr("DISTANCE0") = int32_t(CPythonBackground::DISTANCE0);
	m.attr("DISTANCE1") = int32_t(CPythonBackground::DISTANCE1);
	m.attr("DISTANCE2") = int32_t(CPythonBackground::DISTANCE2);
	m.attr("DISTANCE3") = int32_t(CPythonBackground::DISTANCE3);
	m.attr("DISTANCE4") = int32_t(CPythonBackground::DISTANCE4);
	m.attr("DISTANCE_SORT") = int32_t(CMapOutdoor::DISTANCE_SORT);
	m.attr("TEXTURE_SORT") = int32_t(CMapOutdoor::TEXTURE_SORT);
}
