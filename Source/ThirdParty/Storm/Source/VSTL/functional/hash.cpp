//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/functional/hash.hpp>

#if VSTD_COMPILER_MSVC >= 1400
	#include <intrin.h>
#endif

namespace vstd
{

namespace detail
{

namespace
{

BOOST_FORCEINLINE uint32_t rotl32(uint32_t x, int8_t r)
{
#if VSTD_COMPILER_MSVC >= 1400
    return _rotl(x, static_cast<int>(r));
#else
	return (x << r) | (x >> (32 - r));
#endif
}

BOOST_FORCEINLINE uint32_t getblock(const uint32_t * p, int i)
{
	return p[i];
}

BOOST_FORCEINLINE uint32_t fmix(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

}

size_type GetMurmurHash3(const void* data, size_type size)
{
	const uint32_t c1 = 0xcc9e2d51;
	const uint32_t c2 = 0x1b873593;
	uint32_t h1 = 0xdeadbeef;

	const int nblocks = size / 4;

	const uint32_t* blocks = static_cast<const uint32_t*>(data) + nblocks;
	for (int i = -nblocks; i; ++i) {
		uint32_t k1 = getblock(blocks, i);

		k1 *= c1;
		k1 = rotl32(k1, 15);
		k1 *= c2;

		h1 ^= k1;
		h1 = rotl32(h1, 13);
		h1 = h1 * 5 + 0xe6546b64;
	}

	const uint8_t * tail = static_cast<const uint8_t*>(data) + nblocks * 4;
	uint32_t k1 = 0;

	switch (size & 3) {
		case 3: k1 ^= tail[2] << 16;
		case 2: k1 ^= tail[1] << 8;
		case 1: k1 ^= tail[0];
				k1 *= c1;
				k1 = rotl32(k1, 15);
				k1 *= c2;
				h1 ^= k1;
	}

	h1 ^= size;
	h1 = fmix(h1);
	return h1;
}

}

}
