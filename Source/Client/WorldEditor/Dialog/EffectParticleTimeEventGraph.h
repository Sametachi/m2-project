#if !defined(AFX_EFFECTTIMEEVENTGRAPH_H__FEB08EC0_B265_4D09_9AC0_0D2C438B35EB__INCLUDED_)
#define AFX_EFFECTTIMEEVENTGRAPH_H__FEB08EC0_B265_4D09_9AC0_0D2C438B35EB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EffectTimeBarGraph.h : header file
//

#include "GraphCtrl.h"

class CFloatEditDialog;

/////////////////////////////////////////////////////////////////////////////
// CEffectTimeEventGraph window

class CEffectTimeEventGraph : public CStatic
{
public:
	class IValueAccessor
	{
		public:
			IValueAccessor(){}
			virtual ~IValueAccessor(){}

			virtual uint32_t GetValueCount() = 0;

			virtual void Insert(float fTime, float fValue) = 0;
			virtual void Delete(uint32_t dwIndex) = 0;

			virtual bool GetTime(uint32_t dwIndex, float * pfTime) = 0;
			virtual bool GetValue(uint32_t dwIndex, float * pfValue) = 0;

			virtual void SetTime(uint32_t dwIndex, float fTime) = 0;
			virtual void SetValue(uint32_t dwIndex, float fValue) = 0;
	};

	typedef struct SPoint
	{
		SPoint(int32_t ix_ = 0, int32_t iy_ = 0) : ix(ix_), iy(iy_) {}

		int32_t ix;
		int32_t iy;
	} TPoint;
	typedef std::vector<TPoint> TPointVector;

	enum
	{
		POINT_NONE = 0xffffffff,

		MAX_GRAPH_COUNT = 3,
	};

// Construction
public:
	CEffectTimeEventGraph();
	void Initialize(int32_t iTimerID);

// Attributes
public:

// Operations
public:
	void Resizing(int32_t iWidth, int32_t iHeight);
	void SetAccessorPointer(IValueAccessor * pAccessor);

	void SetMaxTime(float fMaxTime);
	void SetMaxValue(float fMaxValue);
	void SetStartValue(float fStartValue);

protected:
	void RenderGrid();
	void RenderGraph();
	void ConnectPoint(TPoint & rLeft, TPoint & rRight);

	bool isCreatingMode();

	void TimeToScreen(float fTime, int32_t * px);
	void ScreenToTime(int32_t ix, float * pfTime);

	void ValueToScreen(float fValue, int32_t * piy);
	void ScreenToValue(int32_t iy, float * pfValue);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEffectTimeEventGraph)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CEffectTimeEventGraph();

	// Generated message map functions
protected:
	//{{AFX_MSG(CEffectTimeEventGraph)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

protected:
	// Mouse
	bool m_isLButtonDown;
	bool m_isMButtonDown;
	CPoint m_LastPoint;

	float m_fMaxTime;
	float m_fMaxValue;
	float m_fStartValue;

	uint32_t m_dwSelectedTableIndex;
	uint32_t m_dwSelectedIndex;
	uint32_t m_dwGrippedIndex;

	int32_t m_iWidth;
	int32_t m_iHeight;
	int32_t m_ixTemporary;
	int32_t m_iyTemporary;
	int32_t m_ixGridCount;
	int32_t m_iyGridCount;
	float m_fxGridStep;
	float m_fyGridStep;

	IValueAccessor * m_pAccessor;
	TPointVector m_PointVector;

	CScreen m_Screen;

	static CFloatEditDialog * ms_pFloatEditDialog;
};

/////////////////////////////////////////////////////////////////////////////
// template class for IValueAccessor


template <typename T>
class CTimeEventTableAccessor : public CEffectTimeEventGraph::IValueAccessor
{
public:
	CTimeEventTableAccessor(){}
	~CTimeEventTableAccessor(){}

	typedef std::vector<CTimeEvent<T> > TTimeEventTableType;

	TTimeEventTableType * m_pTimeEventTable;

	void SetTablePointer(TTimeEventTableType * pTimeEventTable)
	{
		m_pTimeEventTable = pTimeEventTable;
	}

	uint32_t GetValueCount()
	{
		return m_pTimeEventTable->size();
	}

	void GetTimeValue(float fTime, T * pfValue)
	{
		//m_pTimeEventTable->
		GetTimeEventBlendValue(fTime, *m_pTimeEventTable, pfValue);
	}

	bool GetTime(uint32_t dwIndex, float * pfTime)
	{
		if (dwIndex >= m_pTimeEventTable->size())
			return false;

		*pfTime = m_pTimeEventTable->at(dwIndex).m_fTime;

		return true;
	}

	bool GetValue(uint32_t dwIndex, T * pfValue)
	{
		if (dwIndex >= m_pTimeEventTable->size())
			return false;

		*pfValue = m_pTimeEventTable->at(dwIndex).m_Value;

		return true;
	}

	void SetTime(uint32_t dwIndex, float fTime)
	{
		if (dwIndex >= m_pTimeEventTable->size())
			return;

		m_pTimeEventTable->at(dwIndex).m_fTime = fTime;
	}
	void SetValue(uint32_t dwIndex, T fValue)
	{
		if (dwIndex >= m_pTimeEventTable->size())
			return;

		m_pTimeEventTable->at(dwIndex).m_Value = fValue;
	}

	void Insert(float fTime, T fValue)
	{
		InsertItemTimeEvent(m_pTimeEventTable, fTime, fValue);
	}

	void InsertBlend(float fTime)
	{
		T fValue;
		GetTimeValue(fTime, &fValue);
		Insert(fTime, fValue);
	}

	void Delete(uint32_t dwIndex)
	{
		DeleteVectorItem<CTimeEvent<T> >(m_pTimeEventTable, dwIndex);
	}
};

typedef CTimeEventTableAccessor<float> CTimeEventFloatAccessor;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EFFECTTIMEEVENTGRAPH_H__FEB08EC0_B265_4D09_9AC0_0D2C438B35EB__INCLUDED_)
