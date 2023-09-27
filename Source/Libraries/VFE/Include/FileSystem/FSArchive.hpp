#pragma once
#include <VFE/Include/FileSystem/ArchiveFormat.hpp>
#include <VFE/Include/MetinDefault/MappedFile.hpp>
#include <Basic/robin_hood.hpp>
#include <storm/io/File.hpp>
#include <string>
#include <utility>

class CVirtualFile;
class FSArchive;
typedef robin_hood::unordered_map<uint64_t, std::pair<FSArchive*, ArchiveFileEntry>> FSArchiveFileDict;

class FSArchive
{
public:
	bool Create(const std::string& filename, FSArchiveFileDict& dict);
	bool Get(std::string_view path, const ArchiveFileEntry&entry, std::unique_ptr<uint8_t[]> &data, uint32_t &size);
	bool GetFile(const std::string& path, const ArchiveFileEntry& entry, CVirtualFile& fp);

private:
	CVirtualFile m_file;
};
