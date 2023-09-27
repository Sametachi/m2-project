//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_NEW_HPP
#define STORM_MEMORY_NEW_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocate.hpp>

#include <vstl/utility/forward.hpp>

#include <boost/type_traits/alignment_of.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
	#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
	#include <boost/preprocessor/repetition/repeat.hpp>
	#include <boost/preprocessor/iteration/local.hpp>
#endif

/// @file memory/New.hpp
/// Defines the New() and NewArray() functions.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// Allocate and construct an object.
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
/// AbstractAllocator::Allocate function. For details regarding the
/// parameter values, look at the AbstractAllocator documentation and
/// at your implementation's documentation.

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

template <typename T, typename... Args>
T* New(const Allocator& allocator, uint32_t flags, Args&&... args)
{
	BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type");
	return new (allocator.Allocate(sizeof(T),
	                               boost::alignment_of<T>::value,
	                               0, flags))
		T (vstd::forward<Args>(args)...);
}

#else

#define STORM_FORMAT_ARGUMENT(z, n, unused) \
	BOOST_PP_COMMA_IF(n) vstd::forward<A ## n>(arg ## n)

#define BOOST_PP_LOCAL_MACRO(n) \
	template <typename T \
		BOOST_PP_ENUM_TRAILING_PARAMS(n, typename A)> \
	T* New(const Allocator& allocator, uint32_t flags \
	       BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, A, &&arg)) \
	{ \
		BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type"); \
		return new (allocator.Allocate(sizeof(T), \
		                               boost::alignment_of<T>::value, \
		                               0, flags)) \
			T(BOOST_PP_REPEAT(n, STORM_FORMAT_ARGUMENT, 0)); \
	}

#define BOOST_PP_LOCAL_LIMITS (0, VSTD_MAX_EMULATED_VARIADIC_TEMPLATES)
#include BOOST_PP_LOCAL_ITERATE()

#undef STORM_FORMAT_ARGUMENT
#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS

#endif

/// Allocate and construct an array.
///
/// This function allocates an array of the given type using the
/// type's native alignment (determined via boost::alignment_of).
///
/// @param allocator The allocator to use.
///
/// @param size The number of array elements.
///
/// @param flags Special flags that should be passed to the allocator.
///
/// @return The allocated memory block.
///
/// @note This function is just a wrapper for the complex
/// AbstractAllocator::Allocate function. For details regarding the
/// parameter values, look at the AbstractAllocator documentation and
/// at your implementation's documentation.

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

template <typename T, typename... Args>
T* NewArray(const Allocator& allocator, vstd::size_type size,
            uint32_t flags, Args&&... args)
{
	BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type");
	BOOST_STATIC_ASSERT_MSG(sizeof(T) % boost::alignment_of<T>::value == 0,
	                        "Array element size must be a multiple of it's alignment");

	const auto allocSize = sizeof(vstd::size_type) + sizeof(T) * size;
	auto base = allocator.Allocate(allocSize,
	                               boost::alignment_of<T>::value,
	                               sizeof(vstd::size_type), flags);

	std::memcpy(base, &size, sizeof(vstd::size_type));
	auto ptr = reinterpret_cast<T*>(static_cast<uint8_t*>(base) +
	                                sizeof(vstd::size_type));

	for (T* i = ptr, *end = ptr + size; i != end; ++i)
		new (i) T(vstd::forward<Args>(args)...);

	return ptr;
}

#else

#define STORM_FORMAT_ARGUMENT(z, n, unused) \
	BOOST_PP_COMMA_IF(n) vstd::forward<A ## n>(arg ## n)

#define BOOST_PP_LOCAL_MACRO(n) \
	template <typename T BOOST_PP_ENUM_TRAILING_PARAMS(n, typename A)> \
	T* NewArray(const Allocator& allocator, vstd::size_type size, \
	            uint32_t flags \
	            BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(n, A, &&arg)) \
	{ \
		BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type"); \
		BOOST_STATIC_ASSERT_MSG(sizeof(T) % boost::alignment_of<T>::value == 0, \
		                        "Array element size must be a multiple of it's alignment"); \
		const auto allocSize = sizeof(vstd::size_type) + sizeof(T) * size; \
		auto base = allocator.Allocate(allocSize, \
		                               boost::alignment_of<T>::value, \
		                               sizeof(vstd::size_type), flags); \
		std::memcpy(base, &size, sizeof(vstd::size_type)); \
		auto ptr = reinterpret_cast<T*>(static_cast<uint8_t*>(base) + \
		                                sizeof(vstd::size_type)); \
		for (T* i = ptr, *end = ptr + size; i != end; ++i) \
			new (i) T(BOOST_PP_REPEAT(n, STORM_FORMAT_ARGUMENT, 0)); \
		return ptr; \
	}

#define BOOST_PP_LOCAL_LIMITS (0, VSTD_MAX_EMULATED_VARIADIC_TEMPLATES)
#include BOOST_PP_LOCAL_ITERATE()

#undef STORM_FORMAT_ARGUMENT
#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS

#endif

/// Delete an object.
///
/// This function destructs and deallocates the given object.
///
/// @param allocator The allocator to deallocate the object with.
///
/// @param p The object pointer.
template <typename T>
void Delete(const Allocator& allocator, T* p)
{
	p->~T();
	allocator.Deallocate(p);
}

/// Delete an array.
///
/// This function destructs and deallocates the given array.
///
/// @param allocator The allocator to deallocate the array with.
///
/// @param p The array pointer.
///
/// @param size Number of array elements.
template <typename T>
void DeleteArray(const Allocator& allocator, T* p)
{
	auto base = reinterpret_cast<uint8_t*>(p) - sizeof(vstd::size_type);

	auto size = *reinterpret_cast<vstd::size_type*>(base);
	for (T* e = p; size; --size, ++e)
		e->~T();

	allocator.Deallocate(base);
}

}

/// @}

#endif
