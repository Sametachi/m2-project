//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDFILL_HPP
#define VSTL_MEMORY_UNINITIALIZEDFILL_HPP

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

template <class ForwardIterator, class T>
ForwardIterator UninitializedFill(ForwardIterator first, ForwardIterator last,
                                  const T& value)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; first != last; ++first)
		new (static_cast<void*>(&*first)) value_type(value);

	return first;
}

template <class U, class T>
typename boost::enable_if_c<
	sizeof(U) == 1 && boost::is_integral<U>::value,
	U*
>::type UninitializedFill(U* first, U* last, T value)
{
	const auto n = static_cast<std::size_t>(last - first);
	return static_cast<U*>(std::memset(first, value, n * sizeof(U))) + n;
}

}

template <class ForwardIterator, class T>
BOOST_FORCEINLINE ForwardIterator uninitialized_fill(ForwardIterator first,
                                                     ForwardIterator last,
                                                     const T& value)
{
	return vstd::detail::UninitializedFill(unwrap_iterator(first),
	                                       unwrap_iterator(last),
	                                       value);
}

}

#endif
