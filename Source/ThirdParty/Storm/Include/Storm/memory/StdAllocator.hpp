//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_STDALLOCATOR_HPP
#define STORM_MEMORY_STDALLOCATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>

#include <memory>

namespace storm
{

/// Storm allocator to STD allocator wrapper.
///
/// This class stores a pointer to an allocator implementing
/// the storm::AbstractAllocator interface and acts as a bridge between
/// the allocators the STD library expects and the allocators Storm provides.
// TODO(tim): Apparently 2+ years are not enough to adapt to the new standard.
//            At this point I'm simply unwilling to provide construct()/destroy()
//            overloads etc. std::allocator_traits<T> made them optional.
//            This breaks Clang compatibility,
//            which appears to be the only sane compiler -.-
template <typename T>
class StdAllocator : public std::allocator<T>
{
	public:
		using typename std::allocator<T>::size_type;
		using typename std::allocator<T>::pointer;

		template <typename U>
		struct rebind
		{
			typedef StdAllocator<U> other;
		};

		/// Construct a new StdAllocator.
		///
		/// This constructor initializes a new StdAllocator with the current
		/// default allocator.
		StdAllocator() BOOST_NOEXCEPT;

		/// Construct a new StdAllocator.
		///
		/// This constructor initializes a new StdAllocator.
		///
		/// @param alloc The Storm allocator to use
		StdAllocator(const Allocator& alloc);

		/// @brief Copy-constructs a new StdAllocator
		///
		/// @param other The StdAllocator to copy from.
		StdAllocator(const StdAllocator& other) BOOST_NOEXCEPT;

		/// @brief Copy-constructs a new StdAllocator
		///
		/// @param other The StdAllocator to copy from.
		template <typename U>
		StdAllocator(const StdAllocator<U>& other) BOOST_NOEXCEPT;

		/// Get the underlying Storm allocator.
		///
		/// This function returns the used Storm allocator.
		///
		/// @return The Storm allocator used by this StdAllocator
		/// instance.
		const Allocator& GetRealAllocator() const BOOST_NOEXCEPT;

		pointer allocate(size_type count,
		                 std::allocator<void>::const_pointer hint = 0);

		void deallocate(pointer p, size_type n) BOOST_NOEXCEPT;

	private:
		Allocator m_allocator;
};

}

#include <storm/memory/StdAllocator-impl.hpp>

#endif
