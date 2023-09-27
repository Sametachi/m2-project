//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_UNINITIALIZEDDEFAULTN_HPP
#define VSTL_MEMORY_UNINITIALIZEDDEFAULTN_HPP

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

template <class ForwardIterator, typename Size>
ForwardIterator UninitializedDefaultN(ForwardIterator first, Size n)
{
	typedef typename iterator_traits<ForwardIterator>::value_type value_type;

	for ( ; n; ++first, --n)
		new (static_cast<void*>(&*first)) value_type();

	return first;
}

template <class U, typename Size>
typename boost::enable_if_c<
	(sizeof(U) == 1 && boost::has_trivial_default_constructor<U>::value) ||
		boost::is_integral<U>::value,
	U*
>::type UninitializedDefaultN(U* first, Size n)
{
	const auto s = static_cast<std::size_t>(n) * sizeof(U);
	return static_cast<U*>(std::memset(first, U(), s)) + n;
}

}

template <class ForwardIterator, class Size>
BOOST_FORCEINLINE ForwardIterator
	uninitialized_default_n(ForwardIterator first, Size n)
{
	return vstd::detail::UninitializedDefaultN(unwrap_iterator(first), n);
}

}

#endif
