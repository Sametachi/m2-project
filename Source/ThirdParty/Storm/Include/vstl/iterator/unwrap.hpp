//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_UNWRAP_HPP
#define VSTL_ITERATOR_UNWRAP_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/traits.hpp>

#include <boost/utility/enable_if.hpp>

namespace vstd
{

template <class T>
class is_iterator_wrapper
{
	struct Yes { char d[2]; };
	struct No { char d[1]; };

	template <class U>
	static Yes Test(decltype(&U::base));

	template <class U>
	static No Test(...);

	public:
		static const bool value = sizeof(Test<T>(0)) == sizeof(Yes);
};

template <class Iterator>
BOOST_FORCEINLINE typename boost::enable_if<
	is_iterator_wrapper<Iterator>,
	typename Iterator::base_type
>::type unwrap_iterator(Iterator it)
{
	return it.base();
}

template <class Iterator>
BOOST_FORCEINLINE typename boost::disable_if<
	is_iterator_wrapper<Iterator>,
	Iterator
>::type unwrap_iterator(Iterator it)
{
	return it;
}

}

#endif
