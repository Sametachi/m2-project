//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_ITERATORWRAPPER_HPP
#define VSTL_ITERATOR_ITERATORWRAPPER_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/iterator.hpp>
#include <vstl/iterator/traits.hpp>

#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

namespace vstd
{

template <typename Iterator, class Container>
class iterator_wrapper
{
	public:
		typedef typename iterator_traits<Iterator>::iterator_category iterator_category;
		typedef typename iterator_traits<Iterator>::value_type value_type;
		typedef typename iterator_traits<Iterator>::difference_type difference_type;
		typedef typename iterator_traits<Iterator>::pointer pointer;
		typedef typename iterator_traits<Iterator>::reference reference;
		typedef Iterator base_type;

		iterator_wrapper();
		explicit iterator_wrapper(Iterator it);
		iterator_wrapper(const iterator_wrapper<Iterator, Container>& it);

		template <typename U>
		iterator_wrapper(U it, typename boost::enable_if<
			boost::is_convertible<U, Iterator>
		>::type* = 0);

		reference operator*() const;
		pointer operator->() const;

		iterator_wrapper& operator++();
		iterator_wrapper operator++(int);

		iterator_wrapper& operator--();
		iterator_wrapper operator--(int);

		iterator_wrapper& operator+=(difference_type n);
		iterator_wrapper& operator-=(difference_type n);

		reference operator[](difference_type n) const;

		Iterator base() const;

		friend bool operator==(const iterator_wrapper& a,
		                       const iterator_wrapper& b)
		{ return a.m_it == b.m_it; }

		friend bool operator!=(const iterator_wrapper& a,
		                       const iterator_wrapper& b)
		{ return !(a.m_it == b.m_it); }

		friend bool operator<(const iterator_wrapper& a,
		                      const iterator_wrapper& b)
		{ return a.m_it < b.m_it; }

		friend bool operator<=(const iterator_wrapper& a,
		                       const iterator_wrapper& b)
		{ return !(b.m_it < a.m_it); }

		friend bool operator>(const iterator_wrapper& a,
		                      const iterator_wrapper& b)
		{ return b.m_it < a.m_it; }

		friend bool operator>=(const iterator_wrapper& a,
		                       const iterator_wrapper& b)
		{ return !(a.m_it < b.m_it); }

		friend iterator_wrapper operator+(iterator_wrapper it,
		                                  difference_type n)
		{ it.m_it += n; return it; }

		friend iterator_wrapper operator+(difference_type n,
		                                  iterator_wrapper it)
		{ it.m_it -= n; return it; }

		friend iterator_wrapper operator-(iterator_wrapper it,
		                                  difference_type n)
		{ it.m_it -= n; return it; }

		friend iterator_wrapper operator-(difference_type n,
		                                  iterator_wrapper it)
		{ it.m_it += n; return it; }

		friend difference_type operator-(const iterator_wrapper& a,
		                                 const iterator_wrapper& b)
		{ return a.m_it - b.m_it; }

	private:
		Iterator m_it;
};

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>::iterator_wrapper()
	: m_it()
{
	// ctor
}

template <typename Iterator, class Container>
/*explicit*/ iterator_wrapper<Iterator, Container>::
		iterator_wrapper(Iterator it)
	: m_it(it)
{
	// ctor
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>::
		iterator_wrapper(const iterator_wrapper<Iterator, Container>& it)
	: m_it(it.m_it)
{
	// ctor
}

template <typename Iterator, class Container>
template <typename U>
iterator_wrapper<Iterator, Container>::iterator_wrapper(
	U it,
	typename boost::enable_if<
		boost::is_convertible<U, Iterator>
	>::type*
)
	: m_it(it)
{
	// ctor
}

template <typename Iterator, class Container>
typename iterator_wrapper<Iterator, Container>::reference
	iterator_wrapper<Iterator, Container>::operator*() const
{ return *m_it; }

template <typename Iterator, class Container>
typename iterator_wrapper<Iterator, Container>::pointer
	iterator_wrapper<Iterator, Container>::operator->() const
{ return &(*m_it); }

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>&
	iterator_wrapper<Iterator, Container>::operator++()
{
	++m_it;
	return *this;
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>
	iterator_wrapper<Iterator, Container>::operator++(int)
{
	iterator_wrapper tmp(*this);
	++m_it;
	return tmp;
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>&
	iterator_wrapper<Iterator, Container>::operator--()
{
	--m_it;
	return *this;
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>
	iterator_wrapper<Iterator, Container>::operator--(int)
{
	iterator_wrapper tmp(*this);
	--m_it;
	return tmp;
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>& iterator_wrapper<Iterator, Container>::
	operator+=(difference_type n)
{
	m_it += n;
	return *this;
}

template <typename Iterator, class Container>
iterator_wrapper<Iterator, Container>& iterator_wrapper<Iterator, Container>::
	operator-=(difference_type n)
{
	return m_it += -n;
}

template <typename Iterator, class Container>
typename iterator_wrapper<Iterator, Container>::reference
	iterator_wrapper<Iterator, Container>::operator[](difference_type n) const
{ return m_it[n]; }

template <typename Iterator, class Container>
Iterator iterator_wrapper<Iterator, Container>::base() const
{ return m_it; }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator==(const iterator_wrapper<IteratorA, ContainerA>& a,
				const iterator_wrapper<IteratorB, ContainerB>& b)
{ return a.base() == b.base(); }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator!=(const iterator_wrapper<IteratorA, ContainerA>& a,
				const iterator_wrapper<IteratorB, ContainerB>& b)
{ return !(a.base() == b.base()); }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator<(const iterator_wrapper<IteratorA, ContainerA>& a,
			const iterator_wrapper<IteratorB, ContainerB>& b)
{ return a.base() < b.base(); }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator<=(const iterator_wrapper<IteratorA, ContainerA>& a,
				const iterator_wrapper<IteratorB, ContainerB>& b)
{ return !(b.base() < a.base()); }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator>(const iterator_wrapper<IteratorA, ContainerA>& a,
			const iterator_wrapper<IteratorB, ContainerB>& b)
{ return b.base() < a.base(); }

template <typename IteratorA, class ContainerA,
		typename IteratorB, class ContainerB>
bool operator>=(const iterator_wrapper<IteratorA, ContainerA>& a,
				const iterator_wrapper<IteratorB, ContainerB>& b)
{ return !(a.base() < b.base()); }

#ifndef BOOST_NO_CXX11_DECLTYPE_N3276

template <typename IteratorA, class ContainerA,
          typename IteratorB, class ContainerB>
BOOST_FORCEINLINE auto operator-(const iterator_wrapper<IteratorA, ContainerA>& a,
                                 const iterator_wrapper<IteratorB, ContainerB>& b)
	-> decltype(a.base() - b.base())
{ return a.base() - b.base(); }

#else

template <typename IteratorA, class ContainerA,
          typename IteratorB, class ContainerB>
BOOST_FORCEINLINE typename iterator_wrapper<IteratorA, ContainerA>::difference_type
	operator-(const iterator_wrapper<IteratorA, ContainerA>& a,
              const iterator_wrapper<IteratorB, ContainerB>& b)
{ return a.base() - b.base(); }

#endif

}

#endif
