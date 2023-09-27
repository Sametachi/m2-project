//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGINSERTMIXIN_HPP
#define VSTL_STRING_STRINGINSERTMIXIN_HPP

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
class string_insert_mixin
{
	// empty
};

//
// Insertion functions
//

template <class Impl>
BOOST_FORCEINLINE Impl& insert(string_insert_mixin<Impl>& self,
                               size_type pos, typename Impl::const_argument_type other)
{
	auto& impl = static_cast<Impl&>(self);
	impl.insert(pos, other.data(), other.length());
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& insert(string_insert_mixin<Impl>& self,
                               size_type pos, typename Impl::const_argument_type other,
                               size_type otherPos, size_type n)
{
	auto& impl = static_cast<Impl&>(self);
	impl.insert(pos, other.data() + otherPos, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& insert(string_insert_mixin<Impl>& self,
                               size_type pos,
                               typename Impl::const_pointer str)
{
	typedef typename Impl::traits_type Traits;

	auto& impl = static_cast<Impl&>(self);
	impl.insert(pos, str, Traits::length(str));
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& insert(string_insert_mixin<Impl>& self,
                               size_type pos,
                               typename Impl::const_pointer str,
                               size_type n)
{
	auto& impl = static_cast<Impl&>(self);
	impl.insert(pos, str, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE Impl& insert(string_insert_mixin<Impl>& self,
                               size_type pos, size_type n,
                               typename Impl::value_type ch)
{
	auto& impl = static_cast<Impl&>(self);
	impl.insert(pos, ch, n);
	return impl;
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos,
	typename Impl::value_type ch
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, ch, 1);

	return typename Impl::iterator(impl.begin() + diff);
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos, size_type n,
	typename Impl::value_type ch
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, ch, n);

	return typename Impl::iterator(impl.begin() + diff);
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos,
	typename Impl::const_pointer first,
	typename Impl::const_pointer last
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, first, static_cast<size_type>(last - first));

	return typename Impl::iterator(impl.begin() + diff);
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos,
	typename Impl::iterator first,
	typename Impl::iterator last
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, first, static_cast<size_type>(last - first));

	return typename Impl::iterator(impl.begin() + diff);
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos,
	typename Impl::const_iterator first,
	typename Impl::const_iterator last
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, first, static_cast<size_type>(last - first));

	return typename Impl::iterator(impl.begin() + diff);
}

template <class Impl, class InputIterator>
typename boost::enable_if<
	typename detail::IsOnlyStringInputIterator<Impl, InputIterator>,
	typename Impl::iterator
>::type insert(string_insert_mixin<Impl>& self,
               typename Impl::iterator pos,
               InputIterator first, InputIterator last)
{
	auto& impl = static_cast<Impl&>(self);

	const auto size = impl.size();
	const auto diff = pos - impl.begin();

	for ( ; first != last; ++first)
		impl.append(*first, 1);

	auto p = impl.begin();
	vstd::rotate(p + diff, p + size, p + impl.size());

	return p + diff;
}

template <class Impl, class ForwardIterator>
BOOST_FORCEINLINE typename boost::enable_if<
	typename detail::IsOnlyStringForwardIterator<Impl, ForwardIterator>,
	typename Impl::iterator
>::type insert(string_insert_mixin<Impl>& self,
               typename Impl::iterator pos,
               ForwardIterator first, ForwardIterator last)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, first, last);
	return impl.begin() + diff;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator insert(
	string_insert_mixin<Impl>& self,
	typename Impl::iterator pos,
	std::initializer_list<typename Impl::value_type> il
)
{
	auto& impl = static_cast<Impl&>(self);
	const auto diff = pos - impl.begin();

	impl.insert(diff, il.begin(), il.end());
	return typename Impl::iterator(impl.begin() + diff);
}

#endif

}

#endif
