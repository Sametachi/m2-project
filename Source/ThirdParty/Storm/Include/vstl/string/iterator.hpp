//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_ITERATOR_HPP
#define VSTL_STRING_ITERATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/traits.hpp>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

namespace vstd
{

template <typename Iterator, class Traits>
class string_iterator
{
	public:
		typedef random_access_iterator_tag iterator_category;
		typedef typename iterator_traits<Iterator>::value_type value_type;
		typedef typename iterator_traits<Iterator>::difference_type difference_type;
		typedef typename iterator_traits<Iterator>::pointer pointer;
		typedef typename iterator_traits<Iterator>::reference reference;
		typedef Iterator base_type;

		string_iterator();
		explicit string_iterator(Iterator it);
		string_iterator(const string_iterator<Iterator, Traits>& it);

		template <typename U>
		string_iterator(
			const string_iterator<U, Traits>& it,
			typename boost::enable_if<
				boost::is_convertible<U, Iterator>
			>::type* = 0
		);

		template <typename U>
		string_iterator(U it, typename boost::enable_if<
			boost::is_convertible<U, Iterator>
		>::type* = 0);

		reference operator*() const;
		pointer operator->() const;

		string_iterator& operator++();
		string_iterator operator++(int);

		string_iterator& operator--();
		string_iterator operator--(int);

		string_iterator& operator+=(difference_type n);
		string_iterator& operator-=(difference_type n);

		reference operator[](difference_type n) const;

		Iterator base() const;

		friend bool operator==(const string_iterator& a,
		                       const string_iterator& b)
		{ return a.m_it == b.m_it; }

		friend bool operator!=(const string_iterator& a,
		                       const string_iterator& b)
		{ return !(a.m_it == b.m_it); }

		friend bool operator<(const string_iterator& a,
		                      const string_iterator& b)
		{ return a.m_it < b.m_it; }

		friend bool operator<=(const string_iterator& a,
		                       const string_iterator& b)
		{ return !(b.m_it < a.m_it); }

		friend bool operator>(const string_iterator& a,
		                      const string_iterator& b)
		{ return b.m_it < a.m_it; }

		friend bool operator>=(const string_iterator& a,
		                       const string_iterator& b)
		{ return !(a.m_it < b.m_it); }

		friend string_iterator operator+(string_iterator it,
		                                 difference_type n)
		{ it.m_it += n; return it; }

		friend string_iterator operator+(difference_type n,
		                                 string_iterator it)
		{ it.m_it -= n; return it; }

		friend string_iterator operator-(string_iterator it,
		                                 difference_type n)
		{ it.m_it -= n; return it; }

		friend string_iterator operator-(difference_type n,
		                                 string_iterator it)
		{ it.m_it += n; return it; }

		friend difference_type operator-(const string_iterator& a,
		                                 const string_iterator& b)
		{ return a.m_it - b.m_it; }

	private:
		Iterator m_it;
};

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>::string_iterator()
	: m_it()
{
	// ctor
}

template <typename Iterator, class Traits>
/*explicit*/ string_iterator<Iterator, Traits>::
		string_iterator(Iterator it)
	: m_it(it)
{
	// ctor
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>::
		string_iterator(const string_iterator<Iterator, Traits>& it)
	: m_it(it.m_it)
{
	// ctor
}

template <typename Iterator, class Traits>
template <typename U>
string_iterator<Iterator, Traits>::string_iterator(
	const string_iterator<U, Traits>& it,
	typename boost::enable_if<
		boost::is_convertible<U, Iterator>
	>::type*
)
	: m_it(it.base())
{
	// ctor
}

template <typename Iterator, class Traits>
template <typename U>
string_iterator<Iterator, Traits>::string_iterator(
	U it,
	typename boost::enable_if<
		boost::is_convertible<U, Iterator>
	>::type*
)
	: m_it(it)
{
	// ctor
}

template <typename Iterator, class Traits>
typename string_iterator<Iterator, Traits>::reference
	string_iterator<Iterator, Traits>::operator*() const
{ return *m_it; }

template <typename Iterator, class Traits>
typename string_iterator<Iterator, Traits>::pointer
	string_iterator<Iterator, Traits>::operator->() const
{ return m_it; }

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>&
	string_iterator<Iterator, Traits>::operator++()
{
	++m_it;
	return *this;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>
	string_iterator<Iterator, Traits>::operator++(int)
{
	string_iterator tmp(*this);
	++m_it;
	return tmp;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>&
	string_iterator<Iterator, Traits>::operator--()
{
	--m_it;
	return *this;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>
	string_iterator<Iterator, Traits>::operator--(int)
{
	string_iterator tmp(*this);
	--m_it;
	return tmp;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>& string_iterator<Iterator, Traits>::
	operator+=(difference_type n)
{
	m_it += n;
	return *this;
}

template <typename Iterator, class Traits>
string_iterator<Iterator, Traits>& string_iterator<Iterator, Traits>::
	operator-=(difference_type n)
{
	return m_it += -n;
}

template <typename Iterator, class Traits>
typename string_iterator<Iterator, Traits>::reference
	string_iterator<Iterator, Traits>::operator[](difference_type n) const
{ return m_it[n]; }

template <typename Iterator, class Traits>
Iterator string_iterator<Iterator, Traits>::base() const
{ return m_it; }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator==(const string_iterator<IteratorA, TraitsA>& a,
                const string_iterator<IteratorB, TraitsB>& b)
{ return a.base() == b.base(); }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator!=(const string_iterator<IteratorA, TraitsA>& a,
                const string_iterator<IteratorB, TraitsB>& b)
{ return !(a.base() == b.base()); }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator<(const string_iterator<IteratorA, TraitsA>& a,
               const string_iterator<IteratorB, TraitsB>& b)
{ return a.base() < b.base(); }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator<=(const string_iterator<IteratorA, TraitsA>& a,
                const string_iterator<IteratorB, TraitsB>& b)
{ return !(b.base() < a.base()); }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator>(const string_iterator<IteratorA, TraitsA>& a,
               const string_iterator<IteratorB, TraitsB>& b)
{ return b.base() < a.base(); }

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
bool operator>=(const string_iterator<IteratorA, TraitsA>& a,
                const string_iterator<IteratorB, TraitsB>& b)
{ return !(a.base() < b.base()); }

#ifndef BOOST_NO_CXX11_DECLTYPE_N3276

template <typename IteratorA, class TraitsA,
typename IteratorB, class TraitsB>
BOOST_FORCEINLINE auto operator-(const string_iterator<IteratorA, TraitsA>& a,
                                 const string_iterator<IteratorB, TraitsB>& b)
	-> decltype(a.base() - b.base())
{ return a.base() - b.base(); }

#else

template <typename IteratorA, class TraitsA,
          typename IteratorB, class TraitsB>
BOOST_FORCEINLINE typename string_iterator<IteratorA, TraitsA>::difference_type
	operator-(const string_iterator<IteratorA, TraitsA>& a,
              const string_iterator<IteratorB, TraitsB>& b)
{ return a.base() - b.base(); }

#endif

}

#endif
