#pragma once
#include <storm/io/View.hpp>

// An opened VFS file.
// This class represents the user-visible interface for an opened file. 
// It provides several methods to read the file's (meta)data just like in Unreal Engine.
class VSFile
{
public:
	virtual ~VSFile() {}
	virtual uint32_t GetSize() const = 0;
	virtual bool Read(uint32_t offset, void* buffer, uint32_t bytes) const = 0;
	virtual bool GetView(uint32_t offset, storm::View& view, uint32_t bytes) const = 0;
};