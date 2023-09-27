#include "StdAfx.h"
#include <stdlib.h>
#include "lzo.h"
#include <storm/crypt/Xtea.hpp>
#include <storm/Util.hpp>
#include <VFE/Include/VFE.hpp>
#include <Basic/Logging.hpp>

uint32_t CLZObject::ms_dwFourCC = _4CC('M', 'C', 'O', 'Z');

CLZObject::CLZObject()
{
	Initialize();
}

void CLZObject::Initialize()
{
	m_bInBuffer = false;
	m_pbBuffer = nullptr;
	m_dwBufferSize = 0;

	m_pHeader = nullptr;
	m_pbIn = nullptr;
	m_bCompressed = false;
}

void CLZObject::Clear()
{
	Initialize();
}

CLZObject::~CLZObject()
{
	Clear();
}

uint32_t CLZObject::GetSize()
{
	assert(m_pHeader);

	if (m_bCompressed)
	{
		if (m_pHeader->dwEncryptSize)
			return sizeof(THeader) + sizeof(uint32_t) + m_pHeader->dwEncryptSize;
		else
			return sizeof(THeader) + sizeof(uint32_t) + m_pHeader->dwCompressedSize;
	}
	else
		return m_pHeader->dwRealSize;
}

bool CLZObject::BeginDecompress(const void* pvIn)
{
	THeader* pHeader = (THeader*)pvIn;

	if (pHeader->dwFourCC != ms_dwFourCC)
	{
		SysLog("LZObject: not a valid data");
		return false;
	}

	m_pHeader = pHeader;
	m_pbIn = (const uint8_t*)pvIn + (sizeof(THeader) + sizeof(uint32_t));

	m_pbBuffer.reset(new uint8_t[pHeader->dwRealSize]);
	std::memset(m_pbBuffer.get(), 0, pHeader->dwRealSize);
	return true;
}

class DecryptBuffer
{
public:
	enum
	{
		LOCAL_BUF_SIZE = 8 * 1024,
	};

	DecryptBuffer(uint32_t size)
	{
		if (size >= LOCAL_BUF_SIZE)
			m_buf = new uint8_t[size];
		else
			m_buf = m_local_buf;
	}

	~DecryptBuffer()
	{
		if (m_local_buf != m_buf)
			delete[] m_buf;
	}

	uint8_t* GetBufferPtr()
	{
		return m_buf;
	}

private:
	uint8_t* m_buf;
	uint8_t m_local_buf[LOCAL_BUF_SIZE];
};

bool CLZObject::Decompress(const uint32_t* key)
{
	lzo_uint uiSize;

	if (m_pHeader->dwEncryptSize)
	{
		DecryptBuffer buf(m_pHeader->dwEncryptSize);

		storm::DecryptXtea(buf.GetBufferPtr(), m_pbIn - sizeof(uint32_t), key, m_pHeader->dwEncryptSize);

		if (*(uint32_t*)buf.GetBufferPtr() != ms_dwFourCC)
		{
			SysLog("LZObject: key incorrect");
			return false;
		}

		int32_t SafeDecompressSize = lzo1x_decompress(buf.GetBufferPtr() + sizeof(uint32_t), m_pHeader->dwCompressedSize, m_pbBuffer.get(), &uiSize, nullptr);
		if (LZO_E_OK != SafeDecompressSize)
		{
			SysLog("Lzo Decompress failed with {0} for {1} bytes decrypted data", SafeDecompressSize, m_pHeader->dwCompressedSize);
			return false;
		}
	}
	else
	{
		uiSize = m_pHeader->dwRealSize;

		int32_t SafeDecompressSize = lzo1x_decompress(m_pbIn, m_pHeader->dwCompressedSize, m_pbBuffer.get(), &uiSize, nullptr);
		if (LZO_E_OK != SafeDecompressSize)
		{
			SysLog("Lzo Decompress failed with {0} for {1} bytes", SafeDecompressSize, m_pHeader->dwCompressedSize);
			return false;
		}
	}

	if (uiSize != m_pHeader->dwRealSize)
	{
		SysLog("Size mismatch {0} != {1}", uiSize, m_pHeader->dwRealSize);
		return false;
	}

	return true;
}

CLZO::CLZO() : m_pWorkMem(nullptr)
{
	if (lzo_init() != LZO_E_OK)
	{
		SysLog("LZO: cannot initialize");
		return;
	}

	m_pWorkMem = (uint8_t*)malloc(LZO1X_999_MEM_COMPRESS);
	if (nullptr == m_pWorkMem)
	{
		SysLog("LZO: cannot alloc memory");
		return;
	}
}

CLZO::~CLZO()
{
	if (m_pWorkMem)
	{
		free(m_pWorkMem);
		m_pWorkMem = nullptr;
	}
}

bool CLZO::Decompress(CLZObject& rObj, const uint8_t* pbBuf, const uint32_t* pdwKey)
{
	if (!rObj.BeginDecompress(pbBuf))
		return false;

	if (!rObj.Decompress(pdwKey))
		return false;

	return true;
}

uint8_t* CLZO::GetWorkMemory()
{
	return m_pWorkMem;
}

