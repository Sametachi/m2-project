//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_SCOPEDARRAY_HPP
#define STORM_MEMORY_SCOPEDARRAY_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/AbstractAllocator.hpp>
#include <storm/memory/New.hpp>

#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/noncopyable.hpp>

/// @file memory/ScopedArray.hpp
/// Defines the ScopedArray<T> class.
///
/// @addtogroup memory Storm Memory Management
/// The memory management component of the Storm library.
///
/// @{

namespace storm
{

/// A std::unique_ptr like smart pointer.
///
/// This class implements a smart pointer which owns the pointer
/// associated with it. It's semantics are therefore comparable to those
/// of std::unique_ptr (C++11)
template <typename T>
class VSTD_DEPRECATED ScopedArray : boost::noncopyable
{
	public:
		typedef T Type;

		/// Construct a new ScopedArray.
		///
		/// This constructor initializes the ScopedArray using
		/// the given allocator and ptr.
		///
		/// @param allocator The allocator that should be used to
		/// deallocate the object.
		///
		/// @param size The array size.
		///
		/// @param flags Special flags that should be passed to the allocator.
		ScopedArray(const Allocator& allocator,
		            vstd::size_type size = 0,
		            uint32_t flags = 0);

		/// Construct a new ScopedArray.
		///
		/// This constructor move-initializes the ScopedArray using
		/// the given ScopedArray.
		///
		/// @param other The ScopedArray to move from.
		ScopedArray(ScopedArray&& other);

		/// Destruct the ScopedArray.
		///
		/// This destructor deletes the pointer associated with
		/// this ScopedArray.
		~ScopedArray();

		/// Get an array item.
		///
		/// This operator returns the item with the index @c index.
		///
		/// @param index The index of the array item.
		///
		/// @return The requested array item.
		Type& operator[](vstd::size_type index);

		/// Get an array item.
		///
		/// This operator returns the item with the index @c index.
		///
		/// @param index The index of the array item.
		///
		/// @return The requested array item.
		const Type& operator[](vstd::size_type index) const;

		/// Get the owned pointer.
		///
		/// This function returns the managed pointer.
		///
		/// @return The managed pointer.
		Type* Get();

		/// Get the owned pointer.
		///
		/// This function returns the managed pointer.
		///
		/// @return The managed pointer.
		const Type* Get() const;

		/// Set the managed pointer.
		///
		/// This function releases the currently managed pointer
		/// (i.e. deletes it) and assigns the given pointer to the ScopedArray.
		///
		/// @param size The array size.
		///
		/// @param flags Special flags that should be passed to the allocator.
		void Set(vstd::size_type size = 0,
		         uint32_t flags = 0);

		/// Set the managed pointer.
		///
		/// This function releases the currently managed pointer
		/// (i.e. deletes it) and assigns the given pointer to the ScopedArray.
		///
		/// @param allocator The allocator that should be used to
		/// deallocate the object.
		///
		/// @param size The array size.
		///
		/// @param flags Special flags that should be passed to the allocator.
		void Set(const Allocator& allocator,
		         vstd::size_type size = 0,
		         uint32_t flags = 0);

	private:
		Allocator m_allocator;
		Type* m_ptr;
};

template <typename T>
ScopedArray<T>::ScopedArray(const Allocator& allocator,
                            vstd::size_type size,
                            uint32_t flags)
	: m_allocator(allocator)
	, m_ptr(nullptr)
{
	if (size)
		m_ptr = NewArray<T>(m_allocator, size, flags);
}

template <typename T>
ScopedArray<T>::ScopedArray(ScopedArray&& other)
	: m_allocator(other.m_allocator)
	, m_ptr(other.m_ptr)
{
	other.m_ptr = nullptr;
}

template <typename T>
ScopedArray<T>::~ScopedArray()
{
	if (m_ptr)
		DeleteArray(m_allocator, m_ptr);
}

template <typename T>
typename ScopedArray<T>::Type& ScopedArray<T>::
	operator[](vstd::size_type index)
{ return m_ptr[index]; }

template <typename T>
const typename ScopedArray<T>::Type& ScopedArray<T>::
	operator[](vstd::size_type index) const
{ return m_ptr[index]; }

template <typename T>
typename ScopedArray<T>::Type* ScopedArray<T>::Get()
{ return m_ptr; }

template <typename T>
const typename ScopedArray<T>::Type* ScopedArray<T>::Get() const
{ return m_ptr; }

template <typename T>
void ScopedArray<T>::Set(vstd::size_type size, uint32_t flags)
{
	if (m_ptr)
		DeleteArray(m_allocator, m_ptr);

	if (size)
		m_ptr = NewArray<T>(m_allocator, size, flags);
	else
		m_ptr = nullptr;
}

template <typename T>
void ScopedArray<T>::Set(const Allocator& allocator,
                         vstd::size_type size,
                         uint32_t flags)
{
	if (m_ptr)
		DeleteArray(m_allocator, m_ptr);

	if (size)
		m_ptr = NewArray<T>(allocator, size, flags);
	else
		m_ptr = nullptr;

	m_allocator = allocator;
}

}

/// @}

#endif
