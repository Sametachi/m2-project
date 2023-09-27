#pragma once
#include <FileSystem/ArchiveFormat.hpp>

#include <string>
#include <vector>
#include <concurrent_vector.h>

#include <storm/crypt/Xtea.hpp>
#include <storm/io/File.hpp>
#include <storm/io/FileLoader.hpp>

class FilenameWrapper;

class PackageCreator
{
	public:
		~PackageCreator() = default;

		bool Create(const std::string& filename);
		bool Add(const FilenameWrapper& archivedPath, const std::string& diskPath, uint32_t flags);

		bool Save();

	private:
		storm::File m_file;
		concurrency::concurrent_vector<ArchiveFileEntry> m_files;
};