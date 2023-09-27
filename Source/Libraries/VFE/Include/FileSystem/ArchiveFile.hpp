#pragma once
#include <memory>
#include <VFE/Include/FileEnvironment/VSFile.hpp>

namespace storm 
{
	class View;
}

class ArchiveFile : public VSFile
{
public:
	ArchiveFile(std::unique_ptr<uint8_t[]>&& data, uint32_t size);
	virtual ~ArchiveFile() = default;

	uint32_t GetSize() const;
	bool Read(uint32_t offset, void* buffer, uint32_t bytes) const;
	bool GetView(uint32_t offset, storm::View& view, uint32_t bytes) const;

private:
	std::unique_ptr<uint8_t[]> m_data;
	uint32_t m_size;
};
