//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <storm/StringUtil.hpp>
#include <storm/Tokenizer.hpp>
#include <storm/Util.hpp>

#include <vstl/iterator/back_insert_iterator.hpp>

#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/karma_real.hpp>

#include <boost/spirit/home/x3.hpp>

namespace storm
{

namespace
{

const uint8_t kFilenameLowercaseTable[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
	0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
	0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
	0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
	0x5b, 0x2f, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
	0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
	0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81,
	0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e,
	0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b,
	0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
	0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5,
	0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2,
	0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc,
	0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
	0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

BOOST_FORCEINLINE char LowercaseAscii(char ch)
{
	if (ch <= 'Z' && ch >= 'A')
		return ch - ('Z' - 'z');

	return ch;
}

BOOST_FORCEINLINE char UppercaseAscii(char ch)
{
	if (ch <= 'z' && ch >= 'a')
		return ch - ('z' - 'Z');

	return ch;
}

// http://www.azillionmonkeys.com/qed/asmexample.html
// http://src.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/lib/libc/sparcv9/gen/ascii_strcasecmp.s

BOOST_FORCEINLINE uint32_t Lowercase32BitAscii(uint32_t ch)
{
	const uint32_t b = ch & ~0x80808080;
	const uint32_t c = b + 0x3f3f3f3f;
	const uint32_t d = ~(b + 0x25252525);
	const uint32_t e = (c & d) & (0x80808080 & ~ch);
	return ch + (e >> 2);
}

BOOST_FORCEINLINE uint32_t Uppercase32BitAscii(uint32_t ch)
{
	const uint32_t b = 0x80808080 | ch;
	const uint32_t c = b - 0x61616161;
	const uint32_t d = ~(b - 0x7b7b7b7b);
	const uint32_t e = (c & d) & (~ch & 0x80808080);
	return ch - (e >> 2);
}

union CharToUint32
{
	char ch[4];
	uint32_t u;
};

template <class Str>
bool EscapeArgumentAux(Str& out, const StringRef& input)
{
	const bool hasQuotationMark = input.find('"') != StringRef::npos;
	const bool hasSpace = input.find_first_of("\t ") != StringRef::npos;

	// We can't escape quotation marks if we're already escaping spaces
	if (hasQuotationMark && hasSpace)
		return false;

	if (hasSpace) {
		out.append("\"");
		out.append(input);
		out.append("\"");
	} else {
		out.append(input);
	}

	return true;
}

template <typename Integral>
typename std::enable_if<
	std::is_integral<Integral>::value &&
	!std::is_same<Integral, bool>::value &&
	std::is_unsigned<Integral>::value, Integral>::type
 ParseNumberAux(const StringRef& str, Integral& val)
{
	const auto intParser = bspirit::x3::uint_;

	auto it = str.begin();
	auto end = str.end();

	return phrase_parse(it, end, intParser, bspirit::x3::space, val);
}

template <typename Integral>
typename std::enable_if<
	std::is_integral<Integral>::value &&
	!std::is_same<Integral, bool>::value &&
	std::is_signed<Integral>::value, Integral>::type ParseNumberAux(const StringRef& str, Integral& val)
{
	const auto intParser = bspirit::x3::int_;

	auto it = str.begin();
	auto end = str.end();

	return phrase_parse(it, end, intParser, bspirit::x3::space, val);
}

bool ParseNumberAux(const StringRef& str, int64_t& val)
{
	const auto intParser = bspirit::x3::int64;

	auto it = str.begin();
	auto end = str.end();

	return phrase_parse(it, end, intParser, bspirit::x3::space, val);
}

bool ParseNumberAux(const StringRef& str, uint64_t& val)
{
	const auto uintParser = bspirit::x3::uint64;

	auto it = str.begin();
	auto end = str.end();

	return phrase_parse(it, end, uintParser, bspirit::x3::space, val);
}


template <typename T>
typename std::enable_if<
	std::is_floating_point<T>::value, bool
>::type ParseNumberAux(const StringRef& str, T& val)
{
	const auto floatParse = bspirit::x3::float_;

	auto it = str.begin();
	auto end = str.end();

	return phrase_parse(it, end, floatParse, bspirit::x3::space, val);
}



template <typename T>
typename std::enable_if<
	std::is_integral<T>::value &&
	std::is_unsigned<T>::value, uint32_t
>::type FormatNumberAux(char* buf, T val)
{
	auto* out = buf;
	bspirit::karma::uint_generator<T, 10> fm;
	if (bspirit::karma::generate(out, fm, val))
		return out - buf;

	return 0;
}
uint64_t FormatNumberAux(char* buf, uint64_t val)
{
	auto* out = buf;
	bspirit::karma::uint_generator<uint64_t, 10> fm;
	if (bspirit::karma::generate(out, fm, val))
		return out - buf;

	return 0;
}

int64_t FormatNumberAux(char* buf, int64_t val)
{
	auto* out = buf;
	bspirit::karma::int_generator<int64_t, 10> fm;
	if (bspirit::karma::generate(out, fm, val))
		return out - buf;

	return 0;
}

template <typename T>
typename std::enable_if<
		std::is_integral<T>::value &&
		std::is_signed<T>::value, int32_t
>::type FormatNumberAux(char* buf, T val)
{
	auto* out = buf;
	bspirit::karma::int_generator<T, 10> fm;
	if (bspirit::karma::generate(out, fm, val))
		return out - buf;

	return 0;
}

template <typename T>
typename std::enable_if<
	std::is_floating_point<T>::value, uint32_t
>::type FormatNumberAux(char* buf, T val)
{
	auto* out = buf;
	bspirit::karma::real_generator<T> fm;
	if (bspirit::karma::generate(out, fm, val))
		return out - buf;

	return 0;
}

}

ArgumentExtractor::ArgumentExtractor(const StringRef& input)
	: m_input(input)
	, m_begin(input.begin())
	, m_end(input.begin())
{
	// ctor
}

void DoBoostSpiritX3(std::string_view str)
{

}

bool ArgumentExtractor::Next()
{
	if (m_end != m_input.end()) {
		auto begin =  m_end != m_input.begin() ? m_end + 1 : m_input.begin();
		auto end = m_input.end();

		while (begin != end && IsSpace(*begin))
			++begin;

		if (begin != end) {
			if (*begin == '\"') {
				end = std::find(++begin, end, '"');
			} else {
				auto found = begin;
				while (found != end && !IsSpace(*found))
					++found;

				end = found;
			}
		}

		m_begin = begin;
		m_end = end;

		//
		// It is possible that the last character in our string is
		// a quotation mark, which needs to be ignored. Thus we need a
		// second string-end check here.
		//

		return begin != m_input.end();
	}

	m_begin = m_end;
	return false;
}

bool EscapeArgument(String& out, const StringRef& input)
{ return EscapeArgumentAux(out, input); }


bool CopyStringSafe(char* dst, const StringRef& src, uint32_t size)
{
	if (size != 0) {
		auto cnt = std::min<uint32_t>(src.length(), size - 1);
		std::memcpy(dst, src.data(), cnt);
		dst[cnt] = '\0';
		return cnt != size - 1;
	} else {
		dst[0] = '\0';
		return true;
	}
}

//uint32_t ReplaceAll(String& str,
  //                  const StringRef& from,
 //                   const StringRef& to)
//{
//	uint32_t count = 0;

//	auto it = str.find(from.data(), from.length());

//	for (vstd::size_type pos = 0; it != str.end(); ++count) {
//		pos = it - str.begin() + to.length();
//		str.replace(it, it + from.length(), to);

//		it = str.find(str.begin() + pos, str.end(),
//		          from.data(), from.length());
//	}

//	return count;
//}

bool MatchesWildcard(const char* wild, const char* string)
{
	// Written by Jack Handy - jakkhandy@hotmail.com
	const char* cp = nullptr;
	const char* mp = nullptr;

	while (*string && (*wild != '*')) {
		if ((*wild != *string) && (*wild != '?'))
			return false;

		++wild;
		++string;
	}

	while (*string) {
		if (*wild == '*') {
			if (!*++wild)
				return true;

			mp = wild;
			cp = string + 1;
		} else if ((*wild == *string) || (*wild == '?')) {
			++wild;
			++string;
		} else {
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
		++wild;

	return !*wild;
}

void TrimAndAssign(StringRef& s)
{
	typedef vstd::char_traits<char> Traits;

	static const char whitespace[] = { ' ', '\t', '\r', '\n', 0 };

	auto begin = s.begin();
	auto end = s.end();

	for ( ; begin != end; ++begin)
		if (!Traits::find(whitespace, STORM_ARRAYSIZE(whitespace), *begin))
			break;

	for ( ; begin != end; --end)
		if (!Traits::find(whitespace, STORM_ARRAYSIZE(whitespace), *(end - 1)))
			break;

	s = make_string_view(begin, end);
}

void TrimAndAssign(String& s)
{
	typedef vstd::char_traits<char> Traits;

	static const char whitespace[] = { ' ', '\t', '\r', '\n', 0 };

	auto begin = s.begin();
	auto end = s.end();

	for ( ; begin != end; ++begin)
		if (!Traits::find(whitespace, STORM_ARRAYSIZE(whitespace), *begin))
			break;

	s.erase(s.begin(), begin);

	begin = s.begin();
	end = s.end();

	for ( ; begin != end; --end)
		if (!Traits::find(whitespace, STORM_ARRAYSIZE(whitespace), *(end - 1)))
			break;

	s.erase(end, s.end());
}

void CopyAndLowercaseAsciiString(char* dst, const char* src,
                                 vstd::size_type length)
{
	for ( ; length >= 4; length -= 4) {
		const CharToUint32* in = (const CharToUint32*)(src);
		CharToUint32* out = (CharToUint32*)(dst);

		out->u = Lowercase32BitAscii(in->u);

		src += 4; dst += 4;
	}

	switch (length) {
		case 3: *dst++ = LowercaseAscii(*src++);
		case 2: *dst++ = LowercaseAscii(*src++);
		case 1: *dst++ = LowercaseAscii(*src++);
		case 0: *dst = '\0';
	}
}

void CopyAndUppercaseAsciiString(char* dst, const char* src,
                                 vstd::size_type length)
{
	for ( ; length >= 4; length -= 4) {
		const CharToUint32* in = (const CharToUint32*)(src);
		CharToUint32* out = (CharToUint32*)(dst);

		out->u = Uppercase32BitAscii(in->u);

		src += 4; dst += 4;
	}

	switch (length) {
		case 3: *dst++ = UppercaseAscii(*src++);
		case 2: *dst++ = UppercaseAscii(*src++);
		case 1: *dst++ = UppercaseAscii(*src++);
		case 0: *dst = '\0';
	}
}

void CopyAndLowercaseFilename(char* dst, const char* src, uint32_t length)
{
	const uint8_t* s = reinterpret_cast<const uint8_t*>(src);
	const uint32_t leftover = length % 4;
	uint32_t i = 0;

	for (const uint32_t words = length - leftover; i < words; i += 4) {
		const uint8_t c1 = s[i], c2 = s[i + 1],
		              c3 = s[i + 2], c4 = s[i + 3];

		dst[0] = kFilenameLowercaseTable[c1];
		dst[1] = kFilenameLowercaseTable[c2];
		dst[2] = kFilenameLowercaseTable[c3];
		dst[3] = kFilenameLowercaseTable[c4];

		dst += 4;
	}

	switch (leftover) {
		case 3: *dst++ = kFilenameLowercaseTable[s[i++]];
		case 2: *dst++ = kFilenameLowercaseTable[s[i++]];
		case 1: *dst++ = kFilenameLowercaseTable[s[i]];
		case 0: *dst = '\0';
	}
}

bool EqualsIgnoreCase(const char* s1, uint32_t len1, const char* s2, uint32_t len2)
{
	if (len1 != len2)
		return false;

	for ( ; len1 >= 4; len1 -= 4) {
		const CharToUint32* c1 = (const CharToUint32*)(s1);
		const CharToUint32* c2 = (const CharToUint32*)(s2);

		if (Lowercase32BitAscii(c1->u) != Lowercase32BitAscii(c2->u))
			return false;

		s1 += 4; s2 += 4;
	}

	switch (len1) {
		case 3: if (LowercaseAscii(*s1++) != LowercaseAscii(*s2++)) return false;
		case 2: if (LowercaseAscii(*s1++) != LowercaseAscii(*s2++)) return false;
		case 1: if (LowercaseAscii(*s1++) != LowercaseAscii(*s2++)) return false;
		case 0: return true;
	}

	// We'll never reach this, but MSVC likes to complain...
	return false;
}

//
// Hex conversion functions
//

static const char kHexStringLookupTable[513] = {
	"000102030405060708090a0b0c0d0e0f"
	"101112131415161718191a1b1c1d1e1f"
	"202122232425262728292a2b2c2d2e2f"
	"303132333435363738393a3b3c3d3e3f"
	"404142434445464748494a4b4c4d4e4f"
	"505152535455565758595a5b5c5d5e5f"
	"606162636465666768696a6b6c6d6e6f"
	"707172737475767778797a7b7c7d7e7f"
	"808182838485868788898a8b8c8d8e8f"
	"909192939495969798999a9b9c9d9e9f"
	"a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
	"c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
	"d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
	"e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
	"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
};

void ConvertBinaryToHex(const uint8_t* src, vstd::size_type len, char* output)
{
	while (len--) {
		const uint32_t offset = static_cast<uint8_t>(*src++) * 2;
		std::memcpy(output, kHexStringLookupTable + offset, 2);
		output += 2;
	}

	*output = '\0';
}

static const uint8_t kHexDecLookupTable[] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // gap before first hex digit
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,2,3,4,5,6,7,8,9, // 0123456789
	0,0,0,0,0,0,0, // :;<=>?@ (gap)
	10,11,12,13,14,15, // ABCDEF
	0,0,0,0,0,0,0,0,0,0,0,0,0, // GHIJKLMNOPQRS (gap)
	0,0,0,0,0,0,0,0,0,0,0,0,0, // TUVWXYZ[/]^_` (gap)
	10,11,12,13,14,15 // abcdef
};

void ConvertHexToBinary(const char* src, uint32_t len, uint8_t* dst)
{
	STORM_ASSERT((len % 2) == 0,
	             "Hex-encoded message cannot be odd-sized");

	while (len) {
		const uint8_t ch1 = static_cast<uint8_t>(src[0]);
		const uint8_t ch2 = static_cast<uint8_t>(src[1]);

		*dst++ = (kHexDecLookupTable[ch1] << 4) | kHexDecLookupTable[ch2];

		len -= 2;
		src += 2;
	}
}

//
// Number conversion functions
//

#define STORM_MAKE_FUNC(type) \
	bool ParseNumber(const StringRef& str, type & value) \
	{ return ParseNumberAux(str, value); } \
	uint32_t FormatNumber(char* buf, type value) \
	{ return uint32_t(FormatNumberAux(buf, value)); } \
	bool FormatNumber(String& str, type value) \
	{ \
		char buffer[1024]; \
		auto r = FormatNumberAux(buffer, value); \
		if (r != 0) { str.append(buffer, size_t(r)); return true; } \
		return false; \
	}

STORM_MAKE_FUNC(signed char)
STORM_MAKE_FUNC(signed short)
STORM_MAKE_FUNC(signed int)
STORM_MAKE_FUNC(signed long)
STORM_MAKE_FUNC(signed long long)

STORM_MAKE_FUNC(unsigned char)
STORM_MAKE_FUNC(unsigned short)
STORM_MAKE_FUNC(unsigned int)
STORM_MAKE_FUNC(unsigned long)
STORM_MAKE_FUNC(unsigned long long)

STORM_MAKE_FUNC(float)
STORM_MAKE_FUNC(double)
STORM_MAKE_FUNC(long double)

#undef STORM_MAKE_FUNC

}
