//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define STORM_BUILDING_STRING 1
#include <storm/String.hpp>

namespace vstd
{

template class basic_string<char, char_traits<char>, storm::VstlAllocator>;
template class basic_string_ref<char, char_traits<char> >;
template class mutable_basic_string_ref<char, char_traits<char> >;
template class basic_string_buffer<char, char_traits<char> >;

}
