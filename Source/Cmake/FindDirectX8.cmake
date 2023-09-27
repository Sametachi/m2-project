# - try to find part of DirectX 8 SDK
#
# Cache Variables: (probably not for direct use in your scripts)
#  DIRECTX8_INCLUDE_DIR
#
# Variables you should use in your CMakeLists.txt:
#  DIRECTX8_DXGUID_LIBRARY - deprecated, see below
#  DIRECTX8_DXERR_LIBRARY - deprecated, see http://blogs.msdn.com/b/chuckw/archive/2012/04/24/where-s-dxerr-lib.aspx
#  DIRECTX8_DINPUT_LIBRARY
#  DIRECTX8_DINPUT_INCLUDE_DIR
#  DIRECTX8_D3D8_LIBRARY
#  DIRECTX8_D3DXOF_LIBRARY
#  DIRECTX8_D3DX_LIBRARIES
#  DIRECTX8_INCLUDE_DIRS
#  DIRECTX8_FOUND - if this is not true, do not attempt to use this library
#
# Defines these macros:
#  find_directx_include - wrapper for find_path that provides PATHS, HINTS, and PATH_SUFFIXES.
#  find_directx_library - wrapper for find_library that provides PATHS, HINTS, and PATH_SUFFIXES.
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#  SelectLibraryConfigurations
#
# Original Author:
# 2012 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright 2012, Iowa State University
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
# SPDX-License-Identifier: BSL-1.0


set(DIRECTX8_ROOT_DIR
	"${DIRECTX8_ROOT_DIR}"
	CACHE
	PATH
	"Root directory to search for DirectX")

if(MSVC)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		message(FATAL_ERROR "DirectX8 does not support 64-bit")
	else()
		set(_dx_lib_suffixes lib lib)
	endif()
	set(DXSDK_DIRS)

	file(TO_CMAKE_PATH "$ENV{DX8SDK_DIR}" ENV_DXSDK_DIR)
	if(ENV_DXSDK_DIR)
		list(APPEND DXSDK_DIRS ${ENV_DXSDK_DIR})
	endif()
else()
	set(_dx_lib_suffixes lib)
	set(DXSDK_DIRS /mingw)
endif()

find_path(DIRECTX8_INCLUDE_DIR
	NAMES
	dinput.h
	dxerr8.h
	PATHS
	${DXSDK_DIRS}
	HINTS
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	include)
find_path(DIRECTX8_DINPUT_INCLUDE_DIR
	NAMES
	dinput.h
	PATHS
	${DXSDK_DIRS}
	HINTS
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	include)

set(DXLIB_HINTS)

find_library(DIRECTX8_DXGUID_LIBRARY
	NAMES
	dxguid
	PATHS
	${DXLIB_HINTS}
	${DXSDK_DIRS}
	HINTS
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

if(DIRECTX8_DXGUID_LIBRARY)
	get_filename_component(_dxsdk_lib_dir ${DIRECTX8_DXGUID_LIBRARY} PATH)
	list(APPEND DXLIB_HINTS "${_dxsdk_lib_dir}")
endif()

find_library(DIRECTX8_DINPUT_LIBRARY
	NAMES
	dinput8
	dinput
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

find_library(DIRECTX8_DXERR_LIBRARY
	NAMES
	dxerr
	dxerr8
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

find_library(DIRECTX8_D3D8_LIBRARY
	NAMES
	d3d8
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

find_library(DIRECTX8_D3DXOF_LIBRARY
	NAMES
	d3dxof
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

find_library(DIRECTX8_D3DX8_LIBRARY_RELEASE
	NAMES
	d3dx8
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

find_library(DIRECTX8_D3DX8_LIBRARY_DEBUG
	NAMES
	d3dx8
	PATHS
	${DXSDK_DIRS}
	HINTS
	${DXLIB_HINTS}
	"${DIRECTX8_ROOT_DIR}"
	PATH_SUFFIXES
	${_dx_lib_suffixes})

include(SelectLibraryConfigurations)
select_library_configurations(DIRECTX8_D3DX8)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DirectX8
	DEFAULT_MSG
	DIRECTX8_DXGUID_LIBRARY
	DIRECTX8_DINPUT_LIBRARY
	DIRECTX8_INCLUDE_DIR)

if(DIRECTX8_FOUND)
	set(DIRECTX8_LIBRARIES
		"${DIRECTX8_DXGUID_LIBRARY}"
		"${DIRECTX8_DINPUT_LIBRARY}"
		"${DIRECTX8_D3DXOF_LIBRARY}"
		"${DIRECTX8_D3D8_LIBRARY}")

	set(DIRECTX8_INCLUDE_DIRS "${DIRECTX8_INCLUDE_DIR}")

	mark_as_advanced(DIRECTX8_ROOT_DIR)
endif()

macro(find_directx8_library)
	find_library(${ARGN}
		PATHS
		${DXSDK_DIRS}
		HINTS
		${DXLIB_HINTS}
		"${DIRECTX8_ROOT_DIR}"
		PATH_SUFFIXES
		${_dx_lib_suffixes})
endmacro()
macro(find_directx8_include)
	find_path(${ARGN}
		PATHS
		${DXSDK_DIRS}
		HINTS
		${DIRECTX_INCLUDE_DIR}
		"${DIRECTX8_ROOT_DIR}"
		PATH_SUFFIXES
		include)
endmacro()

mark_as_advanced(DIRECTX8_DINPUT_LIBRARY_DEBUG
	DIRECTX8_DINPUT_LIBRARY_RELEASE
	DIRECTX8_DXGUID_LIBRARY
	DIRECTX8_DXERR_LIBRARY
	DIRECTX8_D3D8_LIBRARY
	DIRECTX8_D3DXOF_LIBRARY
	DIRECTX8_D3DX8_LIBRARY_RELEASE
	DIRECTX8_D3DX8_LIBRARY_DEBUG
	DIRECTX8_INCLUDE_DIR)
