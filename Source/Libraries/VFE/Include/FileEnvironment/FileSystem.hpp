#pragma once
#include <VFE/Include/FileEnvironment/VSFile.hpp>
#include <VFE/Include/FileSystem/FilenameWrapper.hpp>
#include <VFE/Include/FileSystem/FSArchive.hpp>
#include <VFE/Include/MetinDefault/MappedFile.hpp>
#include <Basic/robin_hood.hpp>

#include <storm/String.hpp>
#include <memory>
#include <vector>
#include <string>
#include <optional>
class FileSystemProvider;
// To control buffering, we are using Unbuffered and Buffered types.
// By default, it's set to automatic (Buffered).
static const uint32_t Unbuffered = 1 << 0;
static const uint32_t Buffered = 1 << 1;

// A [read-only] Virtual File Environment
class FileSystem
{
public:
	void RegisterProvider(FileSystemProvider* provider);
	bool Exists(const storm::String& path);

	// Open a file for reading.
	std::unique_ptr<VSFile> Open(std::string_view path, uint32_t flags = Buffered);
	// Load a file to string.
	std::optional<std::string> LoadFileToString(FileSystem& vfs, const storm::String& filename);
	// Open and load a file
	bool SeekFile(const FilenameWrapper& path, CVirtualFile& fp) const;

	// Preferences
	void ReadDiskFiles(bool canDo);

	// If the Disk file is 'False' then it will log every single files that are not found in Packages!
	void LogMissingFiles(bool canDo);

private:
	FSArchiveFileDict m_files;
	std::vector<FileSystemProvider*> m_providers;
};

FileSystem& CallFS();
void SetVirtualStepFileSyster(FileSystem* step_file_syster);
extern bool CanReadDiskFiles;
extern bool LogMissingPackageFiles;