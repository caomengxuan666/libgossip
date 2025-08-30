cmake_minimum_required(VERSION 3.15)
project(
  libgossip
  VERSION 1.1.1
  LANGUAGES CXX C)

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
option(ENABLE_COVERAGE "Enable code coverage reporting" ON)

# Load dependency management functions
include(FindDependencies)

set(LIBGOSSIP_CORE_SRC src/core/gossip_core.cpp src/core/gossip_c.cpp
                       include/libgossip_api.h)

# Create the main library
add_library(libgossip ${LIBGOSSIP_CORE_SRC})
add_library(libgossip::core ALIAS libgossip)

# Handle ASIO dependency
setup_bundled_asio()

# Network library
set(LIBGOSSIP_NET_SRC src/net/udp_transport.cpp src/net/tcp_transport.cpp
                      src/net/transport_factory.cpp)

add_library(libgossip_net ${LIBGOSSIP_NET_SRC})
add_library(libgossip::network ALIAS libgossip_net)

# Find and link ASIO
if(ASIO_FOUND)
  target_include_directories(
    libgossip_net PUBLIC $<BUILD_INTERFACE:${ASIO_INCLUDE_DIR}>
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
target_include_directories(
  libgossip PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                   $<INSTALL_INTERFACE:include>)

target_compile_definitions(libgossip PUBLIC LIBGOSSIP_BUILD)
# Set target properties
set_target_properties(libgossip PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION
                                                                      1)

set_target_properties(libgossip_net PROPERTIES VERSION ${PROJECT_VERSION}
                                               SOVERSION 1)

# Create export header
include(GenerateExportHeader)
generate_export_header(libgossip)

# Export targets for build directory
export(
  TARGETS libgossip libgossip_net
  NAMESPACE libgossip::
  FILE libgossipTargets.cmake)

# Install targets
include(GNUInstallDirs)
install(
  TARGETS libgossip libgossip_net
  EXPORT libgossipTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES
  DESTINATION include)

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Create and install config files
install(
  EXPORT libgossipTargets
  FILE libgossipTargets.cmake
  NAMESPACE libgossip::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  libgossipConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

configure_package_config_file(
  cmake/libgossipConfig.cmake.in libgossipConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libgossipConfigVersion.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/libgossip_export.h
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libgossip)

# Examples
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
# Tests
if(BUILD_TESTS)
  find_or_fetch_googletest()
  enable_testing()
  add_subdirectory(tests)

  # Enable coverage for test targets if requested
  if(ENABLE_COVERAGE)
    include(CodeCoverage)
    enable_coverage_for_targets()

    # Do not automatically run tests as part of the build process Tests should
    # be run manually with 'ctest' after the build is complete

    # Generate coverage report target (to be run after tests)
    add_custom_target(
      coverage_report
      COMMAND ${CMAKE_COMMAND} -E echo "Generating coverage report..."
      COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info
              --ignore-errors mismatch,version
      COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file
              coverage_filtered.info --ignore-errors unused
      COMMAND ${LCOV_PATH} --remove coverage_filtered.info '*/googletest/*'
              --output-file coverage_filtered.info --ignore-errors unused
      COMMAND ${LCOV_PATH} --remove coverage_filtered.info '*/third_party/*'
              --output-file coverage_filtered.info --ignore-errors unused
      COMMAND ${GENHTML_PATH} coverage_filtered.info --output-directory
              ${CMAKE_BINARY_DIR}/coverage_report_filtered
      COMMAND
        ${CMAKE_COMMAND} -E echo
        "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage_report_filtered/index.html"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating code coverage report..."
      VERBATIM)

    # Add a target that runs tests and then generates coverage report
    add_custom_target(
      test_and_coverage
      COMMAND ${CMAKE_CTEST_COMMAND} -V
      COMMAND ${CMAKE_COMMAND} --build . --target coverage_report
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running tests and generating coverage report..."
      DEPENDS gossip_core_test transport_test serializer_test
      VERBATIM)

    message(STATUS "Coverage analysis enabled")
    message(
      STATUS
        "Use 'make test_and_coverage' or 'ninja test_and_coverage' to run tests and generate coverage report"
    )
    message(
      STATUS
        "Or run tests with 'ctest -V' and then generate report with 'make coverage_report'"
    )
    message(
      STATUS
        "Final report will be located at: ${CMAKE_BINARY_DIR}/coverage_report_filtered/index.html"
    )
  endif()
endif()

if(BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python)
  
  # Add Python tests to the main test target if both BUILD_TESTS and BUILD_PYTHON_BINDINGS are enabled
  if(BUILD_TESTS)
    find_package(Python COMPONENTS Interpreter)
    if(Python_Interpreter_FOUND)
      # Check if pytest is available, if not, try to install it from requirements
      execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import pytest"
        RESULT_VARIABLE PYTEST_AVAILABLE
        OUTPUT_QUIET
        ERROR_QUIET
      )
      
      if(NOT PYTEST_AVAILABLE EQUAL 0)
        message(STATUS "pytest not found, trying to install test dependencies...")
        execute_process(
          COMMAND ${Python_EXECUTABLE} -m pip install -r ${CMAKE_SOURCE_DIR}/requirements-test.txt
          RESULT_VARIABLE PIP_INSTALL_RESULT
          OUTPUT_VARIABLE PIP_INSTALL_OUTPUT
          ERROR_VARIABLE PIP_INSTALL_ERROR
        )
        
        if(NOT PIP_INSTALL_RESULT EQUAL 0)
          message(WARNING "Failed to install test dependencies automatically. Please install them manually with: pip install -r requirements-test.txt")
        else()
          message(STATUS "Successfully installed test dependencies")
        endif()
      endif()
      
      # Add a CTest target for Python tests
      # Set PYTHONPATH to use the built module instead of any installed version
      add_test(
        NAME python_tests
        COMMAND ${Python_EXECUTABLE} -m pytest ${CMAKE_SOURCE_DIR}/bindings/python/test_libgossip.py -v
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bindings/python
      )
      
      # Set environment for the test to use the built module
      set_tests_properties(python_tests PROPERTIES
        ENVIRONMENT "PYTHONPATH=${CMAKE_SOURCE_DIR}/bindings/python:$ENV{PYTHONPATH}"
        PASS_REGULAR_EXPRESSION ".* passed.*"
        FAIL_REGULAR_EXPRESSION ".* failed.*"
      )
      
      add_custom_target(
        run_python_tests
        COMMAND ${Python_EXECUTABLE} -m pytest test_libgossip.py -v
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bindings/python
        COMMENT "Running Python tests"
        USES_TERMINAL
      )
      
      # Add a target that runs both C++ and Python tests
      add_custom_target(
        all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} -V
        COMMAND ${Python_EXECUTABLE} -m pytest test_libgossip.py -v
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/bindings/python
        COMMENT "Running all tests (C++ and Python)"
        USES_TERMINAL
      )
      
      message(STATUS "Python tests integrated. Use 'make run_python_tests' to run Python tests")
      message(STATUS "Use 'make all_tests' to run both C++ and Python tests")
      message(STATUS "Python tests are registered with CTest. Use 'ctest -V' to run all tests including Python tests")
    endif()
  endif()
endif()