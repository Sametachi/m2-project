# Find source files
file(GLOB SOURCES
	"src/*.cpp"
	"include/*.h"
)

add_library(thecore STATIC ${SOURCES})
target_link_libraries(thecore PUBLIC YitsoraBasic)
target_include_directories(thecore PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

if (CFG_VCPKG)
	add_dep(NAME unofficial-libmariadb PROJECT thecore EXPORT PUBLIC)
else()
	add_dep(NAME libmariadb PROJECT thecore EXPORT PUBLIC)
endif()

set_target_properties(thecore PROPERTIES FOLDER Libraries/Server)
