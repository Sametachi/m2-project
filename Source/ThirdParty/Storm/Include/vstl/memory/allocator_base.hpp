//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_ALLOCATORBASE_HPP
#define VSTL_MEMORY_ALLOCATORBASE_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/static_assert.hpp>
#include <boost/current_function.hpp>

namespace vstd
{

#if VSTD_TRACK_MEMORY
	#define VSTD_ALLOC_NAME(s) (s)
#else
	#define VSTD_ALLOC_NAME(s) ((const char*)0)
#endif

#ifndef VSTD_DEFAULT_ALLOC_NAME
	#define VSTD_DEFAULT_ALLOC_NAME "vstl-allocator"
#endif

/// Allocation flags.
///
/// This enum defines the various optional flags
/// an allocation request can have.
VSTD_SCOPED_ENUM_BEGIN(allocation_flags)
{
	/// (Semi-)permanent object lifetime.
	///
	/// Tells the allocator that this is a (semi-)permanent object,
	/// thus signalling it that this object will live for a long time.
	perm = (1 << 0),

	/// Temporary object lifetime.
	///
	/// Tells the allocator that this is a temporary object,
	/// thus signalling it that this object will be freed very soon.
	temp = (1 << 1),

	/// Allocations can fail.
	///
	/// Tells the allocater that it can return NULL in case of
	/// failure instead of raising a fatal error.
	can_fail = (1 << 2)
};
VSTD_SCOPED_ENUM_END(allocation_flags)

class allocator_base
{
	public:
		BOOST_FORCEINLINE allocator_base(const char* name =
				VSTD_ALLOC_NAME(VSTD_DEFAULT_ALLOC_NAME))
#if VSTD_TRACK_MEMORY
			: m_name(name)
#endif
		{
			(void)name;
		}

		BOOST_FORCEINLINE allocator_base(const allocator_base& other)
#if VSTD_TRACK_MEMORY
			: m_name(other.m_name)
#endif
		{
			(void)other;
		}

#if VSTD_TRACK_MEMORY
		BOOST_FORCEINLINE const char* get_name() const
		{ return m_name; }

		BOOST_FORCEINLINE void set_name(const char* name)
		{ m_name = name; }
#else
		BOOST_FORCEINLINE const char* get_name() const
		{ return nullptr; }

		BOOST_FORCEINLINE void set_name(const char* /*name*/)
		{  }
#endif

	protected:
		enum { kDefaultAlignment = 8 };

#if VSTD_TRACK_MEMORY
		const char* m_name;
#endif

};

template <class Allocator>
struct is_fixed_size_allocator : public boost::mpl::false_
{ };

template <class Allocator>
struct get_allocator_fixed_size
{
	BOOST_STATIC_ASSERT_MSG(is_fixed_size_allocator<Allocator>::value,
							"Allocator has to be a fixed-size allocator");

	typedef size_type type;
	typedef typename Allocator::size value;
};

//
// Returns true if the allocator supports the:
//		size_type expand(void* p, size_type needed)
// method.
//

template <class Allocator>
struct is_allocator_memory_expandable : public boost::mpl::false_
{ };

template <class T>
struct is_allocator_aware : public boost::mpl::false_
{ };

}

#endif
