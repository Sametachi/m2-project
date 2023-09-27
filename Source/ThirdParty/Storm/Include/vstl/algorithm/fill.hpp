//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FILL_HPP
#define VSTL_ALGORITHM_FILL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/utility/enable_if.hpp>

#include <cstring>

namespace vstd
{

namespace detail
{

template <class OutputIterator, class T>
OutputIterator Fill(OutputIterator first, OutputIterator last, const T& value)
{
	for ( ; first != last; ++first)
		*first = value;

	return first;
}

template <class U, class T>
typename boost::enable_if_c<
	sizeof(T) == 1 && boost::has_trivial_assign<T>::value,
	U*
>::type Fill(U* first, U* last, T value)
{
	const size_type n = static_cast<size_type>(last - first);
	return static_cast<U*>(std::memset(first, static_cast<uint8_t>(value),
	                       static_cast<std::size_t>(n * sizeof(U)))) + n;
}

}

template <class OutputIterator, class T>
BOOST_FORCEINLINE OutputIterator
	fill(OutputIterator first, OutputIterator last, const T& value)
{
	return vstd::detail::Fill(unwrap_iterator(first),
	                          unwrap_iterator(last),
	                          value);
}

}

#endif
