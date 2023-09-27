//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_TYPETRAITS_ISTRIVIALLYPOSTMOVEDESTRUCTIBLE_HPP
#define VSTL_TYPETRAITS_ISTRIVIALLYPOSTMOVEDESTRUCTIBLE_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/type_traits/has_trivial_destructor.hpp>

namespace vstd
{

template <typename T>
struct is_trivially_post_move_destructible
	: public boost::has_trivial_destructor<T>
{ };

}

#endif
