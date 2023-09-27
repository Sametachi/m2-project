//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_STRINGUTIL_HPP
#define STORM_STRINGUTIL_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/String.hpp>

#include <cstdarg>

namespace storm
{

/// Extract arguments from a string.
///
/// This class extracts arguments from a given string,
/// destroying the input string in the process.
///
/// @par Definition of an 'argument'
/// An argument can either be a space-separated string
/// (if there are no space characters in the source string,
/// the whole string is considered one argument.),
/// or a quoted string (only " counts as a quote character).
///
/// @par Examples
/// These are simple examples showing the semantics of the ArgumentExtractor
/// class, not the actual use in code.
/// @code
/// ExtractArgument("Hello World", ...) -> "Hello"
/// ExtractArgument("Hello\tWorld", ...) -> "Hello"
/// ExtractArgument("Hello_World", ...) -> "Hello_World"
/// ExtractArgument("\"Hello World\"", ...) -> "Hello World"
/// ExtractArgument("\"Hello World", ...) -> "Hello World" (missing terminating quote ignored)
/// @endcode
///
/// @remarks This class holds a reference to the input string.
/// It doesn not make a copy. Therefore the input string needs to live at
/// least as long as the @c ArgumentExtractor instance it is used with.
class ArgumentExtractor
{
	public:
		/// Construct a new @c Tokenizer.
		///
		/// This constructor initializes the @c Tokenizer with the given
		/// input string.
		///
		/// @param input The string to tokenize.
		ArgumentExtractor(const StringRef& input);

		/// Get the current token as @c StringRef
		///
		/// @return The current token as @c StringRef
		StringRef GetCurrent() const;

		/// Get the unprocessed part of the input string.
		///
		/// @return The unprocessed part of the input string.
		StringRef GetRemaining() const;

		/// Advance to the next argument.
		///
		/// This function finds the next argument inside the input string.
		///
		/// @return A @c bool denoting whether the extractor
		/// has reached the end of the input string.
		/// If this function returns @c false, the current argument is empty.
		bool Next();

	private:
		StringRef m_input;

		StringRef::const_iterator m_begin;
		StringRef::const_iterator m_end;
};

/// Extract arguments from a string.
///
/// This function uses the @c ArgumentExtractor class
/// to extract arguments from the input string.
/// The individual args are appended to the given container.
///
/// @param input The input string to tokenize.
///
/// @param container The container that receives the individual args.
template <class Container>
void ExtractArguments(const StringRef& input, Container& container);

/// Check whether a character is a space character.
///
/// This function checks whether @c ch is a space.
///
/// @param ch The character.
///
/// @return @c true, if the character's a space,
/// @c false otherwise.
bool IsSpace(int8_t ch);

/// Escape a string as argument.
///
/// This function escapes the given input string like a typical
/// argument and appends the resulting data to @c out.
///
/// @param out A reference to the output string, receiving the escaped
/// input sequence.
///
/// @param input The input string to escape.
///
/// @return A @c bool denoting whether the escaping succeeded.
bool EscapeArgument(String& out, const StringRef& input);

/// Copy characters.
///
/// This function copies at most @c (size - 1) characters from
/// @c src to @c dst. It stops earlier if it encounters a NUL in
/// @c src.
///
/// @param dst The destination string.
///
/// @param src The source string.
///
/// @param size The maximum length of @c dst.
///
/// @return A bool denoting whether the string had to be truncated.
/// @c true means the string was successfully copied, while @c false
/// means the string got truncated.
bool CopyStringSafe(char* dst, const StringRef& src, uint32_t size);

/// Replace all occurences of a given string
///
/// This function replaces all occurences of @c from in @c str with
/// @c to.
///
/// @param str The string in which the replacement should take place.
///
/// @param from The string to replace.
///
/// @param to The replacement string.
///
/// @return The number of times @c str was replaced.
uint32_t ReplaceAll(String& str,
                    const StringRef& from,
                    const StringRef& to);

bool MatchesWildcard(const char* wild, const char* string);

void TrimAndAssign(StringRef& s);
void TrimAndAssign(String& s);

StringRef Trim(StringRef s);

void CopyAndLowercaseAsciiString(char* dst, const char* src, uint32_t length);
void CopyAndUppercaseAsciiString(char* dst, const char* src, uint32_t length);

void CopyAndLowercaseFilename(char* dst, const char* src, uint32_t length);

bool EqualsIgnoreCase(const char* s1, uint32_t len1,
                      const char* s2, uint32_t len2);

void ConvertBinaryToHex(const uint8_t* src, uint32_t len, char* output);
void ConvertHexToBinary(const char* src, uint32_t len, uint8_t* dst);

//
// Integer conversion functions
//

bool ParseNumber(const StringRef& str, signed char& value);
bool ParseNumber(const StringRef& str, unsigned char& value);
bool ParseNumber(const StringRef& str, signed short& value);
bool ParseNumber(const StringRef& str, unsigned short& value);
bool ParseNumber(const StringRef& str, signed int& value);
bool ParseNumber(const StringRef& str, unsigned int& value);
bool ParseNumber(const StringRef& str, signed long& value);
bool ParseNumber(const StringRef& str, unsigned long& value);
bool ParseNumber(const StringRef& str, signed long long& value);
bool ParseNumber(const StringRef& str, unsigned long long& value);
bool ParseNumber(const StringRef& str, float& value);
bool ParseNumber(const StringRef& str, double& value);
bool ParseNumber(const StringRef& str, long double& value);

uint32_t FormatNumber(char* buf, signed char value);
uint32_t FormatNumber(char* buf, unsigned char value);
uint32_t FormatNumber(char* buf, signed short value);
uint32_t FormatNumber(char* buf, unsigned short value);
uint32_t FormatNumber(char* buf, signed int value);
uint32_t FormatNumber(char* buf, unsigned int value);
uint32_t FormatNumber(char* buf, signed long value);
uint32_t FormatNumber(char* buf, unsigned long value);
uint32_t FormatNumber(char* buf, signed long long value);
uint32_t FormatNumber(char* buf, unsigned long long value);
uint32_t FormatNumber(char* buf, float value);
uint32_t FormatNumber(char* buf, double value);
uint32_t FormatNumber(char* buf, long double value);

bool FormatNumber(String& str, signed char value);
bool FormatNumber(String& str, unsigned char value);
bool FormatNumber(String& str, signed short value);
bool FormatNumber(String& str, unsigned short value);
bool FormatNumber(String& str, signed int value);
bool FormatNumber(String& str, unsigned int value);
bool FormatNumber(String& str, signed long value);
bool FormatNumber(String& str, unsigned long value);
bool FormatNumber(String& str, signed long long value);
bool FormatNumber(String& str, unsigned long long value);
bool FormatNumber(String& str, float value);
bool FormatNumber(String& str, double value);
bool FormatNumber(String& str, long double value);

}

#include <storm/StringUtil-impl.hpp>

#endif
