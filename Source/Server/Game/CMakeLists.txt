# Output directory
file(GLOB_RECURSE sources *.cpp *.c *.h)

configure_file(
  "${CMAKE_CURRENT_LIST_DIR}/version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gen/version.h"
)

add_executable(game ${sources})

set(LINK_DEPS DevIL lzo2 IlmBase OpenEXR PNG JPEG lcms2 tiff jasper LibLZMA)
auto_dep_exe(game)
target_include_directories(game PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_link_libraries(game PRIVATE ServerCommon thecore lua YitsoraCore VFE)
set_target_properties(game PROPERTIES FOLDER Server)
do_pch(PROJECT game NAME stdafx.h)
MultiTargetFix(game game ${SERVER_BASE_PATH})
target_compile_definitions(game PRIVATE -D__SERVER__=1)
LinkVersion(game)
