#pragma once
#pragma warning(disable:4786) // Character 255
#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <vector>
#include <map>
#include <Basic/robin_hood.hpp>

using CTokenVector = std::vector<std::string>;
using CTokenMap = robin_hood::unordered_map<std::string, std::string>;
using CTokenVectorMap = robin_hood::unordered_map<std::string, CTokenVector>;
using TStringMap = robin_hood::unordered_map<std::string, std::string>;

// CMemoryTextFileLoader class
class CMemoryTextFileLoader
{
public:
	CMemoryTextFileLoader() = default;
	virtual ~CMemoryTextFileLoader() = default;

	void Bind(std::string_view data);
	uint32_t GetLineCount() const;
	bool CheckLineIndex(uint32_t dwLine) const;
	bool SplitLine(uint32_t dwLine, CTokenVector* pstTokenVector, const char* c_szDelimeter = " \t");
	int32_t SplitLine2(uint32_t dwLine, CTokenVector* pstTokenVector, const char* c_szDelimeter = " \t");
	bool SplitLineByTab(uint32_t dwLine, CTokenVector* pstTokenVector);
	const std::string& GetLineString(uint32_t dwLine);

protected:
	std::vector<std::string> m_stLineVector;
};

// CMemoryFileLoader class
class CMemoryFileLoader
{
public:
	CMemoryFileLoader(int32_t size, const void* c_pvMemoryFile);
	virtual ~CMemoryFileLoader() = default;

	bool Read(int32_t size, void* pvDst);

	int32_t GetPosition() const;
	int32_t GetSize() const;

protected:
	bool IsReadableSize(int32_t size) const;
	const char* GetCurrentPositionPointer() const;

protected:
	const char* m_pcBase;
	int32_t m_size;
	int32_t m_pos;
};

// CDiskFileLoader class
class CDiskFileLoader
{
public:
	CDiskFileLoader();
	virtual ~CDiskFileLoader();

	void Close();
	bool Open(const char* c_szFileName);
	bool Read(int32_t size, void* pvDst) const;
	int32_t GetSize() const;

protected:
	void Initialize();

protected:
	FILE* m_fp{};
	int32_t m_size{};
};