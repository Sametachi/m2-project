#pragma once
#include <string>
#include "../EterLib/StateManager.h"

class ScissorsSetter
{
	public:
		ScissorsSetter(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		~ScissorsSetter();
	
	private:
		RECT oldRect_;
		RECT newRect_;
		uint32_t wasEnabled_;
};

