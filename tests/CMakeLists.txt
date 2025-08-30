# Only build tests if GoogleTest is available
if(TARGET GTest::gtest)
  enable_testing()

  # Function to create a test target from source file
  function(create_test_from_source SOURCE_FILE)
    # Extract the base name without extension to use as target name
    get_filename_component(TEST_NAME ${SOURCE_FILE} NAME_WE)

    add_executable(${TEST_NAME} ${SOURCE_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE libgossip libgossip_net
                                               GTest::gtest GTest::gtest_main)

    target_include_directories(${TEST_NAME}
                               PRIVATE ${PROJECT_SOURCE_DIR}/include)

    # Register test
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
  endfunction()

  # Automatically find and create test targets for all _test.cpp files
  file(GLOB TEST_SOURCES "*_test.cpp")
  foreach(TEST_SOURCE ${TEST_SOURCES})
    create_test_from_source(${TEST_SOURCE})
  endforeach()

  # Apply coverage settings to all test targets at once if enabled
  if(ENABLE_COVERAGE)
    # Get all created test targets
    set(TEST_TARGETS gossip_core_tests transport_tests serializer_tests c_binding_tests)
    include(CodeCoverage)
    apply_coverage_to_targets(${TEST_TARGETS})
  endif()

else()
  message(WARNING "GoogleTest not found, skipping unit tests")
endif()