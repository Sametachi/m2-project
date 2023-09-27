//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_UNIX_EINTRWRAPPER_HPP
#define STORM_UNIX_EINTRWRAPPER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !VSTD_PLATFORM_WINDOWS

#include <cerrno>

/// This macro provides an easy way to handle EINTR errors
/// (i.e. the function being interrupted by a signal)
/// by retrying the call.
///
/// Do *not* use this to wrap calls to close().
/// By the time close() returns, the fd is already gone
/// and could be re-used by another thread. Retrying the call
/// would then close someone elses' fd, which is certainly not wanted.
/// see: http://alobbs.com/post/54503240599/close-and-eintr
#define STORM_HANDLE_EINTR(x) ({ \
	auto storm_eintr_result = (x); \
	while (storm_eintr_result == -1 && errno == EINTR) \
		storm_eintr_result = (x); \
	storm_eintr_result; \
})

/// This macro provides an easy way to ignore EINTR errors.
/// If one occurs, the expression's result value will be set to 0.
///
/// This macro should be used for close() calls.
#define STORM_IGNORE_EINTR(x) ({ \
	auto storm_eintr_result = (x); \
	if (storm_eintr_result == -1 && errno == EINTR) \
		storm_eintr_result = 0; \
	storm_eintr_result; \
})

#else

#define STORM_HANDLE_EINTR(x) (x)
#define STORM_IGNORE_EINTR(x) (x)

#endif

#endif
