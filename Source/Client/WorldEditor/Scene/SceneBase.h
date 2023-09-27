#pragma once

#include "../../../Libraries/eterlib/GrpTextInstance.h"

#define WORLDEDITOR_CAMERA	3

class CCamera;

class CSceneBase : public CScreen
{
	public:
		enum ERenderingMode
		{
			RENDERING_MODE_SOLID = D3DFILL_SOLID,
			RENDERING_MODE_WIRE_FRAME = D3DFILL_WIREFRAME,
		};

	public:
		CSceneBase();
		virtual ~CSceneBase();

		void		Update();
		void		Render(BOOL bClear);
		void		RenderUI();

		void		KeyDown(int32_t iChar);
		void		KeyUp(int32_t iChar);
		void		MouseMove(int32_t ix, int32_t iy);
		void		LButtonDown(UINT nFlags, CPoint point);
		void		LButtonUp();
		void		RButtonDown();
		void		RButtonUp();
		BOOL		MouseWheel(int16_t zDelta);
		void		MovePosition(float fx, float fy);
		void		UpdateStatusBar();

		static void	CreateUI();

		static int32_t	GetRenderingMode();
		static void	SetRenderingMode(ERenderingMode RenderingMode);

	protected:
		void		RenderGrid();

	protected:
		virtual void	OnUpdate() = 0;
		virtual void	OnRender(BOOL bClear) = 0;
		virtual void	OnRenderUI(float fx, float fy) = 0;

		virtual void	OnKeyDown(int32_t iChar) = 0;
		virtual void	OnKeyUp(int32_t iChar) = 0;
		virtual void	OnMouseMove(int32_t x, int32_t y) = 0;
		virtual void	OnLButtonDown(UINT nFlags, CPoint point) = 0;
		virtual void	OnLButtonUp() = 0;
		virtual void	OnRButtonDown() = 0;
		virtual void	OnRButtonUp() = 0;
		virtual BOOL	OnMouseWheel(int16_t zDelta) = 0;
		virtual void	OnMovePosition(float fx, float fy) = 0;

	protected:
		static D3DXCOLOR		ms_GridLarge;
		static D3DXCOLOR		ms_GridSmall;

		static D3DXVECTOR3		ms_vecMousePosition;

		//////////////////////////////////
		static ERenderingMode	ms_RenderingMode;
		static CCamera *		ms_Camera;

		//////////////////////////////////
		static CGraphicTextInstance ms_TextInstanceFaceCount;
		static CGraphicTextInstance ms_TextInstanceCameraDistance;
		static CGraphicTextInstance ms_TextInstancePureRenderingTime;

		static int32_t ms_iPureRenderingTime;
};