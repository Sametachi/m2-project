//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/Config.hpp>

namespace vstd
{

bool HandleOutOfBounds(const char* filename, int lineno,
					const char* functionName, const char* expr)
{
	storm::HandleAssertionFailure(filename, lineno,
								functionName, expr,
								"vstl: OutOfBounds");

	return true;
}

bool HandleInvalidArgument(const char* filename, int lineno,
						const char* functionName, const char* expr)
{
	storm::HandleAssertionFailure(filename, lineno,
								functionName, expr,
								"vstl: InvalidArgument");

	return true;
}

bool HandleAssertionFailure(const char* filename, int lineno,
							const char* functionName, const char* expr,
							const char* msg)
{
	storm::HandleAssertionFailure(filename, lineno, functionName,
								expr, msg);

	return true;
}

}
