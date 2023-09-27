//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_STREAMUTIL_HPP
#define STORM_IO_STREAMUTIL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/system/error_code.hpp>

/// @file io/StreamUtil.hpp
/// Defines *Stream utility functions
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

namespace detail
{

// This allows us to not include the logging stuff here
void HandleRwFailure(bool wasRead,
                     const void* buffer, uint32_t bytes, uint32_t ret,
                     const bsys::error_code& ec);

}

/// Reads data from a BasicInputStream.
///
/// This function is a simple wrapper around the Read() function
/// provided by the BasicInputStream implementation.
///
/// The one notable difference is, that this function cannot fail.
/// If the underlying stream functions fail, this function will invoke
/// the application's fatal failure handler and never return.
///
/// @param stream The stream object to read from.
///
/// @param buffer The target buffer in which data should be copied to.
///
/// @param bytes The number of bytes to copy.
template <class BasicInputStream>
void Read(BasicInputStream& stream, void* buffer, uint32_t bytes)
{
	bsys::error_code ec;
	const uint32_t r = stream.Read(buffer, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(true, buffer, bytes, r, ec);
}

template <class BasicInputStream>
void ReadExact(BasicInputStream& stream, uint64_t offset, void* buffer, uint32_t bytes, bsys::error_code ec)
{
	const uint32_t r = stream.Read(offset, buffer, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(true, buffer, bytes, r, ec);
}

template <class BasicInputStream>
void ReadExact(BasicInputStream& stream, void* buffer, uint32_t bytes, bsys::error_code ec)
{
	const uint32_t r = stream.Read(buffer, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(true, buffer, bytes, r, ec);
}


/// Writes data to a BasicOutputStream.
///
/// This function is a simple wrapper around the Write() function
/// provided by the BasicOutputStream implementation.
///
/// The one notable difference is, that this function cannot fail.
/// If the underlying stream functions fail, this function will invoke
/// the application's fatal failure handler and never return.
///
/// @param stream The stream object to write to.
///
/// @param buffer The target buffer from which the data should be copied from.
///
/// @param bytes The number of bytes to copy.
template <class BasicOutputStream>
void Write(BasicOutputStream& stream, const void* buffer, uint32_t bytes)
{
	bsys::error_code ec;
	const uint32_t r = stream.Write(buffer, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(false, buffer, bytes, r, ec);
}

template <class BasicOutputStream>
void WriteExact(BasicOutputStream& stream,uint64_t offset ,const void* buffer, uint32_t bytes, bsys::error_code ec)
{
	const uint32_t r = stream.Write(buffer, offset, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(false, buffer, bytes, r, ec);
}

template <class BasicOutputStream>
void WriteExact(BasicOutputStream& stream, const void* buffer, uint32_t bytes, bsys::error_code ec)
{
	const uint32_t r = stream.Write(buffer, bytes, ec);

	if (ec || r != bytes)
		detail::HandleRwFailure(false, buffer, bytes, r, ec);
}

}

/// @}

#endif
