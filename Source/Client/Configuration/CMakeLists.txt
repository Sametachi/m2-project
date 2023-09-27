add_executable(Configuration MainDialog.cpp MainDialog.h Config.cpp)
set(RC resource.h Config.rc)

target_compile_definitions(Configuration PRIVATE -DARCHIVER)
target_link_libraries(Configuration PRIVATE ${DIRECTX_DXERR_LIBRARY} Storm)
set_target_properties(Configuration PROPERTIES FOLDER "Tools")
set(LINK_DEPS spdlog Boost WTL)
auto_dep_exe(Configuration)
auto_boost(PROJECT Configuration COMPONENTS PROGRAM_OPTIONS)
