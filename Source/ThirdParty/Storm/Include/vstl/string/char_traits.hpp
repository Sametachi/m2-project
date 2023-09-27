//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_CHARTRAITS_HPP
#define VSTL_STRING_CHARTRAITS_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <boost/mpl/bool.hpp>

#include <cstring>
#include <cwchar>

namespace vstd
{

template <typename T>
struct has_specialized_char_traits : public boost::mpl::false_
{ };

template <typename C>
struct char_traits
{
	typedef C char_type;

	static const C whitespaces[];

	static void initialize(C* ch)
	{
		new (ch) C();
	}

	static C* initialize(C* str, size_type count)
	{
		while (count--)
			new (str++) C();

		return str;
	}

	static int compare(const C* first, const C* second, size_type count)
	{
		for ( ; 0 < count; --count, ++first, ++second)
			if (*first != *second)
				return *first - *second;

		return 0;
	}

	static size_type length(const C* str)
	{
		size_type i = 0;
		while (str[i++] != C())
			;

		return i;
	}

	static C* copy(C* dest, const C* second, size_type count)
	{
		for ( ; count--; ++dest, ++second)
			*dest = *second;

		return dest;
	}

	static const C* find(const C* str, size_type count, const C& ch)
	{
		for ( ; count--; ++str)
			if (*str == ch)
				return str;

		return 0;
	}

	static const C* rfind(const C* str, size_type count, const C& ch)
	{
		str += count;

		for (--str; count--; --str)
			if (*str == ch)
				return str;

		return 0;
	}

	static C* move(C* first, const C* second, size_type count)
	{
		if (second < first && first < second + count) {
			first += count, second += count;
			for (C* i = first; count--; )
				*--i = *--second;
		} else {
			for (; count--; ++first, ++second)
				*first = *second;
		}

		return first;
	}

	static C* assign(C* str, size_type count, C ch)
	{
		while (count--)
			*str++ = ch;

		return str;
	}
};

template <typename C>
/*static*/ const C char_traits<C>::whitespaces[] = {
	C(' '), C('\t'), C('\r'), C('\n')
};

template <>
struct char_traits<wchar_t>
{
	typedef wchar_t char_type;

	static const wchar_t whitespaces[];

	static void initialize(wchar_t* ch)
	{
		*ch = wchar_t();
	}

	static wchar_t* initialize(wchar_t* str, size_type count)
	{
		return std::wmemset(str, wchar_t(), count) + count;
	}

	static int compare(const wchar_t* first, const wchar_t* second, size_type count)
	{ return std::wmemcmp(first, second, count); }

	static size_type length(const wchar_t* str)
	{ return std::wcslen(str); }

	static wchar_t* copy(wchar_t* first, const wchar_t* second, size_type count)
	{ return std::wmemcpy(first, second, count) + count; }

	static const wchar_t* find(const wchar_t* str, size_type count, wchar_t ch)
	{ return std::wmemchr(str, ch, count); }

	static const wchar_t* rfind(const wchar_t* str, size_type count, wchar_t ch)
	{
		str += count;

		for (--str; count--; --str)
			if (*str == ch)
				return str;

		return 0;
	}

	static wchar_t* move(wchar_t* first, const wchar_t* second, size_type count)
	{ return std::wmemmove(first, second, count) + count; }

	static wchar_t* assign(wchar_t *str, size_type count, wchar_t ch)
	{ return std::wmemset(str, ch, count) + count; }
};

template <>
struct has_specialized_char_traits<wchar_t> : public boost::mpl::true_
{ };

template <>
struct char_traits<char>
{
	typedef char char_type;

	static const char whitespaces[];

	static void initialize(char* ch)
	{
		*ch = char();
	}

	static char* initialize(char* str, size_type count)
	{
		return static_cast<char*>(std::memset(str, char(), count)) + count;
	}

	static int compare(const char* first, const char* second, size_type count)
	{ return std::memcmp(first, second, count); }

	static size_type length(const char* str)
	{ return std::strlen(str); }

	static char* copy(char* first, const char* second, size_type count)
	{ return static_cast<char*>(std::memcpy(first, second, count)) + count; }

	static const char* find(const char* str, size_type count, char ch)
	{ return static_cast<const char*>(std::memchr(str, ch, count)); }

	static const char* rfind(const char* str, size_type count, char ch)
	{
		str += count;

		for (--str; count--; --str)
			if (*str == ch)
				return str;

		return 0;
	}

	static char* move(char* first, const char* second, size_type count)
	{ return static_cast<char*>(std::memmove(first, second, count)) + count; }

	static char* assign(char* str, size_type count, char ch)
	{ return static_cast<char*>(std::memset(str, ch, count)) + count; }
};

template <>
struct has_specialized_char_traits<char> : public boost::mpl::true_
{ };

}

#endif
