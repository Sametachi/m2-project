//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_UTILITY_PAIR_HPP
#define VSTL_UTILITY_PAIR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/utility/move.hpp>
#include <vstl/utility/forward.hpp>

#include <boost/mpl/and.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_convertible.hpp>

#if VSTD_USE_STD_PAIR
	#include <utility>
#endif

namespace vstd
{

#if !VSTD_USE_STD_PAIR

template <class T1, class T2>
struct pair
{
	typedef T1 first_type;
	typedef T2 second_type;

	BOOST_CONSTEXPR pair()
		: first()
		, second()
	{
		// ctor
	}

	BOOST_CONSTEXPR pair(const T1& first, const T2& second)
		: first(first)
		, second(second)
	{
		// ctor
	}

	template <class U1, class U2>
	BOOST_CONSTEXPR pair(const pair<U1, U2>& p,
			typename boost::enable_if<boost::mpl::and_<
				boost::is_convertible<const U1&, T1>,
				boost::is_convertible<const U2&, T2> > >::type* = 0)
		: first(p.first)
		, second(p.second)
	{
		// ctor
	}

#ifndef BOOST_NO_CXX11_DEFAULTED_FUNCTIONS

	BOOST_CONSTEXPR pair(const pair& p) = default;
	BOOST_CONSTEXPR pair(pair&& p) = default;

#else

	BOOST_CONSTEXPR pair(const pair& p)
		: first(p.first)
		, second(p.second)
	{
		// ctor
	}

	BOOST_CONSTEXPR pair(pair&& p)
		: first(vstd::forward<first_type>(p.first))
		, second(vstd::forward<second_type>(p.second))
	{
		// ctor
	}

#endif

	pair& operator=(const pair& p)
	{
		first = p.first;
		second = p.second;
		return *this;
	}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

	template <class U1, class U2>
	BOOST_CONSTEXPR pair(U1&& first, U2&& second, typename boost::enable_if<boost::mpl::and_<
			boost::is_convertible<U1, first_type>,
			boost::is_convertible<U2, second_type> > >::type* = 0)
		: first(vstd::forward<U1>(first))
		, second(vstd::forward<U2>(second))
	{
		// ctor
	}

	template <class U1, class U2>
	BOOST_CONSTEXPR pair(pair<U1, U2>&& p,
			typename boost::enable_if<boost::mpl::and_<
				boost::is_convertible<U1, T1>,
				boost::is_convertible<U2, T2> > >::type* = 0)
		: first(vstd::forward<U1>(p.first))
		, second(vstd::forward<U2>(p.second))
	{
		// ctor
	}


	pair& operator=(pair&& p)
	{
		first = vstd::forward<first_type>(p.first);
		second = vstd::forward<second_type>(p.second);
		return *this;
	}

#endif

	void swap(pair& p)
	{
		vstd::swap(first, p.first);
		vstd::swap(second, p.second);
	}

	T1 first;
	T2 second;
};

#else

using std::pair;

#endif

}

#endif
