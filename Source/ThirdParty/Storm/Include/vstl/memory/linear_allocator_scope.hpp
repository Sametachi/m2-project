//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_LINEARALLOCATORSCOPE_HPP
#define VSTL_MEMORY_LINEARALLOCATORSCOPE_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/linear_allocator.hpp>
#include <vstl/utility/forward.hpp>

#include <boost/type_traits/alignment_of.hpp>
#include <boost/static_assert.hpp>

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	#include <boost/preprocessor/repetition/enum_binary_params.hpp>
	#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
	#include <boost/preprocessor/repetition/repeat.hpp>
	#include <boost/preprocessor/iteration/local.hpp>
#endif

namespace vstd
{

#ifndef VSTD_DEFAULT_LINEAR_ALLOC_SCOPE_NAME
	#define VSTD_DEFAULT_LINEAR_ALLOC_SCOPE_NAME "vstl-linear-scope"
#endif

class linear_allocator_scope : public allocator_base, boost::noncopyable
{
	public:
		linear_allocator_scope(linear_allocator& alloc, const char* name =
					VSTD_ALLOC_NAME(VSTD_DEFAULT_LINEAR_ALLOC_SCOPE_NAME));

		linear_allocator_scope(const linear_allocator_scope& other);

		~linear_allocator_scope();

		void* allocate(size_type bytes, size_type alignment,
					size_type alignmentOffset = 0,
					uint32_t flags = 0);

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
		template <typename T, typename... Args>
		T* new_object(Args&&... args)
		{
			BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, "T must be a complete type");
			Finalizer* f = AllocateWithFinalizer(sizeof(T),
							boost::alignment_of<T>::value);

      		T* p = new (static_cast<void*>(&f[1]))
      				T(vstd::forward<Args>(args)...);

      		f->dtor = &DestructObject<T>;
      		return p;
		}
#else
		#define VSTD_FORMAT_ARGUMENT(z, n, unused) \
			BOOST_PP_COMMA_IF(n) vstd::forward<A ## n>(arg ## n)

		#define BOOST_PP_LOCAL_MACRO(n) \
			template <typename T \
				BOOST_PP_ENUM_TRAILING_PARAMS(n, typename A)> \
			T* new_object(BOOST_PP_ENUM_BINARY_PARAMS(n, A, &&arg)) \
			{ \
				BOOST_STATIC_ASSERT_MSG(sizeof(T) > 0, \
										"T must be a complete type"); \
				Finalizer* f = AllocateWithFinalizer(sizeof(T), \
							boost::alignment_of<T>::value); \
				T* p = new (static_cast<void*>(&f[1])) \
						T(BOOST_PP_REPEAT(n, VSTD_FORMAT_ARGUMENT, 0)); \
				f->dtor = &DestructObject<T>; \
				return p; \
			}

		#define BOOST_PP_LOCAL_LIMITS (0, VSTD_MAX_EMULATED_VARIADIC_TEMPLATES)
		#include BOOST_PP_LOCAL_ITERATE()

		#undef VSTD_FORMAT_ARGUMENT
		#undef BOOST_PP_LOCAL_MACRO
		#undef BOOST_PP_LOCAL_LIMITS
#endif

		BOOST_FORCEINLINE void deallocate(void* /*p*/)
		{ }

		BOOST_FORCEINLINE bool operator==(const linear_allocator_scope& other)
		{
			return m_alloc == other.m_alloc && m_rewindPoint == other.m_rewindPoint;
		}

		BOOST_FORCEINLINE bool operator!=(const linear_allocator_scope& other)
		{
			return m_alloc != other.m_alloc || m_rewindPoint != other.m_rewindPoint;
		}

	private:
		struct Finalizer
		{
			void (* dtor)(void* obj);
			Finalizer* next;
		};

		template <class T>
		static void DestructObject(void* obj)
		{
			static_cast<T*>(obj)->~T();
		}

        Finalizer* AllocateWithFinalizer(size_type bytes,
									size_type alignment);

		linear_allocator& m_alloc;
		uint8_t* m_rewindPoint;
		Finalizer* m_finalizerChain;
};

}

#endif
