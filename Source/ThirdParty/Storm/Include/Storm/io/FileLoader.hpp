//
// Copyright (c) 2012-2014 Tim Niederhausen (tim@rnc-ag.de)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STORM_IO_FILELOADER_HPP
#define STORM_IO_FILELOADER_HPP

#include <storm/Config.hpp>

#if VSTD_HAS_PRAGMA_ONCE
#pragma once
#endif

#include <storm/memory/Allocator.hpp>
#include <storm/io/Config.hpp>
#include <storm/StringFwd.hpp>

#include <boost/system/error_code.hpp>
#include <boost/noncopyable.hpp>

/// @file io/FileLoader.hpp
/// Defines the FileLoader class.
///
/// @addtogroup io Storm I/O
/// The input/output component of the Storm library.
///
/// @{

namespace storm
{

/// A simple class representing a file loaded into memory.
///
/// The FileLoader class loads a complete file into memory and manages
/// the buffer necessary for doing so.
///
/// @par Example
/// @code
/// bsys::error_code ec;
/// FileLoader loader(m_allocator);
///
/// loader.Load("hello_world.txt", ec);
/// if (loader.IsLoaded()) {
///     printf("File size is %u, content ptr is %p",
///            loader.GetSize(), loader.GetData());
/// }
/// @endcode
///
/// @note The FileLoader's buffer is guranteed to be at least as big as
/// the file.
///
/// @note All memory allocated by the FileLoader class is guranteed to be
/// flagged as temporary.
class FileLoader : boost::noncopyable
{
	public:
		/// Construct a new FileLoader
		///
		/// This constructor initializes the FileLoader with
		/// @c allocator.
		///
		/// @param allocator The allocator which should be used to allocate
		/// the memory needed for storing the file's data
		FileLoader(const Allocator& allocator);

		/// Destruct a FileLoader object
		///
		/// This destructor frees all the allocated memory using
		/// the FileLoader's allocator, if necessary.
		~FileLoader();

		/// Load a file into memory.
		///
		/// This function loads a file into memory.
		/// The new memory block allocated for this purpose
		/// is guranteed to be aligned to a multiple of @c alignment.
		///
		/// @param filename A NUL-terminated UTF8 string containing the path
		/// of the file to load.
		///
		/// @param ec The error_code object
		///
		/// @param alignment The alignment of the memory block containing
		/// the file data.
		///
		/// @param flags Additional flags that should be passed to the
		/// allocator.
		///
		/// @warning This function expects an empty FileLoader object
		/// (i.e. no file loaded).
		void Load(const StringRef& filename,
		          bsys::error_code& ec,
		          vstd::size_type alignment = 1,
		          uint32_t flags = AllocationFlags::kTemp);

		/// Get the content of the loaded file.
		///
		/// This function returns a pointer to the data of the loaded file.
		///
		/// @return A pointer to the file's data.
		uint8_t* GetData();

		/// Get the content of the loaded file.
		///
		/// This function returns a pointer to the data of the loaded file.
		///
		/// @return A pointer to the file's data.
		const uint8_t* GetData() const;

		/// Get the size of the loaded file.
		///
		/// This function returns the size of the loaded file,
		/// or zero if no file is loaded (or the loading failed).
		///
		/// @return The loaded file's size.
		vstd::size_type GetSize() const;

		/// Check whether a file was successfully loaded.
		///
		/// This function returns a @c bool, denoting whether the file
		/// was successfully loaded.
		///
		/// @return @c true if a file was successfully loaded, @c false
		/// otherwise.
		bool IsLoaded() const;

		/// Steal the data buffer.
		///
		/// This function transfers the ownership of the data buffer to
		/// the caller.
		void StealBuffer();

	private:
		Allocator m_allocator;
		uint8_t* m_data;
		vstd::size_type m_size;
		bool m_isLoaded;
};

BOOST_FORCEINLINE FileLoader::FileLoader(const Allocator& allocator)
	: m_allocator(allocator)
	, m_data(nullptr)
	, m_size(0)
	, m_isLoaded(false)
{
	// ctor
}

BOOST_FORCEINLINE FileLoader::~FileLoader()
{
	m_allocator.Deallocate(m_data);
}

BOOST_FORCEINLINE uint8_t* FileLoader::GetData()
{ return m_data; }

BOOST_FORCEINLINE const uint8_t* FileLoader::GetData() const
{ return m_data; }

BOOST_FORCEINLINE vstd::size_type FileLoader::GetSize() const
{ return m_size; }

BOOST_FORCEINLINE bool FileLoader::IsLoaded() const
{ return m_isLoaded; }

BOOST_FORCEINLINE void FileLoader::StealBuffer()
{ m_data = nullptr; }

}

/// @}

#endif
