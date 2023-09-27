//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/StreamUtil.hpp>
#include <spdlog/spdlog.h>
#include <boost/system/error_code.hpp>

namespace storm
{

namespace detail
{

void HandleRwFailure(bool wasRead,
                     const void* buffer, uint32_t bytes, uint32_t ret,
                     const boost::system::error_code& ec)
{
	const char* functionName = wasRead ? "Read" : "Write";
	(void)functionName;

	SPDLOG_CRITICAL("{0} (buffer={1}, bytes={2}, ec={3}) -> {4} failed",
	          functionName, fmt::ptr(buffer), bytes, ec.message(), ret);

	if (wasRead)
		STORM_FAIL_FATALLY("Failed to read from a stream");
	else
		STORM_FAIL_FATALLY("Failed to write to a stream");
}

}

}
