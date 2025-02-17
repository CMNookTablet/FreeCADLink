if(WIN32)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(WIN32)
endif(WIN32)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${SMSH_INCLUDE_DIR}
    ${SMESH_INCLUDE_DIR}
)

if(MSVC)
    set(CamGui_LIBS
        Cam
        Mesh
        PartGui
        FreeCADGui
        importlib_atlas.lib 
        importlib_umfpackamd.lib
        ${SMSH_LIBRARIES}
        ${SMESH_LIBRARIES}
        #OCCAdaptMesh
    )
else(MSVC)
    set(CamGui_LIBS
        Cam
        PartGui
        FreeCADGui
    )
endif(MSVC)

set(CamGui_MOC_HDRS
    Cutting.h
)
fc_wrap_cpp(CamGui_MOC_SRCS ${CamGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${CamGui_MOC_SRCS})

set(CamGui_UIC_SRCS
    Cutting.ui
)
qt4_wrap_ui(CamGui_UIC_HDRS ${CamGui_UIC_SRCS})

SET(CamGui_SRCS
    ${CamGui_UIC_HDRS}
    AppCamGui.cpp
    Command.cpp
    Cutting.cpp
    Cutting.h
    Cutting.ui
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(CamGui SHARED ${CamGui_SRCS})
target_link_libraries(CamGui ${CamGui_LIBS})

fc_target_copy_resource(CamGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Cam
    ${CMAKE_BINARY_DIR}/Mod/Cam
    InitGui.py)

SET_BIN_DIR(CamGui CamGui /Mod/Cam)
if(WIN32)
    set_target_properties(CamGui PROPERTIES SUFFIX ".pyd")
endif(WIN32)

INSTALL(TARGETS CamGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
