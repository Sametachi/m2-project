# Function to add DirectX (8/9) dependencies to a project
function(add_dx_deps proj)
    if (CFG_NO_DIRECTX)
        return()
    endif()

	if (CFG_DIRECTX8)
		target_link_libraries(${proj} PUBLIC debug ${DIRECTX8_D3DX8_LIBRARY_DEBUG})
		target_link_libraries(${proj} PUBLIC optimized ${DIRECTX8_D3DX8_LIBRARY_RELEASE})
		target_link_libraries(${proj} PUBLIC ${DIRECTX8_LIBRARIES})
		target_include_directories(${proj} PUBLIC ${DIRECTX8_INCLUDE_DIR})
	else()
		target_link_libraries(${proj} PUBLIC debug ${DIRECTX_D3DX9_LIBRARY_DEBUG})
		target_link_libraries(${proj} PUBLIC optimized ${DIRECTX_D3DX9_LIBRARY_RELEASE})
		target_link_libraries(${proj} PUBLIC ${DIRECTX_LIBRARIES})
		target_include_directories(${proj} PUBLIC ${DIRECTX_INCLUDE_DIR})
	endif()
	target_compile_definitions(${proj} PUBLIC DIRECTINPUT_VERSION=0x0800)
endfunction()

# Find DirectX 
if (NOT CFG_NO_DIRECTX)
	if (CFG_DIRECTX8)
		find_package(DirectX8 REQUIRED)
	else()
	    find_package(DirectX REQUIRED)
	endif()
endif()

