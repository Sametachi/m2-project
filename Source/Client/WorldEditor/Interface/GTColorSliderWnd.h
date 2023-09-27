#if !defined(AFX_GTCOLORSLIDERWND_H__F818AFD3_4DE0_11D6_A56D_525400EA266C__INCLUDED_)
#define AFX_GTCOLORSLIDERWND_H__F818AFD3_4DE0_11D6_A56D_525400EA266C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GTColorSliderWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// GTColorSliderWnd window

#include "GTDrawHelper.h"

// slider visual modes against max, max_component_1 and max_component_2
const int32_t max_slider [6][3] =
{
	{ max_rgb_red, max_rgb_green, max_rgb_blue },	// max_rgb red: R, G, B
	{ max_rgb_green, max_rgb_red, max_rgb_blue },	// max_rgb green: G, R, B
	{ max_rgb_blue, max_rgb_red, max_rgb_green },	// max_rgb blue: B, R, G
	{ max_hsv_hue, max_hsv_sat, max_hsv_value },	// max_hsv hue: H, S, V
	{ max_hsv_sat, max_hsv_hue, max_hsv_value },	// max_hsv sat: S, H, V
	{ max_hsv_value, max_hsv_hue, max_hsv_sat }		// max_hsv value: V, H, S
};

class GTColorSliderWnd : public CWnd
{
	public:
		// Constroctor
		GTColorSliderWnd();

	// Attributes
	public:
		//Create Arrow Type Window
		BOOL Create(uint32_t dwStyle,
					CRect rcPos,
					CWnd* pParent,
					UINT nID,
					int32_t nBulletType,
					BOOL bPopup = FALSE);

		// get/set layout
		inline int32_t GetLayoutValue(void) const { return (m_nCurLayout); };
		void SetLayoutValue(int32_t value);

		// get/set visual mode
		int32_t GetDataValue(void) const { return (m_nCurMode & modes_mask); };
		void SetDataValue(int32_t value);

		// get/set value at the begining of the slider
		inline int32_t GetStartValue(void) const { return (m_nStartValue); };
		void SetStartValue(int32_t value);

		// get/set value at the end of the slider
		inline int32_t GetEndValue(void) const { return (m_nEndValue); };
		void SetEndValue(IN int32_t value);

		// get/set position of the slider
		inline int32_t GetPosition(void) const { return (m_nCurMode & modes_reverse ? m_nEndValue - m_nCurPosition : m_nCurPosition); };
		void SetPosition(IN int32_t value);

		// get/set one-of slider additional components
		inline int32_t GetControlValue(int32_t index) const { return (additional_components [index]); };
		void SetControlValue(int32_t index,int32_t value);

		// painting methods
		void OnDraw(void);

		// draws blend in rgb mode - includes red, green and blue modes
		void DoDrawRGB(void);

		// draws blend in hsv mode - hue, sat and value modes separately
		void DoDrawHSV_Hue(void);
		void DoDrawHSV_Sat(void);
		void DoDrawHSV_Val(void);

		// parent notification methods
		virtual LRESULT	SendMessageToParent(IN UINT code);

	public:
		// Operations
		enum layouts
		{
			layout_horizontal = 0x8000,	// horizontal slider
			layout_tr_top = 0x2000,		// triangles
			layout_tr_bottom = 0x1000,

			layout_vertical = 0x4000,	// vertical slider
			layout_tr_left = 0x2000,	// triangles
			layout_tr_right = 0x1000,
		};

	protected:
		int32_t		m_nCurMode;	// visual mode (colorspace)
		int32_t		m_nCurLayout;

		int32_t		m_nStartValue;
		int32_t		m_nEndValue;
		int32_t		m_nCurPosition;
		int32_t		m_nPreviousPos;
		int32_t		m_nSteps;
		int32_t		m_nPageSizes;

		CRect	m_rcPanel;
		CRect	m_rcPaint;
		int32_t		m_nTriangle;

		// additional components depends on current visual mode (indexes given below):
		// - rgb red - 0 is green in [0, 255], 1 is blue in [0, 255]
		// - rgb green - 0 is red in [0, 255], 1 is blue in [0, 255]
		// - rgb blue - 0 is red in [0, 255], 1 is green in [0, 255]
		// - hsv hue - 0 is sat in [0, 1000 (1000 is 100.0% or 1.0)], 1 is value in [0, 1000]
		// - hsv sat - 0 is hue in [0, 3599 (3599 is 359.9 degs)], 1 is value in [0, 1000]
		// - hsv value - 0 is hue in [0, 3599], 1 is sat in [0, 1000]
		int32_t		additional_components [4];

		uint32_t	*row_buffer;			// 1px-high 32-bit bitmap, having same width as blend
		int32_t		m_nBuffSize;			// size of the row buffer in doublewords

		// cache DIB bitmap information
		BITMAPINFO	bmp_info;
		HBITMAP		bmp_handle;
		uint32_t		*bmp_data;

		CRect	m_rcFrame;				// control's m_rcFrame (bounding box)
		CRect	m_rcInteraction;	// area, where mouse-drags will be traced (client area)
		bool	m_bTracking;			// whether mouse is being tracked (drag)

	public:
	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(GTColorSliderWnd)
		protected:
		virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
		virtual void PreSubclassWindow();
		virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
		//}}AFX_VIRTUAL

	// Implementation
	public:
		// Destructor
		virtual ~GTColorSliderWnd();

		// Generated message map functions
	protected:
		//{{AFX_MSG(GTColorSliderWnd)

		//message map function On Erase Background
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);

		//message map function On left button double clicked
		afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);

		//message map function On left button down
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

		//message map function On left button up
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);

		//message map function On middle button double clicked
		afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);

		//message map function On middle button down
		afx_msg void OnMButtonDown(UINT nFlags, CPoint point);

		//message map function On middle button up
		afx_msg void OnMButtonUp(UINT nFlags, CPoint point);

		//message map function On Mouse Move
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);

		// message map function On keystroke  nonsystem character
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);

		//message map function On window Destroyed
		afx_msg void OnDestroy();

	   //message map function On window painted
		afx_msg void OnPaint();

		//message map function On right button down
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);

		//message map function On right button up
		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

		//message map function On Cancel Mode
		afx_msg void OnCancelMode();

		//message map function On Key Down
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		//message map function On window size changed
		afx_msg void OnSize(UINT nType, int32_t cx, int32_t cy);
		// System color change.
		afx_msg void OnSysColorChange();
		afx_msg UINT OnGetDlgCode();
		//}}AFX_MSG

		//message map function On Select Day OK
		afx_msg int32_t OnSelectBulletOK(UINT wParam, int32_t lParam);
		DECLARE_MESSAGE_MAP()

	public:
		// The pointer to Notify Window
		CWnd *pNotifyWnd;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GTCOLORSLIDERWND_H__F818AFD3_4DE0_11D6_A56D_525400EA266C__INCLUDED_)
