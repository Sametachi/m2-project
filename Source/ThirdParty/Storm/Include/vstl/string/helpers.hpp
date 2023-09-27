//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_HELPERS_HPP
#define VSTL_STRING_HELPERS_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/traits.hpp>

#include <boost/type_traits/is_same.hpp>
#include <boost/mpl/and.hpp>
#include <boost/mpl/not.hpp>

namespace vstd
{

namespace detail
{

template <class Impl, class Tp>
struct IsNotStringPointer : boost::mpl::and_<
	boost::mpl::not_<boost::is_same<
		Tp, typename Impl::pointer
	> >,
	boost::mpl::not_<boost::is_same<
		Tp, typename Impl::const_pointer
	> >
>
{ };

template <class Impl, class InputIterator>
struct IsStringInputIterator : boost::mpl::and_<
	vstd::is_input_iterator<InputIterator>,
	IsNotStringPointer<Impl, InputIterator>
>
{ };

template <class Impl, class InputIterator>
struct IsOnlyStringInputIterator : boost::mpl::and_<
	vstd::is_input_iterator<InputIterator>,

	// Pointers are forward-iterators too, so we can skip the
	// IsNotStringPointer check here.
	boost::mpl::not_<
		is_forward_iterator<InputIterator>
	>
>
{ };

template <class Impl, class ForwardIterator>
struct IsOnlyStringForwardIterator : boost::mpl::and_<
	IsNotStringPointer<Impl, ForwardIterator>,
	vstd::is_forward_iterator<ForwardIterator>,

	boost::mpl::not_<
		boost::is_same<ForwardIterator, typename Impl::iterator>
	>,

	boost::mpl::not_<
		boost::is_same<ForwardIterator, typename Impl::const_iterator>
	>
>
{ };

}

}

#endif
