# EterBase Cmake
set(CIPHER_SRC tea.cpp tea.h)
set(UTIL_SRC Random.cpp Random.h Stl.cpp Stl.h vk.h Utils.cpp Utils.h)
set(UNSORTED_SRC Timer.cpp Timer.h CPostIt.cpp CPostIt.h lzo.cpp lzo.h)
set(VID_SRC VideoMode.h VideoMode.cpp)

source_group(Security FILES ${CIPHER_SRC})
source_group(FileAPI FILES ${FILE_SRC})
source_group(Utilities FILES ${UTIL_SRC})
source_group(Video FILES ${VID_SRC})

add_library(EterBase STATIC ${UTIL_SRC} ${CIPHER_SRC} ${FILE_SRC} ${UNSORTED_SRC} ${VID_SRC})
target_link_libraries(EterBase PUBLIC YitsoraCore)
set_target_properties(EterBase PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT EterBase)
add_dx_deps(EterBase)
