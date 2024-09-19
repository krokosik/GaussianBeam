file(GLOB HEADER_LIST CONFIGURE DEPENDS "${GaussianBeam_SOURCE_DIR}/include/*.h")

add_library(
    lib
    GaussianBeam.cpp
    Optics.cpp
    OpticsBench.cpp
    OpticsBench.cpp
    GaussianFit.cpp
    Function.cpp
    OpticsFunction.cpp
    Cavity.cpp
    Utils.cpp
    IO.cpp
    lmmin.c
    ${HEADER_LIST}
)

target_include_directories(lib PUBLIC ${GaussianBeam_SOURCE_DIR}/include)

target_link_libraries(lib PRIVATE LibXml2::LibXml2 LibXslt::LibXslt)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)