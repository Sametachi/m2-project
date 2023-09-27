//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_ABSTRACTFACTORY_HPP
#define STORM_ABSTRACTFACTORY_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file AbstractFactory.hpp
/// Defines Storm's AbstractFactory class.

namespace storm
{

/// Abstract base class for single-object factories.
///
/// This class is a simple base class for abstract factories
/// that only need to create/destroy a single object.
template <class T>
class AbstractFactory
{
	public:
		/// Create a new instance of @c T.
		///
		/// This function creates a new instance of T.
		/// The way this instance is created is unspecified.
		///
		/// @return A fully constructed instance of T.
		virtual T* Create() = 0;

		/// Destroy an instance of T.
		///
		/// This function destroys an instance of T.
		/// The object has to be created by this factory
		/// instance.
		///
		/// @param object The instance to destroy.
		virtual void Destroy(T* object) = 0;
};

}

#endif
