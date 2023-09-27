#include <StdAfx.hpp>
/*
#include <VFE/Include/VFE.hpp>
#include <storm/io/View.hpp>
#include <Basic/Singleton.h>
#include <VFE/Include/ResourceManager/Resource.hpp>
#include <VFE/Include/ResourceManager/ResourceManager.hpp>

AbstractResource::AbstractResource(): m_flags(0), m_dataSize(0) {}

bool AbstractResource::Reload()
{
	Clear();
	
	WarnLog("AbstractResource::Reload {0}", GetFileName());
	const auto fp = CallFS().Open(GetFileName(), Buffered);
	
	if (!fp)
		return false;
	
	const auto size = fp->GetSize();
	
	storm::View data(storm::GetDefaultAllocator());
	fp->GetView(0, data, size);
	
	return Load(size, data.GetData());
}

bool AbstractResource::Create(std::string_view filename, const void *data, uint32_t dataSize)
{
	WarnLog("AbstractResource::Create({0})", filename);
	
	m_filename = filename;
	m_dataSize = dataSize;
	
	if (!Load(dataSize, data))
		return false;
	
	m_flags |= kResourceFlagAlive;
	return true;
}

void DestroyReferenceCounted(AbstractResource *resource)
{
	auto resMgr = CResourceManager::InstancePtr();
	if (resMgr)
		resMgr->DestroyResourceLazy(resource);
}
*/