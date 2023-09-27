# EterPythonLib Cmake

set(INT_SRC PythonSlotWindow.cpp
PythonSlotWindow.h
PythonGridSlotWindow.cpp
PythonGridSlotWindow.h
PythonWindow.cpp
PythonWindow.h
PythonWindowManager.cpp
PythonWindowManager.h
PythonWindowManagerModule.cpp
PythonEditLine.h
PythonEditLine.cpp
PythonScissor.h
PythonScissor.cpp
PythonUtilityHeader.hpp)

set(GRP_SRC PythonGraphic.cpp
PythonGraphic.h
PythonGraphicImageModule.cpp
PythonGraphicModule.cpp
PythonGraphicTextModule.cpp
PythonGraphicThingModule.cpp)

source_group(Interface FILES ${INT_SRC})
source_group(Graphics FILES ${GRP_SRC})

set(MAIN_SRC StdAfx.h)
source_group(Main FILES ${MAIN_SRC})

add_library(EterPythonLib STATIC ${MAIN_SRC} ${GRP_SRC} ${INT_SRC})
target_link_libraries(EterPythonLib PUBLIC EterLib ScriptLib)
set_target_properties(EterPythonLib PROPERTIES FOLDER Libraries/Client)
do_pch(PROJECT EterPythonLib)
