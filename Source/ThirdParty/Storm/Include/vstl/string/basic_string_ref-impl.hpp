//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRINGREF_IMPL_HPP
#define VSTL_STRING_BASICSTRINGREF_IMPL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/empty.hpp>

namespace vstd
{

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref() BOOST_NOEXCEPT
{
	leak();
}

template <typename T, class Traits>
/*explicit*/ BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(skip_initialization_t) BOOST_NOEXCEPT
{
	// Don't do anything here!
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const basic_string_ref& other)
	: start(const_cast<pointer>(other.data()))
	, finish(const_cast<pointer>(other.data()) + other.length())
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const basic_string_ref& other,
		                 size_type pos,
		                 size_type n)
	: start(const_cast<pointer>(other.data()) + pos)
{
	const size_type len = other.length();
	VSTD_CHECK_BOUNDS(pos < len);

	finish = start + vstd::min(n, len - pos);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(basic_string_ref&& other)
	: start(const_cast<pointer>(other.data()))
	, finish(const_cast<pointer>(other.data()) + other.length())
{
	other.leak();
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const_pointer str)
	: start(const_cast<pointer>(str))
	, finish(const_cast<pointer>(str) + traits_type::length(str))
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(std::string str)
	: start(const_cast<pointer>(str.data()))
	, finish(const_cast<pointer>(str.data()) + str.length())
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(std::string_view str)
	: start(const_cast<pointer>(str.data()))
	, finish(const_cast<pointer>(str.data()) + str.size())
{
	// ctor
}


template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const_pointer str, size_type n)
	: start(const_cast<pointer>(str))
	, finish(const_cast<pointer>(str) + n)
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const_pointer first, const_pointer last)
	: start(const_cast<pointer>(first))
	, finish(const_cast<pointer>(last))
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>::
		basic_string_ref(const_iterator first, const_iterator last)
	: start(const_cast<pointer>(first.base()))
	, finish(const_cast<pointer>(last.base()))
{
	// ctor
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>&
	basic_string_ref<T, Traits>::operator=(const basic_string_ref& other)
{
	start = other.start;
	finish = other.finish;
	return *this;
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>&
	basic_string_ref<T, Traits>::operator=(basic_string_ref&& other)
{
	start = other.start;
	finish = other.finish;

	other.leak();
	return *this;
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_ref<T, Traits>&
	basic_string_ref<T, Traits>::operator=(const_pointer str)
{
	start = const_cast<pointer>(str);
	finish = const_cast<pointer>(str) + traits_type::length(str);
	return *this;
}

//
// Iterators
//

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_iterator
	basic_string_ref<T, Traits>::begin() const BOOST_NOEXCEPT
{ return const_iterator(start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_iterator
	basic_string_ref<T, Traits>::cbegin() const BOOST_NOEXCEPT
{ return const_iterator(start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_iterator
	basic_string_ref<T, Traits>::end() const BOOST_NOEXCEPT
{ return const_iterator(finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_iterator
	basic_string_ref<T, Traits>::cend() const BOOST_NOEXCEPT
{ return const_iterator(finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reverse_iterator
	basic_string_ref<T, Traits>::rbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reverse_iterator
	basic_string_ref<T, Traits>::crbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reverse_iterator
	basic_string_ref<T, Traits>::rend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reverse_iterator
	basic_string_ref<T, Traits>::crend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(start); }

//
// Misc
//

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_pointer
	basic_string_ref<T, Traits>::data() const BOOST_NOEXCEPT
{ return start; }

template <typename T, class Traits>
typename basic_string_ref<T, Traits>::const_reference
	basic_string_ref<T, Traits>::operator[](size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return start[pos];
}

template <typename T, class Traits>
typename basic_string_ref<T, Traits>::const_reference
	basic_string_ref<T, Traits>::at(size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return start[pos];
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reference
	basic_string_ref<T, Traits>::front() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(start != finish, "String is empty");
	return *start;
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_ref<T, Traits>::const_reference
	basic_string_ref<T, Traits>::back() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(start != finish, "String is empty");
	return finish[-1];
}

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_ref<T, Traits>::
	size() const BOOST_NOEXCEPT
{ return static_cast<size_type>(finish - start); }

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_ref<T, Traits>::
	length() const BOOST_NOEXCEPT
{ return static_cast<size_type>(finish - start); }

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_ref<T, Traits>::
	capacity() const BOOST_NOEXCEPT
{ return static_cast<size_type>(finish - start); }

template <typename T, class Traits>
BOOST_FORCEINLINE bool basic_string_ref<T, Traits>::
	empty() const BOOST_NOEXCEPT
{ return finish == start; }

//
// Size management operations (reserve, resize, shrink...)
//

template <typename T, class Traits>
void basic_string_ref<T, Traits>::clear() BOOST_NOEXCEPT
{
	leak();
}

template <typename T, class Traits>
void basic_string_ref<T, Traits>::leak() BOOST_NOEXCEPT
{
	pointer empty = const_cast<pointer>(empty_string<value_type>());
	start = finish = empty;
}

//
// Assignment functions
//

template <typename T, class Traits>
void basic_string_ref<T, Traits>::assign(const_pointer src, size_type len)
{
	VSTD_CHECK_ARGUMENT(src);

	start = const_cast<pointer>(src);
	finish = const_cast<pointer>(src) + len;
}

template <typename T, class Traits>
void basic_string_ref<T, Traits>::assign(basic_string_ref&& other)
{
	start = other.start;
	finish = other.finish;

	other.leak();
}

//
// Erasure functions
//

template <typename T, class Traits>
void basic_string_ref<T, Traits>::pop_front(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= length());
	start += n;
}

template <typename T, class Traits>
void basic_string_ref<T, Traits>::pop_back(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= length());
	finish -= n;
}
template <typename T, class Traits>
std::string basic_string_ref<T, Traits>::to_std_string()
{
	return std::string(data(), length());
}



}


#endif
