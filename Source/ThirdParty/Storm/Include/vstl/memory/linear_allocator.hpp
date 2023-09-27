//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_LINEARALLOCATOR_HPP
#define VSTL_MEMORY_LINEARALLOCATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator.hpp>

#include <boost/noncopyable.hpp>

namespace vstd
{

#ifndef VSTD_DEFAULT_LINEAR_ALLOC_NAME
	#define VSTD_DEFAULT_LINEAR_ALLOC_NAME "vstl-linear"
#endif

class linear_allocator : public allocator_base, boost::noncopyable
{
	public:
		linear_allocator(uint8_t* ptr, size_type size, const char* name =
						VSTD_ALLOC_NAME(VSTD_DEFAULT_LINEAR_ALLOC_NAME));

		void* allocate(size_type bytes, size_type alignment,
					size_type alignmentOffset = 0,
					uint32_t flags = 0);

		void deallocate(void* /*p*/);

		uint8_t* ptr();
		void rewind(uint8_t* ptr);

		bool operator==(const linear_allocator& other) const BOOST_NOEXCEPT;
		bool operator!=(const linear_allocator& other) const BOOST_NOEXCEPT;

	private:
		uint8_t* m_ptr;
		uint8_t* m_end;
};

BOOST_FORCEINLINE linear_allocator::linear_allocator(uint8_t* ptr,
								size_type size, const char* name)
	: allocator_base(name)
	, m_ptr(ptr)
	, m_end(ptr + size)
{
	// ctor
}

BOOST_FORCEINLINE void linear_allocator::deallocate(void* /*p*/)
{
	// empty
}

BOOST_FORCEINLINE uint8_t* linear_allocator::ptr()
{
	return m_ptr;
}

BOOST_FORCEINLINE void linear_allocator::rewind(uint8_t* ptr)
{
	m_ptr = ptr;
}

BOOST_FORCEINLINE bool linear_allocator::
	operator==(const linear_allocator& other) const BOOST_NOEXCEPT
{
	return m_ptr == other.m_ptr && m_end == other.m_end;
}

BOOST_FORCEINLINE bool linear_allocator::
	operator!=(const linear_allocator& other) const BOOST_NOEXCEPT
{
	return m_ptr != other.m_ptr || m_end != other.m_end;
}

}

#endif
