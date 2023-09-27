# imgui 1.88
add_library(imgui STATIC
    imconfig.h
    imgui.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    imgui.cpp
    backends/imgui_impl_dx9.h
    backends/imgui_impl_dx9.cpp
    backends/imgui_impl_win32.h
    backends/imgui_impl_win32.cpp
)

target_include_directories(imgui PRIVATE ${CMAKE_CURRENT_LIST_DIR})
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")
add_dx_deps(imgui)
