#include <StdAfx.hpp>
#include <VFE/Include/FileSystem/FSArchive.hpp>

#include <VFE/Include/Modules/lz4.h>
#include <VFE/Include/Modules/xxhash.h>
#include <storm/io/StreamUtil.hpp>

bool FSArchive::Create(const std::string& filename, FSArchiveFileDict& dict)
{
    if (!m_file.Create(filename))
        return false;

    ArchiveHeader hdr{};
    if (!m_file.Read(&hdr, sizeof(hdr)))
        return false;

    if (hdr.fourCc != ArchiveHeader::kFourCc)
        return false;

    if (hdr.version != ArchiveHeader::kVersion)
        return false;

    if (hdr.misMatchFixer != ArchiveHeader::kmisMatchFixer)
    {
        FatalLog("Beep-buup, beep-buup, loading files...");
        FatalLog("Sike, get the fuck out of my file system.");
        return false;
    }

    m_file.Seek(hdr.fileInfoOffset);

    for (uint32_t i = 0; i != hdr.fileCount; ++i)
    {
        ArchiveFileEntry e{};
        if (!m_file.Read(&e, sizeof(e)))
            return false;

        dict.insert_or_assign(e.filenameHash, std::make_pair(this, e));
    }

    return true;
}

bool FSArchive::Get(std::string_view path, const ArchiveFileEntry& entry, std::unique_ptr<uint8_t[]>& data, uint32_t& size)
{
    if (entry.prefixFixer != ArchiveFileEntry::kprefixFixer)
    {
        FatalLog("Are you trying to do what?");
        return false;
    }

    size = entry.size;
    data.reset(new uint8_t[entry.size]);

    m_file.Seek(entry.offset);

    if (entry.flags & kFileFlagLz4)
    {
        const int32_t read = LZ4_decompress_safe(reinterpret_cast<const char*>(m_file.GetCurrentSeekPoint()), reinterpret_cast<char*>(data.get()), entry.diskSize, entry.size);

        if (read < 0)
        {
            SysLog("Failed to decompress {0} with {1}", path, read);
            return false;
        }
    }

    return true;
}

bool FSArchive::GetFile(const std::string& path, const ArchiveFileEntry& entry, CVirtualFile& fp)
{
    if (entry.prefixFixer != ArchiveFileEntry::kprefixFixer)
    {
        FatalLog("Are you trying to do what?");
        return false;
    }

    fp.Create(entry.size);

    TempOrOutputBuffer diskData(fp.GetCurrentSeekPoint());

    // If the file is compressed, we'll always need a temporary buffer.
    const bool needsTempBuffer = !!(entry.flags & kFileFlagLz4);
    if (needsTempBuffer)
        diskData.MakeTemporaryBuffer(entry.diskSize);

    m_file.Seek(entry.offset);

    if (!m_file.Read(diskData.pointer, entry.diskSize))
    {
        SysLog("Failed to read {0}", path.c_str());
        return false;
    }

    if (entry.flags & kFileFlagLz4)
    {
        const auto r = LZ4_decompress_fast(reinterpret_cast<const char*>(diskData.pointer), reinterpret_cast<char*>(fp.GetCurrentSeekPoint()), entry.size);

        if (r < 0 || r != entry.diskSize)
        {
            SysLog("Failed to decompress {0} with {1}", path.c_str(), r);
            return false;
        }
    }

    return true;
}
