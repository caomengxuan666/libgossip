# Version Check Module for libgossip
# Ensures compatibility with minimum required versions of dependencies

include(CheckCXXSourceCompiles)
include(CheckCXXSymbolExists)

function(check_compiler_version)
    message(STATUS "Checking compiler compatibility...")
    
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0")
            message(FATAL_ERROR "GCC version 7.0 or higher is required (found ${CMAKE_CXX_COMPILER_VERSION})")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
            message(FATAL_ERROR "Clang version 5.0 or higher is required (found ${CMAKE_CXX_COMPILER_VERSION})")
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.14")
            message(FATAL_ERROR "MSVC version 19.14 (2017) or higher is required (found ${CMAKE_CXX_COMPILER_VERSION})")
        endif()
    else()
        message(WARNING "Unknown compiler ${CMAKE_CXX_COMPILER_ID}, version check skipped")
    endif()
    
    message(STATUS "Compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} is compatible")
endfunction()

function(check_cxx_standard_support)
    message(STATUS "Checking C++17 support...")
    
    check_cxx_source_compiles("
        #include <memory>
        #include <optional>
        #include <string_view>
        #include <array>
        
        int main() {
            std::optional<int> opt;
            std::string_view sv = \"test\";
            std::array<int, 5> arr;
            return 0;
        }
    " CXX17_SUPPORTED)
    
    if(NOT CXX17_SUPPORTED)
        message(FATAL_ERROR "C++17 support is required but not detected")
    endif()
    
    message(STATUS "C++17 support confirmed")
endfunction()

function(check_asio_version)
    message(STATUS "Checking ASIO version...")
    
    if(ASIO_FOUND)
        # Try to detect ASIO version
        check_cxx_source_compiles("
            #include <asio.hpp>
            #if ASIO_VERSION < 101800
            #error \"ASIO version 1.18.0 or higher is required\"
            #endif
            int main() { return 0; }
        " ASIO_VERSION_CHECK)
        
        if(ASIO_VERSION_CHECK)
            message(STATUS "ASIO version check passed")
        else()
            message(WARNING "Could not verify ASIO version, assuming compatibility")
        endif()
    endif()
endfunction()

function(check_nlohmann_json_version)
    message(STATUS "Checking nlohmann/json version...")
    
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/single_include/nlohmann/json.hpp")
        check_cxx_source_compiles("
            #include <nlohmann/json.hpp>
            #if NLOHMANN_JSON_VERSION_MAJOR < 3
            #error \"nlohmann/json version 3.11.0 or higher is required\"
            #endif
            #if NLOHMANN_JSON_VERSION_MAJOR == 3 && NLOHMANN_JSON_VERSION_MINOR < 11
            #error \"nlohmann/json version 3.11.0 or higher is required\"
            #endif
            int main() { return 0; }
        " JSON_VERSION_CHECK)
        
        if(JSON_VERSION_CHECK)
            message(STATUS "nlohmann/json version check passed")
        else()
            message(WARNING "Could not verify nlohmann/json version, assuming compatibility")
        endif()
    endif()
endfunction()

function(run_all_version_checks)
    check_compiler_version()
    check_cxx_standard_support()
    check_asio_version()
    check_nlohmann_json_version()
    message(STATUS "All version checks passed successfully")
endfunction()