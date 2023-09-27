#include "StdAfx.h"
#include "MapBase.h"

CMapBase::CMapBase()
{
	Clear();
}

CMapBase::~CMapBase()
{
	Clear();
}

void CMapBase::Clear()
{
	m_strName.clear();
	m_eType = MAPTYPE_INVALID;
	mc_pEnvironmentData = nullptr;
	Leave();
}

bool CMapBase::Enter()
{
	m_bReady = true;
	return true;
}

bool CMapBase::Leave()
{
	m_bReady = false;
	return true;
}

void CMapBase::SetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData)
{
	mc_pEnvironmentData = c_pEnvironmentData;
	OnSetEnvironmentDataPtr();
}

void CMapBase::ResetEnvironmentDataPtr(const TEnvironmentData* c_pEnvironmentData)
{
	mc_pEnvironmentData = c_pEnvironmentData;
	OnResetEnvironmentDataPtr();
}

void CMapBase::Render()
{
	if (IsReady())
		OnRender();
}

void CMapBase::RenderShadow()
{
	if (IsReady())
		OnRenderShadow();
}

void CMapBase::RenderTreeShadow()
{
	if (IsReady())
		OnRenderTreeShadow();
}

bool CMapBase::LoadProperty()
{
	std::string strFileName = GetName() + "\\MapProperty.txt";

	CTokenVectorMap stTokenVectorMap;

	if (!LoadMultipleTextData(strFileName.c_str(), stTokenVectorMap))
	{
		SysLog("CMapBase::LoadProperty(FileName={0}) - LoadMultipleTextData ERROR It is very likely that there are no files.", strFileName.c_str());
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("scripttype"))
	{
		SysLog("CMapBase::LoadProperty(FileName={0}) - FIND 'scripttype' - FAILED", strFileName.c_str());
		return false;
	}

	if (stTokenVectorMap.end() == stTokenVectorMap.find("maptype"))
	{
		SysLog("CMapBase::LoadProperty(FileName={0}) - FIND 'maptype' - FAILED", strFileName.c_str());
		return false;
	}

	if (stTokenVectorMap.end() != stTokenVectorMap.find("parentmapname"))
		m_strParentMapName = stTokenVectorMap["parentmapname"][0];

	const std::string& c_rstrType = stTokenVectorMap["scripttype"][0];
	const std::string& c_rstrMapType = stTokenVectorMap["maptype"][0];

	if ("MapProperty" != c_rstrType)
	{
		SysLog("CMapBase::LoadProperty(FileName={0}) - Resourse Type ERROR", strFileName.c_str());
		return false;
	}

	if ("Indoor" == c_rstrMapType)
		SetType(MAPTYPE_INDOOR);
	else if ("Outdoor" == c_rstrMapType)
		SetType(MAPTYPE_OUTDOOR);
	else if ("Invalid" == c_rstrMapType)
		SetType(MAPTYPE_OUTDOOR);

	return true;
}