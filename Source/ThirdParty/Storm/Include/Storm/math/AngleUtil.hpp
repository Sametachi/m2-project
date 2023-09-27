//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MATH_ANGLEUTIL_HPP
#define STORM_MATH_ANGLEUTIL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/math/Eigen.hpp>

#include <boost/math/constants/constants.hpp>

#include <cmath>

namespace storm
{

template<typename T>
BOOST_FORCEINLINE T SignedDegreeToUnsignedDegree(T fSrc)
{
	if (fSrc < 0.0f)
	{
		return T(360.0 + T(fmod(fSrc, 360.0)));
	}

	return T(fmod(fSrc, 360.0));
}

BOOST_FORCEINLINE float RadiansFromDegrees(float value)
{
	return value * boost::math::float_constants::pi / 180.0f;
}

BOOST_FORCEINLINE float DegreesFromRadians(float value)
{
	return value * 180.0f / boost::math::float_constants::pi;
}

BOOST_FORCEINLINE float GetAngleFromPosition(float x, float y)
{
	return std::atan2(x, y);
}

BOOST_FORCEINLINE float GetAngleFromPosition(const Vector2f& pos1,
                                             const Vector2f& pos2)
{
	const Vector2f p = pos2 - pos1;
	return std::atan2(p.x(), p.y());
}

BOOST_FORCEINLINE Vector2f GetRotatedPosition(const Vector2f& p, float angle)
{
	return Vector2f(
		p.x() * std::cos(angle) - p.y() * std::sin(angle),
		p.x() * std::sin(angle) + p.y() * std::cos(angle)
	);
}

BOOST_FORCEINLINE Vector2f GetPositionFromAngle(float distance, float angle)
{
	return Vector2f(
		distance * std::sin(angle),
		distance * std::cos(angle)
	);
}

}

#endif
