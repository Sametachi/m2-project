//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_STRINGFLAGS_HPP
#define STORM_STRINGFLAGS_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/StringUtil.hpp>
#include <storm/String.hpp>
#include <storm/Tokenizer.hpp>

#include <vstl/algorithm/find_if.hpp>

/// @file StringFlags.hpp
/// Definess the ParseBitflagString() functions.

namespace storm
{

/// Parse a bitflag string
///
/// This function parses a bitflag string and returns an integer
/// containing all the bits, set in the range [first, last).
///
/// @param input The string to parse.
///
/// @return The final integer, having all the bits contained in the source
/// string set.
template <typename U, typename Function>
U ParseBitflagString(const String& input, Function func)
{
	U bits = 0;

	Tokenizer tok(input);
	for (auto pred = IsChar('|'); tok.Next(pred); ) {
		auto val = tok.GetCurrent();
		TrimAndAssign(val);

		if (!val.empty())
			bits |= func(val);
	}

	return bits;
}

/// String to value table.
///
/// A simple struct containing
/// the flag string and it's integer
/// representation.
template <typename U>
struct StringValueTable
{
	typedef U FlagType;

	const storm::StringRef str;
	U value;
};

/// Parse a bitflag string
///
/// This function parses a bitflag string and returns an integer
/// containing all the bits, set in the range [first, last).
///
/// @param input The string to parse.
///
/// @param out The final integer, having all the bits contained in the source
/// string set.
///
/// @param table A table containing all the possible bitflags.
///
/// @return A bool denoting whether all flags found could be resolved
/// to their integer representation.
template <typename U, size_t N>
bool ParseBitflagStringWithTable(const String& input,
                                 U& out,
                                 StringValueTable<U> const (&table) [N])
{
	U bits = 0;

	std::vector<std::string> tokens;
	storm::Tokenize(input, "|", tokens);

	for (auto & val : tokens) {
		TrimAndAssign(val);

		if (!val.empty()) {
			auto checker = [val] (const StringValueTable<U>& t) -> bool
			{ return val == t.str; };

			const auto t = vstd::find_if(table, table + N, checker);
			if (t != table + N)
				bits |= t->value;
		}
	}

	out = bits;
	return true;
}

/// Parse a string.
///
/// This function checks the given table for the input string
/// and returns the associated value to the caller.
///
/// @param s The string to parse.
///
/// @param out The output integer. This will be set to the @c value member
/// of the corresponding StringValueTable entry.
///
/// @param table The lookup table to search in.
///
/// @return A bool denoting whether the function was able to find the given
/// string in the table.
template <typename U, size_t N>
bool ParseStringWithTable(const StringRef& s,
                          U& out,
                          StringValueTable<U> const (&table) [N])
{
	auto checker = [s] (const StringValueTable<U>& t) -> bool
	{ return s == t.str; };

	const auto t = vstd::find_if(table, table + N, checker);
	if (t != table + N) {
		out = t->value;
		return true;
	} else {
		return false;
	}
}

/// Format a bitflag integer.
///
/// This function splits the given integer into bitflags
/// and formats the bitflag names into a bitflag string.
///
/// @param s The output string.
///
/// @param in The input integer value.
///
/// @param table The lookup table to search in.
///
/// @return A bool denoting whether all flags found could be resolved
/// to their string representation.
template <class Str, typename U, size_t N>
bool FormatBitflagWithTable(Str& s, U in,
                            StringValueTable<U> const (&table) [N])
{
	s.clear();

	bool isFirst = true;
	for (size_t i = 0; i < N && in != 0; ++i) {
		const auto& t = table[i];

		if (!!(in & t.value)) {
			if (!isFirst)
				s.append(" | ");
			else
				isFirst = false;

			s.append(t.str);
			in &= ~t.value;
		}
	}

	return in == 0;
}

/// Format an integer.
///
/// This function finds the string corresponding to the given integer.
///
/// @param s The output string.
///
/// @param in The input integer value.
///
/// @param table The lookup table to search in.
///
/// @return A bool denoting whether the function was able to find the given
/// integer in the table.
template <typename U, size_t N>
bool FormatValueWithTable(storm::StringRef& s, U in,
                          StringValueTable<U> const (&table) [N])
{
	for (size_t i = 0; i < N; ++i) {
		const auto& t = table[i];
		if (in == t.value) {
			s = t.str;
			return true;
		}
	}

	return false;
}

}

#endif
