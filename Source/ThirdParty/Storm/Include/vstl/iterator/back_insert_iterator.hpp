//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ITERATOR_BACKINSERTITERATOR_HPP
#define VSTL_ITERATOR_BACKINSERTITERATOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/iterator/iterator.hpp>
#include <vstl/utility/move.hpp>

namespace vstd
{

template <class Container>
class back_insert_iterator
	: public iterator<output_iterator_tag, void, void, void, void>
{
	public:
		typedef Container container_type;
		typedef typename Container::value_type container_value;

		explicit back_insert_iterator(Container& c);
		back_insert_iterator& operator=(const container_value& value);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		back_insert_iterator& operator=(container_value&& value);
#endif

		back_insert_iterator& operator*();
		back_insert_iterator& operator++();
		back_insert_iterator operator++(int);

	protected:
		Container* container;
};

template <class Container>
back_insert_iterator<Container>::back_insert_iterator(Container& c)
	: container(&c)
{
	// ctor
}

template <class Container>
back_insert_iterator<Container>& back_insert_iterator<Container>::
	operator=(const container_value& value)
{
	push_back(*container, value);
	return *this;
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <class Container>
back_insert_iterator<Container>& back_insert_iterator<Container>::
	operator=(container_value&& value)
{
	push_back(*container, vstd::move(value));
	return *this;
}

#endif

template <class Container>
back_insert_iterator<Container>& back_insert_iterator<Container>::operator*()
{ return *this; }

template <class Container>
back_insert_iterator<Container>& back_insert_iterator<Container>::operator++()
{ return *this; }

template <class Container>
back_insert_iterator<Container> back_insert_iterator<Container>::operator++(int)
{ return *this; }

template <class Container>
BOOST_FORCEINLINE back_insert_iterator<Container> back_inserter(Container& c)
{
	return back_insert_iterator<Container>(c);
}

}

#endif
