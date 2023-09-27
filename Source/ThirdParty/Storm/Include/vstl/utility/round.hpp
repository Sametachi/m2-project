//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_UTILITY_ROUND_HPP
#define VSTL_UTILITY_ROUND_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

template <typename IntType>
IntType round_down(IntType number, IntType factor)
{ return number - number % factor; }

template <typename IntType>
IntType round_down_pow2(IntType number, IntType factor)
{ return number - number & (factor - 1); }

template <typename IntType>
IntType round_up(IntType number, IntType factor)
{ return number + factor - 1 - (number - 1) % factor; }

template <typename IntType>
IntType round_up_pow2(IntType number, IntType factor)
{
	--factor;
	return (number + factor) & ~factor;
}

}

#endif
