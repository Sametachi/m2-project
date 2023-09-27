//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRING_IMPL_HPP
#define VSTL_STRING_BASICSTRING_IMPL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/empty.hpp>
#include <vstl/iterator/distance.hpp>
#include <vstl/algorithm/minmax.hpp>

#include <boost/type_traits/alignment_of.hpp>

namespace vstd
{

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string() BOOST_NOEXCEPT
	: m_rep()
{
	leak();
}

template <typename T, class Traits, class Allocator>
/*explicit*/ BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const allocator_type& alloc)
	: m_rep(alloc)
{
	leak();
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const basic_string& other)
	: m_rep(static_cast<const allocator_type&>(other.m_rep))
{
	const size_type otherLen = other.length();
	Initialize(other.data(), otherLen, otherLen);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const basic_string_ref<T, Traits>& other)
	: m_rep()
{
	const size_type otherLen = other.length();
	Initialize(other.data(), otherLen, otherLen);
}

template <typename T, class Traits, class Allocator>
template <class Allocator2>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::basic_string(
		const basic_string<T, Traits, Allocator2>& other,
		const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type otherLen = other.length();
	Initialize(other.data(), otherLen, otherLen);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const basic_string& other, size_type pos,
					size_type n)
	: m_rep(other.get_allocator())
{
	const size_type len = other.length();
	VSTD_CHECK_BOUNDS(pos < len);

	n = vstd::min(n, len - pos);
	Initialize(other.data() + pos, n, n);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::basic_string(
		const basic_string& other, size_type pos,
					const allocator_type& alloc, size_type n)
	: m_rep(alloc)
{
	const size_type len = other.length();
	VSTD_CHECK_BOUNDS(pos < len);

	n = vstd::min(n, len - pos);
	Initialize(other.data() + pos, n, n);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(basic_string&& other)
	: m_rep(vstd::move(other.m_rep))
{
	other.leak();
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(basic_string&& other, const allocator_type& alloc)
	: m_rep(alloc)
{
	if (other.get_allocator() == alloc) {
		assign(vstd::move(other));
	} else {
		VSTD_NOTIFY_BAD_PERFORMANCE("Inequal allocators force us to do a copy instead of a move!");
		Initialize(other.data(), other.length(), other.capacity());
		other.Deallocate();
		other.leak();
	}
}

#endif

template <typename T, class Traits, class Allocator>
/*explicit*/ BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str)
	: m_rep()
{
	const size_type len = traits_type::length(str);
	Initialize(str, len, len);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str, const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type len = traits_type::length(str);
	Initialize(str, len, len);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str, size_type n)
	: m_rep()
{
	Initialize(str, n, n);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str, size_type n,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(str, n, n);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(size_type n, value_type ch)
	: m_rep()
{
	Initialize(ch, n, n);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(size_type n, value_type ch, const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(ch, n, n);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str, size_type n, size_type cap)
	: m_rep()
{
	Initialize(str, n, cap);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer str, size_type n, size_type cap,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(str, n, cap);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(size_type n, value_type ch, size_type cap,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(ch, n, cap);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer first, const_pointer last)
	: m_rep()
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first, len, len + 1);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_pointer first, const_pointer last,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first, len, len);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(iterator first, iterator last)
	: m_rep()
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first.base(), len, len + 1);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(iterator first, iterator last,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first.base(), len, len + 1);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_iterator first, const_iterator last)
	: m_rep()
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first.base(), len, len + 1);
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(const_iterator first, const_iterator last,
					const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type len = static_cast<size_type>(last - first);
	Initialize(first.base(), len, len + 1);
}

template <typename T, class Traits, class Allocator>
template <class InputIterator>
basic_string<T, Traits, Allocator>::basic_string(
		InputIterator first, InputIterator last,
		typename boost::enable_if<
			typename detail::
				IsOnlyStringInputIterator<string_type, InputIterator>
		>::type*)
	: m_rep()
{
	if (first != last) {
		Initialize(*first++, 1, 10); // reasonable default cap.

		for ( ; first != last; ++first)
			append(*first, 1);
	}
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::basic_string(
		ForwardIterator first, ForwardIterator last,
		typename boost::enable_if<
			typename detail::
				IsOnlyStringForwardIterator<string_type, ForwardIterator>
		>::type*)
	: m_rep()
{
	InitializeFromForwardIterator(first, last);
}

template <typename T, class Traits, class Allocator>
template <class InputIterator>
basic_string<T, Traits, Allocator>::basic_string(
	InputIterator first, InputIterator last,
	const allocator_type& alloc,
	typename boost::enable_if<
		typename detail::
			IsOnlyStringInputIterator<string_type, InputIterator>
	>::type*)
	: m_rep(alloc)
{
	if (first != last) {
		Initialize(*first++, 1, 10); // reasonable default cap.

		for ( ; first != last; ++first)
			append(*first, 1);
	}
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::basic_string(
	ForwardIterator first, ForwardIterator last,
	const allocator_type& alloc,
	typename boost::enable_if<
		typename detail::
			IsOnlyStringForwardIterator<string_type, ForwardIterator>
	>::type*)
	: m_rep(alloc)
{
	Initialize(first, last);
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
		basic_string(std::initializer_list<value_type> il)
	: m_rep()
{
	Initialize(il.begin(), il.end());
}

#endif

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::~basic_string()
{
	Deallocate();
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(const basic_string& other)
{
	assign(other.data(), other.length());
	return *this;
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(const const_ref_type& other)
{
	assign(other.data(), other.length());
	return *this;
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(basic_string&& other)
{
	if (other.get_allocator() == get_allocator()) {
		assign(vstd::move(other));
	} else {
		VSTD_NOTIFY_BAD_PERFORMANCE("Inequal allocators force us to do a copy instead of a move!");
		assign(other.data(), other.length());
		other.Deallocate();
		other.leak();
	}

	return *this;
}

#endif

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(const_pointer str)
{
	assign(str, traits_type::length(str));
	return *this;
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(value_type ch)
{
	assign(ch, 1);
	return *this;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>&
	basic_string<T, Traits, Allocator>::operator=(std::initializer_list<value_type> il)
{
	assign(il.begin(), il.end());
	return *this;
}

#endif

//
// Iterators
//

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::iterator
	basic_string<T, Traits, Allocator>::begin() BOOST_NOEXCEPT
{ return iterator(m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_iterator
	basic_string<T, Traits, Allocator>::begin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_iterator
	basic_string<T, Traits, Allocator>::cbegin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::iterator
	basic_string<T, Traits, Allocator>::end() BOOST_NOEXCEPT
{ return iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_iterator
	basic_string<T, Traits, Allocator>::end() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_iterator
	basic_string<T, Traits, Allocator>::cend() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::reverse_iterator
	basic_string<T, Traits, Allocator>::rbegin() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reverse_iterator
	basic_string<T, Traits, Allocator>::rbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reverse_iterator
	basic_string<T, Traits, Allocator>::crbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::reverse_iterator
	basic_string<T, Traits, Allocator>::rend() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reverse_iterator
	basic_string<T, Traits, Allocator>::rend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reverse_iterator
	basic_string<T, Traits, Allocator>::crend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

//
// Misc
//

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
	operator argument_type() BOOST_NOEXCEPT
{ return m_rep; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE basic_string<T, Traits, Allocator>::
	operator const_argument_type() const BOOST_NOEXCEPT
{ return m_rep; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::allocator_type&
	basic_string<T, Traits, Allocator>::get_allocator()
{ return m_rep; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE const typename basic_string<T, Traits, Allocator>::allocator_type&
	basic_string<T, Traits, Allocator>::get_allocator() const
{ return m_rep; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_pointer
	basic_string<T, Traits, Allocator>::c_str() const BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::pointer
	basic_string<T, Traits, Allocator>::data() BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_pointer
	basic_string<T, Traits, Allocator>::data() const BOOST_NOEXCEPT
{ return m_rep.start; }

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::reference
	basic_string<T, Traits, Allocator>::operator[](size_type pos) BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::const_reference
	basic_string<T, Traits, Allocator>::operator[](size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::reference
	basic_string<T, Traits, Allocator>::at(size_type pos) BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::const_reference
	basic_string<T, Traits, Allocator>::at(size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < length());
	return m_rep.start[pos];
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::reference
	basic_string<T, Traits, Allocator>::front() BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return *m_rep.start;
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reference
	basic_string<T, Traits, Allocator>::front() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return *m_rep.start;
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::reference
	basic_string<T, Traits, Allocator>::back() BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return m_rep.finish[-1];
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE typename basic_string<T, Traits, Allocator>::const_reference
	basic_string<T, Traits, Allocator>::back() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(m_rep.start != m_rep.finish, "String is empty");
	return m_rep.finish[-1];
}

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE size_type basic_string<T, Traits, Allocator>::
	size() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.finish - m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE size_type basic_string<T, Traits, Allocator>::
	length() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.finish - m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE size_type basic_string<T, Traits, Allocator>::
	capacity() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.capacity - m_rep.start); }

template <typename T, class Traits, class Allocator>
BOOST_FORCEINLINE bool basic_string<T, Traits, Allocator>::
	empty() const BOOST_NOEXCEPT
{ return m_rep.finish == m_rep.start; }

//
// Size management operations (reserve, resize, shrink...)
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::resize(size_type n)
{
	const size_type len = length();
	const size_type cap = capacity();

	if (n > cap) {
		const size_type newCap = GetRecommendedSize(n);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, len);

		end = traits_type::initialize(end, n - len);
		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	} else {
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
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::
	resize(size_type n, value_type ch)
{
	const size_type len = length();
	const size_type cap = capacity();

	if (n > cap) {
		const size_type newCap = GetRecommendedSize(n);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, len);

		end = traits_type::assign(end, n - len, ch);
		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	} else {
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
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::reserve(size_type minimum)
{
	const size_type cap = capacity();
	if (minimum > cap) {
		const size_type newCap = GetRecommendedSize(minimum);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, length());

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::shrink_to_fit()
{
	if (m_rep.finish != m_rep.capacity) {
		const size_type len = length();
		if (len != 0) {
			pointer begin = Allocate(len);
			pointer end = traits_type::copy(begin, m_rep.start, len);

			traits_type::initialize(end);

			Deallocate();

			m_rep.start = begin;
			m_rep.finish = end;
			m_rep.capacity = begin + len;
		} else {
			leak();
		}
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::clear() BOOST_NOEXCEPT
{
	if (m_rep.start != m_rep.finish) {
		traits_type::initialize(m_rep.start);
		m_rep.finish = m_rep.start;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::leak() BOOST_NOEXCEPT
{
	auto p = const_cast<pointer>(empty_string<value_type>());
	m_rep.start = m_rep.finish = m_rep.capacity = p;
}

//
// Assignment functions
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::assign(const_pointer src,
                                                size_type len)
{
	VSTD_CHECK_ARGUMENT(src);

	if (len <= capacity()) {
		pointer end = traits_type::copy(m_rep.start, src, len);

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type newCap = GetRecommendedSize(len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, src, len);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::assign(value_type ch, size_type len)
{
	if (len <= capacity()) {
		pointer end = traits_type::assign(m_rep.start, len, ch);

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type newCap = GetRecommendedSize(len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::assign(begin, len, ch);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::assign(basic_string&& other)
{
	Deallocate();

	m_rep.start = other.m_rep.start;
	m_rep.finish = other.m_rep.finish;
	m_rep.capacity = other.m_rep.capacity;

	other.leak();
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
void basic_string<T, Traits, Allocator>::assign(ForwardIterator first,
                                                ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type len = static_cast<size_type>(vstd::distance(first, last));
	if (len <= capacity()) {
		pointer end = m_rep.start;

		for (; first != last; ++first, ++end)
			*end = *first;

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type newCap = GetRecommendedSize(len);

		pointer begin = Allocate(newCap);
		pointer end = begin;

		for (; first != last; ++first, ++end)
			*end = *first;

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

//
// Appending functions
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::append(const_pointer src,
                                                size_type len)
{
	VSTD_CHECK_ARGUMENT(src);

	const size_type remCap = static_cast<size_type>(m_rep.capacity - m_rep.finish);
	if (len <= remCap) {
		pointer end = traits_type::copy(m_rep.finish, src, len);

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type curLen = length();
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, curLen);
		        end = traits_type::copy(end, src, len);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::append(value_type ch, size_type len)
{
	const size_type remCap = static_cast<size_type>(m_rep.capacity - m_rep.finish);
	if (len <= remCap) {
		pointer end = traits_type::assign(m_rep.finish, len, ch);

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type curLen = length();
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, curLen);
		        end = traits_type::assign(end, len, ch);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
void basic_string<T, Traits, Allocator>::append(ForwardIterator first,
                                                ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type len = static_cast<size_type>(vstd::distance(first, last));
	const size_type remCap = static_cast<size_type>(m_rep.capacity - m_rep.finish);

	if (len <= remCap) {
		pointer end = m_rep.finish;

		for (; first != last; ++first, ++end)
			*end = *first;

		traits_type::initialize(end);

		m_rep.finish = end;
	} else {
		const size_type curLen = length();
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, curLen);

		for (; first != last; ++first, ++end)
			*end = *first;

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

//
// Insertion functions
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::insert(size_type position,
                                                const_pointer src,
                                                size_type len)
{
	const size_type curLen = length();
	const size_type cap = capacity();

	VSTD_CHECK_ARGUMENT(src);
	VSTD_CHECK_BOUNDS(position < curLen);

	if (cap - curLen >= len) {
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
	} else {
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, position);
		        end = traits_type::copy(end, src, len);
		        end = traits_type::copy(end, m_rep.start + position,
		                                curLen - position);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::insert(size_type position,
                                                value_type ch,
                                                size_type len)
{
	const size_type curLen = length();
	const size_type cap = capacity();

	VSTD_CHECK_BOUNDS(position < curLen);

	if (cap - curLen >= len) {
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
	} else {
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, position);
		        end = traits_type::assign(end, len, ch);
		        end = traits_type::copy(end, m_rep.start + position,
		                                curLen - position);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
void basic_string<T, Traits, Allocator>::insert(size_type position,
                                                ForwardIterator first,
                                                ForwardIterator last)
{
	BOOST_STATIC_ASSERT(is_forward_iterator<ForwardIterator>::value);

	const size_type curLen = length();
	const size_type cap = capacity();
	VSTD_CHECK_BOUNDS(position < curLen);

	const size_type len = vstd::distance(first, last);
	if (cap - curLen >= len) {
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
	} else {
		const size_type newCap = GetRecommendedSize(curLen + len);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, position);

		for (; first != last; ++first, ++end)
			*end = *first;

		end = traits_type::copy(end, m_rep.start + position,
		                        curLen - position);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

//
// Replace functions
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::replace(size_type position,
                                                 size_type count,
                                                 const_pointer str,
                                                 size_type len)
{
	VSTD_CHECK_ARGUMENT(str);

	const size_type curLen = length();
	VSTD_CHECK_BOUNDS(position < curLen);

	count = vstd::min(curLen - position, count);

	const size_type cap = capacity();
	if (cap - curLen + count >= len) {
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
	} else {
		const size_type newCap = GetRecommendedSize(curLen + len - count);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, position);
		        end = traits_type::copy(end, str, len);
		        end = traits_type::copy(end,
		                                m_rep.start + position + count,
		                                curLen - position - count);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::replace(size_type position,
                                                 size_type count,
                                                 value_type ch,
                                                 size_type len)
{
	const size_type curLen = length();
	VSTD_CHECK_BOUNDS(position < curLen);

	count = vstd::min(curLen - position, count);

	const size_type cap = capacity();
	if (cap - curLen + count >= len) {
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
	} else {
		const size_type newCap = GetRecommendedSize(curLen + len - count);

		pointer begin = Allocate(newCap);
		pointer end = traits_type::copy(begin, m_rep.start, position);
		        end = traits_type::assign(end, len, ch);
		        end = traits_type::copy(end,
		                                m_rep.start + position + count,
		                                curLen - position - count);

		traits_type::initialize(end);

		Deallocate();

		m_rep.start = begin;
		m_rep.finish = end;
		m_rep.capacity = begin + newCap;
	}
}

//
// Erasure functions
//

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::iterator
	basic_string<T, Traits, Allocator>::
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

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::pop_back(size_type count)
{
	VSTD_CHECK_BOUNDS(count < length());

	if (count != 0)
		traits_type::initialize((m_rep.finish -= count));
}

//
// Private implementation
//

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::
	Initialize(const_pointer src, size_type len, size_type cap)
{
	VSTD_CHECK_ARGUMENT(src);
	VSTD_CHECK_BOUNDS(cap >= len);

	if (cap) {
		pointer ptr = Allocate(cap);

		m_rep.start = ptr;
		m_rep.capacity = ptr + cap;

		ptr = traits_type::copy(ptr, src, len);
		traits_type::initialize(ptr);

		m_rep.finish = ptr;
	} else {
		leak();
	}
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::
	Initialize(value_type ch, size_type len, size_type cap)
{
	VSTD_CHECK_BOUNDS(cap >= len);

	if (cap) {
		pointer ptr = Allocate(cap);

		m_rep.start = ptr;
		m_rep.capacity = ptr + cap;

		ptr = traits_type::assign(ptr, len, ch);
		traits_type::initialize(ptr);

		m_rep.finish = ptr;
	} else {
		leak();
	}
}

template <typename T, class Traits, class Allocator>
template <class ForwardIterator>
void basic_string<T, Traits, Allocator>::Initialize(ForwardIterator first,
                                                    ForwardIterator last)
{
	const size_type len = static_cast<size_type>(vstd::distance(first, last));

	if (len) {
		pointer ptr = Allocate(len);

		m_rep.start = ptr;

		for (; first != last; ++first, ++ptr)
			*ptr = *first;

		traits_type::initialize(ptr);

		m_rep.capacity = m_rep.finish = ptr;
	} else {
		leak();
	}
}

template <typename T, class Traits, class Allocator>
typename basic_string<T, Traits, Allocator>::pointer
	basic_string<T, Traits, Allocator>::Allocate(size_type cap)
{
	// NB: Allow user-defined alignment_of structs
	using boost::alignment_of;

	const size_type bytes = (cap + 1) * sizeof(value_type);
	const size_type alignment = alignment_of<value_type>::value;

	return static_cast<pointer>(m_rep.allocate(bytes, alignment));
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::
	Deallocate()
{
	if (m_rep.start != m_rep.capacity)
		m_rep.deallocate(m_rep.start);
}

}

#endif
