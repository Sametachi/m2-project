//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_CRYPT_XTEA_HPP
#define STORM_CRYPT_XTEA_HPP

#include <storm/Config.hpp>

namespace storm
{

// For internal usage only.
void EncipherXtea(const uint32_t* src, uint32_t* out, const uint32_t key[4]);
void DecipherXtea(const uint32_t* src, uint32_t* out, const uint32_t key[4]);

// Encrypts data with the XTEA algorithm
uint32_t EncryptXtea(uint8_t* dst, const uint8_t* src,
                     const uint32_t key[4], uint32_t size);

// Decrypts XTEA encrypted data
uint32_t DecryptXtea(uint8_t* dst, const uint8_t* src,
                     const uint32_t key[4], uint32_t size);

}

#endif
