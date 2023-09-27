unset(SERVER_BASE_PATH CACHE)
set(SERVER_BASE_PATH ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Serverb ase path" FORCE)
mark_as_advanced(SERVER_BASE_PATH) # hide base path in cache

add_subdirectory(Game)
add_subdirectory(Database)
add_subdirectory(Quest)
