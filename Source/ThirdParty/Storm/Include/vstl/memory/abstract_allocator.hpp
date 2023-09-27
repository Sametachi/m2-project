//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_ABSTRACTALLOCATOR_HPP
#define VSTL_MEMORY_ABSTRACTALLOCATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator_base.hpp>

namespace vstd
{

class abstract_allocator : public allocator_base
{
	public:
		BOOST_FORCEINLINE abstract_allocator(const char* name =
				VSTD_ALLOC_NAME(VSTD_DEFAULT_ALLOC_NAME))
			: allocator_base(name)
		{
			// ctor
		}

		virtual ~abstract_allocator()
		{
			// dtor
		}

		virtual void* allocate(size_type bytes, size_type alignment,
		                       size_type alignmentOffset = 0,
		                       uint32_t flags = 0) = 0;

		virtual void deallocate(void* p) = 0;
};

}

#endif
