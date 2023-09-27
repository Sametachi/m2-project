//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_VECTOR_VECTORIMPL_HPP
#define VSTL_VECTOR_VECTORIMPL_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/algorithm/minmax.hpp>
#include <vstl/algorithm/copy.hpp>
#include <vstl/algorithm/copy_backward.hpp>
#include <vstl/algorithm/fill_n.hpp>
#include <vstl/algorithm/move.hpp>
#include <vstl/algorithm/move_backward.hpp>
#include <vstl/iterator/reverse_iterator.hpp>
#include <vstl/iterator/advance.hpp>
#include <vstl/iterator/distance.hpp>
#include <vstl/iterator/unwrap.hpp>
#include <vstl/type_traits/is_trivially_post_move_destructible.hpp>
#include <vstl/memory/allocator_base.hpp> // Needed because of helper macros
#include <vstl/memory/uninitialized_copy.hpp>
#include <vstl/memory/uninitialized_default_n.hpp>
#include <vstl/memory/uninitialized_move.hpp>
#include <vstl/memory/uninitialized_fill_n.hpp>

#include <boost/call_traits.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/alignment_of.hpp>

namespace vstd
{

template <typename T, class Allocator>
class vector_impl
{
	public:
		typedef vector_impl<T, Allocator> this_type;
		typedef Allocator allocator_type;
		typedef T value_type;
		typedef typename boost::call_traits<T>::param_type param_type;
		typedef typename boost::call_traits<T>::reference reference;
		typedef typename boost::call_traits<T>::const_reference const_reference;
		typedef typename boost::call_traits<T>::value_type result_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef size_type size_type;
		typedef ptrdiff_t difference_type;
		typedef pointer iterator;
		typedef const_pointer const_iterator;
		typedef vstd::reverse_iterator<iterator> reverse_iterator;
		typedef vstd::reverse_iterator<const_iterator> const_reverse_iterator;

		vector_impl();
		vector_impl(const allocator_type& alloc);
		vector_impl(vector_impl&& other);

		allocator_type& get_allocator();
		const allocator_type& get_allocator() const;

		const_pointer data() const BOOST_NOEXCEPT
		{ return m_rep.begin; }

		pointer mutable_data() BOOST_NOEXCEPT
		{ return m_rep.begin; }

		//
		// Iterators
		//

		iterator begin() BOOST_NOEXCEPT
		{ return m_rep.begin; }

		const_iterator begin() const BOOST_NOEXCEPT
		{ return m_rep.begin; }

		const_iterator cbegin() const BOOST_NOEXCEPT
		{ return m_rep.begin; }

		iterator end() BOOST_NOEXCEPT
		{ return m_rep.end; }

		const_iterator end() const BOOST_NOEXCEPT
		{ return m_rep.end; }

		const_iterator cend() const BOOST_NOEXCEPT
		{ return m_rep.end; }

		size_type size() const BOOST_NOEXCEPT
		{ return static_cast<size_type>(m_rep.end - m_rep.begin); }

		size_type byte_size() const BOOST_NOEXCEPT
		{ return size() * sizeof(value_type); }

		size_type length() const BOOST_NOEXCEPT
		{ return static_cast<size_type>(m_rep.end - m_rep.begin); }

		size_type capacity() const BOOST_NOEXCEPT
		{ return static_cast<size_type>(m_rep.capacity - m_rep.begin); }

		size_type byte_capacity() const BOOST_NOEXCEPT
		{ return capacity() * sizeof(value_type); }

		bool empty() const BOOST_NOEXCEPT
		{ return m_rep.end == m_rep.begin; }

		bool full() const BOOST_NOEXCEPT
		{ return m_rep.end == m_rep.capacity; }

		//
		// Size management operations (reserve, resize, shrink...)
		//

		void resize(size_type n);
		void resize(size_type n, value_type value);

		void reserve(size_type minimum = 0);

		void shrink_to_fit();

		void clear() BOOST_NOEXCEPT;

	protected:
		struct Rep : public allocator_type
		{
			BOOST_FORCEINLINE Rep()
				: allocator_type()
			{
				// ctor
			}

			BOOST_FORCEINLINE Rep(const allocator_type& alloc)
				: allocator_type(alloc)
			{
				// ctor
			}

			BOOST_FORCEINLINE Rep(allocator_type&& alloc)
				: allocator_type(vstd::move(alloc))
			{
				// ctor
			}

			// Begin of the string / allocated storage
			pointer begin;

			// End of the string / used storage (without terminating zero)
			pointer end;

			// End of usable storage (of course without terminating zero)
			pointer capacity;
		};

		//
		// Initialization functions
		//

		void Initialize();

		// Allocates the needed space and copies @c value @c len times into it.
		void Initialize(param_type value, size_type len, size_type cap);

		void InitializeFromMoved(const vector_impl& other);

		template <class ForwardIterator>
		void InitializeFromForwardIterator(ForwardIterator first,
									ForwardIterator last);

		// Destroys this vector.
		// Called by the basic_string destructor.
		// Eases chaining of string implementations.
		void Destroy();

		void Assign(param_type val, size_type len);

		void AssignFromMoved(const vector_impl& other);

		template <class ForwardIterator>
		void AssignFromForwardIterator(ForwardIterator first,
									ForwardIterator last);

		pointer Append();

		iterator Insert(const_iterator position, pointer& storage);
		iterator Insert(const_iterator position, param_type val, size_type len);

		template <class ForwardIterator>
		iterator InsertFromForwardIterator(const_iterator position,
										ForwardIterator first,
										ForwardIterator last);

		iterator Erase(const_iterator first, const_iterator last);

		// special function to delete the last @c count characters.
		void EraseAtEnd(const_iterator newEnd);

		static void DestructRange(pointer begin, pointer end);
		static void DestructMovedRange(pointer begin, pointer end);

		static size_type GetRecommendedSize(size_type size)
		{ return (size * 2) + 1; }

		pointer Resize(size_type len);
		pointer Grow(size_type growth);

		pointer Allocate(size_type cap);

		Rep m_rep;
};

template <typename T, class Allocator>
vector_impl<T, Allocator>::vector_impl()
	: m_rep()
{
	// ctor
}

template <typename T, class Allocator>
vector_impl<T, Allocator>::vector_impl(const allocator_type& alloc)
	: m_rep(alloc)
{
	// ctor
}

template <typename T, class Allocator>
vector_impl<T, Allocator>::vector_impl(vector_impl&& other)
	: m_rep(vstd::move(other.get_allocator()))
{
	// ctor
}

template <typename T, class Allocator>
Allocator& vector_impl<T, Allocator>::get_allocator()
{
	return m_rep;
}

template <typename T, class Allocator>
const Allocator& vector_impl<T, Allocator>::get_allocator() const
{
	return m_rep;
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::resize(size_type n)
{
	const size_type size = static_cast<size_type>(m_rep.end - m_rep.begin);
	if (n > size) {
		const size_type cap =
			static_cast<size_type>(m_rep.capacity - m_rep.begin);

		if (n <= cap) {
			m_rep.end = vstd::uninitialized_default_n(m_rep.end, n);
		} else {
			pointer begin = Resize(n);
			pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);
					end = vstd::uninitialized_default_n(end, n);

			DestructMovedRange(m_rep.begin, m_rep.end);
			m_rep.deallocate(m_rep.begin);

			m_rep.begin = begin;
			m_rep.end = end;
		}
	} else {
		pointer newEnd = m_rep.end - (size - n);
		DestructRange(newEnd, m_rep.end);
		m_rep.end = newEnd;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::resize(size_type n, value_type value)
{
	const size_type size = static_cast<size_type>(m_rep.end - m_rep.begin);
	if (n > size) {
		const size_type cap =
			static_cast<size_type>(m_rep.capacity - m_rep.begin);

		if (n <= cap) {
			vstd::uninitialized_fill_n(value, n, m_rep.end);
			m_rep.end += n;
		} else {
			pointer begin = Resize(n);
			pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);
					end = vstd::uninitialized_fill_n(value, n, end);

			DestructMovedRange(m_rep.begin, m_rep.end);
			m_rep.deallocate(m_rep.begin);

			m_rep.begin = begin;
			m_rep.end = end;
		}
	} else {
		pointer newEnd = m_rep.end - (size - n);
		DestructRange(newEnd, m_rep.end);
		m_rep.end = newEnd;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::reserve(size_type minimum)
{
	const size_type cap =
		static_cast<size_type>(m_rep.capacity - m_rep.begin);

	if (minimum > cap) {
		pointer begin = Resize(minimum);
		pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::shrink_to_fit()
{
	if (m_rep.end != m_rep.capacity) {
		const size_type size = static_cast<size_type>(m_rep.end - m_rep.begin);

		pointer begin = Allocate(size);
		pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;
		m_rep.capacity = begin + size;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::clear() BOOST_NOEXCEPT
{
	DestructRange(m_rep.begin, m_rep.end);
	m_rep.end = m_rep.begin;
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::Initialize()
{
	m_rep.begin = m_rep.end = m_rep.capacity = nullptr;
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::Initialize(param_type value, size_type len,
										size_type cap)
{
	if (len) {
		pointer begin = Allocate(cap);

		m_rep.begin = begin;
		m_rep.end = vstd::uninitialized_fill_n(begin, len, value);
		m_rep.capacity = begin + cap;
	} else {
		m_rep.begin = m_rep.end = m_rep.capacity = nullptr;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::InitializeFromMoved(const vector_impl& other)
{
	m_rep.begin = other.m_rep.begin;
	m_rep.end = other.m_rep.end;
	m_rep.capacity = other.m_rep.capacity;
}

template <typename T, class Allocator>
template <class ForwardIterator>
void vector_impl<T, Allocator>::
	InitializeFromForwardIterator(ForwardIterator first,
								ForwardIterator last)
{
	typedef iterator_traits<ForwardIterator>::difference_type difference;

	difference len = vstd::distance(first, last);
	if (len) {
		pointer begin = Allocate(len);
		pointer end = vstd::uninitialized_copy(first, last, begin);

		m_rep.begin = begin;
		m_rep.end = end;
		m_rep.capacity = begin + len;
	} else {
		m_rep.begin = m_rep.end = m_rep.capacity = nullptr;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::Destroy()
{
	DestructRange(m_rep.begin, m_rep.end);
	m_rep.deallocate(m_rep.begin);

	m_rep.begin = m_rep.end = m_rep.capacity = nullptr;
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::Assign(param_type value, size_type len)
{
	if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		difference_type size =
			static_cast<difference_type>(m_rep.end - m_rep.begin);

		if (size >= len) {
			pointer end = vstd::fill(value, len, m_rep.begin);

			DestructRange(end, m_rep.end);
			m_rep.end = end;
		} else {
			pointer end = vstd::fill_n(value, size, m_rep.begin);
					end = vstd::uninitialized_fill_n(end, len - size, value);

			m_rep.end = end;
		}
	} else {
		pointer begin = Resize(len);
		pointer end = vstd::uninitialized_fill_n(begin, len, value);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;
	}
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::AssignFromMoved(const vector_impl& other)
{
	m_rep.begin = other.m_rep.begin;
	m_rep.end = other.m_rep.end;
	m_rep.capacity = other.m_rep.capacity;
}

template <typename T, class Allocator>
template <class ForwardIterator>
void vector_impl<T, Allocator>::
	AssignFromForwardIterator(ForwardIterator first, ForwardIterator last)
{
	typedef iterator_traits<ForwardIterator>::difference_type difference;

	difference len = vstd::distance(first, last);
	if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		difference_type size =
			static_cast<difference_type>(m_rep.end - m_rep.begin);

		if (size >= len) {
			pointer end = vstd::copy(first, last, m_rep.begin);

			DestructRange(end, m_rep.end);
			m_rep.end = end;
		} else {
			ForwardIterator mid = first;
			vstd::advance(mid, size);

			pointer end = vstd::copy(first, mid, m_rep.begin);
					end = vstd::uninitialized_copy(mid, last, end);

			m_rep.end = end;
		}
	} else {
		pointer begin = Resize(len);
		pointer end = vstd::uninitialized_copy(first, last, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;
	}
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::pointer vector_impl<T, Allocator>::
	Append()
{
	if (m_rep.end != m_rep.capacity) {
		return m_rep.end++;
	} else {
		pointer begin = Grow(1);
		pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end + 1;

		return end;
	}
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::iterator vector_impl<T, Allocator>::
	Insert(const_iterator position, pointer& storage)
{
	pointer p = const_cast<iterator>(position);

	if (m_rep.end != m_rep.capacity) {
		new (m_rep.end) T(std::move(m_rep.end[-1]));

		vstd::move_backward(p, m_rep.end - 1, m_rep.end);
		++m_rep.end;

		storage = p;
		return p;
	} else {
		pointer begin = Grow(1);
		pointer end = vstd::uninitialized_move(m_rep.begin, p, begin);
		storage = end++;
		end = vstd::uninitialized_move(p, m_rep.end, end);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;

		return storage;
	}
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::iterator vector_impl<T, Allocator>::
	Insert(const_iterator position, param_type val, size_type len)
{
	pointer p = const_cast<iterator>(position);

	if (len) {
		if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
            difference_type toMove =
            	static_cast<difference_type>(m_rep.end - p);

			if (toMove > len) {
				pointer end = vstd::uninitialized_move(m_rep.end - len,
													m_rep.end, m_rep.end,
													get_allocator());

				vstd::move_backward(p, m_rep.end - len, m_rep.end);
				vstd::fill_n(p, len, val);

				m_rep.end = end;
			} else {
				pointer end = vstd::uninitialized_fill_n(m_rep.end,
														val,
														len - toMove,
														get_allocator());

				// Move the current content past the newly constructed elements
				vstd::uninitialized_move(p, m_rep.end,
										end, get_allocator());

				vstd::fill_n(p, toMove, val);

				m_rep.end = end;
			}
		} else {
			pointer begin = Grow(len);
			pointer end = vstd::uninitialized_move(m_rep.begin, p, begin);
					end = vstd::uninitialized_fill_n(end, len, val);
					end = vstd::uninitialized_move(p, m_rep.end, end);

			DestructMovedRange(m_rep.begin, m_rep.end);
			m_rep.deallocate(m_rep.begin);

			m_rep.begin = begin;
			m_rep.end = end;
		}
	}
}

template <typename T, class Allocator>
template <class ForwardIterator>
typename vector_impl<T, Allocator>::iterator vector_impl<T, Allocator>::
	InsertFromForwardIterator(const_iterator position, ForwardIterator first,
							ForwardIterator last)
{
	typedef iterator_traits<ForwardIterator>::difference_type difference;

	pointer p = const_cast<iterator>(position);

	difference len = vstd::distance(first, last);
	if (len) {
		if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
            difference_type toMove =
            	static_cast<difference_type>(m_rep.end - p);

			if (toMove > len) {
				pointer end = vstd::uninitialized_move(m_rep.end - len,
													m_rep.end, m_rep.end,
													get_allocator());

				vstd::move_backward(p, m_rep.end - len, m_rep.end);
				vstd::copy(first, last, p);

				m_rep.end = end;
			} else {
				// Copy-construct the elements at the current end.
				ForwardIterator mid = first;
				vstd::advance(mid, toMove);

				pointer end = vstd::uninitialized_copy(mid, last,
													m_rep.end,
													get_allocator());

				// Move the current content past the newly constructed elements
				vstd::uninitialized_move(p, m_rep.end,
										end, get_allocator());

				// Copy the remaining (exactly @c toMove) elements
				vstd::copy(first, mid, p);

				m_rep.end = end;
			}
		} else {
			pointer begin = Grow(len);
			pointer end = vstd::uninitialized_move(m_rep.begin, p, begin);
					end = vstd::uninitialized_copy(first, last, end);
					end = vstd::uninitialized_move(p, m_rep.end, end);

			DestructMovedRange(m_rep.begin, m_rep.end);
			m_rep.deallocate(m_rep.begin);

			m_rep.begin = begin;
			m_rep.end = end;
		}
	}
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::iterator
	vector_impl<T, Allocator>::Erase(const_iterator first, const_iterator last)
{
	const difference_type count = static_cast<difference_type>(last - first);

	pointer p = const_cast<iterator>(first);
	pointer newEnd = vstd::move(p + count, m_rep.end, p);
	DestructRange(newEnd, m_rep.end);

	m_rep.end = newEnd;
    return p;
}

template <typename T, class Allocator>
void vector_impl<T, Allocator>::EraseAtEnd(const_iterator newEnd)
{
	VSTD_CHECK_BOUNDS(newEnd < m_rep.end);

	pointer p = const_cast<iterator>(newEnd);
	DestructRange(p, m_rep.end);
	m_rep.end = p;
}

template <typename T, class Allocator>
/*static*/ void vector_impl<T, Allocator>::
	DestructRange(pointer begin, pointer end)
{
	if (!boost::has_trivial_destructor<T>::value) {
		for ( ; begin != end; ++begin)
			begin->~T();
	}
}

template <typename T, class Allocator>
/*static*/ void vector_impl<T, Allocator>::
	DestructMovedRange(pointer begin, pointer end)
{
	if (!is_trivially_post_move_destructible<T>::value) {
		for ( ; begin != end; ++begin)
			begin->~T();
	}
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::pointer vector_impl<T, Allocator>::
	Resize(size_type len)
{
	size_type size = static_cast<size_type>(m_rep.capacity - m_rep.begin);
	size = vstd::max(len, GetRecommendedSize(size));

	pointer p = Allocate(size);
	m_rep.capacity = p + size;
	return p;
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::pointer vector_impl<T, Allocator>::
	Grow(size_type growth)
{
	size_type size = static_cast<size_type>(m_rep.capacity - m_rep.begin);
	size = vstd::max(size + growth, GetRecommendedSize(size));

	pointer p = Allocate(size);
	m_rep.capacity = p + size;
	return p;
}

template <typename T, class Allocator>
typename vector_impl<T, Allocator>::pointer
	vector_impl<T, Allocator>::Allocate(size_type cap)
{
	using boost::alignment_of;
	VSTD_ASSERT(cap != 0, "Cannot allocate zero size vector");

	return static_cast<pointer>(m_rep.allocate(cap * sizeof(value_type),
								alignment_of<value_type>::value));
}

}

#endif
