#pragma once
#include <windows.h>
#include <lzo/lzo1x.h>
#include <Basic/Singleton.h>
#include <memory>

class CLZObject
{
public:
#pragma pack(4)
	typedef struct SHeader
	{
		uint32_t dwFourCC;
		uint32_t dwEncryptSize;
		uint32_t dwCompressedSize;
		uint32_t dwRealSize;
	} THeader;
#pragma pack()

	CLZObject();
	~CLZObject();

	void Clear();
	bool BeginDecompress(const void* pvIn);
	bool Decompress(const uint32_t* pdwKey = nullptr);

	const THeader& GetHeader()
	{
		return *m_pHeader;
	}
	uint8_t* GetBuffer()
	{
		return m_pbBuffer.get();
	}
	uint32_t GetSize();
	uint32_t GetBufferSize()
	{
		return m_dwBufferSize;
	}

private:
	void Initialize();
	std::unique_ptr<uint8_t[]> m_pbBuffer;
	uint32_t m_dwBufferSize;
	THeader* m_pHeader;
	const uint8_t* m_pbIn;
	bool m_bCompressed;
	bool m_bInBuffer;

public:
	static uint32_t	ms_dwFourCC;
};

class CLZO : public Singleton<CLZO>
{
public:
	CLZO();
	virtual ~CLZO();
	bool Decompress(CLZObject& rObj, const uint8_t* pbBuf, const uint32_t* pdwKey = nullptr);
	uint8_t* GetWorkMemory();

private:
	uint8_t* m_pWorkMem;
};
