#pragma once

#include <cstdint>

enum EDataType
{
	D_DWORD,
	D_WORD,
	D_BYTE
};

// Used to process map properties
class CAttribute
{
public:
	// Fill all zeros with the dword type.
	CAttribute(uint32_t width, uint32_t height);

	// Attr is read and attributes are read smartly.
	CAttribute(uint32_t * attr, uint32_t width, uint32_t height);

	~CAttribute();

	int32_t GetDataType();
	void * GetDataPtr();

	uint32_t Get(uint32_t position) const;
	uint32_t Get(uint32_t x, uint32_t y);
	void Set(uint32_t x, uint32_t y, uint32_t attr);
	void Remove(uint32_t x, uint32_t y, uint32_t attr);

private:
	void Initialize(uint32_t width, uint32_t height);
	void Alloc(bool initialize);

	int32_t dataType;
	uint32_t defaultAttr;
	uint32_t width, height;

	void* data;

	uint8_t* bytePtr;
	uint16_t* wordPtr;
	uint32_t* dwordPtr;
};