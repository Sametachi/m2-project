//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDDEFAULT_HPP
#define VSTL_MEMORY_UNINITIALIZEDDEFAULT_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/unwrap.hpp>

#include <cstring>

#include <boost/type_traits/has_trivial_constructor.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/call_traits.hpp>

namespace vstd
{

namespace detail
{

template <class ForwardIterator>
ForwardIterator UninitializedDefault(ForwardIterator first, ForwardIterator last)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; first != last; ++first)
		new (static_cast<void*>(&*first)) value_type();

	return first;
}

template <class U>
typename boost::enable_if<
	(sizeof(U) == 1 && boost::has_trivial_default_constructor<U>::value) ||
		boost::is_integral<U>::value,
	U*
>::type UninitializedDefault(U* first, U* last)
{
	const auto n = static_cast<std::size_t>(last - first);
	return static_cast<U*>(std::memset(first, U(), n * sizeof(U))) + n;
}

}

template <class ForwardIterator, class T>
BOOST_FORCEINLINE ForwardIterator uninitialized_default(ForwardIterator first,
                                                        ForwardIterator last)
{
	return vstd::detail::UninitializedDefault(unwrap_iterator(first),
	                                          unwrap_iterator(last));
}

}

#endif
