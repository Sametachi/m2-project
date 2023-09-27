//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGAPPENDMIXIN_HPP
#define VSTL_STRING_STRINGAPPENDMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/helpers.hpp>

#include <boost/utility/enable_if.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
	#include <initializer_list>
#endif

namespace vstd
{

template <class Impl>
class string_append_mixin
{
	// empty
};

//
// Appending operators
//

template <class Impl>
BOOST_FORCEINLINE Impl& operator+=(string_append_mixin<Impl>& self,
                                   typename Impl::const_argument_type other)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(other.data(), other.length());
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& operator+=(string_append_mixin<Impl>& self,
                                   typename Impl::const_pointer str)
{
	typedef typename Impl::traits_type Traits;

	auto& impl = static_cast<Impl&>(self);
	impl.append(str, Traits::length(str));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& operator+=(string_append_mixin<Impl>& self,
                                   typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(ch, 1);
	return impl;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <class Impl>
BOOST_FORCEINLINE Impl& operator+=(string_append_mixin<Impl>& self,
                                   std::initializer_list<typename Impl::value_type> il)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(il.begin(), il.end());
	return impl;
}

#endif

//
// Appending functions
//

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_argument_type other)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(other.data(), other.size());
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_argument_type other,
                               size_type pos, size_type n)
{
	VSTD_CHECK_BOUNDS(other.size() >= pos + n);

	auto& impl = static_cast<Impl&>(self);
	impl.append(other.data() + pos, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_pointer str)
{
	typedef typename Impl::traits_type Traits;

	auto& impl = static_cast<Impl&>(self);
	impl.append(str, Traits::length(str));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_pointer str,
                               size_type n)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(str, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(ch, 1);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               size_type n,
                               typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(ch, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_pointer first,
                               typename Impl::const_pointer last)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(first, static_cast<size_type>(last - first));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::iterator first,
                               typename Impl::iterator last)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(first.base(), static_cast<size_type>(last - first));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               typename Impl::const_iterator first,
                               typename Impl::const_iterator last)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(first.base(), static_cast<size_type>(last - first));
	return impl;
}

template <class Impl, class InputIterator>
typename boost::enable_if<
	typename detail::IsOnlyStringInputIterator<Impl, InputIterator>,
	Impl&
>::type append(string_append_mixin<Impl>& self,
               InputIterator first, InputIterator last)
{
	auto& impl = static_cast<Impl&>(self);
	for ( ; first != last; ++first)
		impl.append(*first, 1);

	return impl;
}

template <class Impl, class ForwardIterator>
BOOST_FORCEINLINE typename boost::enable_if<
	typename detail::IsOnlyStringForwardIterator<Impl, ForwardIterator>,
	Impl&
>::type append(string_append_mixin<Impl>& self,
               ForwardIterator first, ForwardIterator last)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(first, last);
	return impl;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <class Impl>
BOOST_FORCEINLINE Impl& append(string_append_mixin<Impl>& self,
                               std::initializer_list<typename Impl::value_type> il)
{
	auto& impl = static_cast<Impl&>(self);
	impl.append(il.begin(), il.size());
	return impl;
}

#endif

template <class Impl>
BOOST_FORCEINLINE void push_back(string_append_mixin<Impl>& self,
                                 typename Impl::value_type ch)
{
	static_cast<Impl&>(self).append(ch, 1);
}

}

#endif
