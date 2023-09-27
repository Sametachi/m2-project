//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_STRINGUTIL_IMPL_HPP
#define STORM_STRINGUTIL_IMPL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/VstlAllocator.hpp>
#include <storm/String.hpp>
#include <boost/spirit/home/x3.hpp>

#include <cstdarg>


namespace storm
{
BOOST_FORCEINLINE StringRef ArgumentExtractor::GetCurrent() const
{ return make_string_view(m_begin, m_end); }

BOOST_FORCEINLINE StringRef ArgumentExtractor::GetRemaining() const
{
	if (m_end != m_input.end())
		return make_string_view(m_end + 1, m_input.end());

	return StringRef();
}

template <class Container>
void ExtractArguments(const StringRef& input, Container& container)
{

	namespace x3 = boost::spirit::x3;

	auto write = [&container](auto const& ctx)
    {
        container.push_back(storm::StringRef(&(x3::_attr(ctx))[0], x3::_attr(ctx).size()));
    };
    auto skip = x3::char_(" \n\t\r\f\"\'");
    x3::parse(
        input.begin(), input.end(),
        x3::raw[+~skip][write] % +skip
    );

}

BOOST_FORCEINLINE bool IsSpace(int8_t ch)
{
	// A very simple definition of whitespaces.
	return ch == ' ' || ch == '\t';
}

// Helpers for convenient usage

template <std::size_t N>
BOOST_FORCEINLINE bool CopyStringSafe(char (&dst) [N], const StringRef& src)
{ return CopyStringSafe(dst, src, N); }

BOOST_FORCEINLINE StringRef Trim(StringRef s)
{
	TrimAndAssign(s);
	return s;
}

BOOST_FORCEINLINE String Trim(String s)
{
	TrimAndAssign(s);
	return s;
}

BOOST_FORCEINLINE bool EqualsIgnoreCase(const char* first, const char* second)
{ return EqualsIgnoreCase(first, strlen(first), second, strlen(second)); }

BOOST_FORCEINLINE bool EqualsIgnoreCase(const char* first,
                                        uint32_t firstlen,
                                        const char* second)
{ return EqualsIgnoreCase(first, firstlen, second, strlen(second)); }

BOOST_FORCEINLINE bool EqualsIgnoreCase(const StringRef& first, const char* second)
{
	return EqualsIgnoreCase(first.data(), first.length(),
	                        second, strlen(second));
}

BOOST_FORCEINLINE bool EqualsIgnoreCase(const StringRef& first,
                                        const StringRef& second)
{
	return EqualsIgnoreCase(first.data(), first.length(),
	                        second.data(), second.length());
}

BOOST_FORCEINLINE bool EqualsIgnoreCase(const char* first, const StringRef& second)
{
	return EqualsIgnoreCase(first, strlen(first),
	                        second.data(), second.length());
}

}

#endif
