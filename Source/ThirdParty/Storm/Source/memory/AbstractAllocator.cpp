//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/memory/AbstractAllocator.hpp>

namespace storm
{

namespace
{

AbstractAllocator* defaultAllocator = nullptr;

}

AbstractAllocator::AbstractAllocator()
	: Allocator(this)
{
	// ctor
}

AbstractAllocator::~AbstractAllocator()
{
	// dtor
}

AbstractAllocator* GetDefaultAllocator()
{
	STORM_ASSERT(defaultAllocator != nullptr, "No allocator set");
	return defaultAllocator;
}

void SetDefaultAllocator(AbstractAllocator* allocator)
{
	defaultAllocator = allocator;
}

}
