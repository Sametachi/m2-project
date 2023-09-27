#ifndef VSTL_VECTOR_vector_HPP
#define VSTL_VECTOR_vector_HPP

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
#include <vstl/memory/uninitialized_copy.hpp>
#include <vstl/memory/uninitialized_copy_n.hpp>
#include <vstl/memory/uninitialized_default_n.hpp>
#include <vstl/memory/uninitialized_move.hpp>
#include <vstl/memory/uninitialized_fill_n.hpp>

#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/type_traits/alignment_of.hpp>

namespace vstd
{

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::vector() BOOST_NOEXCEPT
	: m_rep()
{
	leak();
}

template <typename T, class Allocator>
/*explicit*/ BOOST_FORCEINLINE vector<T, Allocator>::
		vector(const allocator_type& alloc)
	: m_rep(alloc)
{
	leak();
}

template <typename T, class Allocator>
/*explicit*/ BOOST_FORCEINLINE vector<T, Allocator>::
		vector(size_type n)
	: m_rep()
{
	Initialize(n);
}

template <typename T, class Allocator>
/*explicit*/ BOOST_FORCEINLINE vector<T, Allocator>::
		vector(size_type n, const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(n);
}

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(size_type n, const value_type& val)
	: m_rep()
{
	Initialize(val, n, n);
}

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(size_type n, const value_type& val, const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(val, n, n);
}

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(size_type n, const value_type& val, size_type cap,
		       const allocator_type& alloc)
	: m_rep(alloc)
{
	Initialize(val, n, cap);
}

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(const vector& other)
	: m_rep(other.m_rep)
{
	const size_type otherLen = other.length();
	Initialize(other.data(), otherLen, otherLen);
}

template <typename T, class Allocator>
template <class Allocator2>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(const vector<T, Allocator2>& other,
		       const allocator_type& alloc)
	: m_rep(alloc)
{
	const size_type otherLen = other.length();
	Initialize(other.data(), otherLen, otherLen);
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(vector&& other)
	: m_rep(vstd::move(other.m_rep))
{
	Initialize(vstd::move(other));
	other.leak();
}

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(vector&& other, const allocator_type& alloc)
	: m_rep(alloc)
{
	if (other.get_allocator() == alloc) {
		Initialize(vstd::move(other));
		other.leak();
	} else {
		VSTD_NOTIFY_BAD_PERFORMANCE("Inequal allocators force us to do a copy instead of a move!");
		Initialize(other.data(), other.length(), other.capacity());
		other.Deallocate();
	}
}

#endif

template <typename T, class Allocator>
template <class InputIterator>
vector<T, Allocator>::
		vector(InputIterator first, InputIterator last,
					typename boost::enable_if<
						typename policy_type::template
							is_only_input_iterator<InputIterator>
					>::type*)
	: m_rep()
{
	if (first != last) {
		Initialize(*first++, 1, 10); // reasonable default cap.

		for ( ; first != last; ++first)
			append(*first, 1);
	}
}

template <typename T, class Allocator>
template <class ForwardIterator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(ForwardIterator first, ForwardIterator last,
					typename boost::enable_if<
						typename policy_type::template
							is_only_forward_iterator<ForwardIterator>
					>::type*)
	: m_rep()
{
	InitializeFromForwardIterator(first, last);
}

template <typename T, class Allocator>
template <class InputIterator>
vector<T, Allocator>::
		vector(InputIterator first, InputIterator last,
					const allocator_type& alloc,
					typename boost::enable_if<
						typename policy_type::template
							is_only_input_iterator<InputIterator>
					>::type*)
	: m_rep(alloc)
{
	if (first != last) {
		Initialize(*first++, 1, 10); // reasonable default cap.

		for ( ; first != last; ++first)
			append(*first, 1);
	}
}

template <typename T, class Allocator>
template <class ForwardIterator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(ForwardIterator first, ForwardIterator last,
					const allocator_type& alloc,
					typename boost::enable_if<
						typename policy_type::template
							is_only_forward_iterator<ForwardIterator>
					>::type*)
	: m_rep(alloc)
{
	Initialize(first, last);
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <typename T, class Allocator>
BOOST_FORCEINLINE vector<T, Allocator>::
		vector(std::initializer_list<value_type> il)
	: m_rep()
{
	Initialize(il.begin(), il.end());
}

#endif

template <typename T, class Allocator>
vector<T, Allocator>::~vector()
{
	DestructRange(m_rep.begin, m_rep.end);
	Deallocate();
}

template <typename T, class Allocator>
typename vector<T, Allocator>::allocator_type&
	vector<T, Allocator>::get_allocator()
{ return m_rep; }

template <typename T, class Allocator>
const typename vector<T, Allocator>::allocator_type&
	vector<T, Allocator>::get_allocator() const
{ return m_rep; }

template <typename T, class Allocator>
vector<T, Allocator>::const_pointer
	vector<T, Allocator>::data() BOOST_NOEXCEPT
{ return m_rep.begin; }

template <typename T, class Allocator>
typename vector<T, Allocator>::pointer
	vector<T, Allocator>::data() const BOOST_NOEXCEPT
{ return m_rep.begin; }

template <typename T, class Allocator>
size_type vector<T, Allocator>::size() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.end - m_rep.begin); }

template <typename T, class Allocator>
size_type vector<T, Allocator>::byte_size() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.end - m_rep.begin) * sizeof(value_type); }

template <typename T, class Allocator>
size_type vector<T, Allocator>::length() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.end - m_rep.begin); }

template <typename T, class Allocator>
size_type vector<T, Allocator>::capacity() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.capacity - m_rep.begin); }

template <typename T, class Allocator>
size_type vector<T, Allocator>::byte_capacity() const BOOST_NOEXCEPT
{ return static_cast<size_type>(m_rep.capacity - m_rep.begin) * sizeof(value_type); }

template <typename T, class Allocator>
bool vector<T, Allocator>::empty() const BOOST_NOEXCEPT
{ return m_rep.begin == m_rep.end; }

template <typename T, class Allocator>
bool vector<T, Allocator>::full() const BOOST_NOEXCEPT
{ return m_rep.end == m_rep.capacity; }

//
// Single element accessors
//

template <typename T, class Allocator>
typename vector<T, Allocator>::const_reference
	vector<T, Allocator>::operator[](size_type pos) const
{ return m_rep.begin[pos]; }

template <typename T, class Allocator>
typename vector<T, Allocator>::reference
	vector<T, Allocator>::operator[](size_type pos)
{ return m_rep.begin[pos]; }

template <typename T, class Allocator>
typename vector<T, Allocator>::reference vector<T, Allocator>::front()
{
	VSTD_ASSERT(m_rep.begin != m_rep.end, "vector<> is empty");
	return *m_rep.begin;
}

template <typename T, class Allocator>
typename vector<T, Allocator>::const_reference vector<T, Allocator>::front() const
{
	VSTD_ASSERT(m_rep.begin != m_rep.end, "vector<> is empty");
	return *m_rep.begin;
}

template <typename T, class Allocator>
typename vector<T, Allocator>::reference vector<T, Allocator>::back()
{
	VSTD_ASSERT(m_rep.begin != m_rep.end, "vector<> is empty");
	return *(m_rep.end - 1);
}

template <typename T, class Allocator>
typename vector<T, Allocator>::const_reference vector<T, Allocator>::back() const
{
	VSTD_ASSERT(m_rep.begin != m_rep.end, "vector<> is empty");
	return *(m_rep.end - 1);
}

//
// Iterators
//

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::iterator
	vector<T, Allocator>::begin() BOOST_NOEXCEPT
{ return iterator(m_rep.begin); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_iterator
	vector<T, Allocator>::begin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.begin); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_iterator
	vector<T, Allocator>::cbegin() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.begin); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::iterator
	vector<T, Allocator>::end() BOOST_NOEXCEPT
{ return iterator(m_rep.finish); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_iterator
	vector<T, Allocator>::end() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_iterator
	vector<T, Allocator>::cend() const BOOST_NOEXCEPT
{ return const_iterator(m_rep.finish); }

//
// Reverse-order iterators
//

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::reverse_iterator
	vector<T, Allocator>::rbegin() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.finish); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_reverse_iterator
	vector<T, Allocator>::rbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::reverse_iterator
	vector<T, Allocator>::rend() BOOST_NOEXCEPT
{ return reverse_iterator(m_rep.start); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_reverse_iterator
	vector<T, Allocator>::rend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_reverse_iterator
	vector<T, Allocator>::crbegin() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.finish); }

template <typename T, class Allocator>
BOOST_FORCEINLINE typename vector<T, Allocator>::const_reverse_iterator
	vector<T, Allocator>::crend() const BOOST_NOEXCEPT
{ return const_reverse_iterator(m_rep.start); }

//
// Assignment operators
//


template <typename T, class Allocator>
vector<T, Allocator>& vector<T, Allocator>::
	operator=(const vector& other)
{
	assign(other.data(), other.size());
	return *this;
}

template <typename T, class Allocator>
template <class Allocator2, class Impl2>
vector<T, Allocator>& vector<T, Allocator>::
	operator=(const vector<T, Allocator2, Impl2>& other)
{
	assign(other.data(), other.size());
	return *this;
}

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES

template <typename T, class Allocator>
vector<T, Allocator>& vector<T, Allocator>::
	operator=(vector&& other)
{
	assign(std::move(other));
	return *this;
}

#endif

template <typename T, class Allocator>
vector<T, Allocator>& vector<T, Allocator>::
	operator=(const typename vector<T, Allocator>::value_type& val)
{
	assign(&val, 1);
	return *this;
}

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST

template <typename T, class Allocator>
vector<T, Allocator>& vector<T, Allocator>::
	operator=(std::initializer_list<value_type> il)
{
	assign(il.begin(), il.end());
	return *this;
}

#endif

//
// Size management operations (reserve, resize, shrink...)
//

template <typename T, class Allocator>
void vector<T, Allocator>::resize(size_type n)
{
	const auto size = static_cast<size_type>(m_rep.end - m_rep.begin);
	if (n > size) {
		const auto cap = static_cast<size_type>(m_rep.capacity - m_rep.begin);
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
void vector<T, Allocator>::resize(size_type n, value_type value)
{
	const auto size = static_cast<size_type>(m_rep.end - m_rep.begin);
	if (n > size) {
		const auto cap = static_cast<size_type>(m_rep.capacity - m_rep.begin);
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
void vector<T, Allocator>::reserve(size_type minimum)
{
	const auto cap = static_cast<size_type>(m_rep.capacity - m_rep.begin);
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
void vector<T, Allocator>::shrink_to_fit()
{
	if (m_rep.end != m_rep.capacity) {
		const auto size = static_cast<size_type>(m_rep.end - m_rep.begin);

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
void vector<T, Allocator>::clear() BOOST_NOEXCEPT
{
	DestructRange(m_rep.begin, m_rep.end);
	m_rep.end = m_rep.begin;
}

template <typename T, class Allocator>
void vector<T, Allocator>::leak()
{
	m_rep.begin = m_rep.end = m_rep.capacity = nullptr;
}

//
// Manipulation
//

template <typename T, class Allocator>
void vector<T, Allocator>::assign(const vector&& other)
{
	m_rep.begin = other.m_rep.begin;
	m_rep.end = other.m_rep.end;
	m_rep.capacity = other.m_rep.capacity;
	other.leak();
}

template <typename T, class Allocator>
void vector<T, Allocator>::assign(const_pointer ptr, size_type len)
{
	if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		const auto size = static_cast<difference_type>(m_rep.end - m_rep.begin);
		if (size >= len) {
			pointer end = vstd::copy(ptr, len, m_rep.begin);

			DestructRange(end, m_rep.end);
			m_rep.end = end;
		} else {
			pointer end = vstd::copy_n(ptr, size, m_rep.begin);
			        end = vstd::uninitialized_copy_n(ptr, len - size, end);

			m_rep.end = end;
		}
	} else {
		pointer begin = Resize(len);
		pointer end = vstd::uninitialized_copy_n(ptr, len, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end;
	}
}

template <typename T, class Allocator>
void vector<T, Allocator>::assign(param_type value, size_type len)
{
	if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		const auto size = static_cast<difference_type>(m_rep.end - m_rep.begin);
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
template <class ForwardIterator>
void vector<T, Allocator>::assign(ForwardIterator first, ForwardIterator last)
{
	const auto len = vstd::distance(first, last);
	if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		const auto size = static_cast<difference_type>(m_rep.end - m_rep.begin);
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
typename vector<T, Allocator>::pointer vector<T, Allocator>::append(size_type count)
{
	if (count <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
		pointer p = m_rep.end;
		m_rep.end += count;
		return p;
	} else {
		pointer begin = Grow(count);
		pointer end = vstd::uninitialized_move(m_rep.begin, m_rep.end, begin);

		DestructMovedRange(m_rep.begin, m_rep.end);
		m_rep.deallocate(m_rep.begin);

		m_rep.begin = begin;
		m_rep.end = end + count;
		return end;
	}
}

template <typename T, class Allocator>
typename vector<T, Allocator>::iterator vector<T, Allocator>::
	insert(const_iterator position, pointer ptr, size_type len)
{
	pointer p = const_cast<iterator>(position);

	if (len) {
		if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
			const auto toMove = static_cast<difference_type>(m_rep.end - p);
			if (toMove > len) {
				pointer end = vstd::uninitialized_move(m_rep.end - len,
				                                       m_rep.end, m_rep.end,
				                                       get_allocator());

				vstd::move_backward(p, m_rep.end - len, m_rep.end);
				vstd::copy_n(p, len, ptr);

				m_rep.end = end;
			} else {
				pointer end = vstd::uninitialized_copy_n(m_rep.end,
				                                         len - toMove,
				                                         ptr);

				ptr += len - toMove;

				// Move the current content past the newly constructed elements
				vstd::uninitialized_move(p, m_rep.end,
				                         end, get_allocator());

				vstd::copy_n(p, toMove, val);

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
typename vector<T, Allocator>::iterator vector<T, Allocator>::
	insert(const_iterator position, param_type val, size_type len)
{
	pointer p = const_cast<iterator>(position);

	if (len) {
		if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
			const auto toMove = static_cast<difference_type>(m_rep.end - p);
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
typename vector<T, Allocator>::iterator vector<T, Allocator>::
	insert(const_iterator position,
		ForwardIterator first,
		ForwardIterator last)
{
	pointer p = const_cast<iterator>(position);

	const auto len = vstd::distance(first, last);
	if (len) {
		if (len <= static_cast<size_type>(m_rep.capacity - m_rep.end)) {
			const auto toMove = static_cast<difference_type>(m_rep.end - p);
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
typename vector<T, Allocator>::iterator
	vector<T, Allocator>::Erase(const_iterator first, const_iterator last)
{
	const auto count = static_cast<difference_type>(last - first);

	pointer p = const_cast<iterator>(first);
	pointer newEnd = vstd::move(p + count, m_rep.end, p);
	DestructRange(newEnd, m_rep.end);

	m_rep.end = newEnd;
	return p;
}

template <typename T, class Allocator>
void vector<T, Allocator>::pop_back(size_type count)
{
	VSTD_CHECK_BOUNDS(newEnd < m_rep.end);

	pointer p = m_rep.begin + count;
	DestructRange(p, m_rep.end);
	m_rep.end = p;
}


template <typename T, class Allocator>
void vector<T, Allocator>::Initialize(param_type value, size_type len,
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
void vector<T, Allocator>::InitializeFromMoved(const vector& other)
{
	m_rep.begin = other.m_rep.begin;
	m_rep.end = other.m_rep.end;
	m_rep.capacity = other.m_rep.capacity;
}

template <typename T, class Allocator>
template <class ForwardIterator>
void vector<T, Allocator>::
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
/*static*/ void vector<T, Allocator>::
	DestructRange(pointer begin, pointer end)
{
	if (!boost::has_trivial_destructor<T>::value) {
		for ( ; begin != end; ++begin)
			begin->~T();
	}
}

template <typename T, class Allocator>
/*static*/ void vector<T, Allocator>::
	DestructMovedRange(pointer begin, pointer end)
{
	if (!is_trivially_post_move_destructible<T>::value) {
		for ( ; begin != end; ++begin)
			begin->~T();
	}
}

template <typename T, class Allocator>
typename vector<T, Allocator>::pointer vector<T, Allocator>::
	Resize(size_type len)
{
	size_type size = static_cast<size_type>(m_rep.capacity - m_rep.begin);
	size = vstd::max(len, GetRecommendedSize(size));

	pointer p = Allocate(size);
	m_rep.capacity = p + size;
	return p;
}

template <typename T, class Allocator>
typename vector<T, Allocator>::pointer vector<T, Allocator>::
	Grow(size_type growth)
{
	size_type size = static_cast<size_type>(m_rep.capacity - m_rep.begin);
	size = vstd::max(size + growth, GetRecommendedSize(size));

	pointer p = Allocate(size);
	m_rep.capacity = p + size;
	return p;
}

template <typename T, class Allocator>
typename vector<T, Allocator>::pointer
	vector<T, Allocator>::Allocate(size_type cap)
{
	using boost::alignment_of;
	VSTD_ASSERT(cap != 0, "Cannot allocate zero size vector");

	return static_cast<pointer>(m_rep.allocate(cap * sizeof(value_type),
	                            alignment_of<value_type>::value));
}

template <typename T, class Traits, class Allocator>
void basic_string<T, Traits, Allocator>::
	Deallocate()
{
	if (m_rep.start != m_rep.capacity)
		m_rep.deallocate(m_rep.begin);
}

}

#endif

