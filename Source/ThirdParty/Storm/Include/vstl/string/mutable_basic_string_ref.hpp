//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_MUTABLEBASICSTRINGREF_HPP
#define VSTL_STRING_MUTABLEBASICSTRINGREF_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/string_assign_mixin.hpp>
#include <vstl/string/string_compare_mixin.hpp>
#include <vstl/string/string_find_mixin.hpp>
#include <vstl/string/mutable_basic_string_ref_fwd.hpp>
#include <vstl/string/basic_string_ref.hpp>
#include <vstl/string/iterator.hpp>
#include <vstl/string/char_traits.hpp>
#include <vstl/iterator/iterator_wrapper.hpp>
#include <vstl/iterator/reverse_iterator.hpp>
#include <vstl/utility/skip_initialization.hpp>
#include <vstl/functional/hash.hpp>

namespace vstd
{

template <typename T, class Traits = char_traits<T> >
class mutable_basic_string_ref : public basic_string_ref<T, Traits>
{
	public:
		typedef mutable_basic_string_ref<T, Traits> string_type;
		typedef basic_string_ref<T, Traits> base_type;
		typedef const base_type& implicit_ref;

		typedef typename base_type::traits_type traits_type;

		typedef typename base_type::value_type value_type;
		typedef typename base_type::difference_type difference_type;

		typedef value_type& reference;
		typedef typename base_type::const_reference const_reference;
		typedef typename base_type::pointer pointer;
		typedef typename base_type::const_pointer const_pointer;

		typedef string_iterator<pointer, traits_type> iterator;
		typedef typename base_type::const_iterator const_iterator;
		typedef vstd::reverse_iterator<iterator> reverse_iterator;
		typedef typename base_type::const_reverse_iterator const_reverse_iterator;

		mutable_basic_string_ref() BOOST_NOEXCEPT;

		explicit mutable_basic_string_ref(skip_initialization_t) BOOST_NOEXCEPT;

		mutable_basic_string_ref(const mutable_basic_string_ref& other);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		mutable_basic_string_ref(mutable_basic_string_ref&& other);
#endif

		mutable_basic_string_ref(pointer str);
		mutable_basic_string_ref(pointer str, size_type n);
		mutable_basic_string_ref(pointer first, pointer last);

		mutable_basic_string_ref(iterator first, iterator last);

		//
		// Assignment operators
		//

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		mutable_basic_string_ref& operator=(mutable_basic_string_ref&& other);
#endif

		mutable_basic_string_ref& operator=(pointer str);

		//
		// Iterators
		//

		iterator begin() const BOOST_NOEXCEPT;
		iterator end() const BOOST_NOEXCEPT;

		reverse_iterator rbegin() const BOOST_NOEXCEPT;
		reverse_iterator rend() const BOOST_NOEXCEPT;

		//
		// Misc
		//

		pointer data() const BOOST_NOEXCEPT;

		reference operator[](size_type pos) const BOOST_NOEXCEPT;

		reference at(size_type pos) const BOOST_NOEXCEPT;

		reference front() const BOOST_NOEXCEPT;
		reference back() const BOOST_NOEXCEPT;

		//
		// Assignment functions
		//

		void assign(mutable_basic_string_ref&& other);
		void assign(pointer first, pointer last);
		void assign(pointer src, size_type len);

		//
		// Erasure functions
		//

		// They just move pointers!
		void pop_front(size_type count = 1);
		void pop_back(size_type count = 1);
};

template <typename T, class Traits>
struct hash<mutable_basic_string_ref<T, Traits> >
{
	size_type operator()(const mutable_basic_string_ref<T, Traits>& s) const
	{
		return detail::GetMurmurHash3(s.data(), s.size());
	}
};

}

#include <vstl/string/mutable_basic_string_ref-impl.hpp>

#endif
