//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_INPUTSTREAM_HPP
#define STORM_IO_INPUTSTREAM_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/Endian.hpp>

#include <boost/system/error_code.hpp>

/// @file io/InputStream.hpp
/// Defines the InputStream class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

// TODO(tim): Re-add endian conversion support (when needed)

template <class InputStream, typename ScalarType>
ScalarType ReadSimpleScalar(InputStream& stream, bsys::error_code& ec)
{
	ScalarType val;
	Read(stream, &val, sizeof(val), ec);

	if (!ec)
		return val;

	return 0;
}

template <class InputStream, typename ScalarType>
ScalarType ReadMultiByteScalar(InputStream& stream, bsys::error_code& ec)
{
	ScalarType val;
	Read(stream, &val, sizeof(val), ec);

	if (!ec) {
		//ConvertEndianness(val, UsedEndian);
		return val;
	}

	return 0;
}

/// Reads an unsigned 8-bit integer from the stream.
/// This function reads an unsigned 8-bit integer from the stream.
///
/// @return The read unsigned 8-bit integer.
template <class InputStream>
BOOST_FORCEINLINE uint8_t ReadU8(InputStream& stream, bsys::error_code& ec)
{ return ReadSimpleScalar<uint8_t>(stream, ec); }

/// Reads an signed 8-bit integer from the stream.
/// This function reads an signed 8-bit integer from the stream.
///
/// @return The read signed 8-bit integer.
template <class InputStream>
BOOST_FORCEINLINE int8_t ReadI8(InputStream& stream, bsys::error_code& ec)
{ return ReadSimpleScalar<int8_t>(stream, ec); }

/// Reads an unsigned 16-bit integer from the stream.
/// This function reads an unsigned 16-bit integer from the stream.
///
/// @return The read unsigned 16-bit integer.
template <class InputStream>
BOOST_FORCEINLINE uint16_t ReadU16(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<uint16_t>(stream, ec); }

/// Reads an signed 16-bit integer from the stream.
/// This function reads an signed 16-bit integer from the stream.
///
/// @return The read signed 16-bit integer.
template <class InputStream>
BOOST_FORCEINLINE int16_t ReadI16(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<int16_t>(stream, ec); }

/// Reads an unsigned 32-bit integer from the stream.
/// This function reads an unsigned 32-bit integer from the stream.
///
/// @return The read unsigned 32-bit integer.
template <class InputStream>
BOOST_FORCEINLINE uint32_t ReadU32(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<uint32_t>(stream, ec); }

/// Reads an signed 32-bit integer from the stream.
/// This function reads an signed 32-bit integer from the stream.
///
/// @return The read signed 32-bit integer.
template <class InputStream>
BOOST_FORCEINLINE int32_t ReadI32(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<int32_t>(stream, ec); }

/// Reads an unsigned 64-bit integer from the stream.
/// This function reads an unsigned 64-bit integer from the stream.
///
/// @return The read unsigned 64-bit integer.
template <class InputStream>
BOOST_FORCEINLINE uint64_t ReadU64(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<uint64_t>(stream, ec); }

/// Reads an signed 64-bit integer from the stream.
/// This function reads an signed 64-bit integer from the stream.
///
/// @return The read signed 64-bit integer.
template <class InputStream>
BOOST_FORCEINLINE int64_t ReadI64(InputStream& stream, bsys::error_code& ec)
{ return ReadMultiByteScalar<int64_t>(stream, ec); }

/// Reads a string value from the stream.
/// This function reads a string value into @c s.
///
/// @param s The target string
template <class InputStream, class StringType>
BOOST_FORCEINLINE void ReadString(InputStream& stream,
                                  StringType& s,
                                  bsys::error_code& ec)
{
	const uint16_t len = ReadMultiByteScalar<uint16_t>(stream, ec);
	if (len && !ec) {
		s.resize(len);
		Read(stream, s.data(), len, ec);
	}
}

}

/// @}

#endif
