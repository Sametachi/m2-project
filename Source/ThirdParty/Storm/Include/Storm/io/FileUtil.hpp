//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_FILEOPERATIONS_HPP
#define STORM_IO_FILEOPERATIONS_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/Config.hpp>
#include <storm/StringFwd.hpp>

#include <boost/system/error_code.hpp>

#include <ctime>

/// @file io/FileOperations.hpp
/// Defines the FileOperations class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// Stat a file.
///
/// This function returns data associated with the given
/// file.
///
/// @param filename The path of the file.
///
/// @param size A pointer to the variable that receives
/// the file's size or NULL.
///
/// @param creationTime A pointer to the variable that receives
/// the file's creation time or NULL.
///
/// @param lastAccessTime A pointer to the variable that receives
/// the file's last access time or NULL.
///
/// @param lastWriteTime A pointer to the variable that receives
/// the file's last write time or NULL.
///
/// @param flags A pointer to the variable that receives
/// the file's flags or NULL.
///
/// @param ec A reference to the variable that receives the
/// function's error code.
///
/// @note On platforms that don't store a file's creation time,
/// the value is set to zero.
void StatFile(const StringRef& filename,
              uint64_t* size,
              std::time_t* creationTime,
              std::time_t* lastAccessTime,
              std::time_t* lastWriteTime,
              uint32_t* flags,
              bsys::error_code& ec);

/// Copies a file to a new location.
///
/// This function copies the source file to a new
/// location.
///
/// @param srcFilename Path of the source file.
///
/// @param dstFilename New file path.
///
/// @param ec A reference to the variable that receives the
/// function's error code.
///
/// @param overwriteIfExists If @c true,
/// the destination file will be overwritten.
/// Otherwise the function will fail,
/// should the destination path already exist.
void CopyFile(const StringRef& srcFilename,
              const StringRef& dstFilename,
              bsys::error_code& ec,
              bool overwriteIfExists = false);

/// Moves a file to a new location.
///
/// This function renames the source file.
///
/// @param srcFilename Path of the source file.
///
/// @param dstFilename New file path.
///
/// @param ec A reference to the variable that receives the
/// function's error code.
void MoveFile(const StringRef& srcFilename,
              const StringRef& dstFilename,
              bsys::error_code& ec);

/// Read a file into a string.
///
/// This function reads |filename| into |str|.
///
/// @param filename File path
///
/// @param str The output string.
void ReadFileToString(const StringRef& filename,
                      storm::String& str,
                      bsys::error_code& ec);

}

/// @}

#endif
