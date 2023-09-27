# Find source files
file(GLOB_RECURSE SOURCES
	"src/*.c"
	"src/*.h"
	"include/*.h"
)

add_library(lua STATIC ${SOURCES})
target_include_directories(lua PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
set_target_properties(lua PROPERTIES FOLDER ThirdParty)
