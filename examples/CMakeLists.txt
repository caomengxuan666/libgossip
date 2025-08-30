# Examples CMakeLists.txt

# Try to find libgossip package
#find_package(libgossip QUIET)

if(NOT libgossip_FOUND)
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

add_executable(seed_cluster seed_cluster.cpp)
target_link_libraries(seed_cluster ${LIBGOSSIP_TARGET})

add_executable(advanced_cluster advanced_cluster.cpp)
target_link_libraries(advanced_cluster ${LIBGOSSIP_TARGET})

add_executable(network_example network_example.cpp)
target_link_libraries(network_example libgossip libgossip_net)

add_executable(udp_transport_example udp_transport_example.cpp)
target_link_libraries(udp_transport_example libgossip libgossip_net)

add_executable(tcp_transport_example tcp_transport_example.cpp)
target_link_libraries(tcp_transport_example libgossip libgossip_net)

add_executable(transport_factory_example transport_factory_example.cpp)
target_link_libraries(transport_factory_example libgossip libgossip_net)

add_executable(serializer_example serializer_example.cpp)
target_link_libraries(serializer_example libgossip libgossip_net)

add_executable(large_scale_cluster_example large_scale_cluster_example.cpp)
target_link_libraries(large_scale_cluster_example libgossip libgossip_net)

add_executable(simple_cluster_stats simple_cluster_stats.cpp)
target_link_libraries(simple_cluster_stats libgossip libgossip_net)

add_executable(redis_cluster_gossip_example redis_cluster_gossip_example.cpp)
target_link_libraries(redis_cluster_gossip_example libgossip libgossip_net)

# ==========================C Bindings===================================== #

# Simple C cluster example
add_executable(simple_cluster_c simple_cluster_c.c)
target_link_libraries(simple_cluster_c PRIVATE libgossip)

# C API with network layer example
add_executable(net_c_api_example net_c_api_example.c)
target_link_libraries(net_c_api_example PRIVATE libgossip libgossip_net)

# C API examples are built only on Windows (due to different linking requirements)
if(WIN32)
  # Simple C cluster example for Windows
  add_executable(simple_cluster_c_win simple_cluster_c.c)
  target_link_libraries(simple_cluster_c_win PRIVATE libgossip)
endif()