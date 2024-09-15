file(GLOB HEADER_LIST CONFIGURE DEPENDS "${GaussianBeam_SOURCE_DIR}/include/logic/*.h")

add_library(
    logic
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

target_include_directories(logic PUBLIC ../../include)

source_group(
    TREE "${PROJECT_SOURCE_DIR}/include/logic"
    PREFIX "Header Files"
    FILES ${HEADER_LIST}
)