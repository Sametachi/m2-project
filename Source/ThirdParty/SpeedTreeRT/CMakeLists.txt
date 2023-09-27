# SpeedTreeRT 1.6 Cmake
set(FILES BillboardLeaf.cpp
BillboardLeaf.h
Branch.cpp
BranchGeometry.cpp
BranchGeometry.h
BranchInfo.cpp
Camera.cpp
Debug.h
Endian.h
EvalCode.h
EvalTest.h
FileAccess.cpp
FileAccess.h
FrondEngine.cpp
FrondEngine.h
IndexedGeometry.cpp
IndexedGeometry.h
Instances.h
LeafGeometry.cpp
LeafGeometry.h
LeafInfo.cpp
LeafLod.cpp
LeafLod.h
LightingEngine.cpp
LightingEngine.h
ProjectedShadow.cpp
ProjectedShadow.h
Random.h
SFileNameEnDe.cpp
SFileNameEnDe.h
SimpleBillboard.cpp
SimpleBillboard.h
SpeedTreeRT.cpp
SpeedTreeRT.h
StructsInfo.h
StructsSupport.h
TreeEngine.cpp
TreeEngine.h
TreeInfo.cpp
UpVector.h
WindEngine.cpp
WindEngine.h
WindInfo.cpp
LibVector_Source/IdvFastMath.h
LibVector_Source/IdvVector.h
LibVector_Source/Region.cpp
LibVector_Source/RotTransform.cpp
LibVector_Source/Transform.cpp
LibVector_Source/Vec.cpp
LibVector_Source/Vec3.cpp
LibSpline_Source/IdvSpline.cpp
LibSpline_Source/IdvSpline.h
LibGlobals_Source/IdvGlobals.h
LibFilename_Source/IdvFilename.h
LibRandom_Source/ExtendedReal.cpp
LibRandom_Source/ExtendedReal.h
LibRandom_Source/IdvRandom.cpp
LibRandom_Source/IdvRandom.h
LibRandom_Source/RobertDavies_Random.cpp
LibRandom_Source/RobertDavies_Random.h
)

add_library(SpeedTreeRT STATIC ${FILES})
set_target_properties(SpeedTreeRT PROPERTIES FOLDER ThirdParty)
target_include_directories(SpeedTreeRT PUBLIC ${CMAKE_CURRENT_LIST_DIR})
