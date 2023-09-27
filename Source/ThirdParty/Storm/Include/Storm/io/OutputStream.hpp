//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_OUTPUTSTREAM_HPP
#define STORM_IO_OUTPUTSTREAM_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/Endian.hpp>

#include <boost/system/error_code.hpp>

/// @file io/OutputStream.hpp
/// Defines the OutputStream class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

// TODO(tim): Re-add endian conversion support (when needed)

template <class OutputStream, typename ScalarType>
void WriteSimpleScalar(OutputStream& stream,
                       ScalarType val,
                       bsys::error_code& ec)
{
	Write(stream, &val, sizeof(val), ec);
}

template <class OutputStream, typename ScalarType>
void WriteMultiByteScalar(OutputStream& stream,
                          ScalarType val,
                          bsys::error_code& ec)
{
	// ConvertEndianness(val, UsedEndian);
	Write(stream, &val, sizeof(val), ec);
}

/// Write an unsigned 8-bit integer to the stream.
///
/// This function is used to write an unsigned 8-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteU8(OutputStream& stream,
                               uint8_t val,
                               bsys::error_code& ec)
{ WriteSimpleScalar(stream, val, ec); }

/// Writes an signed 8-bit integer to the stream.
///
/// This function is used to write an signed 8-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteI8(OutputStream& stream,
                               int8_t val,
                               bsys::error_code& ec)
{ WriteSimpleScalar(stream, val, ec); }

/// Write an unsigned 16-bit integer to the stream.
///
/// This function is used to write an unsigned 16-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteU16(OutputStream& stream,
                                uint16_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write an signed 16-bit integer to the stream.
///
/// This function is used to write an signed 16-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteI16(OutputStream& stream,
                                int16_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write an unsigned 32-bit integer to the stream.
///
/// This function is used to write an unsigned 32-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteU32(OutputStream& stream,
                                uint32_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write an signed 32-bit integer to the stream.
///
/// This function is used to write an signed 32-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteI32(OutputStream& stream,
                                int32_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write an unsigned 64-bit integer to the stream.
///
/// This function is used to write an unsigned 64-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteU64(OutputStream& stream,
                                uint64_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write an signed 64-bit integer to the stream.
///
/// This function is used to write an signed 64-bit integer to
/// the stream. If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param val The value to write
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteI64(OutputStream& stream,
                                int64_t val,
                                bsys::error_code& ec)
{ WriteMultiByteScalar(stream, val, ec); }

/// Write a string value to the stream.
///
/// This function is used to write a string to the stream.
/// If the function fails, @c ec will be set. If @c ec
/// is set, the stream's state is undefined.
///
/// @param s The source string
///
/// @param ec The error_code object
template <class OutputStream>
BOOST_FORCEINLINE void WriteString(OutputStream& stream,
                                   const StringRef& s,
                                   bsys::error_code& ec)
{
	const vstd::size_type len = s.length();
	WriteU16(stream, len, ec);
	if (ec) return;
	Write(stream, s.data(), len, ec);
}

}

/// @}

#endif
