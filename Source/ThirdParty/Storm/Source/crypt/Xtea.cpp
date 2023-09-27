//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/crypt/Xtea.hpp>

#include <boost/preprocessor/repetition/repeat.hpp>

#include <cstring>

namespace storm
{

enum { kXteaRounds = 32 };

static const uint32_t kXteaDelta = 0x9E3779B9u;
static const uint32_t kXteaLimit = kXteaRounds * kXteaDelta;

#define EncipherXteaInnerLoop(i1, i2, i3) \
do { v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]); \
	sum += kXteaDelta; \
	v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]); } while (false);

void EncipherXtea(const uint32_t* src, uint32_t* out, const uint32_t key[4])
{
	uint32_t v0 = src[0],
	         v1 = src[1],
	         sum = 0;

	static_assert (32 == kXteaRounds, "Consistency check!");
	BOOST_PP_REPEAT(32, EncipherXteaInnerLoop, char unused)

	out[0] = v0, out[1] = v1;
}

#undef EncipherXteaInnerLoop

#define DecipherXteaInnerLoop(i1, i2, i3) \
do { v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]); \
	sum -= kXteaDelta; \
	v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]); } while (false);

void DecipherXtea(const uint32_t* src, uint32_t* out, const uint32_t key[4])
{
	uint32_t v0 = src[0],
	         v1 = src[1],
	         sum = kXteaDelta * kXteaRounds;

	static_assert (32 == kXteaRounds, "Consistency check!");
	BOOST_PP_REPEAT(32, DecipherXteaInnerLoop, char unused)

	out[0] = v0, out[1] = v1;
}

#undef DecipherXteaInnerLoop

uint32_t EncryptXtea(uint8_t* dst, const uint8_t* src,
                     const uint32_t key[4], uint32_t size)
{
	const uint8_t remaining = size & 7;

	size &= 0xFFFFFFF8;

	for (uint32_t i = 0; i < size; i += 8) {
		EncipherXtea(reinterpret_cast<const uint32_t*>(src + i),
		             reinterpret_cast<uint32_t*>(dst + i),
		             key);
	}

	if (remaining) {
		uint32_t dummy[2] = {0, 0};

		memcpy(dummy, src + size, remaining);

		EncipherXtea(dummy,
		             reinterpret_cast<uint32_t*>(dst + size),
		             key);

		size += 8;
	}

	return size;
}

uint32_t DecryptXtea(uint8_t* dst, const uint8_t* src,
                     const uint32_t key[4], uint32_t size)
{
	size &= 0xFFFFFFF8;

	for (uint32_t i = 0; i < size; i += 8) {
		DecipherXtea(reinterpret_cast<const uint32_t*>(src + i),
		             reinterpret_cast<uint32_t*>(dst + i),
		             key);
	}

	return size;
}

}
