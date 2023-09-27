#include <StdAfx.hpp>
#include "PackageCreator.hpp"
#include <FileSystem/FilenameWrapper.hpp>

#include <MetinDefault/MappedFile.hpp>

#include <Modules/xxhash.h>
#include <Modules/lz4.h>
#include <Modules/lz4hc.h>

bool PackageCreator::Create(const std::string& filename)
{
	bsys::error_code error_code;
	m_file.Open(filename, error_code, storm::AccessMode::kWrite, storm::CreationDisposition::kCreateAlways, storm::ShareMode::kNone, storm::UsageHint::kSequential);

	if (error_code)
	{
		WarnLog("Failed to open {0} with {1}", filename, error_code.message());
		return false;
	}

	m_file.Seek(storm::Whence::kBegin, sizeof(ArchiveHeader), error_code);

	if (error_code)
	{
		WarnLog("Failed to open/seek {0} with {1}", filename, error_code.message());
		return false;
	}

	return true;
}

bool PackageCreator::Add(const FilenameWrapper& archivedPath, const std::string& diskPath, uint32_t flags)
{
	storm::FileLoader src(storm::GetDefaultAllocator());

	bsys::error_code error_code;
	src.Load(diskPath, error_code);

	if (error_code)
	{
		WarnLog("Failed to open {0} with {1}", diskPath, error_code.message());
		return false;
	}

	if (src.GetSize() == 0) 
	{
		SysLog("There is no point to create a file which has 0 size..");
		return false;
	}

	ArchiveFileEntry entry{};
	entry.filenameHash = archivedPath.GetHash();
	entry.flags = flags;
	entry.offset = m_file.Seek(storm::Whence::kCurrent, 0, error_code);
	entry.size = src.GetSize();
	entry.diskSize = entry.size;
	entry.prefixFixer = ArchiveFileEntry::kprefixFixer;

	TempOrOutputBuffer diskData(src.GetData());

	if (entry.flags & kFileFlagLz4) 
	{
		const int32_t bound = LZ4_compressBound(entry.size);
		diskData.MakeTemporaryBuffer(bound);

		const int32_t compressedSize = LZ4_compress_HC(reinterpret_cast<const char*>(src.GetData()), reinterpret_cast<char*>(diskData.pointer), entry.size, bound, CompressionSize);
		if (0 == compressedSize)
			return false;

		entry.diskSize = compressedSize;
	}

	m_file.Write(diskData.pointer, entry.diskSize, error_code);

	if (error_code)
	{
		WarnLog("Failed to write {0} bytes with {1}", entry.diskSize, error_code.message());
		return false;
	}
	
	m_files.push_back(entry);
	return true;
}

bool PackageCreator::Save()
{
	ArchiveHeader hdr{};
	hdr.fourCc = ArchiveHeader::kFourCc;
	hdr.version = ArchiveHeader::kVersion;
	bsys::error_code error_code;
	hdr.fileInfoOffset = m_file.Seek(storm::Whence::kCurrent, 0, error_code);
	hdr.fileCount = m_files.size();
	hdr.misMatchFixer = ArchiveHeader::kmisMatchFixer;

	for (const ArchiveFileEntry& fileEntry : m_files)
	{
		m_file.Write(&fileEntry, sizeof(fileEntry), error_code);
		if (error_code)
		{
			WarnLog("Failed to Write with error code: {0}", error_code.message());
			return false;
		}
	}

	m_file.Write(0, &hdr, sizeof(hdr), error_code);
	if (error_code)
	{
		WarnLog("Failed to Write with error code: {0}", error_code.message());
		return false;
	}

	return true;
}
