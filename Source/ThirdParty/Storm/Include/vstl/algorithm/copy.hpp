//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_COPY_HPP
#define VSTL_ALGORITHM_COPY_HPP

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

template <class InputIterator, class OutputIterator>
OutputIterator Copy(InputIterator first, InputIterator last,
                    OutputIterator result)
{
	for ( ; first != last; ++first, ++result)
		*result = *first;

	return result;
}

template <class T, class U>
typename boost::enable_if<
	boost::mpl::and_<
		boost::is_same<typename boost::remove_const<T>::type, U>,
		boost::has_trivial_assign<U>
	>, U*
>::type Copy(T* first, T* last, U* result)
{
	const size_type n = static_cast<size_type>(last - first);
	return static_cast<U*>(std::memmove(result, first, n * sizeof(U))) + n;
}

}

template <class InputIterator, class OutputIterator>
BOOST_FORCEINLINE OutputIterator copy(InputIterator first, InputIterator last,
                                      OutputIterator result)
{
	return vstd::detail::Copy(unwrap_iterator(first), unwrap_iterator(last),
	                          unwrap_iterator(result));
}

}

#endif
