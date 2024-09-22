cmake_minimum_required(VERSION 3.17)
project(GaussianBeam VERSION 0.5.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Compiler)
include(Dependencies)
include(Package)

embed_icon(gaussianbeam_SRCS)

add_subdirectory(src/lib)
add_subdirectory(src/gui)

enable_ipo_for_target(gaussianbeam)

# install files
install(TARGETS gaussianbeam DESTINATION bin)
