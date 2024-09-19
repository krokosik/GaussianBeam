set(HEADER_LIST GaussianBeamPlot.h Names.h Unit.h)

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
target_link_libraries(gaussianbeam PUBLIC Qt5::Widgets Qt5::Gui Qt5::Xml)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/src/gui"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)