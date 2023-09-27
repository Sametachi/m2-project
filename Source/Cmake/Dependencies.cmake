if (NOT CFG_NO_PKGCONFIG)
    include(FindPkgConfig)
endif()

# General dependency resolver with pkg-config and vcpkg as a fallback
function(find_dep)
    cmake_parse_arguments(FIND_DEP "" "NAME;TARGET;DEBUG_POSTFIX" "COMPONENTS" ${ARGN})

    if (NOT DEFINED FIND_DEP_NAME)
        message(FATAL_ERROR "Invalid find_dep call")
    endif()
    
    set(NO_CONFIG_DEPS Boost PNG ZLIB Python3 LibLZMA OpenSSL)
    if (CFG_VCPKG) # hotfix for boost which should not use CONFIG as vcpkg does not provide FindBoost.cmake
        set(FIND_DEP_CONFIG CONFIG)
        foreach(ncfg IN LISTS NO_CONFIG_DEPS)
            if (${ncfg} STREQUAL ${FIND_DEP_NAME})
                unset(FIND_DEP_CONFIG)
            endif()
        endforeach()
    endif()

    if (NOT DEFINED FIND_DEP_DEBUG_POSTFIX)
        set(FIND_DEP_DEBUG_POSTFIX "")
    endif()

    # Default cmake find (config for vcpkg)
    if (DEFINED FIND_DEP_COMPONENTS)
        find_package(${FIND_DEP_NAME} ${FIND_DEP_CONFIG} COMPONENTS ${FIND_DEP_COMPONENTS} QUIET)
    else()
        find_package(${FIND_DEP_NAME} ${FIND_DEP_CONFIG} QUIET)
    endif()

    if (${FIND_DEP_NAME}_FOUND)
        set(${FIND_DEP_NAME}_FOUND TRUE CACHE BOOL "" FORCE)
        set(${FIND_DEP_NAME}_IDIR ${${FIND_DEP_NAME}_INCLUDE_DIR} CACHE STRING "" FORCE)

        if (NOT DEFINED FIND_DEP_TARGET)
            set(${FIND_DEP_NAME}_TARGET ${${FIND_DEP_NAME}_LIBRARIES} CACHE STRING "" FORCE)
        else()
            set(${FIND_DEP_NAME}_TARGET ${FIND_DEP_TARGET} CACHE STRING "" FORCE)
        endif()
        return()
    endif()

    # Reset found/dir and hide dir
    unset(${FIND_DEP_NAME}_FOUND)
    unset(${FIND_DEP_NAME}_DIR CACHE)
    mark_as_advanced(${FIND_DEP_NAME}_DIR)

    # Find via pkg-config
    if (NOT CFG_NO_PKGCONFIG)
        pkg_check_modules(${FIND_DEP_NAME} QUIET)
        if (${FIND_DEP_NAME}_FOUND)
            set(${FIND_DEP_NAME}_FOUND TRUE CACHE BOOL "" FORCE)
            set(${FIND_DEP_NAME}_TARGET ${${FIND_DEP_NAME}_LIBRARIES} CACHE STRING "" FORCE)
            set(${FIND_DEP_NAME}_LDIR ${${FIND_DEP_NAME}_LIBRARY_DIRS} CACHE STRING "" FORCE)
            set(${FIND_DEP_NAME}_IDIR ${${FIND_DEP_NAME}_INCLUDE_DIRS} CACHE STRING "" FORCE)
            return()
        endif()
        # Reset found or find_library will fail
        unset(${FIND_DEP_NAME}_FOUND)
    endif()

    # Manual find or vcpkg find
    if (CFG_VCPKG)
        find_library(${FIND_DEP_NAME}_FOUND NAMES ${FIND_DEP_NAME} PATHS "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    else()
        find_library(${FIND_DEP_NAME}_FOUND NAMES ${FIND_DEP_NAME})
    endif()

    if (NOT ${FIND_DEP_NAME}_FOUND)
        message(FATAL_ERROR "Dependency ${FIND_DEP_NAME} not found")
    endif()

    set(${FIND_DEP_NAME}_FOUND TRUE CACHE BOOL "" FORCE)

    if (CFG_VCPKG)
        # this is required for vcpkg manual lib find as vcpkg does not export everything
        set(${FIND_DEP_NAME}_DO_LINK TRUE CACHE BOOL "" FORCE)
        set(${FIND_DEP_NAME}_OPTIMIZED ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${FIND_DEP_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX} CACHE STRING "" FORCE)
        set(${FIND_DEP_NAME}_DEBUG ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${FIND_DEP_NAME}${FIND_DEP_DEBUG_POSTFIX}${CMAKE_STATIC_LIBRARY_SUFFIX} CACHE STRING "" FORCE)
    else()
        # found inside the system (eg: Linux)
        set(${FIND_DEP_NAME}_TARGET ${FIND_DEP_NAME} CACHE STRING "" FORCE)
    endif()
endfunction()

function(add_dep)
    cmake_parse_arguments(ADD_DEP "" "NAME;PROJECT;EXPORT" "" ${ARGN})

    if (NOT DEFINED ADD_DEP_NAME OR NOT DEFINED ADD_DEP_EXPORT OR NOT DEFINED ADD_DEP_PROJECT)
        message(FATAL_ERROR "Invalid add_dep call")
    endif()

    if (NOT ${ADD_DEP_NAME}_DO_LINK)
        target_link_libraries(${ADD_DEP_PROJECT} ${ADD_DEP_EXPORT} ${${ADD_DEP_NAME}_TARGET})
        target_include_directories(${ADD_DEP_PROJECT} ${ADD_DEP_EXPORT} ${${ADD_DEP_NAME}_IDIR})
        target_link_directories(${ADD_DEP_PROJECT} ${ADD_DEP_EXPORT} ${${ADD_DEP_NAME}_LDIR})
    else()
        target_link_libraries(${ADD_DEP_PROJECT} ${ADD_DEP_EXPORT} debug ${${ADD_DEP_NAME}_DEBUG})
        target_link_libraries(${ADD_DEP_PROJECT} ${ADD_DEP_EXPORT} optimized ${${ADD_DEP_NAME}_OPTIMIZED})
    endif()
endfunction()

macro(auto_boost)
    cmake_parse_arguments(AUTO_BOOST "" "PROJECT;EXPORT" "COMPONENTS" ${ARGN})
    if (NOT DEFINED AUTO_BOOST_PROJECT OR NOT DEFINED AUTO_BOOST_COMPONENTS)
        message(FATAL_ERROR "Invalid auto_boost call")
    endif()

    if (NOT DEFINED AUTO_BOOST_EXPORT)
        set(AUTO_BOOST_EXPORT PRIVATE)
    endif()

    foreach (dep IN LISTS AUTO_BOOST_COMPONENTS)
        target_link_libraries(${AUTO_BOOST_PROJECT} ${AUTO_BOOST_EXPORT} debug ${Boost_${dep}_LIBRARY_DEBUG})
        target_link_libraries(${AUTO_BOOST_PROJECT} ${AUTO_BOOST_EXPORT} optimized ${Boost_${dep}_LIBRARY_RELEASE})
    endforeach()
endmacro()

# automatic dependency linker (with export option)
macro(auto_dep project exp)
    foreach(dep IN LISTS LINK_DEPS)
        add_dep(NAME ${dep} PROJECT ${project} EXPORT ${exp})
    endforeach()
endmacro()

# automatic dependency linker for executables
macro(auto_dep_exe project)
    auto_dep(${project} PRIVATE)
    if (NOT CFG_NO_PTHREADS)
        target_link_libraries(${project} PRIVATE Threads::Threads)
    endif()
    if (DEFINED LINK_EXTRA)
        target_link_libraries(${project} PRIVATE ${LINK_EXTRA})
    endif()
endmacro()

if (DEFINED VCPKG_TARGET_TRIPLET)
    set(CFG_VCPKG TRUE)
endif()

if (CFG_VCPKG)
    if (NOT "${VCPKG_TARGET_TRIPLET}" MATCHES "-static" AND WIN32)
        message(FATAL_ERROR "Windows static builds are required")
    endif()
endif()
