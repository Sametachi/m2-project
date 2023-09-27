//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_CONFIG_CONFIG_HPP
#define VSTL_CONFIG_CONFIG_HPP

#include <vstl/config/platform.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

//
// todo: write something cool & descriptive here...
//

#ifdef VSTD_USERCONFIG_FILE
	#include VSTD_USERCONFIG_FILE
#endif

//
// Defines whether library debugging should be enabled.
//

#ifndef VSTD_DEBUG
	#if defined(DEBUG) || defined(_DEBUG)
		#define VSTD_DEBUG 1
	#endif
#endif

//
// Unsigned and signed size integer types.
// This is vstd's equivalent to size_t and ssize_t.
// We default to uint32_t here as 2^32-1 is usually
// big enough.
//

#ifndef VSTD_SIZE_TYPE
	#define VSTD_SIZE_TYPE uint32_t
	#define VSTD_SSIZE_TYPE int32_t
#endif

namespace vstd
{

typedef VSTD_SIZE_TYPE size_type;
typedef VSTD_SSIZE_TYPE ssize_type;

}

//
// Defines whether vstl should use the standard
// iterator category tags.
// This allows vstl iterators to work with STL algorithms/containers.
//

#ifndef VSTD_USE_STD_ITERATOR_CAT
	#define VSTD_USE_STD_ITERATOR_CAT 1
#endif

//
// Defines whether vstl should use the standard
// pair class (std::pair).
// This allows greater interoperability with other libraries that
// expect std::pair<> usage.
//

#ifndef VSTD_USE_STD_PAIR
	#define VSTD_USE_STD_PAIR 1
#endif

//
// Safety/Speed tradeoffs.
// Enabling the features below will turn on additional checks.
//

//
// Defines whether vstl should check for out-of-bounds conditions.
//
// This includes checks that would lead to a std::out_of_range exception
// in a standard C++ implementation.
//

#ifndef VSTD_ENABLE_BOUNDS_CHECKING
	#define VSTD_ENABLE_BOUNDS_CHECKING 0
#endif

//
// Defines whether vstl should check arguments for invalid values.
//
// This includes checks that would lead to a std::invalid_argument exception
// in a standard C++ implementation.
//

#ifndef VSTD_ENABLE_ARGUMENT_CHECKING
	#define VSTD_ENABLE_ARGUMENT_CHECKING 0
#endif

//
// Defines whether vstl should check user input for valid values,
// which force the implementation to take a slower path.
//

#ifndef VSTD_ENABLE_PERFORMANCE_CHECKING
	#define VSTD_ENABLE_PERFORMANCE_CHECKING 0
#endif

//
// Defines whether single-argument basic_string constructors should be
// explicit. Doing so helps spotting implicit conversions that might
// have a negative impact on your code's performance.
//

#ifndef VSTD_STRING_USE_EXPLICIT_CTORS
	#define VSTD_STRING_USE_EXPLICIT_CTORS 1
#endif

//
// Specifies the maximum number of variadic templates to emulate,
// if compiler support is unavailable.
//

#ifndef VSTD_MAX_EMULATED_VARIADIC_TEMPLATES
	#define VSTD_MAX_EMULATED_VARIADIC_TEMPLATES 4
#endif

#endif
