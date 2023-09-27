//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_ABSTRACTALLOCATOR_HPP
#define STORM_MEMORY_ABSTRACTALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>

/// @file memory/AbstractAllocator.hpp
/// Defines the AbstractAllocator class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// Abstract base class for memory allocators.
///
/// This class specifies the minimum interface all memory allocator
/// need to implement.
///
/// @note If memory tracking is enabled, this class will hold a pointer
/// to the allocator's name.
class AbstractAllocator : public Allocator
{
	// Needed because Allocator needs to call (De)AllocateReal
	friend class Allocator;

	protected:
		AbstractAllocator();
		virtual ~AbstractAllocator();

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
		virtual void* AllocateReal(vstd::size_type bytes,
		                           vstd::size_type alignment,
		                           vstd::size_type alignmentOffset,
		                           uint32_t flags) = 0;

		/// Deallocate memory.
		///
		/// This function deallocates the given block of memory.
		///
		/// @param p NULL or a pointer returned by Allocate.
		virtual void DeallocateReal(void* p) = 0;
};

/// Get the app-global default memory allocator.
///
/// This functions returns the global memory allocator.
///
/// @return The global default memory allocator.
AbstractAllocator* GetDefaultAllocator();

/// Set the app-global default memory allocator.
///
/// This functions sets the global memory allocator.
///
/// @param allocator The new global default memory allocator.
void SetDefaultAllocator(AbstractAllocator* allocator);

}

/// @}

#endif
