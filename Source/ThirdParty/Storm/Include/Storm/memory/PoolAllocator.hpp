//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_POOLALLOCATOR_HPP
#define STORM_MEMORY_POOLALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/AbstractAllocator.hpp>

#include <boost/type_traits/alignment_of.hpp>
#include <boost/static_assert.hpp>

/// @file memory/PoolAllocator.hpp
/// Defines the PoolAllocator and ObjectPoolAllocator classes.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// A pool allocator.
///
/// This class implements a pool allocator that splits it's elements across
/// several pools
///
/// The element size and the number of elements per pool can be set using the
/// @c ElementSize and @c PoolSize template arguments.
///
/// @note @c PoolSize has to be smaller than 255, because it will be stored in a byte
/// to save space.
template <uint32_t ElementSize, uint32_t PoolSize = 254>
class PoolAllocator : public AbstractAllocator
{
	BOOST_STATIC_ASSERT_MSG(PoolSize < 255, "Pool size has to fit in a single byte");

	public:
		/// Construct a new PoolAllocator.
		///
		/// @param super The allocator to obtain the pool memory
		/// from.
		///
		/// @param alignment The required alignment.
		PoolAllocator(const Allocator& super,
		              vstd::size_type alignment);

		/// Destruct a PoolAllocator.
		///
		/// This destructor frees the allocated memory.
		~PoolAllocator();

		/// Get the number of pools.
		///
		/// This function returns the number of pools allocated.
		///
		/// @return the number of pools currently allocated.
		uint32_t GetPoolCount() const;

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

	private:
		struct Pool
		{
			Pool* next;

			uint8_t topOfFreeStack;
			uint8_t available;
			uint8_t freeStack[PoolSize];
			uint8_t pool[ElementSize * PoolSize];
		};

		Pool* CreatePool();

		Allocator m_super;
		vstd::size_type m_alignment;
		Pool* m_head;
};

template <uint32_t ElementSize, uint32_t PoolSize>
PoolAllocator<ElementSize, PoolSize>::PoolAllocator(const Allocator& super,
                                                    vstd::size_type alignment)
	: m_super(super)
	, m_alignment(alignment)
{
	STORM_ASSERT(alignment != 0, "Alignment cannot be zero");
	m_head = CreatePool();
}

template <uint32_t ElementSize, uint32_t PoolSize>
PoolAllocator<ElementSize, PoolSize>::~PoolAllocator()
{
	STORM_ASSERT(m_head->next == nullptr, "Pool is still in use");

	for (auto current = m_head; current; ) {
		auto next = current->next;
		m_super.Deallocate(current);
		current = next;
	}
}

template <uint32_t ElementSize, uint32_t PoolSize>
uint32_t PoolAllocator<ElementSize, PoolSize>::GetPoolCount() const
{
	uint32_t count = 0;
	for (auto current = m_head; current; current = current->next)
		++count;

	return count;
}

template <uint32_t ElementSize, uint32_t PoolSize>
/*virtual*/ void* PoolAllocator<ElementSize, PoolSize>::AllocateReal(
	vstd::size_type bytes,
	vstd::size_type alignment, vstd::size_type alignmentOffset,
	uint32_t flags
)
{
	(void)flags;

	if (bytes != ElementSize)
		STORM_FAIL_FATALLY("storm::PoolAllocator<> used with incompatible size");

	if (m_alignment % alignment != 0)
		STORM_FAIL_FATALLY("storm::PoolAllocator<> used with incompatible alignment");

	if (alignmentOffset % m_alignment != 0)
		STORM_FAIL_FATALLY("storm::PoolAllocator<> used with incompatible alignment offset");

	for (Pool* current = m_head; true; current = current->next) {
		//
		// Check whether we have items in our free stack
		// (i.e. holes in the pool) and use them instead of
		// touching new elements.
		//

		if (current->topOfFreeStack == 0) {
			//
			// If we don't have any left, we either need to search the next
			// pool or allocate a new pool (depending on whether we have a
			// next pool to search).
			//

			if (current->available) {
				uint8_t* ptr = current->pool + ElementSize * (PoolSize - current->available);
				--current->available;
				return ptr;
			} else if (!current->next) {
				current->next = CreatePool();
			}
		} else {
			const auto index = current->freeStack[--current->topOfFreeStack];
			return current->pool + (index * ElementSize);
		}
	}
}

template <uint32_t ElementSize, uint32_t PoolSize>
void PoolAllocator<ElementSize, PoolSize>::DeallocateReal(void* ptr)
{
	if (!ptr)
		return;

	uint8_t* item = static_cast<uint8_t*>(ptr);

	for (Pool* current = m_head, *prev = current; true; ) {
		if (item < current->pool ||
		    item >= current->pool + (ElementSize * PoolSize)) {
			prev = current;
			current = current->next;

			if (!current) {
				STORM_FAIL_FATALLY("Memory is not allocated from this pool");
				return;
			}
		} else {
			STORM_ASSERT(current->topOfFreeStack < PoolSize,
			             "Free stack cannot be bigger than the pool itself");

			STORM_ASSERT((item - current->pool) % ElementSize == 0,
			             "Item has to be aligned to the element size");

			current->freeStack[current->topOfFreeStack++] =
				(item - current->pool) / ElementSize;

			//
			// Check whether all previously allocated items
			// are returned again.
			//

			if (current != m_head &&
			    current->topOfFreeStack == (PoolSize - current->available)) {
				// Remove current from the linked list
				prev->next = current->next;
				m_super.Deallocate(current);
			}

			return;
		}
	}
}

template <uint32_t ElementSize, uint32_t PoolSize>
typename PoolAllocator<ElementSize, PoolSize>::Pool*
	PoolAllocator<ElementSize, PoolSize>::CreatePool()
{
	auto pool = static_cast<Pool*>(m_super.Allocate(sizeof(Pool),
	                                                m_alignment,
	                                                offsetof(Pool, pool)));

	pool->next = nullptr;
	pool->topOfFreeStack = 0;
	pool->available = PoolSize;
	return pool;
}

}

/// @}

#endif
