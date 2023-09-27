//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_PATH_HPP
#define STORM_IO_PATH_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/String.hpp>

#include <utility>

/// @ingroup I/O
/// @file io/Path.hpp
/// Defines various types and constants relating to filesystem paths

namespace storm
{

/// @brief A path's maximum length (including the terminating NUL).
/// @details This is platform/implementation specific.
/// On Windows the agreed limit is 260 characters
/// (including the terminating NUL), while on Unix systems it is theoretically
/// unlimited, but capped to 1024 characters
/// (including the terminating NUL) here.
const int kMaxPathLength =
#if VSTD_PLATFORM_WINDOWS
	260;
#else
	1024;
#endif

/// @brief An array of UTF8 characters big enough to hold a path of
/// kMaxPathLength characters.
typedef char FilePathBuffer[kMaxPathLength];

/// Split a path.
///
/// This function splits a path into a pair of @c head, @c tail. The @c tail
/// is the last component of the input path (thus it never contains a slash).
/// The @c head is everything before it.
///
/// @note If @c path ends with a slash, @c tail will be empty.
/// If @c path doesn't contain a slash at all, @c head will be empty.
///
/// @return A @c pair<head, tail>.
std::pair<StringRef, StringRef> SplitPath(const StringRef& path);

/// Split a path into head and extension.
///
/// This function splits @c path into a pair of @c head, @c ext. The @c ext
/// part contains only the file extension (including the dot), while @c head
/// contains everything else.
///
/// @remarks If @c path starts with a dot and doesn't contain a second dot,
/// @c ext will be empty.
///
/// @return A @c pair<head, ext>.
std::pair<StringRef, StringRef> SplitExtension(const StringRef& path);

}

#endif
