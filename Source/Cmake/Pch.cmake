# this function computes precompiled header
function(do_pch)
    cmake_parse_arguments(DO_PCH "" "PROJECT;NAME" "" ${ARGN})

    if (NOT DEFINED DO_PCH_PROJECT)
        message(FATAL_ERROR "Invalid do_pch call")
    endif()

    if (NOT DEFINED DO_PCH_NAME)
        set(DO_PCH_NAME "Stdafx.h")
    endif()

    target_precompile_headers(${DO_PCH_PROJECT} PRIVATE ${DO_PCH_NAME})
    source_group("PCH/src_cxx" cmake_pch.cxx)
    source_group("PCH/include_cxx" cmake_pch.hxx)
    source_group("PCH/src_cc" cmake_pch.c)
    source_group("PCH/include_cc" cmake_pch.h)
endfunction()
