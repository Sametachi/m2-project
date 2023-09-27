set(CFG_NO_DIRECTX TRUE)

unset(BUILD_NO_CLIENT CACHE)
set(BUILD_NO_CLIENT TRUE CACHE BOOL "Disable building of Client" FORCE)
mark_as_advanced(BUILD_NO_CLIENT)

