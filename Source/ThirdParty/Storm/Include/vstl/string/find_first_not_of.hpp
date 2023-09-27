//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_FINDFIRSTNOTOF_HPP
#define VSTL_STRING_FINDFIRSTNOTOF_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/iterator.hpp>

namespace vstd
{

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits> find_first_not_of(
	string_iterator<Iterator, Traits> first,
	string_iterator<Iterator, Traits> last,
	const typename Traits::char_type* str,
	size_type n
)
{
	if (n != 0) {
		for ( ; first != last; ++first) {
			if (!Traits::find(str, n, *first))
				return first;
		}
	} else {
		return first;
	}

	return last;
}

template <typename Iterator, class Traits>
typename Traits::const_iterator find_first_not_of(
	string_iterator<Iterator, Traits> firstA,
	string_iterator<Iterator, Traits> lastA,
	const typename Traits::char_type* firstB,
	const typename Traits::char_type* lastB
)
{
	return vstd::find_first_not_of(firstA, lastA, firstB, lastB - firstB);
}

}

#endif
