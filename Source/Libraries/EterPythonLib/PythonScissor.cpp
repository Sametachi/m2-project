#include "StdAfx.h"
#include "PythonScissor.h"

ScissorsSetter::ScissorsSetter(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	wasEnabled_ = STATEMANAGER->GetRenderState(D3DRS_SCISSORTESTENABLE);
	STATEMANAGER->GetDevice()->GetScissorRect(&oldRect_);
	newRect_.left = x;
	newRect_.right = x + width;
	newRect_.top = y;
	newRect_.bottom = y + height;
	STATEMANAGER->GetDevice()->SetScissorRect(&newRect_);
	STATEMANAGER->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
}

ScissorsSetter::~ScissorsSetter()
{
	STATEMANAGER->GetDevice()->SetScissorRect(&oldRect_);
	STATEMANAGER->GetDevice()->SetRenderState(D3DRS_SCISSORTESTENABLE, wasEnabled_);
}