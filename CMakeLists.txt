cmake_minimum_required(VERSION 3.15)
project(libgossip VERSION 1.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include our custom dependency management module
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
option(BUILD_TESTS "Build unit tests" ON)

# Load dependency management functions
include(FindDependencies)

set(LIBGOSSIP_CORE_SRC
    src/core/gossip_core.cpp
    src/core/gossip_c.cpp
    include/libgossip_api.h
)

# Create the main library
add_library(libgossip ${LIBGOSSIP_CORE_SRC})
add_library(libgossip::core ALIAS libgossip)

# Handle ASIO dependency
setup_bundled_asio()

# Network library
set(LIBGOSSIP_NET_SRC
    src/net/udp_transport.cpp
    src/net/tcp_transport.cpp
    src/net/transport_factory.cpp
)

add_library(libgossip_net ${LIBGOSSIP_NET_SRC})
add_library(libgossip::network ALIAS libgossip_net)

# Find and link ASIO
if(ASIO_FOUND)
    target_include_directories(libgossip_net PUBLIC 
        $<BUILD_INTERFACE:${ASIO_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>)
else()
    message(FATAL_ERROR "ASIO is required for the network library")
endif()

target_link_libraries(libgossip_net PUBLIC libgossip)

if(MSVC)
target_compile_options(libgossip PRIVATE /utf-8)
target_compile_options(libgossip_net PRIVATE /utf-8)
endif()

# Set include directories for the library
target_include_directories(libgossip PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(libgossip PUBLIC LIBGOSSIP_BUILD)
# Set target properties
set_target_properties(libgossip PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

set_target_properties(libgossip_net PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Create export header
include(GenerateExportHeader)
generate_export_header(libgossip)

# Export targets for build directory
export(TARGETS libgossip libgossip_net
    NAMESPACE libgossip::
    FILE libgossipTargets.cmake
)

# Install targets
include(GNUInstallDirs)
install(TARGETS libgossip libgossip_net
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
    cmake/libgossipConfig.cmake.in
    libgossipConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip
)

install(FILES 
    ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfigVersion.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libgossip_export.h
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip
)

# Examples
if(BUILD_EXAMPLES)
    add_executable(simple_cluster examples/simple_cluster.cpp)
    target_link_libraries(simple_cluster libgossip)
    
    add_executable(seed_cluster examples/seed_cluster.cpp)
    target_link_libraries(seed_cluster libgossip)
    
    add_executable(advanced_cluster examples/advanced_cluster.cpp)
    target_link_libraries(advanced_cluster libgossip)
    
    add_executable(simple_cluster_c examples/simple_cluster_c.c)
    target_link_libraries(simple_cluster_c libgossip)
    
    add_executable(network_example examples/network_example.cpp)
    target_link_libraries(network_example libgossip libgossip_net)
    
    # New examples
    add_executable(udp_transport_example examples/udp_transport_example.cpp)
    target_link_libraries(udp_transport_example libgossip libgossip_net)
    
    add_executable(tcp_transport_example examples/tcp_transport_example.cpp)
    target_link_libraries(tcp_transport_example libgossip libgossip_net)
    
    add_executable(transport_factory_example examples/transport_factory_example.cpp)
    target_link_libraries(transport_factory_example libgossip libgossip_net)
    
    add_executable(serializer_example examples/serializer_example.cpp)
    target_link_libraries(serializer_example libgossip libgossip_net)
endif()

# Tests
if(BUILD_TESTS)
    find_or_fetch_googletest()
    
    # Only build tests if GoogleTest is available
    if(TARGET GTest::gtest)
        enable_testing()
        
        add_executable(gossip_core_tests 
            tests/gossip_core_test.cpp)
        target_link_libraries(gossip_core_tests 
            libgossip 
            GTest::gtest 
            GTest::gtest_main)
        
        add_executable(transport_tests 
            tests/transport_test.cpp)
        target_link_libraries(transport_tests 
            libgossip 
            libgossip_net
            GTest::gtest 
            GTest::gtest_main)
        
        add_executable(serializer_tests 
            tests/serializer_test.cpp)
        target_link_libraries(serializer_tests 
            libgossip 
            libgossip_net
            GTest::gtest 
            GTest::gtest_main)
        
        # Register tests
        add_test(NAME gossip_core_tests COMMAND gossip_core_tests)
        add_test(NAME transport_tests COMMAND transport_tests)
        add_test(NAME serializer_tests COMMAND serializer_tests)
    else()
        message(WARNING "GoogleTest not found, skipping unit tests")
    endif()
endif()