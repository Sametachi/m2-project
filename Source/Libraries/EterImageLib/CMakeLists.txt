# EterImageLib Cmake

file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

add_library(EterImageLib STATIC ${sources})
target_link_libraries(EterImageLib PUBLIC EterBase)
set_target_properties(EterImageLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT EterImageLib)
add_dx_deps(EterImageLib)
