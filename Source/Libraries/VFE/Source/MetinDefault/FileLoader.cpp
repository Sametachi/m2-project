#include <StdAfx.hpp>
#include <VFE/Include/MetinDefault/FileLoader.hpp>
#include <assert.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

// CMemoryTextFileLoader class
bool CMemoryTextFileLoader::SplitLineByTab(uint32_t dwLine, CTokenVector* pstTokenVector)
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();

	const std::string& c_rstLine = GetLineString(dwLine);
	const int32_t c_iLineLength = c_rstLine.length();

	if (0 == c_iLineLength)
		return false;

	int32_t basePos = 0;

	do
	{
		const int32_t beginPos = c_rstLine.find_first_of('\t', basePos);
		pstTokenVector->emplace_back(c_rstLine.substr(basePos, beginPos - basePos));
		basePos = beginPos + 1;
	} 
	while (basePos < c_iLineLength && basePos > 0);

	return true;
}

int32_t CMemoryTextFileLoader::SplitLine2(uint32_t dwLine, CTokenVector* pstTokenVector, const char* c_szDelimeter)
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();
	const std::string& c_rstLine = GetLineString(dwLine);
	uint32_t basePos = 0;

	do
	{
		int32_t beginPos = c_rstLine.find_first_not_of(c_szDelimeter, basePos);

		if (beginPos < 0)
			return -1;

		int32_t endPos;

		if (c_rstLine[beginPos] == '"')
		{
			++beginPos;
			endPos = c_rstLine.find_first_of('\"', beginPos);

			if (endPos < 0)
				return -2;

			basePos = endPos + 1;
		}
		else
		{
			endPos = c_rstLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->emplace_back(c_rstLine.substr(beginPos, endPos - beginPos));

		// Check if there is a tab at the end.
		if (int32_t(c_rstLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} 
	while (basePos < c_rstLine.length());

	return 0;
}

bool CMemoryTextFileLoader::SplitLine(uint32_t dwLine, CTokenVector* pstTokenVector, const char* c_szDelimeter)
{
	pstTokenVector->reserve(10);
	pstTokenVector->clear();
	const std::string& c_rstLine = GetLineString(dwLine);
	uint32_t basePos = 0;

	do
	{
		int32_t beginPos = c_rstLine.find_first_not_of(c_szDelimeter, basePos);
		if (beginPos < 0)
			return false;

		int32_t endPos;

		if (c_rstLine[beginPos] == '"')
		{
			++beginPos;
			endPos = c_rstLine.find_first_of('\"', beginPos);

			if (endPos < 0)
				return false;

			basePos = endPos + 1;
		}
		else
		{
			endPos = c_rstLine.find_first_of(c_szDelimeter, beginPos);
			basePos = endPos;
		}

		pstTokenVector->emplace_back(c_rstLine.substr(beginPos, endPos - beginPos));

		// Check if there is a tab at the end.
		if (int32_t(c_rstLine.find_first_not_of(c_szDelimeter, basePos)) < 0)
			break;
	} 
	while (basePos < c_rstLine.length());

	return true;
}

uint32_t CMemoryTextFileLoader::GetLineCount() const
{
	return m_stLineVector.size();
}

bool CMemoryTextFileLoader::CheckLineIndex(uint32_t dwLine) const
{
	return dwLine < m_stLineVector.size();
}

const std::string& CMemoryTextFileLoader::GetLineString(uint32_t dwLine)
{
	assert(CheckLineIndex(dwLine));
	return m_stLineVector[dwLine];
}

void CMemoryTextFileLoader::Bind(std::string_view data)
{
	m_stLineVector.reserve(128);
	m_stLineVector.clear();

	boost::split(m_stLineVector, data, boost::is_any_of("\r\n"));
}

// CMemoryFileLoader class
CMemoryFileLoader::CMemoryFileLoader(int32_t size, const void* c_pvMemoryFile)
{
	assert(c_pvMemoryFile != nullptr);

	m_pos = 0;
	m_size = size;
	m_pcBase = static_cast<const char*>(c_pvMemoryFile);
}

int32_t CMemoryFileLoader::GetSize() const
{
	return m_size;
}

int32_t CMemoryFileLoader::GetPosition() const
{
	return m_pos;
}

bool CMemoryFileLoader::IsReadableSize(int32_t size) const
{
	return m_pos + size <= m_size;
}

bool CMemoryFileLoader::Read(int32_t size, void* pvDst)
{
	if (!IsReadableSize(size))
		return false;

	std::memcpy(pvDst, GetCurrentPositionPointer(), size);
	m_pos += size;
	return true;
}

const char* CMemoryFileLoader::GetCurrentPositionPointer() const
{
	assert(m_pcBase != nullptr);
	return (m_pcBase + m_pos);
}

// CDiskFileLoader class
CDiskFileLoader::CDiskFileLoader()
{
	Initialize();
}

CDiskFileLoader::~CDiskFileLoader()
{
	Close();
}

int32_t CDiskFileLoader::GetSize() const
{
	return m_size;
}

bool CDiskFileLoader::Read(int32_t size, void* pvDst) const
{
	assert(m_fp != nullptr);

	const int32_t ret = fread(pvDst, size, 1, m_fp);
	return ret > 0;
}

bool CDiskFileLoader::Open(const char* c_szFileName)
{
	Close();

	if (!c_szFileName[0])
		return false;

	m_fp = fopen(c_szFileName, "rb");

	if (!m_fp)
		return false;

	fseek(m_fp, 0, SEEK_END);
	m_size = ftell(m_fp);
	fseek(m_fp, 0, SEEK_SET);
	return true;
}

void CDiskFileLoader::Close()
{
	if (m_fp)
		fclose(m_fp);

	Initialize();
}

void CDiskFileLoader::Initialize()
{
	m_fp = nullptr;
	m_size = 0;
}
