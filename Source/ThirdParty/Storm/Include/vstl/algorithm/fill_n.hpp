//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FILLN_HPP
#define VSTL_ALGORITHM_FILLN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <cstring>

#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/utility/enable_if.hpp>

namespace vstd
{

namespace detail
{

template <class OutputIterator, typename Size, class T>
OutputIterator FillN(OutputIterator first, Size n, const T& value)
{
	for ( ; n; --n)
		*first++ = value;

	return first;
}

template <class U, typename Size, class T>
typename boost::enable_if_c<
	sizeof(T) == 1 && boost::has_trivial_assign<T>::value,
	U*
>::type FillN(U* first, Size n, T value)
{
	return static_cast<U*>(std::memset(first, static_cast<uint8_t>(value),
	                       static_cast<std::size_t>(n * sizeof(U)))) + n;
}

}

template <class OutputIterator, typename Size, class T>
BOOST_FORCEINLINE OutputIterator
	fill_n(OutputIterator first, Size n, const T& value)
{
	return vstd::detail::FillN(unwrap_iterator(first), n, value);
}

}

#endif
