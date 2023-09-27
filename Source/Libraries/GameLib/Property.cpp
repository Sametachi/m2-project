#include "StdAfx.h"
#include <cstring>

#include "PropertyManager.h"
#include "Property.h"
#include <Storm/StringUtil.hpp>

CProperty::CProperty(const FilenameWrapper& c_pszFileName) : mc_pFileName(nullptr), m_dwCRC(0)
{
	m_stFileName = c_pszFileName.GetPath().c_str();
	StringPath(m_stFileName);

	mc_pFileName = strrchr(m_stFileName.c_str(), '/');

	if (!mc_pFileName)
		mc_pFileName = m_stFileName.c_str();
	else
		++mc_pFileName;
}

uint32_t CProperty::GetCRC()
{
	return m_dwCRC;
}

const char* CProperty::GetFileName()
{
	return (m_stFileName.c_str());
}

bool CProperty::GetString(const char* c_pszKey, const char** c_ppString)
{
	std::string stTempKey = c_pszKey;
	stl_lowers(stTempKey);
	auto it = m_stTokenMap.find(stTempKey);

	if (m_stTokenMap.end() == it)
		return false;

	*c_ppString = it->second[0].c_str();
	return true;
}

size_t CProperty::GetSize()
{
	return m_stTokenMap.size();
}

bool CProperty::GetVector(const char* c_pszKey, CTokenVector& rTokenVector)
{
	std::string stTempKey = c_pszKey;
	stl_lowers(stTempKey);

	auto it = m_stTokenMap.find(stTempKey);
	if (m_stTokenMap.end() == it)
		return false;

	CTokenVector& rSourceTokenVector = it->second;

	auto itor = rSourceTokenVector.begin();
	for (; itor != rSourceTokenVector.end(); ++itor)
		rTokenVector.emplace_back(*itor);

	return true;
}

void CProperty::PutString(const char* c_pszKey, const char* c_pszString)
{
	std::string stTempKey = c_pszKey;
	stl_lowers(stTempKey);

	auto itor = m_stTokenMap.find(stTempKey);
	if (itor != m_stTokenMap.end())
		m_stTokenMap.erase(itor);

	CTokenVector tokenVector;
	tokenVector.emplace_back(c_pszString);

	m_stTokenMap.emplace(CTokenVectorMap::value_type(stTempKey, tokenVector));
}

void CProperty::PutVector(const char* c_pszKey, const CTokenVector& c_rTokenVector)
{
	std::string stTempKey = c_pszKey;
	stl_lowers(stTempKey);

	m_stTokenMap.emplace(CTokenVectorMap::value_type(stTempKey, c_rTokenVector));
}

#pragma pack(push, 1)
struct PropertyHeader
{
	uint32_t fourCc;
	char sep[2];
};
#pragma pack(pop)

bool CProperty::ReadFromMemory(const void* c_pvData, int iLen, const char* c_pszFileName)
{
	PropertyHeader header;
	if (iLen < sizeof(header))
	{
		SysLog("Property file {0} is too small", c_pszFileName);
		return false;
	}

	std::memcpy(&header, c_pvData, sizeof(header));

	if (header.fourCc != _4CC('Y', 'P', 'R', 'T'))
	{
		SysLog("Property file FourCC is invalid for {0}", c_pszFileName);
		return false;
	}

	if (header.sep[0] != '\r' || header.sep[1] != '\n')
	{
		SysLog("CProperty::ReadFromMemory: File format error after FourCC: {0}\n", c_pszFileName);
		return false;
	}

	const char* pcData = (const char*)c_pvData + sizeof(header);
	iLen -= sizeof(header);

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(std::string_view(pcData, iLen));
	storm::ParseNumber(textFileLoader.GetLineString(0), m_dwCRC);

	CTokenVector stTokenVector;
	for (uint32_t i = 1; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &stTokenVector))
			continue;

		stl_lowers(stTokenVector[0]);
		std::string stKey = stTokenVector[0];

		stTokenVector.erase(stTokenVector.begin());
		PutVector(stKey.c_str(), stTokenVector);
	}
	ConsoleLog("Property: {0}\n", c_pszFileName);
	return true;
}

bool CProperty::ReadFromXML(const char* c_pszCRC)
{
	m_dwCRC = atoi(c_pszCRC);
	return true;
}

void CProperty::Clear()
{
	m_stTokenMap.clear();
}
