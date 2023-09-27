#include "StdAfx.h"
#include "MapTreeLoader.h"

#include "../Resource.h"
#include "MapObjectPage.h"

bool CTreeLoader::OnFolder(const char * c_szFilter, const char * c_szPathName, const char * c_szFileName)
{
	char szItemName[32+1];
	_snprintf(szItemName, 32, "[ %s ]", c_szFileName);
	m_hNextItem = m_pTreeControler->InsertItem(szItemName, 0, 1, m_hParentItem, TVI_SORT);

	// Insert Path Map
	std::string strFullPath;
	strFullPath  = c_szPathName;
	strFullPath += c_szFileName;
	StringPath(strFullPath);
	m_pTreeControler->RegisterPath(m_hNextItem, strFullPath.c_str());

	// Insert Path Map
	CTreeLoader TreeLoader;
	TreeLoader.SetPropertyManager(m_pPropertyManager);
	TreeLoader.SetParentItem(m_hNextItem);
	TreeLoader.SetTreeControler(m_pTreeControler);
	TreeLoader.Create(c_szFilter, strFullPath.c_str());
	return true;
}

bool CTreeLoader::OnFile(const char* c_szPathName, const char* c_szFileName)
{
	std::string strExtension;
	GetFileExtension(c_szFileName, strlen(c_szFileName), &strExtension);
	stl_lowers(strExtension);

	HTREEITEM hCurrentItem;

	if (0 == strExtension.compare("prt"))
		hCurrentItem = m_pTreeControler->InsertItem(c_szFileName, 2, 2, m_hParentItem, TVI_SORT);
	else if (0 == strExtension.compare("prb"))
		hCurrentItem = m_pTreeControler->InsertItem(c_szFileName, 3, 3, m_hParentItem, TVI_SORT);
	else if (0 == strExtension.compare("pre"))
		hCurrentItem = m_pTreeControler->InsertItem(c_szFileName, 4, 4, m_hParentItem, TVI_SORT);
	else if (0 == strExtension.compare("pra"))
		hCurrentItem = m_pTreeControler->InsertItem(c_szFileName, 5, 5, m_hParentItem, TVI_SORT);
	else if (0 == strExtension.compare("prd"))
		hCurrentItem = m_pTreeControler->InsertItem(c_szFileName, 6, 6, m_hParentItem, TVI_SORT);
	else
	{
		std::string stPath(c_szPathName);
		stPath += c_szFileName;
		StringPath(stPath);

		if (0 == stPath.compare("property/reserve"))
			CPropertyManager::Instance().LoadReservedCRC(stPath.c_str());

		return true;
	}

	std::string strNewPathName;
	StringPath(c_szPathName, strNewPathName);

	uint32_t dwCRC = CPropertyLoader::RegisterFile(strNewPathName.c_str(), c_szFileName);
	m_pTreeControler->SetItemData(hCurrentItem, dwCRC);

	return true;
}

void CTreeLoader::SetTreeControler(CPropertyTreeControler * pTreeCtrl)
{
	m_pTreeControler = pTreeCtrl;
}

void CTreeLoader::SetParentItem(HTREEITEM hItem)
{
	m_hParentItem = hItem;
}

CTreeLoader::CTreeLoader()
{
	m_pTreeControler = NULL;
	m_hParentItem = NULL;
	m_hNextItem = NULL;
}

CTreeLoader::~CTreeLoader()
{
}
