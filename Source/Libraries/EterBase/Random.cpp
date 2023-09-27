#include "StdAfx.h"
#include <cassert>
#include <random>

std::mt19937& GetRand()
{
	static std::mt19937 rnd;
	return rnd;
}

static uint32_t randseed = 1;

void srandom(uint32_t seed)
{
	randseed = seed;
}

uint32_t random()
{
	int32_t x = randseed;
	int32_t hi = x / 127773;
	int32_t lo = x % 127773;
	int32_t t = 16807 * lo - 2836 * hi;
	if (t <= 0)
		t += 0x7fffffff;
	randseed = t;
	return (t);
}

float frandom(float flLow, float flHigh)
{
	float fl = float(random()) / float(2147483648.0f);
	return (fl * (flHigh - flLow)) + flLow;
}

int32_t random_range(int32_t from, int32_t to)
{
	assert(from <= to);
	return ((random() % (to - from + 1)) + from);
}

uint32_t GetRandomInt(__in uint32_t uMin, __in uint32_t uMax)
{
	if (uMax < (uint32_t)0xFFFFFFFF)
		uMax++;

	return (rand() % (uMax - uMin)) + uMin;
}

LPSTR GetRandomStringA(__in uint32_t uMin)
{
	LPSTR pString = (LPSTR)malloc(uMin + 64);

	while (strlen(pString) < uMin)
	{
		CHAR pSubString[32] = { 0x0, 0x0 };

		_itoa_s(GetRandomInt(1, 0xffffff), pSubString, 6, 0x10);
		strcat_s(pString, uMin + 64, pSubString);
	}

	return pString;
}

