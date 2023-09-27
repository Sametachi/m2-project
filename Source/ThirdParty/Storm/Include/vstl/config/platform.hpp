//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_CONFIG_PLATFORM_HPP
#define VSTL_CONFIG_PLATFORM_HPP

//
// Supported compilers:
// <Name>                   <Macro>
// Gnu Compiler Collection  VSTD_COMPILER_GNUC
// Microsoft Visual C++     VSTD_COMPILER_MSVC
// Clang++                  VSTD_COMPILER_CLANG
//

#if defined(__GNUC__)
	#ifdef __GNUC_PATCHLEVEL__
		#define VSTD_COMPILER_VERSION (__GNUC__ * 0x10000 + __GNUC_MINOR__ * 0x100 + __GNUC_PATCHLEVEL__)
	#else
		#define VSTD_COMPILER_VERSION (__GNUC__ * 0x10000 + __GNUC_MINOR__ * 0x100)
	#endif

	#define VSTD_COMPILER_GNUC VSTD_COMPILER_VERSION
	#define VSTD_COMPILER_NAME "GCC"
#elif defined(__clang__)
	#define VSTD_COMPILER_VERSION (__clang_major__ * 0x1000 + __clang_minor__ * 0x100 + __clang_patchlevel__)
	#define VSTD_COMPILER_CLANG VSTD_COMPILER_VERSION
	#define VSTD_COMPILER_NAME "Clang++"
#elif defined(_MSC_VER)
	#define VSTD_COMPILER_VERSION _MSC_VER
	#define VSTD_COMPILER_MSVC _MSC_VER
	#define VSTD_COMPILER_NAME "Microsoft Visual C++"
#else
	#error "Unknown compiler"
#endif

//
// Macros:
// VSTD_NOINLINE
// VSTD_LIKELY
// VSTD_UNLIKELY
// VSTD_UNUSED
// VSTD_DEPRECATED
// VSTD_ALIGN
// VSTD_PREFETCH
// VSTD_FASTCALL
// VSTD_PURE
// VSTD_CONST
// VSTD_RESTRICT
// VSTD_NORETURN
// VSTD_HAS_PRAGMA_ONCE
//

/*
	Use the already existent BOOST_FORCEINLINE macro
	See http://www.boost.org/libs/config/doc/html/boost_config/boost_macro_reference.html
#ifndef VSTD_FORCEINLINE
	#if (VSTD_COMPILER_GNUC >= 0x030200ul)
		#ifdef __cplusplus
			#define VSTD_FORCEINLINE inline __attribute__((__always_inline__))
		#else
			#define VSTD_FORCEINLINE __inline__ __attribute__((__always_inline__))
		#endif
	#elif (VSTD_COMPILER_MSVC >= 1200)
		#define VSTD_FORCEINLINE __forceinline
	#else
		#define VSTD_FORCEINLINE inline
	#endif
#endif*/

#ifndef VSTD_NOINLINE
	#if ((VSTD_COMPILER_GNUC >= 0x040000ul) && (VSTD_COMPILER_GNUC < 0x040003ul))
		#define VSTD_NOINLINE __attribute__((__noinline__,__used__))
	#elif (VSTD_COMPILER_GNUC >= 0x030200ul)
		#define VSTD_NOINLINE __attribute__((__noinline__))
	#elif (VSTD_COMPILER_MSVC >= 1300)
		#define VSTD_NOINLINE __declspec(noinline)
	#else
		#define VSTD_NOINLINE
	#endif
#endif

#if !defined(VSTD_LIKELY) && !defined(VSTD_UNLIKELY)
	#if (VSTD_COMPILER_GNUC >= 0x030200ul) || (VSTD_CC_INTELC >= 800)
		#define VSTD_LIKELY(e) (__builtin_expect(!!(e), 1))
		#define VSTD_UNLIKELY(e) (__builtin_expect(!!(e), 0))
	#else
		#define VSTD_LIKELY(e) (e)
		#define VSTD_UNLIKELY(e) (e)
	#endif
#endif

#ifndef VSTD_UNUSED
	#if VSTD_COMPILER_GNUC
		#define VSTD_UNUSED __attribute__((unused))
	#else
		#define VSTD_UNUSED
	#endif
#endif

#ifndef VSTD_DEPRECATED
	#if VSTD_COMPILER_GNUC
		#define VSTD_DEPRECATED __attribute__((deprecated))
	#elif (VSTD_COMPILER_MSVC >= 1300)
		#define VSTD_DEPRECATED __declspec(deprecated)
	#else
		#define VSTD_DEPRECATED
	#endif
#endif

#ifndef VSTD_PREFETCH
	#if VSTD_COMPILER_GNUC || VSTD_COMPILER_CLANG
		// See: http://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html
		#define VSTD_PREFETCH_READ 0
		#define VSTD_PREFETCH_WRITE 1

		#define VSTD_PREFETCH_NO_TEMPORAL_LOCALITY 0
		#define VSTD_PREFETCH_LOW_TEMPORAL_LOCALITY 1
		#define VSTD_PREFETCH_MODERATE_TEMPORAL_LOCALITY 2
		#define VSTD_PREFETCH_HIGH_TEMPORAL_LOCALITY 3

		#define VSTD_PREFETCH __builtin_prefetch
	#else
		#define VSTD_PREFETCH(...) (void)0
	#endif
#endif

#ifndef VSTD_FASTCALL
	#if VSTD_COMPILER_GNUC || VSTD_COMPILER_CLANG
		#define VSTD_FASTCALL __attribute__((fastcall))
	#elif VSTD_COMPILER_MSVC
		#define VSTD_FASTCALL __fastcall
	#else
		#define VSTD_FASTCALL
	#endif
#endif

#ifndef VSTD_PURE
	#if VSTD_COMPILER_GNUC || VSTD_COMPILER_CLANG
		#define VSTD_PURE __attribute__((pure))
	#elif VSTD_COMPILER_MSVC
		#define VSTD_PURE
	#endif
#endif

#ifndef VSTD_CONST
	#if VSTD_COMPILER_GNUC || VSTD_COMPILER_CLANG
		#define VSTD_CONST __attribute__((const))
	#elif VSTD_COMPILER_MSVC
		#define VSTD_CONST
	#endif
#endif

#ifndef VSTD_RESTRICT
	#if (VSTD_COMPILER_MSVC >= 1400) || VSTD_COMPILER_GNUC || VSTD_COMPILER_CLANG
		#define VSTD_RESTRICT __restrict
	#else
		#define VSTD_RESTRICT
	#endif
#endif

#ifndef VSTD_NORETURN
	#if VSTD_COMPILER_MSVC
		#define VSTD_NORETURN __declspec(noreturn)
	#elif defined(VSTD_COMPILER_GNUC)
		#define VSTD_NORETURN __attribute__((noreturn))
	#else
		#define VSTD_NORETURN
	#endif
#endif

#ifndef VSTD_HAS_PRAGMA_ONCE
	#if (VSTD_COMPILER_MSVC >= 1020) || VSTD_COMPILER_METROWERKS || VSTD_COMPILER_GNUC || VSTD_COMPILER_SN || VSTD_COMPILER_INTEL || VSTD_COMPILER_CLANG
		#define VSTD_HAS_PRAGMA_ONCE 1
	#else
		#define VSTD_HAS_PRAGMA_ONCE 0
	#endif
#endif

//
// Platform detection
//

#if VSTD_PLATFORM_LINUX || (defined(__linux) || defined(__linux__))
	#undef VSTD_PLATFORM_LINUX
	#define VSTD_PLATFORM_LINUX 1
	#define VSTD_PLATFORM_UNIX 1
#elif VSTD_PLATFORM_WINDOWS || defined(_WIN32)
	#undef VSTD_PLATFORM_WINDOWS
	#define VSTD_PLATFORM_WINDOWS 1
	#define VSTD_PLATFORM_NAME "Windows"
#elif VSTD_PLATFORM_FREEBSD || defined(__FreeBSD__)
	#undef VSTD_PLATFORM_FREEBSD
	#define VSTD_PLATFORM_FREEBSD 1
	#define VSTD_PLATFORM_UNIX 1
	#define VSTD_PLATFORM_NAME "FreeBSD"
#else
	#error Failed to detect platform
#endif

//
// Supported architectures:
// <Name>           <Macro constant>
// x86              VSTD_ARCH_X86
// x86-64           VSTD_ARCH_X86_64
// IA-64            VSTD_ARCH_IA64
// PowerPC          VSTD_ARCH_POWERPC and VSTD_ARCH_POWERPC_32
// PowerPC (64-Bit) VSTD_ARCH_POWERPC and VSTD_ARCH_POWERPC_64
// ARM              VSTD_ARCH_ARM
//

#if defined(__i386__) || defined(__intel__) || defined(_M_IX86)
	#define VSTD_ARCH_X86 1
	#define VSTD_ARCH_32BIT 1
	#define VSTD_ARCH_LITTLE_ENDIAN 1
	#define VSTD_ARCH_NAME "x86"
#elif defined(__x86_64__) || defined(_M_AMD64)
	#define VSTD_ARCH_X86_64 1
	#define VSTD_ARCH_64BIT 1
	#define VSTD_ARCH_LITTLE_ENDIAN 1
	#define VSTD_ARCH_NAME "x86-64 (amd64)"
#elif defined(_M_IA64)
	#define VSTD_ARCH_IA64 1
	#define VSTD_ARCH_64BIT 1
	#define VSTD_ARCH_LITTLE_ENDIAN 1
	#define VSTD_ARCH_NAME "IA-64"
#elif defined(__powerpc64__)
	#define VSTD_ARCH_POWERPC 1
	#define VSTD_ARCH_64BIT 1
	#define VSTD_ARCH_BIG_ENDIAN 1
	#define VSTD_ARCH_NAME "PowerPC 64"
#elif defined(__powerpc__)
	#define VSTD_ARCH_POWERPC 1
	#define VSTD_ARCH_32BIT 1
	#define VSTD_ARCH_BIG_ENDIAN 1
	#define VSTD_ARCH_NAME "PowerPC"
#elif defined(__arm__) || defined(_M_ARM)
	#define VSTD_ARCH_ARM 1
	#define VSTD_ARCH_32BIT 1
	#define VSTD_ARCH_LITTLE_ENDIAN 1
	#define VSTD_ARCH_NAME "ARM"
#else
	#error Unknown processor
	#error Unknown endianness
#endif

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

namespace vstd
{

using boost::int8_t;
using boost::int_least8_t;
using boost::int_fast8_t;
using boost::uint8_t;
using boost::uint_least8_t;
using boost::uint_fast8_t;

using boost::int16_t;
using boost::int_least16_t;
using boost::int_fast16_t;
using boost::uint16_t;
using boost::uint_least16_t;
using boost::uint_fast16_t;

using boost::int32_t;
using boost::int_least32_t;
using boost::int_fast32_t;
using boost::uint32_t;
using boost::uint_least32_t;
using boost::uint_fast32_t;

#ifndef BOOST_NO_INT64_T

using boost::int64_t;
using boost::int_least64_t;
using boost::int_fast64_t;
using boost::uint64_t;
using boost::uint_least64_t;
using boost::uint_fast64_t;

#endif

using boost::intmax_t;
using boost::uintmax_t;

}

#endif
