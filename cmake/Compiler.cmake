set(CMAKE_CXX_STANDARD 17 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-pedantic -Wall -Wno-long-long -Wno-unused-local-typedefs -g")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CUDA_COMPILER_LAUNCHER "${CCACHE_PROGRAM}") # CMake 3.9+
endif() 

find_program(CLANG_TIDY_PROGRAM clang-tidy)
if(CLANG_TIDY_PROGRAM)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM};-fix")
endif()

set(CMAKE_LINK_WHAT_YOU_USE TRUE)

function(ENABLE_IPO_FOR_TARGET TARGET)
    # Enable Interprocedural Optimization if available
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported)
    if(ipo_supported)
        set_target_properties(${TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif()
endfunction()