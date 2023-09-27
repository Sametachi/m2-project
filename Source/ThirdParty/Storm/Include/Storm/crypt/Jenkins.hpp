//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_CRYPT_JENKINS_HPP
#define STORM_CRYPT_JENKINS_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace storm
{

// Keep the old names, but move them into a separate NS.
namespace jenkins
{

uint32_t hashword(const uint32_t *k, size_t length, uint32_t initval);
uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
void hashlittle2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);
uint32_t bjhash(const unsigned char *k, uint32_t length, uint32_t initval);

}

}

#endif
