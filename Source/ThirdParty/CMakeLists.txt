add_subdirectory(SpeedTreeRT)
add_subdirectory(soloud/contrib)
set_target_properties(soloud PROPERTIES FOLDER ThirdParty)
add_subdirectory(granny2)
add_subdirectory(Storm)
add_subdirectory(ImGui)
add_subdirectory(PythonCAPI)

set(MSL_FILES
msl/include/msl/assert.h
msl/include/msl/bench.h
msl/include/msl/cast.h
msl/include/msl/file_ptr.h
msl/include/msl/macro.h
msl/include/msl/msl.h
msl/include/msl/random.h
msl/include/msl/range.h
msl/include/msl/utils.h
)

add_library(MSL INTERFACE ${MSL_FILES})
target_include_directories(MSL INTERFACE msl/include)
set_target_properties(MSL PROPERTIES FOLDER ThirdParty)