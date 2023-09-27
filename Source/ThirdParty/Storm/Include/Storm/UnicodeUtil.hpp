//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_UNICODEUTIL_HPP
#define STORM_UNICODEUTIL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/system/error_code.hpp>

/// @file UnicodeUtil.hpp
/// Defines various unicode-related functions.

namespace storm
{

/// Convert UTF16 text to UTF8.
///
/// This function converts the UTF16 input text
/// (the range [source, sourceLast)) to UTF8 (the range [targetFirst, targetLast)).
/// The input is checked for validity. If the function fails, it returns
/// zero and sets @c ec.
///
/// @param sourceFirst A pointer to the start of the source string.
///
/// @param sourceLast A pointer to one character past the end of the source
/// string.
///
/// @param targetFirst A pointer to the start of the target string.
///
/// @param targetLast A pointer to one character past the end of the target
/// string.
///
/// @param ec The @c error_code object which will be set in case of invalid
/// data or an insufficient buffer.
///
/// @return The number of characters written

vstd::size_type ConvertUtf16ToUtf8(const wchar_t* sourceFirst,
                                   const wchar_t* sourceLast,
                                   char* targetFirst,
                                   char* targetLast,
                                   boost::system::error_code& ec);

vstd::size_type ConvertUtf8ToUtf16(const char* sourceFirst,
                                   const char* sourceLast,
                                   wchar_t* targetFirst,
                                   wchar_t* targetLast,
                                   boost::system::error_code& ec);

}

#endif
