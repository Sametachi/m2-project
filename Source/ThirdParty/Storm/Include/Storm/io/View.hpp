//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_VIEW_HPP
#define STORM_IO_VIEW_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/AbstractAllocator.hpp>

#include <boost/noncopyable.hpp>

/// @file io/InputStream.hpp
/// Defines the InputStream class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// A lightweight class modelling the ViewConcept.
///
/// This class is a lightweight implementation of the ViewConcept.
///
/// @par Example
/// The following example shows the usage of a read-only view.
///
/// @code
/// boost::system::error_code ec;
/// View view(m_allocator);
/// if (m_dataSource.CreateView(view, 1024, ec))
///     m_networkSink.Send(view.GetData(), 1024, ec);
/// @endcode
///
/// This code replaces the following where you'd allocate an extra buffer,
/// regardless of whether the data source supports getting a read-only buffer
/// without allocation.
///
/// @code
/// boost::system::error_code ec;
/// boost::scoped_array<uint8_t> buffer(new uint8_t[1024]);
/// if (m_dataSource.Read(buffer.get(), 1024, ec) == 1024 && !ec)
///     m_networkSink.Send(buffer.get(), 1024, ec);
/// @endcode
class View : boost::noncopyable
{
	public:
		/// Construct a new View
		///
		/// This constructor creates, but doesn't initiailize
		/// the View.
		///
		/// @param allocator The allocator that should be used by the View,
		/// if a memory allocation is needed
		View(const Allocator& allocator);

		/// Destruct the View
		///
		/// If the View owns a buffer, this destructor will free it.
		~View();

		/// Initialize the View.
		///
		/// This function initializes the View using @c ptr.
		/// The View assumes that the @c ptr is owner by another
		/// object and thus won't attempt to deallocate it in the
		/// destructor.
		///
		/// @param ptr The pointer to initialize the View with
		///
		/// @sa Initialize(vstd::size_type)
		void Initialize(const uint8_t* ptr);

		/// Initialize the View.
		///
		/// This function initializes the View using @c size.
		/// The View will allocate a @c size large buffer and return a
		/// pointer to it to the caller. The caller then has to fill this
		/// buffer with no more than @c size bytes.
		///
		/// @param size The maximum size of the View
		///
		/// @return A pointer to the newly allocated buffer
		///
		/// @sa Initialize(const uint8_t* ptr)
		uint8_t* Initialize(vstd::size_type size);

		/// Get the allocator used by this View.
		///
		/// This function returns the allocator associated to this View.
		///
		/// @return The allocator used by this View
		const Allocator& GetAllocator();

		/// Get a pointer to the View's data.
		///
		/// This function returns a pointer to the View's data.
		///
		/// @return A pointer to the View's data
		const uint8_t* GetData() const;

	private:
		Allocator m_allocator;
		uint8_t* m_data;
		bool m_isDataOwner;
};

BOOST_FORCEINLINE View::View(const Allocator& allocator)
	: m_allocator(allocator)
	, m_data(nullptr)
	, m_isDataOwner(false)
{
	// ctor
}

BOOST_FORCEINLINE View::~View()
{
	if (m_isDataOwner)
		m_allocator.Deallocate(m_data);
}

BOOST_FORCEINLINE void View::Initialize(const uint8_t* ptr)
{
	m_data = const_cast<uint8_t*>(ptr);
	m_isDataOwner = false;
}

BOOST_FORCEINLINE const Allocator& View::GetAllocator()
{ return m_allocator; }

BOOST_FORCEINLINE const uint8_t* View::GetData() const
{ return m_data; }

}

/// @}

#endif
