//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define VSTD_BUILDING_STRING 1

#include <vstl/string.hpp>

namespace vstd
{

#ifndef BOOST_NO_CXX11_EXTERN_TEMPLATE
template class basic_string<wchar_t, char_traits<wchar_t>, allocator>;
#endif

}
