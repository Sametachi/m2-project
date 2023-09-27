#pragma once
#include <wtypes.h>
extern void srandom(uint32_t seed);
extern uint32_t random();
extern float frandom(float flLow, float flHigh);
extern int32_t random_range(int32_t from, int32_t to);
extern LPSTR GetRandomStringA(__in uint32_t uMin);

#include <random>
#include <utility>
#include <type_traits>
#include <boost/utility/enable_if.hpp>
std::mt19937& GetRand();

template <typename T>
typename boost::enable_if<std::is_integral<T>, T>::type GetRandom(T lo, T hi)
{
	if (lo == hi)
		return lo;

	if (hi < lo)
		std::swap(lo, hi);

	std::uniform_int_distribution<T> d(lo, hi);
	return d(GetRand());
}

template <typename T>
typename boost::enable_if<std::is_floating_point<T>, T>::type GetRandom(T lo, T hi)
{
	if (lo == hi)
		return lo;

	if (hi < lo)
		std::swap(lo, hi);

	std::uniform_real_distribution<T> d(lo, hi);
	return d(GetRand());
}

template <typename T>
T GetRandom(const std::pair<T, T>& lohi)
{
	return GetRandom<T>(lohi.first, lohi.second);
}