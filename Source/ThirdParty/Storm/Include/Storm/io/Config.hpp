//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_CONFIG_HPP
#define STORM_IO_CONFIG_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/system/error_code.hpp>

/// @file io/Config.hpp
/// Defines various core types used thoroughly the entire I/O component.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// Access modes.
///
/// This enum contains the various modes
/// a file can be accessed with.
namespace AccessMode
{

/// Represents read-only access to the object.
BOOST_STATIC_CONSTEXPR uint32_t kRead = (1 << 0);

/// Represents write-only access to the object.
BOOST_STATIC_CONSTEXPR uint32_t kWrite = (1 << 1);

/// Reperesents read and write access to the object.
/// This is equivalent to kAccessModeRead | kAccessModeWrite.
/// (In fact, it's actually implemented like that.)
BOOST_STATIC_CONSTEXPR uint32_t kReadWrite = (1 << 0) | (1 << 1);

/// Marks the object as executable.
/// This access-mode is very specific and isn't supported on all streams/files
/// on all operating-systems.
///
/// Passing this flag to functions not supporting it is undefined.
BOOST_STATIC_CONSTEXPR uint32_t kExecutable = (1 << 2);

}

/// Share modes.
///
/// This enum contains the various modes
/// a file can be shared with.
namespace ShareMode
{

/// Disallows any other object to open this resource while
/// the current object still holds a reference to it.
BOOST_STATIC_CONSTEXPR uint32_t kNone = (0 << 0);

/// Allows other objects to open this resource for reading,
/// while the current object still holds a reference to it.
/// This is the default value.
BOOST_STATIC_CONSTEXPR uint32_t kRead = (1 << 0);

/// Allows other objects to open this resource for writing,
/// while the current object still holds a reference to it.
/// By default a resource cannot be opened for writing by more
/// than one object at the same time.
BOOST_STATIC_CONSTEXPR uint32_t kWrite = (1 << 1);

/// Allows other objects to delete this resource, while the
/// current object still holds a reference to it.
BOOST_STATIC_CONSTEXPR uint32_t kDelete = (1 << 2);

}

/// Usage hints.
///
/// This enum contains the various usage hints
/// that can be given to an Object / the OS.
namespace UsageHint
{

/// Optimizes for sequential accesses.
/// On platforms where this
/// flag is supported (as of the time of writing, only Win32 is supported),
/// this flag tells the cache manager to fetch blocks in a sequential manner.
///
/// @sa kRandomAccess
/// @sa http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858%28v=vs.85%29.aspx#caching_behavior
BOOST_STATIC_CONSTEXPR uint32_t kSequential = 0;

/// Optimizes for random accesses.
/// On platforms where this
/// flag is supported (as of the time of writing, only Win32 is supported),
/// this tells the cache manager to optimize for random access.
///
/// @sa kSequential
/// @sa http://msdn.microsoft.com/en-us/library/windows/desktop/aa363858%28v=vs.85%29.aspx#caching_behavior
BOOST_STATIC_CONSTEXPR uint32_t kRandomAccess = 1;

}

namespace Whence
{

/// Beginning of the file.
BOOST_STATIC_CONSTEXPR uint32_t kBegin = 0;

/// The current file position.
BOOST_STATIC_CONSTEXPR uint32_t kCurrent = 1;

/// The file's size.
BOOST_STATIC_CONSTEXPR uint32_t kEnd = 2;

}

/// File flags.
///
/// This enum contains the flags Stat() can return for a file.
namespace FileFlags
{

/// File is a directory.
BOOST_STATIC_CONSTEXPR uint32_t kDirectory = (1 << 0);

}

VSTD_SCOPED_ENUM_BEGIN(CreationDisposition)
{
	/// The default value. This is not a real value, rather the real creation-
	/// disposition is determined by the object depending on the other flags
	/// (access mode, hints, platform-specific parameters) passed.
	kDefault,

	/// Creates a new file. If the file is already existent, the function
	/// fails.
	kCreateNew,

	/// Always create a new file. If the file exists already, the function
	/// truncates it to zero and succeeds.
	kCreateAlways,

	/// Opens an existing file. If the file doesn't exist, the function fails.
	kOpenExisting,

	/// Open a file. If the file doesn't exist, it is created.
	kOpenAlways,

	/// Opens and truncates a file to zero length. If the file doesn't exist,
	/// the function fails.
	kTruncateExisting,

	/// @cond DEV
	/// Max value. Only used in checking code.
	kMax
	/// @encdond
};
VSTD_SCOPED_ENUM_END(CreationDisposition)

}

#endif
