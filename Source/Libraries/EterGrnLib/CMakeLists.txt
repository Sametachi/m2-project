# EterGrnLib Cmake

## TODO: Merge filters
file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

add_library(EterGrnLib STATIC ${sources})
target_link_libraries(EterGrnLib PUBLIC EterBase granny2)
set_target_properties(EterGrnLib PROPERTIES FOLDER Libraries/Client)
add_dx_deps(EterGrnLib)
do_pch(PROJECT EterGrnLib)
