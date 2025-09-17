cmake_minimum_required(VERSION 3.16)

project(ottsr 
    VERSION 2.0.0
    DESCRIPTION "Study Timer Pro - A modern study timer with customizable profiles"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)

# Find GTK3
pkg_check_modules(GTK3 REQUIRED gtk+-3.0>=3.22)

# Find JSON-GLib (try different package names)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
if(NOT JSON_GLIB_FOUND)
    pkg_check_modules(JSON_GLIB REQUIRED libjson-glib-1.0)
endif()

# Find additional libraries
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

# Set compiler flags for different build types
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")

# Add executable
add_executable(${PROJECT_NAME}
    src/ottsr.c
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${JSON_GLIB_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GIO_INCLUDE_DIRS}
    src/
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    ${GTK3_LIBRARIES}
    ${JSON_GLIB_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
)

# Compiler-specific options
target_compile_options(${PROJECT_NAME} PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${JSON_GLIB_CFLAGS_OTHER}
    ${GLIB_CFLAGS_OTHER}
    ${GIO_CFLAGS_OTHER}
)

# Set install paths
include(GNUInstallDirs)

# Install binary
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install desktop file
install(FILES data/ottsr.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

# Install icon
install(FILES data/ottsr.png
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/128x128/apps
)

# Install man page
install(FILES data/ottsr.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
)

# Create directories for configuration
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory $ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME})")

# Install documentation
install(FILES 
    README.md
    LICENSE
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)

# Packaging
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "g-flame")
set(CPACK_PACKAGE_CONTACT "https://github.com/g-flame/ottsr")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Platform-specific packaging
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "Study Timer Pro")
    set(CPACK_NSIS_PACKAGE_NAME "Study Timer Pro")
    set(CPACK_NSIS_HELP_LINK "https://github.com/g-flame/ottsr")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/g-flame/ottsr")
    set(CPACK_NSIS_CONTACT "https://github.com/g-flame/ottsr/issues")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_BUNDLE_NAME "Study Timer Pro")
    set(CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/data/Info.plist")
    set(CPACK_BUNDLE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/data/ottsr.icns")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    
    # Debian package
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "g-flame")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libgtk-3-0 (>= 3.22), libjson-glib-1.0-0, libglib2.0-0")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/g-flame/ottsr")
    
    # RPM package
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
    set(CPACK_RPM_PACKAGE_URL "https://github.com/g-flame/ottsr")
    set(CPACK_RPM_PACKAGE_REQUIRES "gtk3 >= 3.22, json-glib, glib2")
endif()

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "Configuration Summary:")
message(STATUS "  Project Name:     ${PROJECT_NAME}")
message(STATUS "  Version:          ${PROJECT_VERSION}")
message(STATUS "  Build Type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C Compiler:       ${CMAKE_C_COMPILER}")
message(STATUS "  C Flags:          ${CMAKE_C_FLAGS}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  GTK3:             ${GTK3_VERSION}")
message(STATUS "  JSON-GLib:        ${JSON_GLIB_VERSION}")
message(STATUS "  GLib:             ${GLIB_VERSION}")
message(STATUS "")

# Custom targets for development
add_custom_target(format
    COMMAND find src/ -name "*.c" -o -name "*.h" | xargs clang-format -i
    COMMENT "Formatting source code"
)

add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Deep clean build directory"
)
