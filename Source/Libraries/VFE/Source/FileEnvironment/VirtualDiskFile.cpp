#include <StdAfx.hpp>
#include <VFE/Include/FileEnvironment/VirtualDiskFile.hpp>
#include <VFE/Include/FileEnvironment/FileSystem.hpp>
#include <limits>
#include <storm/io/View.hpp>

bool VirtualDiskFile::Open(const storm::StringRef& path)
{
	bsys::error_code error_code;
	m_file.Open(path, error_code, storm::AccessMode::kRead, storm::CreationDisposition::kOpenExisting, storm::ShareMode::kRead);

	if (error_code)
	{
		if (LogMissingPackageFiles)
		{
			SysLog("Failed to open '{0}' for reading with '{1}'", path, error_code.message());
		}
		return false;
	}

	return true;
}

uint32_t VirtualDiskFile::GetSize() const
{
	const uint64_t size = m_file.GetSize();
	// We don't want to have more size than unsigned int's maximum!
	STORM_ASSERT(size < std::numeric_limits<uint32_t>::max(), "Sanity check");

	return static_cast<uint32_t>(size);
}

bool VirtualDiskFile::Read(uint32_t offset, void* buffer, uint32_t bytes) const
{
	bsys::error_code error_code;
	vstd::size_type read = m_file.Read(offset, buffer, bytes, error_code);

	if (error_code)
	{
		SysLog("Failed to read {0} bytes from file at pos {1} with {2}", bytes, offset, error_code.message());
		return false;
	}

	if (read != bytes) 
	{
		SysLog("Could only read {0} (instead of {1}) from file pos {2}", read, bytes, offset);
		return false;
	}

	return true;
}

bool VirtualDiskFile::GetView(uint32_t offset, storm::View& view, uint32_t bytes) const
{
	uint8_t* view_pointer = view.Initialize(bytes);
	return Read(offset, view_pointer, bytes);
}
