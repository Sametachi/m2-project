#include "StdAfx.h"
#include <boost/locale/utf.hpp>

std::vector<std::string> SplitLines(std::string_view input, uint32_t limit)
{
	std::vector<std::string> lines;

	auto first = input.begin();
	auto last = input.end();
	auto line = first;

	for (uint32_t codePointCount = 0; first != last; ++codePointCount)
	{
		if (codePointCount == limit)
		{
			lines.emplace_back(line, first);

			line = first;
			codePointCount = 0;
		}

		boost::locale::utf::utf_traits<char>::decode(first, last);
	}

	if (line != first)
		lines.emplace_back(line, first);

	return lines;
}

static std::vector<std::string> grpTextGetLines(std::string szText, uint32_t limit)
{
	const auto lines = SplitLines(szText, limit);

	std::vector<std::string> vec;

	for (uint32_t i = 0, s = lines.size(); i < s; ++i)
	{
		vec.push_back(lines[i]);
	}

	return lines;
}

PYBIND11_EMBEDDED_MODULE(grpText, m)
{
	m.def("GetLines", grpTextGetLines);
}
