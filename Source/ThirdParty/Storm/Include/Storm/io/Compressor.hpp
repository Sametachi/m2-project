//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_COMPRESSOR_HPP
#define STORM_IO_COMPRESSOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file io/Compressor.hpp
/// Defines the Compressor class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// Abstract base class for all compression engines.
///
/// This class is the base class of all compression engines.
/// An engine is required to implement all the pure virtual methods
/// specified below.
class Compressor
{
	public:
		/// Destruct a Compressor.
		///
		/// This function destructs the Compressor
		/// and frees all associated resources.
		virtual ~Compressor() {}

		/// Reset the engine.
		///
		/// This function resets the engine in such a way,
		/// that it can be used to compress another block of memory
		/// independently.
		///
		/// @param state The state to reset.
		virtual void Reset() = 0;

		/// Get the max. size of the compressed data.
		///
		/// This function returns the maximum size incompressible data could
		/// occupy.
		///
		/// @param size The source size.
		///
		/// @return The max. size the compressed data could occupy.
		virtual vstd::size_type GetMaxCompressedSize(vstd::size_type size) = 0;

		/// Compress a block of memory.
		///
		/// This function compresses a block of memory using the given state.
		///
		/// @param state The state to use
		///
		/// @param input The input data.
		///
		/// @param size The size (in bytes) of the input data.
		///
		/// @param output The output buffer.
		///
		/// @param outSize A reference to a vstd::size_type containing
		/// the size of the output buffer. Upon successful compression
		/// @c outSize is set to the size of the compressed data.
		///
		/// @return A bool denoting whether the data was
		/// successfully compressed.
		virtual bool Compress(const void* input, vstd::size_type size,
		                      void* output, vstd::size_type& outSize) = 0;
};

}

/// @}

#endif
