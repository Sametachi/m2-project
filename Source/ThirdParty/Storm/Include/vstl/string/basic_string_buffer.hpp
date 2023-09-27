//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRINGBUFFER_HPP
#define VSTL_STRING_BASICSTRINGBUFFER_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/string_append_mixin.hpp>
#include <vstl/string/string_assign_mixin.hpp>
#include <vstl/string/string_insert_mixin.hpp>
#include <vstl/string/string_erase_mixin.hpp>
#include <vstl/string/string_replace_mixin.hpp>
#include <vstl/string/string_compare_mixin.hpp>
#include <vstl/string/string_find_mixin.hpp>
#include <vstl/string/basic_string_buffer_fwd.hpp>
#include <vstl/string/basic_string_ref.hpp>
#include <vstl/string/mutable_basic_string_ref.hpp>
#include <vstl/string/iterator.hpp>
#include <vstl/string/char_traits.hpp>
#include <vstl/iterator/iterator_wrapper.hpp>
#include <vstl/iterator/reverse_iterator.hpp>
#include <vstl/utility/skip_initialization.hpp>
#include <vstl/functional/hash.hpp>

namespace vstd
{

template <typename T, class Traits = char_traits<T> >
class basic_string_buffer
#define VSTD_MAKE_STRING_MIXIN(name) name <basic_string_buffer<T, Traits> >
	: public VSTD_MAKE_STRING_MIXIN(string_append_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_basic_assign_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_ext_assign_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_insert_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_erase_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_replace_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_compare_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_find_mixin)
#undef VSTD_MAKE_STRING_MIXIN
{
	public:
		typedef basic_string_buffer<T, Traits> string_type;

		typedef mutable_basic_string_ref<T, Traits> ref_type;
		typedef basic_string_ref<T, Traits> const_ref_type;

		typedef const ref_type& argument_type;
		typedef const const_ref_type& const_argument_type;

		typedef Traits traits_type;

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

		basic_string_buffer() BOOST_NOEXCEPT;

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string_buffer(basic_string_buffer&& other);
#endif

		basic_string_buffer(pointer str, size_type n, size_type cap);
		basic_string_buffer(pointer first, pointer last);

		basic_string_buffer(iterator first, iterator last);

		//
		// Assignment operators
		//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string_buffer& operator=(basic_string_buffer&& other);
#endif

		basic_string_buffer& operator=(const_pointer str);
		basic_string_buffer& operator=(value_type ch);

#ifndef BOOST_NO_CXX11_HDR_INITIALIZER_LIST
		basic_string_buffer& operator=(std::initializer_list<value_type> il);
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

		void clear() BOOST_NOEXCEPT;
		void leak() BOOST_NOEXCEPT;

		//
		// Assignment functions
		//

		void assign(basic_string_buffer&& other);
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
		struct Rep : public ref_type
		{
			BOOST_FORCEINLINE Rep()
				: ref_type(skip_initialization)
			{
				// ctor
			}

			pointer capacity;
		};

		Rep m_rep;
};

template <typename T, class Traits>
struct hash<basic_string_buffer<T, Traits> >
{
	size_type operator()(const basic_string_buffer<T, Traits>& s) const
	{
		return detail::GetMurmurHash3(s.data(), s.size());
	}
};

}

#include <vstl/string/basic_string_buffer-impl.hpp>

#endif
