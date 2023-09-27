// ToolBar256.h : header file
//
//

#if !defined(AFX_TOOLBAR256_H__BC9421B2_59EE_11D4_AFB2_004F4C02CAF7__INCLUDED_)
#define AFX_TOOLBAR256_H__BC9421B2_59EE_11D4_AFB2_004F4C02CAF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CToolBar256 window

class CToolBar256 : public CToolBar
{
// Construction
public:
	BOOL LoadBitmap(LPCTSTR lpszResourceName);
	BOOL LoadBitmap(int32_t nResourceID);

	CToolBar256();
	virtual ~CToolBar256();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolBar256)
	//}}AFX_VIRTUAL


protected:
	BOOL AddReplaceBitmap(HBITMAP hbmImageWell);
	int32_t m_nBitmapID;
	HBITMAP GetBitmap256(HINSTANCE hInst, HRSRC hRsrc, BOOL bMono);

// Generated message map functions
	//{{AFX_MSG(CToolBar256)
	afx_msg void OnSysColorChange();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TOOLBAR256_H__BC9421B2_59EE_11D4_AFB2_004F4C02CAF7__INCLUDED_)
