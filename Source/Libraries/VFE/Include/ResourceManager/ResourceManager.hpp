#pragma once
/*

#include <Basic/Singleton.h>
#include <VFE/Include/VFE.hpp>
#include <boost/unordered_map.hpp>
#include <storm/io/View.hpp>
#include <chrono>
#include <list>
#include <string>
#pragma warning (disable: 4996)
class AbstractResource;

class CResourceManager : public CSingleton<CResourceManager>
{
	friend void DestroyReferenceCounted(AbstractResource *resource);

public:
	CResourceManager() = default;
	~CResourceManager();
	
	template <class T>
	
	typename T::Ptr LoadResource(std::string_view filename);
	
	void ReloadAll();
	
	void Update();
	
	void DumpStatistics();

protected:

	typedef boost::unordered_map<std::string, AbstractResource *> ResourceMap;
	typedef std::list<AbstractResource *> ResourceQueue;
	
	struct DeletedResource
	{
		AbstractResource *ptr{};
		std::chrono::steady_clock::time_point deleteTime;
	};
	
	AbstractResource *GetResource(std::string_view filename);
	
	void DestroyResourceLazy(AbstractResource *r);
	uint32_t PruneDeletionQueue(bool ignoreTime = false);
	
	ResourceMap m_resources;
	std::vector<DeletedResource> m_deletionQueue;
};

template <class T>

typename T::Ptr CResourceManager::LoadResource(std::string_view filename)
{
	auto cached = GetResource(filename);
	if (cached)
		return static_cast<T *>(cached);
	
	const auto& vfe = CallFS().Open(filename, Buffered);
	if (!vfe) 
	{
		return nullptr;
	}
	
	const auto size = vfe->GetSize();
	
	storm::View data(storm::GetDefaultAllocator());
	vfe->GetView(0, data, size);
	
	typename T::Ptr r(new T());
	if (!r->Create(filename, data.GetData(), size)) 
	{
		return nullptr;
	}
	
	m_resources.emplace(filename, r.get());
	return r;
}
*/