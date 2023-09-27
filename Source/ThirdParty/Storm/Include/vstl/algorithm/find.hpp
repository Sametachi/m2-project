//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FIND_HPP
#define VSTL_ALGORITHM_FIND_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

template <class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& value)
{
	for ( ; first != last; ++first) {
		if (*first == value)
			return first;
	}

	return last;
}

template <class ForwardIteratorA, class ForwardIteratorB>
ForwardIteratorA find(ForwardIteratorA firstA, ForwardIteratorA lastA,
                      ForwardIteratorB firstB, ForwardIteratorB lastB)
{
	for ( ; ; ++firstA) {
		ForwardIteratorA a = firstA;
		ForwardIteratorB b = firstB;

		for ( ; ; ++a, ++b) {
			if (b == lastB)
				return firstA;

			if (a == lastA)
				return lastA;

			if (!(*a == *b))
				break;
		}
	}
}

}

#endif
