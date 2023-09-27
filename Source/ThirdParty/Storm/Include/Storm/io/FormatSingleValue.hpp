//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_FORMATSINGLEVALUE_HPP
#define STORM_IO_FORMATSINGLEVALUE_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/io/StreamUtil.hpp>
#include <storm/SystemError.hpp>
#include <storm/StringUtil.hpp>

#include <vstl/string/basic_string.hpp>
#include <vstl/string/basic_string_ref.hpp>

#include <boost/system/error_code.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/limits.hpp>

#include <cstring>

namespace storm
{

/// Tag for all types that don't have a specialization
/// of FormatTypeTag
struct DefaultFormatTypeTag {};

template <typename T, typename Enabled = void>
struct FormatTypeTag
{ typedef DefaultFormatTypeTag Type; };

template <class BasicOutputStream, class T>
void FormatSingleValue(BasicOutputStream& out, const T& value,
                       DefaultFormatTypeTag)
{
	const char* msg = "<Unknown type>";
	Write(out, msg, std::strlen(msg));
}

/// Tag for all integral types.
struct IntegralFormatTypeTag {};

template <typename T>
struct FormatTypeTag<T, typename boost::enable_if<boost::is_integral<T> >::type>
{ typedef IntegralFormatTypeTag Type; };

template <class BasicOutputStream, class T>
void FormatSingleValue(BasicOutputStream& out,
                       T value,
                       IntegralFormatTypeTag)
{
	char buffer[std::numeric_limits<T>::digits10 * 2 +
	            std::numeric_limits<T>::is_signed +
	            std::numeric_limits<T>::is_specialized];

	const auto len = FormatNumber(buffer, value);
	Write(out, buffer, len);
}

/// Tag for all real types.
struct RealFormatTypeTag {};

template <typename T>
struct FormatTypeTag<T, typename boost::enable_if<boost::is_floating_point<T> >::type>
{ typedef RealFormatTypeTag Type; };

template <class BasicOutputStream, class T>
void FormatSingleValue(BasicOutputStream& out,
                       T value,
                       RealFormatTypeTag)
{
	// TODO(tim): Accurate calculation...
	char buffer[512];

	const auto len = FormatNumber(buffer, value);
	Write(out, buffer, len);
}

struct BoolFormatTypeTag {};

template <>
struct FormatTypeTag<bool>
{ typedef BoolFormatTypeTag Type; };

template <class BasicOutputStream>
void FormatSingleValue(BasicOutputStream& out, bool value,
                       BoolFormatTypeTag)
{
	const char* v = &"false\0true"[6 * value];
	const vstd::size_type len = 5 - value;

	Write(out, v, len);
}

/// Tag for all pointer types.
struct PointerFormatTypeTag {};

template <typename T>
struct FormatTypeTag<T*>
{ typedef PointerFormatTypeTag Type; };

template <class BasicOutputStream>
void FormatSingleValue(BasicOutputStream& out, const void* value,
                       PointerFormatTypeTag)
{
	FormatSingleValue(out, reinterpret_cast<uintptr_t>(value),
                      IntegralFormatTypeTag());
}

/// Tag for all string types.
struct CStringFormatTypeTag {};

template <>
struct FormatTypeTag<const char*>
{ typedef CStringFormatTypeTag Type; };

template <>
struct FormatTypeTag<char*>
{ typedef CStringFormatTypeTag Type; };

template <class BasicOutputStream>
void FormatSingleValue(BasicOutputStream& out, const char* str,
                       CStringFormatTypeTag)
{
	Write(out, str, std::strlen(str));
}

/// Tag for all string types.
struct StringFormatTypeTag {};

template <class Traits>
struct FormatTypeTag<vstd::basic_string_ref<char, Traits> >
{ typedef StringFormatTypeTag Type; };

template <class Traits, class Allocator>
struct FormatTypeTag<vstd::basic_string<char, Traits, Allocator> >
{ typedef StringFormatTypeTag Type; };

template <class BasicOutputStream, class Traits>
void FormatSingleValue(BasicOutputStream& out,
                       const vstd::basic_string_ref<char, Traits>& str,
                       StringFormatTypeTag)
{
	Write(out, str.data(), str.size());
}

template <class BasicOutputStream, class Traits, class Allocator>
void FormatSingleValue(BasicOutputStream& out,
                       const vstd::basic_string<char, Traits, Allocator>& str,
                       StringFormatTypeTag)
{
	Write(out, str.data(), str.size());
}

/// Tag for all std::basic_string types.
struct StdStringFormatTypeTag {};

template <class Traits, class Allocator>
struct FormatTypeTag<std::basic_string<char, Traits, Allocator> >
{ typedef StdStringFormatTypeTag Type; };

template <class BasicOutputStream, class Traits, class Allocator>
void FormatSingleValue(BasicOutputStream& out,
                       const std::basic_string<char, Traits, Allocator>& str,
                       StdStringFormatTypeTag)
{
	Write(out, str.data(), str.size());
}

/// Tag for boost::system::error_code.
struct ErrorCodeFormatTypeTag {};

template <>
struct FormatTypeTag<bsys::error_code>
{ typedef ErrorCodeFormatTypeTag Type; };

template <class BasicOutputStream>
void FormatSingleValue(BasicOutputStream& out, const bsys::error_code& val,
                       ErrorCodeFormatTypeTag)
{
	const auto str = val.message();
	Write(out, str.data(), str.size());
}

/// Tag for SystemError.
struct SystemErrorFormatTypeTag {};

template <>
struct FormatTypeTag<SystemError>
{ typedef SystemErrorFormatTypeTag Type; };

template <class BasicOutputStream>
void FormatSingleValue(BasicOutputStream& out, const SystemError& val,
                       SystemErrorFormatTypeTag)
{
	const auto str = bsys::system_category().message(val.code);
	Write(out, str.data(), str.size());
}

}

#endif
