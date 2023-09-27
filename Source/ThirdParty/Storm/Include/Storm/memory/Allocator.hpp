//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_ALLOCATOR_HPP
#define STORM_MEMORY_ALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file memory/Allocator.hpp
/// Defines the Allocator class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// Allocation flags.
///
/// This enum defines the various optional flags
/// an allocation request can have.
struct AllocationFlags
{
	/// (Semi-)permanent object lifetime.
	///
	/// Tells the allocator that this is a (semi-)permanent object,
	/// thus signalling it that this object will live for a long time.
	BOOST_STATIC_CONSTEXPR uint32_t kPerm = (1 << 0);

	/// Temporary object lifetime.
	///
	/// Tells the allocator that this is a temporary object,
	/// thus signalling it that this object will be freed very soon.
	BOOST_STATIC_CONSTEXPR uint32_t kTemp = (1 << 1);

	/// Allocations can fail.
	///
	/// Tells the allocater that it can return NULL in case of
	/// failure instead of raising a fatal error.
	BOOST_STATIC_CONSTEXPR uint32_t kCanFail = (1 << 2);
};

/// Make an allocator name
///
/// Depending on whether memory tracking is enabled,
/// this class either returns @c s unchanged, or @c nullptr.
#if STORM_TRACK_MEMORY
	#define STORM_ALLOC_NAME(s) (s)
#else
	#define STORM_ALLOC_NAME(s) static_cast<const char*>(nullptr)
#endif

class AbstractAllocator;

/// Value-type wrapper around an AbstractAllocator.
///
/// This class is a lightweight value-type wrapper around an AbstractAllocator.
class Allocator
{
	public:
		Allocator(AbstractAllocator* allocator,
		          const char* name = STORM_ALLOC_NAME("Default"));

		Allocator(const Allocator& other);
		Allocator(const Allocator& other, const char* name);

		Allocator& operator=(const Allocator& other);
		Allocator& operator=(AbstractAllocator* allocator);

		/// Allocate memory.
		///
		/// This function allocates at least @c bytes bytes.
		/// The resulting pointer is guranteed to be aligned
		/// to @c alignment at offset @c alignmentOffset.
		///
		/// @param bytes The size to allocate. This is the minimum size for the memory block.
		/// The implementation might choose to allocate a larger block.
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
		/// @note If memory tracking is disabled, the @c name argument
		/// is unused.
		void* Allocate(vstd::size_type bytes,
		               vstd::size_type alignment,
		               vstd::size_type alignmentOffset = 0,
		               uint32_t flags = 0) const;

		/// Deallocate memory.
		///
		/// This function deallocates the given block of memory.
		///
		/// @param p NULL or a pointer returned by Allocate.
		void Deallocate(void* p) const;

#if STORM_TRACK_MEMORY
		/// Get the name of the allocator.
		///
		/// This function returns the name of this allocator instance.
		///
		/// @return The allocator instance's name.
		const char* GetName() const;

		/// Set the name of the allocator.
		///
		/// This function updates the allocator instance's name.
		///
		/// @param name The allocator instance's new name
		void SetName(const char* name);
#endif

		/// Get the underlying AbstractAllocator.
		///
		/// This function returns the used AbstractAllocator.
		///
		/// @return The AbstractAllocator used by this Allocator
		/// instance.
		AbstractAllocator* GetRealAllocator() const;

	private:
		AbstractAllocator* m_allocator;

#if STORM_TRACK_MEMORY
		const char* m_name;
#endif
};

}

#include <storm/memory/Allocator-impl.hpp>

/// @}

#endif
