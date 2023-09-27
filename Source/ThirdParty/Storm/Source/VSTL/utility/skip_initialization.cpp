//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define VSTD_BUILDING_SKIPINITIALIZATION 1

#include <vstl/utility/skip_initialization.hpp>

namespace vstd
{

const skip_initialization_t skip_initialization = skip_initialization_t();

}
