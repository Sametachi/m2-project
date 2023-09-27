// WorldEditor.h : main header file for the WORLDEDITOR application
//

#if !defined(AFX_WORLDEDITOR_H__23A6089B_4D11_4492_BA3F_A18389CD231D__INCLUDED_)
#define AFX_WORLDEDITOR_H__23A6089B_4D11_4492_BA3F_A18389CD231D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#define ENABLE_PYTHON_SYSTEM
#include "../../../Libraries/eterlib/GrpDevice.h"
#include "../../../Libraries/eterPack/EterPackManager.h"
#include "../../../Libraries/milesLib/SoundManager.h"
#include "../../../Libraries/EffectLib/EffectManager.h"
#include "../../../Libraries/gamelib/FlyingObjectManager.h"
#include "../../../Libraries/EterLib/GrpLightManager.h"
#include "../../../Libraries/gamelib/GameEventManager.h"
#include "../../../Libraries/EterLib/CullingManager.h"

//------------------------//
// Temporary Code
#include "./DataCtrl/ObjectData.h"
#include "./DataCtrl/MapManagerAccessor.h"
//------------------------//

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp:
// See WorldEditor.cpp for the implementation of this class
//
class CMainFrame;
class CWorldEditorDoc;
class CWorldEditorView;

class CWorldEditorApp : public CWinApp
{
public:
	CWorldEditorApp();
	~CWorldEditorApp();

public:
	CObjectData * GetObjectData();
	CEffectAccessor * GetEffectAccessor();
	CMapManagerAccessor * GetMapManagerAccessor();
	CSceneObject * GetSceneObject();
	CSceneEffect * GetSceneEffect();
	CSceneMap * GetSceneMap();
	CSceneFly * GetSceneFly();

	CGraphicDevice & GetGraphicDevice() { return m_GraphicDevice; }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorApp)
	public:
	virtual BOOL InitInstance();
	virtual int32_t ExitInstance();
	virtual BOOL OnIdle(int32_t lCount);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CWorldEditorApp)
	afx_msg void OnAppAbout();
	afx_msg void OnAppExit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CTimer m_Timer;
	CPythonResource m_PythonResource;

	CEterPackManager m_EterPackManager;

	CGraphicDevice m_GraphicDevice;
	CSoundManager m_SoundManager;
	CEffectManager m_EffectManager;
	CFlyingManager m_FlyingManager;
	CLightManager m_LightManager;
	CGameEventManager m_GameEventManager;

	CCullingManager m_CullingManager;
	CLZO m_lzo;

public:
	CMainFrame* GetFrame();
	CWorldEditorDoc* GetDocument();
	CWorldEditorView* GetView();
};

/////////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_PYTHON_SYSTEM
enum eWeSceneType {
	SCENE_MAP,
	SCENE_OBJECT,
	SCENE_EFFECT,
	SCENE_FLY,
	SCENE_MAX,
};

inline int32_t GetSceneTypeFromID(int32_t id)
{
	switch (id) {
	case ID_VIEW_MAP:
		return SCENE_MAP;
	case ID_VIEW_OBJECT:
		return SCENE_OBJECT;
	case ID_VIEW_EFFECT:
		return SCENE_EFFECT;
	case ID_VIEW_FLY:
		return SCENE_FLY;
	default:
		return SCENE_MAX;
	}
}

extern void initWorldEditor();
extern void defWorldEditor();
extern void RegisterWorldEditorApp(CWorldEditorApp*);
extern CWorldEditorApp* GetWorldEditorApp();
#endif

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WORLDEDITOR_H__23A6089B_4D11_4492_BA3F_A18389CD231D__INCLUDED_)
