cmake_minimum_required(VERSION 3.17)
project(GaussianBeam VERSION 0.5.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Dependencies
include(Dependencies)

# Platform options
include(Platform)

# Compiler options
include(Compiler)

# Sources
set(gaussianbeam_src_SRCS
    src/GaussianBeam.cpp
    src/Optics.cpp
    src/OpticsBench.cpp
    src/OpticsBench.cpp
    src/GaussianFit.cpp
    src/Function.cpp
    src/OpticsFunction.cpp
    src/Cavity.cpp
    src/Utils.cpp
    src/lmmin.c
)

set(gaussianbeam_gui_SRCS
    gui/GaussianBeamWidget.cpp
    gui/OpticsView.cpp
    gui/OpticsWidgets.cpp
    gui/GaussianBeamDelegate.cpp
    gui/GaussianBeamModel.cpp
    gui/GaussianBeamWindow.cpp
    gui/Unit.cpp
    gui/Names.cpp
    gui/GaussianBeamSave.cpp
    gui/GaussianBeamLoad.cpp
    gui/GaussianBeamDelegate.h
    gui/GaussianBeamModel.h
    gui/GaussianBeamWidget.h
    gui/GaussianBeamWindow.h
    gui/OpticsView.h
    gui/OpticsWidgets.h
    gui/GaussianBeamWidget.ui
    gui/GaussianBeamWindow.ui
    gui/OpticsViewProperties.ui
    gui/GaussianBeam.qrc
    gui/main.cpp
)

# Packaging
include(Package)

embed_icon(gaussianbeam_SRCS)

add_library(src ${gaussianbeam_src_SRCS})
add_library(gui ${gaussianbeam_gui_SRCS})
target_link_libraries(gui PRIVATE src Qt5::Widgets Qt5::Gui Qt5::Xml Qt5::XmlPatterns LibXml2::LibXml2 LibXslt::LibXslt)

# gaussianbeam executable
add_executable(gaussianbeam)
target_link_libraries(gaussianbeam gui)

enable_ipo_for_target(gaussianbeam)

# install files
install(TARGETS gaussianbeam DESTINATION bin)

# enable_testing()
# add_executable(tests test/test.cpp)
# target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/gui_autogen/include)
# target_link_libraries(tests PRIVATE Qt5::Test gui)
# add_test(NAME tests COMMAND tests)