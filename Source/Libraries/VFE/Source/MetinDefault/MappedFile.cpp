#include <StdAfx.hpp>
#include <VFE/Include/MetinDefault/MappedFile.hpp>

CVirtualFile::CVirtualFile()
: m_hFM(nullptr)
, m_lpMapData(nullptr)
, m_dataOffset(0)
, m_mapSize(0)
, m_seekPosition(0)
, m_pbBufLinkData(nullptr)
, m_dwBufLinkSize(0)
{
}

CVirtualFile::~CVirtualFile()
{
	Destroy();
}

bool CVirtualFile::Create(const std::string& filename)
{
	Destroy();

	if (!CFileBase::Create(filename, FILEMODE_READ))
		return false;

	return Map(0, 0) > 0;
}

void CVirtualFile::Create(size_t size)
{
	m_ownedBuffer.reset(new uint8_t[size]);

	m_pbBufLinkData = m_ownedBuffer.get();
	m_dwBufLinkSize = size;
}

const void* CVirtualFile::GetData() const
{
	return m_pbBufLinkData;
}

uint32_t CVirtualFile::GetSize() const
{
	return m_dwBufLinkSize;
}

void CVirtualFile::Destroy()
{
	if (nullptr != m_lpMapData)
	{
		Unmap(m_lpMapData);
		m_lpMapData = nullptr;
	}

	if (nullptr != m_hFM)
	{
		CloseHandle(m_hFM);
		m_hFM = nullptr;
	}

	m_pbBufLinkData = nullptr;
	m_dwBufLinkSize = 0;

	m_seekPosition = 0;
	m_dataOffset = 0;
	m_mapSize = 0;

	CFileBase::Destroy();
}

int32_t CVirtualFile::Map(int32_t offset, int32_t size)
{
	m_dataOffset = offset;

	if (size == 0)
		m_mapSize = m_dwSize;
	else
		m_mapSize = size;

	if (m_dataOffset + m_mapSize > m_dwSize)
		return NULL;

	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	uint32_t dwSysGran = SysInfo.dwAllocationGranularity;
	uint32_t dwFileMapStart = (m_dataOffset / dwSysGran) * dwSysGran;
	uint32_t dwMapViewSize = (m_dataOffset % dwSysGran) + m_mapSize;
	int32_t iViewDelta = m_dataOffset - dwFileMapStart;

	m_hFM = CreateFileMapping(m_hFile, // handle
		nullptr, // security
		PAGE_READONLY, // flProtect
		0, // high
		m_dataOffset + m_mapSize, // low
		nullptr); // name

	if (!m_hFM)
	{
		ConsoleLog("CMappedFile::Map !m_hFM\n");
		return 0;
	}

	m_lpMapData = MapViewOfFile(m_hFM, FILE_MAP_READ, 0, dwFileMapStart, dwMapViewSize);

	if (!m_lpMapData)
	{
		SysLog("CMappedFile::Map !m_lpMapData {0}", GetLastError());
		return 0;
	}

	m_seekPosition = 0;
	m_dwBufLinkSize = m_mapSize;
	m_pbBufLinkData = static_cast<uint8_t*>(m_lpMapData) + iViewDelta;
	return m_mapSize;
}

int32_t CVirtualFile::Seek(uint32_t offset, int32_t iSeekType)
{
	switch (iSeekType)
	{
		case SEEK_TYPE_BEGIN:
			if (offset > m_dwSize)
				offset = m_dwSize;

			m_seekPosition = offset;
			break;

		case SEEK_TYPE_CURRENT:
			m_seekPosition = std::min(m_seekPosition + offset, GetSize());
			break;

		case SEEK_TYPE_END:
			m_seekPosition = (GetSize() >= offset) ? GetSize() - offset : 0;
			break;
	}

	return m_seekPosition;
}

bool CVirtualFile::Read(void* dest, uint32_t bytes)
{
	if (m_seekPosition + bytes > GetSize())
		return false;

	std::memcpy(dest, GetCurrentSeekPoint(), bytes);
	m_seekPosition += bytes;
	return true;
}

bool CVirtualFile::Read(size_t seekPos, void* dest, uint32_t bytes)
{
	if (seekPos + bytes > GetSize())
		return false;

	std::memcpy(dest, m_pbBufLinkData + seekPos, bytes);
	return true;
}

uint32_t CVirtualFile::GetSeekPosition(void)
{
	return m_seekPosition;
}

uint8_t* CVirtualFile::GetCurrentSeekPoint()
{
	return m_pbBufLinkData + m_seekPosition;
}

void CVirtualFile::Unmap(LPCVOID data)
{
	if (!UnmapViewOfFile(data))
		SysLog("CMappedFile::Unmap - Error");
}
