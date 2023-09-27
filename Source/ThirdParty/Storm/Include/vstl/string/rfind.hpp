//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_RFIND_HPP
#define VSTL_STRING_RFIND_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/iterator.hpp>

namespace vstd
{

template <typename Iterator, class Impl>
string_iterator<Iterator, Impl> rfind(string_iterator<Iterator, Impl> first,
                                      string_iterator<Iterator, Impl> last,
                                      typename Impl::value_type value)
{
	typedef typename Impl::traits_type Traits;

	const auto p = Traits::rfind(first.base(), last - first, value);
	if (p)
		return string_iterator<Iterator, Impl>(const_cast<Iterator>(p));

	return first;
}

template <typename Iterator, class Impl>
string_iterator<Iterator, Impl> rfind(string_iterator<Iterator, Impl> first,
                                      string_iterator<Iterator, Impl> last,
                                      typename Impl::const_pointer str,
                                      size_type n)
{
	typedef typename Impl::traits_type Traits;

	if (first + n <= last) {
		if (n != 0) {
			auto last2 = last - n;
			++last2;

			do {
				--last2;

				if (*last2 == *str &&
					Traits::compare(last2.base() + 1, str + 1, n - 1) == 0)
					return last2;
			} while (last2 != first);

			return last;
		}

		if (first != last)
			--last;
	}

	return last;
}

template <typename Iterator, class Impl>
string_iterator<Iterator, Impl> rfind(string_iterator<Iterator, Impl> firstA,
                                      string_iterator<Iterator, Impl> lastA,
                                      typename Impl::const_pointer firstB,
                                      typename Impl::const_pointer lastB)
{
	return vstd::rfind(firstA, lastA, firstB, lastB - firstB);
}

}

#endif
