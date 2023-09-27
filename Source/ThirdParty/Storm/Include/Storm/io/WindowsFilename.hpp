//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_WINDOWSFILENAME_HPP
#define STORM_IO_WINDOWSFILENAME_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/Path.hpp>
#include <storm/String.hpp>

#include <boost/system/error_code.hpp>

#include <boost/noncopyable.hpp>

/// @file io/WindowsFilename.hpp
/// Defines the WindowsFilename class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

class WindowsFilename
{
	public:
		/// Construct a WindowsFilename.
		///
		/// This constructor initializes the WindowsFilename using
		/// the given filename.
		///
		/// @param filename The source file path.
		///
		/// @param ec The error_code object that is set
		/// if the function fails.
		WindowsFilename(const StringRef& filename,
		                bsys::error_code& ec);

		/// Get the wide-char. filename's length.
		///
		/// This function returns the wide-char. filename's length.
		///
		/// @return The filename's length.
		vstd::size_type GetLength() const;

		/// Get the wide-char. filename.
		///
		/// This function returns the wide-char. filename this class holds.
		///
		/// @return The wide filename.
		const wchar_t* GetFilename() const;

	private:
		vstd::size_type m_length;
		wchar_t m_wideFilename[kMaxPathLength];
};

BOOST_FORCEINLINE vstd::size_type WindowsFilename::GetLength() const
{ return m_length; }

BOOST_FORCEINLINE const wchar_t* WindowsFilename::GetFilename() const
{ return m_wideFilename; }

}

/// @}

#endif
