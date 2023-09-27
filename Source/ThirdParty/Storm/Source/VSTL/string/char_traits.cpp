//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vstl/string/char_traits.hpp>

namespace vstd
{

const wchar_t char_traits<wchar_t>::whitespaces[] = {
	L' ', L'\t', L'\r', L'\n'
};

const char char_traits<char>::whitespaces[] = {
	' ', '\t', '\r', '\n'
};

}
