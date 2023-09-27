//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_TOKENIZER_HPP
#define STORM_TOKENIZER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/String.hpp>

/// @file Tokenizer.hpp
/// Defines the Tokenize() function.

namespace storm
{

struct IsChar
{
	IsChar(char ch);
	size_t operator()(const std::string& str) const;
	char ch;
};

struct IsString
{
	IsString(const std::string& s);
	size_t operator()(const std::string& str) const;
	std::string s;
};

struct IsAnyOf
{
	IsAnyOf(const std::string& s);
	size_t operator()(const std::string& str) const;
	std::string s;
};

/// Tokenize a string.
///
/// This class extracts tokens from a given string,
/// destroying the input string in the process.
///
/// @remarks This class holds a reference to the input string.
/// It doesn not make a copy. Therefore the input string needs to live at
/// least as long as the @c Tokenizer instance it is used with.
class Tokenizer
{
	public:
		/// Construct a new @c Tokenizer.
		///
		/// This constructor initializes the @c Tokenizer with the given
		/// input string.
		///
		/// @param input The string to tokenize.
		Tokenizer(const std::string& input);

		/// Get the current token as @c std::string
		///
		/// @return The current token as @c std::string
		std::string GetCurrent() const;

		/// Advance to the next token.
		///
		/// This function uses the given predicate to
		/// find the end of the next token.
		///
		/// @param p The predicate to use
		///
		/// @return A @c bool denoting whether the tokenizer
		/// has reached the end of the input string.
		/// If this function returns @c false, the current token is empty.
		template <class Predicate>
		bool Next(const Predicate& p);

	private:
		std::string m_input;
		std::string::const_iterator m_begin;
		std::string::const_iterator m_end;
		bool m_isFirstToken;
};

/// Tokenize a string.
///
/// This function uses the @c Tokenizer class with the given
/// predicate to tokenize the input string.
/// All individual tokens are appended to the output container.
///
/// @param input The input string to tokenize.
///
/// @param p The predicate to use.
///
/// @param container The container that receives the individual tokens.
template <class Container>
void Tokenize(const std::string& input,
              const std::string& p,
              Container& container);

}

#include <storm/Tokenizer-impl.hpp>

#endif
