//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_FOREACH_HPP
#define VSTL_ALGORITHM_FOREACH_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/utility/move.hpp>

namespace vstd
{

template <class InputIterator, class UnaryFunction>
UnaryFunction for_each(InputIterator first, InputIterator last,
                       UnaryFunction func)
{
	for ( ; first != last; ++first)
		func(*first);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
	return vstd::move(func);
#else
	return func;
#endif
}

}

#endif
