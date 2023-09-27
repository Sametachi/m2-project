//
// Copyright (c) 2012-2013 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef VSTL_CONFIG_CPP11_HPP
#define VSTL_CONFIG_CPP11_HPP

#include <vstl/config/config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

// TODO(tim): Remove this. Boost provides a better emulation...
#if !defined(BOOST_NO_CXX11_SCOPED_ENUMS) || defined(DOXYGEN)
	#define VSTD_SCOPED_ENUM_BEGIN(name) enum class name
	#define VSTD_SCOPED_ENUM_END(name)
#else
	#define VSTD_SCOPED_ENUM_BEGIN(name) struct name { enum enum_type
	#define VSTD_SCOPED_ENUM_END(name) \
		BOOST_FORCEINLINE name(enum_type value) BOOST_NOEXCEPT : m_value(value) {} \
		BOOST_FORCEINLINE explicit name(int value) BOOST_NOEXCEPT : m_value(static_cast<enum_type>(value)) {} \
		BOOST_FORCEINLINE operator int() const BOOST_NOEXCEPT { return m_value; } \
		enum_type m_value; \
	};
#endif

#endif
