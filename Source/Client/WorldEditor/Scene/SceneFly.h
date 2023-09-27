#pragma once

#include "PickingArrow.h"

class CFlyingData;
class CFlyingInstance;

class CSceneFly : public CSceneBase
{
	public:
		CSceneFly();
		virtual ~CSceneFly();

		void Initialize();

		void Play();
		void Stop();

		CFlyingData * GetFlyingDataPointer() { return m_pFlyingData;}

	protected:
		void RenderFlyPosition();

		void	OnUpdate() ;
		void	OnRender(BOOL bClear) ;
		void	OnRenderUI(float fx, float fy) ;

		void	OnKeyDown(int32_t iChar) ;
		void	OnKeyUp(int32_t iChar) ;
		void	OnMouseMove(int32_t x, int32_t y) ;
		void	OnLButtonDown(UINT nFlags, CPoint point) ;
		void	OnLButtonUp() ;
		void	OnRButtonDown() ;
		void	OnRButtonUp() ;
		BOOL	OnMouseWheel(int16_t zDelta) ;
		void	OnMovePosition(float fx, float fy) ;

		const D3DXVECTOR3 start;

		uint32_t m_dwPlayStartTime;
		CFlyingData * m_pFlyingData;
		CFlyingInstance * m_pFlyingInstance;

		D3DXVECTOR3 m_v3Target;

		int32_t m_iGrippedDirection;

		D3DXVECTOR3 m_vecGrippedPosition;
		D3DXVECTOR3 m_vecGrippedValue;
};