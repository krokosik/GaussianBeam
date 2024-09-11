cmake_minimum_required(VERSION 3.16)
project(GaussianBeam VERSION 0.5.0)

# Load CMake modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(DownloadProject)

# Dependencies
set(QT_MIN_VERSION "5.15.0")
find_package(Qt5 COMPONENTS Widgets Gui Xml XmlPatterns REQUIRED)

download_project(PROJ XercesC
    GIT_REPOSITORY      https://github.com/apache/xerces-c
    GIT_TAG             v3.2.3
    UPDATE_DISCONNECTED 1
    QUIET
)

add_subdirectory(${XercesC_SOURCE_DIR} ${XercesC_BINARY_DIR})

download_project(PROJ XalanC
    GIT_REPOSITORY      https://github.com/apache/xalan-c
    GIT_TAG             Xalan-C_1_12_0
    UPDATE_DISCONNECTED 1
    QUIET
)

add_subdirectory(${XalanC_SOURCE_DIR} ${XalanC_BINARY_DIR})

# Platform options
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES i386 ppc)   # Build universal library
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)   # Minimum version required
endif()

# Compiler options
# set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wno-long-long -Wno-unused-local-typedefs -g")
endif(CMAKE_COMPILER_IS_GNUCXX)

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
    gui/main.cpp
)

set(gaussianbeam_moc_SRCS
    gui/GaussianBeamDelegate.h
    gui/GaussianBeamModel.h
    gui/GaussianBeamWidget.h
    gui/GaussianBeamWindow.h
    gui/OpticsView.h
    gui/OpticsWidgets.h
)

set(gaussianbeam_SRCS
    ${gaussianbeam_src_SRCS}
    ${gaussianbeam_gui_SRCS}
    gui/GaussianBeamWidget.ui
    gui/GaussianBeamWindow.ui
    gui/OpticsViewProperties.ui
    ${gaussianbeam_moc_SRCS}
    gui/GaussianBeam.qrc
)


# Embed the application icon in the Windows executable
if(WIN32)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o
        COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/gui/GaussianBeam.rc
        -o ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o)
    set(gaussianbeam_SRCS ${gaussianbeam_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o)
else()
    set(gaussianbeam_SRCS ${gaussianbeam_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/gui/GaussianBeam.rc)
endif()

# gaussianbeam executable
add_executable(gaussianbeam ${gaussianbeam_SRCS})
target_include_directories(gaussianbeam PRIVATE
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(gaussianbeam Qt5::Widgets Qt5::Gui Qt5::Xml Qt5::XmlPatterns)

# install files
install(TARGETS gaussianbeam DESTINATION bin)

# Packaging
set(CPACK_GENERATOR DEB RPM TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 5)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_CONTACT      "Jérôme Lodewyck (jerome dot lodewyck at normalesup dot org)")
set(CPACK_DESCRIPTION_SUMMARY  "GaussianBeam is a GUI software that simulated Gaussian laser beams")
# Debian package
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
set(CPACK_DEBIAN_PACKAGE_DEPENDS      "libqt5-core (>= 5.15), libqt5-gui (>= 5.15), libqt5-xml (>= 5.15), libqt5-xmlpatterns (>= 5.15)")
set(CPACK_DEBIAN_PACKAGE_SECTION      "science")
# RPM package

# Windows installer
if(WIN32 AND NOT UNIX)
  set(CPACK_PACKAGE_EXECUTABLES "gaussianbeam" "GaussianBeam")
endif()
# Mac bundle
set(CPACK_BUNDLE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/gui/images/gaussianbeam128.png)

include(CPack)
