#pragma once
#include <boost/smart_ptr/intrusive_ptr.hpp>

template <class T>
class RefCounted
{
	friend void intrusive_ptr_add_ref(T* p)
	{
		++p->m_referenceCount;
	}
	
	friend void intrusive_ptr_release(T* p)
	{
		if (!(--p->m_referenceCount))
			DestroyReferenceCounted(p);
	}
	
public:
	RefCounted& operator=(const RefCounted& other) = delete;

protected:
	RefCounted(): m_referenceCount(0) {}
	
	~RefCounted()
	{
		//assert(!m_referenceCount, "dtor() called with ref count > 0");
	}
	
	RefCounted(const RefCounted& other): m_referenceCount(0) {}

private:
	uint32_t m_referenceCount;
};
