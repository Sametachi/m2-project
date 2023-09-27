# EterLib cmake file

## TODO: Merge filters
file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

add_library(EterLib STATIC ${sources})
set_target_properties(EterLib PROPERTIES FOLDER Libraries/Client)
target_link_libraries(EterLib PUBLIC EterImageLib granny2 Boost::Locale)
do_pch(PROJECT EterLib)

