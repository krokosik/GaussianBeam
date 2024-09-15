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
    lmmin.c
    ${HEADER_LIST}
)

target_include_directories(lib PUBLIC ${GaussianBeam_SOURCE_DIR}/include)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)