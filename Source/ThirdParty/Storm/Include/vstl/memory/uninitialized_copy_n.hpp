//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDCOPYN_HPP
#define VSTL_MEMORY_UNINITIALIZEDCOPYN_HPP

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

template <class InputIterator, typename Size, class ForwardIterator>
ForwardIterator UninitializedCopyN(InputIterator first, Size n,
                                   ForwardIterator result)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; n; --n)
		new (static_cast<void*>(&*result)) value_type(*first);

	return result;
}

template <class T, typename Size, class U>
typename boost::enable_if<
	boost::mpl::and_<
		boost::is_same<typename boost::remove_const<T>::type, U>,
		boost::has_trivial_assign<U>
	>, U*
>::type UninitializedCopyN(T* first, Size n, U* result)
{
	const auto s = static_cast<std::size_t>(n * sizeof(U));
	return static_cast<U*>(std::memcpy(result, first, s)) + n;
}

}

template <class InputIterator, typename Size, class ForwardIterator>
BOOST_FORCEINLINE ForwardIterator
	uninitialized_copy_n(InputIterator first, Size n, ForwardIterator result)
{
	return vstd::detail::UninitializedCopyN(unwrap_iterator(first), n,
	                                        unwrap_iterator(result));
}

}

#endif
