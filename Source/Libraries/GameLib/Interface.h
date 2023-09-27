#pragma once

#include "Basic/Singleton.h"

class IBackground
{
	public:
		IBackground() {}
		virtual ~IBackground() {}

		virtual bool IsBlock(int32_t x, int32_t y) { return false; }
};

extern IBackground* GetBackgroundInstance();
