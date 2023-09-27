//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_ALGORITHM_MINMAX_HPP
#define VSTL_ALGORITHM_MINMAX_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/utility/pair.hpp>

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
	#include <initializer_list>
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef minmax
#undef minmax
#endif

namespace vstd
{

template <class ForwardIterator, class Compare>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last,
                            Compare comp)
{
	if (first != last) {
		ForwardIterator i = first;
		while (++i != last)
			if (comp(*i, *first))
				first = i;
	}

	return first;
}

template <class ForwardIterator>
ForwardIterator min_element(ForwardIterator first, ForwardIterator last)
{
	if (first != last) {
		ForwardIterator i = first;
		while (++i != last)
			if (*i < *first)
				first = i;
	}

	return first;
}

template <class T, class Compare>
BOOST_FORCEINLINE const T& min(const T& a, const T& b, Compare comp)
{
	if (comp(a, b))
		return a;

	return b;
}

template <class T>
BOOST_FORCEINLINE const T& min(const T& a, const T& b)
{
	if (a < b)
		return a;

	return b;
}

template <class ForwardIterator, class Compare>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last,
                            Compare comp)
{
	if (first != last) {
		ForwardIterator i = first;
		while (++i != last)
			if (comp(*first, *i))
				first = i;
	}

	return first;
}

template <class ForwardIterator>
ForwardIterator max_element(ForwardIterator first, ForwardIterator last)
{
	if (first != last) {
		ForwardIterator i = first;
		while (++i != last)
			if (*first < *i)
				first = i;
	}

	return first;
}

template <class T, class Compare>
BOOST_FORCEINLINE const T& max(const T& a, const T& b, Compare comp)
{
	if (comp(a, b))
		return b;

	return a;
}

template <class T>
BOOST_FORCEINLINE const T& max(const T& a, const T& b)
{
	if (a < b)
		return b;

	return a;
}

template <class ForwardIterator, class Compare>
std::pair<ForwardIterator, ForwardIterator> minmax_element(
	ForwardIterator first, ForwardIterator last, Compare comp)
{
	std::pair<ForwardIterator, ForwardIterator> result(first, first);

	if ((first != last) && (++first != last)) {
		if (comp(*first, *result.first))
			result.first = first;
		else
			result.second = first;

		while (++first != last) {
			ForwardIterator i = first;

			if (++first == last) {
				if (comp(*i, *result.first))
					result.first = i;
				else if (!comp(*i, *result.second))
					result.second = i;

				break;
			} else {
				if (comp(*first, *i)) {
					if (comp(*first, *result.first))
						result.first = first;

					if (!comp(*i, *result.second))
						result.second = i;
				} else {
					if (comp(*i, *result.first))
						result.first = i;

					if (!comp(*first, *result.second))
						result.second = first;
				}
			}
		}
	}

	return result;
}

template <class ForwardIterator, class Compare>
std::pair<ForwardIterator, ForwardIterator> minmax_element(
	ForwardIterator first, ForwardIterator last)
{
	std::pair<ForwardIterator, ForwardIterator> result(first, first);

	if ((first != last) && (++first != last)) {
		if (*first < *result.first)
			result.first = first;
		else
			result.second = first;

		while (++first != last) {
			ForwardIterator i = first;

			if (++first == last) {
				if (*i < *result.first)
					result.first = i;
				else if (!(*i < *result.second))
					result.second = i;

				break;
			} else {
				if (*first < *i) {
					if (*first < *result.first)
						result.first = first;

					if (!(*i < *result.second))
						result.second = i;
				} else {
					if (*i < *result.first)
						result.first = i;

					if (!(*first < *result.second))
						result.second = first;
				}
			}
		}
	}

	return result;
}

template <class T, class Compare>
BOOST_FORCEINLINE pair<const T&, const T&> minmax(
	const T& a, const T& b, Compare comp)
{
	typedef pair<const T&, const T&> pair_type;

	if (comp(a, b))
		return pair_type(a, b);

	return pair_type(b, a);
}

template <class T>
BOOST_FORCEINLINE pair<const T&, const T&> minmax(const T& a, const T& b)
{
	typedef pair<const T&, const T&> pair_type;

	if (a < b)
		return pair_type(a, b);

	return pair_type(b, a);
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <class T, class Compare>
BOOST_FORCEINLINE T min(std::initializer_list<T> il, Compare comp)
{
	return *vstd::min_element(il.begin(), il.end(), comp);
}

template <class T>
BOOST_FORCEINLINE T min(std::initializer_list<T> il)
{
	return *vstd::min_element(il.begin(), il.end());
}

template <class T, class Compare>
BOOST_FORCEINLINE T max(std::initializer_list<T> il, Compare comp)
{
	return *vstd::max_element(il.begin(), il.end(), comp);
}

template <class T>
BOOST_FORCEINLINE T max(std::initializer_list<T> il)
{
	return *vstd::max_element(il.begin(), il.end());
}

template <class T>
BOOST_FORCEINLINE pair<T, T> minmax(std::initializer_list<T> il)
{
	pair<const T*, const T*> r = vstd::minmax_element(il.begin(), il.end());
	return pair<T, T>(*r.first, *r.second);
}

template <class T, class Compare>
BOOST_FORCEINLINE pair<T, T> minmax(std::initializer_list<T> il, Compare comp)
{
	pair<const T*, const T*> r = vstd::minmax_element(il.begin(), il.end(), comp);
	return pair<T, T>(*r.first, *r.second);
}

#endif

}

#endif
