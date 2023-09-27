//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_ALLOCATOR_HPP
#define VSTL_MEMORY_ALLOCATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator_base.hpp>
#include <vstl/memory/abstract_allocator.hpp>

namespace vstd
{

class allocator : public allocator_base
{
	public:
		allocator(abstract_allocator* impl = nullptr,
		          const char* name = VSTD_ALLOC_NAME(VSTD_DEFAULT_ALLOC_NAME));

		abstract_allocator* impl() BOOST_NOEXCEPT;
		const abstract_allocator* impl() const BOOST_NOEXCEPT;

		void* allocate(size_type bytes, size_type alignment,
		               size_type alignmentOffset = 0,
		               uint32_t flags = 0);

		void deallocate(void* p);

	private:
		abstract_allocator* m_impl;
};

BOOST_FORCEINLINE abstract_allocator* allocator::impl() BOOST_NOEXCEPT
{ return m_impl; }

BOOST_FORCEINLINE const abstract_allocator* allocator::impl()
	const BOOST_NOEXCEPT
{ return m_impl; }

BOOST_FORCEINLINE void* allocator::allocate(size_type bytes,
                                            size_type alignment,
                                            size_type alignmentOffset,
                                            uint32_t flags)
{
	return m_impl->allocate(bytes, alignment, alignmentOffset, flags);
}

BOOST_FORCEINLINE void allocator::deallocate(void* p)
{ m_impl->deallocate(p); }

BOOST_FORCEINLINE bool operator==(const allocator& a, const allocator& b)
{ return a.impl() == b.impl(); }

BOOST_FORCEINLINE bool operator!=(const allocator& a, const allocator& b)
{ return a.impl() != b.impl(); }

}

#endif
