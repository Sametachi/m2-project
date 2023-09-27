//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGADDMIXIN_HPP
#define VSTL_STRING_STRINGADDMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/utility/move.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
	#include <initializer_list>
#endif

namespace vstd
{

template <class Impl>
class string_add_mixin
{
	// empty
};

template <class ImplA, class ImplB>
ImplA operator+(const string_add_mixin<ImplA>& a1,
                const string_add_mixin<ImplB>& a2)
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	const size_type s1Length = s1.length(), s2Length = s2.length();

	ImplA ret(s1.data(), s1Length, s1Length + s2Length, s1.get_allocator());
	ret.append(s2.data(), s2Length);
	return vstd::move(ret);
}

template <class Impl>
Impl operator+(typename Impl::const_pointer s1,
               const string_add_mixin<Impl>& a2)
{
	typedef typename Impl::traits_type Traits;

	const auto& s2 = static_cast<const Impl&>(a2);
	const size_type s1Length = Traits::length(s1),
	                s2Length = s2.length();

	Impl ret(s1, s1Length, s1Length + s2Length, s2.get_allocator());
	ret.append(s2.data(), s2Length);
	return vstd::move(ret);
}

template <class Impl>
Impl operator+(typename Impl::value_type ch,
               const string_add_mixin<Impl>& a2)
{
	const auto& s = static_cast<const Impl&>(a2);
	const size_type len = s.length();

	Impl ret(1, ch, len + 1, s.get_allocator());
	ret.append(s.data(), len);
	return vstd::move(ret);
}

template <class Impl>
Impl operator+(const string_add_mixin<Impl>& a1,
               typename Impl::const_pointer s2)
{
	typedef typename Impl::traits_type Traits;

	const auto& s1 = static_cast<const Impl&>(a1);
	const size_type s2Length = Traits::length(s2),
	                s1Length = s1.length();

	Impl ret(s1.data(), s1Length, s1Length + s2Length, s1.get_allocator());
	ret.append(s2, s2Length);
	return vstd::move(ret);
}

template <class Impl>
Impl operator+(const string_add_mixin<Impl>& a1,
               typename Impl::value_type ch)
{
	const auto& s1 = static_cast<const Impl&>(a1);
	const size_type len = s1.length();

	Impl ret(s1.data(), len, len + 1, s1.get_allocator());
	ret.append(ch);
	return vstd::move(ret);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <class ImplA, class ImplB>
ImplA operator+(string_add_mixin<ImplA>&& a1,
                const string_add_mixin<ImplB>& a2)
{
	auto&& s1 = static_cast<ImplA&&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	s1.append(s2.data(), s2.length());
	return vstd::move(s1);
}

template <class ImplA, class ImplB>
ImplA operator+(const string_add_mixin<ImplA>& a1,
                string_add_mixin<ImplB>&& a2)
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	auto&& s2 = static_cast<ImplB&&>(a2);

	s2.insert(0, s1.data(), s1.length());
	return vstd::move(s2);
}

template <class Impl>
Impl operator+(typename Impl::const_pointer s1,
               string_add_mixin<Impl>&& a2)
{
	typedef typename Impl::traits_type Traits;

	auto&& s2 = static_cast<Impl&&>(a2);
	s2.insert(0, s1, Traits::length(s1));
	return vstd::move(s2);
}

template <class Impl>
Impl operator+(typename Impl::value_type ch,
               string_add_mixin<Impl>&& a2)
{
	auto&& s2 = static_cast<Impl&&>(a2);
	s2.insert(0, ch, 1);
	return vstd::move(s2);
}

template <class Impl>
Impl operator+(string_add_mixin<Impl>&& a1,
               typename Impl::const_pointer s2)
{
	typedef typename Impl::traits_type Traits;

	auto&& s1 = static_cast<Impl&&>(a1);
	s1.append(s2, Traits::length(s2));
	return vstd::move(s1);
}

template <class Impl>
Impl operator+(string_add_mixin<Impl>&& a1,
               typename Impl::value_type ch)
{
	auto&& s1 = static_cast<Impl&&>(a1);
	s1.append(ch, 1);
	return vstd::move(s1);
}

#endif

}

#endif
