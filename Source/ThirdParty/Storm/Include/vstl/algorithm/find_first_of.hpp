//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FINDFIRSTOF_HPP
#define VSTL_ALGORITHM_FINDFIRSTOF_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

template <class InputIterator, class ForwardIterator>
InputIterator find_first_of(InputIterator firstA, InputIterator lastA,
                            ForwardIterator firstB, ForwardIterator lastB)
{
	for ( ; firstA != lastA; ++firstA) {
		for (auto b = firstB; b != lastB; ++b) {
			if (*firstA == *b)
				return firstA;
		}
	}

	return lastA;
}

template <class InputIterator, class ForwardIterator, class BinaryPredicate>
InputIterator find_first_of(InputIterator firstA, InputIterator lastA,
                            ForwardIterator firstB, ForwardIterator lastB,
                            BinaryPredicate pred)
{
	for ( ; firstA != lastA; ++firstA) {
		for (auto b = firstB; b != lastB; ++b) {
			if (pred(*firstA, *b))
				return firstA;
		}
	}

	return lastA;
}

}

#endif
