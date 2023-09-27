#include <StdAfx.hpp>
/*

#include <VFE/Include/VFE.hpp>
#include <chrono>
#include <VFE/Include/ResourceManager/ReferenceCounter.hpp>
#include <VFE/Include/ResourceManager/Resource.hpp>
#include <VFE/Include/ResourceManager/ResourceManager.hpp>

CResourceManager::~CResourceManager()
{
	// Resources itself might hold references to other resources. We need to make sure that we processed all of them.
	
	for (uint32_t count; (count = PruneDeletionQueue(true));)
	{
		WarnLog("Destroyed {0} deleted resources in this pass", count);
	}
	
	// If we have no leaks, we can exit right away
	if (m_resources.empty())
		return;
	
	WarnLog("Leaking {0} resources!", m_resources.size());
	
	for (const auto &p : m_resources)
	{
		WarnLog("{0:p}: {1}", static_cast<void *>(p.second), p.second->GetFileName());
	
		p.second->Clear();
	}
}

void CResourceManager::ReloadAll()
{
	for (const auto &p : m_resources)
		p.second->Reload();
}

void CResourceManager::Update()
{
	const auto count = PruneDeletionQueue();
	if (count != 0)
		WarnLog("Destroyed {0} deleted resources", count);
}

void CResourceManager::DumpStatistics()
{
	uint32_t totalSize = 0;
	for (const auto &p : m_resources)
		totalSize += p.second->GetRawSize();
	
	SysLog("Total resource consumption: {0} MiB", totalSize / (1024.0f * 1024.0f));
}

AbstractResource *CResourceManager::GetResource(std::string_view filename)
{
	const auto it = m_resources.find(filename, boost::hash<std::string_view>(), std::equal_to<>());
	if (it != m_resources.end())
	{
		auto r = it->second;
	
		// Mark the resource as alive so we don't accidentally delete it if it is in the deletion queue.
		r->AddFlags(kResourceFlagAlive);
		return r;
	}
	
	return nullptr;
}

void CResourceManager::DestroyResourceLazy(AbstractResource *r)
{
	const std::chrono::minutes kDeleteTimeout(1);
	
	DeletedResource d;
	d.ptr = r;
	d.deleteTime = std::chrono::steady_clock::now() + kDeleteTimeout;
	
	auto cmp = [](const DeletedResource &a, const DeletedResource &b) 
	{ 
		return a.deleteTime < b.deleteTime; 
	};
	
	// We need to ensure that the resource has only one entry in the deletion queue. Otherwise we might end up deleting it twice+ or forget one entry when reviving it.
	if (!r->HasFlags(kResourceFlagDeletePending))
	{
		auto hint = std::lower_bound(m_deletionQueue.begin(), m_deletionQueue.end(), d, cmp);
	
		m_deletionQueue.emplace(hint, d);
		r->AddFlags(kResourceFlagDeletePending);
	}
	
	r->RemoveFlags(kResourceFlagAlive);
}

uint32_t CResourceManager::PruneDeletionQueue(bool ignoreTime)
{
	if (m_deletionQueue.empty())
		return 0;
	
	auto now = std::chrono::steady_clock::now();
	
	std::vector<AbstractResource *> marked;
	
	auto it = std::find_if(m_deletionQueue.begin(), m_deletionQueue.end(), [&](const DeletedResource &d) -> bool 
	{
		if (!ignoreTime && now < d.deleteTime)
			return true; // End the search...
	
		d.ptr->RemoveFlags(kResourceFlagDeletePending);
	
		// If the resource got revived, just log it and do nothing.
		if (d.ptr->HasFlags(kResourceFlagAlive))
		{
			WarnLog("Resource {0} was revived", d.ptr->GetFileName());
			return false;
		}
	
		marked.emplace_back(d.ptr);
		return false;
	});
	
	const uint32_t count = it - m_deletionQueue.begin();
	m_deletionQueue.erase(m_deletionQueue.begin(), it);
	
	for (const auto &r : marked)
	{
		m_resources.erase(r->GetFileName());
	
		// We shouldn't need to call Clear()
		r->Clear();
		delete r;
	}
	
	return count;
}
*/