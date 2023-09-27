//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_MUTABLEBASICSTRINGREF_IMPL_HPP
#define VSTL_STRING_MUTABLEBASICSTRINGREF_IMPL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/empty.hpp>

namespace vstd
{

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref() BOOST_NOEXCEPT
{
	base_type::leak();
}

template <typename T, class Traits>
/*explicit*/ BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(skip_initialization_t) BOOST_NOEXCEPT
{
	// Don't do anything here!
}

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(const mutable_basic_string_ref& other)
	: base_type(other.start, other.finish)
{
	// ctor
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(mutable_basic_string_ref&& other)
	: base_type(other.start, other.finish)
{
	other.leak();
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(pointer str)
	: base_type(str, str + traits_type::length(str))
{
	traits_type::initialize(this->finish);
}

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(pointer str, size_type n)
	: base_type(str, str + n)
{
	traits_type::initialize(this->finish);
}

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(pointer first, pointer last)
	: base_type(first, last)
{
	traits_type::initialize(this->finish);
}

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>::
		mutable_basic_string_ref(iterator first, iterator last)
	: base_type(first.base(), last.base())
{
	traits_type::initialize(this->finish);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>&
	mutable_basic_string_ref<T, Traits>::operator=(mutable_basic_string_ref&& other)
{
	this->start = other.start;
	this->finish = other.finish;

	other.leak();
	return *this;
}

#endif

template <typename T, class Traits>
BOOST_FORCEINLINE mutable_basic_string_ref<T, Traits>&
	mutable_basic_string_ref<T, Traits>::operator=(pointer str)
{
	this->start = str;
	this->finish = str + traits_type::length(str);

	traits_type::initialize(this->finish);
	return *this;
}

//
// Iterators
//

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::iterator
	mutable_basic_string_ref<T, Traits>::begin() const BOOST_NOEXCEPT
{ return iterator(this->start); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::iterator
	mutable_basic_string_ref<T, Traits>::end() const BOOST_NOEXCEPT
{ return iterator(this->finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::reverse_iterator
	mutable_basic_string_ref<T, Traits>::rbegin() const BOOST_NOEXCEPT
{ return reverse_iterator(this->finish); }

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::reverse_iterator
	mutable_basic_string_ref<T, Traits>::rend() const BOOST_NOEXCEPT
{ return reverse_iterator(this->start); }

//
// Misc
//

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::pointer
	mutable_basic_string_ref<T, Traits>::data() const BOOST_NOEXCEPT
{ return this->start; }

template <typename T, class Traits>
typename mutable_basic_string_ref<T, Traits>::reference
	mutable_basic_string_ref<T, Traits>::operator[](size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < this->length());
	return this->start[pos];
}

template <typename T, class Traits>
typename mutable_basic_string_ref<T, Traits>::reference
	mutable_basic_string_ref<T, Traits>::at(size_type pos) const BOOST_NOEXCEPT
{
	VSTD_CHECK_BOUNDS(pos < this->length());
	return this->start[pos];
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::reference
	mutable_basic_string_ref<T, Traits>::front() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(this->start != this->finish, "String is empty");
	return *this->start;
}

template <typename T, class Traits>
BOOST_FORCEINLINE typename mutable_basic_string_ref<T, Traits>::reference
	mutable_basic_string_ref<T, Traits>::back() const BOOST_NOEXCEPT
{
	VSTD_ASSERT(this->start != this->finish, "String is empty");
	return this->finish[-1];
}

//
// Assignment functions
//

template <typename T, class Traits>
void mutable_basic_string_ref<T, Traits>::assign(pointer src, size_type len)
{
	VSTD_CHECK_ARGUMENT(src);

	this->start = src;
	this->finish = src + len;

	traits_type::initialize(this->finish);
}

template <typename T, class Traits>
void mutable_basic_string_ref<T, Traits>::assign(pointer first, pointer last)
{
	VSTD_CHECK_ARGUMENT(first);
	VSTD_CHECK_ARGUMENT(last);

	this->start = first;
	this->finish = last;

	traits_type::initialize(this->finish);
}

template <typename T, class Traits>
void mutable_basic_string_ref<T, Traits>::assign(mutable_basic_string_ref&& other)
{
	this->start = other.start;
	this->finish = other.finish;

	other.leak();
}

//
// Erasure functions
//

template <typename T, class Traits>
void mutable_basic_string_ref<T, Traits>::pop_front(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= this->length());
	this->start += n;
}

template <typename T, class Traits>
void mutable_basic_string_ref<T, Traits>::pop_back(size_type n)
{
	VSTD_CHECK_BOUNDS(n <= this->length());
	traits_type::initialize((this->finish -= n));
}

}

#endif
