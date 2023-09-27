//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_ALLOCATORADAPTOR_HPP
#define VSTL_MEMORY_ALLOCATORADAPTOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator_base.hpp>

namespace vstd
{

template <class Allocator>
class allocator_adaptor
{
	public:
		BOOST_FORCEINLINE allocator_adaptor(Allocator* alloc)
			: m_allocator(alloc)
		{
			// ctor
		}

		BOOST_FORCEINLINE allocator_adaptor(const allocator_adaptor& other)
			: m_allocator(other.m_allocator)
		{
			// ctor
		}

		BOOST_FORCEINLINE allocator_adaptor&
			operator=(const allocator_adaptor& other)
		{
			VSTD_ASSERT(other.m_allocator, "allocator_adaptor is uninitialized");
			m_allocator = other.m_allocator;
		}

		BOOST_FORCEINLINE Allocator& base() const
		{
			VSTD_ASSERT(m_allocator, "allocator_adaptor is uninitialized");
			return *m_allocator;
		}

#if VSTD_TRACK_MEMORY
		BOOST_FORCEINLINE const char* get_name() const
		{
			VSTD_ASSERT(m_allocator, "allocator_adaptor is uninitialized");
			return m_allocator->get_name();
		}

		BOOST_FORCEINLINE void set_name(const char* name)
		{
			VSTD_ASSERT(m_allocator, "allocator_adaptor is uninitialized");
			m_allocator->set_name(name);
		}
#endif

		BOOST_FORCEINLINE void* allocate(size_type bytes,
						size_type alignment,
						size_type alignmentOffset = 0,
						uint32_t flags = 0)
		{
			VSTD_ASSERT(m_allocator, "allocator_adaptor is uninitialized");
			return m_allocator->allocate(bytes, alignment, alignmentOffset,
										flags);
		}

		BOOST_FORCEINLINE void deallocate(void* p)
		{
			VSTD_ASSERT(m_allocator, "allocator_adaptor is uninitialized");
			m_allocator->deallocate(p);
		}

	private:
		mutable Allocator* m_allocator;
};

template <class Allocator>
BOOST_FORCEINLINE bool operator==(const allocator_adaptor<Allocator>& lhs,
								const allocator_adaptor<Allocator>& rhs)
{ return lhs.base() == rhs.base(); }

template <class Allocator>
BOOST_FORCEINLINE bool operator!=(const allocator_adaptor<Allocator>& lhs,
								const allocator_adaptor<Allocator>& rhs)
{ return lhs.base() != rhs.base(); }

}

#endif
