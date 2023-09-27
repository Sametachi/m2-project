#pragma once
#include <VFE/Include/FileEnvironment/VSFile.hpp>
#include <storm/io/File.hpp>

class VirtualDiskFile : public VSFile
{
public:
	virtual ~VirtualDiskFile() = default;
	bool Open(const storm::StringRef& path);
	virtual uint32_t GetSize() const;
	virtual bool Read(uint32_t offset, void* buffer, uint32_t bytes) const;
	virtual bool GetView(uint32_t offset, storm::View& view, uint32_t bytes) const;

private:
	mutable storm::File m_file;
};