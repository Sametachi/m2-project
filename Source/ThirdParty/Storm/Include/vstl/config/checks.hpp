//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_CONFIG_CHEKCS_HPP
#define VSTL_CONFIG_CHEKCS_HPP

#include <vstl/config/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/current_function.hpp>

namespace vstd
{

// The user is expected to supply these...

bool HandleOutOfBounds(const char* filename, int lineno,
                       const char* functionName, const char* expr);

bool HandleInvalidArgument(const char* filename, int lineno,
                           const char* functionName, const char* expr);

bool HandlePerformanceNotification(const char* filename, int lineno,
                                   const char* functionName, const char* msg);

bool HandleAssertionFailure(const char* filename, int lineno,
                            const char* functionName, const char* expr,
                            const char* msg);

}

#if VSTD_ENABLE_BOUNDS_CHECKING
	// TODO(tim): Output value, min, max?
	#define VSTD_CHECK_BOUNDS(expr) (expr) || vstd::HandleOutOfBounds(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, #expr)
#else
	#define VSTD_CHECK_BOUNDS(expr) (void)0
#endif

#if VSTD_ENABLE_ARGUMENT_CHECKING
	#define VSTD_CHECK_ARGUMENT(expr) (expr) || vstd::HandleInvalidArgument(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, #expr)
#else
	#define VSTD_CHECK_ARGUMENT(expr) (void)0
#endif

#if VSTD_ENABLE_PERFORMANCE_CHECKING
	#define VSTD_NOTIFY_BAD_PERFORMANCE(msg) vstd::HandlePerformanceNotification(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, #msg)
#else
	#define VSTD_NOTIFY_BAD_PERFORMANCE(msg) (void)0
#endif

#if VSTD_DEBUG
	#define VSTD_ASSERT(expr, msg) (expr) || vstd::HandleAssertionFailure(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, #expr, msg)
#else
	#define VSTD_ASSERT(expr, msg) (void)0
#endif

#endif
