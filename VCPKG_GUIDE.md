# Libgossip VCPKG Port Guide

## Overview

libgossip is a production-ready C++ Gossip protocol library designed for distributed systems. This guide helps you integrate libgossip into your project using VCPKG.

## Installation

### Basic Installation

```bash
vcpkg install libgossip
```

### Installation with Features

```bash
# Install with examples
vcpkg install libgossip[examples]

# Install with Python bindings
vcpkg install libgossip[python]

# Install with tests
vcpkg install libgossip[tests]

# Install with all features
vcpkg install libgossip[examples,python,tests]
```

## Integration with CMake

### Using find_package

```cmake
find_package(libgossip CONFIG REQUIRED)

# Link core library
target_link_libraries(your_target PRIVATE libgossip::core)

# Link network library (includes core)
target_link_libraries(your_target PRIVATE libgossip::network)
```

### Using add_subdirectory

```cmake
add_subdirectory(path/to/libgossip)

target_link_libraries(your_target PRIVATE libgossip::core)
```

## Features

### Core Features (Default)

- **Gossip Protocol**: Full SWIM protocol implementation
- **Node Discovery**: Automatic node discovery and management
- **Failure Detection**: Efficient failure detection with suspicion mechanism
- **Metadata Propagation**: Anti-entropy gossip for metadata distribution
- **Event System**: Node status change notifications

### Optional Features

#### Examples

Build example applications demonstrating libgossip usage:

```bash
vcpkg install libgossip[examples]
```

Examples are installed to:
- Windows: `<vcpkg-root>/tools/libgossip/examples/`
- Linux/Mac: `<vcpkg-root>/tools/libgossip/examples/`

Available examples:
- `real_cluster_example` - Real network cluster example
- `network_example` - Network layer usage
- `simple_cluster` - Basic cluster setup
- `seed_cluster` - Seed-based cluster formation

#### Python Bindings

Python bindings for easy integration with Python applications:

```bash
vcpkg install libgossip[python]
```

Python bindings are installed to:
- `<vcpkg-root>/tools/libgossip/python/`

Usage:
```python
import libgossip

# Create a node
node = libgossip.node_view()
node.ip = "127.0.0.1"
node.port = 8000

# Create gossip core
core = libgossip.gossip_core(node)

# Drive the protocol
core.tick()
```

#### Tests

Unit tests for verifying library functionality:

```bash
vcpkg install libgossip[tests]
```

## Dependencies

### Required Dependencies

- **nlohmann-json** (>= 3.11.0): JSON serialization

### Optional Dependencies

- **pybind11**: Python bindings (when `python` feature is enabled)
- **gtest**: Unit tests (when `tests` feature is enabled)

## Usage Examples

### Basic Gossip Core Usage

```cpp
#include <libgossip/core/gossip_core.hpp>
#include <libgossip/net/transport_factory.hpp>

using namespace libgossip;
using namespace gossip::net;

// Create node view
node_view self_node;
self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};
self_node.ip = "127.0.0.1";
self_node.port = 8000;
self_node.status = node_status::online;

// Create gossip core
auto core = std::make_shared<gossip_core>(
    self_node,
    [](const gossip_message& msg, const node_view& target) {
        // Send callback
    },
    [](const node_view& node, node_status old_status) {
        // Event callback
    }
);

// Create transport
auto transport = transport_factory::create_transport(
    transport_type::tcp, "127.0.0.1", 8000);
transport->set_gossip_core(core);
transport->set_serializer(std::make_unique<json_serializer>());
transport->start();

// Drive the protocol
while (running) {
    core->tick();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### Real Network Cluster

See `real_cluster_example` for a complete working example with multiple nodes communicating over real TCP network:

```bash
# Run 3-node cluster
./real_cluster_example 3 9000

# Run 5-node cluster
./real_cluster_example 5 9100
```

## Troubleshooting

### Port Already in Use

If you see "Address already in use" error:

1. Change the port number in your code
2. Kill the process using the port:
   ```bash
   # Linux/Mac
   lsof -ti:9000 | xargs kill -9
   
   # Windows
   netstat -ano | findstr :9000
   taskkill /PID <PID> /F
   ```

### Node Discovery Issues

If nodes cannot discover each other:

1. Check firewall settings
2. Ensure ports are open
3. Verify network connectivity
4. Check that seed node is running

### Compilation Errors

If you encounter compilation errors:

1. Ensure you're using C++17 or later
2. Check that all dependencies are installed
3. Verify VCPKG toolchain is properly set
4. Try cleaning and rebuilding:
   ```bash
   vcpkg remove libgossip --recurse
   vcpkg install libgossip
   ```

## Performance Considerations

### Resource Usage

- **Memory**: ~10-50MB per node (depending on cluster size)
- **CPU**: ~5-10% per node (idle), ~20-30% during heavy gossip
- **Network**: ~100-500 bytes per message, ~100-500 messages/sec

### Optimization Tips

1. **Gossip Interval**: Adjust `heartbeat_interval` for your use case
2. **Cluster Size**: Smaller clusters (< 50 nodes) converge faster
3. **Network**: Use UDP for low-latency, TCP for reliability
4. **Serialization**: JSON is readable but slower; consider binary formats for production

## Security Considerations

### Production Deployment

1. **TLS Encryption**: Enable TLS for network communication
2. **Node Authentication**: Implement node identity verification
3. **Message Signing**: Add message integrity checks
4. **Firewall**: Restrict access to trusted networks

### Current Limitations

- No built-in encryption (use TLS proxy for now)
- No node authentication (implement custom scheme)
- No message signing (add if needed)

## Support

- **GitHub Issues**: https://github.com/caomengxuan666/libgossip/issues
- **Documentation**: https://github.com/caomengxuan666/libgossip
- **Examples**: See `examples/` directory

## Contributing

Contributions are welcome! Please see the contributing guidelines in the repository.

## License

MIT License - see LICENSE file for details.