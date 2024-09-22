set(QT_MIN_VERSION "6.7.0")
find_package(Qt6 REQUIRED COMPONENTS Widgets Xml)
qt_standard_project_setup()

if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS
        "${Qt6_DIR}/../../../bin"       # Adjust based on your Qt installation layout
        "${Qt6_DIR}/bin"
        ENV QT_BIN_DIR                  # Environment variable (optional)
        NO_DEFAULT_PATH                 # Ignore default search paths
    )

    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR "windeployqt not found!")
    else()
        message(STATUS "Found windeployqt at: ${WINDEPLOYQT_EXECUTABLE}")
    endif()
endif()

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

option(LIBXML2_WITH_C14N "Add the Canonicalization support" OFF)
option(LIBXML2_WITH_CATALOG "Add the Catalog support" OFF)
option(LIBXML2_WITH_HTML "Add the HTML support" ON)
option(LIBXML2_WITH_PROGRAMS "Build programs" OFF)
option(LIBXML2_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXML2_WITH_TESTS "Build tests" OFF)
option(LIBXML2_WITH_ICONV "Use iconv" OFF)
option(LIBXML2_WITH_ICU "Use ICU" OFF)
option(LIBXML2_WITH_LZMA "Use LZMA" OFF)
option(LIBXML2_WITH_ZLIB "Use ZLIB" OFF)
option(LIBXML2_WITH_THREADS "Use threads" OFF)
option(LIBXSLT_WITH_PYTHON "Build Python bindings" OFF)
option(LIBXSLT_WITH_PROGRAMS "Build programs" OFF)
option(LIBXSLT_WITH_MODULES "Add the module support" OFF)
option(LIBXSLT_WITH_TESTS "Build tests" OFF)
option(LIBXSLT_WITH_THREADS "Add multithread support" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

FetchContent_MakeAvailable(LibXml2)
FetchContent_MakeAvailable(LibXslt)
