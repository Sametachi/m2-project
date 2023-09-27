//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_MEMORY_ALIGN_HPP
#define VSTL_MEMORY_ALIGN_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <cstddef>

namespace vstd
{

void* align(size_type alignment, size_type size,
            void*& ptr, size_type& space);

}

#endif
