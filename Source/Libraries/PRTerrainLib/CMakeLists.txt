# PRTerrainLib Cmake

set(FILES TextureSet.h TextureSet.cpp TerrainType.h Terrain.h Terrain.cpp StdAfx.h StdAfx.cpp)

add_library(PRTerrainLib STATIC ${FILES})
target_link_libraries(PRTerrainLib PUBLIC EterLib EterGrnLib ScriptLib)
set_target_properties(PRTerrainLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT PRTerrainLib)
