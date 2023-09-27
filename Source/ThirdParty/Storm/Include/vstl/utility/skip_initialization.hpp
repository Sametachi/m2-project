//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_UTILITY_SKIPINITIALIZATION_HPP
#define VSTL_UTILITY_SKIPINITIALIZATION_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

struct skip_initialization_t { };

#if !defined(BOOST_NO_CXX11_CONSTEXPR) && !VSTD_BUILDING_SKIPINITIALIZATION
constexpr skip_initialization_t skip_initialization = skip_initialization_t();
#else
extern const skip_initialization_t skip_initialization;
#endif

}

#endif
