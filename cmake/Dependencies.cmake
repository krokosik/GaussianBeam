set(QT_MIN_VERSION "5.15.0")
find_package(Qt5 COMPONENTS Widgets Test Gui Xml XmlPatterns REQUIRED)

include(FetchContent)

FetchContent_Declare(
    LibXml2
    GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxml2.git
    GIT_TAG v2.13.4
)

FetchContent_Declare(
    LibXslt
    GIT_REPOSITORY https://gitlab.gnome.org/GNOME/libxslt.git
    GIT_TAG v1.1.42
)

option(LIBXML2_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXML2_WITH_TESTS "Build tests" OFF)
option(LIBXSLT_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXSLT_WITH_TESTS "Build tests" OFF)

FetchContent_MakeAvailable(LibXml2)
FetchContent_MakeAvailable(LibXslt)
