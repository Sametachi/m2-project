//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_STRING_BASICSTRINGREF_HPP
#define VSTL_STRING_BASICSTRINGREF_HPP

#include <vstl/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <vstl/string/string_assign_mixin.hpp>
#include <vstl/string/string_compare_mixin.hpp>
#include <vstl/string/string_find_mixin.hpp>
#include <vstl/string/basic_string_ref_fwd.hpp>
#include <vstl/string/iterator.hpp>
#include <vstl/string/char_traits.hpp>
#include <vstl/string/basic_string.hpp>
#include <vstl/memory/allocator.hpp>

#include <vstl/iterator/iterator_wrapper.hpp>
#include <vstl/iterator/reverse_iterator.hpp>
#include <vstl/utility/skip_initialization.hpp>
#include <vstl/functional/hash.hpp>
#include <string_view>
#include <string>

namespace vstd
{

template <typename T, class Traits = char_traits<T> >
class basic_string_ref
#define VSTD_MAKE_STRING_MIXIN(name) name <basic_string_ref<T, Traits> >
	: public VSTD_MAKE_STRING_MIXIN(string_basic_assign_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_compare_mixin)
	, public VSTD_MAKE_STRING_MIXIN(string_find_mixin)
#undef VSTD_MAKE_STRING_MIXIN
{
	public:
		typedef basic_string_ref<T, Traits> string_type;

		typedef const basic_string_ref<T, Traits>& const_argument_type;

		typedef Traits traits_type;

		typedef typename traits_type::char_type value_type;
		typedef ptrdiff_t difference_type;

		typedef const value_type& const_reference;
		typedef value_type* pointer; // Needed internally
		typedef const value_type* const_pointer;

		typedef string_iterator<const_pointer, traits_type> const_iterator;
		typedef vstd::reverse_iterator<const_iterator> const_reverse_iterator;

		static BOOST_CONSTEXPR_OR_CONST size_type npos =
			static_cast<size_type>(-1);

		basic_string_ref() BOOST_NOEXCEPT;

		explicit basic_string_ref(skip_initialization_t) BOOST_NOEXCEPT;

		basic_string_ref(const basic_string_ref& other);
		basic_string_ref(const basic_string_ref& other, size_type pos,
						size_type n = npos);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string_ref(basic_string_ref&& other);
#endif

		basic_string_ref(const_pointer str);
		basic_string_ref(std::string str);
		basic_string_ref(std::string_view str);
		basic_string_ref(const_pointer str, size_type n);
		basic_string_ref(const_pointer first, const_pointer last);

		basic_string_ref(const_iterator first, const_iterator last);

		//
		// Assignment operators
		//

		basic_string_ref& operator=(const basic_string_ref& other);

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
		basic_string_ref& operator=(basic_string_ref&& other);
#endif

		basic_string_ref& operator=(const_pointer str);

		//
		// Iterators
		//

		const_iterator begin() const BOOST_NOEXCEPT;
		const_iterator cbegin() const BOOST_NOEXCEPT;

		const_iterator end() const BOOST_NOEXCEPT;
		const_iterator cend() const BOOST_NOEXCEPT;

		const_reverse_iterator rbegin() const BOOST_NOEXCEPT;
		const_reverse_iterator crbegin() const BOOST_NOEXCEPT;

		const_reverse_iterator rend() const BOOST_NOEXCEPT;
		const_reverse_iterator crend() const BOOST_NOEXCEPT;

		//
		// Misc
		//

		const_pointer data() const BOOST_NOEXCEPT;

		const_reference operator[](size_type pos) const BOOST_NOEXCEPT;
		const_reference at(size_type pos) const BOOST_NOEXCEPT;

		const_reference front() const BOOST_NOEXCEPT;
		const_reference back() const BOOST_NOEXCEPT;

		size_type size() const BOOST_NOEXCEPT;
		size_type length() const BOOST_NOEXCEPT;
		size_type capacity() const BOOST_NOEXCEPT;

		bool empty() const BOOST_NOEXCEPT;

		//
		// Size management operations (reserve, resize, shrink...)
		//

		void clear() BOOST_NOEXCEPT;
		void leak() BOOST_NOEXCEPT;

		//
		// Assignment functions
		//

		void assign(basic_string_ref&& other);
		void assign(const_pointer src, size_type len);

		//
		// Erasure functions
		//

		// They just move pointers!
		void pop_front(size_type count = 1);
		void pop_back(size_type count = 1);

		// Helper

		std::string to_std_string();
		
		basic_string<T, Traits, allocator> to_string() const
		{
			return basic_string<T, Traits, allocator>(data(), size());
		}


		template<typename OStream>
		friend OStream &operator<<(OStream &os, const basic_string_ref &c)
		{
			return os << c.data();
		}


		//
		// Members
		//

		pointer start;
		pointer finish;
};

template <typename T, class Traits>
struct hash<basic_string_ref<T, Traits> >
{
	size_type operator()(const basic_string_ref<T, Traits>& s) const
	{
		return detail::GetMurmurHash3(s.data(), s.size());
	}
};
template <typename T, class Traits>

    std::size_t hash_value(basic_string_ref<T, Traits> const& s)
    {
		return detail::GetMurmurHash3(s.data(), s.size());
    }

}

#include <vstl/string/basic_string_ref-impl.hpp>

#endif
