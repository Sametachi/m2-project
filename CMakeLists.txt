# Main cmake processor
cmake_minimum_required(VERSION 3.20)
project(SecretProject LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_LIST_DIR}/Source/Cmake")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(BINARY_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Files/Client)
set(SERVER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Files/Server)
set(BUILD_PERMISSIVE TRUE)
set(CMAKE_BASE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Source/Cmake)
set(CLIENT_BASE_REVISION 28249)
set(SERVER_BASE_REVISION 40267)
set(V_SERVER_NUMBER 0)
set(V_SERVER_NAME "Metin2")
set(CRASHPAD_HANDLER_NAME "CrashpadHandler")
set(CRASHPAD_URL "https://127.0.0.1/crashpad.php")

option(BUILD_NO_SERVER "Disable building of the server" OFF)
option(BUILD_NO_CLIENT "Disable building of the client" OFF)

# 0. Include cmake rules
include(Directives)
include(Dependencies)
include(Pch)
include(GitWc)
include(MultiTarget)
include(ver/Version)

# 1. Common dependencies
find_package(Git) # used for version generation
include(DirectX) # DirectX deps search
find_dep(NAME DevIL)
find_dep(NAME lzo2)
find_dep(NAME Boost COMPONENTS System Program_Options FileSystem Locale TARGET Boost::boost)
find_dep(NAME fmt TARGET fmt::fmt DEBUG_POSTFIX d)
find_dep(NAME spdlog TARGET spdlog::spdlog DEBUG_POSTFIX d)
find_dep(NAME ZLIB TARGET ZLIB::ZLIB DEBUG_POSTFIX d)
find_dep(NAME xxHash TARGET xxHash::xxhash)
find_dep(NAME lz4 TARGET lz4::lz4 DEBUG_POSTFIX d)
find_dep(NAME G3LOG TARGET g3log)
find_dep(NAME GLM TARGET glm::glm)
find_dep(NAME FREETYPE TARGET freetype)
find_dep(NAME TWEENY TARGET tweeny)
find_dep(NAME cxxopts TARGET cxxopts::cxxopts)
find_dep(NAME OpenSSL TARGET "OpenSSL::SSL;OpenSSL::Crypto")

## devil deps
find_dep(NAME PNG TARGET PNG::PNG DEBUG_POSTFIX d)
find_dep(NAME tiff DEBUG_POSTFIX d)
find_dep(NAME lcms2 TARGET lcms2::lcms2)
find_dep(NAME JPEG)
find_dep(NAME jasper DEBUG_POSTFIX d)
find_dep(NAME LibLZMA TARGET LibLZMA::LibLZMA)
find_dep(NAME IlmBase TARGET "IlmBase::Iex;IlmBase::Half;IlmBase::Imath;IlmBase::IexMath")
find_dep(NAME OpenEXR TARGET "OpenEXR::IlmImf;OpenEXR::IlmImfUtil;OpenEXR::IlmImfConfig")

if (NOT CFG_NO_PTHREADS)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_dep(NAME Threads TARGET Threads::Threads)
endif()

# We need to find the interpreter first because vcpkg will override it
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if (NOT Python3_Interpreter_FOUND)
    message(FATAL_ERROR "A python3 interpreter must be installed on the computer")
endif()
set(PYTHON3_REAL_INTERPRETER ${Python3_EXECUTABLE})
unset(Python3_FOUND CACHE)

# 2. Client only deps
if (NOT BUILD_NO_CLIENT)
    find_dep(NAME Python3 COMPONENTS Development TARGET Python3::Python DEBUG_POSTFIX _d)
    find_dep(NAME pybind11 TARGET "pybind11::embed;pybind11::module")
    find_dep(NAME libffi TARGET libffi)
    find_dep(NAME expat TARGET expat::expat)
    find_dep(NAME crashpad TARGET "crashpad::crashpad;crashpad")
endif()

# 3. Server only deps
if (NOT BUILD_NO_SERVER)
    if (CFG_VCPKG)
        find_dep(NAME unofficial-libmariadb TARGET unofficial::libmariadb)
    else()
        find_dep(NAME libmariadb)
    endif()
endif()

# 4. Extra operations
if (Git_FOUND)
    GIT_WC_INFO(${CMAKE_CURRENT_LIST_DIR} GIT)
endif()

if (BUILD_STATIC)
    add_definitions(-DIL_STATIC_LIB=1)
endif()

# 5. Add source directory
add_subdirectory(Source)

# 6. Add files
add_subdirectory(Files/SetupFiles)

