//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_ERRORREPORTING_HPP
#define STORM_ERRORREPORTING_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#if STORM_DEBUG || STORM_ENABLE_ASSERT
	#include <boost/current_function.hpp>
#endif

/// @file ErrorReporting.hpp
/// Defines the Storm assertion macro(s).

namespace storm
{

/// Handle an assertion failure.
///
/// This function handles an assertion failure.
/// It is guranteed that this function never returns control
/// to the caller.
///
/// @param filename Name of the file which contains the expression in question.
///
/// @param lineno Line number in the file.
///
/// @param functionName Name of the function containing the expression in
/// question.
///
/// @param expr The expression in question.
///
/// @param msg A simple error message describing the assertion.
///
/// @return Actually this function never returns. The return
/// value is only here to allow the use of @code (expr) || storm::Handle...
/// @endcode
bool HandleAssertionFailure(const char* filename, int lineno,
                            const char* functionName, const char* expr,
                            const char* msg);

/// Handle a fatal failure.
///
/// This function handles a fatal failure.
/// It is guranteed that this function never returns control
/// to the caller.
///
/// @param filename Name of the file which contains the expression in question.
///
/// @param lineno Line number in the file.
///
/// @param functionName Name of the function containing the expression in
/// question.
///
/// @param msg A simple error message describing the fatal failure.
///
/// @return Actually this function never returns. The return
/// value is only here to allow the use of @code (expr) || storm::Handle...
/// @endcode
/// 
#pragma warning( disable: 4010)
#if STORM_DEBUG || STORM_ENABLE_ASSERT
	#define STORM_ASSERT(expr, msg) //\
		(expr) || storm::HandleAssertionFailure(__FILE__, __LINE__, \
		                                        BOOST_CURRENT_FUNCTION, #expr, msg)
#else
	#define STORM_ASSERT(expr, msg) (void)0
#endif

#if STORM_DEBUG || STORM_ENABLE_ASSERT
	#define STORM_FAIL_FATALLY(msg) //\
		//storm::HandleFatalFailure(__FILE__, __LINE__, \
		                          BOOST_CURRENT_FUNCTION, msg)
#else
	#define STORM_FAIL_FATALLY(msg) //\
		storm::HandleFatalFailure(nullptr, 0, nullptr, msg)
#endif

}

#endif
