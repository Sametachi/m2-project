#include <StdAfx.hpp>
#include <VFE/Include/FileEnvironment/VirtualDiskProvider.hpp>
#include <VFE/Include/FileEnvironment/VirtualDiskFile.hpp>
#include <VFE/Include/FileEnvironment/FileSystemProvider.hpp>

#include <storm/io/FileUtil.hpp>

// Main Disk provider
VirtualDiskProvider::VirtualDiskProvider(): ::FileSystemProvider("Disk") {}

bool VirtualDiskProvider::DoesFileExist(const storm::String& path)
{
	bsys::error_code error_code;
	storm::StatFile(path, nullptr, nullptr, nullptr, nullptr, nullptr, error_code);
	return !error_code;
}

std::unique_ptr<VSFile> VirtualDiskProvider::OpenFile(std::string_view path, uint32_t flags)
{
	std::unique_ptr<VirtualDiskFile> file(new VirtualDiskFile());
	if (!file->Open(path))
		return nullptr;

	return std::unique_ptr<VSFile>(std::move(file));
}
