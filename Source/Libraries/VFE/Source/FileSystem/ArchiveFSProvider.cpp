#include <StdAfx.hpp>
#include <VFE/Include/FileSystem/ArchiveFSProvider.hpp>
#include <VFE/Include/FileSystem/FSArchive.hpp>
#include <VFE/Include/FileSystem/ArchiveFile.hpp>
#include <VFE/Include/FileEnvironment/FileSystem.hpp>
#include <VFE/Include/Modules/xxhash.h>

// Default Provider
ArchiveFSProvider::ArchiveFSProvider(): FileSystemProvider("VirtualPackages") {}

// Setting up Archives
bool ArchiveFSProvider::AddArchive(const std::string& path, const std::string& prefix, bool isHardPrefix)
{
    std::unique_ptr<FSArchive> archive = std::make_unique<FSArchive>();

	if (isHardPrefix)
	{
		if (!archive->Create(prefix + path, m_files))
		{
			FatalLog("AddArchive:: Failed to load Archive by name: {0}", path.c_str());
			return false;
		}
	}
	else
	{
		if (!archive->Create(path, m_files))
		{
			FatalLog("AddArchive:: Failed to load Archive by name: {0}", path.c_str());
			return false;
		}
	}

    m_paks.emplace_back(std::move(archive));
    return true;
}

// Faster way to check if the File exist
bool ArchiveFSProvider::DoesFileExist(const storm::String& path)
{
	return m_files.find(XXH64(path.data(), path.size(), kFilenameMagic)) != m_files.end();
}

// Main opening, this is used by all other VFE calls.
std::unique_ptr<VSFile> ArchiveFSProvider::OpenFile(std::string_view path, uint32_t flags)
{
	const auto it = m_files.find(XXH64(path.data(), path.size(), kFilenameMagic));
	if (it != m_files.end()) 
	{
		const std::pair<FSArchive*, ArchiveFileEntry>& entry = it->second;
		STORM_ASSERT(entry.first, "No archive bound?");

		std::unique_ptr<uint8_t[]> data;
		uint32_t size;

		if (entry.first->Get(path, entry.second, data, size))
			return std::make_unique<ArchiveFile>(std::move(data), size);
	} 
	else 
	{
		if (LogMissingPackageFiles)
		{
			FatalLog("Couldn't initialize Archive entry by name: {0}", path);
		}
	}

	return nullptr;
}
