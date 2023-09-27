#pragma once
#include <VFE/Include/VFE.hpp>
#include <Basic/Singleton.h>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <functional>

template <class T, class U> constexpr T inherit_cast(U&& u) noexcept
{
	return static_cast<T>(std::forward<U>(u));
}

class CResource;
class CResourceManager : public Singleton <CResourceManager>
{
	using TNewFunc = std::function<CResource* (const FilenameWrapper&)>;
	using NewFunc = std::pair <std::string, TNewFunc>;
	using TResourcePointerMap = std::unordered_map <uint64_t, CResource*>;
	using TResourceDeletingMap = std::unordered_map<CResource*, uint64_t>;

public:
	CResourceManager() = default;
	virtual ~CResourceManager();

	void LoadStaticCache(const char* c_szFileName);

	void DestroyDeletingList();
	void Destroy();

	CResource* InsertResourcePointer(uint64_t nameHash, CResource* pResource);
	CResource* FindResourcePointer(uint64_t nameHash);

	template <typename T>
	T* LoadResource(std::string_view stFileName);

	bool isResourcePointerData(uint64_t nameHash);

	void RegisterAllowedExtension(const char* c_szFileExt, TNewFunc pResNewFunc);

	static bool IsFileExist(const char* c_szFileName);

	void Update();
	void ReserveDeletingResource(CResource* pResource);

protected:
	void __DestroyDeletingResourceMap();
	void __DestroyResourceMap();
	void __DestroyCacheMap();

private:
	TResourcePointerMap		m_pCacheMap;
	TResourcePointerMap		m_pResMap;
	std::vector <NewFunc>	m_newFuncs;
	TResourceDeletingMap	m_ResourceDeletingMap;
};

//Global for resource loading, CResource* pResource = CResourceManager::GetInstance()->GetResourcePointer<CResource>(c_szFileName);
template <typename T>
T* CResourceManager::LoadResource(std::string_view stFileName)
{
	if (stFileName.empty())
	{
		SysLog("CResourceManager::GetResourcePointer: filename error!");
		return nullptr;
	}

	const auto filename = FilenameWrapper{ stFileName };
	const auto hash = filename.GetHash();

	auto pResource = FindResourcePointer(hash);
	if (pResource)
		return inherit_cast<T*>(pResource);

	TNewFunc newFunc = nullptr;

	stFileName = filename.GetPath();
	const auto fileExt = stFileName.rfind('.');
	if (fileExt != std::string::npos)
	{
		const auto it = std::find_if(m_newFuncs.begin(), m_newFuncs.end(),
			[&filename, fileExt, &stFileName]
		(const NewFunc& nf)
			{
				return 0 == stFileName.compare(fileExt + 1, std::string::npos, nf.first);
			}
		);

		if (it != m_newFuncs.end())
			newFunc = it->second;
	}
	else
	{
		SysLog("ResourceManager::GetResourcePointer: BROKEN FILE NAME: {0}", stFileName);
		return nullptr;
	}

	if (!newFunc)
	{
		SysLog("ResourceManager::GetResourcePointer: NOT SUPPORT FILE {0}", stFileName);
		return nullptr;
	}

	pResource = InsertResourcePointer(hash, newFunc(filename));
	return inherit_cast<T*>(pResource);
}
