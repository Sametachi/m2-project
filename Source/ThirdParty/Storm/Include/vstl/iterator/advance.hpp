//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_ADVANCE_HPP
#define VSTL_ITERATOR_ADVANCE_HPP

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

template <class RandomAccessIterator, class Distance>
void AdvanceImpl(RandomAccessIterator& it, Distance n,
                 random_access_iterator_tag)
{
	it += n;
}

template <class InputIterator, class Distance>
void AdvanceImpl(InputIterator& it, Distance n,
                 input_iterator_tag)
{
	while (n--)
		++it;
}

}

template <class InputIterator, class Distance>
BOOST_FORCEINLINE void advance(InputIterator& it, Distance n)
{
	return vstd::detail::AdvanceImpl(
		it, n,
		typename iterator_traits<InputIterator>::iterator_category()
	);
}

}

#endif
