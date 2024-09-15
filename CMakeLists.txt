cmake_minimum_required(VERSION 3.17)
project(GaussianBeam VERSION 0.5.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Dependencies)
include(Compiler)
include(Package)

embed_icon(gaussianbeam_SRCS)

add_subdirectory(src/lib)
add_subdirectory(src/gui)

enable_ipo_for_target(gaussianbeam)

# install files
install(TARGETS gaussianbeam DESTINATION bin)

# enable_testing()
# add_executable(tests test/test.cpp)
# target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}/gui_autogen/include)
# target_link_libraries(tests PRIVATE Qt5::Test gui)
# add_test(NAME tests COMMAND tests)