#pragma once
/*
#include <VFE/Include/ResourceManager/ReferenceCounter.hpp>

class CResourceManager;

enum ResourceFlags
{
	// AbstractResource is added to the deletion queue. This flag is cleared, should the element be removed from the deletion queue.
	kResourceFlagDeletePending = (1 << 0),
	
	/// The resource is in use.
	kResourceFlagAlive = (1 << 1),
};

class AbstractResource : public RefCounted<AbstractResource>
{
	friend class CResourceManager;
	friend void DestroyReferenceCounted(AbstractResource *resource);

public:
	AbstractResource();
	virtual ~AbstractResource() = default;
	
	virtual void Clear() = 0;
	
	bool Reload();
	virtual bool Load(int32_t iSize, const void *c_pvBuf) = 0;
	
	uint32_t GetRawSize() const;
	
	const char *GetFileName() const;
	const std::string &GetFileNameString() const;

protected:
	bool HasFlags(uint32_t flags) const;
	void AddFlags(uint32_t flags);
	void RemoveFlags(uint32_t flags);
	
	std::string m_filename;
	uint32_t m_flags;
	
	uint32_t m_dataSize;
private:
	bool Create(std::string_view filename, const void *data, uint32_t dataSize);
};

__forceinline uint32_t AbstractResource::GetRawSize() const
{
	return m_dataSize;
}

__forceinline const std::string &AbstractResource::GetFileNameString() const
{
	return m_filename;
}

__forceinline const char *AbstractResource::GetFileName() const
{
	return m_filename.c_str();
}

__forceinline bool AbstractResource::HasFlags(uint32_t flags) const
{
	return !!(m_flags & flags);
}

__forceinline void AbstractResource::AddFlags(uint32_t flags)
{
	m_flags |= flags;
}

__forceinline void AbstractResource::RemoveFlags(uint32_t flags)
{
	m_flags &= ~flags;
}

template <class T>
class ConcreteResource : public AbstractResource
{
public:
	typedef T RealType;
	typedef boost::intrusive_ptr<T> Ptr;
};

typedef AbstractResource CResource;
*/