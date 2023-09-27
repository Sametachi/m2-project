//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/FileOperations.hpp>
#include <storm/io/File.hpp>
#include <storm/io/WindowsFilename.hpp>
#include <storm/String.hpp>

#include <boost/system/error_code.hpp>

#if !VSTD_PLATFORM_WINDOWS
#include <unistd.h>
#include <sys/stat.h>
#else
#include <windows.h>
#endif

namespace storm
{

	namespace
	{

#if VSTD_PLATFORM_WINDOWS

		const uint64_t kTicksPerSecond = 10000000;
		const uint64_t kEpochDifference = 11644473600;

		void FileTimeToStandardTime(uint32_t lowPart,
			uint32_t highPart,
			std::time_t& ctime)
		{
			ULARGE_INTEGER conv;
			conv.LowPart = lowPart;
			conv.HighPart = highPart;

			ctime = conv.QuadPart / kTicksPerSecond - kEpochDifference;
		}

		void StandardTimeToFileTime(uint32_t& lowPart,
			uint32_t& highPart,
			std::time_t ctime)
		{
			ULARGE_INTEGER conv;
			conv.QuadPart = ctime * kTicksPerSecond + kEpochDifference;

			lowPart = conv.LowPart;
			highPart = conv.HighPart;
		}

#endif

	}

	void StatFile(const StringRef& filename,
		uint64_t* size,
		std::time_t* creationTime,
		std::time_t* lastAccessTime,
		std::time_t* lastWriteTime,
		uint32_t* flags,
		boost::system::error_code& ec)
	{
		STORM_ASSERT(!filename.empty(), "Filename cannot be empty");

#if VSTD_PLATFORM_WINDOWS
		WindowsFilename wideFilename(filename, ec);
		if (ec)
			return;

		WIN32_FILE_ATTRIBUTE_DATA data;

		if (GetFileAttributesExW(wideFilename.GetFilename(),
			GetFileExInfoStandard,
			&data)) {
			if (size)
				*size = (static_cast<uint64_t>(data.nFileSizeHigh) << 32) +
				data.nFileSizeLow;

			if (creationTime)
				FileTimeToStandardTime(data.ftCreationTime.dwLowDateTime,
					data.ftCreationTime.dwHighDateTime,
					*creationTime);

			if (lastAccessTime)
				FileTimeToStandardTime(data.ftLastAccessTime.dwLowDateTime,
					data.ftLastAccessTime.dwHighDateTime,
					*lastAccessTime);

			if (lastWriteTime)
				FileTimeToStandardTime(data.ftLastWriteTime.dwLowDateTime,
					data.ftLastWriteTime.dwHighDateTime,
					*lastWriteTime);

			if (flags) {
				*flags = !!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ?
					FileFlags::kDirectory : 0;
			}
		}
		else {
			ec.assign(GetLastError(), boost::system::system_category());
		}
#else
		struct stat st;
		String nullFilename(filename);
		if (stat(nullFilename.c_str(), &st) == 0) {
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
		}
		else {
			ec.assign(errno, boost::system::system_category());
		}
#endif
	}

	void CopyFile(const StringRef& srcFilename,
		const StringRef& dstFilename,
		boost::system::error_code& ec,
		bool overwriteIfExists)
	{
		STORM_ASSERT(!srcFilename.empty(), "Source filename cannot be empty");
		STORM_ASSERT(!dstFilename.empty(), "Dest. filename cannot be empty");

#if VSTD_PLATFORM_WINDOWS
		WindowsFilename wideSrcFilename(srcFilename, ec);
		if (ec)
			return;

		WindowsFilename wideDstFilename(dstFilename, ec);
		if (ec)
			return;

		if (!CopyFileW(wideSrcFilename.GetFilename(),
			wideDstFilename.GetFilename(),
			overwriteIfExists))
			ec.assign(GetLastError(), boost::system::system_category());
#else
		File src;
		src.Open(srcFilename, ec,
			AccessMode::kRead,
			CreationDisposition::kOpenExisting,
			ShareMode::kRead,
			UsageHint::kSequential);

		if (!ec) {
			const auto creationDisposition = overwriteIfExists ?
				CreationDisposition::kCreateAlways :
				CreationDisposition::kCreateNew;

			File dst;
			dst.Open(dstFilename, ec,
				AccessMode::kWrite,
				creationDisposition,
				ShareMode::kNone,
				UsageHint::kSequential);

			if (!ec) {
				uint8_t buffer[8192];

				vstd::size_type s = 0;
				do {
					s = src.Read(buffer, sizeof(buffer), ec);
					auto s2 = dst.Write(buffer, s, ec);
					STORM_ASSERT(s == s2, "Sizes don't match");
				} while (!ec && s);
			}
		}
#endif
	}

	void MoveFile(const StringRef& srcFilename,
		const StringRef& dstFilename,
		boost::system::error_code& ec)
	{
		STORM_ASSERT(!srcFilename.empty(), "Source filename cannot be empty");
		STORM_ASSERT(!dstFilename.empty(), "Dest. filename cannot be empty");

#if VSTD_PLATFORM_WINDOWS
		WindowsFilename wideSrcFilename(srcFilename, ec);
		if (ec)
			return;

		WindowsFilename wideDstFilename(dstFilename, ec);
		if (ec)
			return;

		if (!MoveFileExW(wideSrcFilename.GetFilename(),
			wideDstFilename.GetFilename(),
			MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
			ec.assign(GetLastError(), boost::system::system_category());
#else
		String nullSrcFilename(srcFilename);
		String nullDstFilename(dstFilename);
		if (rename(nullSrcFilename.c_str(), nullDstFilename.c_str()) != 0)
			ec.assign(errno, boost::system::system_category());
#endif
	}

}
