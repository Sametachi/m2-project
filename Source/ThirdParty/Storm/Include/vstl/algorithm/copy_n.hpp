//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_COPYN_HPP
#define VSTL_ALGORITHM_COPYN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <boost/mpl/and.hpp>
#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/utility/enable_if.hpp>

#include <cstring>

namespace vstd
{

namespace detail
{

template <class InputIterator, typename Size, class OutputIterator>
OutputIterator CopyN(InputIterator first, Size n,
					OutputIterator result)
{
	for ( ; n; --n)
		*result++ = *first++;

	return result;
}

template <class T, typename Size, class U>
typename boost::enable_if<
	boost::mpl::and_<
		boost::is_same<typename boost::remove_const<T>::type, U>,
		boost::has_trivial_assign<U>
	>, U*
>::type CopyN(T* first, Size n, U* result)
{
	return static_cast<U*>(std::memmove(result, first,
	                       n * sizeof(U))) + n;
}

template <class InputIterator, typename Size, class OutputIterator>
OutputIterator CopyNNonOverlapped(InputIterator first, Size n,
					OutputIterator result)
{
    for ( ; n; --n)
        *result++ = *first++;

    return result;
}

template <class T, typename Size, class U>
typename boost::enable_if<boost::mpl::and_<
		boost::is_same<typename boost::remove_const<T>::type, U>,
		boost::has_trivial_assign<U> >
	, U*
>::type CopyNNonOverlapped(T* first, Size n, U* result)
{
    return static_cast<U*>(std::memcpy(result, first,
							static_cast<std::size_t>(n * sizeof(U)))) + n;
}

}

template <class InputIterator, typename Size, class OutputIterator>
BOOST_FORCEINLINE OutputIterator
	copy_n(InputIterator first, Size n, OutputIterator result)
{
    return vstd::detail::CopyN(unwrap_iterator(first), n,
							unwrap_iterator(result));
}

template <class InputIterator, typename Size, class OutputIterator>
BOOST_FORCEINLINE OutputIterator
	copy_n_nonoverlapped(InputIterator first, Size n,
					OutputIterator result)
{
    return vstd::detail::CopyNNonOverlapped(unwrap_iterator(first), n,
										unwrap_iterator(result));
}

}

#endif
