// SimpleUI.h : header file
//
//


#if !defined(AFX_SIMPLEUI_H__7513B080_76E5_11D4_97FE_004F4C02CAF7__INCLUDED_)
#define AFX_SIMPLEUI_H__7513B080_76E5_11D4_97FE_004F4C02CAF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GTColorDialogDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CColorBox window

class CColorBox : public CStatic
{
// Construction
public:
	CColorBox();
	virtual ~CColorBox();

// Attributes
public:

// Operations
public:
	class CColorObserver : public CGTColorDialogDlg::IObserver
	{
		public:
			void SendColor(COLORREF & rColor)
			{
				m_pOwner->RecvColor(rColor);
			}
			void SetOwner(CColorBox * pOwner)
			{
				m_pOwner = pOwner;
			}
			CColorBox * m_pOwner;
	};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL Create(CWnd * pParent);
	void SetColor(const COLORREF & rColor);
	void GetColor(COLORREF * pColor);
	void Update();
	void (*pfnCallBack)();

	COLORREF m_Color;

	CColorObserver m_Observer;
	CGTColorDialogDlg * m_dlgLight;

	void RecvColor(COLORREF & rColor);

// Generated message map functions
protected:

	//{{AFX_MSG(CColorBox)
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nHitTest, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CStatusProgress window

class CStatusProgress : public CProgressCtrl
{
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStatusProgress)
	//}}AFX_VIRTUAL

// Implementation
public:
	void SetText(CString s);
	void SetCallbackDelay(int32_t nDelay);
	static void SetCallbackPos(int32_t nPos);
	void Destroy();
	void Create();

	~CStatusProgress()
	{
		if (IsWindow(m_hWnd))
		{
			SetText("");
			Destroy();
		}
	}

	// Generated message map functions
protected:
	CWaitCursor *m_pWait;

	//{{AFX_MSG(CStatusProgress)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif