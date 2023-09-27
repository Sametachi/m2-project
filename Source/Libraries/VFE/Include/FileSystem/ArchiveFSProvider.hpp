#pragma once
#include <VFE/Include/FileSystem/FSArchive.hpp>
#include <VFE/Include/FileEnvironment/FileSystemProvider.hpp>
#include <storm/String.hpp>
#include <vector>
#include <memory>

class ArchiveFSProvider : public FileSystemProvider
{
public:
	ArchiveFSProvider();
    bool AddArchive(const std::string& path, const std::string& prefix, bool isHardPrefix);
	virtual bool DoesFileExist(const storm::String& path);

	virtual std::unique_ptr<VSFile> OpenFile(std::string_view path, uint32_t flags);

private:
	FSArchiveFileDict m_files;
	std::vector<std::unique_ptr<FSArchive>> m_paks;
};
