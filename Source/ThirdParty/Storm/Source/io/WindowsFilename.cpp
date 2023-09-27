//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/WindowsFilename.hpp>
#include <storm/UnicodeUtil.hpp>

#include <boost/system/error_code.hpp>

namespace storm
{

WindowsFilename::WindowsFilename(const StringRef& filename,
                                 bsys::error_code& ec)
{
	m_length = ConvertUtf8ToUtf16(filename.data(),
	                              filename.data() + filename.size(),
	                              m_wideFilename,
	                              m_wideFilename + kMaxPathLength,
	                              ec);

	if (!ec)
		m_wideFilename[m_length] = L'\0';
}

}
