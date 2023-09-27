//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_DELETER_HPP
#define STORM_MEMORY_DELETER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/New.hpp>

/// @file memory/Deleter.hpp
/// Defines the Deleter<T> class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{


template <class T>
class Deleter
{
	public:
		BOOST_CONSTEXPR Deleter(const Allocator& a) BOOST_NOEXCEPT
			: m_alloc(a)
		{}

		template <class U>
		Deleter(const Deleter<U>& o) BOOST_NOEXCEPT
			: m_alloc(o.m_alloc)
		{}

		void operator()(T* ptr) const BOOST_NOEXCEPT
		{
			static_assert(sizeof(T) > 0, "Deleter can not delete incomplete type");
			Delete(m_alloc, ptr);
		}

	private:
		Allocator m_alloc;
};

template <class T>
struct Deleter<T[]>
{
	public:
		BOOST_CONSTEXPR Deleter(const Allocator& a) BOOST_NOEXCEPT
			: m_alloc(a)
		{}

		template <class U>
		Deleter(const Deleter<U[]>& o) BOOST_NOEXCEPT
			: m_alloc(o.m_alloc)
		{}

		void operator()(T* ptr) const BOOST_NOEXCEPT
		{
			static_assert(sizeof(T) > 0, "Deleter can not delete incomplete type");
			DeleteArray(m_alloc, ptr);
		}

	private:
		Allocator m_alloc;
};

}

/// @}

#endif
