//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_STDALLOCATOR_IMPL_HPP
#define STORM_MEMORY_STDALLOCATOR_IMPL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>
#include <storm/memory/Allocate.hpp>

#include <memory>

namespace storm
{

template <typename T>
BOOST_FORCEINLINE StdAllocator<T>::StdAllocator() BOOST_NOEXCEPT
	: m_allocator(nullptr)
{
	// ctor
}

template <typename T>
BOOST_FORCEINLINE StdAllocator<T>::StdAllocator(const Allocator& alloc)
	: m_allocator(alloc)
{
	// ctor
}

template <typename T>
BOOST_FORCEINLINE StdAllocator<T>::StdAllocator(const StdAllocator& other) BOOST_NOEXCEPT
	: m_allocator(other.m_allocator)
{
	// ctor
}

template <typename T>
template <typename U>
BOOST_FORCEINLINE StdAllocator<T>::StdAllocator(const StdAllocator<U>& other) BOOST_NOEXCEPT
	: m_allocator(other.GetRealAllocator())
{
	// ctor
}

template <typename T>
BOOST_FORCEINLINE const Allocator& StdAllocator<T>::GetRealAllocator() const BOOST_NOEXCEPT
{ return m_allocator; }

template <typename T>
BOOST_FORCEINLINE typename StdAllocator<T>::pointer StdAllocator<T>::allocate(
	size_type count,
	std::allocator<void>::const_pointer hint)
{
	(void)hint;
	return AllocateArray<T>(m_allocator, count);
}

template <typename T>
BOOST_FORCEINLINE void StdAllocator<T>::deallocate(pointer p, size_type n) BOOST_NOEXCEPT
{
	(void)n;
	m_allocator.Deallocate(p);
}

template <typename T, typename U>
BOOST_FORCEINLINE bool operator==(const StdAllocator<T>& lhs,
                                  const StdAllocator<U>& rhs)
{ return lhs.GetRealAllocator() == rhs.GetRealAllocator(); }

template <typename T, typename U>
BOOST_FORCEINLINE bool operator!=(const StdAllocator<T>& lhs,
                                  const StdAllocator<U>& rhs)
{ return lhs.GetRealAllocator() != rhs.GetRealAllocator(); }

}

#endif
