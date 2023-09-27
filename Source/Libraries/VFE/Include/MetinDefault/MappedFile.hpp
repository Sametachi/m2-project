#pragma once
#include <VFE/Include/MetinDefault/FileBase.hpp>
#include <memory>

class CVirtualFile : public CFileBase
{
public:
	enum ESeekType
	{
		SEEK_TYPE_BEGIN,
		SEEK_TYPE_CURRENT,
		SEEK_TYPE_END
	};

	CVirtualFile();
	~CVirtualFile() override;

	bool Create(const std::string& filename);
	void Create(size_t size);

	void Destroy();

	int32_t Map(int32_t offset=0, int32_t size=0);
	int32_t Seek(uint32_t offset, int32_t iSeekType = SEEK_TYPE_BEGIN);

	uint32_t GetSize() const;
	const void* GetData() const;

	bool Read(void* dest, uint32_t bytes);
	bool Read(size_t seekPos, void* dest, uint32_t bytes);

	uint32_t GetSeekPosition();
	uint8_t* GetCurrentSeekPoint();

private:
	void Unmap(LPCVOID data);

	uint8_t* m_pbBufLinkData;
	uint32_t m_dwBufLinkSize;
	uint32_t m_seekPosition;
	HANDLE m_hFM;
	uint32_t m_dataOffset;
	uint32_t m_mapSize;
	LPVOID m_lpMapData;
	std::unique_ptr<uint8_t[]> m_ownedBuffer;
};
