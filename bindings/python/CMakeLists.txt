# Python bindings for libgossip

# Create the Python module with a different name to avoid conflicts
pybind11_add_module(libgossip_py 
    gossip_py.cpp
)

# Link against libgossip
target_link_libraries(libgossip_py PRIVATE libgossip)

# Include directories
target_include_directories(libgossip_py PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Add definition for conditional compilation
target_compile_definitions(libgossip_py PRIVATE LIBGOSSIP_BUILD)

# Install the Python module
install(TARGETS libgossip_py
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/python
)

# Also install to site-packages for development
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install prefix" FORCE)
endif()

# Copy the built module back to the source directory for development
add_custom_command(TARGET libgossip_py POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:libgossip_py>
    ${CMAKE_CURRENT_SOURCE_DIR}/libgossip/$<TARGET_FILE_NAME:libgossip_py>
    COMMENT "Copying Python module to libgossip package directory"
)