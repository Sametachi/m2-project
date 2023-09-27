//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGREPLACEMIXIN_HPP
#define VSTL_STRING_STRINGREPLACEMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/helpers.hpp>
#include <vstl/algorithm/rotate.hpp>

#include <boost/utility/enable_if.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
	#include <initializer_list>
#endif

namespace vstd
{

template <class Impl>
class string_replace_mixin
{
	// empty
};

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                size_type position, size_type n,
                                typename Impl::const_argument_type str)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(position, n, str.data(), str.length());
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                size_type pos1, size_type n1,
                                typename Impl::const_argument_type str,
                                size_type pos2, size_type n2)
{
	VSTD_CHECK_BOUNDS(pos2 < str.length());

	auto& impl = static_cast<Impl&>(self);
	impl.replace(pos1, n1, str.data() + pos2, n2);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                size_type pos, size_type n1,
                                typename Impl::const_pointer s,
                                size_type n2)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(pos, n1, s, n2);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                size_type pos, size_type n1,
                                typename Impl::const_pointer s)
{
	typedef typename Impl::traits_type Traits;

	auto& impl = static_cast<Impl&>(self);
	impl.replace(pos, n1, s, Traits::length(s));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                size_type pos, size_type n1,
                                size_type n2,
                                typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(pos, n1, ch, n2);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::const_argument_type str)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(),
                 last - first,
                 str.data(), str.length());

	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::const_pointer s,
                                size_type n)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first, s, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::const_pointer s)
{
	typedef typename Impl::traits_type Traits;

	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first,
                 s, Traits::length(s));

	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                size_type n,
                                typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first, ch, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::const_pointer first2,
                                typename Impl::const_pointer last2)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first,
	             first2, static_cast<size_type>(last2 - first2));

	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::iterator first2,
                                typename Impl::iterator last2)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first,
                 first2.base(), static_cast<size_type>(last2 - first2));

	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                typename Impl::const_iterator first2,
                                typename Impl::const_iterator last2)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first,
                 first2.base(), static_cast<size_type>(last2 - first2));

	return impl;
}

template <class Impl, class InputIterator>
typename boost::enable_if<
	typename detail::IsOnlyStringInputIterator<Impl, InputIterator>,
	Impl&
>::type replace(string_replace_mixin<Impl>& self,
                typename Impl::iterator first,
                typename Impl::iterator last,
                InputIterator first2, InputIterator last2)
{
	auto& impl = static_cast<Impl&>(self);

	for ( ; first != last && first2 != last2; ++first, ++first2)
		*first = *first2;

	if (first2 != last2)
		insert(impl, first, first2, last2);
	else if (first != last)
		impl.erase(first - impl.begin(), last - first);

	return impl;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <class Impl>
BOOST_FORCEINLINE Impl& replace(string_replace_mixin<Impl>& self,
                                typename Impl::iterator first,
                                typename Impl::iterator last,
                                std::initializer_list<typename Impl::value_type> il)
{
	auto& impl = static_cast<Impl&>(self);
	impl.replace(first - impl.begin(), last - first, il.begin(), il.size());
	return impl;
}

#endif

}

#endif
