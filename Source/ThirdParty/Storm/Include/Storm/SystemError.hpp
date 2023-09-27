//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_SYSTEMERROR_HPP
#define STORM_SYSTEMERROR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file SystemError.hpp
/// Defines the SystemError struct.

namespace storm
{

/// A lightweight struct wrapping an OS error.
///
/// This struct wraps an OS error code. It's only purpose
/// is to allow type-based functions etc. react differently for
/// error codes. (Otherwise they'd be handled just like plain integers.)
struct SystemError
{
	/// Construct a new SystemError.
	///
	/// This constructor sets the SystemError's code
	/// member to the passed value.
	///
	/// @param code The error code.
	SystemError(int code);

	/// The OS error code.
	int code;
};

BOOST_FORCEINLINE SystemError::SystemError(int code)
	: code(code)
{
	// ctor
}

}

#endif
