//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/File.hpp>
#include <storm/io/WindowsFilename.hpp>
#include <storm/WindowsPlatform.hpp>
#include <storm/String.hpp>

#include <boost/system/error_code.hpp>
#include <boost/static_assert.hpp>
#include <spdlog/spdlog.h>

namespace storm
{

namespace
{

BOOST_STATIC_ASSERT_MSG(
	Whence::kBegin == FILE_BEGIN &&
	Whence::kCurrent == FILE_CURRENT &&
	Whence::kEnd == FILE_END,
	"Storm origins must match POSIX ones"
);

const uint64_t kTicksPerSecond = 10000000;
const uint64_t kEpochDifference = 11644473600;

void FileTimeToStandardTime(uint32_t lowPart, uint32_t highPart, time_t& ctime)
{
	ULARGE_INTEGER conv;
	conv.LowPart = lowPart;
	conv.HighPart = highPart;

	ctime = conv.QuadPart / kTicksPerSecond - kEpochDifference;
}

void StandardTimeToFileTime(uint32_t& lowPart, uint32_t& highPart, time_t ctime)
{
	ULARGE_INTEGER conv;
	conv.QuadPart = ctime * kTicksPerSecond + kEpochDifference;

	lowPart = conv.LowPart;
	highPart = conv.HighPart;
}

vstd::size_type ReadFileSafe(HANDLE handle,
                             void* buffer, vstd::size_type size,
                             bsys::error_code& ec)
{
	DWORD bytesRead = 0;
	if (ReadFile(handle, buffer, size, &bytesRead, NULL))
		return static_cast<vstd::size_type>(bytesRead);

	auto err = GetLastError();
	if (err == ERROR_HANDLE_EOF)
		return 0;

	ec.assign(err, bsys::system_category());
	return 0;
}

vstd::size_type ReadFileSafe(HANDLE handle,
                             uint64_t offset,
                             void* buffer, vstd::size_type size,
                             bsys::error_code& ec)
{
	LARGE_INTEGER offset2;
	offset2.QuadPart = offset;

	OVERLAPPED overlapped = {0};
	overlapped.Offset = offset2.LowPart;
	overlapped.OffsetHigh = offset2.HighPart;

	DWORD bytesRead = 0;
	if (ReadFile(handle, buffer, size, &bytesRead, &overlapped))
		return static_cast<vstd::size_type>(bytesRead);

	auto err = GetLastError();
	if (err == ERROR_HANDLE_EOF)
		return 0;

	ec.assign(err, bsys::system_category());
	return 0;
}

}

File::File()
	: m_handle(INVALID_HANDLE_VALUE)
{
	// ctor
}

File::File(File&& file)
	: m_handle(file.m_handle)
{
	file.m_handle = INVALID_HANDLE_VALUE;
}

File::~File()
{
	Close();
}

File& File::operator=(File&& file)
{
	Close();
	m_handle = file.m_handle;
	file.m_handle = INVALID_HANDLE_VALUE;
	return *this;
}

void File::Open(const StringRef& filename, bsys::error_code& ec,
                uint32_t accessMode, CreationDisposition creationDisposition,
                uint32_t shareMode, uint32_t usageHint)
{
	STORM_ASSERT(!IsOpen(), "Open() called on already opened file.");

	//
	// Translate the creation disposition.
	// kDefault is dispatched to either OPEN_ALWAYS or OPEN_EXISTING,
	// depending on the access mode.
	//

	DWORD dwCreationDisposition = 0;
	switch (creationDisposition) {
		case CreationDisposition::kDefault: {
			if (accessMode & AccessMode::kWrite)
				dwCreationDisposition = OPEN_ALWAYS;
			else
				dwCreationDisposition = OPEN_EXISTING;

			break;
		}

		case CreationDisposition::kCreateNew:
			dwCreationDisposition = CREATE_NEW;
			break;

		case CreationDisposition::kCreateAlways:
			dwCreationDisposition = CREATE_ALWAYS;
			break;

		case CreationDisposition::kOpenExisting:
			dwCreationDisposition = OPEN_EXISTING;
			break;

		case CreationDisposition::kOpenAlways:
			dwCreationDisposition = OPEN_ALWAYS;
			break;

		case CreationDisposition::kTruncateExisting:
			dwCreationDisposition = TRUNCATE_EXISTING;
			break;

		default:
			ec = make_error_code(bsys::errc::invalid_argument);
#ifdef _DEBUG
		//SPDLOG_ERROR("Creation disposition ({0}) is invalid.", creationDisposition);
#endif
			return;
	}

	// Translate access mode flags
	DWORD dwDesiredAccess = 0;
	if (accessMode & AccessMode::kRead)
		dwDesiredAccess |= GENERIC_READ;
	if (accessMode & AccessMode::kWrite)
		dwDesiredAccess |= GENERIC_WRITE;

	// Translate share mode flags
	DWORD dwShareMode = 0;
	if (shareMode & ShareMode::kRead)
		dwShareMode |= FILE_SHARE_READ;
	if (shareMode & ShareMode::kWrite)
		dwShareMode |= FILE_SHARE_WRITE;

	// Translate usage hint
	DWORD dwFlagsAndAttributes = 0;
	if (usageHint == UsageHint::kSequential)
		dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
	else if (usageHint == UsageHint::kRandomAccess)
		dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;

	WindowsFilename realFilename(filename, ec);
	if (!ec) {
		m_handle = CreateFileW(realFilename.GetFilename(),
		                       dwDesiredAccess,
		                       dwShareMode,
		                       nullptr,
		                       dwCreationDisposition,
		                       dwFlagsAndAttributes,
		                       NULL);

		if (m_handle == INVALID_HANDLE_VALUE)
			ec.assign(GetLastError(), bsys::system_category());
	}
}

void File::Close()
{
	auto handle = m_handle;
	if (handle != INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle(handle))
		{
			STORM_FAIL_FATALLY("CloseHandle() failed");
		}

		m_handle = INVALID_HANDLE_VALUE;
	}
}

bool File::IsOpen()
{
	return m_handle != INVALID_HANDLE_VALUE;
}

uint64_t File::GetSize()
{
	STORM_ASSERT(IsOpen(), "Cannot use GetSize() on a closed file!");

	LARGE_INTEGER fileSize;

	if (GetFileSizeEx(m_handle, &fileSize))
		return static_cast<uint64_t>(fileSize.QuadPart);

	// There's no reason why this should fail.
	// (Given our file is valid, for which we account above.)
#ifdef _DEBUG	
		SPDLOG_CRITICAL("Failed to query file size with {0}",
	          GetLastError());
#endif
	STORM_FAIL_FATALLY("Failed to query file size");
	return 0;
}

uint64_t File::Seek(uint32_t whence, int64_t offset, bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use GetPosition() on a closed file!");

	LARGE_INTEGER pos, res;
	pos.QuadPart = offset;

	if (SetFilePointerEx(m_handle, pos, &res, static_cast<DWORD>(whence)))
		return res.QuadPart;

	ec.assign(GetLastError(), bsys::system_category());
	return 0;
}

vstd::size_type File::Read(void* buffer, vstd::size_type bytes,
                           bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Read() on a closed file!");

	uint8_t* byteBuffer = static_cast<uint8_t*>(buffer);
	vstd::size_type bytesRead = 0;

	while (bytesRead != bytes) {
		auto readNow = ReadFileSafe(m_handle,
		                            byteBuffer + bytesRead,
		                            bytes - bytesRead,
		                            ec);

		// An error occurred - exit now
		if (ec)
			return 0;

		// EOF reached
		if (readNow == 0)
			break;

		bytesRead += readNow;
	}

	return bytesRead;
}

vstd::size_type File::Read(uint64_t offset,
                           void* buffer, vstd::size_type bytes,
                           bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Read() on a closed file!");

	uint8_t* byteBuffer = static_cast<uint8_t*>(buffer);
	vstd::size_type bytesRead = 0;

	while (bytesRead != bytes) {
		auto readNow = ReadFileSafe(m_handle,
		                            offset + bytesRead,
		                            byteBuffer + bytesRead,
		                            bytes - bytesRead,
		                            ec);

		// An error occurred - exit now
		if (ec)
			return 0;

		// EOF reached
		if (readNow == 0)
			break;

		bytesRead += readNow;
	}

	return bytesRead;
}

vstd::size_type File::Write(const void* buffer, vstd::size_type bytes,
                            bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use Write() on a closed file!");

	const uint8_t* byteBuffer = static_cast<const uint8_t*>(buffer);
	vstd::size_type bytesWritten = 0;

	while (bytesWritten != bytes) {
		DWORD writtenNow = 0;
		const auto res = ::WriteFile(m_handle,
		                             byteBuffer + bytesWritten,
		                             bytes - bytesWritten,
		                             &writtenNow,
		                             NULL);

		if (!res) {
			ec.assign(GetLastError(), bsys::system_category());
			return 0;
		}

		bytesWritten += writtenNow;
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

	while (bytesWritten != bytes) {
		LARGE_INTEGER offset2;
		offset2.QuadPart = offset + bytesWritten;

		OVERLAPPED overlapped = {0};
		overlapped.Offset = offset2.LowPart;
		overlapped.OffsetHigh = offset2.HighPart;

		DWORD writtenNow = 0;
		const auto res = ::WriteFile(m_handle,
		                             byteBuffer + bytesWritten,
		                             bytes - bytesWritten,
		                             &writtenNow,
		                             &overlapped);

		if (!res) {
			ec.assign(GetLastError(), bsys::system_category());
			return 0;
		}

		bytesWritten += writtenNow;
	}

	return bytesWritten;
}

void File::GetFileTimes(std::time_t& creationTime,
                        std::time_t& lastAccessTime,
                        std::time_t& lastWriteTime,
                        bsys::error_code& ec)
{
	STORM_ASSERT(IsOpen(), "Cannot use GetFileTimes() on a closed file!");

	FILETIME ftCreationTime, ftAccessTime, ftWriteTime;

	if (GetFileTime(m_handle, &ftCreationTime, &ftAccessTime, &ftWriteTime)) {
		FileTimeToStandardTime(ftCreationTime.dwLowDateTime,
		                       ftCreationTime.dwHighDateTime,
		                       creationTime);

		FileTimeToStandardTime(ftAccessTime.dwLowDateTime,
		                       ftAccessTime.dwHighDateTime,
		                       lastAccessTime);

		FileTimeToStandardTime(ftWriteTime.dwLowDateTime,
		                       ftWriteTime.dwHighDateTime,
		                       lastWriteTime);
	} else {
		ec.assign(GetLastError(), bsys::system_category());
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

#if _WIN32_WINNT > 0x502
	FILE_BASIC_INFO basicInfo;
	FILE_STANDARD_INFO stdInfo;

	if (GetFileInformationByHandleEx(m_handle, FileBasicInfo,
		                             &basicInfo, sizeof(basicInfo)) &&
		GetFileInformationByHandleEx(m_handle, FileStandardInfo,
		                             &stdInfo, sizeof(stdInfo))) {
		if (size)
			*size = static_cast<uint64_t>(stdInfo.EndOfFile.QuadPart);

		if (creationTime)
			FileTimeToStandardTime(basicInfo.CreationTime.LowPart,
			                       basicInfo.CreationTime.HighPart,
			                       *creationTime);

		if (lastAccessTime)
			FileTimeToStandardTime(basicInfo.LastAccessTime.LowPart,
			                       basicInfo.LastAccessTime.HighPart,
			                       *lastAccessTime);

		if (lastWriteTime)
			FileTimeToStandardTime(basicInfo.LastWriteTime.LowPart,
			                       basicInfo.LastWriteTime.HighPart,
			                       *lastWriteTime);

		if (flags) {
			if (!!(basicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				*flags = FileFlags::kDirectory;
			else
				*flags = 0;
		}
	} else {
		ec.assign(GetLastError(), bsys::system_category());
	}
#else
	ec = make_error_code(bsys::errc::not_supported);
#endif
}

}
