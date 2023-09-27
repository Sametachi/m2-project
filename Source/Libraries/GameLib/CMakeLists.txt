# GameLib Cmake

## TODO: Merge filters
file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

add_library(GameLib STATIC ${sources})
target_link_libraries(GameLib PUBLIC EterLib MilesLib EffectLib EterPythonLib SpeedTreeLib PRTerrainLib)
set_target_properties(GameLib PROPERTIES FOLDER Libraries/Client)
target_compile_definitions(GameLib PUBLIC BACKGROUND_LOADING=1)
do_pch(PROJECT GameLib)
