# FindDependencies.cmake
# This module handles finding dependencies in a specific order:
# 1. System libraries
# 2. VCPKG libraries
# 3. Package manager libraries
# 4. Downloaded libraries

include(FetchContent)

# Function to find dependency with specific order
function(find_dependency_ordered DEPENDENCY_NAME)
    set(options SYSTEM_ONLY VCPKG_ONLY PKGCONFIG_ONLY DOWNLOAD_ONLY)
    set(oneValueArgs VERSION)
    set(multiValueArgs COMPONENTS)
    cmake_parse_arguments(FIND_DEP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    message(STATUS "Looking for ${DEPENDENCY_NAME}...")
    
    # 1. Try system libraries first
    if(NOT FIND_DEP_SYSTEM_ONLY)
        find_package(${DEPENDENCY_NAME} ${FIND_DEP_VERSION} QUIET COMPONENTS ${FIND_DEP_COMPONENTS})
        if(${DEPENDENCY_NAME}_FOUND)
            message(STATUS "Found ${DEPENDENCY_NAME} in system libraries")
            return()
        endif()
    endif()
    
    # 2. Try VCPKG libraries
    if(DEFINED VCPKG_INSTALLED_DIR AND NOT FIND_DEP_VCPKG_ONLY)
        find_package(${DEPENDENCY_NAME} ${FIND_DEP_VERSION} 
                    PATHS ${VCPKG_INSTALLED_DIR} 
                    QUIET COMPONENTS ${FIND_DEP_COMPONENTS})
        if(${DEPENDENCY_NAME}_FOUND)
            message(STATUS "Found ${DEPENDENCY_NAME} in VCPKG")
            return()
        endif()
    endif()
    
    # 3. Try package manager libraries (pkg-config)
    if(NOT FIND_DEP_PKGCONFIG_ONLY AND UNIX)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(${DEPENDENCY_NAME} QUIET ${DEPENDENCY_NAME})
            if(${DEPENDENCY_NAME}_FOUND)
                message(STATUS "Found ${DEPENDENCY_NAME} via pkg-config")
                return()
            endif()
        endif()
    endif()
    
    # 4. Download if requested
    if(NOT FIND_DEP_DOWNLOAD_ONLY)
        message(WARNING "${DEPENDENCY_NAME} not found in system, VCPKG, or pkg-config. "
                        "Consider installing it or enabling automatic download.")
    endif()
endfunction()

# Function to fetch content with error handling
function(fetch_content_safe NAME)
    set(oneValueArgs URL GIT_REPOSITORY GIT_TAG)
    set(multiValueArgs PATCHES)
    cmake_parse_arguments(FETCH "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    # Try to fetch content
    FetchContent_Declare(
        ${NAME}
        ${ARGN}
        DOWNLOAD_EXTRACT_TIMESTAMP true
    )
    
    FetchContent_GetProperties(${NAME})
    if(NOT ${NAME}_POPULATED)
        message(STATUS "Fetching ${NAME}...")
        FetchContent_Populate(${NAME})
    endif()
endfunction()

# Special handling for GoogleTest
function(find_or_fetch_googletest)
    option(USE_SYSTEM_GOOGLETEST "Use system-installed GoogleTest instead of downloading" OFF)
    
    if(USE_SYSTEM_GOOGLETEST)
        # Try to find system GoogleTest
        find_package(GTest QUIET)
        if(GTest_FOUND)
            message(STATUS "Using system GoogleTest")
            return()
        else()
            message(WARNING "System GoogleTest not found, but USE_SYSTEM_GOOGLETEST is ON")
        endif()
    endif()
    
    # Try to find GoogleTest in common locations
    find_package(GTest QUIET)
    if(GTest_FOUND)
        message(STATUS "Found GoogleTest in standard location")
        return()
    endif()
    
    # Try to fetch GoogleTest
    message(STATUS "GoogleTest not found, attempting to fetch...")
    fetch_content_safe(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    
    # Check if fetch was successful
    FetchContent_GetProperties(googletest)
    if(googletest_POPULATED)
        # Add googletest directly to the build
        add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
        message(STATUS "Successfully fetched and added GoogleTest")
    else()
        message(WARNING "Failed to fetch GoogleTest")
    endif()
endfunction()

# Function to setup ASIO from third_party directory
function(setup_bundled_asio)
    # Check if bundled ASIO exists
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio)
        message(STATUS "Using bundled ASIO from third_party/asio")
        # ASIO is header-only, just need to add the include directory
        set(ASIO_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/asio/include PARENT_SCOPE)
        set(ASIO_FOUND TRUE PARENT_SCOPE)
    else()
        message(FATAL_ERROR "ASIO not found in third_party/asio. Please initialize submodules with: git submodule update --init")
    endif()
endfunction()