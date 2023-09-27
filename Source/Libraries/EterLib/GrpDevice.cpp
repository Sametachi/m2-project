#include "StdAfx.h"
#include "GrpDevice.h"
#include <EterBase/Stl.h>
#include <DxErr.h>
#include "../../ThirdParty/ImGUI/imgui.h"
#include "../../ThirdParty/ImGUI/backends/imgui_impl_dx9.h"

bool GRAPHICS_CAPS_CAN_NOT_DRAW_LINE = false;
bool GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW = false;
bool GRAPHICS_CAPS_HALF_SIZE_IMAGE = false;
bool GRAPHICS_CAPS_CAN_NOT_TEXTURE_ADDRESS_BORDER = false;

D3DPRESENT_PARAMETERS g_kD3DPP;

CGraphicDevice::CGraphicDevice()
: m_uBackBufferCount(0)
{
	__Initialize();
}

CGraphicDevice::~CGraphicDevice()
{
	Destroy();
}

void CGraphicDevice::__Initialize()
{
	ms_iD3DAdapterInfo=D3DADAPTER_DEFAULT;
	ms_iD3DDevInfo=D3DADAPTER_DEFAULT;
	ms_iD3DModeInfo=D3DADAPTER_DEFAULT;

	ms_lpd3d			= NULL;
	ms_lpd3dDevice		= NULL;
	ms_lpd3dMatStack	= NULL;

	ms_dwWavingEndTime = 0;
	ms_dwFlashingEndTime = 0;

	m_smallPdtVertexBuffer = nullptr;
	m_largePdtVertexBuffer = nullptr;

	m_pStateManager		= NULL;

	__InitializeDefaultIndexBufferList();
}

void CGraphicDevice::RegisterWarningString(UINT uiMsg, const char * c_szString)
{
	m_kMap_strWarningMessage[uiMsg] = c_szString;
}

void CGraphicDevice::__WarningMessage(HWND hWnd, UINT uiMsg)
{
	if (m_kMap_strWarningMessage.end() == m_kMap_strWarningMessage.find(uiMsg))
		return;
	MessageBox(hWnd, m_kMap_strWarningMessage[uiMsg].c_str(), "Warning", MB_OK|MB_TOPMOST);
}

bool CGraphicDevice::ResizeBackBuffer(UINT uWidth, UINT uHeight)
{
	if (!ms_lpd3dDevice)
		return false;

	D3DPRESENT_PARAMETERS& rkD3DPP=ms_d3dPresentParameter;
	if (rkD3DPP.Windowed)
	{
		if (rkD3DPP.BackBufferWidth!=uWidth || rkD3DPP.BackBufferHeight!=uHeight)
		{
			rkD3DPP.BackBufferWidth=uWidth;
			rkD3DPP.BackBufferHeight=uHeight;

			IDirect3DDevice9& rkD3DDev=*ms_lpd3dDevice;

			HRESULT hr=rkD3DDev.Reset(&rkD3DPP);
			if (FAILED(hr))
			{
				return false;
			}

			STATEMANAGER->SetDefaultState();
		}
	}

	return true;
}

DWORD CGraphicDevice::CreatePNTStreamVertexShader()
{
	assert(ms_lpd3dDevice != NULL);
	return D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
}

DWORD CGraphicDevice::CreatePNT2StreamVertexShader()
{
	assert(ms_lpd3dDevice != NULL);
	return D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 | D3DFVF_TEX2;
}

CGraphicDevice::EDeviceState CGraphicDevice::GetDeviceState()
{
	if (!ms_lpd3dDevice)
		return DEVICESTATE_NULL;

	HRESULT hr;

	if (FAILED(hr = ms_lpd3dDevice->TestCooperativeLevel()))
	{
		if (D3DERR_DEVICELOST == hr)
			return DEVICESTATE_BROKEN;

		if (D3DERR_DEVICENOTRESET == hr)
			return DEVICESTATE_NEEDS_RESET;

		return DEVICESTATE_BROKEN;
	}

	return DEVICESTATE_OK;
}

void CGraphicDevice::OnLostDevice()
{
	__DestroyPDTVertexBufferList();
	ImGui_ImplDX9_InvalidateDeviceObjects();

	//CRenderTargetManager::InstancePtr()->ReleaseRenderTargetTextures();
	ConsoleLog("Successfully removed device data!");
}

void CGraphicDevice::OnResetDevice()
{
	m_pStateManager->SetDefaultState();

	ImGui_ImplDX9_CreateDeviceObjects();
	//CRenderTargetManager::InstancePtr()->CreateRenderTargetTextures();

	__CreatePDTVertexBufferList();
	ConsoleLog("Successfully restored device data!");
}

bool CGraphicDevice::Reset()
{
	OnLostDevice();
	HRESULT hr;
	if (FAILED(hr = ms_lpd3dDevice->Reset(&ms_d3dPresentParameter)))
	{
		SysLog("Failed to reset: {0} {1}", DXGetErrorString(hr), DXGetErrorDescription(hr));
		return false;
	}

	OnResetDevice();
	return true;
}

static LPDIRECT3DSURFACE9 s_lpStencil;
static uint32_t   s_MaxTextureWidth, s_MaxTextureHeight;

LPDIRECT3D9 CGraphicDevice::GetDirectx9()
{
	return ms_lpd3d;
}

LPDIRECT3DDEVICE9 CGraphicDevice::GetDevice() { return ms_lpd3dDevice; }

BOOL EL3D_ConfirmDevice(D3DCAPS9& rkD3DCaps, UINT uBehavior, D3DFORMAT /*eD3DFmt*/)
{
	if (uBehavior & D3DCREATE_PUREDEVICE)
        return FALSE;

	if (uBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING)
	{
		// DirectionalLight
		if (!(rkD3DCaps.VertexProcessingCaps & D3DVTXPCAPS_DIRECTIONALLIGHTS))
			return FALSE;

		// PositionalLight
		if (!(rkD3DCaps.VertexProcessingCaps & D3DVTXPCAPS_POSITIONALLIGHTS))
			return FALSE;

		// Shadow/Terrain
		if (!(rkD3DCaps.VertexProcessingCaps & D3DVTXPCAPS_TEXGEN))
			return FALSE;
	}

	s_MaxTextureWidth = rkD3DCaps.MaxTextureWidth;
	s_MaxTextureHeight = rkD3DCaps.MaxTextureHeight;

	return TRUE;
}

uint32_t GetMaxTextureWidth()
{
	return s_MaxTextureWidth;
}

uint32_t GetMaxTextureHeight()
{
	return s_MaxTextureHeight;
}

bool CGraphicDevice::__IsInDriverBlackList(D3D_CAdapterInfo& rkD3DAdapterInfo)
{
	D3DADAPTER_IDENTIFIER9& rkD3DAdapterIdentifier=rkD3DAdapterInfo.GetIdentifier();

	char szSrcDriver[256];
	strncpy(szSrcDriver, rkD3DAdapterIdentifier.Driver, sizeof(szSrcDriver)-1);
	uint32_t dwSrcHighVersion=rkD3DAdapterIdentifier.DriverVersion.QuadPart>>32;
	uint32_t dwSrcLowVersion=rkD3DAdapterIdentifier.DriverVersion.QuadPart&0xffffffff;

	bool ret=false;

	FILE* fp=fopen("grpblk.txt", "r");
	if (fp)
	{
		uint32_t dwChkHighVersion;
		uint32_t dwChkLowVersion;

		char szChkDriver[256];

		char szLine[256];
		while (fgets(szLine, sizeof(szLine)-1, fp))
		{
			sscanf(szLine, "%s %x %x", szChkDriver, &dwChkHighVersion, &dwChkLowVersion);

			if (strcmp(szSrcDriver, szChkDriver)==0)
				if (dwSrcHighVersion==dwChkHighVersion)
					if (dwSrcLowVersion==dwChkLowVersion)
					{
						ret=true;
						break;
					}

			szLine[0]='\0';
		}
		fclose(fp);
	}

	return ret;
}

int32_t CGraphicDevice::Create(HWND hWnd, int32_t iHres, int32_t iVres, bool Windowed, int32_t /*iBit*/, int32_t iReflashRate)
{
	int32_t iRet = CREATE_OK;

	Destroy();

	ms_iWidth	= iHres;
	ms_iHeight	= iVres;

	ms_hWnd = hWnd;
	ms_hDC = GetDC(hWnd);
	ms_lpd3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!ms_lpd3d)
		return CREATE_NO_DIRECTX;

	if (!ms_kD3DDetector.Build(*ms_lpd3d, EL3D_ConfirmDevice))
		return CREATE_ENUM;

	if (!ms_kD3DDetector.Find(800, 600, 32, TRUE, &ms_iD3DModeInfo, &ms_iD3DDevInfo, &ms_iD3DAdapterInfo))
		return CREATE_DETECT;

	std::string stDevList;
	ms_kD3DDetector.GetString(&stDevList);

	//Tracen(stDevList.c_str());
	//Tracenf("adapter %d, device %d, mode %d", ms_iD3DAdapterInfo, ms_iD3DDevInfo, ms_iD3DModeInfo);

	D3D_CAdapterInfo * pkD3DAdapterInfo = ms_kD3DDetector.GetD3DAdapterInfop(ms_iD3DAdapterInfo);
	if (!pkD3DAdapterInfo)
	{
		TraceLog("adapter {} is EMPTY", ms_iD3DAdapterInfo);
		return CREATE_DETECT;
	}

	if (__IsInDriverBlackList(*pkD3DAdapterInfo))
	{
		iRet |= CREATE_BAD_DRIVER;
		__WarningMessage(hWnd, CREATE_BAD_DRIVER);
	}

	D3D_SModeInfo * pkD3DModeInfo = pkD3DAdapterInfo->GetD3DModeInfop(ms_iD3DDevInfo, ms_iD3DModeInfo);
	if (!pkD3DModeInfo)
	{
		TraceLog("device {}, mode %d is EMPTY", ms_iD3DDevInfo, ms_iD3DModeInfo);
		return CREATE_DETECT;
	}

	D3DADAPTER_IDENTIFIER9& rkD3DAdapterId=pkD3DAdapterInfo->GetIdentifier();
	if (Windowed &&
		strnicmp(rkD3DAdapterId.Driver, "3dfx", 4)==0 &&
		22 == pkD3DAdapterInfo->GetDesktopD3DDisplayModer().Format)
	{
		return CREATE_FORMAT;
	}

	if (pkD3DModeInfo->m_dwD3DBehavior==D3DCREATE_SOFTWARE_VERTEXPROCESSING)
	{
		iRet |= CREATE_NO_TNL;

		// DISABLE_NOTIFY_NOT_SUPPORT_TNL_MESSAGE
		//__WarningMessage(hWnd, CREATE_NO_TNL);
		// END_OF_DISABLE_NOTIFY_NOT_SUPPORT_TNL_MESSAGE
	}

	std::string stModeInfo;
	pkD3DModeInfo->GetString(&stModeInfo);

	//Tracen(stModeInfo.c_str());

	int32_t ErrorCorrection = 0;

RETRY:
	ZeroMemory(&ms_d3dPresentParameter, sizeof(ms_d3dPresentParameter));

	ms_d3dPresentParameter.Windowed							= Windowed;
	ms_d3dPresentParameter.BackBufferWidth					= iHres;
	ms_d3dPresentParameter.BackBufferHeight					= iVres;
	ms_d3dPresentParameter.hDeviceWindow					= hWnd;
	ms_d3dPresentParameter.BackBufferCount					= m_uBackBufferCount;
	ms_d3dPresentParameter.SwapEffect						= D3DSWAPEFFECT_DISCARD;

	if (Windowed)
	{
		ms_d3dPresentParameter.BackBufferFormat				= pkD3DAdapterInfo->GetDesktopD3DDisplayModer().Format;
	}
	else
	{
		ms_d3dPresentParameter.BackBufferFormat				= pkD3DModeInfo->m_eD3DFmtPixel;
		ms_d3dPresentParameter.FullScreen_RefreshRateInHz	= iReflashRate;
	}

	ms_d3dPresentParameter.Flags							= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	ms_d3dPresentParameter.EnableAutoDepthStencil			= TRUE;
	ms_d3dPresentParameter.AutoDepthStencilFormat			= pkD3DModeInfo->m_eD3DFmtDepthStencil;

	ms_dwD3DBehavior = pkD3DModeInfo->m_dwD3DBehavior;

	if (FAILED(ms_hLastResult = ms_lpd3d->CreateDevice(
				ms_iD3DAdapterInfo,
				D3DDEVTYPE_HAL,
				hWnd,
				pkD3DModeInfo->m_dwD3DBehavior,
				&ms_d3dPresentParameter,
				&ms_lpd3dDevice)))
	{
		switch (ms_hLastResult)
		{
			case D3DERR_INVALIDCALL:
				TraceLog("IDirect3DDevice.CreateDevice - ERROR D3DERR_INVALIDCALL\nThe method call is invalid. For example, a method's parameter may have an invalid value.");
				break;
			case D3DERR_NOTAVAILABLE:
				TraceLog("IDirect3DDevice.CreateDevice - ERROR D3DERR_NOTAVAILABLE\nThis device does not support the queried technique. ");
				break;
			case D3DERR_OUTOFVIDEOMEMORY:
				TraceLog("IDirect3DDevice.CreateDevice - ERROR D3DERR_OUTOFVIDEOMEMORY\nDirect3D does not have enough display memory to perform the operation");
				break;
			default:
				TraceLog("IDirect3DDevice.CreateDevice - ERROR %d", ms_hLastResult);
				break;
		}

		if (ErrorCorrection)
			return CREATE_DEVICE;

		iReflashRate = 0;
		++ErrorCorrection;
		iRet = CREATE_REFRESHRATE;
		goto RETRY;
	}

	// Check DXT Support Info
	if(ms_lpd3d->CheckDeviceFormat(
				ms_iD3DAdapterInfo,
				D3DDEVTYPE_HAL,
				ms_d3dPresentParameter.BackBufferFormat,
				0,
				D3DRTYPE_TEXTURE,
				D3DFMT_DXT1) == D3DERR_NOTAVAILABLE)
	{
		ms_bSupportDXT = false;
	}

	if(ms_lpd3d->CheckDeviceFormat(
				ms_iD3DAdapterInfo,
				D3DDEVTYPE_HAL,
				ms_d3dPresentParameter.BackBufferFormat,
				0,
				D3DRTYPE_TEXTURE,
				D3DFMT_DXT3) == D3DERR_NOTAVAILABLE)
	{
		ms_bSupportDXT = false;
	}

	if(ms_lpd3d->CheckDeviceFormat(
				ms_iD3DAdapterInfo,
				D3DDEVTYPE_HAL,
				ms_d3dPresentParameter.BackBufferFormat,
				0,
				D3DRTYPE_TEXTURE,
				D3DFMT_DXT5) == D3DERR_NOTAVAILABLE)
	{
		ms_bSupportDXT = false;
	}

	if (FAILED((ms_hLastResult = ms_lpd3dDevice->GetDeviceCaps(&ms_d3dCaps))))
	{
		TraceLog("IDirect3DDevice.GetDeviceCaps - ERROR {}", ms_hLastResult);
		return CREATE_GET_DEVICE_CAPS2;
	}

	if (!Windowed)
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, iHres, iVres, SWP_SHOWWINDOW);

	//Tracef("vertex shader version : %X\n",(uint32_t)ms_d3dCaps.VertexShaderVersion);

	ms_lpd3dDevice->GetViewport(&ms_Viewport);

	m_pStateManager = new CStateManager(ms_lpd3dDevice);

	D3DXCreateMatrixStack(0, &ms_lpd3dMatStack);
	ms_lpd3dMatStack->LoadIdentity();

	ms_pntVS = CreatePNTStreamVertexShader();
	ms_pnt2VS = CreatePNT2StreamVertexShader();

	D3DXMatrixIdentity(&ms_matIdentity);
	D3DXMatrixIdentity(&ms_matView);
	D3DXMatrixIdentity(&ms_matProj);
	D3DXMatrixIdentity(&ms_matInverseView);
	D3DXMatrixIdentity(&ms_matInverseViewYAxis);
	D3DXMatrixIdentity(&ms_matScreen0);
	D3DXMatrixIdentity(&ms_matScreen1);
	D3DXMatrixIdentity(&ms_matScreen2);

	ms_matScreen0._11 = 1;
	ms_matScreen0._22 = -1;

	ms_matScreen1._41 = 1;
	ms_matScreen1._42 = 1;

	ms_matScreen2._11 = (float) iHres / 2;
	ms_matScreen2._22 = (float) iVres / 2;

	D3DXCreateSphere(ms_lpd3dDevice, 1.0f, 32, 32, &ms_lpSphereMesh, NULL);
	D3DXCreateCylinder(ms_lpd3dDevice, 1.0f, 1.0f, 1.0f, 8, 8, &ms_lpCylinderMesh, NULL);

	ms_lpd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);

	if (!__CreateDefaultIndexBufferList())
		return false;

	if (!__CreatePDTVertexBufferList())
		return false;

	uint32_t dwTexMemSize = GetAvailableTextureMemory();

	if (dwTexMemSize < 64 * 1024 * 1024)
		ms_isLowTextureMemory = true;
	else
		ms_isLowTextureMemory = false;

	if (dwTexMemSize > 100 * 1024 * 1024)
		ms_isHighTextureMemory = true;
	else
		ms_isHighTextureMemory = false;

	if (ms_d3dCaps.TextureAddressCaps & D3DPTADDRESSCAPS_BORDER)
		GRAPHICS_CAPS_CAN_NOT_TEXTURE_ADDRESS_BORDER=false;
	else
		GRAPHICS_CAPS_CAN_NOT_TEXTURE_ADDRESS_BORDER=true;

	//D3DADAPTER_IDENTIFIER8& rkD3DAdapterId=pkD3DAdapterInfo->GetIdentifier();
	if (strnicmp(rkD3DAdapterId.Driver, "SIS", 3) == 0)
	{
		GRAPHICS_CAPS_CAN_NOT_DRAW_LINE = true;
		GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW = true;
		GRAPHICS_CAPS_HALF_SIZE_IMAGE = true;
		ms_isLowTextureMemory = true;
	}
	else if (strnicmp(rkD3DAdapterId.Driver, "3dfx", 4) == 0)
	{
		GRAPHICS_CAPS_CAN_NOT_DRAW_SHADOW = true;
		GRAPHICS_CAPS_HALF_SIZE_IMAGE = true;
		ms_isLowTextureMemory = true;
	}

	return (iRet);
}

void CGraphicDevice::__DestroyPDTVertexBufferList()
{
	if (m_smallPdtVertexBuffer)
	{
		m_smallPdtVertexBuffer->Release();
		m_smallPdtVertexBuffer = nullptr;
	}

	if (m_largePdtVertexBuffer)
	{
		m_largePdtVertexBuffer->Release();
		m_largePdtVertexBuffer = nullptr;
	}

	if (m_largePdt2DVertexBuffer)
	{
		m_largePdt2DVertexBuffer->Release();
		m_largePdt2DVertexBuffer = nullptr;
	}
}

bool CGraphicDevice::__CreatePDTVertexBufferList()
{
	auto res = ms_lpd3dDevice->CreateVertexBuffer(sizeof(TPDTVertex) * kSmallPdtVertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, TPDTVertex::kFVF,
		D3DPOOL_DEFAULT, &m_smallPdtVertexBuffer, nullptr);

	if (FAILED(res))
		return false;

	res = ms_lpd3dDevice->CreateVertexBuffer(sizeof(TPDTVertex) * kLargePdtVertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, TPDTVertex::kFVF,
		D3DPOOL_DEFAULT, &m_largePdtVertexBuffer, nullptr);

	if (FAILED(res))
		return false;

	res = ms_lpd3dDevice->CreateVertexBuffer(sizeof(TPDTVertex2D) * kLargePdtVertexBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, TPDTVertex2D::kFVF,
		D3DPOOL_DEFAULT, &m_largePdt2DVertexBuffer, nullptr);

	if (FAILED(res))
		return false;

	return true;
}

void CGraphicDevice::__InitializeDefaultIndexBufferList()
{
	for (UINT i = 0; i < DEFAULT_IB_NUM; ++i)
		ms_alpd3dDefIB[i] = nullptr;
}

void CGraphicDevice::__DestroyDefaultIndexBufferList()
{
	for (UINT i = 0; i < DEFAULT_IB_NUM; ++i)
		if (ms_alpd3dDefIB[i])
		{
			ms_alpd3dDefIB[i]->Release();
			ms_alpd3dDefIB[i] = nullptr;
		}
}

bool CGraphicDevice::__CreateDefaultIndexBuffer(UINT eDefIB, UINT uIdxCount, const uint16_t* c_awIndices)
{
	assert(ms_alpd3dDefIB[eDefIB] == nullptr);

	if (FAILED(
		ms_lpd3dDevice->CreateIndexBuffer(
			sizeof(uint16_t) * uIdxCount,
			D3DUSAGE_WRITEONLY,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&ms_alpd3dDefIB[eDefIB],
			nullptr)
	)) return false;

	uint16_t* dstIndices = nullptr;
	if (FAILED(
		ms_alpd3dDefIB[eDefIB]->Lock(0, 0, (void**)&dstIndices, 0)
	)) return false;

	std::memcpy(dstIndices, c_awIndices, sizeof(uint16_t) * uIdxCount);

	ms_alpd3dDefIB[eDefIB]->Unlock();

	return true;
}

bool CGraphicDevice::__CreateDefaultIndexBufferList()
{
	static const uint16_t c_awLineIndices[2] = { 0, 1, };
	static const uint16_t c_awLineTriIndices[6] = { 0, 1, 0, 2, 1, 2, };
	static const uint16_t c_awLineRectIndices[8] = { 0, 1, 0, 2, 1, 3, 2, 3,};
	static const uint16_t c_awLineCubeIndices[24] = {
		0, 1, 0, 2, 1, 3, 2, 3,
		0, 4, 1, 5, 2, 6, 3, 7,
		4, 5, 4, 6, 5, 7, 6, 7,
	};
	static const uint16_t c_awFillTriIndices[3]= { 0, 1, 2, };
	static const uint16_t c_awFillRectIndices[6] = { 0, 2, 1, 2, 3, 1, };
	static const uint16_t c_awFillCubeIndices[36] = {
		0, 1, 2, 1, 3, 2,
		2, 0, 6, 0, 4, 6,
		0, 1, 4, 1, 5, 4,
		1, 3, 5, 3, 7, 5,
		3, 2, 7, 2, 6, 7,
		4, 5, 6, 5, 7, 6,
	};
	static const uint16_t c_awFont[6] = { 0, 1, 2, 0, 2, 3 };

	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE, 2, c_awLineIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_TRI, 6, c_awLineTriIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_RECT, 8, c_awLineRectIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_LINE_CUBE, 24, c_awLineCubeIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_TRI, 3, c_awFillTriIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_RECT, 6, c_awFillRectIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FILL_CUBE, 36, c_awFillCubeIndices))
		return false;
	if (!__CreateDefaultIndexBuffer(DEFAULT_IB_FONT, 6, c_awFont))
		return false;

	return true;
}

void CGraphicDevice::InitBackBufferCount(UINT uBackBufferCount)
{
	m_uBackBufferCount=uBackBufferCount;
}

void CGraphicDevice::Destroy()
{
	__DestroyPDTVertexBufferList();
	__DestroyDefaultIndexBufferList();

	if (ms_hDC)
	{
		ReleaseDC(ms_hWnd, ms_hDC);
		ms_hDC = NULL;
	}

	if (ms_pntVS)
	{
		ms_pntVS = 0;
	}

	if (ms_pnt2VS)
	{
		ms_pnt2VS = 0;
	}

	safe_release(ms_lpSphereMesh);
	safe_release(ms_lpCylinderMesh);

	safe_release(ms_lpd3dMatStack);
	safe_release(ms_lpd3dDevice);
	safe_release(ms_lpd3d);

	if (m_pStateManager)
	{
		delete m_pStateManager;
		m_pStateManager = NULL;
	}

	__Initialize();
}
