//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/View.hpp>

namespace storm
{

uint8_t* View::Initialize(vstd::size_type size)
{
	auto p = static_cast<uint8_t*>(m_allocator.Allocate(
		size, 1, 0,
		AllocationFlags::kTemp
	));

	m_isDataOwner = true;
	m_data = p;
	return p;
}

}
