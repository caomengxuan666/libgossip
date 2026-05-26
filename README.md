# libgossip

## Language Index

- [English](#english)
- [中文版本](README_ZH.md)

<a id="english"></a>
# libgossip

libgossip is a C++17 implementation of the Gossip protocol, designed for decentralized distributed systems. It provides robust node discovery, failure detection, and metadata propagation with an emphasis on reliability and performance.

**Version: 1.4.1**

See [CHANGELOG.md](CHANGELOG.md) for release notes and
[RELEASE.md](RELEASE.md) for the release checklist.
Citation metadata is available in [CITATION.cff](CITATION.cff).

## Features

- **Gossip Protocol Core**: Implements SWIM (Scalable Weakly-consistent Infection-style process group Membership) protocol for decentralized node membership management
- **Failure Detection**: Efficiently detects node failures using heartbeat mechanism with suspicion timeouts
- **Metadata Propagation**: Distributes node metadata across the cluster using anti-entropy gossip
- **Event System**: Notifies applications of node status changes and metadata updates
- **Modular Design**: Separates core protocol implementation from network transport layer
- **Pluggable Serialization**: Support for custom serialization formats (JSON, Protobuf, FlatBuffers, MessagePack)
- **High-Level Manager**: `gossip_manager` class for easy lifecycle management
- **Node ID Utilities**: Functions for generating, parsing, and converting node IDs
- **C API**: Provides C bindings for non-C++ applications
- **Python Bindings**: Provides Python bindings for easy integration with Python applications

## Installation and Integration

### Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+

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

### CMake Presets

This project includes CMake presets for easier configuration and building:

- `CMakePresets.json` - Version-controlled presets suitable for all developers
- `CMakeUserPresets.json` - User-specific presets with local environment configurations (ignored in version control)

To use a preset, you can configure with:
```bash
cmake --preset=gcc-debug
```

And build with:
```bash
cmake --build --preset=gcc-debug
```

Available presets include:
- `gcc-debug`/`gcc-release` - GCC compiler with Debug/Release build types
- `clang-debug`/`clang-release` - Clang compiler with Debug/Release build types
- `msvc-debug`/`msvc-release` - MSVC compiler with Debug/Release build types (Windows only)

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
- [gossip_manager](include/core/gossip_manager.hpp) - High-level manager for easy lifecycle management
- [gossip_config](include/core/gossip_config.hpp) - Configuration structure for gossip_manager
- [node_id_utils](include/core/node_id_utils.hpp) - Node ID utilities (generate, parse, convert)
- [message_serializer](include/core/message_serializer.hpp) - Abstract serializer interface (pluggable)
- [node_view](include/core/gossip_core.hpp) - Node representation with metadata
- [gossip_message](include/core/gossip_core.hpp) - Message structure for network transport

### Network Module

The network module provides transport layer implementations for the Gossip protocol.

- [Transport interface](include/net/udp_transport.hpp) - Abstract transport interface
- [UDP Transport](include/net/udp_transport.hpp) - UDP-based transport implementation using ASIO
- [TCP Transport](include/net/tcp_transport.hpp) - TCP-based reliable transport implementation
- [Transport Factory](include/net/transport_factory.hpp) - Factory for creating transport instances (recommended)
- [Serializer Factory](include/net/serializer_factory.hpp) - Factory for registering and creating serializers
- [JSON Serializer](include/net/json_serializer.hpp) - JSON-based message serialization implementation

## Usage Examples

See the [examples](examples/) directory for detailed usage examples:

- [GossipManager Example](examples/gossip_manager_example.cpp) - High-level manager API (recommended)
- [Simple Cluster](examples/simple_cluster.cpp) - Basic full-mesh cluster setup
- [Seed-based Cluster](examples/seed_cluster.cpp) - Real-world deployment using seed nodes
- [Advanced Cluster](examples/advanced_cluster.cpp) - Advanced features including metadata and graceful leave
- [Simple Cluster (C API)](examples/simple_cluster_c.c) - Using C API bindings
- [Network Example](examples/network_example.cpp) - Network layer usage example
- [Serializer Example](examples/serializer_example.cpp) - Custom serializer usage

Each example demonstrates different aspects of library usage.

## API Reference

### Core Classes

- [gossip_core](include/core/gossip_core.hpp) - Main protocol implementation
- [gossip_manager](include/core/gossip_manager.hpp) - High-level manager for easy lifecycle management
- [gossip_config](include/core/gossip_config.hpp) - Configuration structure
- [node_id_utils](include/core/node_id_utils.hpp) - Node ID utilities
- [message_serializer](include/core/message_serializer.hpp) - Abstract serializer interface (pluggable)
- [node_view](include/core/gossip_core.hpp) - Node representation with metadata
- [gossip_message](include/core/gossip_core.hpp) - Message structure for network transport

### Network Classes

- [transport](include/net/udp_transport.hpp) - Abstract transport interface
- [udp_transport](include/net/udp_transport.hpp) - UDP-based transport implementation (**deprecated**, use transport_factory instead)
- [tcp_transport](include/net/tcp_transport.hpp) - TCP-based transport implementation (**deprecated**, use transport_factory instead)
- [transport_factory](include/net/transport_factory.hpp) - Factory for creating transport instances (recommended)
- [serializer_factory](include/net/serializer_factory.hpp) - Factory for registering and creating serializers
- [json_serializer](include/net/json_serializer.hpp) - JSON-based message serialization implementation

### Logging System

libgossip includes a configurable logging system for debugging and monitoring:

- [logger](include/core/logger.hpp) - Logging system with file and stderr output
- Controlled by `LIBGOSSIP_ENABLE_LOGGING` macro (default: enabled)
- Supports multiple log levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- Thread-safe singleton design

```cpp
// Initialize logger (optional, defaults to stderr output)
libgossip::Logger::Instance().Init("libgossip.log", libgossip::LogLevel::DEBUG);

// Use logging macros
LIBGOSSIP_LOG_DEBUG("Node " << node_id << " joined");
LIBGOSSIP_LOG_INFO("Cluster size: " << core.size());
```

### C API Functions

- `gossip_core_create()` - Create a new gossip core instance
- `gossip_core_destroy()` - Destroy gossip core instance
- `gossip_core_tick()` - Drive the protocol (should be called periodically)
- `gossip_core_handle_message()` - Handle incoming messages
- `gossip_core_meet()` - Introduce a new node to the cluster
- `gossip_core_join()` - Join an existing node
- `gossip_core_leave()` - Gracefully leave the cluster
- `gossip_core_update_self_metadata()` - Update self node metadata (thread-safe)

### Key Methods

- `gossip_core::tick()` - Drive the protocol (should be called periodically)
- `gossip_core::handle_message()` - Handle incoming messages
- `gossip_core::meet()` - Introduce a new node to the cluster
- `gossip_core::join()` - Join an existing node
- `gossip_core::leave()` - Gracefully leave the cluster
- `gossip_core::update_self_metadata()` - Update self node metadata (thread-safe, can be called from any thread)

## Example Guide

Detailed examples are provided in the [examples](examples/) directory:

1. **gossip_manager_example.cpp** - Shows the high-level manager API (recommended)
2. **simple_cluster.cpp** - Shows basic cluster formation with full-mesh connectivity
3. **seed_cluster.cpp** - Demonstrates real-world deployment using seed nodes
4. **advanced_cluster.cpp** - Shows advanced features like metadata and graceful leave
5. **simple_cluster_c.c** - Shows how to use the C API bindings
6. **network_example.cpp** - Shows how to use the network layer and different transport protocols
7. **serializer_example.cpp** - Shows how to use custom serializers

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
   ninja coverage_report  # For Ninja builds
   # or
   ninja test_and_coverage
   # or
   make coverage_report   # For Make builds
   # or
   make test_and_coverage
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
Coverage is generated from the provided build targets and filtered report script.

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

# Create a node using high-level SDK
node = libgossip.create_node("127.0.0.1", 8000)
node.start()

# Or use the JSON serializer directly
serializer = libgossip.JsonSerializer()
msg = libgossip.GossipMessage()
msg.sender = libgossip.NodeId.generate_random()
msg.type = libgossip.MessageType.PING

error_code, data = serializer.serialize(msg)
error_code, decoded_msg = serializer.deserialize(data)
```

For more detailed examples, see the [Python examples](bindings/python/) directory.

## Contributing

Contributions are welcome! Please read our [contributing guidelines](CONTRIBUTING.md) before submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
