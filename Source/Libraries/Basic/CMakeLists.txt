file(GLOB_RECURSE sources *.cpp *.c *.h *.hpp)

add_library(YitsoraBasic STATIC ${sources})

target_include_directories(YitsoraBasic PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../)
target_link_libraries(YitsoraBasic PUBLIC Storm Boost::FileSystem)
set_target_properties(YitsoraBasic PROPERTIES FOLDER "Libraries/Common")
set(LINK_DEPS G3LOG spdlog fmt ZLIB Boost)
auto_dep(YitsoraBasic PUBLIC)
