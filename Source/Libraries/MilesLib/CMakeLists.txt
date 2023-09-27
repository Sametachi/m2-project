# MilesLib Cmake

set(FILES SoundBase.cpp
SoundData.cpp
SoundInstance.cpp
SoundInstanceStream.cpp
SoundManager.cpp
Type.cpp
SoundBase.h
SoundData.h
SoundInstance.h
SoundManager.h
Stdafx.h
Type.h)

add_library(MilesLib STATIC ${FILES})
target_link_libraries(MilesLib PUBLIC EterBase soloud)
set_target_properties(MilesLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT MilesLib)
target_include_directories(MilesLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../ThirdParty/soloud/include)
add_dx_deps(MilesLib)
