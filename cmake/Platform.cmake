if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64)   # Build for x64 architecture
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)   # Minimum version required
endif()