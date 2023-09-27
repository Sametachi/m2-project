//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_REVERSEITERATOR_HPP
#define VSTL_ITERATOR_REVERSEITERATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/iterator.hpp>
#include <vstl/iterator/traits.hpp>

namespace vstd
{

template <typename Iterator>
class reverse_iterator
{
	public:
		typedef typename iterator_traits<Iterator>::iterator_category iterator_category;
		typedef typename iterator_traits<Iterator>::value_type value_type;
		typedef typename iterator_traits<Iterator>::difference_type difference_type;
		typedef typename iterator_traits<Iterator>::pointer pointer;
		typedef typename iterator_traits<Iterator>::reference reference;
		typedef Iterator base_type;

		reverse_iterator();
		explicit reverse_iterator(Iterator it);
		reverse_iterator(const reverse_iterator<Iterator>& it);

		template <typename U>
		reverse_iterator(U it);

		reference operator*() const;
		pointer operator->() const;

		reverse_iterator& operator++();
		reverse_iterator operator++(int);

		reverse_iterator& operator--();
		reverse_iterator operator--(int);

		reverse_iterator& operator+=(difference_type n);
		reverse_iterator& operator-=(difference_type n);

    	reference operator[](difference_type n) const;

		Iterator base() const;

		friend bool operator==(const reverse_iterator& a,
		                       const reverse_iterator& b)
		{ return a.m_it == b.m_it; }

		friend bool operator!=(const reverse_iterator& a,
		                       const reverse_iterator& b)
		{ return !(a.m_it == b.m_it); }

		friend bool operator<(const reverse_iterator& a,
		                      const reverse_iterator& b)
		{ return b.m_it < a.m_it; }

		friend bool operator<=(const reverse_iterator& a,
		                       const reverse_iterator& b)
		{ return !(a.m_it < b.m_it); }

		friend bool operator>(const reverse_iterator& a,
		                      const reverse_iterator& b)
		{ return a.m_it < b.m_it; }

		friend bool operator>=(const reverse_iterator& a,
		                       const reverse_iterator& b)
		{ return !(b.m_it < a.m_it); }

		friend reverse_iterator operator+(reverse_iterator it,
		                                  difference_type n)
		{ it.m_it -= n; return it; }

		friend reverse_iterator operator+(difference_type n,
		                                  reverse_iterator it)
		{ it.m_it += n; return it; }

		friend reverse_iterator operator-(reverse_iterator it,
		                                  difference_type n)
		{ it.m_it += n; return it; }

		friend reverse_iterator operator-(difference_type n,
		                                  reverse_iterator it)
		{ it.m_it -= n; return it; }

		friend difference_type operator-(const reverse_iterator& a,
		                                 const reverse_iterator& b)
		{ return b.m_it - a.m_it; }

	private:
		Iterator m_it;
};

template <typename Iterator>
reverse_iterator<Iterator>::reverse_iterator()
	: m_it()
{
	// ctor
}

template <typename Iterator>
/*explicit*/ reverse_iterator<Iterator>::
		reverse_iterator(Iterator it)
	: m_it(it)
{
	// ctor
}

template <typename Iterator>
reverse_iterator<Iterator>::
		reverse_iterator(const reverse_iterator<Iterator>& it)
	: m_it(it.m_it)
{
	// ctor
}

template <typename Iterator>
template <typename U>
reverse_iterator<Iterator>::reverse_iterator(U it)
	: m_it(it)
{
	// ctor
}

template <typename Iterator>
typename reverse_iterator<Iterator>::reference
	reverse_iterator<Iterator>::operator*() const
{
	auto it = m_it;
	return *--it;
}

template <typename Iterator>
typename reverse_iterator<Iterator>::pointer
	reverse_iterator<Iterator>::operator->() const
{ return &(operator*()); }

template <typename Iterator>
reverse_iterator<Iterator>&
	reverse_iterator<Iterator>::operator++()
{
	--m_it;
	return *this;
}

template <typename Iterator>
reverse_iterator<Iterator>
	reverse_iterator<Iterator>::operator++(int)
{
	reverse_iterator tmp(*this);
	--m_it;
	return tmp;
}

template <typename Iterator>
reverse_iterator<Iterator>&
	reverse_iterator<Iterator>::operator--()
{
	++m_it;
	return *this;
}

template <typename Iterator>
reverse_iterator<Iterator>
	reverse_iterator<Iterator>::operator--(int)
{
	reverse_iterator tmp(*this);
	++m_it;
	return tmp;
}

template <typename Iterator>
reverse_iterator<Iterator>& reverse_iterator<Iterator>::
	operator+=(difference_type n)
{
	m_it -= n;
	return *this;
}

template <typename Iterator>
reverse_iterator<Iterator>& reverse_iterator<Iterator>::
	operator-=(difference_type n)
{
	return m_it += n;
}

template <typename Iterator>
typename reverse_iterator<Iterator>::reference
	reverse_iterator<Iterator>::operator[](difference_type n) const
{ return *(*this + n); }

template <typename Iterator>
Iterator reverse_iterator<Iterator>::base() const
{ return m_it; }

template <typename IteratorA, typename IteratorB>
bool operator==(const reverse_iterator<IteratorA>& a,
                const reverse_iterator<IteratorB>& b)
{ return a.base() == b.base(); }

template <typename IteratorA, typename IteratorB>
bool operator!=(const reverse_iterator<IteratorA>& a,
                const reverse_iterator<IteratorB>& b)
{ return !(a.base() == b.base()); }

template <typename IteratorA, typename IteratorB>
bool operator<(const reverse_iterator<IteratorA>& a,
               const reverse_iterator<IteratorB>& b)
{ return b.base() < a.base(); }

template <typename IteratorA, typename IteratorB>
bool operator<=(const reverse_iterator<IteratorA>& a,
                const reverse_iterator<IteratorB>& b)
{ return !(a.base() < b.base()); }

template <typename IteratorA, typename IteratorB>
bool operator>(const reverse_iterator<IteratorA>& a,
               const reverse_iterator<IteratorB>& b)
{ return a.base() < b.base(); }

template <typename IteratorA, typename IteratorB>
bool operator>=(const reverse_iterator<IteratorA>& a,
                const reverse_iterator<IteratorB>& b)
{ return !(b.base() < a.base()); }

#ifndef BOOST_NO_CXX11_DECLTYPE_N3276

template <typename IteratorA, typename IteratorB>
BOOST_FORCEINLINE auto operator-(const reverse_iterator<IteratorA>& a,
                                 const reverse_iterator<IteratorB>& b)
	-> decltype(a.base() - b.base())
{ return b.base() - a.base(); }

#else

template <typename IteratorA, typename IteratorB>
BOOST_FORCEINLINE typename reverse_iterator<IteratorA>::difference_type
	operator-(const reverse_iterator<IteratorA>& a,
              const reverse_iterator<IteratorB>& b)
{ return b.base() - a.base(); }

#endif

}

#endif
