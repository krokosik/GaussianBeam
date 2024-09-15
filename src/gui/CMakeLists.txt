file(GLOB HEADER_LIST CONFIGURE DEPENDS "${GaussianBeam_SOURCE_DIR}/include/gui/*.h")

add_library(
    gui
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

target_include_directories(gui PUBLIC ../../include)

target_link_libraries(gui PRIVATE logic)
target_link_libraries(gui PUBLIC Qt5::Widgets Qt5::Gui Qt5::Xml Qt5::XmlPatterns LibXml2::LibXml2 LibXslt::LibXslt)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include/gui"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)