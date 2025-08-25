cmake_minimum_required(VERSION 3.15)
project(libgossip VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_EXAMPLES "Build example applications" ON)

set(LIBGOSSIP_CORE_SRC
    src/core/gossip_core.cpp
)

# Create the main library
add_library(libgossip ${LIBGOSSIP_CORE_SRC})
add_library(libgossip::core ALIAS libgossip)

if(MSVC)
target_compile_options(libgossip PRIVATE /utf-8)
endif()

# Set include directories for the library
target_include_directories(libgossip PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Set target properties
set_target_properties(libgossip PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Create export header
include(GenerateExportHeader)
generate_export_header(libgossip)

# Export targets for build directory
export(TARGETS libgossip
    NAMESPACE libgossip::
    FILE libgossipTargets.cmake
)

# Install targets
include(GNUInstallDirs)
install(TARGETS libgossip
    EXPORT libgossipTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION include
)

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Create and install config files
install(EXPORT libgossipTargets
    FILE libgossipTargets.cmake
    NAMESPACE libgossip::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    libgossipConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libgossipConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip
)

# Install examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
    
    # Install example source files
    install(DIRECTORY examples/ 
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples
        FILES_MATCHING 
        PATTERN "*.cpp"
        PATTERN "CMakeLists.txt" EXCLUDE
    )
endif()

# CPack packaging
set(CPACK_PACKAGE_NAME "libgossip")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A modern C++17 implementation of the gossip protocol for distributed systems")
set(CPACK_PACKAGE_VENDOR "caomengxuan666")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)