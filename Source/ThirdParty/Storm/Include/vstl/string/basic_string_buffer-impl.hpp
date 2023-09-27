//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRINGBUFFER_IMPL_HPP
#define VSTL_STRING_BASICSTRINGBUFFER_IMPL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/empty.hpp>

namespace vstd
{

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
		basic_string_buffer() BOOST_NOEXCEPT
{
	leak();
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
		basic_string_buffer(basic_string_buffer&& other)
{
	m_rep.start = other.m_rep.start;
	m_rep.finish = other.m_rep.finish;
	m_rep.capacity = other.m_rep.capacity;

	other.leak();
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
		basic_string_buffer(pointer str, size_type n, size_type cap)
{
	m_rep.start = str;
	m_rep.finish = str + n;
	m_rep.capacity = str + cap;
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
		basic_string_buffer(pointer first, pointer last)
{
	m_rep.start = first;
	m_rep.finish = first;
	m_rep.capacity = last;
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
		basic_string_buffer(iterator first, iterator last)
{
	m_rep.start = first.base();
	m_rep.finish = first.base();
	m_rep.capacity = last.base();
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>&
	basic_string_buffer<T, Traits>::operator=(basic_string_buffer&& other)
{
	assign(vstd::move(other));
	other.leak();
	return *this;
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>&
	basic_string_buffer<T, Traits>::operator=(const_pointer str)
{
	assign(str, traits_type::length(str));
	return *this;
}

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>&
	basic_string_buffer<T, Traits>::operator=(value_type ch)
{
	assign(ch, 1);
	return *this;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>&
	basic_string_buffer<T, Traits>::operator=(std::initializer_list<value_type> il)
{
	assign(il.begin(), il.end());
	return *this;
}

#endif

//
// Iterators
//

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::iterator
	basic_string_buffer<T, Traits>::begin() BOOST_NOEXCEPT
{ return iterator(m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_iterator
	basic_string_buffer<T, Traits>::begin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_iterator
	basic_string_buffer<T, Traits>::cbegin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::iterator
	basic_string_buffer<T, Traits>::end() BOOST_NOEXCEPT
{ return iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_iterator
	basic_string_buffer<T, Traits>::end() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_iterator
	basic_string_buffer<T, Traits>::cend() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::reverse_iterator
	basic_string_buffer<T, Traits>::rbegin() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reverse_iterator
	basic_string_buffer<T, Traits>::rbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reverse_iterator
	basic_string_buffer<T, Traits>::crbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::reverse_iterator
	basic_string_buffer<T, Traits>::rend() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reverse_iterator
	basic_string_buffer<T, Traits>::rend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reverse_iterator
	basic_string_buffer<T, Traits>::crend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

//
// Misc
//

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
	operator argument_type() BOOST_NOEXCEPT
{ return m_rep; }

template <typename T, class Traits>
BOOST_FORCEINLINE basic_string_buffer<T, Traits>::
	operator const_argument_type() const BOOST_NOEXCEPT
{ return m_rep; }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_pointer
	basic_string_buffer<T, Traits>::c_str() const BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::pointer
	basic_string_buffer<T, Traits>::data() BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_pointer
	basic_string_buffer<T, Traits>::data() const BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits>
typename basic_string_buffer<T, Traits>::reference
	basic_string_buffer<T, Traits>::operator[](size_type pos) BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits>
typename basic_string_buffer<T, Traits>::const_reference
	basic_string_buffer<T, Traits>::operator[](size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits>
typename basic_string_buffer<T, Traits>::reference
	basic_string_buffer<T, Traits>::at(size_type pos) BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits>
typename basic_string_buffer<T, Traits>::const_reference
	basic_string_buffer<T, Traits>::at(size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::reference
	basic_string_buffer<T, Traits>::front() BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return *m_rep.start;
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reference
	basic_string_buffer<T, Traits>::front() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return *m_rep.start;
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::reference
	basic_string_buffer<T, Traits>::back() BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return m_rep.finish[-1];
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename basic_string_buffer<T, Traits>::const_reference
	basic_string_buffer<T, Traits>::back() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return m_rep.finish[-1];
}

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_buffer<T, Traits>::
	size() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.finish - m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_buffer<T, Traits>::
	length() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.finish - m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE size_type basic_string_buffer<T, Traits>::
	capacity() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.finish - m_rep.start); }

template <typename T, class Traits>
BOOST_FORCEINLINE bool basic_string_buffer<T, Traits>::
	empty() const BOOST_NOEXCEPT
{ return m_rep.finish == m_rep.start; }

//
// Size management operations (reserve, resize, shrink...)
//

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::resize(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= capacity());

	const size_type len = length();
	if (n > len) {
		pointer end = traits_type::initialize(m_rep.finish, n - len);
		traits_type::initialize(end);
		m_rep.finish = end;
	} else {
		pointer end = m_rep.finish - (len - n);
		traits_type::initialize(end);
		m_rep.finish = end;
	}
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::resize(size_type n, value_type ch)
{
	VSTD_CHECK_BOUNDS(n <= capacity());

	const size_type len = length();
	if (n > len) {
		pointer end = traits_type::assign(m_rep.finish, n - len, ch);
		traits_type::initialize(end);
		m_rep.finish = end;
	} else {
		pointer end = m_rep.finish - (len - n);
		traits_type::initialize(end);
		m_rep.finish = end;
	}
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::clear() BOOST_NOEXCEPT
{
	if (m_rep.start != m_rep.finish) {
		traits_type::initialize(m_rep.start);
		m_rep.finish = m_rep.start;
	}
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::leak() BOOST_NOEXCEPT
{
	pointer empty = const_cast<pointer>(empty_string<value_type>());
	m_rep.start = m_rep.finish = m_rep.capacity = empty;
}

//
// Assignment functions
//


//
// Assignment functions
//

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::assign(const_pointer src, size_type len)
{
	VSTD_CHECK_ARGUMENT(src);
	VSTD_CHECK_BOUNDS(len < capacity());

	pointer end = traits_type::copy(m_rep.start, src, len);
	traits_type::initialize(end);
	m_rep.finish = end;
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::assign(value_type ch, size_type len)
{
	VSTD_CHECK_BOUNDS(len < capacity());

	pointer end = traits_type::assign(m_rep.start, len, ch);
	traits_type::initialize(end);
	m_rep.finish = end;
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::assign(basic_string_buffer&& other)
{
	m_rep.start = other.m_rep.start;
	m_rep.finish = other.m_rep.finish;
	m_rep.capacity = other.m_rep.capacity;

	other.leak();
}

template <typename T, class Traits>
template <class ForwardIterator>
void basic_string_buffer<T, Traits>::assign(ForwardIterator first,
                                            ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type len = static_cast<size_type>(vstd::distance(first, last));
	VSTD_CHECK_BOUNDS(len < capacity());

	pointer end = m_rep.start;

	for (; first != last; ++first, ++end)
		*end = *first;

	traits_type::initialize(end);
	m_rep.finish = end;
}


//
// Appending functions
//

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::append(const_pointer src, size_type len)
{
	VSTD_CHECK_ARGUMENT(src);
	VSTD_CHECK_BOUNDS(len <= static_cast<size_type>(m_rep.capacity - m_rep.finish));

	pointer end = traits_type::copy(m_rep.finish, src, len);
	traits_type::initialize((m_rep.finish = end));
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::append(value_type ch, size_type len)
{
	VSTD_CHECK_BOUNDS(len <= static_cast<size_type>(m_rep.capacity - m_rep.finish));

	pointer end = traits_type::assign(m_rep.finish, len, ch);
	traits_type::initialize((m_rep.finish = end));
}

template <typename T, class Traits>
template <class ForwardIterator>
void basic_string_buffer<T, Traits>::append(ForwardIterator first,
                                            ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type len = static_cast<size_type>(vstd::distance(first, last));
	VSTD_CHECK_BOUNDS(len <= static_cast<size_type>(m_rep.capacity - m_rep.finish));

	pointer end = m_rep.finish;

	for (; first != last; ++first, ++end)
		*end = *first;

	traits_type::initialize((m_rep.finish = end));
}

//
// Insertion functions
//

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::insert(size_type position,
                                            const_pointer src,
                                            size_type len)
{
	const size_type curLen = length();

	VSTD_CHECK_ARGUMENT(src);
	VSTD_CHECK_BOUNDS(position < curLen);
	VSTD_CHECK_BOUNDS(capacity() - curLen >= len);

	if (len) {
		pointer p = m_rep.start + position;

		const size_type charsAfter = curLen - position;
		if (charsAfter) {
			//
			// We're not at the string's end, so we need to
			// make room for the string to insert.
			//

			traits_type::move(p + len, p, charsAfter);

			if (src >= p && src < m_rep.finish) {
				//
				// The string to insert is in the area we just
				// moved.
				//

				src += len;
			}
		}

		traits_type::move(p, src, len);
		traits_type::initialize((m_rep.finish += len));
	}
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::insert(size_type position,
                                            value_type ch,
                                            size_type len)
{
	const size_type curLen = length();

	VSTD_CHECK_BOUNDS(position < curLen);
	VSTD_CHECK_BOUNDS(capacity() - curLen >= len);

	if (len) {
		pointer p = m_rep.start + position;

		const size_type charsAfter = curLen - position;
		if (charsAfter) {
			//
			// We're not at the string's end, so we need to
			// make room for the characters to insert.
			//

			traits_type::move(p + len, p, charsAfter);
		}

		traits_type::assign(p, len, ch);
		traits_type::initialize((m_rep.finish += len));
	}
}

template <typename T, class Traits>
template <class ForwardIterator>
void basic_string_buffer<T, Traits>::insert(size_type position,
                                            ForwardIterator first,
                                            ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type len = vstd::distance(first, last);
	const size_type curLen = length();

	VSTD_CHECK_BOUNDS(position < curLen);
	VSTD_CHECK_BOUNDS(capacity() - curLen >= len);

	if (len) {
		pointer p = m_rep.start + position;

		const size_type charsAfter = curLen - position;
		if (charsAfter) {
			//
			// We're not at the string's end, so we need to
			// make room for the characters to insert.
			//

			traits_type::move(p + len, p, charsAfter);
		}

		for (; first != last; ++first, ++p)
			*p = *first;

		traits_type::initialize((m_rep.finish += len));
	}
}

//
// Replace functions
//

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::replace(size_type position,
                                             size_type count,
                                             const_pointer str,
                                             size_type len)
{
	VSTD_CHECK_ARGUMENT(str);

	const size_type curLen = length();

	count = vstd::min(curLen - position, count);

	VSTD_CHECK_BOUNDS(position < curLen);
	VSTD_CHECK_BOUNDS(capacity() - curLen + count >= len);

	pointer first = m_rep.start + position,
	        last  = m_rep.start + position + count;

	const size_type charsAfter = curLen - position - count;
	if (charsAfter != 0 && count > len) {
		//
		// Replacement is shorter, so we just move it in and shuffle
		// the remaining characters backwards.
		//

		traits_type::move(first, str, len);
		traits_type::move(first + len, last, charsAfter);
	} else if (charsAfter != 0 && count != len) {
		if (str < m_rep.finish && str > first) {
			// Replacement is in the source string
			if (str >= last) {
				//
				// Replacement is outside the area to replace ([first, last))
				// which means we need to make room first and then adjust our
				// replacement string to account for the moved characters.
				//

				traits_type::move(first + len, last, charsAfter);
				traits_type::move(first, str + (len - count), len);
			} else {
				//
				// Replacement is partly inside the area to replace.
				// This is tricky, so we first overwrite the part we
				// are allowed to modify ([first, first + count)).
				// After that we just insert the remaining characters.
				//

				traits_type::move(first, str, count);
				traits_type::move(first + len, last, charsAfter);
				traits_type::move(first + count, str + len, len - count);
			}
		} else {
			//
			// Replacement is larger, so we need to make room
			// for the characters to insert.
			//

			traits_type::move(first + len, last, charsAfter);
			traits_type::move(first, str, len);
		}
	} else {
		traits_type::move(first, str, len);
	}

	traits_type::initialize((m_rep.finish += len - count));
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::replace(size_type position,
                                             size_type count,
                                             value_type ch,
                                             size_type len)
{
	const size_type curLen = length();

	count = vstd::min(curLen - position, count);

	VSTD_CHECK_BOUNDS(position < curLen);
	VSTD_CHECK_BOUNDS(capacity() - curLen + count >= len);

	pointer p = m_rep.start + position;

	if (count != len) {
		const size_type charsAfter = curLen - position - count;
		if (charsAfter != 0) {
			//
			// We're not at the string's end and have unequal lengths,
			// which means we need to move the remaining characters.
			//

			traits_type::move(p + len, p + count, charsAfter);
		}
	}

	traits_type::assign(p, len, ch);
	traits_type::initialize((m_rep.finish += len - count));
}

//
// Erasure functions
//

template <typename T, class Traits>
typename basic_string_buffer<T, Traits>::iterator
	basic_string_buffer<T, Traits>::
	erase(size_type position, size_type count)
{
	const size_type len = length();
	VSTD_CHECK_BOUNDS(position < len);

	if (count != 0) {
		const size_type rem = len - position;
		if (count < rem)
			traits_type::move(m_rep.start + position,
			                  m_rep.start + position + count,
			                  rem - count);
		else
			count = rem;

		traits_type::initialize((m_rep.finish -= count));
		return iterator(m_rep.start + position + count);
	}

	return iterator(m_rep.start + position);
}

template <typename T, class Traits>
void basic_string_buffer<T, Traits>::pop_back(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= length());
	traits_type::initialize((m_rep.finish -= n));
}

}

#endif
