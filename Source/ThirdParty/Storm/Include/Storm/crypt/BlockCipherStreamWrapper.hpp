//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_CRYPT_BLOCKCIPHERSTREAMWRAPPER_HPP
#define STORM_CRYPT_BLOCKCIPHERSTREAMWRAPPER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <cstring>

/// @file storm/crypt/BlockCipherStreamWrapper.hpp
/// Defines BlockCipherStreamWrapper class.

namespace storm
{

/// A stream wrapper around a block cipher.
///
/// This class wraps a block cipher and enables processing data
/// whose size is not a multiple of the block size.
template <vstd::size_type BlockSize>
class BlockCipherStreamWrapper
{
	public:
		static const vstd::size_type kBlockSize = BlockSize;

		/// Construct a new BlockCipherStreamWrapper.
		///
		/// This constructor zero-initializes the BlockCipherStreamWrapper.
		BlockCipherStreamWrapper();

		/// Reset the BlockCipherStreamWrapper.
		///
		/// This function resets the BlockCipherStreamWrapper to the
		/// zero-initialized state it had after construction.
		void Reset();

		/// Process data.
		///
		/// This function uses the given block cipher function @c func to
		/// process blocks of @c BlockSize size. If the source buffer size
		/// isn't a multiply of the block size, the remaining data will be
		/// cached and flushed to the destination buffer once the function
		/// is called again and there's enough data available.
		/// In flush mode the last partial block is just copied as-is.
		///
		/// @param func The cipher function to use. It's signature is
		/// required to be compatible to the following:
		/// @code
		/// void (void* dst, const void* src,
		///       const void* key, vstd::size_type size)
		/// @endcode
		///
		/// @param src The source data.
		///
		/// @param dst The destination buffer.
		///
		/// @param key The key to use.
		///
		/// @param size The size of the source data.
		///
		/// @param flush Tells the function to not cache partial blocks
		/// anymore, because this is the last time this function is called
		/// before being resetted via Reset().
		///
		/// @return The number of bytes written to the output buffer.
		/// Depending on the input size and number of cached bytes, this
		/// value might be more or less than the input size.
		template <typename Func>
		vstd::size_type Process(Func func, const void* src, void* dst,
		                        const void* key, vstd::size_type size,
		                        bool flush);

	private:
		vstd::size_type m_buffered;
		uint8_t m_buffer[BlockSize];
};

template <vstd::size_type BlockSize>
BlockCipherStreamWrapper<BlockSize>::BlockCipherStreamWrapper()
	: m_buffered(0)
{
	// ctor
}

template <vstd::size_type BlockSize>
void BlockCipherStreamWrapper<BlockSize>::Reset()
{
	m_buffered = 0;
}

template <vstd::size_type BlockSize>
template <typename Func>
vstd::size_type BlockCipherStreamWrapper<BlockSize>::
	Process(Func func, const void* in, void* out, const void* key,
			vstd::size_type size, bool flush)
{
	const uint8_t* src = static_cast<const uint8_t*>(in);
	uint8_t* dst = static_cast<uint8_t*>(out);

	vstd::size_type processed = 0;
	if (m_buffered > 0) {
		STORM_ASSERT(m_buffered < BlockSize,
					"partial-block buffer cannot contain a full block");

		const vstd::size_type needed = BlockSize - m_buffered;
		if (size >= needed) {
			std::memcpy(m_buffer + m_buffered, src, needed);

			src += needed;
			size -= needed;

			func(dst, m_buffer, key, BlockSize);

			dst += BlockSize;
			processed += BlockSize;
		} else {
			std::memcpy(m_buffer + m_buffered, src, size);
			m_buffered += size;

			if (flush) {
				std::memcpy(dst, m_buffer, m_buffered);
				return m_buffered;
			}

			return 0;
		}
	}

	const vstd::size_type remaining = size % BlockSize;
	const vstd::size_type toProcess = size - remaining;

	if (toProcess) {
		func(dst, src, key, toProcess);

		src += toProcess;
		dst += toProcess;
		processed += toProcess;
	}

	if (remaining) {
		if (flush) {
			if (src != dst)
				std::memcpy(dst, src, remaining);

			processed += remaining;
		} else {
			std::memcpy(m_buffer, src, remaining);
			m_buffered = remaining;
		}
	}

	return processed;
}

}

#endif
