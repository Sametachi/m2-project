#pragma once
#include <VFE/Include/FileEnvironment/FileSystemProvider.hpp>

class VSFile;
class VirtualDiskProvider : public FileSystemProvider
{
	public:
		VirtualDiskProvider();
		virtual bool DoesFileExist(const storm::String& path);
		virtual std::unique_ptr<VSFile> OpenFile(std::string_view path, uint32_t flags);
};
