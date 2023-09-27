//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_MATH_XORSHIFTGENERATOR_HPP
#define STORM_MATH_XORSHIFTGENERATOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/Assert.hpp>

/// @file math/XorshiftGenerator.hpp
/// Defines some stuff needed for Eigen and includes the Eigen main header.
///
/// @addtogroup math Storm Mathematics
/// Storm's mathematics library features mostly simple linear
/// algebra stuff needed for game development.
///
/// @{

namespace storm
{

/// A Xorshift 128-bit random number generator.
///
/// This class implements the Xorshift random number generator using
/// a 128-bit state.
///
/// @sa http://de.wikipedia.org/wiki/Xorshift
class XorshiftGenerator
{
	public:
		/// Construct a new Xorshift generator.
		///
		/// This constructor initializes the internal state
		/// to the given values.
		///
		/// @param x Part of the generator's initial state.
		///
		/// @param y Part of the generator's initial state.
		///
		/// @param z Part of the generator's initial state.
		///
		/// @param w Part of the generator's initial state.
		XorshiftGenerator(uint32_t x = 123456789, uint32_t y = 362436069,
		                  uint32_t z = 521288629, uint32_t w = 88675123);

		/// Set the internal RNG state.
		///
		/// This function sets the Xorshift generator's internal state
		/// to the given values.
		void SetState(uint32_t x, uint32_t y, uint32_t z, uint32_t w);

		/// Get a new random value.
		///
		/// This function modifies the internal state and returns
		/// a new random value.
		///
		/// @return A new random value
		uint32_t GetNextRandom();

	private:
		uint32_t m_x;
		uint32_t m_y;
		uint32_t m_z;
		uint32_t m_w;
};

BOOST_FORCEINLINE XorshiftGenerator::XorshiftGenerator(
		uint32_t x, uint32_t y, uint32_t z, uint32_t w)
	: m_x(x)
	, m_y(y)
	, m_z(z)
	, m_w(w)
{
	// ctor
}

BOOST_FORCEINLINE void XorshiftGenerator::SetState(
		uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_w = w;
}

uint32_t XorshiftGenerator::GetNextRandom()
{
	const uint32_t t = m_x ^ (m_x << 11);

	m_x = m_y;
	m_y = m_z;
	m_z = m_w;
	m_w ^= (m_w >> 19) ^ t ^ (t >> 8);

	return w;
}

}

/// @}

#endif
