//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_ENCRYPTER_HPP
#define STORM_IO_ENCRYPTER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

/// @file io/Encrypter.hpp
/// Defines the Encrypter class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// Abstract base class for all encryption engines.
///
/// This class is the base class of all encryption engines.
/// An engine is required to implement all the pure virtual methods
/// specified below.
class Encrypter
{
	public:
		/// Destruct a Encrypter.
		///
		/// This function destructs the Encrypter
		/// and frees all associated resources.
		virtual ~Encrypter() {}

		/// Reset the engine.
		///
		/// This function resets the engine in such a way,
		/// that it can be used to encrypt another block of memory
		/// independently.
		///
		/// @param state The state to reset.
		virtual void Reset() = 0;

		/// Update the engine's key.
		///
		/// This function update's the engine's key.
		///
		/// @param privateData The private data that should be used to
		/// generate the key.
		///
		/// @param privateDataSize Size of the given private data.
		virtual void UpdateKey(const void* privateData,
		                       vstd::size_type privateDataSize) = 0;

		/// Encrypt a block of memory.
		///
		/// This function encrypts a block of memory.
		/// Input and output buffers are allowed to be the same.
		///
		/// @param state The state to use
		///
		/// @param input The input data.
		///
		/// @param output The output buffer.
		///
		/// @param size The size (in bytes) of the data.
		///
		/// @param flush Indicates whether this call is the last before
		/// the engine state is reset.
		virtual void Encrypt(const void* input,
		                     void* output,
		                     vstd::size_type size,
		                     bool flush) = 0;
};

}

/// @}

#endif
