//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDFILLN_HPP
#define VSTL_MEMORY_UNINITIALIZEDFILLN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <cstring>

#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>

namespace vstd
{

namespace detail
{

template <class ForwardIterator, typename Size, class T>
ForwardIterator UninitializedFillN(ForwardIterator first, Size n, const T& value)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; n; ++first, --n)
		new (static_cast<void*>(&*first)) value_type(value);

	return first;
}

template <class U, typename Size, class T>
typename boost::enable_if_c<
	sizeof(U) == 1 && boost::is_integral<U>::value,
	U*
>::type UninitializedFillN(U* first, Size n, T value)
{
	const auto s = static_cast<std::size_t>(n) * sizeof(U);
	return static_cast<U*>(std::memset(first, value, s)) + n;
}

}

template <class ForwardIterator, typename Size, class T>
BOOST_FORCEINLINE ForwardIterator uninitialized_fill_n(ForwardIterator first,
                                                       Size n,
                                                       const T& value)
{
    return vstd::detail::UninitializedFillN(unwrap_iterator(first), n, value);
}

}

#endif
