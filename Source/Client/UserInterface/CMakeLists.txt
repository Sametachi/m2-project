# Metin2Client Cmake
configure_file(${CMAKE_CURRENT_LIST_DIR}/CrashpadConfig.h.in ${CMAKE_BINARY_DIR}/generated/CrashpadConfig.h)

if (MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO /NODEFAULTLIB:libci.lib /SUBSYSTEM:WINDOWS")
endif()

file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

set(RC resource.h UserInterface.rc)
set(CURSORS Resources/cursor_pick.cur Resources/cursor.cur Resources/cursor_attack.cur Resources/cursor_buy.cur Resources/cursor_camera_rotate.cur Resources/cursor_chair.cur Resources/cursor_door.cur Resources/cursor_hsize.cur Resources/cursor_hvsize.cur Resources/cursor_no.cur Resources/cursor_sell.cur Resources/cursor_talk.cur Resources/cursor_vsize.cur)
set(ICONS Resources/yitsora.ico)
source_group(Resources/Cursors FILES ${CURSORS})
source_group(Resources/Icons FILES ${ICONS})
source_group(Resources sources ${RC})

add_executable(UserInterface WIN32 ${sources} ${RC} ${CURSORS} ${ICONS})
target_link_libraries(UserInterface PUBLIC GameLib SphereLib strmiids dmoguids ddraw VFE CAPILibs Iphlpapi imgui)
set_target_properties(UserInterface PROPERTIES FOLDER Client VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
do_pch(PROJECT UserInterface)
MultiTargetFix(UserInterface ${V_SERVER_NAME} ${BINARY_OUTPUT_DIR})
target_compile_definitions(UserInterface PRIVATE -DUSE_LOD=1 -D__CLIENT__=1)

set(LINK_DEPS crashpad DevIL lzo2 IlmBase OpenEXR PNG JPEG lcms2 tiff jasper LibLZMA OpenSSL cxxopts tweeny FREETYPE expat)
auto_dep_exe(UserInterface)
LinkVersion(UserInterface)

if (WIN32)
    target_link_libraries(UserInterface PRIVATE Imm32 ${DIRECTX_DXERR_LIBRARY})
endif()

if (MSVC)
    target_link_options(UserInterface PRIVATE "/SAFESEH:NO;/NODEFAULTLIB:libci.lib")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /SUBSYSTEM:CONSOLE")
endif()
