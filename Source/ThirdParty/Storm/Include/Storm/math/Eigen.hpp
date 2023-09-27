//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MATH_EIGEN_HPP
#define STORM_MATH_EIGEN_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file math/Eigen.hpp
/// Defines some stuff needed for Eigen and includes the Eigen main header.
///
/// @addtogroup math Storm Mathematics
/// Storm's mathematics library features mostly simple linear
/// algebra stuff needed for game development.
///
/// @{

#include <Eigen/Core>
#ifndef eigen_assert
#ifndef EIGEN_NO_ASSERTION_CHECKING
	#define eigen_assert(expr) STORM_ASSERT(expr, "eigen_assert(expr)")
#endif
#endif

#if STORM_DEBUG
	#define EIGEN_INTERNAL_DEBUGGING
#endif


namespace storm
{

#define STORM_IMPORT_EIGEN_TYPES(Type, TypeSuffix, Size, SizeSuffix)   \
using Eigen::Matrix##SizeSuffix##TypeSuffix;  \
using Eigen::Vector##SizeSuffix##TypeSuffix;  \
using Eigen::RowVector##SizeSuffix##TypeSuffix;

#define STORM_IMPORT_FIXED_EIGEN_TYPES(Type, TypeSuffix, Size) \
using Eigen::Matrix##Size##X##TypeSuffix; \
using Eigen::Matrix##X##Size##TypeSuffix;

#define STORM_IMPORT_EIGEN_TYPES_ALL(Type, TypeSuffix) \
STORM_IMPORT_EIGEN_TYPES(Type, TypeSuffix, 2, 2) \
STORM_IMPORT_EIGEN_TYPES(Type, TypeSuffix, 3, 3) \
STORM_IMPORT_EIGEN_TYPES(Type, TypeSuffix, 4, 4) \
STORM_IMPORT_EIGEN_TYPES(Type, TypeSuffix, Dynamic, X) \
STORM_IMPORT_FIXED_EIGEN_TYPES(Type, TypeSuffix, 2) \
STORM_IMPORT_FIXED_EIGEN_TYPES(Type, TypeSuffix, 3) \
STORM_IMPORT_FIXED_EIGEN_TYPES(Type, TypeSuffix, 4)

STORM_IMPORT_EIGEN_TYPES_ALL(int, i)
STORM_IMPORT_EIGEN_TYPES_ALL(float, f)
STORM_IMPORT_EIGEN_TYPES_ALL(double, d)
STORM_IMPORT_EIGEN_TYPES_ALL(std::complex<float>, cf)
STORM_IMPORT_EIGEN_TYPES_ALL(std::complex<double>, cd)

#undef STORM_IMPORT_EIGEN_TYPES_ALL
#undef STORM_IMPORT_EIGEN_TYPES
#undef STORM_IMPORT_FIXED_EIGEN_TYPES

}

/// @}

#endif
