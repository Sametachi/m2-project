//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_NEWALLOCATOR_HPP
#define STORM_MEMORY_NEWALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/AbstractAllocator.hpp>

/// @file memory/NewAllocator.hpp
/// Defines the NewAllocator class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// Simple 'operator new'-based allocator.
///
/// This class implements a simple Storm allocator
/// which just uses malloc/new.
///
/// @note This class just forwards all calls to the standard
/// memory APIs.
class NewAllocator : public AbstractAllocator
{
	public:
		/// Construct a NewAllocator.
		///
		/// This constructor does nothing
		/// (apart from setting the name, if this is turned on).
		NewAllocator();

		/// Destruct a NewAllocator.
		///
		/// This destructor does nothing.
		~NewAllocator();

	protected:
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
		/*virtual*/ void* AllocateReal(vstd::size_type bytes,
		                               vstd::size_type alignment,
		                               vstd::size_type alignmentOffset,
		                               uint32_t flags);

		/// Deallocate memory.
		///
		/// This function deallocates the given block of memory.
		///
		/// @param p NULL or a pointer returned by Allocate.
		/*virtual*/ void DeallocateReal(void* p);
};

}

/// @}

#endif
