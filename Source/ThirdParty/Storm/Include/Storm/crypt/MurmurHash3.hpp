//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef STORM_CRYPT_MURMURHASH3_HPP
#define STORM_CRYPT_MURMURHASH3_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace storm
{

//
// Keep the old names. A new NS is not necessary here, as all functions
// are prefixed with MurmurHash3_*.
//

void MurmurHash3_x86_32(const void * key, int len, uint32_t seed, void * out);
void MurmurHash3_x86_128(const void * key, int len, const uint32_t* seed, void * out);
void MurmurHash3_x64_128(const void * key, int len, uint32_t seed, void * out);

}

#endif
