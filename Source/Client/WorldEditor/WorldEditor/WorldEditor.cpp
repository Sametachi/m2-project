// WorldEditor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WorldEditor.h"

#include "MainFrm.h"
#include "WorldEditorDoc.h"
#include "WorldEditorView.h"

#include "DataCtrl/NonPlayerCharacterInfo.h"
#include "../../../Libraries/eterlib/Camera.h"

#include <SpeedTreeRT.h>

#include <fmt/fmt.h>

#include "Config/Globals.h"
#include "../../../Libraries/EterBase/error.h"

#pragma comment(linker, "/NODEFAULTLIB:libci.lib")

#if GrannyProductMinorVersion==4
#pragma comment( lib, "granny2.4.0.10.lib" )
#elif GrannyProductMinorVersion==7
#pragma comment( lib, "granny2.7.0.30.lib" )
#elif GrannyProductMinorVersion==9
#pragma comment( lib, "granny2.9.12.0.lib" )
#elif GrannyProductMinorVersion==11
#pragma comment( lib, "granny2.11.8.0.lib" )
#else
#error "unknown granny version"
#endif
#pragma comment( lib, "mss32.lib" )

#pragma comment( lib, "DevIL.lib" )
#pragma comment( lib, "ILU.lib" )

#pragma comment( lib, "oldnames.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )

#pragma comment(lib, "d3d8.lib")
#pragma comment(lib, "d3dx8.lib")

#pragma comment( lib, "version.lib" )
#pragma comment( lib, "imagehlp.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "imm32.lib" )
#pragma comment( lib, "SpeedTreeRT.lib" )

#include <cryptopp/cryptoppLibLink.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

float CCamera::CAMERA_MIN_DISTANCE = 200.0f;
float CCamera::CAMERA_MAX_DISTANCE = 2500.0f;

//#define USE_PACK

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp

BEGIN_MESSAGE_MAP(CWorldEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CWorldEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp construction

CWorldEditorApp::CWorldEditorApp()
{
	timeBeginPeriod(1);
}

CWorldEditorApp::~CWorldEditorApp()
{
	timeEndPeriod(1);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWorldEditorApp object

CWorldEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp initialization

void PackInitialize(const char * c_pszFolder)
{
	if (_access(c_pszFolder, 0) != 0)
	{
		return;
	}

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	std::string stFileName(stFolder);
	stFileName += "Index";

	CMappedFile file;
	LPCVOID pvData;

	if (!file.Create(stFileName.c_str(), &pvData, 0, 0))
	{
		LogBoxf("FATAL ERROR! File not exist: %s", stFileName.c_str());
		TraceError("FATAL ERROR! File not exist: %s", stFileName.c_str());
		return;
	}

	CMemoryTextFileLoader TextLoader;
	TextLoader.Bind(file.Size(), pvData);

	for (uint32_t i = 1; i < TextLoader.GetLineCount() - 1; i += 2)
	{
		const std::string & c_rstFolder = TextLoader.GetLineString(i);
		const std::string & c_rstName = TextLoader.GetLineString(i + 1);

		CEterPackManager::Instance().RegisterPack((stFolder + c_rstName).c_str(), c_rstFolder.c_str());
	}

	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());
	CEterPackManager::Instance().SetSearchMode(CEterPackManager::SEARCH_FILE_FIRST);
	//CEterPackManager::Instance().SetRelativePathMode();
	CSoundData::SetPackMode();
}

#ifdef CWE_CENTERED_WINDOW
void SetWindowCentered(HWND hWnd)
{
	//Getting the desktop hadle and rectangule
	HWND   hwndScreen;
	RECT   rectScreen;
	hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);

	// Get the current width and height of the world editor window
	RECT rConsole;
	GetWindowRect(hWnd, &rConsole);
	int32_t Width = rConsole.left = rConsole.right;
	int32_t Height = rConsole.bottom - rConsole.top;

	//move the world editor window to center of the screen
	int32_t ConsolePosX;
	int32_t ConsolePosY;
	ConsolePosX = ((rectScreen.right - rectScreen.left) / 2 - Width / 2);
	ConsolePosY = ((rectScreen.bottom - rectScreen.top) / 2 - Height / 2);
	SetWindowPos(hWnd, NULL, ConsolePosX, ConsolePosY, Width, Height, SWP_SHOWWINDOW || SWP_NOSIZE);
}
#endif

#ifdef ENABLE_PYTHON_SYSTEM
void PythonLoadAllModules()
{
	initdbg();
	initWorldEditor();
}

void PythonLoadAllConstants()
{
	defWorldEditor();
}
#endif

BOOL CWorldEditorApp::InitInstance()
{
#ifdef _DEBUG
	OpenConsoleWindow();
	OpenLogFile();
#endif
	SetEterExceptionHandler();
#ifdef USE_WE_CONFIG
	globals::readGlobalConfigs("WorldEditorRemix.ini");
#endif
	#ifdef CWE_MULTI_LANGUAGE
	if (globals::dft::LOCALE != "EN")
	{
		auto libraryName = fmt::format("WorldEditorRemix_{}.dll", globals::dft::LOCALE);
		HINSTANCE hInst = LoadLibrary(libraryName.c_str());
		if (hInst)
			AfxSetResourceHandle(hInst);
		else
			TraceError("Language Pack %s not loaded", libraryName.c_str());
	}
	#endif

	PackInitialize("pack");

	AfxEnableControlContainer();

	// Standard initialization

#if _MFC_VER < 0x0500
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey(_T("WorldEditor ReMIX"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register document templates
	CSingleDocTemplate * pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(IDR_MAINFRAME,
										  RUNTIME_CLASS(CWorldEditorDoc),
										  RUNTIME_CLASS(CMainFrame),       // main SDI frame window
										  RUNTIME_CLASS(CWorldEditorView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	////////////////////////////////////////////////////////////////////
	//// Specialize Code Start
	if (!_access("pack", 0) && !_access("pack/property.eix", 0) && !_access("pack/property.epk", 0)) // @fixme112 check if folder property exists
		CPropertyManager::Instance().Initialize("pack/property");
	else
		CPropertyManager::Instance().Initialize(NULL);

	CNonPlayerCharacterInfo::Instance().LoadNonPlayerData(globals::dft::MOB_PROTO_PATH.c_str());
	CNonPlayerCharacterInfo::Instance().LoadNPCGroupData("group.txt");

	_getcwd(g_szProgramPath, PROGRAM_PATH_LENGTH);
	_getcwd(g_szProgramWindowPath, PROGRAM_PATH_LENGTH);
	StringPath(g_szProgramPath);

	m_GraphicDevice.Create(m_pMainWnd->GetSafeHwnd(), globals::dft::WINDOW_WIDTH_SIZE, globals::dft::WINDOW_HEIGHT_SIZE);
	m_pMainWnd->SetWindowText("WorldEditor ReMIX");
	CreateUtilData();

	g_PopupHwnd = m_pMainWnd->GetSafeHwnd();

	// Initialize
	CMainFrame * pFrame = (CMainFrame *) m_pMainWnd;
	pFrame->Initialize();

	srandom(time(0));

	m_SoundManager.Create();

	m_LightManager.Initialize();
	//// Specialize Code End
	////////////////////////////////////////////////////////////////////

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	/////
	CWorldEditorApp * pApplication = (CWorldEditorApp *) AfxGetApp();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();
	CRect Rect;
	pView->GetClientRect(&Rect);
	pApplication->GetGraphicDevice().ResizeBackBuffer(Rect.Width(), Rect.Height());

#ifdef CWE_CENTERED_WINDOW
	if (globals::dft::CENTERED_WINDOW)
		SetWindowCentered(g_PopupHwnd);
#endif

#ifdef ENABLE_PYTHON_SYSTEM
	RegisterWorldEditorApp(this);
	PythonLoadAllModules();
	wcpy::App::Init();
	PythonLoadAllConstants();
#endif

	return TRUE;
}

int32_t CWorldEditorApp::ExitInstance()
{
	DestroyUtilData();
	m_GraphicDevice.Destroy();
	m_SoundManager.Destroy();
	m_EffectManager.Destroy();
#ifdef ENABLE_PYTHON_SYSTEM
	wcpy::App::Clear();
	RegisterWorldEditorApp(nullptr);
#endif

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWorldEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CWorldEditorApp::OnAppExit()
{
	ExitInstance();
	CWinApp::OnAppExit();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp normal functions

CObjectData * CWorldEditorApp::GetObjectData()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetObjectData();
}

CEffectAccessor * CWorldEditorApp::GetEffectAccessor()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetEffectAccessor();
}

CMapManagerAccessor * CWorldEditorApp::GetMapManagerAccessor()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();

	// Temporary
	if (!pFrame->IsWindowVisible())
		return NULL;
	// Temporary

	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetMapManagerAccessor();
}

CSceneObject * CWorldEditorApp::GetSceneObject()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetSceneObject();
}

CSceneEffect * CWorldEditorApp::GetSceneEffect()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetSceneEffect();
}

CSceneMap * CWorldEditorApp::GetSceneMap()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetSceneMap();
}

CSceneFly * CWorldEditorApp::GetSceneFly()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();

	if (!pDocument)
		return NULL;

	return pDocument->GetSceneFly();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp message handlers

BOOL CWorldEditorApp::OnIdle(int32_t lCount)
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();

	pView->Process();

	return CWinApp::OnIdle(lCount);
}

CMainFrame* CWorldEditorApp::GetFrame()
{
	return (CMainFrame*)AfxGetMainWnd();
}

CWorldEditorDoc* CWorldEditorApp::GetDocument()
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc* pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();
	return pDocument;
}

CWorldEditorView* CWorldEditorApp::GetView()
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView* pView = (CWorldEditorView*)pFrame->GetActiveView();
	return pView;
}
