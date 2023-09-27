//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MEMORY_UNIQUEPTR_HPP
#define STORM_MEMORY_UNIQUEPTR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/AbstractAllocator.hpp>

#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <boost/noncopyable.hpp>

/// @file memory/UniquePtr.hpp
/// Defines the UniquePtr<T> class.
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
class VSTD_DEPRECATED UniquePtr : boost::noncopyable
{
	public:
		typedef T Type;

		/// Construct a new UniquePtr.
		///
		/// This constructor initializes the UniquePtr using
		/// the given allocator and ptr.
		///
		/// @param allocator The allocator that should be used to
		/// deallocate the object.
		///
		/// @param p The pointer whose ownership is transferred to
		/// this UniquePtr.
		UniquePtr(Allocator& allocator, Type* p = nullptr);

		/// Construct a new UniquePtr.
		///
		/// This constructor move-initializes the UniquePtr using
		/// the given UniquePtr.
		///
		/// @param other The UniquePtr to move from.
		UniquePtr(UniquePtr&& other);

		/// Destruct the UniquePtr.
		///
		/// This destructor deletes the pointer associated with
		/// this UniquePtr.
		~UniquePtr();

		/// Get the owned pointer.
		///
		/// This operator returns the managed pointer.
		///
		/// @return The managed pointer.
		Type* operator->();

		/// Get the owned pointer.
		///
		/// This operator returns the managed pointer.
		///
		/// @return The managed pointer.
		const Type* operator->() const;

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
		/// (i.e. deletes it) and assigns the given pointer to the UniquePtr.
		///
		/// @param p The new pointer.
		void Set(Type* p = nullptr);

	private:
		Allocator* m_allocator;
		Type* m_ptr;
};

template <typename T>
UniquePtr<T>::UniquePtr(Allocator& allocator, Type* p)
	: m_allocator(&allocator)
	, m_ptr(p)
{
	// ctor
}

template <typename T>
UniquePtr<T>::UniquePtr(UniquePtr&& other)
	: m_allocator(other.m_allocator)
	, m_ptr(other.m_ptr)
{
	other.m_allocator = nullptr;
	other.m_ptr = nullptr;
}

template <typename T>
UniquePtr<T>::~UniquePtr()
{
	if (m_ptr) {
		if (!boost::has_trivial_destructor<T>::value)
			m_ptr->~Type();

		m_allocator->Deallocate(m_ptr);
	}
}

template <typename T>
typename UniquePtr<T>::Type* UniquePtr<T>::operator->()
{ return m_ptr; }

template <typename T>
const typename UniquePtr<T>::Type* UniquePtr<T>::operator->() const
{ return m_ptr; }

template <typename T>
typename UniquePtr<T>::Type* UniquePtr<T>::Get()
{ return m_ptr; }

template <typename T>
const typename UniquePtr<T>::Type* UniquePtr<T>::Get() const
{ return m_ptr; }

template <typename T>
void UniquePtr<T>::Set(Type* p = nullptr)
{
	if (m_ptr) {
		if (!boost::has_trivial_destructor<T>::value)
			m_ptr->~T();

		m_allocator->Deallocate(m_ptr);
	}

	m_ptr = p;
}

/// A std::unique_ptr like smart pointer.
///
/// This class implements a smart pointer which owns the pointer
/// associated with it. It's semantics are therefore comparable to those
/// of std::unique_ptr (C++11)
template <typename T>
class UniquePtr<T[]> : boost::noncopyable
{
	public:
		typedef typename boost::remove_extent<T>::type Type;

		/// Construct a new UniquePtr.
		///
		/// This constructor initializes the UniquePtr using
		/// the given allocator and ptr.
		///
		/// @param allocator The allocator that should be used to
		/// deallocate the object.
		///
		/// @param p The pointer whose ownership is transferred to
		/// this UniquePtr.
		///
		/// @param size The array size.
		UniquePtr(Allocator& allocator, Type* p = nullptr,
				vstd::size_type size = 0);

		/// Construct a new UniquePtr.
		///
		/// This constructor move-initializes the UniquePtr using
		/// the given UniquePtr.
		///
		/// @param other The UniquePtr to move from.
		UniquePtr(UniquePtr&& other);

		/// Destruct the UniquePtr.
		///
		/// This destructor deletes the pointer associated with
		/// this UniquePtr.
		~UniquePtr();

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
		/// (i.e. deletes it) and assigns the given pointer to the UniquePtr.
		///
		/// @param p The new pointer.
		///
		/// @param size The array size.
		void Set(Type* p = nullptr, vstd::size_type size = 0);

	private:
		Allocator* m_allocator;
		Type* m_ptr;
		vstd::size_type m_size;
};

template <typename T>
UniquePtr<T[]>::UniquePtr(Allocator& allocator, Type* p,
						vstd::size_type size)
	: m_allocator(&allocator)
	, m_ptr(p)
	, m_size(size)
{
	STORM_ASSERT(p == nullptr || size > 0,
				"Allocated array cannot have zero size");
}

template <typename T>
UniquePtr<T[]>::UniquePtr(UniquePtr&& other)
	: m_allocator(other.m_allocator)
	, m_ptr(other.m_ptr)
	, m_size(other.m_size)
{
	other.m_allocator = nullptr;
	other.m_ptr = nullptr;
	other.m_size = 0;
}

template <typename T>
UniquePtr<T[]>::~UniquePtr()
{
	if (m_ptr) {
		if (!boost::has_trivial_destructor<T>::value) {
			for (Type* p = m_ptr, *end = p + m_size; p != end; ++p)
				p->~T();
		}

		m_allocator->Deallocate(m_ptr);
	}
}

template <typename T>
typename UniquePtr<T[]>::Type& UniquePtr<T[]>::
	operator[](vstd::size_type index)
{ return m_ptr[index]; }

template <typename T>
const typename UniquePtr<T[]>::Type& UniquePtr<T[]>::
	operator[](vstd::size_type index) const
{ return m_ptr[index]; }

template <typename T>
typename UniquePtr<T[]>::Type* UniquePtr<T[]>::Get()
{ return m_ptr; }

template <typename T>
const typename UniquePtr<T[]>::Type* UniquePtr<T[]>::Get() const
{ return m_ptr; }

template <typename T>
void UniquePtr<T[]>::Set(Type* p = nullptr, vstd::size_type size = 0)
{
	STORM_ASSERT(p == nullptr || size > 0,
				"Allocated array cannot have zero size");

	if (m_ptr) {
		if (!boost::has_trivial_destructor<T>::value) {
			for (Type* p = m_ptr, *end = p + m_size; p != end; ++p)
				p->~T();
		}

		m_allocator->Deallocate(m_ptr);
	}

	m_ptr = p;
	m_size = size;
}

}

/// @}

#endif
