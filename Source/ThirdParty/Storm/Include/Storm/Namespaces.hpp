//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_NAMESPACES_HPP
#define STORM_NAMESPACES_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file Namespaces.hpp
/// Defines various namespace aliases for commonly used third-party namespaces.

namespace boost
{

namespace system { }
namespace spirit { }
namespace locale { }

}


namespace storm
{

namespace bsys = boost::system;
namespace bspirit = boost::spirit;
namespace blocale = boost::locale;

}

#endif
