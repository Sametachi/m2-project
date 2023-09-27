# Windows directives
add_definitions(-D__WIN32__ -DSTRICT -D_WIN32_WINNT=0x0601 -DNOMINMAX) # Default target: Windows 7

if (NOT MINGW AND NOT CYGWIN AND NOT MSYS)
    set(CFG_NO_PKGCONFIG TRUE)
    set(CFG_NO_PTHREADS TRUE)
endif()

set(LINK_EXTRA ws2_32 Version Setupapi)
