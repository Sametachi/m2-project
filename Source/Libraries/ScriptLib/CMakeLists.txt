# ScriptLib Cmake
file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp *.inl)

add_library(ScriptLib STATIC ${sources})
target_link_libraries(ScriptLib PUBLIC EterLib GameLib EterGrnLib)
set_target_properties(ScriptLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT ScriptLib)
add_dep(NAME Python3 PROJECT ScriptLib EXPORT PUBLIC)
add_dep(NAME pybind11 PROJECT ScriptLib EXPORT PUBLIC)
