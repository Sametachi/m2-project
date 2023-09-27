# VFE cmake file
set(INC
Include/FileEnvironment/FileSystem.hpp
Include/FileEnvironment/FileSystemProvider.hpp
Include/FileEnvironment/VirtualDiskFile.hpp
Include/FileEnvironment/VirtualDiskProvider.hpp
Include/FileEnvironment/VSFile.hpp
Include/FileSystem/ArchiveFile.hpp
Include/FileSystem/ArchiveFormat.hpp
Include/FileSystem/ArchiveFSProvider.hpp
Include/FileSystem/FilenameWrapper.hpp
Include/FileSystem/FSArchive.hpp
Include/MetinDefault/CRC32.hpp
Include/MetinDefault/FileBase.hpp
Include/MetinDefault/FileDir.hpp
Include/MetinDefault/FileLoader.hpp
Include/MetinDefault/MappedFile.hpp
Include/MetinDefault/XTea.hpp
Include/Modules/xorstr.hpp
Include/Modules/xor_buffer.hpp
Include/ResourceManager/ReferenceCounter.hpp
Include/ResourceManager/Resource.hpp
Include/ResourceManager/ResourceManager.hpp
Include/StdAfx.hpp
Include/VFE.hpp
#Include/FileSystem/EterPackProvider.hpp
)

set(SRC
Source/BuildCheck.cpp
Source/FileEnvironment/FileSystem.cpp
Source/FileEnvironment/VirtualDiskFile.cpp
Source/FileEnvironment/VirtualDiskProvider.cpp
Source/FileSystem/ArchiveFile.cpp
Source/FileSystem/ArchiveFSProvider.cpp
Source/FileSystem/FilenameWrapper.cpp
Source/FileSystem/FSArchive.cpp
Source/MetinDefault/CRC32.cpp
Source/MetinDefault/FileBase.cpp
Source/MetinDefault/FileDir.cpp
Source/MetinDefault/FileLoader.cpp
Source/MetinDefault/MappedFile.cpp
Source/MetinDefault/XTea.cpp
Source/ResourceManager/Resource.cpp
Source/ResourceManager/ResourceManager.cpp
#Source/FileSystem/EterPackProvider.cpp
)

add_library(VFE STATIC ${SRC} ${INC})
set_target_properties(VFE PROPERTIES FOLDER "Libraries/Common")
target_link_libraries(VFE PUBLIC Storm YitsoraBasic)
target_include_directories(VFE PUBLIC Include)
set(LINK_DEPS xxHash lz4)
auto_dep(VFE PUBLIC)
