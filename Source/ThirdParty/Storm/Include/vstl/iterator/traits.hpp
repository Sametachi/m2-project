//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_TRAITS_HPP
#define VSTL_ITERATOR_TRAITS_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#if VSTD_USE_STD_ITERATOR_CAT
	#include <iterator>
#endif

#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/mpl/or.hpp>

namespace vstd
{

#if VSTD_USE_STD_ITERATOR_CAT

using std::input_iterator_tag;
using std::output_iterator_tag;
using std::forward_iterator_tag;
using std::bidirectional_iterator_tag;
using std::random_access_iterator_tag;

#else

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : public input_iterator_tag {};
struct bidirectional_iterator_tag : public forward_iterator_tag {};
struct random_access_iterator_tag : public bidirectional_iterator_tag {};

#endif

template <class Iterator, bool HasCategory>
struct is_category_valid : public boost::false_type
{};

template <class Iterator>
struct is_category_valid<Iterator, true> : boost::mpl::or_<
	boost::is_convertible<typename Iterator::iterator_category,
	                      input_iterator_tag>,
	boost::is_convertible<typename Iterator::iterator_category,
	                      output_iterator_tag>
>
{};

template <class T>
class is_iterator_category_valid
{
	struct Yes { char d[2]; };
	struct No { char d[1]; };

	template <class U>
	static Yes Test(typename U::iterator_category*);

	template <class U>
	static No Test(...);

	public:
		static const bool value =
			is_category_valid<T, sizeof(Test<T>(0)) == sizeof(Yes)>::value;
};

template <class Iterator, bool HasValidCategory>
struct real_iterator_traits
{};

template <class Iterator>
struct real_iterator_traits<Iterator, true>
{
	typedef typename Iterator::difference_type difference_type;
	typedef typename Iterator::value_type value_type;
	typedef typename Iterator::pointer pointer;
	typedef typename Iterator::reference reference;
	typedef typename Iterator::iterator_category iterator_category;
};

template <class Iterator>
struct iterator_traits
	: real_iterator_traits<Iterator, is_iterator_category_valid<Iterator>::value>
{};

template <class T>
struct iterator_traits<T*>
{
	typedef ptrdiff_t difference_type;
	typedef typename boost::remove_const<T>::type value_type;
	typedef T* pointer;
	typedef T& reference;
	typedef random_access_iterator_tag iterator_category;
};

template <class Iterator, class Tag, bool HasValidCategory =
          is_iterator_category_valid< iterator_traits<Iterator> >::value>
struct has_iterator_category_convertible_to : boost::is_convertible<
	typename iterator_traits<Iterator>::iterator_category,
	Tag
>
{};

template <class Iterator, class Tag>
struct has_iterator_category_convertible_to<Iterator, Tag, false>
	: boost::mpl::false_
{};

template <class Iterator>
struct is_input_iterator : has_iterator_category_convertible_to<
	Iterator, input_iterator_tag
>
{};

template <class Iterator>
struct is_forward_iterator : has_iterator_category_convertible_to<
	Iterator, forward_iterator_tag
>
{};

template <class Iterator>
struct is_bidirectional_iterator : has_iterator_category_convertible_to<
	Iterator, bidirectional_iterator_tag
>
{};

template <class Iterator>
struct is_random_access_iterator : has_iterator_category_convertible_to<
	Iterator, random_access_iterator_tag
>
{};

}

#endif
