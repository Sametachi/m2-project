//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDCOPY_HPP
#define VSTL_MEMORY_UNINITIALIZEDCOPY_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <cstring>

#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/and.hpp>
#include <boost/utility/enable_if.hpp>

namespace vstd
{

namespace detail
{

template <class InputIterator, class ForwardIterator>
ForwardIterator UninitializedCopy(InputIterator first, InputIterator last,
                                  ForwardIterator result)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; first != last; ++first, ++result)
		new (static_cast<void*>(&*result)) value_type(*first);

	return result;
}

template <class T, class U>
typename boost::enable_if<
	boost::mpl::and_<
		boost::is_same<typename boost::remove_const<T>::type, U>,
		boost::has_trivial_assign<U>
	>, U*
>::type UninitializedCopy(T* first, T* last, U* result)
{
	const auto n = static_cast<std::size_t>(last - first);
	return static_cast<U*>(std::memcpy(result, first, n * sizeof(U))) + n;
}

}

template <class InputIterator, class ForwardIterator>
BOOST_FORCEINLINE ForwardIterator
	uninitialized_copy(InputIterator first, InputIterator last,
                       ForwardIterator result)
{
	return vstd::detail::UninitializedCopy(unwrap_iterator(first),
	                                       unwrap_iterator(last),
	                                       unwrap_iterator(result));
}

}

#endif
