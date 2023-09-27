//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_DISTANCE_HPP
#define VSTL_ITERATOR_DISTANCE_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/iterator.hpp>
#include <vstl/iterator/traits.hpp>

namespace vstd
{

namespace detail
{

template <class RandomAccessIterator>
typename iterator_traits<RandomAccessIterator>::difference_type
	DistanceImpl(RandomAccessIterator first, RandomAccessIterator last,
				random_access_iterator_tag)
{
	return last - first;
}

template <class InputIterator>
typename iterator_traits<InputIterator>::difference_type
	DistanceImpl(InputIterator first, InputIterator last,
				input_iterator_tag)
{
	typename iterator_traits<InputIterator>::difference_type count(0);
	for ( ; first != last; ++first)
		++count;

	return count;
}

}

template <class InputIterator>
BOOST_FORCEINLINE typename iterator_traits<InputIterator>::difference_type
	distance(InputIterator first, InputIterator last)
{
	return vstd::detail::DistanceImpl(first, last,
			typename iterator_traits<InputIterator>::iterator_category());
}

}

#endif
