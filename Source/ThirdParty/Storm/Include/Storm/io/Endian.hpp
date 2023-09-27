//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_ENDIAN_HPP
#define STORM_IO_ENDIAN_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#if VSTD_COMPILER_MSVC
	#include <intrin.h>
#endif

/// @file io/Endian.hpp
/// Defines various functions and constants related to endianness
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

#if !VSTD_ARCH_LITTLE_ENDIAN && !VSTD_ARCH_BIG_ENDIAN
	#error Invalid platform configuration
#endif

/// Defines the two endians (big and little) plus
/// two values always representing the native and non-native endian.
namespace Endian
{

/// Least-significant byte first
///
/// @sa http://en.wikipedia.org/wiki/Endianness
BOOST_STATIC_CONSTEXPR int kLittle = 0;

/// Most-significant byte first
///
/// @sa http://en.wikipedia.org/wiki/Endianness
BOOST_STATIC_CONSTEXPR int kBig = 1;

/// Always represents the target platform's native endian.
BOOST_STATIC_CONSTEXPR int kNative
#if VSTD_ARCH_LITTLE_ENDIAN
	= kLittle
#elif VSTD_ARCH_BIG_ENDIAN
	= kBig
#endif
;

/// Always represents the target platform's non-native endian.
BOOST_STATIC_CONSTEXPR int kNonNative
#if VSTD_ARCH_LITTLE_ENDIAN
	= kBig
#elif VSTD_ARCH_BIG_ENDIAN
	= kLittle
#endif
;

}

/// Reverses the  byte-order of the given integer
BOOST_FORCEINLINE void SwapValue(uint16_t& a)
{
#if VSTD_COMPILER_MSVC
	a = _byteswap_ushort(a);
#else
	a = ((a & 0x00ffu) << 8)|
	    ((a & 0xff00u) >> 8);
#endif
}

/// @copydoc SwapU16
BOOST_FORCEINLINE void SwapValue(uint32_t& a)
{
#if VSTD_COMPILER_MSVC
	a = _byteswap_ulong(a);
#elif (VSTD_COMPILER_GNUC >= 0x040300ul)
	a = __builtin_bswap32(a);
#else
	a = ((a & 0x000000fful) << 24) |
	    ((a & 0x0000ff00ul) << 8) |
	    ((a & 0x00ff0000ul) >> 8) |
	    ((a & 0xff000000ul) >> 24);
#endif
}

/// @copydoc SwapU16
BOOST_FORCEINLINE void SwapValue(uint64_t& a)
{
#if VSTD_COMPILER_MSVC
	a = _byteswap_uint64(a);
#elif (VSTD_COMPILER_GNUC >= 0x040300ul)
	a = __builtin_bswap64(a);
#else
	a = ((a & 0x00000000000000ffull) << 56) |
	    ((a & 0x000000000000ff00ull) << 40) |
	    ((a & 0x0000000000ff0000ull) << 24) |
	    ((a & 0x00000000ff000000ull) << 8) |
	    ((a & 0x000000ff00000000ull) >> 8) |
	    ((a & 0x0000ff0000000000ull) >> 24) |
	    ((a & 0x00ff000000000000ull) >> 40) |
	    ((a & 0xff00000000000000ull) >> 56);
#endif
}

#define STORM_ENDIANNESSS_CONVERTER(srctype, dsttype) \
	BOOST_FORCEINLINE void ConvertEndianness(srctype& value, int endian) \
	{ ConvertEndianness(*((dsttype*)(&value)), endian); }


BOOST_FORCEINLINE void ConvertEndianness(uint16_t& value, int endian)
{
	if (endian == Endian::kNonNative)
		SwapValue(value);
}

STORM_ENDIANNESSS_CONVERTER(int16_t, uint16_t)

BOOST_FORCEINLINE void ConvertEndianness(uint32_t& value, int endian)
{
	if (endian == Endian::kNonNative)
		SwapValue(value);
}

STORM_ENDIANNESSS_CONVERTER(int32_t, uint32_t)

BOOST_FORCEINLINE void ConvertEndianness(uint64_t& value, int endian)
{
	if (endian == Endian::kNonNative)
		SwapValue(value);
}

STORM_ENDIANNESSS_CONVERTER(int64_t, uint64_t)

#undef STORM_ENDIANNESSS_CONVERTER

}

#endif
