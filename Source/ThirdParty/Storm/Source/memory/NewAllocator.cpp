//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/memory/NewAllocator.hpp>

#include <vstl/memory/align.hpp>

#include <cstdlib>

namespace storm
{

NewAllocator::NewAllocator()
{
	// ctor
}

NewAllocator::~NewAllocator()
{
	// dtor
}

/*virtual*/ void* NewAllocator::AllocateReal(vstd::size_type bytes,
                                             vstd::size_type alignment,
                                             vstd::size_type alignmentOffset,
                                             uint32_t flags)
{
	if (bytes != 0) {
		STORM_ASSERT(alignment > 0, "Alignment must be at least one byte.");

		vstd::size_type actualSize = bytes + (alignment - 1) + sizeof(void*);

		uint8_t* baseAddress = static_cast<uint8_t*>(std::malloc(actualSize));
		if (baseAddress) {
			// Don't count our helper pointer as usable memory.
			actualSize -= sizeof(void*);

			//
			// We just use vstd::align() to align our pointer at the
			// specified alignment offset.
			//

			void* ptr = baseAddress + sizeof(void*) + alignmentOffset;
			vstd::align(alignment, bytes, ptr, actualSize);
			ptr = static_cast<uint8_t*>(ptr) - alignmentOffset;

			reinterpret_cast<uint8_t**>(ptr)[-1] = baseAddress;
			return ptr;
		}
	}

	return nullptr;
}

/*virtual*/ void NewAllocator::DeallocateReal(void* p)
{
	if (p)
		std::free(reinterpret_cast<void**>(p)[-1]);
}

}
