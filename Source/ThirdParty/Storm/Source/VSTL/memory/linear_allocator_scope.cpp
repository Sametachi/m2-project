//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/memory/linear_allocator_scope.hpp>

namespace vstd
{

linear_allocator_scope::linear_allocator_scope(linear_allocator& alloc,
											const char* name)
	: allocator_base(name)
	, m_alloc(alloc)
	, m_rewindPoint(alloc.ptr())
	, m_finalizerChain(nullptr)
{
	// ctor
}

linear_allocator_scope::
		linear_allocator_scope(const linear_allocator_scope& other)
	: allocator_base(other)
	, m_alloc(other.m_alloc)
	, m_rewindPoint(other.m_alloc.ptr())
	, m_finalizerChain(nullptr)
{
	// ctor
}

linear_allocator_scope::~linear_allocator_scope()
{
	for (Finalizer* f = m_finalizerChain; f; f = f->next)
		f->dtor(&f[1]);
}

void* linear_allocator_scope::
	allocate(size_type bytes, size_type alignment,
			size_type alignmentOffset, uint32_t flags)
{
	if (bytes != 0) {
		VSTD_ASSERT(alignment > 0, "Alignment must be at least one byte.");
		return m_alloc.allocate(bytes, alignment, alignmentOffset, flags);
	}

	return nullptr;
}

linear_allocator_scope::Finalizer* linear_allocator_scope::
	AllocateWithFinalizer(size_type bytes,
						size_type alignment)
{
	if (bytes != 0) {
		VSTD_ASSERT(alignment > 0, "Alignment must be at least one byte.");

		void* ptr = m_alloc.allocate(bytes, alignment, sizeof(Finalizer), 0);
		if (ptr) {
			Finalizer* f = static_cast<Finalizer*>(ptr);

			f->next = m_finalizerChain;
			m_finalizerChain = f;

			return f;
		}
	}

	return nullptr;
}

}
