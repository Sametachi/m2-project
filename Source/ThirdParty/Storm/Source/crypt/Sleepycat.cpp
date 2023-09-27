//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/crypt/Sleepycat.hpp>

namespace storm
{

uint32_t SleepycatHash(const char* str, vstd::size_type length)
{
	uint32_t hash = 0;
	while (length--) {
		const uint32_t c = static_cast<uint8_t>(*str++);

		hash = c + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

}
