//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_WINDOWSPLATFORM_HPP
#define STORM_WINDOWSPLATFORM_HPP

#include <windows.h>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef Yield
#undef Yield
#endif

#ifdef CopyFile
#undef CopyFile
#endif

#ifdef MoveFile
#undef MoveFile
#endif

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#endif
