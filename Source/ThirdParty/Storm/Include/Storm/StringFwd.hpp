//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_STRINGFWD_HPP
#define STORM_STRINGFWD_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/VstlAllocator.hpp>

#include <vstl/memory/allocator_fwd.hpp>
#include <vstl/string/char_traits_fwd.hpp>
#include <vstl/string/mutable_basic_string_ref_fwd.hpp>
#include <vstl/string/basic_string_buffer_fwd.hpp>

#include <string_view>

/// @file String.hpp
/// Defines Storm's string classes.

namespace storm
{

using StringRef = std::basic_string_view<char>;
using String = std::basic_string<char>;

/// Storm's mutable_string_ref class
typedef vstd::mutable_basic_string_ref<char, vstd::char_traits<char> > MutableStringRef;

/// Storm's string_buffer class
typedef vstd::basic_string_buffer<char, vstd::char_traits<char> > StringBuffer;


}

#endif
