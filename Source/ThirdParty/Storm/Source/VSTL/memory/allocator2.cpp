//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/memory/allocator.hpp>
#include <vstl/memory/align.hpp>

#include <cstdlib>

namespace vstd
{

namespace
{

class DefaultAllocator : public abstract_allocator
{
	public:
		DefaultAllocator()
			: abstract_allocator(VSTD_ALLOC_NAME("Default"))
		{
			// ctor
		}

		virtual void* allocate(size_type bytes, size_type alignment,
		                       size_type alignmentOffset, uint32_t flags);

		virtual void deallocate(void* p);
};

/*virtual*/ void* DefaultAllocator::allocate(
	size_type bytes,
	size_type alignment, size_type alignmentOffset,
	uint32_t flags
)
{
	if (bytes != 0) {
		VSTD_ASSERT(alignment > 0, "Alignment must be at least one byte.");

		size_type actualSize = bytes + (alignment - 1) + sizeof(void*);

		uint8_t* baseAddress = reinterpret_cast<uint8_t*>(malloc(actualSize));
		if (baseAddress) {
			actualSize -= sizeof(void*);

			void* ptr = baseAddress + sizeof(void*) + alignmentOffset;
			align(alignment, bytes, ptr, actualSize);
			ptr = static_cast<uint8_t*>(ptr) - alignmentOffset;

			reinterpret_cast<uint8_t**>(ptr)[-1] = baseAddress;
			return ptr;
		}
	}

	return nullptr;
}

/*virtual*/ void DefaultAllocator::deallocate(void* p)
{
	if (p)
		free(reinterpret_cast<void**>(p)[-1]);
}

abstract_allocator* GetDefaultAllocator()
{
	static DefaultAllocator defaultAlloc;
	return &defaultAlloc;
}

}

allocator::allocator(abstract_allocator* impl, const char* name)
	: allocator_base(name)
	, m_impl(impl ? impl : GetDefaultAllocator())
{
	// ctor
}

}
