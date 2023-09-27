//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_UTILITY_RELOPS_HPP
#define VSTL_UTILITY_RELOPS_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

namespace rel_ops
{

template <class T>
BOOST_FORCEINLINE bool operator!=(const T& first, const T& second)
{
    return !(first == second);
}

template <class T>
BOOST_FORCEINLINE bool operator>(const T& first, const T& second)
{
    return second < first;
}

template <class T>
BOOST_FORCEINLINE bool operator<=(const T& first, const T& second)
{
    return !(second < first);
}

template <class T>
BOOST_FORCEINLINE bool operator>=(const T& first, const T& second)
{
    return !(first < second);
}

}

}

#endif
