add_executable(qc crc32.h crc32.cc qc.cc)

target_link_libraries(qc PRIVATE lua YitsoraBasic)
set_target_properties(qc PROPERTIES FOLDER Tools)
target_include_directories(qc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../Libraries/LibLua/src)
MultiTargetFix(qc qc ${SERVER_BASE_PATH})
