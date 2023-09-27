//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_DECOMPRESSOR_HPP
#define STORM_IO_DECOMPRESSOR_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file io/Decompressor.hpp
/// Defines the Decompressor class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// Abstract base class for all decompression engines.
///
/// This class is the base class of all decompression engines.
/// An engine is required to implement all the pure virtual methods
/// specified below.
class Decompressor
{
	public:
		/// Destruct a Decompressor.
		///
		/// This function destructs the Decompressor
		/// and frees all associated resources.
		virtual ~Decompressor() {}

		/// Reset the engine.
		///
		/// This function resets the engine in such a way,
		/// that it can be used to decompress another block of memory
		/// independently.
		///
		/// @param state The state to reset.
		virtual void Reset() = 0;

		/// Decompress a block of memory.
		///
		/// This function decompresses a block of memory.
		///
		/// @param input The input data.
		///
		/// @param size The size (in bytes) of the input data.
		///
		/// @param output The output buffer.
		///
		/// @param outSize The size of the output buffer.
		///
		/// @return A bool denoting whether the data was
		/// successfully decompressed.
		virtual bool Decompress(const void* input, vstd::size_type size,
		                        void* output, vstd::size_type& outSize) = 0;
};

}

/// @}

#endif
