//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_FUNCTIONAL_HASH_HPP
#define VSTL_FUNCTIONAL_HASH_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/utility/enable_if.hpp>

namespace vstd
{

namespace detail
{

size_type GetMurmurHash3(const void* data, size_type size);

template <typename T, class Enabler = void>
struct scalar_hash
{
	size_type operator()(const T& v) const
	{ return GetMurmurHash3(&v, sizeof(T)); }
};

template <typename T>
struct scalar_hash<T, boost::enable_if_c<
	sizeof(T) == sizeof(size_type)
> >
{
	size_type operator()(T v) const
	{ return static_cast<size_type>(v); }
};

}

template <typename T>
struct hash;

template <>
struct hash<bool> : detail::scalar_hash<bool>
{ };

template <>
struct hash<char> : detail::scalar_hash<char>
{ };

template <>
struct hash<signed char> : detail::scalar_hash<signed char>
{ };

template <>
struct hash<unsigned char> : detail::scalar_hash<unsigned char>
{ };

template <>
struct hash<wchar_t> : detail::scalar_hash<wchar_t>
{ };

template <>
struct hash<short> : detail::scalar_hash<short>
{ };

template <>
struct hash<unsigned short> : detail::scalar_hash<unsigned short>
{ };

template <>
struct hash<int> : detail::scalar_hash<int>
{ };

template <>
struct hash<unsigned int> : detail::scalar_hash<unsigned int>
{ };

template <>
struct hash<long> : detail::scalar_hash<long>
{ };

template <>
struct hash<long long> : detail::scalar_hash<long long>
{ };

template <>
struct hash<unsigned long> : detail::scalar_hash<unsigned long>
{ };

template <>
struct hash<unsigned long long> : detail::scalar_hash<unsigned long long>
{ };

template <>
struct hash<float> : detail::scalar_hash<float>
{
	size_type operator()(float f) const
	{
		if (f == 0.0f)
			return 0;

		return scalar_hash<float>::operator()(f);
	}
};

template <>
struct hash<double> : detail::scalar_hash<double>
{
	size_type operator()(double d) const
	{
		if (d == 0.0)
			return 0;

		return scalar_hash<double>::operator()(d);
	}
};

template <>
struct hash<long double> : detail::scalar_hash<long double>
{
	size_type operator()(long double d) const
	{
		if (d == 0.0)
			return 0;

		return scalar_hash<long double>::operator()(d);
	}
};

template<class T>
struct hash<T*> : detail::scalar_hash<T*>
{ };

}

#endif
