# libgossip

## Language Index

- [English](#english)
- [中文版本](README_ZH.md)

<a id="english"></a>
# libgossip

libgossip is a C++17 implementation of the Gossip protocol, designed for decentralized distributed systems. It provides robust node discovery, failure detection, and metadata propagation with an emphasis on reliability and performance.

## Features

- **Gossip Protocol Core**: Implements SWIM (Scalable Weakly-consistent Infection-style process group Membership) protocol for decentralized node membership management
- **Failure Detection**: Efficiently detects node failures using heartbeat mechanism with suspicion timeouts
- **Metadata Propagation**: Distributes node metadata across the cluster using anti-entropy gossip
- **Event System**: Notifies applications of node status changes and metadata updates
- **Modular Design**: Separates core protocol implementation from network transport layer
- **C API**: Provides C bindings for non-C++ applications
- **Python Bindings**: Provides Python bindings for easy integration with Python applications

## Installation and Integration

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.12+

### Building from Source

```bash
git clone https://github.com/caomengxuan666/libgossip.git
cd libgossip
git submodule update --init  # Initialize pybind11 submodule
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### Integration with CMake

You can integrate libgossip into your project in two ways:

1. Using find_package (after installation):

```cmake
find_package(libgossip REQUIRED)
target_link_libraries(your_target libgossip::core)
# For network functionality:
target_link_libraries(your_target libgossip::network)
```

2. Using add_subdirectory (without installation):

```cmake
add_subdirectory(path/to/libgossip)
target_link_libraries(your_target libgossip::core)
# For network functionality:
target_link_libraries(your_target libgossip::network)
```

### Installation

To install libgossip system-wide:

```bash
mkdir build
cd build
cmake ..
cmake --build .
sudo cmake --install .
```

## Modules

### Core Module

The core module implements the Gossip protocol logic including membership management, failure detection, and metadata propagation.

- [gossip_core](include/core/gossip_core.hpp) - Main protocol implementation
- [node_view](include/core/gossip_core.hpp) - Node representation with metadata
- [gossip_message](include/core/gossip_core.hpp) - Message structure for network transport

### Network Module

The network module provides transport layer implementations for the Gossip protocol.

- [Transport interface](include/net/udp_transport.hpp) - Abstract transport interface
- [UDP Transport](include/net/udp_transport.hpp) - UDP-based transport implementation using ASIO
- [TCP Transport](include/net/tcp_transport.hpp) - TCP-based reliable transport implementation
- [Transport Factory](include/net/transport_factory.hpp) - Factory for creating transport instances
- [Message Serializer](include/net/udp_transport.hpp) - Abstract message serialization interface
- [JSON Serializer](include/net/json_serializer.hpp) - JSON-based message serialization implementation

## Usage Examples

See the [examples](examples/) directory for detailed usage examples:

- [Simple Cluster](examples/simple_cluster.cpp) - Basic full-mesh cluster setup
- [Seed-based Cluster](examples/seed_cluster.cpp) - Real-world deployment using seed nodes
- [Advanced Cluster](examples/advanced_cluster.cpp) - Advanced features including metadata and graceful leave
- [Simple Cluster (C API)](examples/simple_cluster_c.c) - Using C API bindings
- [Network Example](examples/network_example.cpp) - Network layer usage example

Each example demonstrates different aspects of library usage.

## API Reference

### Core Classes

- [gossip_core](include/core/gossip_core.hpp) - Main protocol implementation
- [node_view](include/core/gossip_core.hpp) - Node representation with metadata
- [gossip_message](include/core/gossip_core.hpp) - Message structure for network transport

### Network Classes

- [transport](include/net/udp_transport.hpp) - Abstract transport interface
- [udp_transport](include/net/udp_transport.hpp) - UDP-based transport implementation
- [tcp_transport](include/net/tcp_transport.hpp) - TCP-based transport implementation
- [transport_factory](include/net/transport_factory.hpp) - Factory for creating transport instances
- [message_serializer](include/net/udp_transport.hpp) - Abstract message serialization interface
- [json_serializer](include/net/json_serializer.hpp) - JSON-based message serialization implementation

### C API Functions

- `gossip_core_create()` - Create a new gossip core instance
- `gossip_core_destroy()` - Destroy gossip core instance
- `gossip_core_tick()` - Drive the protocol (should be called periodically)
- `gossip_core_handle_message()` - Handle incoming messages
- `gossip_core_meet()` - Introduce a new node to the cluster
- `gossip_core_join()` - Join an existing node
- `gossip_core_leave()` - Gracefully leave the cluster

### Key Methods

- `gossip_core::tick()` - Drive the protocol (should be called periodically)
- `gossip_core::handle_message()` - Handle incoming messages
- `gossip_core::meet()` - Introduce a new node to the cluster
- `gossip_core::join()` - Join an existing node
- `gossip_core::leave()` - Gracefully leave the cluster

## Example Guide

Detailed examples are provided in the [examples](examples/) directory:

1. **simple_cluster.cpp** - Shows basic cluster formation with full-mesh connectivity
2. **seed_cluster.cpp** - Demonstrates real-world deployment using seed nodes
3. **advanced_cluster.cpp** - Shows advanced features like metadata and graceful leave
4. **simple_cluster_c.c** - Shows how to use the C API bindings
5. **network_example.cpp** - Shows how to use the network layer and different transport protocols

Building and running examples:

```bash
cd build
cmake ..
cmake --build .
./examples/simple_cluster
./examples/seed_cluster
./examples/advanced_cluster
./examples/simple_cluster_c
./examples/network_example
```

## Roadmap: From Core Library to Production System

libgossip-core is already a solid foundation for distributed systems. Here's the roadmap for future development:

### 1. libgossip-net: Network Transport Layer
The core layer is network-agnostic, now we need concrete transport implementations.

✅ **Features:**
- UDP transport: Lightweight unreliable transport for local networks
- TCP transport: Reliable transport for WAN deployments
- Serialization layer: Pluggable message serialization (JSON, FlatBuffers, Cap'n Proto)

### 2. libgossip-cluster: Cluster Management
Build higher-level cluster management abstractions.

✅ **Features:**
- Cluster formation: Automated cluster creation and bootstrap
- Node discovery: Service discovery for cluster nodes
- Configuration sync: Distributed configuration management
- Leader election: Simple leader election mechanisms

### 3. libgossip-utils: Utility Functions
Provide utility functions for common distributed system tasks.

✅ **Features:**
- Time synchronization: Clock synchronization across nodes
- Load balancing: Distribute workload across cluster
- Consistent hashing: Key distribution for sharding
- Rate limiting: Control message flow in high-load scenarios

### 4. libgossip-monitoring: Monitoring and Observability
Provide monitoring and observability features for production deployments.

✅ **Features:**
- Metrics collection: Collect and expose system metrics
- Health checks: Node and cluster health monitoring
- Logging: Structured logging with different levels
- Tracing: Distributed tracing for request flows

### 5. Testing and Benchmarking
Ensure reliability and performance through comprehensive testing.

✅ **Features:**
- Unit tests: Comprehensive unit test coverage
- Integration tests: End-to-end integration testing
- Performance benchmarks: Regular performance regression testing
- Chaos engineering: Fault injection for resilience testing

### 6. Documentation and Tutorials
Create comprehensive documentation and learning resources.

✅ **Features:**
- API documentation: Complete API reference with examples
- User guides: Step-by-step guides for common use cases
- Architecture docs: Deep dive into system architecture
- Video tutorials: Visual learning resources

### 7. Security Layer
Provide security guarantees for production environments.

✅ **Features:**
- TLS encryption: Protect transport security
- Node authentication: Based on certificates or shared keys
- Message signing: Prevent forgery

### 8. Higher-level Applications
Build real distributed systems using libgossip.

✅ **Project Ideas:**
- Astra-CacheServer: we would refactor the existing cache server to use libgossip for node discovery and fault tolerance
- gossip-job: Distributed task scheduling, ensuring tasks are executed by only one node
- gossip-raft: Implement Raft protocol on top of Gossip for strong consistency

### 9. Cross-platform and Embedded Support
Port libgossip to more platforms.

✅ **Target Platforms:**
- Embedded Linux (IoT devices)
- WebAssembly (browser-based node discovery)
- Mobile devices (Android/iOS P2P networks)

### 10. Open Source and Community Building
Share your achievements with the world.

## Code Coverage

This project includes code coverage analysis tools to help ensure quality and thorough testing.

### Generating Coverage Reports

To generate a code coverage report:

1. Configure with coverage enabled:
   ```bash
   mkdir build
   cd build
   cmake .. -DENABLE_COVERAGE=ON
   ```

2. Build the project:
   ```bash
   cmake --build .
   ```

3. Run tests:
   ```bash
   ctest -V
   ```

4. Generate coverage report:
   ```bash
   ninja coverage  # For Ninja builds
   # or
   make coverage   # For Make builds
   ```

5. View the HTML report by opening `build/coverage_report_filtered/index.html` in your browser.

### Using the Coverage Script

Alternatively, you can use the provided Python script to generate a filtered coverage report:

```bash
python script/generate_coverage.py --build-dir build
```

This script will generate a report that excludes system libraries, third-party code, and Google Test framework code, 
focusing only on the actual project code.

### Coverage Goals

The project aims for:
- Line coverage: >90%
- Function coverage: >90%

Current status: 97.5% line coverage, 97.5% function coverage (as of latest test run).

## Python Bindings

libgossip also provides Python bindings for easy integration with Python applications.

### Building Python Bindings

To build the Python bindings, you need to have pybind11 installed:

```bash
# Clone the pybind11 repository as a submodule
git submodule update --init

# Build with Python bindings enabled (default)
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

### Using Python Bindings

Example Python usage:

```python
import libgossip

# Create a node view
node = libgossip.node_view()
node.ip = "127.0.0.1"
node.port = 8000

# Create a gossip core instance
core = libgossip.gossip_core(node)

# Drive the protocol periodically
core.tick()
```

For more detailed examples, see the [Python examples](bindings/python/examples/) directory.

## Contributing

Contributions are welcome! Please read our [contributing guidelines](CONTRIBUTING.md) before submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.