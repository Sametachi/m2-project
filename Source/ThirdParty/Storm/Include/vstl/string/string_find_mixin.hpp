//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGFINDMIXIN_HPP
#define VSTL_STRING_STRINGFINDMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/find.hpp>
#include <vstl/string/rfind.hpp>
#include <vstl/string/find_first_of.hpp>
#include <vstl/string/find_last_of.hpp>
#include <vstl/string/find_first_not_of.hpp>
#include <vstl/string/find_last_not_of.hpp>
#include <vstl/algorithm/minmax.hpp>

namespace vstd
{

template <class Impl>
class string_find_mixin
{
	// empty
};

template <class Impl>
typename Impl::const_iterator find(const string_find_mixin<Impl>& self,
                                   typename Impl::const_argument_type str,
                                   size_type pos = 0) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find(impl.begin() + pos, impl.end(),
	                  str.data(), str.length());
}

template <class Impl>
typename Impl::const_iterator find(const string_find_mixin<Impl>& self,
                                   typename Impl::const_pointer s,
                                   size_type pos, size_type n) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find(impl.begin() + pos, impl.end(), s, n);
}

template <class Impl>
typename Impl::const_iterator find(const string_find_mixin<Impl>& self,
                                   typename Impl::const_pointer s,
                                   size_type pos = 0) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find(impl.begin() + pos, impl.end(), s, Traits::length(s));
}

template <class Impl>
typename Impl::const_iterator find(const string_find_mixin<Impl>& self,
                                   typename Impl::value_type c,
                                   size_type pos = 0) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find(impl.begin() + pos, impl.end(), c);
}

template <class Impl>
typename Impl::const_iterator rfind(const string_find_mixin<Impl>& self,
                                    const Impl& str,
                                    size_type pos = Impl::npos) BOOST_NOEXCEPT
{
	return vstd::rfind(self, str.data(), pos, str.length());
}

template <class Impl>
typename Impl::const_iterator rfind(const string_find_mixin<Impl>& self,
                                    typename Impl::const_pointer s,
                                    size_type pos,
                                    size_type n) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	const auto len = impl.length();

	if (n != 0) {
		if (len >= n) {
			auto first = impl.begin();
			auto last = first + vstd::min(len - n, pos);

			if (first != last) {
				++last;
				do {
					--last;
					if (*last == *s &&
						Traits::compare(last.base() + 1, s + 1, n - 1) == 0)
						return last;
				} while (last != first);
			}
		}

		return impl.end();
	} else {
		if (len)
			return impl.begin() + vstd::min(len, pos);

		return impl.end();
	}
}

template <class Impl>
typename Impl::const_iterator rfind(const string_find_mixin<Impl>& self,
                                    typename Impl::const_pointer s,
                                    size_type pos = Impl::npos) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;
	return vstd::rfind(self, s, pos, Traits::length(s));
}

template <class Impl>
typename Impl::const_iterator rfind(const string_find_mixin<Impl>& self,
                                    typename Impl::value_type c,
                                    size_type pos = Impl::npos) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	const auto len = impl.length();

	if (len) {
		const auto p = Traits::rfind(impl.begin().base(),
		                             vstd::min(len - 1, pos) + 1,
		                             c);

		if (p)
			return typename Impl::const_iterator(p);
	}

	return impl.end();
}

template <class Impl>
typename Impl::const_iterator find_first_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_argument_type str, size_type pos = 0
) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_of(impl.begin() + pos, impl.end(),
	                           str.data(), str.length());
}

template <class Impl>
typename Impl::const_iterator find_first_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos, size_type n
) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_of(impl.begin() + pos, impl.end(), s, n);
}

template <class Impl>
typename Impl::const_iterator find_first_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos = 0
) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_of(impl.begin() + pos, impl.end(),
	                           s, Traits::length(s));
}

template <class Impl>
typename Impl::const_iterator find_first_of(
	const string_find_mixin<Impl>& self,
	typename Impl::value_type c,
	size_type pos = 0
) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_of(impl.begin() + pos, impl.end(), c);
}

template <class Impl>
typename Impl::const_iterator find_last_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_argument_type str,
	size_type pos = Impl::npos
) BOOST_NOEXCEPT
{
	return vstd::find_last_of(self, str.data(), pos, str.length());
}

template <class Impl>
typename Impl::const_iterator find_last_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos, size_type n
) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	const auto len = impl.length();

	if (n != 0) {
		auto first = impl.begin();
		auto last = first + vstd::min(len - 1, pos);

		if (first != last) {
			++last;
			do {
				--last;
				if (Traits::find(s, n, *last))
					return last;
			} while (last != first);
		}

		return impl.end();
	} else {
		if (len)
			return impl.begin() + vstd::min(len, pos);

		return impl.end();
	}
}

template <class Impl>
typename Impl::const_iterator find_last_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos = Impl::npos
) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;
	return vstd::find_last_of(self, s, pos, Traits::length(s));
}

template <class Impl>
typename Impl::const_iterator find_first_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_argument_type str,
	size_type pos = 0
) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_not_of(impl.begin() + pos, impl.end(),
	                               str.data(), str.length());
}

template <class Impl>
typename Impl::const_iterator find_first_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos,
	size_type n
) BOOST_NOEXCEPT
{
	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_not_of(impl.begin() + pos, impl.end(), s, n);
}

template <class Impl>
typename Impl::const_iterator find_first_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos = 0
) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	VSTD_CHECK_BOUNDS(pos <= impl.length());

	return vstd::find_first_not_of(impl.begin() + pos, impl.end(),
	                               s, Traits::length(s));
}

template <class Impl>
typename Impl::const_iterator find_last_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_argument_type str,
	size_type pos = Impl::npos
) BOOST_NOEXCEPT
{
	return vstd::find_last_not_of(self, str.data(), pos, str.length());
}

template <class Impl>
typename Impl::const_iterator find_last_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos, size_type n
) BOOST_NOEXCEPT
{

	typedef typename Impl::traits_type Traits;

	const auto& impl = static_cast<const Impl&>(self);
	const auto len = impl.length();

	if (n != 0) {
		auto first = impl.begin();
		auto last = first + vstd::min(len - 1, pos);

		if (first != last) {
			++last;
			do {
				--last;
				if (!Traits::find(s, n, *last))
					return last;
			} while (last != first);
		}

		return impl.end();
	} else {
		if (len)
			return impl.begin() + vstd::min(len, pos);

		return impl.end();
	}
}

template <class Impl>
typename Impl::const_iterator find_last_not_of(
	const string_find_mixin<Impl>& self,
	typename Impl::const_pointer s,
	size_type pos = Impl::npos
) BOOST_NOEXCEPT
{
	typedef typename Impl::traits_type Traits;
	return vstd::find_last_not_of(self, s, pos, Traits::length(s));
}

}

#endif
