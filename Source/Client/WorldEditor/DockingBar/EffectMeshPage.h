#if !defined(AFX_EFFECTMESHPAGE_H__4FD0E72E_84BA_4BD3_BB0D_86BD94481345__INCLUDED_)
#define AFX_EFFECTMESHPAGE_H__4FD0E72E_84BA_4BD3_BB0D_86BD94481345__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EffectMeshPage.h : header file
//

//------------------------//
// Test Code
#include "EffectUI.h"
//------------------------//

#include "../Dialog/EffectParticleTimeEventGraph.h"

/////////////////////////////////////////////////////////////////////////////
// CEffectMeshPage dialog

class CEffectMeshPage : public CPageCtrl
{
// Construction
public:
	CEffectMeshPage(CWnd* pParent = NULL);   // standard constructor
	BOOL Create(CWnd * pParent);

	void UpdateUI();

	void SetData(uint32_t dwEffectIndex);
	void SetElement(uint32_t dwEffectIndex, uint32_t dwElementIndex);

// Dialog Data
	//{{AFX_DATA(CEffectMeshPage)
	enum { IDD = IDD_EFFECT_MESH };
	CEffectTimeEventGraph	m_ctrlAlphaGraph;
	COperationTypeComboBox	m_ctrlColorOperation;
	CColorBox	m_ctrlColorFactor;
	CSliderCtrl	m_ctrlMeshAnimationDelay;
	CSliderCtrl	m_ctrlTextureAnimationDelay;
	CBlendTypeComboBox	m_ctrlBlendDestType;
	CBlendTypeComboBox	m_ctrlBlendSrcType;
	CComboBox	m_ctrlMeshList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEffectMeshPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	void SetBillboardType(int32_t iType);
	void SetBlendType(int32_t iSrcType, int32_t iDestType);

	bool GetEffectElement(uint32_t dwIndex, CEffectAccessor::TEffectElement ** ppElement);
	uint32_t GetMeshElementCount(uint32_t dwEffectIndex);
	bool GetMeshElementData(uint32_t dwEffectIndex, uint32_t dwMeshIndex, CEffectMeshScript::TMeshData ** ppMeshData);

	void RefreshMeshElementList();

	static void CallBack();

	// Generated message map functions
	//{{AFX_MSG(CEffectMeshPage)
	afx_msg void OnLoadModel();
	afx_msg void OnSelectBillboardOff();
	afx_msg void OnSelectBillboardAll();
	afx_msg void OnSelectBillboardY();
	afx_msg void OnInsertTexture();
	afx_msg void OnEnableBlend();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnAcceptTextureAnimationStartFrame();
	afx_msg void OnEnableTextureAlpha();
	afx_msg void OnAcceptAllBillboard();
	afx_msg void OnAcceptAllAlpha();
	afx_msg void OnAcceptAllTextureAnimation();
	afx_msg void OnEffectMeshAlignMove();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Attributes
protected:
	uint32_t m_dwElementIndex;
	uint32_t m_dwMeshIndex;
	static CEffectMeshPage * ms_pThis;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EFFECTMESHPAGE_H__4FD0E72E_84BA_4BD3_BB0D_86BD94481345__INCLUDED_)
