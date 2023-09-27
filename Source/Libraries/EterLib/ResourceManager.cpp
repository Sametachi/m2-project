#include "StdAfx.h"
#include "../eterBase/Timer.h"
#include "../eterBase/Stl.h"
#include "ResourceManager.h"
#include "GrpImage.h"
#include <io.h>

const int32_t c_Deleting_Wait_Time = 30000;
const int32_t c_DeletingCountPerFrame = 30;
const int32_t c_Reference_Decrease_Wait_Time = 30000;

void CResourceManager::LoadStaticCache(const char* c_szFileName)
{
	CResource* pkRes = LoadResource<CResource>(c_szFileName);
	if (!pkRes)
	{
		SysLog("CResourceManager::LoadStaticCache {0} - FAILED", c_szFileName);
		return;
	}

	uint32_t dwCacheKey = GetCRC32(c_szFileName, strlen(c_szFileName));
	auto f = m_pCacheMap.find(dwCacheKey);
	if (m_pCacheMap.end() != f)
		return;

	pkRes->AddReference();
	m_pCacheMap.emplace(dwCacheKey, pkRes);
}

void CResourceManager::__DestroyCacheMap()
{
	for (auto& cache : m_pCacheMap)
	{
		cache.second->Release();
	}

	m_pCacheMap.clear();
}

void CResourceManager::__DestroyDeletingResourceMap()
{
#ifdef USE_RESOURCE_DEBUG
	ConsoleLog("CResourceManager::__DestroyDeletingResourceMap {0}", m_ResourceDeletingMap.size());
#endif

	for (auto& i : m_ResourceDeletingMap)
		(i.first)->Clear();

	m_ResourceDeletingMap.clear();
}

void CResourceManager::__DestroyResourceMap()
{
#ifdef USE_RESOURCE_DEBUG
	ConsoleLog("CResourceManager::__DestroyResourceMap {0}", m_pResMap.size());
#endif

	for (auto& resMap : m_pResMap)
	{
		resMap.second->Release();
	}

	m_pResMap.clear();
}

void CResourceManager::DestroyDeletingList()
{
	CResource::SetDeleteImmediately(true);

	__DestroyCacheMap();
	__DestroyDeletingResourceMap();
}

void CResourceManager::Destroy()
{
	assert(m_ResourceDeletingMap.empty() && "CResourceManager::Destroy - YOU MUST CALL DestroyDeletingList");
	__DestroyResourceMap();
}

CResource* CResourceManager::InsertResourcePointer(uint64_t nameHash, CResource* pResource)
{
	auto itor = m_pResMap.find(nameHash);
	if (m_pResMap.end() != itor)
	{
		const auto& stRefResourceName = pResource->GetFileNameString();

		SysLog("CResource::InsertResourcePointer: {0} is already registered\n", stRefResourceName.c_str());
		assert(!"CResource::InsertResourcePointer: Resource already resistered");

		delete pResource;
		return itor->second;
	}

	m_pResMap.emplace(nameHash, pResource);
	return pResource;
}

CResource* CResourceManager::FindResourcePointer(uint64_t nameHash)
{
	auto itor = m_pResMap.find(nameHash);
	if (m_pResMap.end() == itor)
		return nullptr;

	return itor->second;
}

bool CResourceManager::isResourcePointerData(uint64_t nameHash)
{
	auto itor = m_pResMap.find(nameHash);

	if (m_pResMap.end() == itor)
		return false;

	return (itor->second)->IsData();
}

void CResourceManager::RegisterAllowedExtension(const char* c_szFileExt, TNewFunc pNewFunc)
{
	m_newFuncs.emplace_back(c_szFileExt, pNewFunc);
}

bool CResourceManager::IsFileExist(const char* c_szFileName)
{
	return CallFS().Exists(c_szFileName);
}

void CResourceManager::Update()
{
	uint32_t CurrentTime = ELTimer_GetMSec();
	CResource* pResource;
	int32_t Count = 0;

	auto itor = m_ResourceDeletingMap.begin();

	while (itor != m_ResourceDeletingMap.end())
	{
		pResource = itor->first;

		if (CurrentTime >= itor->second)
		{
			if (pResource->canDestroy())
			{
#ifdef USE_RESOURCE_DEBUG
				ConsoleLog("Resource Clear {0}\n", pResource->GetFileNameString());
#endif
				pResource->Clear();
			}

			itor = m_ResourceDeletingMap.erase(itor);

			if (++Count >= c_DeletingCountPerFrame)
				break;
		}
		else
			++itor;
	}
}

void CResourceManager::ReserveDeletingResource(CResource* pResource)
{
	uint32_t dwCurrentTime = ELTimer_GetMSec();
	m_ResourceDeletingMap.emplace(pResource, dwCurrentTime + c_Deleting_Wait_Time);
}

CResourceManager::~CResourceManager()
{
	Destroy();
}
