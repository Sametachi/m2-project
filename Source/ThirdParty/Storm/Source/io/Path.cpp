//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/io/Path.hpp>

namespace storm
{

typedef std::pair<StringRef, StringRef> StrPair;

std::pair<StringRef, StringRef> SplitPath(const StringRef& path)
{
	auto it = path.find_last_of("\\/");
	if (it != StringRef::npos) {
		return StrPair(
			make_string_view(path.begin(), path.begin() + it),
			make_string_view(path.begin() + it + 1, path.end())
		);
	}

	return StrPair(StringRef(), path);
}

std::pair<StringRef, StringRef> SplitExtension(const StringRef& path)
{
	auto it = path.rfind('.');
	if (it == StringRef::npos || it == StringRef::npos) {
		return StrPair(path, StringRef());
	} else {
		return StrPair(
			make_string_view(path.begin(), path.begin() + it),
			make_string_view(path.begin() + it, path.end())
		);
	}
}

}
