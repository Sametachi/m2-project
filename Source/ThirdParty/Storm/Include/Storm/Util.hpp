//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_UTIL_HPP
#define STORM_UTIL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace storm
{

#ifndef STORM_ARRAYSIZE
	#define STORM_ARRAYSIZE(obj) (sizeof(obj) / sizeof(obj[0]))
#endif

#ifndef STORM_MAKEFOURCC
	#define STORM_MAKEFOURCC(b0, b1, b2, b3) \
		(uint32_t(uint8_t(b0)) | (uint32_t(uint8_t(b1)) << 8) | \
		(uint32_t(uint8_t(b2)) << 16) | (uint32_t(uint8_t(b3)) << 24))
#endif

}

#endif
