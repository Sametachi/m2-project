//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_EMPTY_HPP
#define VSTL_STRING_EMPTY_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace vstd
{

namespace detail
{

// Union holding an empty representation of
// the most commonly used string data types;
union EmptyStringData
{
	char ch;
	wchar_t wch;

	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
};

extern EmptyStringData kEmptyStringData;

}

//
// Empty string constants
//

template <typename T>
const T* empty_string();

template <>
BOOST_FORCEINLINE const char* empty_string<char>()
{ return &detail::kEmptyStringData.ch; }

template <>
BOOST_FORCEINLINE const wchar_t* empty_string<wchar_t>()
{ return &detail::kEmptyStringData.wch; }

template <>
BOOST_FORCEINLINE const uint8_t* empty_string<uint8_t>()
{ return &detail::kEmptyStringData.u8; }

template <>
BOOST_FORCEINLINE const uint16_t* empty_string<uint16_t>()
{ return &detail::kEmptyStringData.u16; }

template <>
BOOST_FORCEINLINE const uint32_t* empty_string<uint32_t>()
{ return &detail::kEmptyStringData.u32; }

}

#endif
