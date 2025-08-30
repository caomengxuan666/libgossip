# Copyright (c) 2025 libgossip project
# SPDX-License-Identifier: MIT

# Check prereqs
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(NOT GCOV_PATH)
    message(WARNING "gcov not found! Cannot generate coverage reports.")
endif()

if(NOT LCOV_PATH)
    message(WARNING "lcov not found! Cannot generate HTML coverage reports.")
endif()

if(NOT GENHTML_PATH)
    message(WARNING "genhtml not found! Cannot generate HTML coverage reports.")
endif()

# Setup coverage compiler flags
function(setup_coverage_target target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(${target} PRIVATE 
            --coverage
            -g
            -O0
        )
        
        target_link_libraries(${target} PRIVATE --coverage)
    endif()
endfunction()

# Function to apply coverage settings to multiple targets
function(apply_coverage_to_targets)
    if(NOT GCOV_PATH)
        message(WARNING "Coverage analysis skipped: gcov not found")
        return()
    endif()
    
    # Apply coverage to all provided targets
    foreach(target IN LISTS ARGN)
        if(TARGET ${target})
            setup_coverage_target(${target})
        endif()
    endforeach()
endfunction()

# Function to enable coverage for multiple targets
function(enable_coverage_for_targets)
    if(NOT GCOV_PATH)
        message(WARNING "Coverage analysis skipped: gcov not found")
        return()
    endif()
    

    if(TARGET gossip_core_test)
        setup_coverage_target(gossip_core_test)
    endif()
    
    if(TARGET transport_test)
        setup_coverage_target(transport_test)
    endif()
    
    if(TARGET serializer_test)
        setup_coverage_target(serializer_test)
    endif()
    
    # Add coverage target with filtering
    add_custom_target(coverage
        COMMAND ${LCOV_PATH} --capture --directory . --output-file coverage.info --ignore-errors mismatch
        COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' --output-file coverage_filtered.info
        COMMAND ${LCOV_PATH} --remove coverage_filtered.info '*/googletest/*' --output-file coverage_filtered.info
        COMMAND ${LCOV_PATH} --remove coverage_filtered.info '*/third_party/*' --output-file coverage_filtered.info
        COMMAND ${GENHTML_PATH} coverage_filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report_filtered
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating filtered code coverage report..."
        VERBATIM
    )
    
    # Add coverage cleanup target
    add_custom_target(coverage_clean
        COMMAND ${LCOV_PATH} --directory . --zerocounters
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Resetting code coverage counters..."
        VERBATIM
    )
endfunction()