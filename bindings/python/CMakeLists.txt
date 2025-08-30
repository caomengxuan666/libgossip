# Python bindings for libgossip

# Use bundled pybind11 from third_party directory
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/pybind11)
  message(STATUS "Using bundled pybind11 from third_party/pybind11")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/pybind11
                   ${CMAKE_CURRENT_BINARY_DIR}/pybind11)
else()
  message(
    FATAL_ERROR
      "pybind11 not found in third_party. Please initialize submodules with: git submodule update --init"
  )
endif()

# Create the Python module with a different name to avoid conflicts
pybind11_add_module(libgossip_py gossip_py.cpp)

# Link against libgossip
target_link_libraries(libgossip_py PRIVATE libgossip libgossip_net)

# Include directories
target_include_directories(libgossip_py PRIVATE ${CMAKE_SOURCE_DIR}/include
                                                ${CMAKE_SOURCE_DIR}/src)

# Add definition for conditional compilation
target_compile_definitions(libgossip_py PRIVATE LIBGOSSIP_BUILD)

# Install the Python module
install(TARGETS libgossip_py DESTINATION ${CMAKE_INSTALL_LIBDIR}/python)

# Also install to site-packages for development
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE PATH "Default install prefix" FORCE)
endif()

# Copy the built module back to the source directory for development
add_custom_command(
  TARGET libgossip_py
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:libgossip_py>
          ${CMAKE_CURRENT_SOURCE_DIR}/libgossip/$<TARGET_FILE_NAME:libgossip_py>
  COMMENT "Copying Python module to libgossip package directory")

# Add pytest support when BUILD_TESTS is enabled
if(BUILD_TESTS)
  find_package(Python COMPONENTS Interpreter)
  if(Python_Interpreter_FOUND)
    # Add a custom target for running Python tests
    add_custom_target(
      python_tests
      COMMAND ${Python_EXECUTABLE} -m pytest
              ${CMAKE_CURRENT_SOURCE_DIR}/test_libgossip.py
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running Python tests with pytest"
      USES_TERMINAL)

    # Add a target that builds the Python module and then runs tests
    add_custom_target(
      build_and_test_python
      COMMAND ${CMAKE_COMMAND} --build . --target libgossip_py
      COMMAND ${Python_EXECUTABLE} -m pytest
              ${CMAKE_CURRENT_SOURCE_DIR}/test_libgossip.py -v
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building Python module and running tests"
      USES_TERMINAL)

    message(
      STATUS
        "Python tests enabled. Use 'make python_tests' or 'ninja python_tests' to run Python tests"
    )
    message(
      STATUS
        "Use 'make build_and_test_python' or 'ninja build_and_test_python' to build and test Python bindings"
    )
  endif()
endif()
