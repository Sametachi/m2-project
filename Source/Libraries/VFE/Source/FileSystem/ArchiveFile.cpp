#include <StdAfx.hpp>
#include <VFE/Include/FileSystem/ArchiveFile.hpp>

#include <storm/io/View.hpp>
#include <cstring>

ArchiveFile::ArchiveFile(std::unique_ptr<uint8_t[]>&& data, uint32_t size)
: m_data(std::move(data))
, m_size(size)
{
}

uint32_t ArchiveFile::GetSize() const
{
    return m_size;
}

bool ArchiveFile::Read(uint32_t offset, void* buffer, uint32_t bytes) const
{
    if (offset + bytes <= m_size) 
    {
        std::memcpy(buffer, m_data.get() + offset, bytes);
        return true;
    }

    return false;
}

bool ArchiveFile::GetView(uint32_t offset, storm::View& view, uint32_t bytes) const
{
    if (offset + bytes <= m_size) 
    {
        view.Initialize(m_data.get() + offset);
        return true;
    }

    return false;
}
