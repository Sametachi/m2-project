#pragma once
#include <cstdint>

#define _4CC(b0, b1, b2, b3) (uint32_t(uint8_t(b0)) | (uint32_t(uint8_t(b1)) << 8) | (uint32_t(uint8_t(b2)) << 16) | (uint32_t(uint8_t(b3)) << 24))

enum FileFlags
{
	kFileFlagLz4 = 1 << 18,
};

static const uint64_t kFilenameMagic = 7420643061997133436ull;
static const int32_t CompressionSize = 5;

#pragma pack(push, 1)
struct ArchiveHeader
{
	static const uint32_t kFourCc = _4CC('', '', '', '');
	static const uint32_t kVersion = 1;
	static const uint64_t kmisMatchFixer = 12398384230;

	uint32_t fourCc;
	uint32_t version;
	uint32_t fileInfoOffset;
	uint32_t fileCount;
	uint64_t misMatchFixer;
};

struct ArchiveFileEntry
{
	static const uint64_t kprefixFixer = 9982371244;
	uint64_t filenameHash;
	uint32_t flags;
	uint32_t offset;
	uint32_t diskSize;
	uint32_t size;
	uint64_t prefixFixer;
};
#pragma pack(pop)

// We have to point on the Output or the Temporary buffer,
// In this class we use abstract memoryManagement away from the Consumer.
// So we can Dynamically decide to work with Direct provider buffer or,
// we have to allocate a Temporary buffer for one file!

struct TempOrOutputBuffer
{
	static const uint32_t kLocalSize = 8 * 1024; // 8192

	TempOrOutputBuffer(uint8_t* output) : pointer(output), output(output) {}

	~TempOrOutputBuffer()
	{
		if (pointer != output && pointer != local)
			delete[] pointer;
	}

	void MakeTemporaryBuffer(std::size_t size)
	{
		if (size < sizeof(local))
			pointer = local;
		else
			pointer = new uint8_t[size];
	}

	uint8_t* pointer{};
	uint8_t* output{};
	uint8_t local[kLocalSize]{};
};