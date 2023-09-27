#pragma once

class CGraphCtrl : public CScreen
{
	public:
		class IValueAccessor
		{
			public:
				IValueAccessor(){}
				virtual ~IValueAccessor(){}

				virtual uint32_t GetValueCount() = 0;

				virtual void Insert(float fTime, uint32_t dwValue) = 0;
				virtual void Delete(uint32_t dwIndex) = 0;

				virtual bool GetTime(uint32_t dwIndex, float * pTime) = 0;
				virtual bool GetValue(uint32_t dwIndex, uint32_t * pValue) = 0;

				virtual void SetTime(uint32_t dwIndex, float fTime) = 0;
				virtual void SetValue(uint32_t dwIndex, uint32_t dwValue) = 0;
		};

		typedef struct SPoint
		{
			SPoint(int32_t ix_ = 0, int32_t iy_ = 0) : ix(ix_), iy(iy_) {}

			int32_t ix;
			int32_t iy;
		} TPoint;

	public:
		enum
		{
			POINT_NONE = 0xffffffff,

			HORIZON_ZOOMING_MAX = 10,
			HORIZON_ZOOMING_MIN = 1,
		};
		enum EGraphType
		{
			GRAPH_TYPE_BOOLEAN,
			GRAPH_TYPE_BRIDGE,
			GRAPH_TYPE_LINEAR,
			GRAPH_TYPE_BLOCK,
		};
		enum EValueType
		{
			VALUE_TYPE_CENTER,
			VALUE_TYPE_ONLY_UP,
			VALUE_TYPE_ONLY_DOWN,
			VALUE_TYPE_UP_AND_DOWN,
		};

		typedef std::vector<CGraphicTextInstance*> TTextInstanceVector;
		typedef std::vector<TPoint> TPointVector;

	public:
		CGraphCtrl();
		virtual ~CGraphCtrl();

		void Initialize();
		void SetGraphType(int32_t iType);
		void SetValueType(int32_t iType);
		void SetAccessorPointer(IValueAccessor * pAccessor);
		void SetSize(int32_t iWidth, int32_t iHeight);

		void Move(float fx, float fy);

		void Update();
		void GraphBegin();
		void GraphEnd(RECT * pRect, HWND hWnd);
		void Render();
		void RenderTimeLine(float fTime);
		void RenderEndLine(float fTime);

		void ZoomInHorizon();
		void ZoomOutHorizon();

		void OnMouseMove(int32_t ix, int32_t iy);
		void OnLButtonDown(int32_t ix, int32_t iy);
		void OnLButtonUp(int32_t ix, int32_t iy);
		void OnRButtonDown(int32_t ix, int32_t iy);
		void OnRButtonUp(int32_t ix, int32_t iy);
		void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

		uint32_t GetSelectedIndex();
		void SetSelectedIndex(uint32_t dwSelectedIndex);

	protected:
		void BuildHorizontalLine(int32_t iCount);
		void UpdateHorizontalLine(int32_t iStartPosition, int32_t iZoom);

		void RenderGrid();
		void RenderGraph();
		void ConnectPoint(TPoint & rLeft, TPoint & rRight);

		void TimeToScreen(float fTime, int32_t * px);
		void ScreenToTime(uint32_t ix, float * pTime);

		bool isCreatingMode();

	protected:
		int32_t m_iGraphType;
		int32_t m_iValueType;

		IValueAccessor * m_pAccessor;

		// Mouse Control
		uint32_t m_dwSelectedIndex;
		uint32_t m_dwGrippedIndex;

		TPointVector m_PointVector;

		// For Rendering
		int32_t m_iWidth;
		int32_t m_iHeight;

		float m_fxPosition;
		float m_fyPosition;
		int32_t m_iHorizontalZoom;

		int32_t m_ixTemporary;
		int32_t m_iyTemporary;
		int32_t m_ixGridStep;
		int32_t m_iyGridStep;
		int32_t m_ixGridCount;
		int32_t m_iyGridCount;

		// Text
		TTextInstanceVector m_HorizontalTextLine;
		TTextInstanceVector m_VerticalTextLine;

		CGraphicTextInstance::TPool m_textInstancePool;
};