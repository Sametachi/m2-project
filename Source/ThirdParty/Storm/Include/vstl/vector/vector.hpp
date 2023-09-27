//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_VECTOR_VECTOR_HPP
#define VSTL_VECTOR_VECTOR_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/memory/allocator_base.hpp> // Needed because of helper macros

#include <boost/call_traits.hpp>

namespace vstd
{

template <typename T, class Allocator>
class vector
{
	public:
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

		vector() BOOST_NOEXCEPT;

		explicit vector(const allocator_type& alloc);
		explicit vector(size_type n);
		explicit vector(size_type n, const allocator_type& alloc);

		vector(size_type n, const value_type& val);

		vector(size_type n, const value_type& val,
		       const allocator_type& alloc);

		vector(size_type n, const value_type& val, size_type cap,
		       const allocator_type& alloc);

		vector(const vector& other);

		template <class Allocator2>
		vector(const vector<T, Allocator2>& other,
		       const allocator_type& alloc);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		vector(vector&& other);
		vector(vector&& other, const allocator_type& alloc);
#endif

		template <class InputIterator>
		vector(InputIterator first, InputIterator last,
				typename boost::enable_if<boost::mpl::and_<
					is_input_iterator<InputIterator>,
					boost::mpl::not_<is_forward_iterator<InputIterator>
				> > >::type* = 0);

		template <class InputIterator>
		vector(InputIterator first, InputIterator last,
					typename boost::enable_if<
						is_forward_iterator<InputIterator>
						>::type* = 0);

		template <class InputIterator>
		vector(InputIterator first, InputIterator last,
					const allocator_type& alloc,
					typename boost::enable_if<boost::mpl::and_<
						is_input_iterator<InputIterator>,
							boost::mpl::not_<is_forward_iterator<InputIterator>
						> > >::type* = 0);

		template <class InputIterator>
		vector(InputIterator first, InputIterator last,
					const allocator_type& alloc,
					typename boost::enable_if<
						is_forward_iterator<InputIterator>
						>::type* = 0);

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
		vector(std::initializer_list<value_type> il);
		vector(std::initializer_list<value_type> il, const allocator_type& alloc);
#endif

		~vector();

		allocator_type& get_allocator();
		const allocator_type& get_allocator() const;

		const_pointer data() const BOOST_NOEXCEPT;
		pointer data() BOOST_NOEXCEPT;

		size_type size() const BOOST_NOEXCEPT;
		size_type byte_size() const BOOST_NOEXCEPT;
		size_type length() const BOOST_NOEXCEPT;

		size_type capacity() const BOOST_NOEXCEPT;
		size_type byte_capacity() const BOOST_NOEXCEPT;

		bool empty() const BOOST_NOEXCEPT;
		bool full() const BOOST_NOEXCEPT;

		//
		// Single element accessors
		//

		const_reference operator[](size_type pos) const;
		reference operator[](size_type pos);

		reference front();
		const_reference front() const;
		reference back();
		const_reference back() const;

		//
		// Iterators
		//

		iterator begin() BOOST_NOEXCEPT;
		const_iterator begin() const BOOST_NOEXCEPT;
		const_iterator cbegin() const BOOST_NOEXCEPT;

		iterator end() BOOST_NOEXCEPT;
		const_iterator end() const BOOST_NOEXCEPT;
		const_iterator cend() const BOOST_NOEXCEPT;

		//
		// Reverse-order iterators
		//

		reverse_iterator rbegin() BOOST_NOEXCEPT;
		const_reverse_iterator rbegin() const BOOST_NOEXCEPT;
		const_reverse_iterator crbegin() const BOOST_NOEXCEPT;

		reverse_iterator rend() BOOST_NOEXCEPT;
		const_reverse_iterator rend() const BOOST_NOEXCEPT;
		const_reverse_iterator crend() const BOOST_NOEXCEPT;

		//
		// Assignment operators
		//

		vector& operator=(const vector& other);

		template <class Allocator2, class Impl2>
		vector& operator=(const vector<T, Allocator2, Impl2>& other);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		vector& operator=(vector&& other);
#endif

		vector& operator=(const value_type& val);

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
		vector& operator=(std::initializer_list<value_type> il);
#endif

		//
		// Size management operations (reserve, resize, shrink...)
		//

		void resize(size_type n);
		void resize(size_type n, value_type value);

		void reserve(size_type minimum = 0);

		void shrink_to_fit();

		void clear() BOOST_NOEXCEPT;

		void leak() BOOST_NOEXCEPT;

		//
		// Manipulation
		//

		void assign(vector&& other);

		void assign(const_pointer ptr, size_type len);
		void assign(param_type val, size_type len);

		template <class ForwardIterator>
		void assign(ForwardIterator first,
		            ForwardIterator last);

		pointer append(size_type count = 1);

		iterator insert(const_iterator position, const_pointer ptr, size_type len);
		iterator insert(const_iterator position, param_type val, size_type len);

		template <class ForwardIterator>
		iterator insert(const_iterator position,
		                ForwardIterator first,
		                ForwardIterator last);

		iterator erase(const_iterator first, const_iterator last);

		void pop_back(size_type count = 1);

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

		void Initialize(size_type len);
		void Initialize(const_pointer src, size_type len, size_type cap);
		void Initialize(param_type val, size_type len, size_type cap);
		void Initialize(basic_string&& other);

		template <class ForwardIterator>
		void Initialize(ForwardIterator first,
		                ForwardIterator last);

		static void DestructRange(pointer begin, pointer end);
		static void DestructMovedRange(pointer begin, pointer end);

		static size_type GetRecommendedSize(size_type size);

		pointer Resize(size_type len);
		pointer Grow(size_type growth);

		pointer Allocate(size_type cap);
		void Deallocate();

		Rep m_rep;
};

}

#include <vstl/vector/vector-impl.hpp>

#endif
