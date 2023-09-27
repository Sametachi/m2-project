#include "StdAfx.h"
#include <EterLib/Camera.h>
#include <EterLib/AttributeInstance.h>
#include <EterLib/UTF8/utf8/core.h>
#include <EterLib/UTF8/utf8/checked.h>
#include <EterGrnLib/Material.h>
#include <GameLib/AreaTerrain.h>

#include "resource.h"
#include "PythonApplication.h"
#include "PythonCharacterManager.h"
#include "AbstractResources.h"

#include "../../ThirdParty/ImGUI/imgui.h"
#include "../../ThirdParty/ImGUI/backends/imgui_impl_dx9.h"
#include "../../ThirdParty/ImGUI/backends/imgui_impl_win32.h"
#include <shellapi.h>


float MIN_FOG = 2400.0f;
double g_specularSpd=0.007f;

float c_fDefaultCameraRotateSpeed = 1.5f;
float c_fDefaultCameraPitchSpeed = 1.5f;
float c_fDefaultCameraZoomSpeed = 0.05f;

CPythonApplication::CPythonApplication()
	: m_bCursorVisible(true), m_bLiarCursorOn(false), m_iCursorMode(CURSOR_MODE_HARDWARE), m_hCurrentCursor(0)
	, m_iCursorNum(CURSOR_SHAPE_NORMAL), m_iContinuousCursorNum(CURSOR_SHAPE_NORMAL)
	, m_fCameraRotateSpeed(c_fDefaultCameraRotateSpeed), m_fCameraPitchSpeed(c_fDefaultCameraPitchSpeed)
	, m_fCameraZoomSpeed(c_fDefaultCameraZoomSpeed), m_iCameraMode(CAMERA_MODE_NORMAL)
	, m_poMouseHandler(nullptr), m_isWindowFullScreenEnable(false), m_v3CenterPosition(D3DXVECTOR3(0.0f, 0.0f, 0.0f))
	, m_isWindowed(false), m_isFrameSkipDisable(false), m_dwLButtonDownTime(0), m_dwLButtonUpTime(0)
	, m_fAveRenderTime(0.0f), m_fGlobalTime(0.0f), m_fGlobalElapsedTime(0.0f)
	, m_dwUpdateFPS(0), m_dwRenderFPS(0), m_dwFaceCount(0)
	, m_tServerTime(0), m_tLocalStartTime(0), m_iPort(0)
	, m_isActivateWnd(false), m_isMinimizedWnd(true)
	, m_fRotationSpeed(0.0f), m_fPitchSpeed(0.0f), m_fZoomSpeed(0.0f)
	, m_fFaceSpd(0.0f), m_fBlendCameraStartTime(0.0f), m_fBlendCameraBlendTime(0.0f)
	, m_dwFaceAccCount(0), m_dwFaceAccTime(0), m_dwFaceSpdSum(0), m_dwFaceSpdCount(0)
	, m_iForceSightRange(-1), m_dwStickyKeysFlag(0), m_dwLoad(0)
	, m_dwBufSleepSkipTime(0), m_dwCurRenderTime(0), m_dwCurUpdateTime(0)
	, m_iFPS(60)
{
	ClientConfig::GetInstance()->LoadConfig();
	Engine::SetKeyboardInput(&m_keyboard);
	CTimer::GetInstance()->UseCustomTime();

	m_dwWidth = m_pySystem.GetWidth();
	m_dwHeight = m_pySystem.GetHeight();

	m_dwStartLocalTime = ELTimer_GetMSec();

	m_FlyingManager.SetMapManagerPtr(&m_pyBackground);
	CCameraManager::GetInstance()->AddCamera(EVENT_CAMERA_NUMBER);
}

CPythonApplication::~CPythonApplication() { Destroy(); }
void CPythonApplication::GetMousePosition(POINT* ppt) { CMSApplication::GetMousePosition(ppt); }
void CPythonApplication::SetMinFog(float fMinFog) { MIN_FOG = fMinFog; }

void CPythonApplication::SetFrameSkip(bool isEnable)
{
	if (isEnable)
		m_isFrameSkipDisable=false;
	else
		m_isFrameSkipDisable=true;
}

void CPythonApplication::NotifyHack(const char* c_szFormat, ...)
{
	char szBuf[1024];

	va_list args;
	va_start(args, c_szFormat);	
	_vsnprintf(szBuf, sizeof(szBuf), c_szFormat, args);
	va_end(args);
	m_pyNetworkStream.NotifyHack(szBuf);
}

void CPythonApplication::GetInfo(UINT eInfo, std::string* pstInfo)
{
	switch (eInfo)
	{
	case INFO_ACTOR:
		//m_kChrMgr.GetInfo(pstInfo);
		break;
	case INFO_EFFECT:
		m_kEftMgr.GetInfo(pstInfo);			
		break;
	case INFO_ITEM:
		m_pyItem.GetInfo(pstInfo);
		break;
	case INFO_TEXTTAIL:
		//m_pyTextTail.GetInfo(pstInfo);
		break;
	}
}

void CPythonApplication::Abort()
{
	FatalLog("The client Crashed..");

	if (PyErr_Occurred())
		PyErr_Print();

	PyThreadState* ts = PyThreadState_Get();
	PyFrameObject* frame = ts->frame;
	while (frame != 0)
	{
		auto fn_obj = py::handle(frame->f_code->co_filename);
		auto fn_name = py::handle(frame->f_code->co_name);

		auto filename = fn_obj.cast<std::string>();
		auto name = fn_name.cast<std::string>();
		SysLog("filename={0}, name={1}", filename, name);
		frame = frame->f_back;
	}

	PostQuitMessage(0);
}

void CPythonApplication::Exit()
{
	ConsoleLog("Process exit");
	PostQuitMessage(0);
}

void CPythonApplication::RenderGame()
{	
	float fAspect = m_kWndMgr.GetAspect();
	float fFarClip = m_pyBackground.GetFarClip();

	m_pyGraphic.SetPerspective(30.0f, fAspect, 100.0, fFarClip);

	CCullingManager::GetInstance()->Process();

	m_kChrMgr.Deform();

	CMapOutdoor* map = nullptr;
	if (m_pyBackground.IsMapReady())
		map = &m_pyBackground.GetMapOutdoorRef();

	if (map)
		m_pyBackground.RenderCharacterShadowToTexture();

	m_pyGraphic.SetGameRenderState();
	m_pyGraphic.PushState();

	{
		auto [lx, ly] = m_kWndMgr.GetMousePosition();
		m_pyGraphic.SetCursorPosition(lx, ly);
	}

	if (map)
	{
		map->RenderSky();
		map->RenderBeforeLensFlare();
		map->RenderCloud();
		m_pyBackground.BeginEnvironment();
		m_pyBackground.Render();
		m_pyBackground.SetCharacterDirLight();
	}

	m_kChrMgr.Render();

	if (map)
	{
		m_pyBackground.SetBackgroundDirLight();
		map->RenderWater();
		m_pyBackground.RenderSnow();
		map->RenderEffect();
		m_pyBackground.EndEnvironment();
	}

	m_kEftMgr.Render();
	m_pyItem.Render();
	m_FlyingManager.Render();

	if (map)
	{
		m_pyBackground.BeginEnvironment();
		map->RenderPCBlocker();
		m_pyBackground.EndEnvironment();

		map->RenderAfterLensFlare();
		map->RenderScreenFiltering();
	}
}

void CPythonApplication::UpdateGame()
{
	POINT ptMouse;
	GetMousePosition(&ptMouse);
	{
		CScreen s;
		float fAspect = CWindowManager::GetInstance()->GetAspect();
		float fFarClip = CPythonBackground::GetInstance()->GetFarClip();

		s.SetPerspective(30.0f, fAspect, 100.0f, fFarClip);
		s.BuildViewFrustum();
	}

	TPixelPosition kPPosMainActor;
	m_pyPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
	/*(ipx): This is a very important order for correctly setting character shadows into the map!!*/
	m_kChrMgr.Update();
	m_pyBackground.Update(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
	m_GameEventManager.SetCenterPosition(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
	m_GameEventManager.Update();
	m_kEftMgr.Update();
	m_FlyingManager.Update();
	m_pyItem.Update(ptMouse);
	m_pyPlayer.Update();

	SetCenterPosition(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
}

void CPythonApplication::SkipRenderBuffering(uint32_t dwSleepMSec)
{
	m_dwBufSleepSkipTime=ELTimer_GetMSec()+dwSleepMSec;
}

void CPythonApplication::RenderImgUiWindows()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (m_pySystem.IsShowFPS())
	{
#ifdef NDEBUG
		//	ImGui::SetNextWindowPos(ImVec2(0, -5), ImGuiCond_Always);
		//	ImGui::Begin("FPS", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
		//	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		//	ImGui::End();
		ImGui::Begin("Release Debug Info");
		ImGui::Text("Instances Alive = %d", CPythonCharacterManager::GetInstance()->GetAliveInstanceCount());
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
#else
		ImGui::Begin("Debug Info");
		ImGui::Text("Instances Alive = %d", CPythonCharacterManager::GetInstance()->GetAliveInstanceCount());
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		if (ImGui::Button("Font GigaChad -> 55"))
		{
			CPythonTextTail::GetInstance()->CPythonTextTail::ChangeFontType("BeaufortforLOL-Regular:55");
		}

		if (ImGui::Button("Font UltraSmall -> 5"))
		{
			CPythonTextTail::GetInstance()->CPythonTextTail::ChangeFontType("BeaufortforLOL-Regular:5");
		}

		if (ImGui::Button("DarkMode on"))
		{
			if (!m_pySystem.IsDarkMode())
			{
				m_pySystem.SetDarkMode(true);
				m_pySystem.SaveConfig();
			}
		}

		if (ImGui::Button("DarkMode off"))
		{
			if (m_pySystem.IsDarkMode())
			{
				m_pySystem.SetDarkMode(false);
				m_pySystem.SaveConfig();
			}
		}

		if (ImGui::Button("Mouse Size test"))
		{
			ClientConfig::GetInstance()->SetMouseSize(16);
		}

		ImGui::End();
#endif
	}
	ImGui::EndFrame();
	// Setup Sound right after the IMGui process!
	m_SoundManager.SetMusicVolume(m_pySystem.GetMusicVolume());
	m_SoundManager.SetSoundVolume(m_pySystem.GetSoundVolume());
}

bool CPythonApplication::Process()
{
	ELTimer_SetFrameMSec();

	// 	m_Profiler.Clear();
	uint32_t dwStart = ELTimer_GetMSec();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	static uint32_t	s_dwUpdateFrameCount = 0;
	static uint32_t	s_dwRenderFrameCount = 0;
	static uint32_t	s_dwFaceCount = 0;
	static UINT		s_uiLoad = 0;
	static uint32_t	s_dwCheckTime = ELTimer_GetMSec();

	if (ELTimer_GetMSec() - s_dwCheckTime > 1000)
	{
		m_dwUpdateFPS		= s_dwUpdateFrameCount;
		m_dwRenderFPS		= s_dwRenderFrameCount;
		m_dwLoad			= s_uiLoad;

		m_dwFaceCount		= s_dwFaceCount / std::max(1U, s_dwRenderFrameCount);

		s_dwCheckTime		= ELTimer_GetMSec();

		s_uiLoad = s_dwFaceCount = s_dwUpdateFrameCount = s_dwRenderFrameCount = 0;
	}

	// Update Time
	static BOOL s_bFrameSkip = false;
	static UINT s_uiNextFrameTime = ELTimer_GetMSec();

	auto rkTimer=CTimer::GetInstance();
	rkTimer->Advance();

	m_fGlobalTime = rkTimer->GetCurrentSecond();
	m_fGlobalElapsedTime = rkTimer->GetElapsedSecond();

	UINT uiFrameTime = rkTimer->GetElapsedMilliecond();
	s_uiNextFrameTime += uiFrameTime;	//17 - 1초당 60fps기준.

	uint32_t updatestart = ELTimer_GetMSec();
	// Network I/O	
	m_pyNetworkStream.Process();	
	//m_pyNetworkDatagram.Process();

	m_kGuildMarkUploader.Process();

	m_kGuildMarkDownloader.Process();
	m_kAccountConnector.Process();
	//////////////////////
	// Input Process

	// Mouse
	POINT Point;
	if (GetCursorPos(&Point))
	{
		ScreenToClient(m_hWnd, &Point);
		OnMouseMove(Point.x, Point.y);		
	}
	//////////////////////
	//!@# Alt+Tab 중 SetTransfor 에서 튕김 현상 해결을 위해 - [levites]
	//if (m_isActivateWnd)
	__UpdateCamera();
	// Update Game Playing
	CResourceManager::GetInstance()->Update();
	OnCameraUpdate();
	OnMouseUpdate();
	OnUIUpdate();

	//Update하는데 걸린시간.delta값
	m_dwCurUpdateTime = ELTimer_GetMSec() - updatestart;

	uint32_t dwCurrentTime = ELTimer_GetMSec();
	BOOL  bCurrentLateUpdate = FALSE;

	s_bFrameSkip = false;

	if (dwCurrentTime > s_uiNextFrameTime)
	{
		int32_t dt = dwCurrentTime - s_uiNextFrameTime;
		int32_t nAdjustTime = ((float)dt / (float)uiFrameTime) * uiFrameTime; 

		if ( dt >= 500 )
		{
			s_uiNextFrameTime += nAdjustTime; 
			ConsoleLog("FrameSkip 보정 {}",nAdjustTime);
			CTimer::GetInstance()->Adjust(nAdjustTime);
		}

		s_bFrameSkip = true;
		bCurrentLateUpdate = TRUE;
	}

	//s_bFrameSkip = false;

	//if (dwCurrentTime > s_uiNextFrameTime)
	//{
	//	int32_t dt = dwCurrentTime - s_uiNextFrameTime;

	//	//너무 늦었을 경우 따라잡는다.
	//	//그리고 m_dwCurUpdateTime는 delta인데 delta랑 absolute time이랑 비교하면 어쩌자는겨?
	//	//if (dt >= 500 || m_dwCurUpdateTime > s_uiNextFrameTime)

	//	//기존코드대로 하면 0.5초 이하 차이난 상태로 update가 지속되면 계속 rendering frame skip발생
	//	if (dt >= 500 || m_dwCurUpdateTime > s_uiNextFrameTime)
	//	{
	//		s_uiNextFrameTime += dt / uiFrameTime * uiFrameTime; 
	//		printf("FrameSkip 보정 %d\n", dt / uiFrameTime * uiFrameTime);
	//		CTimer::GetInstance()->Adjust((dt / uiFrameTime) * uiFrameTime);
	//		s_bFrameSkip = true;
	//	}
	//}

	if (m_isFrameSkipDisable)
		s_bFrameSkip = false;

	/*
	static bool s_isPrevFrameSkip=false;
	static uint32_t s_dwFrameSkipCount=0;
	static uint32_t s_dwFrameSkipEndTime=0;

	static uint32_t ERROR_FRAME_SKIP_COUNT = 60*5;
	static uint32_t ERROR_FRAME_SKIP_TIME = ERROR_FRAME_SKIP_COUNT*18;

	//static uint32_t MAX_FRAME_SKIP=0;

	if (IsActive())
	{
	uint32_t dwFrameSkipCurTime=ELTimer_GetMSec();

	if (s_bFrameSkip)
	{
	// 이전 프레임도 스킵이라면..
	if (s_isPrevFrameSkip)
	{
	if (s_dwFrameSkipEndTime==0)
	{
	s_dwFrameSkipCount=0; // 프레임 체크는 로딩 대비
	s_dwFrameSkipEndTime=dwFrameSkipCurTime+ERROR_FRAME_SKIP_TIME; // 시간 체크는 로딩후 프레임 스킵 체크

	//printf("FrameSkipCheck Start\n");
	}
	++s_dwFrameSkipCount;

	//if (MAX_FRAME_SKIP<s_dwFrameSkipCount)
	//	MAX_FRAME_SKIP=s_dwFrameSkipCount;

	//printf("u %d c %d/%d t %d\n", 
	//	dwUpdateTime9-dwUpdateTime1,
	//	s_dwFrameSkipCount, 
	//	MAX_FRAME_SKIP,
	//	s_dwFrameSkipEndTime);

	//#ifndef _DEBUG
	// 일정 시간동안 계속 프레임 스킵만 한다면...
	if (s_dwFrameSkipCount>ERROR_FRAME_SKIP_COUNT && s_dwFrameSkipEndTime<dwFrameSkipCurTime)
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipEndTime=0;
	s_dwFrameSkipCount=0;

	//m_pyNetworkStream.AbsoluteExitGame();

	/*
	TraceError("무한 프레임 스킵으로 접속을 종료합니다");

	{
	FILE* fp=fopen("errorlog.txt", "w");
	if (fp)
	{
	fprintf(fp, "FRAMESKIP\n");
	fprintf(fp, "Total %d\n",		dwUpdateTime9-dwUpdateTime1);
	fprintf(fp, "Timer %d\n",		dwUpdateTime2-dwUpdateTime1);
	fprintf(fp, "Network %d\n",		dwUpdateTime3-dwUpdateTime2);
	fprintf(fp, "Keyboard %d\n",	dwUpdateTime4-dwUpdateTime3);
	fprintf(fp, "Controll %d\n",	dwUpdateTime5-dwUpdateTime4);
	fprintf(fp, "Resource %d\n",	dwUpdateTime6-dwUpdateTime5);
	fprintf(fp, "Camera %d\n",		dwUpdateTime7-dwUpdateTime6);
	fprintf(fp, "Mouse %d\n",		dwUpdateTime8-dwUpdateTime7);
	fprintf(fp, "UI %d\n",			dwUpdateTime9-dwUpdateTime8);
	fclose(fp);

	WinExec("errorlog.exe", SW_SHOW);
	}
	}
	}
	}

	s_isPrevFrameSkip=true;
	}
	else
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipCount=0;
	s_dwFrameSkipEndTime=0;
	}
	}
	else
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipCount=0;
	s_dwFrameSkipEndTime=0;
	}
	*/
	if (!s_bFrameSkip)
	{
		//		static double pos=0.0f;
		//		CGrannyMaterial::TranslateSpecularMatrix(fabs(sin(pos)*0.005), fabs(cos(pos)*0.005), 0.0f);
		//		pos+=0.01f;

		CGrannyMaterial::TranslateSpecularMatrix(g_specularSpd, g_specularSpd, 0.0f);

		uint32_t dwRenderStartTime = ELTimer_GetMSec();		

		bool canRender = true;

		if (m_isMinimizedWnd)
		{
			canRender = false;
		}
		else
		{
			if (DEVICE_STATE_OK != CheckDeviceState())
				canRender = false;
		}

		if (canRender)
		{
			// RestoreLostDevice
			CCullingManager::GetInstance()->Update();
			if (m_pyGraphic.Begin())
			{

				m_pyGraphic.ClearDepthBuffer();

#ifdef _DEBUG
				m_pyGraphic.SetClearColor(0.3f, 0.3f, 0.3f);
				m_pyGraphic.Clear();
#endif

				/////////////////////
				// Interface
				m_pyGraphic.SetInterfaceRenderState();

				OnUIRender();
				OnMouseRender();
				/////////////////////

				RenderImgUiWindows();
				ImGui::Render();
				ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

				m_pyGraphic.End();

				//uint32_t t1 = ELTimer_GetMSec();
				m_pyGraphic.Show();
				//uint32_t t2 = ELTimer_GetMSec();

				uint32_t dwRenderEndTime = ELTimer_GetMSec();

				static uint32_t s_dwRenderCheckTime = dwRenderEndTime;
				static uint32_t s_dwRenderRangeTime = 0;
				static uint32_t s_dwRenderRangeFrame = 0;

				m_dwCurRenderTime = dwRenderEndTime - dwRenderStartTime;			
				s_dwRenderRangeTime += m_dwCurRenderTime;				
				++s_dwRenderRangeFrame;			

				if (dwRenderEndTime-s_dwRenderCheckTime>1000)
				{
					m_fAveRenderTime=float(double(s_dwRenderRangeTime)/double(s_dwRenderRangeFrame));

					s_dwRenderCheckTime=ELTimer_GetMSec();
					s_dwRenderRangeTime=0;
					s_dwRenderRangeFrame=0;
				}										

				uint32_t dwCurFaceCount=m_pyGraphic.GetFaceCount();
				m_pyGraphic.ResetFaceCount();
				s_dwFaceCount += dwCurFaceCount;

				if (dwCurFaceCount > 5000)
				{
					// 프레임 완충 처리
					if (dwRenderEndTime > m_dwBufSleepSkipTime)
					{	
						static float s_fBufRenderTime = 0.0f;

						float fCurRenderTime = m_dwCurRenderTime;

						if (fCurRenderTime > s_fBufRenderTime)
						{
							float fRatio = fMAX(0.5f, (fCurRenderTime - s_fBufRenderTime) / 30.0f);
							s_fBufRenderTime = (s_fBufRenderTime * (100.0f - fRatio) + (fCurRenderTime + 5) * fRatio) / 100.0f;
						}
						else
						{
							float fRatio = 0.5f;
							s_fBufRenderTime = (s_fBufRenderTime * (100.0f - fRatio) + fCurRenderTime * fRatio) / 100.0f;
						}

						// 한계치를 정한다
						if (s_fBufRenderTime > 100.0f)
							s_fBufRenderTime = 100.0f;

						uint32_t dwBufRenderTime = s_fBufRenderTime;

						if (m_isWindowed)
						{						
							if (dwBufRenderTime>58)
								dwBufRenderTime=64;
							else if (dwBufRenderTime>42)
								dwBufRenderTime=48;
							else if (dwBufRenderTime>26)
								dwBufRenderTime=32;
							else if (dwBufRenderTime>10)
								dwBufRenderTime=16;
							else
								dwBufRenderTime=8;
						}

						// 일정 프레임 속도에 맞추어주는쪽에 눈에 편하다
						// 아래에서 한번 하면 됬�?
						//if (m_dwCurRenderTime<dwBufRenderTime)
						//	Sleep(dwBufRenderTime-m_dwCurRenderTime);			

						m_fAveRenderTime=s_fBufRenderTime;
					}

					m_dwFaceAccCount += dwCurFaceCount;
					m_dwFaceAccTime += m_dwCurRenderTime;

					m_fFaceSpd=(m_dwFaceAccCount/m_dwFaceAccTime);

					// 거리 자동 조절
					if (-1 == m_iForceSightRange)
					{
						static float s_fAveRenderTime = 16.0f;
						float fRatio=0.3f;
						s_fAveRenderTime=(s_fAveRenderTime*(100.0f-fRatio)+ std::max(16.0f, float(m_dwCurRenderTime))*fRatio)/100.0f;


						float fFar=25600.0f;
						float fNear=MIN_FOG;
						double dbAvePow=double(1000.0f/s_fAveRenderTime);
						double dbMaxPow=60.0;
						float fDistance= std::max(float(fNear+(fFar-fNear)*(dbAvePow)/dbMaxPow), fNear);
						m_pyBackground.SetViewDistanceSet(0, fDistance);
					}
					// 거리 강제 설정시
					else
					{
						m_pyBackground.SetViewDistanceSet(0, float(m_iForceSightRange));
					}
				}
				else
				{
					// 10000 폴리곤 보다 적을때는 가장 멀리 보이게 한다
					m_pyBackground.SetViewDistanceSet(0, 25600.0f);
				}

				++s_dwRenderFrameCount;
			}
		}
	}

	int32_t rest = s_uiNextFrameTime - ELTimer_GetMSec();

	if (rest > 0 && !bCurrentLateUpdate )
	{
		s_uiLoad -= rest;	// 쉰 시간은 로드에서 뺀다..
		Sleep(rest);
	}	

	++s_dwUpdateFrameCount;

	s_uiLoad += ELTimer_GetMSec() - dwStart;
	//m_Profiler.ProfileByScreen();	
	return true;
}

void CPythonApplication::UpdateClientRect()
{
	RECT rcApp;
	GetClientRect(&rcApp);
	OnSizeChange(rcApp.right - rcApp.left, rcApp.bottom - rcApp.top);
}

void CPythonApplication::SetMouseHandler(pybind11::handle poMouseHandler)
{
	m_poMouseHandler = poMouseHandler;
}

int32_t CPythonApplication::CheckDeviceState()
{
	CGraphicDevice::EDeviceState e_deviceState = m_grpDevice.GetDeviceState();

	switch (e_deviceState)
	{
	case CGraphicDevice::DEVICESTATE_NULL:
		return DEVICE_STATE_FALSE;

	case CGraphicDevice::DEVICESTATE_BROKEN:
		return DEVICE_STATE_SKIP;

	case CGraphicDevice::DEVICESTATE_NEEDS_RESET:
		m_pyBackground.ReleaseCharacterShadowTexture();
		if (!m_grpDevice.Reset())
			return DEVICE_STATE_SKIP;

		m_pyBackground.CreateCharacterShadowTexture();
		break;
	case CGraphicDevice::DEVICESTATE_OK:
		break;
	default:;
	}

	return DEVICE_STATE_OK;
}

bool CPythonApplication::CreateDevice(int32_t width, int32_t height, int32_t Windowed, int32_t bit, int32_t frequency)
{
	m_grpDevice.InitBackBufferCount(Windowed ? 2 : 1);
	int32_t iRet = m_grpDevice.Create(GetWindowHandle(), width, height, Windowed ? true : false, bit, frequency);

	switch (iRet)
	{
	case CGraphicDevice::CREATE_OK:
		Engine::RegisterDevice(&m_grpDevice);
		return true;

	case CGraphicDevice::CREATE_REFRESHRATE:
		return true;

	case CGraphicDevice::CREATE_ENUM:
	case CGraphicDevice::CREATE_DETECT:
		SysLog("CreateDevice: Enum & Detect failed");
		return false;

	case CGraphicDevice::CREATE_NO_DIRECTX:
		SysLog("CreateDevice: DirectX 9 or greater required to run game");
		return false;

	case CGraphicDevice::CREATE_DEVICE:
		SysLog("CreateDevice: GraphicDevice create failed");
		return false;

	case CGraphicDevice::CREATE_FORMAT:
		SysLog("CreateDevice: Change the screen format");
		return false;

	case CGraphicDevice::CREATE_GET_DEVICE_CAPS:
		PyErr_SetString(PyExc_RuntimeError, "GetDevCaps failed");
		SysLog("CreateDevice: GetDevCaps failed");
		return false;

	case CGraphicDevice::CREATE_GET_DEVICE_CAPS2:
		PyErr_SetString(PyExc_RuntimeError, "GetDevCaps2 failed");
		SysLog("CreateDevice: GetDevCaps2 failed");
		return false;

	default:
		if (iRet & CGraphicDevice::CREATE_OK)
		{
			Engine::RegisterDevice(&m_grpDevice);
			return true;
		}
		SysLog("CreateDevice: Unknown Error!");
		return false;
	}
}

void CPythonApplication::Loop()
{
	MSG msg = {};
	PeekMessageW(&msg, nullptr, 0U, 0U, PM_NOREMOVE);

	while (WM_QUIT != msg.message)
	{
		bool messageReceived = (PeekMessageW(&msg, nullptr, 0U, 0U, PM_REMOVE) != 0);

		if (messageReceived)
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			Process();
		}
	}
}

// SUPPORT_NEW_KOREA_SERVER
bool LoadLocaleData(const char* localePath)
{
	auto	rkNPCMgr	= CPythonNonPlayer::GetInstance();
	auto		rkItemMgr	= CItemManager::GetInstance();
	auto		rkSkillMgr	= CPythonSkill::GetInstance();
	auto rkNetStream = CPythonNetworkStream::GetInstance();

	char szItemList[256];
	char szItemProto[256];
	char szItemDesc[256];	
	char szMobProto[256];
	char szMobDesc[256];
	char szSkillDescFileName[256];
	char szSkillTableFileName[256];
	char szInsultList[256];
	snprintf(szItemList,	sizeof(szItemList),		"%s/item_list.txt",		localePath);		
	snprintf(szItemProto,	sizeof(szItemProto),	"%s/item_proto.txt",	localePath);
	snprintf(szItemDesc,	sizeof(szItemDesc),		"%s/itemdesc.txt",		localePath);	
	snprintf(szMobProto,	sizeof(szMobProto),		"%s/mob_proto.txt",			localePath);	
	snprintf(szMobDesc,		sizeof(szMobDesc),		"%s/mobdesc.txt",		localePath);
	snprintf(szSkillDescFileName, sizeof(szSkillDescFileName),	"%s/SkillDesc.txt", localePath);
	snprintf(szSkillTableFileName, sizeof(szSkillTableFileName),	"%s/SkillTable.txt", localePath);	
	snprintf(szInsultList,	sizeof(szInsultList),	"%s/insult.txt", localePath);

	rkNPCMgr->Destroy();
	rkItemMgr->Destroy();
	rkSkillMgr->Destroy();

	if (!rkItemMgr->LoadItemList(szItemList))
	{
		SysLog("LoadLocaleData - LoadItemList({0}) Error", szItemList);
	}	

	if (!rkItemMgr->LoadItemTable(szItemProto))
	{
		SysLog("LoadLocaleData - LoadItemProto({0}) Error", szItemProto);
		return false;
	}

	if (!rkItemMgr->LoadItemDesc(szItemDesc))
	{
		SysLog("LoadLocaleData - LoadItemDesc({0}) Error", szItemDesc);
	}

	if (!rkNPCMgr->LoadMobProto(szMobProto))
	{
		SysLog("LoadLocaleData - LoadMobProto({0}) Error", szMobProto);
		return false;
	}

	if (!rkNPCMgr->LoadMobDesc(szMobDesc))
	{
		SysLog("LoadLocaleData - LoadMobDesc({0}) Error", szMobDesc);
	}

	if (!rkSkillMgr->RegisterSkillDesc(szSkillDescFileName))
	{
		SysLog("LoadLocaleData - RegisterSkillDesc({0}) Error", szMobProto);
		return false;
	}

	if (!rkSkillMgr->RegisterSkillTable(szSkillTableFileName))
	{
		SysLog("LoadLocaleData - RegisterSkillTable({0}) Error", szMobProto);
		return false;
	}

	if (!rkNetStream->LoadInsultList(szInsultList))
	{
		SysLog("CPythonApplication - CPythonNetworkStream::LoadInsultList({0})", szInsultList);
	}

	return true;
}
// END_OF_SUPPORT_NEW_KOREA_SERVER

void CPythonApplication::AdjustClientPosition()
{
	APPBARDATA appBarData{};
	appBarData.cbSize = sizeof(appBarData);
	RECT rWorkArea{};
	SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&rWorkArea, 0);
	uint32_t iYScreenSize = GetSystemMetrics(SM_CYSCREEN);
	uint32_t iXScreenSize = GetSystemMetrics(SM_CXSCREEN);
	uint32_t iTaskBarHeight = iYScreenSize - (rWorkArea.bottom - rWorkArea.top);
	uint32_t iTaskBarWidth = iXScreenSize - (rWorkArea.right);

	if (m_pySystem.GetHeightRes() >= iYScreenSize || m_pySystem.GetWidthRes() >= iXScreenSize)
	{
		SetPosition(-8, 0);
	}
	else
		SetCenterPositionz();
}

unsigned __GetWindowMode(uint8_t WindowMode)
{
	if (WindowMode == ClientConfig::WINDOWED)
	{
		return WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
	}
	else if (WindowMode == ClientConfig::BOARDLESS || WindowMode == ClientConfig::BOARDLESS_FSCREEN)
	{
		return WS_POPUP | WS_MINIMIZEBOX;
	}
	else
	{
		return WS_POPUP;
	}
	return WS_POPUP;
}

bool CPythonApplication::Create(const char * c_szName, int32_t width, int32_t height, int32_t Windowed)
{
	Windowed = m_pySystem.GetWindowMode();

	bool bAnotherWindow = false;

	if (FindWindow(NULL, c_szName))
		bAnotherWindow = true;

	m_dwWidth = width;
	m_dwHeight = height;

	std::string name = c_szName;
	auto end_it = utf8::find_invalid(name.begin(), name.end());

	std::wstring utf16line;
	utf8::utf8to16(name.begin(), end_it, back_inserter(utf16line));

	if (!CMSWindow::Create(utf16line.c_str(), 4, CS_DBLCLKS, __GetWindowMode(Windowed), ::LoadIcon(GetHInstance(), MAKEINTRESOURCE(IDI_METIN2)), IDC_CURSOR_NORMAL))
	{
		SysLog("CMSWindow::Create failed");
		return false;
	}

	AdjustSize(m_pySystem.GetWidth(), m_pySystem.GetHeight());

	if (Windowed == m_pySystem.WINDOWED)
	{
		m_isWindowed = true;
		AdjustClientPosition();
	}
	else if (Windowed == m_pySystem.BOARDLESS)
	{
		m_isWindowed = true;
		AdjustClientPosition();
	}
	else if (Windowed == m_pySystem.BOARDLESS_FSCREEN)
	{
		m_isWindowed = true;
		SetPosition(0, 0);
	}
	else
	{
		m_isWindowed = false;
		SetPosition(0, 0);
	}

	// Cursor & Sound
	if (!CreateCursors() && !m_SoundManager.Initialize())
	{
		SysLog("CMSWindow::Cursors Create Error");
		return false;
	}

	Engine::GetFontManager().PreloadFonts();

	// Device
	if (!CreateDevice(m_pySystem.GetWidth(), m_pySystem.GetHeight(), Windowed, m_pySystem.GetBPP(), m_pySystem.GetFrequency()))
		return false;

	SetVisibleMode(true);

	if (m_isWindowFullScreenEnable)
	{
		SetWindowPos(GetWindowHandle(), HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
	}

	// Mouse
	if (m_pySystem.IsSoftwareCursor())
		SetCursorMode(CURSOR_MODE_SOFTWARE);
	else
		SetCursorMode(CURSOR_MODE_HARDWARE);

	// Network
	if (!m_netDevice.Create())
	{
		SysLog("NetDevice::Create failed");
		return false;
	}

	m_pyItem.Create();

	// TextTails:
	//CPythonTextTail::GetInstance()->Initialize();

	// Light Manager
	m_LightManager.Initialize();

	// Set application window Everythin is the Children of this!
	m_kWndMgr.SetAppWindow(GetWindowHandle());
	CGraphicImageInstance::CreateSystem(32);

	STICKYKEYS sStickKeys;
	std::memset(&sStickKeys, 0, sizeof(sStickKeys));
	sStickKeys.cbSize = sizeof(sStickKeys);
	SystemParametersInfo(SPI_GETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0);
	m_dwStickyKeysFlag = sStickKeys.dwFlags;
	sStickKeys.dwFlags &= ~(SKF_AVAILABLE | SKF_HOTKEYACTIVE);
	SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0);

	// SphereMap
	CGrannyMaterial::CreateSphereMap(0, "d:/ymir work/special/spheremap.jpg");
	CGrannyMaterial::CreateSphereMap(1, "d:/ymir work/special/spheremap01.jpg");

	// ImGui
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(GetWindowHandle());
	ImGui_ImplDX9_Init(m_grpDevice.GetDevice());

	// Resource
	//if (!AbstractResources::GetInstance()->RegisterGuildBuildingList())
	//{
	//	SysLog("Guild Building list load error!");
	//	return false;
	//}

	if (!AbstractResources::GetInstance()->LoadDefaultGameData())
	{
		SysLog("Load Default Resources error!");
		return false;
	}

	if (!AbstractResources::GetInstance()->LoadRaceData())
	{
		SysLog("Load Race Resources error!");
		return false;
	}

	return true;
}

void CPythonApplication::SetGlobalCenterPosition(int32_t x, int32_t y)
{
	auto rkBG=CPythonBackground::GetInstance();
	rkBG->GlobalPositionToLocalPosition(x, y);

	float z = CPythonBackground::GetInstance()->GetHeight(x, y);

	CPythonApplication::GetInstance()->SetCenterPosition(x, y, z);
}

void CPythonApplication::SetCenterPosition(float fx, float fy, float fz)
{
	m_v3CenterPosition.x = +fx;
	m_v3CenterPosition.y = -fy;
	m_v3CenterPosition.z = +fz;
}

void CPythonApplication::GetCenterPosition(TPixelPosition * pPixelPosition)
{
	pPixelPosition->x = +m_v3CenterPosition.x;
	pPixelPosition->y = -m_v3CenterPosition.y;
	pPixelPosition->z = +m_v3CenterPosition.z;
}


void CPythonApplication::SetServerTime(time_t tTime)
{
	m_dwStartLocalTime	= ELTimer_GetMSec();
	m_tServerTime		= tTime;
	m_tLocalStartTime	= time(0);
}

time_t CPythonApplication::GetServerTime()
{
	return (ELTimer_GetMSec() - m_dwStartLocalTime) + m_tServerTime;
}

time_t CPythonApplication::GetServerTimeStamp()
{
	return (time(0) - m_tLocalStartTime) + m_tServerTime;
}

float CPythonApplication::GetGlobalTime()
{
	return m_fGlobalTime;
}

float CPythonApplication::GetGlobalElapsedTime()
{
	return m_fGlobalElapsedTime;
}

void CPythonApplication::SetFPS(int32_t iFPS)
{
	m_iFPS = iFPS;
}

int32_t CPythonApplication::GetWidth()
{
	return m_dwWidth;
}

int32_t CPythonApplication::GetHeight()
{
	return m_dwHeight;
}

void CPythonApplication::SetConnectData(const char * c_szIP, int32_t iPort)
{
	m_strIP = c_szIP;
	m_iPort = iPort;
}

void CPythonApplication::GetConnectData(std::string & rstIP, int32_t & riPort)
{
	rstIP	= m_strIP;
	riPort	= m_iPort;
}

void CPythonApplication::SetCameraSpeed(int32_t iPercentage)
{
	m_fCameraRotateSpeed = c_fDefaultCameraRotateSpeed * float(iPercentage) / 100.0f;
	m_fCameraPitchSpeed = c_fDefaultCameraPitchSpeed * float(iPercentage) / 100.0f;
	m_fCameraZoomSpeed = c_fDefaultCameraZoomSpeed * float(iPercentage) / 100.0f;
}

void CPythonApplication::SetForceSightRange(int32_t iRange)
{
	m_iForceSightRange = iRange;
}

void CPythonApplication::Clear()
{
}

void CPythonApplication::Destroy()
{
	CGrannyMaterial::DestroySphereMap();

	m_kWndMgr.Destroy();

	ClientConfig::GetInstance()->SaveConfig();

	DestroyCollisionInstanceSystem();

	m_pyEventManager.Destroy();	
	m_FlyingManager.Destroy();

	m_pyMiniMap.Destroy();

	//m_pyTextTail.Destroy();
	m_pyChat.Destroy();	
	m_kChrMgr.Destroy();
	m_RaceManager.Destroy();

	m_pyItem.Destroy();
	m_kItemMgr.Destroy();

	m_pyBackground.Destroy();

	m_kEftMgr.Destroy();
	m_LightManager.Destroy();

	m_pyGraphic.Destroy();

	// ImGui
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();

	m_pyRes.Destroy();

	m_kGuildMarkDownloader.Disconnect();

	CGrannyModelInstance::DestroySystem();
	CGraphicImageInstance::DestroySystem();

	m_SoundManager.Destroy();
	m_grpDevice.Destroy();

	CTextFileLoader::DestroySystem();
	DestroyCursors();

	CMSApplication::Destroy();

	STICKYKEYS sStickKeys;
	std::memset(&sStickKeys, 0, sizeof(sStickKeys));
	sStickKeys.cbSize = sizeof(sStickKeys);
	sStickKeys.dwFlags = m_dwStickyKeysFlag;
	SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0);

	Engine::SetKeyboardInput(nullptr);
}
