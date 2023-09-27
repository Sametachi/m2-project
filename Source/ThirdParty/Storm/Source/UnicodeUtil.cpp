//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/UnicodeUtil.hpp>

#include <boost/locale/utf.hpp>

namespace storm
{

typedef blocale::utf::utf_traits<wchar_t> Utf16;
typedef blocale::utf::utf_traits<char> Utf8;

vstd::size_type ConvertUtf16ToUtf8(const wchar_t* sourceFirst,
                                   const wchar_t* sourceLast,
                                   char* targetFirst,
                                   char* targetLast,
                                   boost::system::error_code& ec)
{
	char* targetBegin = targetFirst;
	while (sourceFirst != sourceLast) {
		auto c = Utf16::decode(sourceFirst, sourceLast);
		if (c == blocale::utf::illegal || c == blocale::utf::incomplete) {
			ec = make_error_code(boost::system::errc::illegal_byte_sequence);
			return 0;
		}

		if (targetLast - targetFirst < Utf8::width(c)) {
			ec = make_error_code(boost::system::errc::no_buffer_space);
			return 0;
		}

		targetFirst = Utf8::encode(c, targetFirst);
	}

	return targetFirst - targetBegin;
}

vstd::size_type ConvertUtf8ToUtf16(const char* sourceFirst,
                                   const char* sourceLast,
                                   wchar_t* targetFirst,
                                   wchar_t* targetLast,
                                   boost::system::error_code& ec)
{
	wchar_t* targetBegin = targetFirst;
	while (sourceFirst != sourceLast) {
		auto c = Utf8::decode(sourceFirst, sourceLast);
		if (c == blocale::utf::illegal || c == blocale::utf::incomplete) {
			ec = make_error_code(boost::system::errc::illegal_byte_sequence);
			return 0;
		}

		if (targetLast - targetFirst < Utf16::width(c)) {
			ec = make_error_code(boost::system::errc::no_buffer_space);
			return 0;
		}

		targetFirst = Utf16::encode(c, targetFirst);
	}

	return targetFirst - targetBegin;
}

}
