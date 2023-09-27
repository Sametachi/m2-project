# Microsoft Visual C++ compiler directives

# Strict warnings
# UTF-8 encoding by default
# __cplusplus macro detection
# Multithread compilation
# fast floating point
add_compile_options(/W4 /Zc:__cplusplus /fp:fast /MP /EHsc /utf-8 /sdl)

if (BUILD_PERMISSIVE)
    add_compile_options(/permissive)
else()
    add_compile_options(/permissive-)
endif()

# TODO: investigate the actual /arch: that should be used (perhaos /arch:SSE2/AVX on 32-bit /arch:AVX512 on 64-bit)

# NOTE: 32-bit time_t should be removed
add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)

# Force static libraries
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC Runtime library" FORCE)
mark_as_advanced(CMAKE_MSVC_RUNTIME_LIBRARY)

set(BUILD_STATIC TRUE)
