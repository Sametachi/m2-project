// MapFilePage.cpp : implementation file
//

#include "stdafx.h"
#include "..\WorldEditor.h"
#include "MapFilePage.h"
#include "../Dialog/NewMapDlg.h"
#include "../Dialog/DlgGoto.h"
#include "../Dialog/DlginitBaseTexture.h"
#include "../MainFrm.h"
#include "../WorldEditorDoc.h"
#include "../WorldEditorView.h"
#include "../DataCtrl/MapAccessorOutdoor.h"
#include "../Dialog/changebasexydlg.h"
#include "../Dialog/MapPortalDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../Config/Globals.h"
/////////////////////////////////////////////////////////////////////////////
// CMapFilePage dialog


CMapFilePage::CMapFilePage(CWnd* pParent /*=NULL*/)
	: CPageCtrl(CMapFilePage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapFilePage)
		// NOTE: the ClassWizard will add member initialization here
	strEnvironmentDataPath = "D:\\Ymir Work\\environment";
	//}}AFX_DATA_INIT
}

CMapFilePage::~CMapFilePage()
{
	if (m_pPortalDialog)
	{
		delete m_pPortalDialog;
	}

	m_pPortalDialog = NULL;
}

void CMapFilePage::DoDataExchange(CDataExchange* pDX)
{
	CPageCtrl::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapFilePage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMapFilePage, CPageCtrl)
	//{{AFX_MSG_MAP(CMapFilePage)
	ON_BN_CLICKED(IDC_CHECK_AUTO_SAVE, OnCheckAutoSave)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_WIRE_FRAME, OnCheckOptionWireFrame)
	ON_BN_CLICKED(IDC_CHECK_GRID, OnCheckGrid)
	ON_BN_CLICKED(IDC_CHECK_GRID2, OnCheckGrid2)
	ON_BN_CLICKED(IDC_CHECK_CHARACTER, OnCheckCharacterRendering)
	ON_BN_CLICKED(IDC_MAP_FILE_NEW, OnNewMap)
	ON_BN_CLICKED(IDC_MAP_FILE_LOAD, OnLoadMap)
	ON_BN_CLICKED(IDC_MAP_FILE_SAVE, OnSaveMap)
	ON_BN_CLICKED(IDC_MAP_FILE_SAVEAS, OnSaveAsMap)
	ON_BN_CLICKED(IDC_MAP_FILE_SAVE_COLLISION_DATA, OnSaveCollisionData)
	ON_BN_CLICKED(IDC_MAP_FILE_SAVE_ATTRMAP, OnSaveAttrMap)
	ON_BN_CLICKED(IDC_BUTTON_GOTO, OnGoto)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_OBJECT_COLLISION, OnCheckOptionObjectCollisionRendering)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_OBJECT, OnCheckOptionObjectRendering)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_OBJECT_SHADOW, OnMapTerrainOptionObjectShadow)
	ON_BN_CLICKED(IDC_BUTTON_INIT_BASETEXTUREMAP, OnButtonInitBasetexturemap)
	ON_BN_CLICKED(IDC_CHECK_PATCH_GRID, OnCheckPatchGrid)
	ON_BN_CLICKED(IDC_CHECK_WATER, OnCheckWater)
	ON_BN_CLICKED(IDC_CHECK_COMPASS, OnCheckCompass)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_ATLAS, OnButtonSaveAtlas)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_BASEXY, OnButtonChangeBasexy)
	ON_BN_CLICKED(IDC_BUTTON_SELECTE_ENVIRONMENT_SET, OnButtonSelecteEnvironmentSet)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_TERRAIN, OnCheckTerrainOption)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_TERRAIN_HEIGHT, OnButtonChangeTerrainHeight)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_FAKE_PORTAL_ENABLE, OnMapTerrainOptionFakePortalEnable)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_OPTION_FAKE_PORTAL_ID_LIST, OnMapTerrainOptionFakePortalIdList)
	ON_BN_CLICKED(IDC_MAP_TERRAIN_GUILD_AREA, OnMapTerrainGuildArea)
	ON_BN_CLICKED(IDC_BUTTON_INIT_SHADOWMAP, OnButtonInitShadowMap)
#ifdef ENABLE_MAP_ENCRYPTION
	ON_BN_CLICKED(IDC_MAP_SPECIAL_OPTION_DATA_ENCRYPTION_ENABLE, OnMapSpecialOptionDataEncryptionEnable)
#endif
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapFilePage normal functions

BOOL CMapFilePage::Create(CWnd * pParent)
{
	if (!CPageCtrl::Create(CMapFilePage::IDD, pParent))
		return FALSE;

	m_pPortalDialog = new CMapPortalDialog;
	m_pPortalDialog->Create(CMapPortalDialog::IDD, this);
	m_pPortalDialog->ShowWindow(SW_HIDE);

	return TRUE;
}

void CMapFilePage::Initialize()
{
	CheckDlgButton(IDC_CHECK_AUTO_SAVE, FALSE);
	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_WIRE_FRAME, FALSE);
	CheckDlgButton(IDC_CHECK_GRID, FALSE);
	CheckDlgButton(IDC_CHECK_GRID2, FALSE);
	CheckDlgButton(IDC_CHECK_CHARACTER, FALSE);
	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_OBJECT, TRUE);
	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_OBJECT_COLLISION, FALSE);
	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_OBJECT_SHADOW, FALSE);
	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_TERRAIN, TRUE);
#ifdef USE_WE_CONFIG
	CheckDlgButton(IDC_CHECK_CHARACTER, globals::dft::VIEW_CHAR_OUTPUT_BY_DEFAULT);
	// OnCheckCharacterRendering();

	CheckDlgButton(IDC_MAP_TERRAIN_OPTION_OBJECT_SHADOW, globals::dft::VIEW_SHADOW_OUTPUT_BY_DEFAULT);
	// OnMapTerrainOptionObjectShadow();

	CheckDlgButton(IDC_CHECK_WATER, globals::dft::VIEW_WATER_OUTPUT_BY_DEFAULT);
	OnCheckWater();
#endif
	OnCheckOptionWireFrame();
	OnCheckGrid();
	OnCheckGrid2();
	OnCheckCharacterRendering();
	OnCheckOptionObjectRendering();
	OnCheckOptionObjectCollisionRendering();
}

void CMapFilePage::Initialize2()
{
	OnCheckOptionWireFrame();
	OnMapTerrainOptionObjectShadow();
	// OnMapTerrainOptionFakePortalEnable();
#ifdef USE_WE_CONFIG
	if (globals::dft::NOFOG_ONMAPLOAD)
	{
		CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
		CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

		pMapManagerAccessor->EnableFog(IsDlgButtonChecked(!globals::dft::NOFOG_ONMAPLOAD));
	}
#endif
}

void CMapFilePage::UpdateUI()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	CMapOutdoor & rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();

	m_lViewRadius = rMapOutdoor.GetViewRadius();
	m_fHeightScale = rMapOutdoor.GetHeightScale();

	uint32_t dwBaseX, dwBaseY;
	rMapOutdoor.GetBaseXY(&dwBaseX, &dwBaseY);
	std::string strEnvironmentDataName = rMapOutdoor.GetEnvironmentDataName();

	char buf[128];
	SetDlgItemText(IDC_MAP_SCRIPT_NAME_PRINT, rMapOutdoor.GetName().c_str());

	// @warme101
	sprintf(buf, "%.2f meter", (float) (m_lViewRadius) * (float)(CTerrainImpl::CELLSCALE) / 100.0f);
	SetDlgItemText(IDC_STATIC_VIEW_RADIUS, buf);

	sprintf(buf, "%.2f meter", (float)(CTerrainImpl::CELLSCALE) / 100.0f);
	SetDlgItemText(IDC_STATIC_WORLD_SCALE, buf);

	sprintf(buf, "%.2f meter", m_fHeightScale * 65535.0f / 100.0f);
	SetDlgItemText(IDC_STATIC_MAX_HEIGHT, buf);

	sprintf(buf, "%d cm", dwBaseX);
	SetDlgItemText(IDC_STATIC_BASEX, buf);

	sprintf(buf, "%d cm", dwBaseY);
	SetDlgItemText(IDC_STATIC_BASEY, buf);

	SetDlgItemText(IDC_STATIC_ENVIRONMENT_SET, strEnvironmentDataName.c_str());
#ifdef USE_WE_CONFIG
	if (globals::dft::REFRESHALL_ONUPDATEUI)
	{
		OnCheckOptionWireFrame();
		OnCheckGrid();
		OnCheckGrid2();
		OnCheckCharacterRendering();
		OnCheckOptionObjectCollisionRendering();
		OnCheckOptionObjectRendering();
		OnMapTerrainOptionObjectShadow();
		OnCheckPatchGrid();
		OnCheckWater();
		OnCheckCompass();
		OnCheckTerrainOption();
		OnMapTerrainOptionFakePortalIdList();
		OnMapTerrainGuildArea();
	}
#endif
}

void CMapFilePage::RunLoadMapEvent()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();

	m_pPortalDialog->SetMapManagerHandler(pMapManagerAccessor);
}

/////////////////////////////////////////////////////////////////////////////
// CMapFilePage message handlers

void CMapFilePage::OnNewMap()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *) AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	if (pMapManagerAccessor->IsMapReady() && pMapManagerAccessor->IsAutoSave())
		pMapManagerAccessor->SaveMap();

	CNewMapDlg dlg(AfxGetMainWnd());
	if (IDOK != dlg.DoModal())
		return;

	pMapManagerAccessor->CreateNewOutdoorMap();
}

void CMapFilePage::OnLoadMap()
{
	CWorldEditorApp* pApplication = (CWorldEditorApp*)AfxGetApp();
	CMapManagerAccessor* pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	if (pMapManagerAccessor->IsMapReady() && pMapManagerAccessor->IsAutoSave())
		pMapManagerAccessor->SaveMap();

	char szSavingFolder[256 + 32];
	if (!XBrowseForFolder(GetSafeHwnd(), g_szProgramWindowPath, szSavingFolder, 256 + 32))
		return;

	char * pszDir = strrchr(szSavingFolder, '\\');
	if (!pszDir)
		return;
	pszDir++;

	if (!pMapManagerAccessor->LoadMap(pszDir))
		return;

	CMainFrame * pMainFrame = (CMainFrame *) AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *) pMainFrame->GetActiveView();
	pView->UpdateTargetPosition(0.0f, 0.0f);
	pMainFrame->RunLoadMapEvent();

	UpdateUI();
	// @fixme108
	Initialize2();
}

void CMapFilePage::OnSaveMap()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	pMapManagerAccessor->SaveMap();
}

void CMapFilePage::OnSaveAsMap()
{
	CString cstrMap;

	if (!GetDlgItemText(IDC_SAVEAS_FILENAME, cstrMap))
		return;

	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	pMapManagerAccessor->SaveMap(cstrMap);
}

void CMapFilePage::OnSaveCollisionData()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	// uint32_t dwFlag = OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	// @fixme106
	uint32_t dwFlag = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	const char * c_szFilter = "Metin2 Collision Data File (*.mdc)|*.mdc|All Files (*.*)|*.*|";

	CFileDialog FileOpener(FALSE, "Save", "", dwFlag, c_szFilter, this);
	if (TRUE == FileOpener.DoModal() && strlen(FileOpener.GetPathName()) > 0)
	{
		CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
		CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
		pMapManagerAccessor->SaveCollisionData(FileOpener.GetPathName());
		Tracef("SaveCollisionData successfully saved into %s\n", FileOpener.GetPathName());
	}
}

void CMapFilePage::OnSaveAttrMap()
{
	int32_t iResult = MessageBox("Are you sure you want really initializing?", NULL, MB_YESNO);
	if (6 != iResult)
		return;

	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor)
		return;

	pMapManagerAccessor->ResetToDefaultAttr();
}

void CMapFilePage::OnGoto()
{
	CDlgGoto Goto(AfxGetMainWnd());
	if(IDOK != Goto.DoModal())
		return;
}

// Option

void CMapFilePage::OnCheckOptionWireFrame()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	pMapManagerAccessor->SetWireframe(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_WIRE_FRAME) == 1 ? true : false);
}

void CMapFilePage::OnCheckAutoSave()
{
	CWorldEditorApp* pApplication = (CWorldEditorApp*)AfxGetApp();
	CMapManagerAccessor* pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	pMapManagerAccessor->SetAutoSave(IsDlgButtonChecked(IDC_CHECK_AUTO_SAVE) == 1 ? true : false);
}

void CMapFilePage::OnCheckGrid()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetMeterGrid(IsDlgButtonChecked(IDC_CHECK_GRID) != 0);
}

void CMapFilePage::OnCheckGrid2()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetMapBoundGrid(IsDlgButtonChecked(IDC_CHECK_GRID2) != 0);
}

void CMapFilePage::OnCheckCharacterRendering()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetCharacterRendering(IsDlgButtonChecked(IDC_CHECK_CHARACTER) != 0);
}

void CMapFilePage::OnCheckOptionObjectRendering()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetObjectRendering(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_OBJECT) != 0);
}

void CMapFilePage::OnCheckOptionObjectCollisionRendering()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetObjectCollisionRendering(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_OBJECT_COLLISION) != 0);
}

void CMapFilePage::OnCheckTerrainOption()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetTerrainRendering(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_TERRAIN) != 0);
}

// Option

void CMapFilePage::OnMapTerrainOptionObjectShadow()
{
	// TODO: Add your control notification handler code here
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetObjectShadowRendering(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_OBJECT_SHADOW) != 0);
}

void CMapFilePage::OnButtonInitBasetexturemap()
{
	// TODO: Add your control notification handler code here
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	CDlgInitBaseTexture InitBaseTexture(AfxGetMainWnd());
	std::string getOldMap = pMapManagerAccessor->GetMapOutdoorRef().GetName();
	if(IDOK == InitBaseTexture.DoModal())
	{
		// @fixme110
		// pMapManagerAccessor->LoadMap(pMapManagerAccessor->GetMapOutdoorRef().GetName());
		pMapManagerAccessor->LoadMap(getOldMap);

		CMainFrame * pMainFrame = (CMainFrame *) AfxGetMainWnd();
		CWorldEditorView * pView = (CWorldEditorView *) pMainFrame->GetActiveView();
		pView->UpdateTargetPosition(0.0f, 0.0f);
		pMainFrame->RunLoadMapEvent();

		UpdateUI();
		// @fixme108
		Initialize2();
	}
}

void CMapFilePage::OnCheckPatchGrid()
{
	// TODO: Add your control notification handler code here
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetPatchGrid(IsDlgButtonChecked(IDC_CHECK_PATCH_GRID) != 0);
}

void CMapFilePage::OnCheckWater()
{
	// TODO: Add your control notification handler code here
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetWaterRendering(IsDlgButtonChecked(IDC_CHECK_WATER) != 0);
}

void CMapFilePage::OnCheckCompass()
{
	// TODO: Add your control notification handler code here
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetCompass(IsDlgButtonChecked(IDC_CHECK_COMPASS) != 0);

}

void CMapFilePage::OnButtonSaveAtlas()
{
	// TODO: Add your control notification handler code here
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	pMapManagerAccessor->SaveAtlas();
}

void CMapFilePage::OnButtonChangeBasexy()
{
	// TODO: Add your control notification handler code here
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	CChangeBaseXYDlg aDlg;
	aDlg.DoModal();

	UpdateUI();
}

void CMapFilePage::OnButtonSelecteEnvironmentSet()
{
	// TODO: Add your control notification handler code here
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
		return;

	uint32_t dwFlag = OFN_NOCHANGEDIR;
	const char * c_szFilter = "Environment Files (*.msenv)|*.msenv|*.*|All Files (*.*)|";
	CFileDialog FileOpener(TRUE, "Load", "", dwFlag, c_szFilter, this);
	if (TRUE == FileOpener.DoModal())
	{
		std::string strEnvironmentName;
		GetOnlyFileName(FileOpener.GetPathName(), strEnvironmentName);
		pMapManagerAccessor->GetMapOutdoorRef().SetEnvironmentDataName(strEnvironmentName);
		UpdateUI();
	}
}

void CMapFilePage::OnButtonChangeTerrainHeight()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
	{
		LogBox("Not possible during map creation");
		return;
	}

	CMapOutdoorAccessor * pMapOutdoor = pMapManagerAccessor->GetMapOutdoorPtr();
	pMapOutdoor->ArrangeTerrainHeight();
}

void CMapFilePage::OnMapTerrainOptionFakePortalEnable()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
	{
		LogBox("Not possible during map creation");
		CheckDlgButton(IDC_MAP_TERRAIN_OPTION_FAKE_PORTAL_ENABLE, FALSE);
		return;
	}

	pMapManagerAccessor->EnablePortal(IsDlgButtonChecked(IDC_MAP_TERRAIN_OPTION_FAKE_PORTAL_ENABLE) != 0);
	pMapManagerAccessor->RefreshPortal();
}

void CMapFilePage::OnMapTerrainOptionFakePortalIdList()
{
	// LogBox("This button has no implementation.");
}

#ifdef ENABLE_MAP_ENCRYPTION
void CMapFilePage::OnMapSpecialOptionDataEncryptionEnable()
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();

	if (!pMapManagerAccessor->IsMapReady())
	{
		LogBox("Not possible during map creation");
		CheckDlgButton(IDC_MAP_SPECIAL_OPTION_DATA_ENCRYPTION_ENABLE, FALSE);
		return;
	}

	CMapOutdoorAccessor * pMapOutdoor = pMapManagerAccessor->GetMapOutdoorPtr();
	pMapOutdoor->SetEncryptionSet(IsDlgButtonChecked(IDC_MAP_SPECIAL_OPTION_DATA_ENCRYPTION_ENABLE));
}
#endif

void CMapFilePage::OnMapTerrainGuildArea()
{
	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap* pScene = pDocument->GetSceneMap();
	pScene->SetGuildAreaRendering(IsDlgButtonChecked(IDC_MAP_TERRAIN_GUILD_AREA) != 0);
}

// new stuff
#include <il/il.h>
#include <il/ilu.h>
#include "../../../Client/PRTerrainLib/Terrain.h"
#include <vector>
namespace atr{
enum eAttrEnumRet{RET_OK=0, ATTR_NOT_READABLE=1, SERVERATTR_NOT_WRITABLE=2};
typedef struct sAttrTypeRet_s
{
	int32_t iType;
	std::string szErrFile;
} sAttrTypeRet_t;

#define M2_NEW new
#define M2_DELETE(p) delete (p)
#define M2_DELETE_ARRAY(p) delete[] (p)
#define M2_PTR_REF(p) (p)
#define M2_PTR_DEREF(p) (*(p))

#define IS_SET(flag, bit)                ((flag) & (bit))
#define SET_BIT(var, bit)                ((var) |= (bit))
#define REMOVE_BIT(var, bit)             ((var) &= ~(bit))
#define TOGGLE_BIT(var, bit)             ((var) = (var) ^ (bit))

#define CELL_SIZE		50
#define SECTREE_SIZE	6400
#define SECMAP_SIZE		(SECTREE_SIZE / CELL_SIZE) * (SECTREE_SIZE / CELL_SIZE)						// 16384
#define SECMAP_SIZE2	sizeof(uint32_t) * SECMAP_SIZE													// 65536
#define SECMAP_SIZE3	SECMAP_SIZE/sizeof(uint32_t)													// 4096

#define ATTR_SIZE		CTerrainImpl::ATTRMAP_XSIZE * CTerrainImpl::ATTRMAP_YSIZE * sizeof(uint8_t)	// 65536
#define ATTR_HEAD		sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t)									// 6
#define SECATTR_SIZE	TERRAIN_SIZE/2																// 64
#define SECATTR_SIZE2	TERRAIN_SIZE*2																// 256
#define SECATTR_SIZE3	TERRAIN_SIZE*3																// 384
#define SECATTR_SIZE4	TERRAIN_SIZE*4																// 512

void GetLzoAttr(std::vector<std::vector<std::vector<uint32_t> > > & attrMaps, uint32_t iWidth, uint32_t iHeight, FILE* wfp)
{
	assert(SECMAP_SIZE2==65536); // ==ATTR_SIZE
	assert(SECMAP_SIZE==16384); // ==ATTR_SIZE/4
	assert(TERRAIN_SIZE==128);
	uint32_t uiSize;
	uint32_t uiDestSize;

	size_t maxMemSize = LZOManager::instance().GetMaxCompressedSize(SECMAP_SIZE2);
	Tracef("allocated %u\n", static_cast<uint32_t>(maxMemSize));

	uint8_t * abComp = M2_NEW uint8_t[maxMemSize];
	uint32_t * attr = M2_NEW uint32_t[SECMAP_SIZE];

	// uint8_t wReg;
	int32_t sCountX = iWidth/4;
	int32_t sCountY = iHeight/4;
	Tracef("sCountX %d sCountY %d iWidth %d iHeight %d\n", sCountX, sCountY, iWidth, iHeight);

	for (uint32_t ry = 0; ry < iHeight; ry++)
	{
	for (uint32_t rx = 0; rx < iWidth; rx++)
	{
		// generate server_attr sector by attr.atr data
		uiSize = SECMAP_SIZE2;
		memset(attr, 0, SECMAP_SIZE2);

		std::copy(attrMaps[ry][rx].begin(), attrMaps[ry][rx].end(), attr);
		// compress and finalize server_attr sector
		Tracef("[%d x %d] orig_size %d\n", rx, ry, uiSize);
		LZOManager::instance().Compress((uint8_t *) attr, uiSize, abComp, (lzo_uint*)&uiDestSize);
		Tracef("[%d x %d] new_size %d\n", rx, ry, uiDestSize);

		fwrite(&uiDestSize, sizeof(int32_t), 1, wfp);
		fwrite(abComp, uiDestSize, 1, wfp);
	}
	}
	// delete stuff
	M2_DELETE_ARRAY(attr);
	M2_DELETE_ARRAY(abComp);
}

sAttrTypeRet_t __attrProcess(int16_t sCountX, int16_t sCountY, std::string csMapName)
{
	LZOManager pLZOManager;

	sAttrTypeRet_t retValue;
	retValue.szErrFile=csMapName+"\\server_attr";
	retValue.iType=RET_OK;
	// attr.atr details
	const uint16_t mapver = 2634;
	const int32_t iAttrSize = ATTR_SIZE;

	// server_attr details
	int32_t iWidth = sCountX*4;
	int32_t iHeight = sCountY*4;
	std::vector<std::vector<std::vector<uint32_t> > > attrMaps(iHeight, std::vector<std::vector<uint32_t> >(iWidth, std::vector<uint32_t>(SECMAP_SIZE, 0)));

	Tracef("ServerAttr size %d %d (Attr*4)\n", iWidth, iHeight);
	// assert(iWidth==4 && iHeight==4);

	FILE * wfp = fopen(retValue.szErrFile.c_str(), "wb");
	if (!wfp)
	{
		retValue.iType=SERVERATTR_NOT_WRITABLE;
		return retValue;
	}
	fwrite(&iWidth, sizeof(int32_t), 1, wfp);
	fwrite(&iHeight, sizeof(int32_t), 1, wfp);

	for (int16_t sX = 0; sX < sCountX; ++sX)
	{
		for (int16_t sY = 0; sY < sCountY; ++sY)
		{
			uint32_t wID = (uint32_t) (sX) * 1000L + (uint32_t)(sY);
			char szAttrFileName[MAX_PATH + 1];
			_snprintf(szAttrFileName, MAX_PATH, "%s\\%06u\\attr.atr", csMapName.c_str(), wID);

			char szBufAttr[iAttrSize+6];
			FILE * fp = fopen(szAttrFileName, "rb");
			if (!fp)
			{
				retValue.szErrFile=szAttrFileName;
				retValue.iType=ATTR_NOT_READABLE;
				return retValue;
			}
			fseek(fp, 0L, SEEK_END);
			size_t sz = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
			Tracef("[%s] size %u\n", szAttrFileName, static_cast<uint32_t>(sz));
			// process attr.atr
			fread(szBufAttr, iAttrSize+6, 1, fp);
			fclose(fp);

			ilInit();
			ilEnable(IL_FILE_OVERWRITE);

			ILuint image;
			ilGenImages(1, &image);
			ilBindImage(image);

			ilTexImage(256, 256, 1, 1, IL_RGBA, IL_BYTE, NULL);

			ILubyte * pRawData = ilGetData();
			memcpy(pRawData, szBufAttr+6, ilGetInteger(IL_IMAGE_WIDTH)*ilGetInteger(IL_IMAGE_HEIGHT));

			ilSetData(pRawData);
			iluScale(512, 512, 1);

			ilConvertImage(IL_RGBA, IL_INT); // do not try to save the image after this via DevIL
			pRawData = ilGetData(); // do not try to use ilSetData (bpp>1; crash)
			{
				int32_t ilHeight = ilGetInteger(IL_IMAGE_HEIGHT);
				int32_t ilWidth = ilGetInteger(IL_IMAGE_WIDTH);
				int32_t ilBpp = 4;//ilGetInteger(IL_IMAGE_BPP);
				uint32_t* pdwRawData = M2_NEW uint32_t[ilWidth*ilHeight];
				memcpy(pdwRawData, pRawData, ilWidth*ilHeight*ilBpp);
				Tracef("RawAttrData width=%d, height=%d, bpp=%d\n", ilWidth, ilHeight, ilBpp);
				Tracef("allocated %u\n", ilWidth*ilHeight*ilBpp);
				uint32_t* pdwRawData2 = pdwRawData;
				bool found = false;
				for (int32_t h = 0; h < ilHeight; ++h)
				{
					for (int32_t w = 0; w < ilWidth; ++w)
					{
						*pdwRawData2 >>= 24;
						if (globals::dft::SERVERATTR_REMOVE_WEIRD_FLAGS && IS_SET(*pdwRawData2, 0xFFFFFFF8))
						{
							if (!found)
							{
								TraceError("!!! wrong attr flags in height %d width %d flag 0x%08x", h, w, *pdwRawData2);
								found = true;
							}
							REMOVE_BIT(*pdwRawData2, 0xFFFFFFF8);
						}
						pdwRawData2++;
					}
				}
				for (uint32_t rx = 0; rx < sizeof(uint32_t); rx++)
				{
				for (uint32_t ry = 0; ry < sizeof(uint32_t); ry++)
				{
					for (int32_t y = 0; y < TERRAIN_SIZE; ++y)
					{
						for (int32_t x = 0; x < TERRAIN_SIZE; ++x)
						{
							attrMaps[(rx+sY*4)][(ry+sX*4)][x+(y*TERRAIN_SIZE)] = pdwRawData[(x+ry*TERRAIN_SIZE)+(y*SECATTR_SIZE4+rx*ATTR_SIZE)];
						}
					}
				}
				}
				// ilSetData(pdwRawData); // crash
				if (0)
				{
					char szAttrFileNameRaw[MAX_PATH + 1];
					_snprintf(szAttrFileNameRaw, MAX_PATH, "%s.raw", szAttrFileName);

					FILE * wwfp = fopen(szAttrFileNameRaw, "wb");
					assert(wwfp);
					Tracef("RawAttrData width=%d, height=%d, bpp=%d\n", ilWidth, ilHeight, ilBpp);
					fwrite(pdwRawData, ilWidth*ilHeight*ilBpp, 1, wwfp);
					fclose(wwfp);
				}
				M2_DELETE_ARRAY(pdwRawData);
			}

			ilDeleteImages(1, &image);

			ILenum Error;
			while ((Error = ilGetError()) != IL_NO_ERROR)
			TraceError("DevIL: %d: %s", Error, iluErrorString(Error));
		}
	}
	GetLzoAttr(attrMaps, iWidth, iHeight, wfp);
	fclose(wfp);
	return retValue;
}
}

void CMapFilePage::OnButtonInitShadowMap() // generate server attr
{
	CWorldEditorApp * pApplication = (CWorldEditorApp *)AfxGetApp();
	CMapManagerAccessor * pMapManagerAccessor = pApplication->GetMapManagerAccessor();
	if (!pMapManagerAccessor->IsMapReady())
	{
		LogBox("Load a map before dumping a server_attr file.");
		return;
	}
	int32_t iRet = ::MessageBox(NULL, "This will generate/overwrite a server_attr inside the map folder.\nNote: The file will be created using all the map's attr.atr and not from memory.\nSo save the map before proceeding!", "Info", MB_YESNO);
	if (6 != iRet)
		return;

	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView *)pFrame->GetActiveView();
	pView->Lock();

	{
		CMapOutdoorAccessor * pMapOutdoor = pMapManagerAccessor->GetMapOutdoorPtr();
		// CMapOutdoor & rMapOutdoor = pMapManagerAccessor->GetMapOutdoorRef();

		Tracef("MapName %s\n", pMapOutdoor->GetName().c_str());

		int16_t sCountX, sCountY;
		pMapOutdoor->GetTerrainCount(&sCountX, &sCountY);
		Tracef("AttrMap size %d %d\n", sCountX, sCountY);

		atr::sAttrTypeRet_t retValue = atr::__attrProcess(sCountX, sCountY, pMapOutdoor->GetName());

		switch (retValue.iType)
		{
			case atr::RET_OK:
				LogBoxf("server_attr successfully saved in [%s].", retValue.szErrFile.c_str());
				break;
			case atr::ATTR_NOT_READABLE:
				LogBoxf("[%s] is not readable or missing.", retValue.szErrFile.c_str());
				break;
			case atr::SERVERATTR_NOT_WRITABLE:
				LogBoxf("[%s] is not writable.", retValue.szErrFile.c_str());
				break;
		}
	}
	pView->Unlock();
}

