set(HEADER_LIST Names.h Unit.h)

add_executable(
    gaussianbeam
    main.cpp
    GaussianBeamWidget.cpp
    OpticsView.cpp
    OpticsWidgets.cpp
    GaussianBeamDelegate.cpp
    GaussianBeamModel.cpp
    GaussianBeamWindow.cpp
    Unit.cpp
    Names.cpp
    GaussianBeamSave.cpp
    GaussianBeamLoad.cpp
    GaussianBeamDelegate.h
    GaussianBeamModel.h
    GaussianBeamWindow.h
    OpticsView.h
    OpticsWidgets.h
    GaussianBeamWidget.ui
    GaussianBeamWindow.ui
    OpticsViewProperties.ui
    GaussianBeam.qrc
    ${HEADER_LIST}
)

target_include_directories(gaussianbeam PUBLIC ${GaussianBeam_SOURCE_DIR}/include)

target_link_libraries(gaussianbeam PRIVATE lib)
target_link_libraries(gaussianbeam PUBLIC Qt6::Widgets Qt6::Xml)
if(WIN32)
    add_custom_command(TARGET gaussianbeam POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:gaussianbeam> 
        --no-translations 
        --no-system-d3d-compiler 
        --compiler-runtime 
        --widgets
        --xml
        --no-network
        --no-svg
        --no-opengl-sw
        --no-system-dxc-compiler
        --skip-plugin-types imageformats,iconengines
    )
endif()

source_group(
    TREE "${PROJECT_SOURCE_DIR}/src/gui"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)