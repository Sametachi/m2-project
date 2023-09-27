add_library(ServerCommon INTERFACE
  auction_table.h
  billing.h
  building.h
  cache.h
  d3dtype.h
  length.h
  noncopyable.h
  pool.h
  service.h
  stl.h
  tables.h
  teen_packet.h
  utils.h
  VnumHelper.h
)

target_include_directories(ServerCommon INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../)
set_target_properties(ServerCommon PROPERTIES FOLDER "Libraries/Server")
target_link_libraries(ServerCommon INTERFACE YitsoraBasic YitsoraCore)