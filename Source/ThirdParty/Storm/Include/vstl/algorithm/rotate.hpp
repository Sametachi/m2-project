//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_ROTATE_HPP
#define VSTL_ALGORITHM_ROTATE_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <algorithm>

namespace vstd
{

template <class ForwardIterator>
ForwardIterator rotate(ForwardIterator first, ForwardIterator middle,
                       ForwardIterator last)
{
	return ForwardIterator(std::rotate(unwrap_iterator(first),
	                                   unwrap_iterator(middle),
	                                   unwrap_iterator(last)));
}

}

#endif
