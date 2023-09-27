//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_ALLOCATOR_IMPL_HPP
#define STORM_MEMORY_ALLOCATOR_IMPL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>

/// @file memory/Allocator-impl.hpp
/// Implements the Allocator class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

BOOST_FORCEINLINE Allocator::Allocator(AbstractAllocator* allocator,
                                       const char* name)
	: m_allocator(allocator)
#if STORM_TRACK_MEMORY
	, m_name(name)
#endif
{
	(void)name;
}

BOOST_FORCEINLINE Allocator::Allocator(const Allocator& other)
	: m_allocator(other.m_allocator)
#if STORM_TRACK_MEMORY
	, m_name(other.m_name)
#endif
{
	// ctor
}

BOOST_FORCEINLINE Allocator::Allocator(const Allocator& other,
                                       const char* name)
	: m_allocator(other.m_allocator)
#if STORM_TRACK_MEMORY
	, m_name(name)
#endif
{
	(void)name;
}

BOOST_FORCEINLINE Allocator& Allocator::operator=(const Allocator& other)
{
	m_allocator = other.m_allocator;
	return *this;
}

BOOST_FORCEINLINE Allocator& Allocator::operator=(AbstractAllocator* allocator)
{
	m_allocator = allocator;

#if STORM_TRACK_MEMORY
	m_name = name;
#endif

	return *this;
}

#if STORM_TRACK_MEMORY

BOOST_FORCEINLINE const char* Allocator::GetName() const
{ return m_name; }

BOOST_FORCEINLINE void Allocator::SetName(const char* name)
{ m_name = name; }

#endif

BOOST_FORCEINLINE AbstractAllocator* Allocator::GetRealAllocator() const
{ return m_allocator; }

BOOST_FORCEINLINE bool operator==(const Allocator& lhs,
                                  const Allocator& rhs)
{ return lhs.GetRealAllocator() == rhs.GetRealAllocator(); }

BOOST_FORCEINLINE bool operator!=(const Allocator& lhs,
                                  const Allocator& rhs)
{ return lhs.GetRealAllocator() != rhs.GetRealAllocator(); }

}

#endif
