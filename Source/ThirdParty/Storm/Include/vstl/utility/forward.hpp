//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_UTILITY_FORWARD_HPP
#define VSTL_UTILITY_FORWARD_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/mpl/and.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/remove_reference.hpp>

namespace vstd
{

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template<typename T>
BOOST_CONSTEXPR T&&
    forward(typename boost::remove_reference<T>::type& t) BOOST_NOEXCEPT
{ return static_cast<T&&>(t); }

template<typename T>
BOOST_CONSTEXPR T&&
	forward(typename boost::remove_reference<T>::type&& t) BOOST_NOEXCEPT
{
	return static_cast<T&&>(t);
}

#endif

}

#endif
