# Examples CMakeLists.txt

# Try to find libgossip package
find_package(libgossip QUIET)

if (NOT libgossip_FOUND)
    # If not found, use the local build
    message(STATUS "Using local libgossip build")
    set(LIBGOSSIP_TARGET libgossip)
else()
    # If found, use the installed package
    message(STATUS "Using installed libgossip package")
    set(LIBGOSSIP_TARGET libgossip::core)
endif()

add_executable(simple_cluster simple_cluster.cpp)
target_link_libraries(simple_cluster ${LIBGOSSIP_TARGET})

add_executable(simple_cluster_c simple_cluster_c.c)
target_link_libraries(simple_cluster_c ${LIBGOSSIP_TARGET})

add_executable(seed_cluster seed_cluster.cpp)
target_link_libraries(seed_cluster ${LIBGOSSIP_TARGET})

add_executable(advanced_cluster advanced_cluster.cpp)
target_link_libraries(advanced_cluster ${LIBGOSSIP_TARGET})