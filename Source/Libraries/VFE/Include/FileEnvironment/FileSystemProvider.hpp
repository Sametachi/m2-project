#pragma once
#include <storm/String.hpp>
#include <memory>

class VSFile;

// The FileSystemProvider provides the actual File System implementation.
class FileSystemProvider
{
public:
	FileSystemProvider(const storm::StringRef& name);
	virtual ~FileSystemProvider() {}
	const storm::StringRef& GetName() const;
	virtual bool DoesFileExist(const storm::String& path) = 0;

	virtual std::unique_ptr<VSFile> OpenFile(std::string_view path, uint32_t flags) = 0;

private:
	storm::StringRef m_name;
};

BOOST_FORCEINLINE FileSystemProvider::FileSystemProvider(const storm::StringRef& name): m_name(name) {}
BOOST_FORCEINLINE const storm::StringRef& FileSystemProvider::GetName() const { return m_name; }