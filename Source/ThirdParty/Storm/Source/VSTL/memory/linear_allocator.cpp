//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/memory/linear_allocator.hpp>
#include <vstl/memory/align.hpp>

namespace vstd
{

void* linear_allocator::allocate(size_type bytes, size_type alignment,
								size_type alignmentOffset, uint32_t flags)
{
	if (bytes != 0) {
		VSTD_ASSERT(alignment > 0, "Alignment must be at least one byte.");

		size_type space, prevSpace;
		space = prevSpace = static_cast<size_type>(m_end - m_ptr);

		void* ptr = m_ptr + alignmentOffset;
		if (align(alignment, bytes, ptr, space)) {
			m_ptr += (prevSpace - space) + bytes;
			return static_cast<uint8_t*>(ptr) - alignmentOffset;
		}
	}

	return nullptr;
}

}
