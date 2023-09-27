//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/memory/align.hpp>

namespace vstd
{

void* align(size_type alignment, size_type size, void*& ptr, size_type& space)
{
	if (size <= space) {
		const size_type rest = reinterpret_cast<uintptr_t>(ptr) % alignment;
		if (rest != 0) {
			size_type bytesAligned = alignment - rest;
			if (bytesAligned <= space - size) {
				space -= bytesAligned;

				uint8_t* p = static_cast<uint8_t*>(ptr) + bytesAligned;
				ptr = p;
				return p;
			}
		} else {
			return ptr;
		}
	}

	return nullptr;
}

}
