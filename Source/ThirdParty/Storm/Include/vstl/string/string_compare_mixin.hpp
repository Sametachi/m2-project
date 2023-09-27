//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGCOMPAREMIXIN_HPP
#define VSTL_STRING_STRINGCOMPAREMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/algorithm/minmax.hpp>

#include <boost/mpl/and.hpp>
#include <boost/type_traits/is_same.hpp>

namespace vstd
{

template <class Impl>
class string_compare_mixin
{
	// empty
};

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            typename Impl::const_argument_type str) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	const auto n1 = impl.length();
	const auto n2 = str.length();

	const auto minLength = vstd::min(n1, n2);
	int res = Traits::compare(impl.data(), str.data(), minLength);

	if (res != 0)
		return res;

	if (n1 < n2)
		return -1;

	if (n1 > n2)
		return 1;

	return 0;
}

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            size_type pos1, size_type n1,
            typename Impl::const_argument_type str)
{
	return compare(self, pos1, n1, str.data(), str.length());
}

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            size_type pos1, size_type n1,
            typename Impl::const_argument_type str,
            size_type pos2, size_type n2)
{
	const auto realN2 = str.length();
	VSTD_CHECK_BOUNDS(realN2 >= pos2);

	const auto minLength = vstd::min(n2, realN2 - pos2);
	return compare(self, pos1, n1, str.data() + pos2, minLength);
}

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            const typename Impl::value_type* s) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	VSTD_CHECK_ARGUMENT(s);

	const auto& impl = static_cast<const Impl&>(self);
	const auto n1 = impl.length();
	const auto n2 = Traits::length(s);

	const auto minLength = vstd::min(n1, n2);
	int res = Traits::compare(impl.data(), s, minLength);

	if (res != 0)
		return res;

	if (n1 < n2)
		return -1;

	if (n1 > n2)
		return 1;

	return 0;
}

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            size_type pos1, size_type n1,
            typename Impl::const_pointer s)
{
	typedef typename Impl::traits_type Traits;
	return compare(self, pos1, n1, s, Traits::length(s));
}

template <class Impl>
int compare(const string_compare_mixin<Impl>& self,
            size_type pos1, size_type n1,
            typename Impl::const_pointer s,
            size_type n2)
{
	typedef typename Impl::traits_type Traits;

	VSTD_CHECK_ARGUMENT(s);
	VSTD_CHECK_BOUNDS(pos1 < n1);

	const auto& impl = static_cast<const Impl&>(self);
	const auto minLength = vstd::min(n1, n2);
	int res = Traits::compare(impl.data() + pos1, s, minLength);

	if (res != 0)
		return res;

	if (n1 < n2)
		return -1;

	if (n1 > n2)
		return 1;

	return 0;
}

template <class ImplA, class ImplB>
bool operator==(const string_compare_mixin<ImplA>& a1,
                const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	typedef typename ImplA::traits_type Traits;

	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	const size_type len = s1.length();
	if (len == s2.length())
		return Traits::compare(s1.data(), s2.data(), len) == 0;

	return false;
}

template <class Impl>
bool operator==(typename Impl::const_pointer s1,
                const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& s2 = static_cast<const Impl&>(a2);

	const size_type len = Traits::length(s1);
	if (len == s2.length())
		return Traits::compare(s1, s2.data(), len) == 0;

	return false;
}

template <class Impl>
bool operator==(const string_compare_mixin<Impl>& a1,
                typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& s1 = static_cast<const Impl&>(a1);
	const size_type len = Traits::length(s2);

	if (len == s1.length())
		return Traits::compare(s1.data(), s2, len) == 0;

	return false;
}

template <class ImplA, class ImplB>
bool operator!=(const string_compare_mixin<ImplA>& a1,
                const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	return !(a1 == a2);
}

template <class Impl>
bool operator!=(typename Impl::const_pointer s1,
                const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	return !(s1 == a2);
}

template <class Impl>
bool operator!=(const string_compare_mixin<Impl>& a1,
                typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	return !(a1 == s2);
}

template <class ImplA, class ImplB>
bool operator<(const string_compare_mixin<ImplA>& a1,
               const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	return compare(s1, s2) < 0;
}

template <class Impl>
bool operator<(typename Impl::const_pointer s1,
               const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	const auto& s2 = static_cast<const Impl&>(a2);
	return compare(s2, s1) > 0;
}

template <class Impl>
bool operator<(const string_compare_mixin<Impl>& a1,
               typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const Impl&>(a1);
	return compare(s1, s2) < 0;
}

template <class ImplA, class ImplB>
bool operator>(const string_compare_mixin<ImplA>& a1,
               const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	return compare(s1, s2) > 0;
}

template <class Impl>
bool operator>(typename Impl::const_pointer s1,
               const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	const auto& s2 = static_cast<const Impl&>(a2);
	return compare(s2, s1) < 0;
}

template <class Impl>
bool operator>(const string_compare_mixin<Impl>& a1,
               typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const Impl&>(a1);
	return compare(s1, s2) > 0;
}

template <class ImplA, class ImplB>
bool operator<=(const string_compare_mixin<ImplA>& a1,
                const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	return compare(s1, s2) <= 0;
}

template <class Impl>
bool operator<=(typename Impl::const_pointer s1,
                const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	const auto& s2 = static_cast<const Impl&>(a2);
	return compare(s2, s1) >= 0;
}

template <class Impl>
bool operator<=(const string_compare_mixin<Impl>& a1,
                typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const Impl&>(a1);
	return compare(s1, s2) <= 0;
}

template <class ImplA, class ImplB>
bool operator>=(const string_compare_mixin<ImplA>& a1,
                const string_compare_mixin<ImplB>& a2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const ImplA&>(a1);
	const auto& s2 = static_cast<const ImplB&>(a2);

	return compare(s1, s2) >= 0;
}

template <class Impl>
bool operator>=(typename Impl::const_pointer s1,
                const string_compare_mixin<Impl>& a2) BOOST_NOEXCEPT
{
	const auto& s2 = static_cast<const Impl&>(a2);
	return compare(s2, s1) <= 0;
}

template <class Impl>
bool operator>=(const string_compare_mixin<Impl>& a1,
                typename Impl::const_pointer s2) BOOST_NOEXCEPT
{
	const auto& s1 = static_cast<const Impl&>(a1);
	return compare(s1, s2) >= 0;
}

}

#endif
