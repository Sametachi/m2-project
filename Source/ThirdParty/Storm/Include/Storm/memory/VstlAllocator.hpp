//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_VSTLALLOCATOR_HPP
#define STORM_MEMORY_VSTLALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>
#include <storm/memory/AbstractAllocator.hpp>

namespace storm
{

#define STORM_VSTL_DEFAULT_ALLOC_NAME "storm::VstlAllocator"

/// Storm allocator to vstl allocator wrapper.
///
/// This class stores a pointer to an allocator implementing
/// the storm::AbstractAllocator interface and acts as a bridge between
/// the allocators the vstl library expects and the allocators Storm provides.
class VstlAllocator
{
	public:
		/// Construct a new VstlAllocator.
		///
		/// This constructor initializes a new VstlAllocator with the current
		/// default allocator.
		VstlAllocator();

		/// Construct a new VstlAllocator.
		///
		/// This constructor initializes a new VstlAllocator.
		///
		/// @param alloc The Storm allocator to use
		VstlAllocator(const Allocator& alloc);

		/// @brief Copy-constructs a new VstlAllocator
		///
		/// @param other The VstlAllocator to copy from.
		VstlAllocator(const VstlAllocator& other);

		/// @brief Assigns the allocator @c other to ourself.
		///
		/// @param other The VstlAllocator to copy from.
		VstlAllocator& operator=(const VstlAllocator& other);

		/// Get the underlying Storm allocator.
		///
		/// This function returns the used Storm allocator.
		///
		/// @return The Storm allocator used by this VstlAllocator
		/// instance.
		const Allocator& GetRealAllocator() const;

#if VSTD_TRACK_MEMORY
		/// Get the allocator's name.
		///
		/// This function returns the allocator's name
		///
		/// @return The allocator's name
		const char* get_name() const;

		/// Set the allocator's name.
		///
		/// This function sets the allocator's name
		///
		/// @param name The allocator's name
		void set_name(const char* name);
#endif

		/// Allocate a block of memory.
		///
		/// This function allocates at least @c bytes bytes.
		/// The resulting pointer is guranteed to be aligned
		/// to @c alignment at offset @c alignmentOffset.
		///
		/// @param bytes The number of bytes to allocate.
		///
		/// @param alignment The required alignment.
		///
		/// @param alignmentOffset The position of the alignment target (i.e.
		/// the offset of the first byte that has to be aligned to @c alignment).
		///
		/// @param flags Additional information regarding this allocation request.
		///
		/// @return A pointer to the newly allocated block.
		///
		/// @remarks vstl allocator flags are automatically converted to
		/// the Storm allocator flags, if necessary.
		void* allocate(vstd::size_type bytes,
		               vstd::size_type alignment,
		               vstd::size_type alignmentOffset = 0,
		               uint32_t flags = 0);

		/// Free memory.
		///
		/// This function deallocates the given block of memory.
		///
		/// @param p NULL or a pointer returned by Allocate.
		void deallocate(void* p);

	private:
		Allocator m_allocator;
};

BOOST_FORCEINLINE VstlAllocator::VstlAllocator()
	: m_allocator(GetDefaultAllocator())
{
	// ctor
}

BOOST_FORCEINLINE VstlAllocator::VstlAllocator(const Allocator& alloc)
	: m_allocator(alloc)
{
	// ctor
}

BOOST_FORCEINLINE VstlAllocator::VstlAllocator(const VstlAllocator& other)
	: m_allocator(other.m_allocator)
{
	// ctor
}

BOOST_FORCEINLINE VstlAllocator& VstlAllocator::
	operator=(const VstlAllocator& other)
{
	m_allocator = other.m_allocator;
	return *this;
}

BOOST_FORCEINLINE const Allocator& VstlAllocator::GetRealAllocator() const
{ return m_allocator; }

#if VSTD_TRACK_MEMORY
BOOST_FORCEINLINE const char* VstlAllocator::get_name() const
{
#if STORM_TRACK_MEMORY
	return m_allocator.GetName();
#else
	return STORM_VSTL_DEFAULT_ALLOC_NAME;
#endif
}

BOOST_FORCEINLINE void VstlAllocator::set_name(const char* name)
{
#if STORM_TRACK_MEMORY
	m_allocator.SetName(name);
#endif
}
#endif

BOOST_FORCEINLINE void* VstlAllocator::allocate(vstd::size_type bytes,
                                                vstd::size_type alignment,
                                                vstd::size_type alignmentOffset,
                                                uint32_t flags)
{
	// Currently there's no translation required, as we chose the
	// values very carefully.
	return m_allocator.Allocate(bytes, alignment,
	                            alignmentOffset, flags);
}

BOOST_FORCEINLINE void VstlAllocator::deallocate(void* p)
{
	m_allocator.Deallocate(p);
}

BOOST_FORCEINLINE bool operator==(const VstlAllocator& lhs,
                                  const VstlAllocator& rhs)
{ return lhs.GetRealAllocator() == rhs.GetRealAllocator(); }

BOOST_FORCEINLINE bool operator!=(const VstlAllocator& lhs,
                                  const VstlAllocator& rhs)
{ return lhs.GetRealAllocator() != rhs.GetRealAllocator(); }

}

#endif
