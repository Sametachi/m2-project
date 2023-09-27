//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MATH_APPROXIMATEDISTANCE_HPP
#define STORM_MATH_APPROXIMATEDISTANCE_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/math/Eigen.hpp>

//
// Taken from:
// http://www.flipcode.com/archives/Fast_Approximate_Distance_Functions.shtml
//

namespace storm
{

BOOST_FORCEINLINE int ApproximateDistance(int x, int y)
{
	if (x < 0)
		x = -x;

	if (y < 0)
		y = -y;

	int min, max;

	if (x < y) {
		min = x;
		max = y;
	} else {
		min = y;
		max = x;
	}

	// coefficients equivalent to (123/128 * max) and (51/128 * min)
	return ((max << 8) + (max << 3) - (max << 4) - (max << 1) +
	        (min << 7) - (min << 5) + (min << 3) - (min << 1)) >> 8;
}

BOOST_FORCEINLINE int ApproximateDistance(const Vector2i& pos1,
                                          const Vector2i& pos2)
{
	const Vector2i d = pos2 - pos1;
	return ApproximateDistance(d.x(), d.y());
}

}

#endif
