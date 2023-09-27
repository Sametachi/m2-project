# SphereLib Cmake

set(FILES frustum.cpp frustum.h pool.h sphere.cpp sphere.h spherepack.h spherepack.cpp StdAfx.h vector.h)

add_library(SphereLib STATIC ${FILES})
target_link_libraries(SphereLib PUBLIC EterBase)
set_target_properties(SphereLib PROPERTIES FOLDER Libraries/Client)
add_dx_deps(SphereLib)
do_pch(PROJECT SphereLib)
