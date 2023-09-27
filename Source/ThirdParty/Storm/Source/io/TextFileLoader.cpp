//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/TextFileLoader.hpp>
#include <storm/io/File.hpp>
#include <storm/memory/Allocate.hpp>
#include <storm/String.hpp>

namespace storm
{

void TextFileLoader::Load(const StringRef& filename,
                          bsys::error_code& ec,
                          vstd::size_type alignment,
                          uint32_t flags)
{
	STORM_ASSERT(!m_isLoaded, "Load() can only be called on an empty TextFileLoader");

	File file;
	file.Open(filename, ec, AccessMode::kRead,
	          CreationDisposition::kOpenExisting,
	          ShareMode::kRead, UsageHint::kSequential);

	if (!ec) {
		const vstd::size_type size = vstd::size_type(file.GetSize());
		if (size == 0)
			return;

		char* ptr = AllocateArray<char>(m_allocator, size + 1, flags);
		if (file.Read(ptr, size, ec) == size && !ec) {
			ptr[size] = '\0';

			m_size = size;
			m_data = ptr;
		} else {
			m_allocator.Deallocate(ptr);
		}
	}
}

}
