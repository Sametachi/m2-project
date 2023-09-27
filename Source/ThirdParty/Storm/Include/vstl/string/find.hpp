//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_FIND_HPP
#define VSTL_STRING_FIND_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/iterator.hpp>

namespace vstd
{

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits> find(string_iterator<Iterator, Traits> first,
                                       string_iterator<Iterator, Traits> last,
                                       typename Traits::char_type value)
{

	if (first != last) {
		const auto p = Traits::find(first.base(), last - first, value);
		if (p)
			return string_iterator<Iterator, Traits>(const_cast<Iterator>(p));
	}

	return last;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits> find(string_iterator<Iterator, Traits> first,
                                       string_iterator<Iterator, Traits> last,
                                       const typename Traits::char_type* str,
                                       size_type n)
{
	if (n != 0) {
		if (first + n <= last) {
			for (auto last2 = last - (n - 1); first != last2; ++first) {
				if (*first == *str &&
					Traits::compare(first.base() + 1, str + 1, n - 1) == 0)
					return first;
			}
		}

		return last;
	}

	return first;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits> find(string_iterator<Iterator, Traits> firstA,
                                       string_iterator<Iterator, Traits> lastA,
                                       const typename Traits::char_type* firstB,
                                       const typename Traits::char_type* lastB)
{
	return vstd::find(firstA, lastA, firstB, lastB - firstB);
}

}

#endif
