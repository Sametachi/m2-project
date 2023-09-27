//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FINDIFNOT_HPP
#define VSTL_ALGORITHM_FINDIFNOT_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

template <class InputIterator, class UnaryPredicate>
InputIterator find_if_not(InputIterator first, InputIterator last,
                          UnaryPredicate pred)
{
    for ( ; first != last; ++first) {
        if (!pred(*first))
            return first;
    }

    return last;
}

template <class ForwardIteratorA, class ForwardIteratorB, class BinaryPredicate>
ForwardIteratorA find_if_not(ForwardIteratorA firstA, ForwardIteratorA lastA,
                             ForwardIteratorB firstB, ForwardIteratorB lastB,
                             BinaryPredicate pred)
{
    for ( ; ; ++firstA) {
		ForwardIteratorA a = firstA;
		ForwardIteratorB b = firstB;

		for ( ; ; ++a, ++b) {
			if (b == lastB)
				return firstA;

			if (a == lastA)
				return lastA;

			if (pred(*a, *b))
				break;
		}
	}
}

}

#endif
