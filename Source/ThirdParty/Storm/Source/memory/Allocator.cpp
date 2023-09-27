//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/memory/AbstractAllocator.hpp>
#include <storm/memory/Allocator.hpp>

namespace storm
{

void* Allocator::Allocate(vstd::size_type bytes,
                          vstd::size_type alignment,
                          vstd::size_type alignmentOffset,
                          uint32_t flags) const
{
	STORM_ASSERT(m_allocator, "No AbstractAllocator assigned");
	return m_allocator->AllocateReal(bytes, alignment, alignmentOffset, flags
#if STORM_TRACK_MEMORY
		, m_name
#endif
	);
}

void Allocator::Deallocate(void* p) const
{
	STORM_ASSERT(m_allocator, "No AbstractAllocator assigned");
	m_allocator->DeallocateReal(p);
}

}
