//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_HPP
#define VSTL_STRING_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator.hpp>
#include <vstl/string/char_traits.hpp>
#include <vstl/string/basic_string.hpp>

namespace vstd
{

#if !defined(BOOST_NO_CXX11_EXTERN_TEMPLATE) && !VSTD_BUILDING_STRING
extern template class basic_string<char, char_traits<char>, allocator>;
extern template class basic_string<wchar_t, char_traits<wchar_t>, allocator>;
#endif

typedef basic_string<char, char_traits<char>, allocator> string;
typedef basic_string<wchar_t, char_traits<wchar_t>, allocator> wstring;

}

#endif
