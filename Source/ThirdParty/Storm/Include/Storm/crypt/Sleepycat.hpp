//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_CRYPT_CRYPT_HPP
#define STORM_CRYPT_CRYPT_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace storm
{

uint32_t SleepycatHash(const char* str, vstd::size_type length);

}

#endif
