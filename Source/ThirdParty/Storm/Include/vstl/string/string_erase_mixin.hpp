//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_STRINGERASEMIXIN_HPP
#define VSTL_STRING_STRINGERASEMIXIN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

template <class Impl>
class string_erase_mixin
{
	// empty
};

//
// Erasure functions
//

template <class Impl>
Impl& erase(string_erase_mixin<Impl>& self, size_type pos, size_type n)
{
	auto& impl = static_cast<Impl&>(self);
	if (pos) {
		if (n == Impl::npos)
			impl.erase(pos, impl.size() - pos);
		else
			impl.erase(pos, n);
	} else {
		if (n == Impl::npos)
			impl.clear();
		else
			impl.erase(0, n);
	}

	return impl;
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator
	erase(string_erase_mixin<Impl>& self,
		typename Impl::iterator pos)
{
	auto& impl = static_cast<Impl&>(self);
	return impl.erase(pos - impl.begin(), 1);
}

template <class Impl>
BOOST_FORCEINLINE typename Impl::iterator
	erase(string_erase_mixin<Impl>& self,
		typename Impl::iterator first,
		typename Impl::iterator last)
{
	auto& impl = static_cast<Impl&>(self);
	return impl.erase(first - impl.begin(), last - first);
}

template <class Impl>
BOOST_FORCEINLINE void pop_back(string_erase_mixin<Impl>& self,
                                size_type count = 1)
{
	static_cast<Impl&>(self).pop_back(count);
}

}

#endif
