#include <StdAfx.hpp>
#include <VFE/Include/FileEnvironment/FileSystem.hpp>
#include <VFE/Include/FileEnvironment/FileSystemProvider.hpp>
#include <Storm/StringUtil.hpp>

bool CanReadDiskFiles = false;
bool LogMissingPackageFiles = false;

namespace
{
	static FileSystem* FileSystemInstance = nullptr;
}

void FileSystem::RegisterProvider(FileSystemProvider* provider)
{
	m_providers.emplace_back(provider);
}

bool FileSystem::Exists(const storm::String& path)
{
	std::string output;
	const size_t len = path.length();
	output.resize(len);

	if (len == 0)
		return NULL;

	storm::CopyAndLowercaseFilename(&output[0], path.data(), len);

	for (FileSystemProvider* prov : m_providers)
	{
		if (prov->DoesFileExist(output))
			return true;
	}

	return false;
}

std::unique_ptr<VSFile> FileSystem::Open(std::string_view path, uint32_t flags)
{
	std::string output;
	const size_t len = path.length();
	output.resize(len);

	if (len == 0)
		return nullptr;

	storm::CopyAndLowercaseFilename(&output[0], path.data(), len);

	for (FileSystemProvider* prov : m_providers)
	{
		std::unique_ptr<VSFile> file = prov->OpenFile(output, flags);
		if (file)
			return file;
	}

	return nullptr;
}

std::optional<std::string> FileSystem::LoadFileToString(FileSystem& vfs, const storm::String& filename)
{
	std::string s = "";
	std::unique_ptr<VSFile> filePointer = vfs.Open(filename, Buffered);
	if (!filePointer)
		return std::nullopt;

	const uint32_t size = filePointer->GetSize();
	if (!size)
		return s;

	s.resize(size);
	if (filePointer->Read(0, &*s.begin(), size))
		return s;

	return std::nullopt;
}

bool FileSystem::SeekFile(const FilenameWrapper& path, CVirtualFile& fp) const
{
	if (CanReadDiskFiles)
	{
		if (fp.Create(path.GetPath()))
			return true;

		const auto it = m_files.find(path.GetHash());
		if (it != m_files.end())
		{
			const std::pair<FSArchive*, ArchiveFileEntry>& entry = it->second;
			return entry.first->GetFile(path.GetPath(), entry.second, fp);
		}
		return false;
	}
	else
	{
		const auto it = m_files.find(path.GetHash());
		if (it != m_files.end())
		{
			const std::pair<FSArchive*, ArchiveFileEntry>& entry = it->second;
			return entry.first->GetFile(path.GetPath(), entry.second, fp);
		}

		if (fp.Create(path.GetPath()))
			return true;
	}
	return false;
}

void FileSystem::ReadDiskFiles(bool canDo)
{
	CanReadDiskFiles = canDo;
}

void FileSystem::LogMissingFiles(bool canDo)
{
	LogMissingPackageFiles = canDo;
}

FileSystem& CallFS()
{
	STORM_ASSERT(FileSystemInstance, "nullptr VirtualFileSystemInstance is not support.\nAre you dumb bruhw? XD");
	return *FileSystemInstance;
}

void SetVirtualStepFileSyster(FileSystem* vfs)
{
	FileSystemInstance = vfs;
}