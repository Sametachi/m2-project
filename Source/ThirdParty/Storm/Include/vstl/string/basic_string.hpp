//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRING_HPP
#define VSTL_STRING_BASICSTRING_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <string_view>
#include <string>

#include <vstl/string/string_append_mixin.hpp>
#include <vstl/string/string_assign_mixin.hpp>
#include <vstl/string/string_insert_mixin.hpp>
#include <vstl/string/string_add_mixin.hpp>
#include <vstl/string/string_erase_mixin.hpp>
#include <vstl/string/string_replace_mixin.hpp>
#include <vstl/string/string_compare_mixin.hpp>
#include <vstl/string/string_find_mixin.hpp>
#include <vstl/string/basic_string_fwd.hpp>
#include <vstl/string/basic_string_ref.hpp>
#include <vstl/string/mutable_basic_string_ref.hpp>
#include <vstl/string/iterator.hpp>
#include <vstl/string/char_traits.hpp>
#include <vstl/string/helpers.hpp>
#include <vstl/iterator/iterator_wrapper.hpp>
#include <vstl/iterator/reverse_iterator.hpp>
#include <vstl/memory/allocator.hpp>
#include <vstl/functional/hash.hpp>


namespace vstd
{

template <
	typename T,
	class Traits = char_traits<T>,
	class Allocator = allocator
>
class basic_string
#define VSTD_MAKE_STRING_MIXIN(name) name <basic_string<T, Traits, Allocator> >
	: public VSTD_MAKE_STRING_MIXIN(string_append_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_basic_assign_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_ext_assign_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_insert_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_add_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_erase_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_replace_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_compare_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_find_mixin)
#undef VSTD_MAKE_STRING_MIXIN
{
	public:
		typedef basic_string<T, Traits, Allocator> string_type;

		typedef mutable_basic_string_ref<T, Traits> ref_type;
		typedef basic_string_ref<T, Traits> const_ref_type;

		typedef const ref_type& argument_type;
		typedef const const_ref_type& const_argument_type;

		typedef Traits traits_type;
		typedef Allocator allocator_type;

		typedef typename traits_type::char_type value_type;
		typedef ptrdiff_t difference_type;

		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;

		typedef string_iterator<pointer, traits_type> iterator;
		typedef string_iterator<const_pointer, traits_type> const_iterator;
		typedef vstd::reverse_iterator<iterator> reverse_iterator;
		typedef vstd::reverse_iterator<const_iterator> const_reverse_iterator;

		static BOOST_CONSTEXPR_OR_CONST size_type npos =
			static_cast<size_type>(-1);

		basic_string() BOOST_NOEXCEPT;

		explicit basic_string(const allocator_type& alloc);

		basic_string(const basic_string& other);
		basic_string(const basic_string_ref<T, Traits>& other);

		template <class Allocator2>
		basic_string(const basic_string<T, Traits, Allocator2>& other,
		             const allocator_type& alloc);

		basic_string(const basic_string& other, size_type pos,
		             size_type n = npos);

		basic_string(const basic_string& other, size_type pos,
		             const allocator_type& alloc,
		             size_type n = npos);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string(basic_string&& other);
		basic_string(basic_string&& other, const allocator_type& alloc);
#endif

		explicit basic_string(const_pointer str);

		basic_string(const_pointer str, const allocator_type& alloc);
		basic_string(const_pointer str, size_type n);
		basic_string(const_pointer str, size_type n,
		             const allocator_type& alloc);

		basic_string(const_pointer str, size_type n, size_type cap);
		basic_string(const_pointer str, size_type n, size_type cap,
		             const allocator_type& alloc);

		basic_string(const_pointer first, const_pointer last);

		basic_string(const_pointer first, const_pointer last,
		             const allocator_type& alloc);

		basic_string(iterator first, iterator last);
		basic_string(iterator first, iterator last,
		             const allocator_type& alloc);

		basic_string(const_iterator first, const_iterator last);
		basic_string(const_iterator first, const_iterator last,
		             const allocator_type& alloc);

		template <class InputIterator>
		basic_string(InputIterator first, InputIterator last,
		             typename boost::enable_if<
		                 typename detail::
		                 IsOnlyStringInputIterator<string_type, InputIterator>
		             >::type* = 0);

		template <class ForwardIterator>
		basic_string(ForwardIterator first, ForwardIterator last,
		             typename boost::enable_if<
		                 typename detail::
		                 IsOnlyStringForwardIterator<string_type, ForwardIterator>
		             >::type* = 0);

		template <class InputIterator>
		basic_string(InputIterator first, InputIterator last,
		             const allocator_type& alloc,
		             typename boost::enable_if<
		                 typename detail::
		                 IsOnlyStringInputIterator<string_type, InputIterator>
		             >::type* = 0);

		template <class ForwardIterator>
		basic_string(ForwardIterator first, ForwardIterator last,
		             const allocator_type& alloc,
		             typename boost::enable_if<
		                 typename detail::
		                 IsOnlyStringForwardIterator<string_type, ForwardIterator>
		             >::type* = 0);

		basic_string(size_type n, value_type ch);
		basic_string(size_type n, value_type ch, const allocator_type& alloc);

		basic_string(size_type n, value_type ch, size_type cap,
		             const allocator_type& alloc);

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
		basic_string(std::initializer_list<value_type> il);
#endif

		~basic_string();

		//
		// Assignment operators
		//

		basic_string& operator=(const basic_string& other);
		basic_string& operator=(const const_ref_type& other);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string& operator=(basic_string&& other);
#endif

		basic_string& operator=(const_pointer str);
		basic_string& operator=(value_type ch);

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
		basic_string& operator=(std::initializer_list<value_type> il);
#endif

		//
		// Iterators
		//

		iterator begin() BOOST_NOEXCEPT;
		const_iterator begin() const BOOST_NOEXCEPT;
		const_iterator cbegin() const BOOST_NOEXCEPT;

		iterator end() BOOST_NOEXCEPT;
		const_iterator end() const BOOST_NOEXCEPT;
		const_iterator cend() const BOOST_NOEXCEPT;

		reverse_iterator rbegin() BOOST_NOEXCEPT;
		const_reverse_iterator rbegin() const BOOST_NOEXCEPT;
		const_reverse_iterator crbegin() const BOOST_NOEXCEPT;

		reverse_iterator rend() BOOST_NOEXCEPT;
		const_reverse_iterator rend() const BOOST_NOEXCEPT;
		const_reverse_iterator crend() const BOOST_NOEXCEPT;

		//
		// Misc
		//

		operator argument_type() BOOST_NOEXCEPT;
		operator const_argument_type() const BOOST_NOEXCEPT;

		allocator_type& get_allocator();
		const allocator_type& get_allocator() const;

		const_pointer c_str() const BOOST_NOEXCEPT;

		pointer data() BOOST_NOEXCEPT;
		const_pointer data() const BOOST_NOEXCEPT;

		reference operator[](size_type pos) BOOST_NOEXCEPT;
		const_reference operator[](size_type pos) const BOOST_NOEXCEPT;

		reference at(size_type pos) BOOST_NOEXCEPT;
		const_reference at(size_type pos) const BOOST_NOEXCEPT;

		reference front() BOOST_NOEXCEPT;
		const_reference front() const BOOST_NOEXCEPT;

		reference back() BOOST_NOEXCEPT;
		const_reference back() const BOOST_NOEXCEPT;

		size_type size() const BOOST_NOEXCEPT;
		size_type length() const BOOST_NOEXCEPT;
		size_type capacity() const BOOST_NOEXCEPT;

		bool empty() const BOOST_NOEXCEPT;

		//
		// Size management operations (reserve, resize, shrink...)
		//

		void resize(size_type n);
		void resize(size_type n, value_type ch);

		void reserve(size_type minimum = 0);

		void shrink_to_fit();

		void clear() BOOST_NOEXCEPT;
		void leak() BOOST_NOEXCEPT;

		//
		// Assignment functions
		//

		void assign(basic_string&& other);
		void assign(const_pointer src, size_type len);
		void assign(value_type ch, size_type len);

		template <class ForwardIterator>
		void assign(ForwardIterator first, ForwardIterator last);

		//
		// Appending functions
		//

		void append(const_pointer src, size_type len);
		void append(value_type ch, size_type len);

		template <class ForwardIterator>
		void append(ForwardIterator first, ForwardIterator last);

		//
		// Insertion functions
		//

		void insert(size_type position, const_pointer src, size_type len);
		void insert(size_type position, value_type ch, size_type len);

		template <class ForwardIterator>
		void insert(size_type position,
		            ForwardIterator first,
		            ForwardIterator last);

		//
		// Replace functions
		//

		void replace(size_type position, size_type count,
		             const_pointer str, size_type len);

		void replace(size_type position, size_type count,
		             value_type ch, size_type len);

		//
		// Erasure functions
		//

		iterator erase(size_type position, size_type count);

		void pop_back(size_type count = 1);

	private:
		struct Rep
			: ref_type
			, allocator_type
		{
			BOOST_FORCEINLINE Rep()
				: ref_type(skip_initialization)
				, allocator_type()
			{
				// ctor
			}

			BOOST_FORCEINLINE Rep(const allocator_type& alloc)
				: ref_type(skip_initialization)
				, allocator_type(alloc)
			{
				// ctor
			}

			BOOST_FORCEINLINE Rep(allocator_type&& alloc)
				: ref_type(skip_initialization)
				, allocator_type(vstd::move(alloc))
			{
				// ctor
			}

			pointer capacity;
		};

		//
		// Initialization functions
		//

		void Initialize(const_pointer src, size_type len, size_type cap);
		void Initialize(value_type ch, size_type len, size_type cap);

		template <class ForwardIterator>
		void Initialize(ForwardIterator first,
		                ForwardIterator last);

		static size_type GetRecommendedSize(size_type size)
		{ return (size * 2) + 1; }

		pointer Allocate(size_type cap);
		void Deallocate();

		Rep m_rep;
};

template <typename T, class Traits, class Allocator>
struct hash<basic_string<T, Traits, Allocator> >
{
	size_type operator()(const basic_string<T, Traits, Allocator>& s) const
	{
		return detail::GetMurmurHash3(s.data(), s.size());
	}
};

template <typename T, class Traits, class Allocator>
std::size_t hash_value(basic_string<T, Traits, Allocator> const& s)
{
	return detail::GetMurmurHash3(s.data(), s.size());
};


}

#include <vstl/string/basic_string-impl.hpp>

#endif
