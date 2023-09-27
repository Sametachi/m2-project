# Output directory
file(GLOB_RECURSE sources
  *.cpp *.h
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/gen/version.h"
)

add_executable(db ${sources})

# Set the debug directory on MSVC
#if (MSVC)
#	set_target_properties(db PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
#endif()

set(LINK_DEPS )
auto_dep_exe(db)
target_include_directories(db PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gen)
target_link_libraries(db PRIVATE ServerCommon YitsoraCore thecore lua VFE)
set_target_properties(db PROPERTIES FOLDER Server)
do_pch(PROJECT db NAME stdafx.h)
MultiTargetFix(db db ${SERVER_BASE_PATH})
target_compile_definitions(db PRIVATE -D__SERVER__=1)
LinkVersion(db)
