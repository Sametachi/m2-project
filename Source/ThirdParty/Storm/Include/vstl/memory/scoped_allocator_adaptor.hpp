//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_SCOPEDALLOCATORADAPTOR_HPP
#define VSTL_MEMORY_SCOPEDALLOCATORADAPTOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator_base.hpp>
#include <vstl/utility/move.hpp>
#include <vstl/utility/forward.hpp>

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	#include <boost/preprocessor/arithmetic/add.hpp>
	#include <boost/preprocessor/arithmetic/sub.hpp>
	#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
	#include <boost/preprocessor/repetition/enum_trailing.hpp>
	#include <boost/preprocessor/repetition/enum_params.hpp>
	#include <boost/preprocessor/repetition/repeat.hpp>
	#include <boost/preprocessor/iteration/local.hpp>
	#include <boost/preprocessor/cat.hpp>
#endif

namespace vstd
{

#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES

template <class OuterAlloc, class ...InnerAllocs>
class scoped_allocator_adaptor : public OuterAlloc
{
	public:
		typedef OuterAlloc outer_allocator_type;
		typedef scoped_allocator_adaptor<InnerAllocs...> inner_allocator_type;

		scoped_allocator_adaptor()
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(OuterAlloc2&& outerAlloc, const InnerAllocs&...args)
			: outer_allocator_type(vstd::forward<OuterAlloc2>(outerAlloc))
			, m_inner(args...)
		{
			// ctor
		}

		scoped_allocator_adaptor(const scoped_allocator_adaptor& other)
			: outer_allocator_type(other.outer_allocator())
			, m_inner(other.inner_allocator())
		{
			// ctor
		}

		scoped_allocator_adaptor(scoped_allocator_adaptor&& other)
			: outer_allocator_type(vstd::move(other.outer_allocator()))
			, m_inner(vstd::move(other.inner_allocator()))
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(const scoped_allocator_adaptor<OuterAlloc2, InnerAllocs...>& other)
			: outer_allocator_type(other.outer_allocator())
			, m_inner(other.inner_allocator())
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(scoped_allocator_adaptor<OuterAlloc2, InnerAllocs...>&& other)
			: outer_allocator_type(other.outer_allocator())
			, m_inner(other.inner_allocator())
		{
			// ctor
		}

		scoped_allocator_adaptor& operator=(const scoped_allocator_adaptor& other)
		{
			outer_allocator_type::operator=(other.outer_allocator());
			m_inner = other.inner_allocator();
			return *this;
		}

		scoped_allocator_adaptor& operator=(scoped_allocator_adaptor&& other)
		{
			outer_allocator_type::operator=(vstd::move(other.outer_allocator()));
			m_inner = vstd::move(other.inner_allocator());
			return *this;
		}

		inner_allocator_type& inner_allocator()
		{ return m_inner; }

		inner_allocator_type const& inner_allocator() const
		{ return m_inner; }

		outer_allocator_type& outer_allocator()
		{ return static_cast<outer_allocator_type&>(*this); }

		const outer_allocator_type& outer_allocator() const
		{ return static_cast<const outer_allocator_type&>(*this); }

	private:
		inner_allocator_type m_inner;
};

#else

namespace detail
{

struct NullType;

}

// TODO: move to platform.hpp ?
#define VSTD_PP_IDENTITY(z, n, data) data

#define VSTD_SCOPED_ALLOC_DEFAULTED_INNER(z, n, data) \
	class BOOST_PP_CAT(A, n) = detail::NullType

template <class OuterAlloc
	BOOST_PP_ENUM_TRAILING(BOOST_PP_ADD(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, 1),
	                       VSTD_SCOPED_ALLOC_DEFAULTED_INNER, 0)
>
class scoped_allocator_adaptor;

#define VSTD_SCOPED_ALLOC_CONST_INNER_PARAM(z, n, data) \
	const BOOST_PP_CAT(A, n)& BOOST_PP_CAT(p, n) \

#define BOOST_PP_LOCAL_MACRO(n) \
template <typename OuterAlloc  \
	BOOST_PP_ENUM_TRAILING_PARAMS(n, class A) > \
class scoped_allocator_adaptor<OuterAlloc \
	BOOST_PP_ENUM_TRAILING_PARAMS(n, A) \
	BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, n), \
						VSTD_PP_IDENTITY, detail::NullType) \
> : public OuterAlloc \
{ \
	public: \
		typedef OuterAlloc outer_allocator_type; \
		typedef scoped_allocator_adaptor<BOOST_PP_ENUM_PARAMS(n, A), \
				detail::NullType \
				BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, n), \
						VSTD_PP_IDENTITY, detail::NullType)> \
			inner_allocator_type; \
\
		scoped_allocator_adaptor() \
		{ } \
\
		template <class OuterAlloc2> \
		scoped_allocator_adaptor(OuterAlloc2&& outerAlloc \
				BOOST_PP_ENUM_TRAILING(n, VSTD_SCOPED_ALLOC_CONST_INNER_PARAM, 0)) \
			: outer_allocator_type(vstd::forward<OuterAlloc2>(outerAlloc)) \
			, m_inner(BOOST_PP_ENUM_PARAMS(n, p)) \
		{ } \
\
		scoped_allocator_adaptor(const scoped_allocator_adaptor& other) \
			: outer_allocator_type(other.outer_allocator()) \
			, m_inner(other.inner_allocator()) \
		{} \
\
		scoped_allocator_adaptor(scoped_allocator_adaptor&& other) \
			: outer_allocator_type(vstd::move(other.outer_allocator())) \
			, m_inner(vstd::move(other.inner_allocator())) \
		{ } \
\
		template <class OuterAlloc2> \
		scoped_allocator_adaptor(const scoped_allocator_adaptor<OuterAlloc2 \
				BOOST_PP_ENUM_TRAILING_PARAMS(n, A) \
				BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, n), \
									VSTD_PP_IDENTITY, detail::NullType) \
				>& other) \
			: outer_allocator_type(other.outer_allocator()) \
			, m_inner(other.inner_allocator()) \
		{ } \
\
		template <class OuterAlloc2> \
		scoped_allocator_adaptor(scoped_allocator_adaptor<OuterAlloc2 \
				BOOST_PP_ENUM_TRAILING_PARAMS(n, A) \
				BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, n), \
									VSTD_PP_IDENTITY, detail::NullType) \
				>&& other) \
			: outer_allocator_type(vstd::move(other.outer_allocator())) \
			, m_inner(vstd::move(other.inner_allocator())) \
		{ } \
\
		scoped_allocator_adaptor& operator=(const scoped_allocator_adaptor& other) \
		{ \
			outer_allocator_type::operator=(other.outer_allocator()); \
			m_inner = other.inner_allocator(); \
			return *this; \
		} \
\
		scoped_allocator_adaptor& operator=(scoped_allocator_adaptor&& other) \
		{ \
			outer_allocator_type::operator=(vstd::move(other.outer_allocator())); \
			m_inner = vstd::move(other.inner_allocator()); \
			return *this; \
		} \
\
		inner_allocator_type& inner_allocator() \
		{ return m_inner; } \
\
		inner_allocator_type const& inner_allocator() const \
		{ return m_inner; } \
\
		outer_allocator_type& outer_allocator() \
		{ return static_cast<outer_allocator_type&>(*this); } \
\
		const outer_allocator_type& outer_allocator() const \
		{ return static_cast<const outer_allocator_type&>(*this); } \
\
	private: \
		inner_allocator_type m_inner; \
};

#define BOOST_PP_LOCAL_LIMITS (1, VSTD_MAX_EMULATED_VARIADIC_TEMPLATES)
#include BOOST_PP_LOCAL_ITERATE()

#undef VSTD_SCOPED_ALLOC_CONST_INNER_PARAM
#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS

#endif

template <class OuterAlloc>
class scoped_allocator_adaptor
	<OuterAlloc
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
						VSTD_PP_IDENTITY, detail::NullType)
#endif
	>
	: public OuterAlloc
{
	public:
		typedef OuterAlloc outer_allocator_type;
		typedef scoped_allocator_adaptor<OuterAlloc
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
		BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
							VSTD_PP_IDENTITY, detail::NullType)
#endif
		> inner_allocator_type;

		scoped_allocator_adaptor()
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(OuterAlloc2&& outerAlloc)
			: outer_allocator_type(vstd::forward<OuterAlloc2>(outerAlloc))
		{
			// ctor
		}

		scoped_allocator_adaptor(const scoped_allocator_adaptor& other)
			: outer_allocator_type(other.outer_allocator())
		{
			// ctor
		}

		scoped_allocator_adaptor(scoped_allocator_adaptor&& other)
			: outer_allocator_type(vstd::move(other.outer_allocator()))
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(const scoped_allocator_adaptor<OuterAlloc2
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
				BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
									VSTD_PP_IDENTITY, detail::NullType)
#endif
								>& other)
			: outer_allocator_type(other.outer_allocator())
		{
			// ctor
		}

		template <class OuterAlloc2>
		scoped_allocator_adaptor(scoped_allocator_adaptor<OuterAlloc2
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
				BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
									VSTD_PP_IDENTITY, detail::NullType)
#endif
								>&& other)
			: outer_allocator_type(other.outer_allocator())
		{
			// ctor
		}

		scoped_allocator_adaptor& operator=(const scoped_allocator_adaptor& other)
		{
			outer_allocator_type::operator=(other.outer_allocator());
			return *this;
		}

		scoped_allocator_adaptor& operator=(scoped_allocator_adaptor&& other)
		{
			outer_allocator_type::operator=(vstd::move(other.outer_allocator()));
			return *this;
		}

		inner_allocator_type& inner_allocator()
		{ return *this; }

		inner_allocator_type const& inner_allocator() const
		{ return *this; }

		outer_allocator_type& outer_allocator()
		{ return static_cast<outer_allocator_type&>(*this); }

		const outer_allocator_type& outer_allocator() const
		{ return static_cast<const outer_allocator_type&>(*this); }
};

//
// Comparision ops for innermost allocator
//

template <typename OuterAlloc1, typename OuterAlloc2>
inline bool operator==(const scoped_allocator_adaptor<OuterAlloc1
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
							VSTD_PP_IDENTITY, detail::NullType)
#endif
	>& a,
	const scoped_allocator_adaptor<OuterAlloc2
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
							VSTD_PP_IDENTITY, detail::NullType)
#endif
	>& b)
{
	return a.outer_allocator() == b.outer_allocator();
}

template <typename OuterAlloc1, typename OuterAlloc2>
inline bool operator!=(const scoped_allocator_adaptor<OuterAlloc1
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
							VSTD_PP_IDENTITY, detail::NullType)
#endif
	>& a,
   const scoped_allocator_adaptor<OuterAlloc2
#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	BOOST_PP_ENUM_TRAILING(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES,
							VSTD_PP_IDENTITY, detail::NullType)
#endif
	>& b)
{
	return a.outer_allocator() != b.outer_allocator();
}

//
// Comparision ops for all other allocators
//

template <typename OuterAlloc1, typename OuterAlloc2
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	, typename... InnerAllocs
#else
	BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, class A)
#endif
>
inline bool operator==(const scoped_allocator_adaptor<OuterAlloc1
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	,InnerAllocs...
#else
	BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, A)
#endif
	>& a,
	const scoped_allocator_adaptor<OuterAlloc2
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	,InnerAllocs...
#else
	BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, A)
#endif
	>& b)
{
	return a.outer_allocator() == b.outer_allocator() &&
	       a.inner_allocator() == b.inner_allocator();
}

template <typename OuterAlloc1, typename OuterAlloc2
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	, typename... InnerAllocs
#else
	BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, class A)
#endif
>
inline bool operator!=(const scoped_allocator_adaptor<OuterAlloc1
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	,InnerAllocs...
#else
	BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, A)
#endif
	>& a,
	const scoped_allocator_adaptor<OuterAlloc2
#ifndef BOOST_NO_CXX11_VARIADIC_TEMPLATES
	,InnerAllocs...
#else
	  BOOST_PP_ENUM_TRAILING_PARAMS(VSTD_MAX_EMULATED_VARIADIC_TEMPLATES, A)
#endif
	>& b)
{
	return a.outer_allocator() != b.outer_allocator() &&
	       a.inner_allocator() != b.inner_allocator();
}

}

#endif
