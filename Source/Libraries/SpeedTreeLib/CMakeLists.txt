# SpeedTreeLib Cmake

set(FILES BoundaryShapeManager.h
Constants.h
SpeedGrassRT.h
SpeedGrassWrapper.h
SpeedTreeConfig.h
SpeedTreeForest.h
SpeedTreeForestDirectX9.h
SpeedTreeMaterial.h
SpeedTreeWrapper.h
StdAfx.h
VertexShaders.h
BoundaryShapeManager.cpp
SpeedGrassRT.cpp
SpeedGrassWrapper.cpp
SpeedTreeForest.cpp
SpeedTreeForestDirectX9.cpp
SpeedTreeWrapper.cpp)

add_library(SpeedTreeLib STATIC ${FILES})
target_link_libraries(SpeedTreeLib PUBLIC EterBase SpeedTreeRT)
set_target_properties(SpeedTreeLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT SpeedTreeLib)
add_dx_deps(SpeedTreeLib)
