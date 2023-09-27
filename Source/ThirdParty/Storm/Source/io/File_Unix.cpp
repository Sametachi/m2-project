//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/File.hpp>
#include <storm/String.hpp>
#include <storm/unix/EintrWrapper.hpp>

#include <boost/system/error_code.hpp>
#include <boost/static_assert.hpp>

#if !VSTD_PLATFORM_WINDOWS
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>

namespace storm
{

namespace
{

BOOST_STATIC_ASSERT_MSG(
	Whence::kBegin == SEEK_SET &&
	Whence::kCurrent == SEEK_CUR &&
	Whence::kEnd == SEEK_END,
	"Storm origins must match POSIX ones"
);

}

File::File()
	: m_handle(-1)
{
	// ctor
}

File::File(File&& file)
	: m_handle(file.m_handle)
{
	file.m_handle = -1;
}

File::~File()
{
	Close();
}

File& File::operator=(File&& file)
{
	Close();
	m_handle = file.m_handle;
	file.m_handle = -1;
	return *this;
}

void File::Open(const StringRef& filename, bsys::error_code& ec,
                uint32_t accessMode, CreationDisposition creationDisposition,
                uint32_t shareMode, uint32_t usageHint)
{
	STORM_ASSERT(!IsOpen(), "Open() called on already opened file.");

	int openFlags = 0;

	switch (creationDisposition) {
		case CreationDisposition::kDefault: {
			if (accessMode & AccessMode::kWrite)
				openFlags = O_CREAT;

			break;
		}

		case CreationDisposition::kCreateNew:
			openFlags = O_CREAT | O_EXCL;
			break;

		case CreationDisposition::kCreateAlways:
			openFlags = O_CREAT | O_TRUNC;
			break;

		case CreationDisposition::kOpenExisting:
			break;

		case CreationDisposition::kOpenAlways:
			openFlags = O_CREAT;
			break;

		case CreationDisposition::kTruncateExisting:
			openFlags = O_TRUNC;
			break;

		default:
			// Signal the error to the caller
			ec = make_error_code(bsys::errc::invalid_argument);
			//STORM_DLOG(Error, "Creation disposition ({0}) is invalid.",
			//           creationDisposition);
			return;
	}

	if ((accessMode & AccessMode::kReadWrite) == AccessMode::kReadWrite)
		openFlags |= O_RDWR;
	else if (accessMode & AccessMode::kRead)
		openFlags |= O_RDONLY;
	else if (accessMode & AccessMode::kWrite)
		openFlags |= O_WRONLY;

	// TODO(tim): Share mode
	// TODO(tim): Usage hint

	String nullFilename(filename);
	m_handle = STORM_HANDLE_EINTR(open(nullFilename.c_str(), openFlags, 0644));
	if (m_handle == -1) {
		ec.assign(errno, bsys::system_category());
		return;
	}

	int advice = 0;
	if (usageHint == UsageHint::kSequential)
		advice = POSIX_FADV_SEQUENTIAL;
	else if (usageHint == UsageHint::kRandomAccess)
		advice = POSIX_FADV_RANDOM;

	if (0 != advice) {
		// While there's no reason this should fail, it is
		// unspecified whether "hint" will be used,
		// so ignoring this error is fine.
		// (Actually, it does fail on FreeBSD, when run under valgrind.)
		(void)posix_fadvise(m_handle, 0, 0, advice);
	}
}

void File::Close()
{
	auto handle = m_handle;
	if (handle != -1) {
		// Retrying close() is impossible,
		// since the fd could've been re-used by now.
		if (0 != STORM_IGNORE_EINTR(close(handle)))
			STORM_FAIL_FATALLY("close() failed. Retry impossible");

		m_handle = -1;
	}
}

bool File::IsOpen()
{
	return m_handle != -1;
}

uint64_t File::GetSize()
{
	STORM_ASSERT(IsOpen(), "Cannot use GetSize() on a closed file!");

	struct stat st;
	if (fstat(m_handle, &st) == 0)
		return st.st_size;

	// There's no reason why this should fail.
	// (Given our file is valid, for which we account above.)
	//STORM_LOG(Fatal, "Failed to get file size with {0}",
	 //         SystemError(errno));

	STORM_FAIL_FATALLY("Failed to query file size");
	return 0;
}

uint64_t File::Seek(uint32_t whence, int64_t offset, bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use GetPosition() on a closed file!");

	BOOST_STATIC_ASSERT_MSG(sizeof(off_t) == sizeof(int64_t),
	                        "64bit off_t required");

	auto r = lseek(m_handle,
	               static_cast<off_t>(offset),
	               static_cast<int>(whence));

	if (r != -1)
		return static_cast<uint64_t>(r);

	ec.assign(errno, bsys::system_category());
	return 0;
}

vstd::size_type File::Read(void* buffer,
                           vstd::size_type bytes,
                           bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Read() on a closed file!");

	uint8_t* byteBuffer = static_cast<uint8_t*>(buffer);
	vstd::size_type bytesRead = 0;

	while (bytesRead < bytes) {
		const auto n = STORM_HANDLE_EINTR(read(m_handle,
		                                       byteBuffer + bytesRead,
		                                       bytes - bytesRead));

		// Check for EOF
		if (n == 0)
			break;

		if (n < 0) {
			ec.assign(errno, bsys::system_category());
			return 0;
		}

		bytesRead += n;
	}

	return bytesRead;
}

vstd::size_type File::Read(uint64_t offset,
                           void* buffer,
                           vstd::size_type bytes,
                           bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Read() on a closed file!");

	uint8_t* byteBuffer = static_cast<uint8_t*>(buffer);
	vstd::size_type bytesRead = 0;

	while (bytesRead < bytes) {
		const auto n = STORM_HANDLE_EINTR(pread(m_handle,
		                                        byteBuffer + bytesRead,
		                                        bytes - bytesRead,
		                                        offset + bytesRead));

		// Check for EOF
		if (n == 0)
			break;

		if (n < 0) {
			ec.assign(errno, bsys::system_category());
			return 0;
		}

		bytesRead += n;
	}

	return bytesRead;
}

vstd::size_type File::Write(const void* buffer, vstd::size_type bytes,
                            bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Write() on a closed file!");

	const uint8_t* byteBuffer = static_cast<const uint8_t*>(buffer);
	vstd::size_type bytesWritten = 0;

	while (bytesWritten < bytes) {
		const auto n = STORM_HANDLE_EINTR(write(m_handle,
		                                        byteBuffer + bytesWritten,
		                                        bytes - bytesWritten));

		if (n < 0) {
			ec.assign(errno, bsys::system_category());
			return 0;
		}

		bytesWritten += n;
	}

	return bytesWritten;
}

vstd::size_type File::Write(uint64_t offset,
                            const void* buffer, vstd::size_type bytes,
                            bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Write() on a closed file!");

	const uint8_t* byteBuffer = static_cast<const uint8_t*>(buffer);
	vstd::size_type bytesWritten = 0;

	while (bytesWritten < bytes) {
		const auto n = STORM_HANDLE_EINTR(pwrite(m_handle,
		                                         byteBuffer + bytesWritten,
		                                         bytes - bytesWritten,
		                                         offset + bytesWritten));

		if (n < 0) {
			ec.assign(errno, bsys::system_category());
			return 0;
		}

		bytesWritten += n;
	}

	return bytesWritten;
}

void File::GetFileTimes(std::time_t& creationTime,
                        std::time_t& lastAccessTime,
                        std::time_t& lastWriteTime,
                        bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use GetFileTimes() on a closed file!");

	struct stat st;
	if (fstat(m_handle, &st) == 0) {
		creationTime = 0;
		lastAccessTime = st.st_atime;
		lastWriteTime = st.st_mtime;
	} else {
		ec.assign(errno, bsys::system_category());
	}
}

void File::Stat(uint64_t* size,
                std::time_t* creationTime,
                std::time_t* lastAccessTime,
                std::time_t* lastWriteTime,
                uint32_t* flags,
                bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Stat() on a closed file!");

	struct stat st;
	if (fstat(m_handle, &st) == 0) {
		if (size)
			*size = st.st_size;

		if (creationTime)
			*creationTime = 0;

		if (lastAccessTime)
			*lastAccessTime = st.st_atime;

		if (lastWriteTime)
			*lastWriteTime = st.st_mtime;

		if (flags)
			*flags = S_ISDIR(st.st_mode) ? FileFlags::kDirectory : 0;
	} else {
		ec.assign(errno, bsys::system_category());
	}
}

}
