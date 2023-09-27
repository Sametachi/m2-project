//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_ALLOCATE_HPP
#define STORM_MEMORY_ALLOCATE_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>

#include <boost/type_traits/alignment_of.hpp>
#include <boost/static_assert.hpp>

/// @file memory/Allocate.hpp
/// Defines the Allocate() and AllocateArray() functions.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// Allocate an object.
///
/// This function allocates an object of the given type using the
/// type's native alignment (determined via boost::alignment_of).
///
/// @param allocator The allocator to use.
///
/// @param flags Special flags that should be passed to the allocator.
///
/// @return The allocated memory block.
///
/// @note This function is just a wrapper for the complex
/// Allocator::Allocate function. For details regarding the
/// parameter values, look at the Allocator documentation and
/// at your implementation's documentation.
template <typename T>
T* Allocate(const Allocator& allocator, uint32_t flags = 0)
{
	return static_cast<T*>(allocator.Allocate(sizeof(T),
	                                          boost::alignment_of<T>::value,
	                                          0, flags));
}

/// Allocate an array.
///
/// This function allocates an array of the given type using the
/// type's native alignment (determined via boost::alignment_of).
///
/// @param allocator The allocator to use.
///
/// @param size Number of array elements.
///
/// @param flags Special flags that should be passed to the allocator.
///
/// @return The allocated memory block.
///
/// @note This function is just a wrapper for the complex
/// Allocator::Allocate function. For details regarding the
/// parameter values, look at the Allocator documentation and
/// at your implementation's documentation.
template <typename T>
T* AllocateArray(const Allocator& allocator, vstd::size_type size,
                 uint32_t flags = 0)
{
	BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type");
	BOOST_STATIC_ASSERT_MSG(sizeof(T) % boost::alignment_of<T>::value == 0,
	                        "Array element size must be a multiple of it's alignment");

	return static_cast<T*>(allocator.Allocate(sizeof(T) * size,
	                                          boost::alignment_of<T>::value,
	                                          0, flags));
}

}

/// @}

#endif
