//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_SSOALLOCATOR_HPP
#define VSTL_MEMORY_SSOALLOCATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator.hpp>

#include <cstddef>

#include <boost/type_traits/aligned_storage.hpp>
#include <boost/noncopyable.hpp>

namespace vstd
{

#ifndef VSTD_DEFAULT_SSO_ALLOC_NAME
	#define VSTD_DEFAULT_SSO_ALLOC_NAME "vstl-sso"
#endif

template <size_type ByteSize,
		size_type Alignment,
		class SuperAllocator = allocator>
class sso_allocator : public allocator, boost::noncopyable
{
	public:
		BOOST_FORCEINLINE sso_allocator(SuperAllocator& superAlloc,
				const char* name = VSTD_ALLOC_NAME(VSTD_DEFAULT_ALLOC_NAME))
			: allocator(name)
			, m_superAlloc(superAlloc)
			, m_inUse(false)
		{
			// ctor
		}

		virtual void* allocate(size_type bytes, const char* name,
							size_type alignment,
							uint32_t flags)
		{
			//
			// Currently we're not checking alignment for 0, as this is
			// only a placeholder for the default alignment, which is already
			// already passed as a template argument.
			//

			if (!m_inUse && bytes <= m_storage.size && Alignment / alignment) {
				m_inUse = true;
				return m_storage.address();
			}

			return m_superAlloc.allocate(bytes, name, alignment, flags);
		}

		virtual void deallocate(void* p)
		{
			if (p == m_storage.address()) {
				VSTD_ASSERT(m_inUse);
				m_inUse = false;
			} else {
				m_superAlloc.deallocate(p);
			}
		}

	protected:
		typedef typename boost::aligned_storage<ByteSize, Alignment>::type
				StorageType;

		StorageType m_storage;
		SuperAllocator& m_superAlloc;
		bool m_inUse;

#if VSTD_TRACK_MEMORY
		const char* m_name;
#endif

};

BOOST_FORCEINLINE bool operator==(const allocator&, const allocator&)
{ return true; }

BOOST_FORCEINLINE bool operator!=(const allocator&, const allocator&)
{ return false; }

}

#endif
