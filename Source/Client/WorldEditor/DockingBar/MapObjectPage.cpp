// MapObjectPage.cpp : implementation file
//

#include "stdafx.h"
#include "..\WorldEditor.h"
#include "MapObjectPage.h"
#include "../../../Client/GameLib/Property.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Test Code
#include "../MainFrm.h"
#include "../WorldEditorDoc.h"
#include "../Dialog/MapObjectDirectory.h"
#include "../Dialog/MapObjectProperty.h"
#include "../Dialog/InputNumber.h"
#include "../DataCtrl/MapAccessorOutdoor.h"


/////////////////////////////////////////////////////////////////////////////
// CMapObjectPage dialog


CMapObjectPage::CMapObjectPage(CWnd* pParent /*=NULL*/)
	: CPageCtrl(CMapObjectPage::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMapObjectPage)
	//}}AFX_DATA_INIT
}

CMapObjectPage::~CMapObjectPage()
{
}

void CMapObjectPage::DoDataExchange(CDataExchange* pDX)
{
	CPageCtrl::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapObjectPage)
	DDX_Control(pDX, IDD_MAP_PORTAL_NUMBER_LIST, m_ctrlPortalNumber);
	DDX_Control(pDX, IDC_STATIC_SELECTED_OBJECT_INFO, m_StaticSelectedObjectInfo);
	DDX_Control(pDX, IDC_MAP_OBJECT_SCALE, m_ctrlObjectScale);
	DDX_Control(pDX, IDC_MAP_OBJECT_YAW, m_ctrlCursorYaw);
	DDX_Control(pDX, IDC_MAP_OBJECT_ROLL, m_ctrlCursorRoll);
	DDX_Control(pDX, IDC_MAP_OBJECT_PITCH, m_ctrlCursorPitch);
	DDX_Control(pDX, IDC_MAP_OBJECT_HEIGHT, m_ctrlObjectHeight);
	DDX_Control(pDX, IDC_MAP_OBJECT_GRID_DISTANCE, m_ctrlGridDistance);
	DDX_Control(pDX, IDC_MAP_OBJECT_TREE, m_ctrlPropertyTree);
	//}}AFX_DATA_MAP

	m_ctrlCursorRoll.SetRangeMin(0);
	m_ctrlCursorRoll.SetRangeMax(CArea::ROTATION_STEP_COUNT);
	m_ctrlCursorYaw.SetRangeMin(0);
	m_ctrlCursorYaw.SetRangeMax(CArea::YAW_STEP_COUNT);
	m_ctrlCursorPitch.SetRangeMin(0);
	m_ctrlCursorPitch.SetRangeMax(CArea::PITCH_STEP_COUNT);
	m_ctrlGridDistance.SetRangeMin(1);
	m_ctrlGridDistance.SetRangeMax(50);
	for (int32_t i = 10; i < 50; i+=10)
	{
		m_ctrlGridDistance.SetTic(i);
	}
	m_ctrlObjectHeight.SetRangeMin(0);
	m_ctrlObjectHeight.SetRangeMax(globals::dft::OBJECT_HEIGHT_SLIDER_MAX*2);
	m_ctrlObjectHeight.SetTic(globals::dft::OBJECT_HEIGHT_SLIDER_MAX);
	m_ctrlObjectScale.SetRangeMin(0);
	m_ctrlObjectScale.SetRangeMax(10000);
}


BEGIN_MESSAGE_MAP(CMapObjectPage, CPageCtrl)
	//{{AFX_MSG_MAP(CMapObjectPage)
	ON_BN_CLICKED(IDD_MAP_CREATE_PROPERTY, OnCreateProperty)
	ON_BN_CLICKED(IDD_MAP_CREATE_DIRECTORY, OnCreateDirectory)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDD_MAP_OBJECT_RANDOM_ROTATION, OnCheckRandomRotation)
	ON_BN_CLICKED(IDD_MAP_OBJECT_GRID_MODE, OnCheckGridModeEnable)
	ON_NOTIFY(NM_DBLCLK, IDC_MAP_OBJECT_TREE, OnDblclkMapObjectTree)
	ON_NOTIFY(TVN_SELCHANGED, IDC_MAP_OBJECT_TREE, OnSelchangedMapObjectTree)
	ON_BN_CLICKED(IDD_MAP_DELETE, OnDelete)
	ON_BN_CLICKED(IDD_MAP_UNSELECT, OnUnselect)
	ON_BN_CLICKED(IDD_MAP_OBJECT_INSERT_PORTAL_NUMBER, OnInsertPortalNumber)
	ON_BN_CLICKED(IDD_MAP_OBJECT_DELETE_PORTAL_NUMBER, OnDeletePortalNumber)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyTreeControler normal functions

void CPropertyTreeControler::RegisterPath(HTREEITEM hTreeItem, const std::string & c_rstrPathName)
{
	m_PathMap.insert(TPathMap::value_type(hTreeItem, c_rstrPathName));
}
void CPropertyTreeControler::DeletePath(HTREEITEM hTreeItem)
{
	TPathIterator itor = m_PathMap.find(hTreeItem);

	if (m_PathMap.end() == itor)
		return;

	m_PathMap.erase(itor);
}
bool CPropertyTreeControler::GetPath(HTREEITEM hTreeItem, const char ** c_ppszPathName)
{
	TPathIterator itor = m_PathMap.find(hTreeItem);

	if (m_PathMap.end() == itor)
		return false;

	*c_ppszPathName = (itor->second).c_str();

	return true;
}
CPropertyTreeControler::CPropertyTreeControler()
{
	m_PathMap.insert(TPathMap::value_type(TVI_ROOT, "Property\\"));
}
CPropertyTreeControler::~CPropertyTreeControler()
{
}

const char * GetNumberText(int32_t iNumber)
{
	static char s_szText[16+1];
	_snprintf(s_szText, 16, "%d", iNumber);

	return s_szText;
}

/////////////////////////////////////////////////////////////////////////////
// CMapEditPage normal functions

BOOL CMapObjectPage::Create(CWnd * pParent)
{
	if (!CPageCtrl::Create(CMapObjectPage::IDD, pParent))
		return FALSE;

	return TRUE;
}

void CMapObjectPage::Initialize()
{
	// Make Image List
	CreateHighColorImageList(IDB_MAP_OBJECT_TREE_ITEM_IMAGE_LIST, &m_TreeImageList);
	m_ctrlPropertyTree.SetImageList(&m_TreeImageList, TVSIL_NORMAL);

	LoadProperty();

	// Setting Brush
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();

	int32_t iScale = 100;
	int32_t iDensity = 1;
	int32_t iRandom = 0;
	int32_t iCursorRotation = 0;
	int32_t iGridDistance = 10;
	float fObjectHeight = 0.0f;
	uint32_t dwObjectScale = 1000;
	pSceneMap->SetObjectBrushScale(iScale);
	pSceneMap->SetObjectBrushDensity(iDensity);
	pSceneMap->SetObjectBrushRandom(iRandom);
	pSceneMap->SetObjectHeight(0.0f);
	pSceneMap->SetRandomRotation(IsDlgButtonChecked(IDD_MAP_OBJECT_RANDOM_ROTATION) == 1 ? true : false);
	pSceneMap->SetGridMode(IsDlgButtonChecked(IDD_MAP_OBJECT_GRID_MODE) == 1 ? true : false);
	pSceneMap->SetGridDistance(float(m_ctrlGridDistance.GetPos() * 10));
	pSceneMap->SetCursorScale(dwObjectScale);
	m_ctrlCursorRoll.SetPos(CArea::ROTATION_STEP_COUNT/2);
	m_ctrlCursorYaw.SetPos(CArea::YAW_STEP_COUNT/2);
	m_ctrlCursorPitch.SetPos(CArea::PITCH_STEP_COUNT/2);
	m_ctrlGridDistance.SetPos(iGridDistance);
	m_ctrlObjectHeight.SetPos(int32_t(fObjectHeight) + globals::dft::OBJECT_HEIGHT_SLIDER_MAX);
	m_ctrlObjectScale.SetPos(dwObjectScale);
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_GRID_DISTANCE_PRINT, float(m_ctrlGridDistance.GetPos()/10.0f));
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_HEIGHT_PRINT, float(m_ctrlObjectHeight.GetPos() - globals::dft::OBJECT_HEIGHT_SLIDER_MAX)/100.0f);
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_SCALE_PRINT, float(m_ctrlObjectScale.GetPos())/100.0f);
}

void CMapObjectPage::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();

	pSceneMap->SetCursorYaw(SliderValueToCursorRotation(m_ctrlCursorYaw.GetPos(),CArea::YAW_STEP_COUNT, CArea::YAW_STEP_AMOUNT));
	pSceneMap->SetCursorPitch(SliderValueToCursorRotation(m_ctrlCursorPitch.GetPos(),CArea::PITCH_STEP_COUNT, CArea::PITCH_STEP_AMOUNT));
	pSceneMap->SetCursorRoll(SliderValueToCursorRotation(m_ctrlCursorRoll.GetPos(),CArea::ROTATION_STEP_COUNT, CArea::ROTATION_STEP_AMOUNT));
	pSceneMap->SetGridDistance(float(m_ctrlGridDistance.GetPos() * 10));
	pSceneMap->SetObjectHeight(float(m_ctrlObjectHeight.GetPos() - globals::dft::OBJECT_HEIGHT_SLIDER_MAX));
	pSceneMap->SetCursorScale(m_ctrlObjectScale.GetPos());
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_GRID_DISTANCE_PRINT, float(m_ctrlGridDistance.GetPos()/10.0f));
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_HEIGHT_PRINT, float(m_ctrlObjectHeight.GetPos() - globals::dft::OBJECT_HEIGHT_SLIDER_MAX)/100.0f);
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_SCALE_PRINT, float(m_ctrlObjectScale.GetPos())/100.0f);

	CPageCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMapObjectPage::LoadProperty()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();

	pMapManagerAccessor->LoadProperty(&m_ctrlPropertyTree);
}

void CMapObjectPage::UpdateUI()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	CMapOutdoorAccessor * pOutdoor = (CMapOutdoorAccessor *) &pMapManagerAccessor->GetMapOutdoorRef();

	m_ctrlCursorYaw.SetPos(CursorRotationToSliderValue(pSceneMap->GetCursorYaw(),CArea::YAW_STEP_COUNT,CArea::YAW_STEP_AMOUNT));
	m_ctrlCursorPitch.SetPos(CursorRotationToSliderValue(pSceneMap->GetCursorPitch(),CArea::PITCH_STEP_COUNT,CArea::PITCH_STEP_AMOUNT));
	m_ctrlCursorRoll.SetPos(CursorRotationToSliderValue(pSceneMap->GetCursorRoll(),CArea::ROTATION_STEP_COUNT,CArea::ROTATION_STEP_AMOUNT));
	m_ctrlObjectHeight.SetPos(int32_t(pSceneMap->GetCursorObjectHeight()) + globals::dft::OBJECT_HEIGHT_SLIDER_MAX);
	m_ctrlObjectScale.SetPos(pSceneMap->GetCursorScale());
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_GRID_DISTANCE_PRINT, float(m_ctrlGridDistance.GetPos()/10.0f));
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_HEIGHT_PRINT, float(m_ctrlObjectHeight.GetPos() - globals::dft::OBJECT_HEIGHT_SLIDER_MAX)/100.0f);
	SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_SCALE_PRINT, float(m_ctrlObjectScale.GetPos())/100.0f);

	__RefreshSelectedObjectInfo();
	__RefreshSelectedObjectProperty();

	// Cursor
	const HTREEITEM c_hTreeItem = m_ctrlPropertyTree.GetSelectedItem();
	if (c_hTreeItem)
	{
		uint32_t dwCRC = m_ctrlPropertyTree.GetItemData(c_hTreeItem);
		pSceneMap->ChangeCursor(dwCRC);
	}
	else
	{
		pSceneMap->ClearCursor();
	}
}

void CMapObjectPage::__RefreshSelectedObjectInfo()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	CMapOutdoorAccessor * pOutdoor = (CMapOutdoorAccessor *) &pMapManagerAccessor->GetMapOutdoorRef();

	// Selected Object Name
	const char * c_szSelectedObjectName = pOutdoor->GetSelectedObjectName();
	m_StaticSelectedObjectInfo.SetWindowText(c_szSelectedObjectName);

	// Selected Object Portal
	m_ctrlPortalNumber.ResetContent();
	const CMapOutdoorAccessor::TPortalNumberVector & c_rkVec_iPortalNumber = pOutdoor->GetSelectedObjectPortalVectorRef();
	for (uint32_t i = 0; i != c_rkVec_iPortalNumber.size(); ++i)
	{
		int32_t iNum = c_rkVec_iPortalNumber[i];
		m_ctrlPortalNumber.InsertString(i, _getf("Portal Number : %d", iNum));
	}
}

void CMapObjectPage::__RefreshSelectedObjectProperty()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	CMapOutdoorAccessor * pOutdoor = (CMapOutdoorAccessor *) &pMapManagerAccessor->GetMapOutdoorRef();

	CAreaAccessor * pAreaAccessor;
	if (!pOutdoor->GetAreaAccessor(4, &pAreaAccessor))
		return;

	const CArea::TObjectData* pData = pAreaAccessor->GetLastSelectedObjectData();

	if (pData)
	{
		// Rotation Values
		m_ctrlCursorYaw.SetPos(CursorRotationToSliderValue(pData->m_fYaw, CArea::YAW_STEP_COUNT, CArea::YAW_STEP_AMOUNT));
		m_ctrlCursorPitch.SetPos(CursorRotationToSliderValue(pData->m_fPitch, CArea::PITCH_STEP_COUNT, CArea::PITCH_STEP_AMOUNT));
		m_ctrlCursorRoll.SetPos(CursorRotationToSliderValue(pData->m_fRoll, CArea::ROTATION_STEP_COUNT, CArea::ROTATION_STEP_AMOUNT));

		// Height Bias - Slider Ctrl, Text Value
		m_ctrlObjectHeight.SetPos(int32_t(pData->m_fHeightBias + globals::dft::OBJECT_HEIGHT_SLIDER_MAX));
		SetDialogFloatText(GetSafeHwnd(), IDC_MAP_OBJECT_HEIGHT_PRINT, float(m_ctrlObjectHeight.GetPos() - globals::dft::OBJECT_HEIGHT_SLIDER_MAX)/100.0f);
	}
}


bool CMapObjectPage::GetCurrentParentItem(HTREEITEM * pTreeItem, const char ** c_ppszPathName)
{
	HTREEITEM hTreeItem = m_ctrlPropertyTree.GetSelectedItem();

	if (!hTreeItem) // @fixme124
	{
		*pTreeItem = TVI_ROOT;
		*c_ppszPathName = "property";
		return true;
	}

	// when client select child node which is not directory,
	// change parent root to directory
	while (!m_ctrlPropertyTree.GetPath(hTreeItem, c_ppszPathName))
	{
		hTreeItem = m_ctrlPropertyTree.GetParentItem(hTreeItem);
		if (!hTreeItem)
			hTreeItem = TVI_ROOT;
	}

	*pTreeItem = hTreeItem;

	return true;
}

int32_t CMapObjectPage::SliderValueToCursorRotation(int32_t iStep, int32_t iCount, int32_t iAmount)
{
	if (iStep > iCount/2)
	{
		return (iStep - iCount/2) * iAmount;
	}
	else if (iStep < iCount/2)
	{
		return (iCount/2 + iStep) * iAmount;
	}

	return 0;
}

int32_t CMapObjectPage::CursorRotationToSliderValue(int32_t iRotation, int32_t iCount, int32_t iAmount)
{
	int32_t iStep = iRotation / iAmount;
	iStep = (iStep + iCount/2) % iCount;

	return iStep;
}

/////////////////////////////////////////////////////////////////////////////
// CMapObjectPage message handlers
BOOL CMapObjectPage::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch(LOWORD(wParam))
	{
		case IDC_MAP_OBJECT_TREE:
			switch (LOWORD(lParam))
			{
				case 63528:
				case 63348:
				case 63564:
					{
					}
					break;
			}

			break;
	}
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CMapObjectPage::OnCreateDirectory()
{
	const char * c_szPathName;
	HTREEITEM hParentItem;

	if (!GetCurrentParentItem(&hParentItem, &c_szPathName))
		return;

	if (TVI_ROOT != hParentItem)
		m_ctrlPropertyTree.SelectItem(hParentItem);

	CMapObjectDirectory CreateDirectory;

	if (CreateDirectory.DoModal())
	{
		std::string strPathName;
		strPathName = c_szPathName;
		strPathName += "/";
		strPathName += CreateDirectory.GetDirectoryName();

		::CreateDirectory(strPathName.c_str(), NULL);

		char szDirectoryName[64+1];
		_snprintf(szDirectoryName, 64, "[ %s ]", CreateDirectory.GetDirectoryName());
		HTREEITEM hTreeItem = m_ctrlPropertyTree.InsertItem(szDirectoryName, 0, 1, hParentItem, TVI_SORT);
		m_ctrlPropertyTree.RegisterPath(hTreeItem, strPathName.c_str());
	}
}

void CMapObjectPage::OnCreateProperty()
{
	HTREEITEM hTreeItem;
	const char * c_szPathName;
	if (!GetCurrentParentItem(&hTreeItem, &c_szPathName))
		return;

	if (TVI_ROOT != hTreeItem)
		m_ctrlPropertyTree.SelectItem(hTreeItem);

	CMapObjectProperty CreateProperty;
	CreateProperty.SetPath(c_szPathName);

	if (CreateProperty.DoModal())
	{
		int32_t iType = CreateProperty.GetPropertyType();
		HTREEITEM hNewItem = m_ctrlPropertyTree.InsertItem(CreateProperty.GetPropertyName(), iType+1, iType+1, hTreeItem, TVI_SORT);
		m_ctrlPropertyTree.SetItemData(hNewItem, CreateProperty.GetPropertyCRC32());
	}
}

void CMapObjectPage::OnDelete()
{
	HTREEITEM hTreeItem = m_ctrlPropertyTree.GetSelectedItem();

	if (!hTreeItem)
		return;

	const char * pszPath;

	if (m_ctrlPropertyTree.GetPath(hTreeItem, &pszPath))
	{
		if (!RemoveDirectory(pszPath))
		{
			LogBox("The file or folder exists in child", "Error", GetSafeHwnd());
			return;
		}

		m_ctrlPropertyTree.DeletePath(hTreeItem);
		m_ctrlPropertyTree.DeleteItem(hTreeItem);
	}
	else
	{
		HTREEITEM hParentItem;
		const char * c_szPathName;

		if (!GetCurrentParentItem(&hParentItem, &c_szPathName))
			return;

		if (IDYES != MessageBox("Do you really want to clear?", NULL, MB_YESNO))
			return;

		std::string stFileName;
		stFileName = c_szPathName;
		stFileName += "/";
		stFileName += m_ctrlPropertyTree.GetItemText(hTreeItem);

		if (CPropertyManager::Instance().Erase(stFileName.c_str()))
		{
			m_ctrlPropertyTree.DeletePath(hTreeItem);
			m_ctrlPropertyTree.DeleteItem(hTreeItem);
		}
	}
}

void CMapObjectPage::OnUnselect()
{
	m_ctrlPropertyTree.SelectItem(NULL);

	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pScene = pDocument->GetSceneMap();
	pScene->ChangeCursor(0);
}

void CMapObjectPage::OnCheckRandomRotation()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();

	pSceneMap->SetRandomRotation(IsDlgButtonChecked(IDD_MAP_OBJECT_RANDOM_ROTATION) == 1 ? true : false);
}

void CMapObjectPage::OnCheckGridModeEnable()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pSceneMap = pDocument->GetSceneMap();

	pSceneMap->SetGridMode(IsDlgButtonChecked(IDD_MAP_OBJECT_GRID_MODE) == 1 ? true : false);
}

void CMapObjectPage::OnDblclkMapObjectTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	const HTREEITEM c_hTreeItem = m_ctrlPropertyTree.GetSelectedItem();
	uint32_t dwCRC = m_ctrlPropertyTree.GetItemData(c_hTreeItem);

	CProperty * pProperty;
	if (!CPropertyManager::Instance().Get(dwCRC, &pProperty))
		return;

	printf("EDIT START %s\n", pProperty->GetFileName());

	const HTREEITEM c_hParentItem = m_ctrlPropertyTree.GetParentItem(c_hTreeItem);
	const char * c_szPath;

	if (!m_ctrlPropertyTree.GetPath(c_hParentItem, &c_szPath))
		return;

	CMapObjectProperty ObjectProperty;

	ObjectProperty.SetPath(c_szPath);
	ObjectProperty.SetData(pProperty);

	if (ObjectProperty.DoModal())
	{
		CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
		CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
		CSceneMap * pScene = pDocument->GetSceneMap();

		pScene->RefreshCursor();
		pScene->RefreshArea();
	}
}

void CMapObjectPage::OnSelchangedMapObjectTree(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	const HTREEITEM c_hTreeItem = pNMTreeView->itemNew.hItem;

	if (!c_hTreeItem)
		return;

	uint32_t dwCRC = m_ctrlPropertyTree.GetItemData(c_hTreeItem);

	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CSceneMap * pScene = pDocument->GetSceneMap();

	pScene->ChangeCursor(dwCRC);

	*pResult = 0;
}

void CMapObjectPage::OnInsertPortalNumber()
{
	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	CMapOutdoorAccessor * pOutdoor = (CMapOutdoorAccessor *) &pMapManagerAccessor->GetMapOutdoorRef();

	if (!pMapManagerAccessor->IsSelected())
	{
		LogBox("No object has been selected");
		return;
	}

	CInputNumber InputNumber;
	if (InputNumber.DoModal())
	{
		int32_t iNum = InputNumber.GetNumber();
		pMapManagerAccessor->SetSelectedObjectPortalNumber(iNum);

		__RefreshSelectedObjectInfo();
	}
}

void CMapObjectPage::OnDeletePortalNumber()
{
	uint32_t dwIndex = m_ctrlPortalNumber.GetCurSel();

	CMainFrame * pFrame = (CMainFrame *)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc *)pFrame->GetActiveDocument();
	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	CMapOutdoorAccessor * pOutdoor = (CMapOutdoorAccessor *) &pMapManagerAccessor->GetMapOutdoorRef();
	const CMapOutdoorAccessor::TPortalNumberVector & c_rkVec_iPortalNumber = pOutdoor->GetSelectedObjectPortalVectorRef();

	if (dwIndex >= c_rkVec_iPortalNumber.size())
		return;

	pMapManagerAccessor->DelSelectedObjectPortalNumber(c_rkVec_iPortalNumber[dwIndex]);
#ifdef CWE_AREA_ACCESSOR_MISSING_REFRESH
	__RefreshSelectedObjectInfo();
#endif
}
