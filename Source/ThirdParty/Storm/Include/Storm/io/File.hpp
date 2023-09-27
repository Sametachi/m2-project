//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_FILE_HPP
#define STORM_IO_FILE_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/Config.hpp>
#include <storm/StringFwd.hpp>

#include <boost/system/error_code.hpp>

#include <boost/noncopyable.hpp>

#include <ctime>

/// @file io/File.hpp
/// Defines the File class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// A simple class that represents an OS file.
///
/// The File class is Storm's lowest-level File-I/O class which
/// closely resembles the functionality provided by the
/// OS File-I/O functions.
class File : boost::noncopyable
{
	public:
		/// A typedef to the platform's native file handle type.
#if VSTD_PLATFORM_WINDOWS
		typedef void* HandleType;
#else
		typedef int HandleType;
#endif

		/// Construct an empty file object.
		///
		/// This constructor initializes a new File object.
		File();

		/// Move-Construct a file object.
		///
		/// This constructor move-initializes a new File object.
		///
		/// @param file The File object to move from.
		File(File&& file);

		/// Destruct a File object.
		///
		/// If a file is still opened, it will be closed.
		~File();

		/// Move-assign from a file object.
		///
		/// This function move-assigns a File object.
		///
		/// @param file The File object to move from.
		File& operator=(File&& file);

		/// Open a file.
		///
		/// This function attempts to open @c filename.
		///
		/// @param filename The file's path (UTF8 encoded)
		///
		/// @param ec The error_code object.
		///
		/// @param accessMode The desired access-mode
		///
		/// @param creationDisposition The desired creation-disposition
		///
		/// @param shareMode The desired share mode
		///
		/// @param usageHint A hint to the OS, denoting the file's
		/// possible usage.
		///
		/// @sa AccessMode
		/// @sa CreationDisposition
		/// @sa ShareMode
		/// @sa UsageHint
		void Open(const StringRef& filename,
		          bsys::error_code& ec,
		          uint32_t accessMode = AccessMode::kRead,
		          CreationDisposition creationDisposition = CreationDisposition::kDefault,
		          uint32_t shareMode = ShareMode::kRead,
		          uint32_t usageHint = 0);

		/// Closes the file
		///
		/// Attempts to close the opened file.
		/// If no file is opened, this function does nothing
		void Close();

		/// Checks whether the file is opened.
		///
		/// @return Returns @c true if the file is opened,
		///          @c false if not.
		bool IsOpen();

		/// Returns the native handled, used to talk
		/// to the underlying system.
		HandleType GetNativeHandle();

		/// Returns the file's length.
		/// This function is used the query the file's current position.
		///
		/// @return The current position
		uint64_t GetSize();

		/// Change the current file position.
		///
		/// This function changes the current file position to @c offset
		/// bytes relative to the origin specified with @c whence.
		/// It returns the new position, starting at the file's beginning.
		///
		/// @return The new position, relative to the start of the file.
		uint64_t Seek(uint32_t whence, int64_t offset, bsys::error_code& ec);

		/// Read data from the file.
		///
		/// This function is used to read at most @c bytes from the file.
		/// If @c ec is set, the return value and the content of
		/// @c buffer are unspecified.
		///
		/// @param buffer The buffer in which the data should be read
		///
		/// @param bytes The maximum number of bytes to read
		///
		/// @param ec The error_code object that is set
		/// if the function fails.
		///
		/// @return The actual number of bytes read (can be zero)
		vstd::size_type Read(void* buffer, vstd::size_type bytes,
		                     bsys::error_code& ec);

		/// Read data from the file, starting at a given position.
		///
		/// This function is used to read at most @c bytes from the file,
		/// starting at the given position.
		/// If @c ec is set, the return value and the content of
		/// @c buffer are unspecified.
		///
		/// @param offset The offset (relative to the beginning of the file) of
		/// the first byte to read.
		///
		/// @param buffer The buffer in which the data should be read
		///
		/// @param bytes The maximum number of bytes to read
		///
		/// @param ec The error_code object that is set
		/// if the function fails.
		///
		/// @return The actual number of bytes read (can be zero)
		vstd::size_type Read(uint64_t offset,
		                     void* buffer, vstd::size_type bytes,
		                     bsys::error_code& ec);

		/// Write data to the file.
		///
		/// This function is used to write at most @c bytes to the file.
		/// If @c ec is set, the return value and the content of the file
		/// are unspecified.
		///
		/// @param buffer The buffer containing the bytes to write
		///
		/// @param bytes The maximum number of bytes to write.
		///
		/// @param ec The error_code object that is set
		/// if the function fails.
		///
		/// @return The actual number of bytes written
		vstd::size_type Write(const void* buffer, vstd::size_type bytes,
		                      bsys::error_code& ec);

		/// Write data to the file, starting at a given position.
		///
		/// This function is used to write at most @c bytes to the file,
		/// starting at the given position.
		/// If @c ec is set, the return value and the content of the file
		/// are unspecified.
		///
		/// @param offset The offset (relative to the beginning of the file)
		/// at which the first byte of data should be written.
		///
		/// @param buffer The buffer containing the bytes to write
		///
		/// @param bytes The maximum number of bytes to write.
		///
		/// @param ec The error_code object that is set
		/// if the function fails.
		///
		/// @return The actual number of bytes written
		vstd::size_type Write(uint64_t offset,
		                      const void* buffer, vstd::size_type bytes,
		                      bsys::error_code& ec);

		/// Get the various time's associated with this file.
		///
		/// This function returns the various different time
		/// values that are associated with a file.
		///
		/// @param creationTime A reference to the time_t, which
		/// receives the file's creation time. On platforms where this is
		/// unsupported, the value is set to zero.
		///
		/// @param lastAccessTime A reference to the time_t, which
		/// receives the file's last access time.
		///
		/// @param lastWriteTime A reference to the time_t, which
		/// receives the file's last write time.
		///
		/// @param ec The error_code object, which is set
		/// if the function fails.
		///
		/// @note The file's creation time might not be available on all platforms
		void GetFileTimes(std::time_t& creationTime,
		                  std::time_t& lastAccessTime,
		                  std::time_t& lastWriteTime,
		                  bsys::error_code& ec);

		/// Flush any pending content to disk.
		///
		/// This function is used to flush any pending (cached) file
		/// content to disk.
		///
		/// @param ec The error_code object
		void Flush(bsys::error_code& ec);

		/// Stat the file.
		///
		/// This function returns data associated with this
		/// file.
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
		void Stat(uint64_t* size,
		          std::time_t* creationTime,
		          std::time_t* lastAccessTime,
		          std::time_t* lastWriteTime,
		          uint32_t* flags,
		          bsys::error_code& ec);

	private:
		HandleType m_handle;
};

BOOST_FORCEINLINE File::HandleType File::GetNativeHandle()
{ return m_handle; }

}

/// @}

#endif
