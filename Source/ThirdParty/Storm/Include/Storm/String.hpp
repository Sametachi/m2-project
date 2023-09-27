//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_STRING_HPP
#define STORM_STRING_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/VstlAllocator.hpp>

#include <vstl/string/char_traits.hpp>
#include <vstl/string/basic_string.hpp>
#include <vstl/string/basic_string_ref.hpp>
#include <vstl/string/mutable_basic_string_ref.hpp>
#include <vstl/string/basic_string_buffer.hpp>

// todo:  change to std::basic_string_view<charT> in C++17
template <typename charT> using basic_string_view_type = std::basic_string_view<char>;    

// Creates a string view from a pair of iterators
//  http://stackoverflow.com/q/33750600/882436
template <typename _It>
inline constexpr auto make_string_view( _It begin, _It end )
{
    using result_type = basic_string_view_type<typename std::iterator_traits<_It>::value_type>;

	if(begin > end)
		end = begin;

    return result_type{
        ( begin != end ) ? &*begin : nullptr
        ,  (typename result_type::size_type)
        std::max(
            std::distance(begin, end)
            , (typename result_type::difference_type)0
        )
     };
}   // make_string_view

/// @file String.hpp
/// Defines Storm's string classes.

#if !defined(BOOST_NO_CXX11_EXTERN_TEMPLATE) && !defined(STORM_BUILDING_STRING)

namespace vstd
{
extern template class mutable_basic_string_ref<char, char_traits<char> >;
extern template class basic_string_buffer<char, char_traits<char> >;

}

#endif

namespace storm
{

using StringRef = std::basic_string_view<char>;
using String = std::basic_string<char>;

typedef vstd::mutable_basic_string_ref<char, vstd::char_traits<char> > MutableStringRef;
typedef vstd::basic_string_buffer<char, vstd::char_traits<char> > StringBuffer;

}

#endif
