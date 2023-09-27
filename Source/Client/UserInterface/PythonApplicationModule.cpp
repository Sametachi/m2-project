#include "StdAfx.h"
#include "Resource.h"
#include "PythonApplication.h"
#include "../../Libraries/eterLib/Camera.h"

extern D3DXCOLOR g_fSpecularColor;
extern bool bVisibleNotice = true;
extern double g_specularSpd;
extern int32_t g_iAccumulationTime;

// TEXTTAIL_LIVINGTIME_CONTROL
extern void TextTail_SetLivingTime(int32_t livingTime);
extern bool LoadLocaleData(const char* localePath);

static void appSetTextTailLivingTime(float livingTime)
{
	TextTail_SetLivingTime(int32_t(livingTime*1000.0f));
}

static void appSetCameraMaxDistance(float fMax)
{
	CCamera::SetCameraMaxDistance(fMax);
}

static void appSetMinFog(float fMinFog)
{
	CPythonApplication::GetInstance()->SetMinFog(fMinFog);
}

static void appSetFrameSkip(bool nFrameSkip)
{
	CPythonApplication::GetInstance()->SetFrameSkip(nFrameSkip);
}

static std::tuple<bool, ILint, ILint> appGetImageInfo(std::string szFileName)
{
	bool canLoad=false;
	ILint uWidth=0;
	ILint uHeight=0;

	ILuint uImg;
	ilGenImages(1, &uImg);
	ilBindImage(uImg);
	if (ilLoad(IL_TYPE_UNKNOWN, szFileName.c_str()))
	{
		canLoad=true;
		uWidth=ilGetInteger(IL_IMAGE_WIDTH);
		uHeight=ilGetInteger(IL_IMAGE_HEIGHT);
	}

	ilDeleteImages(1, &uImg);

	return std::make_tuple( canLoad, uWidth, uHeight);
}

static std::string appGetInfo(UINT nInfo)
{
	std::string stInfo;
	CPythonApplication::GetInstance()->GetInfo(nInfo, &stInfo);
	return  stInfo.c_str();
}

static void appUpdateGame()
{
	CPythonApplication::GetInstance()->UpdateGame();
}

static void appRenderGame()
{
	CPythonApplication::GetInstance()->RenderGame();
}

static void appLoop()
{
	CPythonApplication::GetInstance()->Loop();
}

static void appCreate(std::string szName, int32_t width, int32_t height, int32_t Windowed)
{
	auto rkApp=CPythonApplication::GetInstance();
	if (!rkApp->Create(szName.c_str(), width, height, Windowed))
	{
		throw std::runtime_error("App creation fail");
	}
}

static bool appProcess()
{
	if (CPythonApplication::GetInstance()->Process())
		return true;

	return false;
}

static void appExit()
{
	CPythonApplication::GetInstance()->Exit();
}

static void appAbort()
{
	CPythonApplication::GetInstance()->Abort();
}

static void appSetMouseHandler(pybind11::handle poHandler)
{
	CPythonApplication::GetInstance()->SetMouseHandler(poHandler);
}

static bool appIsExistFile(std::string szFileName)
{
	return CallFS().Exists(szFileName);
}

static std::list<std::string> appGetFileList(std::string szFilter)
{
	std::list<std::string> poList;

	WIN32_FIND_DATAA wfd;
	memset(&wfd, 0, sizeof(wfd));

	HANDLE hFind = FindFirstFileA(szFilter.c_str(), &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{	
		do
		{
			poList.push_back(wfd.cFileName);
		} 			
		while (FindNextFile(hFind, &wfd));
		

		FindClose(hFind);
	}

	return poList;

}

static void appSetCamera(float Distance, float Pitch, float Rotation, float fDestinationHeight)
{
	CPythonApplication::GetInstance()->SetCamera(Distance, Pitch, Rotation, fDestinationHeight);
}

static std::tuple<float,float,float,float> appGetCamera()
{
	float Distance, Pitch, Rotation, DestinationHeight;
    CPythonApplication::GetInstance()->GetCamera(&Distance, &Pitch, &Rotation, &DestinationHeight);

	return std::make_tuple( Distance, Pitch, Rotation, DestinationHeight);
}

static float appGetCameraPitch()
{
	return  CPythonApplication::GetInstance()->GetPitch();
}

static float appGetCameraRotation()
{
	return  CPythonApplication::GetInstance()->GetRotation();
}

static float appGetTime()
{
	return  CPythonApplication::GetInstance()->GetGlobalTime();
}

static time_t appGetGlobalTime()
{
	return  CPythonApplication::GetInstance()->GetServerTime();
}

static time_t appGetGlobalTimeStamp()
{
	return  CPythonApplication::GetInstance()->GetServerTimeStamp();
}

static uint32_t appGetUpdateFPS()
{
	return  CPythonApplication::GetInstance()->GetUpdateFPS();
}

static uint32_t appGetRenderFPS()
{
	return  CPythonApplication::GetInstance()->GetRenderFPS();
}

static void appRotateCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->RotateCamera(iDirection);
}

static void appPitchCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->PitchCamera(iDirection);
}

static void appZoomCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->ZoomCamera(iDirection);
}

static void appMovieRotateCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->MovieRotateCamera(iDirection);
}

static void appMoviePitchCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->MoviePitchCamera(iDirection);
}

static void appMovieZoomCamera(int32_t iDirection)
{
	CPythonApplication::GetInstance()->MovieZoomCamera(iDirection);
}

static void appMovieResetCamera()
{
	CPythonApplication::GetInstance()->MovieResetCamera();
}

static uint32_t appGetAvailableTextureMemory()
{
	return CGraphicBase::GetAvailableTextureMemory();
}

static std::tuple<float,uint32_t> appGetRenderTime()
{
	return std::make_tuple(
		CPythonApplication::GetInstance()->GetAveRenderTime(),
		CPythonApplication::GetInstance()->GetCurRenderTime());
}

static uint32_t appGetUpdateTime()
{
	return  CPythonApplication::GetInstance()->GetCurUpdateTime();
}

static uint32_t appGetLoad()
{
	return  CPythonApplication::GetInstance()->GetLoad();
}

static float appGetFaceSpeed()
{
	return  CPythonApplication::GetInstance()->GetFaceSpeed();
}

static uint32_t appGetFaceCount()
{
	return  CPythonApplication::GetInstance()->GetFaceCount();
}

static void appSetFPS(int32_t iFPS)
{
	CPythonApplication::GetInstance()->SetFPS(iFPS);
}

static void appSetGlobalCenterPosition(int32_t x, int32_t y)
{
	CPythonApplication::GetInstance()->SetGlobalCenterPosition(x, y);
}

static void appSetCenterPosition(float fx, float fy, float fz)
{
	CPythonApplication::GetInstance()->SetCenterPosition(fx, -fy, fz);
}

static std::tuple<int32_t, int32_t> appGetCursorPosition()
{
	auto [lx, ly] = CWindowManager::GetInstance()->GetMousePosition();
	return std::make_tuple(lx, ly);
}

static int32_t appGetRandom(int32_t from, int32_t to)
{
	if (from > to)
	{
		int32_t tmp = from;
		from = to;
		to = tmp;
	}

	return  random_range(from, to);
}

static bool appIsPressed(int32_t iKey)
{
	return Engine::GetKeyboardInput().IsKeyPressed(iKey);
}

static void appSetCursor(int32_t iCursorNum)
{
	if (!CPythonApplication::GetInstance()->SetCursorNum(iCursorNum))
		throw std::runtime_error("Wrong Cursor Name: " + std::to_string(iCursorNum));
}

static int32_t appGetCursor()
{
	return CPythonApplication::GetInstance()->GetCursorNum();
}

static void appShowCursor()
{
	CPythonApplication::GetInstance()->SetCursorVisible(TRUE);
}

static void appHideCursor()
{
	CPythonApplication::GetInstance()->SetCursorVisible(FALSE);
}

static bool appIsShowCursor()
{
	return  TRUE == CPythonApplication::GetInstance()->GetCursorVisible();
}

static bool appIsLiarCursorOn()
{
	return  TRUE == CPythonApplication::GetInstance()->GetLiarCursorOn();
}

static void appSetSoftwareCursor()
{
	CPythonApplication::GetInstance()->SetCursorMode(CPythonApplication::CURSOR_MODE_SOFTWARE);
}

static void appSetHardwareCursor()
{
	CPythonApplication::GetInstance()->SetCursorMode(CPythonApplication::CURSOR_MODE_HARDWARE);
}

static void appSetConnectData(std::string szIP, int32_t iPort)
{
	CPythonApplication::GetInstance()->SetConnectData(szIP.c_str(), iPort);
}

static std::tuple<std::string,int32_t> appGetConnectData()
{
	std::string strIP;
	int32_t iPort;

	CPythonApplication::GetInstance()->GetConnectData(strIP, iPort);

	return std::make_tuple( strIP, iPort);
}

static int32_t appGetRotatingDirection(float fSource, float fTarget)
{
	return  GetRotatingDirection(fSource, fTarget);
}

static float appGetDegreeDifference(float fSource, float fTarget)
{
	return GetDegreeDifference(fSource, fTarget);
}

static void appSleep(DWORD iTime)
{
	Sleep(iTime);
}

static void appSetDefaultFontName(std::string szFontName, std::string italic, std::string bold)
{
	Engine::GetFontManager().SetDefaultFont(szFontName, italic, bold);
}

static void appSetGuildSymbolPath(std::string szPathName)
{
	SetGuildSymbolPath(szPathName.c_str());
}

static void appSetCameraSpeed(int32_t iPercentage)
{
	CPythonApplication::GetInstance()->SetCameraSpeed(iPercentage);

	CCamera * pCamera = CCameraManager::GetInstance()->GetCurrentCamera();
	if (pCamera)
		pCamera->SetResistance(float(iPercentage) / 100.0f);
}

static void appSaveCameraSetting(std::string szFileName)
{
	CPythonApplication::GetInstance()->SaveCameraSetting(szFileName.c_str());
}

static bool appLoadCameraSetting(std::string szFileName)
{
	return CPythonApplication::GetInstance()->LoadCameraSetting(szFileName.c_str());
}

static void appSetDefaultCamera()
{
	CPythonApplication::GetInstance()->SetDefaultCamera();
}

static void appSetCameraSetting(int32_t ix, int32_t iy, int32_t iz, int32_t iZoom, int32_t iRotation, int32_t iPitch)
{
	SCameraSetting CameraSetting;
	ZeroMemory(&CameraSetting, sizeof(CameraSetting));
	CameraSetting.v3CenterPosition.x = float(ix);
	CameraSetting.v3CenterPosition.y = float(iy);
	CameraSetting.v3CenterPosition.z = float(iz);
	CameraSetting.fZoom = float(iZoom);
	CameraSetting.fRotation = float(iRotation);
	CameraSetting.fPitch = float(iPitch);
	CPythonApplication::GetInstance()->SetEventCamera(CameraSetting);
}

static void appSetSightRange(int32_t iRange)
{
	CPythonApplication::GetInstance()->SetForceSightRange(iRange);
}

class CTextLineLoader
{
public:
	CTextLineLoader(std::string c_szFileName)
	{
		auto vfs_string = CallFS().LoadFileToString(CallFS(), c_szFileName);
		if (!vfs_string)
		{
			SysLog("Failed to load {0}", c_szFileName);
			return;
		}

		m_kTextFileLoader.Bind(vfs_string.value());
	}

	uint32_t GetLineCount()
	{
		return m_kTextFileLoader.GetLineCount();
	}

	const char* GetLine(uint32_t dwIndex)
	{
		if (dwIndex >= GetLineCount())
			return "";

		return m_kTextFileLoader.GetLineString(dwIndex).c_str();
	}

protected:
	CMemoryTextFileLoader m_kTextFileLoader;
};

static CTextLineLoader* appOpenTextFile(std::string szFileName)
{
	return new CTextLineLoader(szFileName);
}

static void appCloseTextFile(CTextLineLoader* iHandle)
{
	delete iHandle;
}

static uint32_t appGetTextFileLineCount(CTextLineLoader* pTextFileLoader)
{
	return  pTextFileLoader->GetLineCount();
}

static uint32_t appGetTextFileLine(CTextLineLoader* pTextFileLoader)
{
	return  pTextFileLoader->GetLineCount();
}

static std::string appGetLocaleServiceName()
{
	return  ClientConfig::GetInstance()->GetLanguage();
}

static std::string appGetLocaleName()
{
	return  ClientConfig::GetInstance()->GetLanguage();
}

static std::string appGetLocalePath()
{
	return "locale/" + ClientConfig::GetInstance()->GetLanguage();
}

static bool appLoadLocaleData(std::string localePath)
{
	return  LoadLocaleData(localePath.c_str());
}

static unsigned int appGetDefaultCodePage()
{
	return  CP_UTF8;
}

static void appSetControlFP()
{
	_controlfp( _PC_24, _MCW_PC );
}

static void appSetSpecularSpeed(double fSpeed)
{
	g_specularSpd = fSpeed;
}


static std::string appGetSuperPath()
{
	#ifdef _DEBUG
	return ClientConfig::GetInstance()->GetSuperPath(); 
	#else
	return "PythonSystem/";
	#endif
}

static int32_t apptestGetAccumulationTime()
{
	return  g_iAccumulationTime;
}

static void apptestResetAccumulationTime()
{
	g_iAccumulationTime = 0;
}

static void apptestSetSpecularColor(float fr, float fg, float fb)
{
	g_fSpecularColor = D3DXCOLOR(fr, fg, fb, 1.0f);
}

static void appSetVisibleNotice(bool iFlag)
{
	bVisibleNotice = iFlag;
}

static bool appIsVisibleNotice()
{
	return  bVisibleNotice;
}

static void appSetGuildMarkPath(std::string path)
{
	std::string newPath;
	auto pos = path.find(".tga");


	if (pos != std::string::npos)
    {
		newPath = path.substr(0, pos);
    }
    else
		newPath = path;
	
	CGuildMarkManager::GetInstance()->SetMarkPathPrefix(newPath.c_str());
}

PYBIND11_EMBEDDED_MODULE(app, m)
{
	m.def("SetTextTailLivingTime",	appSetTextTailLivingTime);
	m.def("SetCameraMaxDistance",	appSetCameraMaxDistance);
	m.def("SetMinFog",	appSetMinFog);
	m.def("SetFrameSkip",	appSetFrameSkip);
	m.def("GetImageInfo",	appGetImageInfo);
	m.def("GetInfo",	appGetInfo);
	m.def("UpdateGame",	appUpdateGame);
	m.def("RenderGame",	appRenderGame);
	m.def("Loop",	appLoop);
	m.def("Create",	appCreate);
	m.def("Process",	appProcess);
	m.def("Exit",	appExit);
	m.def("Abort",	appAbort);
	m.def("SetMouseHandler",	appSetMouseHandler);
	m.def("IsExistFile",	appIsExistFile);
	m.def("GetFileList",	appGetFileList);
	m.def("SetCamera",	appSetCamera);
	m.def("GetCamera",	appGetCamera);
	m.def("GetCameraPitch",	appGetCameraPitch);
	m.def("GetCameraRotation",	appGetCameraRotation);
	m.def("GetTime",	appGetTime);
	m.def("GetGlobalTime",	appGetGlobalTime);
	m.def("GetGlobalTimeStamp",	appGetGlobalTimeStamp);
	m.def("GetUpdateFPS",	appGetUpdateFPS);
	m.def("GetRenderFPS",	appGetRenderFPS);
	m.def("RotateCamera",	appRotateCamera);
	m.def("PitchCamera",	appPitchCamera);
	m.def("ZoomCamera",	appZoomCamera);
	m.def("MovieRotateCamera",	appMovieRotateCamera);
	m.def("MoviePitchCamera",	appMoviePitchCamera);
	m.def("MovieZoomCamera",	appMovieZoomCamera);
	m.def("MovieResetCamera",	appMovieResetCamera);
	m.def("GetAvailableTextureMemory",	appGetAvailableTextureMemory);
	m.def("GetRenderTime",	appGetRenderTime);
	m.def("GetUpdateTime",	appGetUpdateTime);
	m.def("GetLoad",	appGetLoad);
	m.def("GetFaceSpeed",	appGetFaceSpeed);
	m.def("GetFaceCount",	appGetFaceCount);
	m.def("SetFPS",	appSetFPS);
	m.def("SetGlobalCenterPosition",	appSetGlobalCenterPosition);
	m.def("SetCenterPosition",	appSetCenterPosition);
	m.def("GetCursorPosition",	appGetCursorPosition);
	m.def("GetRandom",	appGetRandom);
	m.def("IsPressed",	appIsPressed);
	m.def("SetCursor",	appSetCursor);
	m.def("GetCursor",	appGetCursor);
	m.def("ShowCursor",	appShowCursor);
	m.def("HideCursor",	appHideCursor);
	m.def("IsShowCursor",	appIsShowCursor);
	m.def("IsLiarCursorOn",	appIsLiarCursorOn);
	m.def("SetSoftwareCursor",	appSetSoftwareCursor);
	m.def("SetHardwareCursor",	appSetHardwareCursor);
	m.def("SetConnectData",	appSetConnectData);
	m.def("GetConnectData",	appGetConnectData);
	m.def("GetRotatingDirection",	appGetRotatingDirection);
	m.def("GetDegreeDifference",	appGetDegreeDifference);
	m.def("Sleep",	appSleep);
	m.def("SetDefaultFontName",	appSetDefaultFontName);
	m.def("SetGuildSymbolPath",	appSetGuildSymbolPath);
	m.def("SetCameraSpeed",	appSetCameraSpeed);
	m.def("SaveCameraSetting",	appSaveCameraSetting);
	m.def("LoadCameraSetting",	appLoadCameraSetting);
	m.def("SetDefaultCamera",	appSetDefaultCamera);
	m.def("SetCameraSetting",	appSetCameraSetting);
	m.def("SetSightRange",	appSetSightRange);
	m.def("OpenTextFile",	appOpenTextFile);
	m.def("CloseTextFile",	appCloseTextFile);
	m.def("GetTextFileLineCount",	appGetTextFileLineCount);
	m.def("GetTextFileLine",	appGetTextFileLine);
	m.def("GetLocaleServiceName",	appGetLocaleServiceName);
	m.def("GetLocaleName",	appGetLocaleName);
	m.def("GetLocalePath",	appGetLocalePath);
	m.def("LoadLocaleData",	appLoadLocaleData);
	m.def("GetDefaultCodePage",	appGetDefaultCodePage);
	m.def("SetControlFP",	appSetControlFP);
	m.def("SetSpecularSpeed",	appSetSpecularSpeed);
	m.def("testGetAccumulationTime",	apptestGetAccumulationTime);
	m.def("testResetAccumulationTime",	apptestResetAccumulationTime);
	m.def("testSetSpecularColor",	apptestSetSpecularColor);
	m.def("SetVisibleNotice",	appSetVisibleNotice);
	m.def("IsVisibleNotice",	appIsVisibleNotice);
	m.def("SetGuildMarkPath",	appSetGuildMarkPath);
	m.def("GetSuperPath", appGetSuperPath);

#ifdef _DEBUG
	m.def("CrashpadTest", []() { int* o = nullptr; *o = 0xFF; });
#endif

	m.attr("INFO_ITEM") =	int32_t(CPythonApplication::INFO_ITEM);
	m.attr("INFO_ACTOR") = int32_t(CPythonApplication::INFO_ACTOR);
	m.attr("INFO_EFFECT") = int32_t(CPythonApplication::INFO_EFFECT);
	m.attr("INFO_TEXTTAIL") = int32_t(CPythonApplication::INFO_TEXTTAIL);
	m.attr("DEGREE_DIRECTION_SAME") = int32_t(DEGREE_DIRECTION_SAME);
	m.attr("DEGREE_DIRECTION_RIGHT") = int32_t(DEGREE_DIRECTION_RIGHT);
	m.attr("DEGREE_DIRECTION_LEFT") = int32_t(DEGREE_DIRECTION_LEFT);
	m.attr("VK_BACK") = int32_t(kVirtualKeyBack);
	m.attr("VK_TAB") = int32_t(kVirtualKeyTab);
	m.attr("VK_CLEAR") = int32_t(kVirtualKeyClear);
	m.attr("VK_RETURN") = int32_t(kVirtualKeyReturn);
	m.attr("VK_SHIFT") = int32_t(kVirtualKeyShift);
	m.attr("VK_CONTROL") = int32_t(kVirtualKeyControl);
	m.attr("VK_MENU") = int32_t(kVirtualKeyMenu);
	m.attr("VK_PAUSE") = int32_t(kVirtualKeyPause);
	m.attr("VK_CAPITAL") = int32_t(kVirtualKeyCapital);
	m.attr("VK_ESCAPE") = int32_t(kVirtualKeyEscape);
	m.attr("VK_SPACE") = int32_t(kVirtualKeySpace);
	m.attr("VK_PRIOR") = int32_t(kVirtualKeyPrior);
	m.attr("VK_NEXT") = int32_t(kVirtualKeyNext);
	m.attr("VK_END") = int32_t(kVirtualKeyEnd);
	m.attr("VK_HOME") = int32_t(kVirtualKeyHome);
	m.attr("VK_LEFT") = int32_t(kVirtualKeyLeft);
	m.attr("VK_UP") = int32_t(kVirtualKeyUp);
	m.attr("VK_RIGHT") = int32_t(kVirtualKeyRight);
	m.attr("VK_DOWN") = int32_t(kVirtualKeyDown);
	m.attr("VK_SELECT") = int32_t(kVirtualKeySelect);
	m.attr("VK_PRINT") = int32_t(kVirtualKeyPrint);
	m.attr("VK_EXECUTE") = int32_t(kVirtualKeyExecute);
	m.attr("VK_SNAPSHOT") = int32_t(kVirtualKeySnapshot);
	m.attr("VK_INSERT") = int32_t(kVirtualKeyInsert);
	m.attr("VK_DELETE") = int32_t(kVirtualKeyDelete);
	m.attr("VK_0") = int32_t(kVirtualKey0);
	m.attr("VK_1") = int32_t(kVirtualKey1);
	m.attr("VK_2") = int32_t(kVirtualKey2);
	m.attr("VK_3") = int32_t(kVirtualKey3);
	m.attr("VK_4") = int32_t(kVirtualKey4);
	m.attr("VK_5") = int32_t(kVirtualKey5);
	m.attr("VK_6") = int32_t(kVirtualKey6);
	m.attr("VK_7") = int32_t(kVirtualKey7);
	m.attr("VK_8") = int32_t(kVirtualKey8);
	m.attr("VK_9") = int32_t(kVirtualKey9);
	m.attr("VK_A") = int32_t(kVirtualKeyA);
	m.attr("VK_B") = int32_t(kVirtualKeyB);
	m.attr("VK_C") = int32_t(kVirtualKeyC);
	m.attr("VK_D") = int32_t(kVirtualKeyD);
	m.attr("VK_E") = int32_t(kVirtualKeyE);
	m.attr("VK_F") = int32_t(kVirtualKeyF);
	m.attr("VK_G") = int32_t(kVirtualKeyG);
	m.attr("VK_H") = int32_t(kVirtualKeyH);
	m.attr("VK_I") = int32_t(kVirtualKeyI);
	m.attr("VK_J") = int32_t(kVirtualKeyJ);
	m.attr("VK_K") = int32_t(kVirtualKeyK);
	m.attr("VK_L") = int32_t(kVirtualKeyL);
	m.attr("VK_M") = int32_t(kVirtualKeyM);
	m.attr("VK_N") = int32_t(kVirtualKeyN);
	m.attr("VK_O") = int32_t(kVirtualKeyO);
	m.attr("VK_P") = int32_t(kVirtualKeyP);
	m.attr("VK_Q") = int32_t(kVirtualKeyQ);
	m.attr("VK_R") = int32_t(kVirtualKeyR);
	m.attr("VK_S") = int32_t(kVirtualKeyS);
	m.attr("VK_T") = int32_t(kVirtualKeyT);
	m.attr("VK_U") = int32_t(kVirtualKeyU);
	m.attr("VK_V") = int32_t(kVirtualKeyV);
	m.attr("VK_W") = int32_t(kVirtualKeyW);
	m.attr("VK_X") = int32_t(kVirtualKeyX);
	m.attr("VK_Y") = int32_t(kVirtualKeyY);
	m.attr("VK_Z") = int32_t(kVirtualKeyZ);
	m.attr("VK_NUMPAD0") = int32_t(kVirtualKeyNumpad0);
	m.attr("VK_NUMPAD1") = int32_t(kVirtualKeyNumpad1);
	m.attr("VK_NUMPAD2") = int32_t(kVirtualKeyNumpad2);
	m.attr("VK_NUMPAD3") = int32_t(kVirtualKeyNumpad3);
	m.attr("VK_NUMPAD4") = int32_t(kVirtualKeyNumpad4);
	m.attr("VK_NUMPAD5") = int32_t(kVirtualKeyNumpad5);
	m.attr("VK_NUMPAD6") = int32_t(kVirtualKeyNumpad6);
	m.attr("VK_NUMPAD7") = int32_t(kVirtualKeyNumpad7);
	m.attr("VK_NUMPAD8") = int32_t(kVirtualKeyNumpad8);
	m.attr("VK_NUMPAD9") = int32_t(kVirtualKeyNumpad9);
	m.attr("VK_MULTIPLY") = int32_t(kVirtualKeyMultiply);
	m.attr("VK_ADD") = int32_t(kVirtualKeyAdd);
	m.attr("VK_SEPARATOR") = int32_t(kVirtualKeySeparator);
	m.attr("VK_SUBTRACT") = int32_t(kVirtualKeySubtract);
	m.attr("VK_F1") = int32_t(kVirtualKeyF1);
	m.attr("VK_F2") = int32_t(kVirtualKeyF2);
	m.attr("VK_F3") = int32_t(kVirtualKeyF3);
	m.attr("VK_F4") = int32_t(kVirtualKeyF4);
	m.attr("VK_F5") = int32_t(kVirtualKeyF5);
	m.attr("VK_F6") = int32_t(kVirtualKeyF6);
	m.attr("VK_F7") = int32_t(kVirtualKeyF7);
	m.attr("VK_F8") = int32_t(kVirtualKeyF8);
	m.attr("VK_F9") = int32_t(kVirtualKeyF9);
	m.attr("VK_F10") = int32_t(kVirtualKeyF10);
	m.attr("VK_F11") = int32_t(kVirtualKeyF11);
	m.attr("VK_F12") = int32_t(kVirtualKeyF12);
	m.attr("VK_COMMA") = int32_t(kVirtualKeyComma);
	m.attr("VK_OEM3") = int32_t(kVirtualKeyOEM3);
	m.attr("VK_OEM5") = int32_t(kVirtualKeyOEM5);
	m.attr("NORMAL") = int32_t(CPythonApplication::CURSOR_SHAPE_NORMAL);
	m.attr("ATTACK") = int32_t(CPythonApplication::CURSOR_SHAPE_ATTACK);
	m.attr("TARGET") = int32_t(CPythonApplication::CURSOR_SHAPE_TARGET);
	m.attr("TALK") = int32_t(CPythonApplication::CURSOR_SHAPE_TALK);
	m.attr("CANT_GO") = int32_t(CPythonApplication::CURSOR_SHAPE_CANT_GO);
	m.attr("PICK") = int32_t(CPythonApplication::CURSOR_SHAPE_PICK);
	m.attr("DOOR") = int32_t(CPythonApplication::CURSOR_SHAPE_DOOR);
	m.attr("CHAIR") = int32_t(CPythonApplication::CURSOR_SHAPE_CHAIR);
	m.attr("MAGIC") = int32_t(CPythonApplication::CURSOR_SHAPE_MAGIC);
	m.attr("BUY") = int32_t(CPythonApplication::CURSOR_SHAPE_BUY);
	m.attr("SELL") = int32_t(CPythonApplication::CURSOR_SHAPE_SELL);
	m.attr("CAMERA_ROTATE") = int32_t(CPythonApplication::CURSOR_SHAPE_CAMERA_ROTATE);
	m.attr("HSIZE") = int32_t(CPythonApplication::CURSOR_SHAPE_HSIZE);
	m.attr("VSIZE") = int32_t(CPythonApplication::CURSOR_SHAPE_VSIZE);
	m.attr("HVSIZE") = int32_t(CPythonApplication::CURSOR_SHAPE_HVSIZE);
	m.attr("CAMERA_TO_POSITIVE") = int32_t(CPythonApplication::CAMERA_TO_POSITIVE);
	m.attr("CAMERA_TO_NEGATIVE") = int32_t(CPythonApplication::CAMERA_TO_NEGITIVE);
	m.attr("CAMERA_STOP") = int32_t(CPythonApplication::CAMERA_STOP);

#ifdef ENABLE_DRAGON_SOUL_SYSTEM
	m.attr("ENABLE_DRAGON_SOUL_SYSTEM") = 1;
#else
	m.attr("ENABLE_DRAGON_SOUL_SYSTEM") = 0;
#endif
	
#ifdef ENABLE_NEW_EQUIPMENT_SYSTEM
	m.attr("ENABLE_NEW_EQUIPMENT_SYSTEM") = 1;
#else
	m.attr("ENABLE_NEW_EQUIPMENT_SYSTEM") =	0;
#endif
}
