function(EMBED_ICON SRCS)
    # Embed the application icon in the Windows executable
    if(WIN32)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o
            COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_CURRENT_SOURCE_DIR}/gui/GaussianBeam.rc
            -o ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o)
        set(SRCS ${SRCS} ${CMAKE_CURRENT_BINARY_DIR}/GaussianBeamIco.o)
    else()
        set(SRCS ${SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/gui/GaussianBeam.rc)
    endif()
endfunction()

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES x86_64)   # Build for x64 architecture
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.4)   # Minimum version required
endif()

set(CPACK_GENERATOR DEB RPM TGZ)
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 5)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_CONTACT      "Jérôme Lodewyck (jerome dot lodewyck at normalesup dot org)")
set(CPACK_DESCRIPTION_SUMMARY  "GaussianBeam is a GUI software that simulates Gaussian laser beams")
# Debian package
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
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