//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_ITERATOR_HPP
#define VSTL_ITERATOR_ITERATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <cstddef>

namespace vstd
{

template<class Category, class Tp, class Distance = ptrdiff_t,
         class Pointer = Tp*, class Reference = Tp&>
struct iterator
{
    typedef Tp value_type;
    typedef Distance difference_type;
    typedef Pointer pointer;
    typedef Reference reference;
    typedef Category iterator_category;
};

}

#endif
