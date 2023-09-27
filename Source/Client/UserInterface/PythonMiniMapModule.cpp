#include "StdAfx.h"
#include "PythonMiniMap.h"


static void minimapSetScale(float fScale)
{

	CPythonMiniMap::GetInstance()->SetScale(fScale);

}

static void minimapScaleUp()
{
	CPythonMiniMap::GetInstance()->ScaleUp();

}

static void minimapScaleDown()
{
	CPythonMiniMap::GetInstance()->ScaleDown();

}

static void minimapSetMiniMapSize(float fWidth, float fHeight)
{

	CPythonMiniMap::GetInstance()->SetMiniMapSize(fWidth, fHeight);

}

static void minimapSetCenterPosition(float fCenterX, float fCenterY)
{
		
	CPythonMiniMap::GetInstance()->SetCenterPosition(fCenterX, fCenterY);

}

static void minimapDestroy()
{
	CPythonMiniMap::GetInstance()->Destroy();

}

static void minimapCreate()
{
	CPythonMiniMap::GetInstance()->Create();

}

static void minimapUpdate(float fCenterX, float fCenterY)
{

	CPythonMiniMap::GetInstance()->Update(fCenterX, fCenterY);

}

static void minimapRender(float fScrrenX, float fScrrenY)
{

	CPythonMiniMap::GetInstance()->Render(fScrrenX, fScrrenY);

}

static void minimapShow()
{
	CPythonMiniMap::GetInstance()->Show();

}

static void minimapHide()
{
	CPythonMiniMap::GetInstance()->Hide();

}

static bool minimapisShow()
{
	return CPythonMiniMap::GetInstance()->CanShow();
}

static std::tuple<bool,std::string, int32_t, int32_t, uint32_t> minimapGetInfo(float fScrrenX, float fScrrenY)
{
	
	std::string aString;
	float fPosX, fPosY;
	uint32_t dwTextColor;
	bool bFind = CPythonMiniMap::GetInstance()->GetPickedInstanceInfo(fScrrenX, fScrrenY, aString, &fPosX, &fPosY, &dwTextColor);
	int32_t iPosX, iPosY;
	PR_FLOAT_TO_INT(fPosX, iPosX);
	PR_FLOAT_TO_INT(fPosY, iPosY);
	iPosX /= 100;
	iPosY /= 100;
	return std::make_tuple(bFind, aString, iPosX, iPosY, dwTextColor);
}

static void minimapLoadAtlas()
{
	if (!CPythonMiniMap::GetInstance()->LoadAtlas())
		TraceLog("CPythonMiniMap::GetInstance()->LoadAtlas() Failed");

}

static void minimapUpdateAtlas()
{
	CPythonMiniMap::GetInstance()->UpdateAtlas();

}

static void minimapRenderAtlas(float fScrrenX, float fScrrenY)
{
		
	CPythonMiniMap::GetInstance()->RenderAtlas(fScrrenX, fScrrenY);

}

static void minimapShowAtlas()
{
	CPythonMiniMap::GetInstance()->ShowAtlas();

}

static void minimapHideAtlas()
{
	CPythonMiniMap::GetInstance()->HideAtlas();

}

static bool minimapisShowAtlas()
{
	return CPythonMiniMap::GetInstance()->CanShowAtlas();
}

static bool minimapIsAtlas()
{
	return CPythonMiniMap::GetInstance()->IsAtlas();
}

static std::tuple<bool,std::string,int32_t, int32_t, uint32_t, uint32_t> minimapGetAtlasInfo(float fScrrenX, float fScrrenY)
{

	std::string aString = "";
	float fPosX = 0.0f;
	float fPosY = 0.0f;
	uint32_t dwTextColor = 0;
	uint32_t dwGuildID = 0;
	bool bFind = CPythonMiniMap::GetInstance()->GetAtlasInfo(fScrrenX, fScrrenY, aString, &fPosX, &fPosY, &dwTextColor, &dwGuildID);
	int32_t iPosX, iPosY;
	PR_FLOAT_TO_INT(fPosX, iPosX);
	PR_FLOAT_TO_INT(fPosY, iPosY);
	iPosX /= 100;
	iPosY /= 100;
	return std::make_tuple(bFind, aString, iPosX, iPosY, dwTextColor, dwGuildID);
}

static std::tuple<bool, int32_t, int32_t> minimapGetAtlasSize()
{
	float fSizeX, fSizeY;
	bool bGet = CPythonMiniMap::GetInstance()->GetAtlasSize(&fSizeX, &fSizeY);

/*
	float fSizeXoo256 = fSizeX / 256.0f;
	float fSizeYoo256 = fSizeY / 256.0f;

	if (fSizeXoo256 >= fSizeYoo256)
	{
		fSizeX /= fSizeYoo256;
		fSizeY = 256.0f;
	}
	else
	{
		fSizeX = 256.0f;
		fSizeY /= fSizeXoo256;
	}
*/

	int32_t iSizeX, iSizeY;
	PR_FLOAT_TO_INT(fSizeX, iSizeX);
	PR_FLOAT_TO_INT(fSizeY, iSizeY);

	return std::make_tuple( bGet, iSizeX, iSizeY);
}

static void minimapAddWayPoint(int iID, float fX, float fY, std::string buf)
{
		
	CPythonMiniMap::GetInstance()->AddWayPoint(CPythonMiniMap::TYPE_WAYPOINT, (uint32_t)iID, fX, fY, buf);
	

}

static void minimapRemoveWayPoint(uint32_t iID)
{
	
	CPythonMiniMap::GetInstance()->RemoveWayPoint(iID);
	

}

static void minimapRegisterAtlasWindow(pybind11::handle poHandler)
{
	CPythonMiniMap::GetInstance()->RegisterAtlasWindow(poHandler);

}

static void minimapUnregisterAtlasWindow()
{
	CPythonMiniMap::GetInstance()->UnregisterAtlasWindow();

}

static uint32_t minimapGetGuildAreaID(float fx, float fy)
{

	return CPythonMiniMap::GetInstance()->GetGuildAreaID(fx, fy);
}



PYBIND11_EMBEDDED_MODULE(miniMap, m)
{
	m.def("SetScale",	minimapSetScale);
	m.def("ScaleUp",	minimapScaleUp);
	m.def("ScaleDown",	minimapScaleDown);
	m.def("SetMiniMapSize",	minimapSetMiniMapSize);
	m.def("SetCenterPosition",	minimapSetCenterPosition);
	m.def("Destroy",	minimapDestroy);
	m.def("Create",	minimapCreate);
	m.def("Update",	minimapUpdate);
	m.def("Render",	minimapRender);
	m.def("Show",	minimapShow);
	m.def("Hide",	minimapHide);
	m.def("isShow",	minimapisShow);
	m.def("GetInfo",	minimapGetInfo);
	m.def("LoadAtlas",	minimapLoadAtlas);
	m.def("UpdateAtlas",	minimapUpdateAtlas);
	m.def("RenderAtlas",	minimapRenderAtlas);
	m.def("ShowAtlas",	minimapShowAtlas);
	m.def("HideAtlas",	minimapHideAtlas);
	m.def("isShowAtlas",	minimapisShowAtlas);
	m.def("IsAtlas",	minimapIsAtlas);
	m.def("GetAtlasInfo",	minimapGetAtlasInfo);
	m.def("GetAtlasSize",	minimapGetAtlasSize);
	m.def("AddWayPoint",	minimapAddWayPoint);
	m.def("RemoveWayPoint",	minimapRemoveWayPoint);
	m.def("RegisterAtlasWindow",	minimapRegisterAtlasWindow);
	m.def("UnregisterAtlasWindow",	minimapUnregisterAtlasWindow);
	m.def("GetGuildAreaID",	minimapGetGuildAreaID);

	m.attr("TYPE_OPC") = int32_t(CPythonMiniMap::TYPE_OPC);
	m.attr("TYPE_OPCPVP") = int32_t(CPythonMiniMap::TYPE_OPCPVP);
	m.attr("TYPE_OPCPVPSELF") = int32_t(CPythonMiniMap::TYPE_OPCPVPSELF);
	m.attr("TYPE_NPC") = int32_t(CPythonMiniMap::TYPE_NPC);
	m.attr("TYPE_MONSTER") = int32_t(CPythonMiniMap::TYPE_MONSTER);
	m.attr("TYPE_WARP") = int32_t(CPythonMiniMap::TYPE_WARP);
	m.attr("TYPE_WAYPOINT") = int32_t(CPythonMiniMap::TYPE_WAYPOINT);
	m.attr("TYPE_PARTY") = int32_t(CPythonMiniMap::TYPE_PARTY);
	m.attr("TYPE_EMPIRE") = int32_t(CPythonMiniMap::TYPE_EMPIRE);
}
