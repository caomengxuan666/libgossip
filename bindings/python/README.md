# libgossip Python Bindings

This package provides Python bindings for the libgossip C++ library, which implements the Gossip protocol for decentralized distributed systems.

## Installation

For regular usage, install the package from PyPI:

```bash
pip install libgossip
```

Then use it in your Python code:

```python
import libgossip
# Use the library
```

## Building from Source

To build the package from source, you need to have CMake and a C++17 compatible compiler installed.

```bash
git clone https://github.com/caomengxuan666/libgossip.git
cd libgossip
git submodule update --init
mkdir build
cd build
cmake .. -DBUILD_PYTHON_BINDINGS=ON
cmake --build .
```

## Running Examples

### For Users (After Installation)

After installing the package with `pip install libgossip`, you can run examples from anywhere:

```bash
python example.py
```

### For Developers (Using Built Source)

The examples can be run directly from the `bindings/python` directory **after building** the project:

```bash
# Build the project first (as shown above)
# Then run examples from the bindings/python directory:

# Method 1: Set PYTHONPATH and run directly
set PYTHONPATH=.
python example.py

# Method 2: Run with python -m
python -m libgossip.example

# Method 3: Run examples from within the libgossip package directory
cd libgossip
python sdk_example.py
python decorator_example.py
cd ..
```

Note: Examples must be run from the `bindings/python` directory to work correctly with the built module.

## Usage

### Low-level API

```python
import libgossip

# Create a node view for ourself
self_node = libgossip.NodeView()
self_node.ip = "127.0.0.1"
self_node.port = 7000
self_node.status = libgossip.NodeStatus.ONLINE

# Define callbacks
def send_callback(msg, target):
    print(f"Sending message of type {msg.type} to {target.ip}:{target.port}")

def event_callback(node, old_status):
    print(f"Node {node.ip}:{node.port} changed from {old_status} to {node.status}")

# Initialize gossip core
core = libgossip.GossipCore(self_node, send_callback, event_callback)

# Meet another node
other_node = libgossip.NodeView()
other_node.ip = "127.0.0.1"
other_node.port = 7001
other_node.status = libgossip.NodeStatus.JOINING

core.meet(other_node)
```

### High-level API

```python
import libgossip

# Create and start a node using the high-level API
with libgossip.create_node("127.0.0.1", 7000) as node:
    # Register handlers using decorators
    @node.on_message
    def log_message(msg, target):
        print(f"[SEND] {msg.type} to {target.ip}:{target.port}")
        
    @node.on_event
    def log_event(node_view, old_status):
        print(f"[EVENT] {node_view.ip}:{node_view.port} changed from {old_status} to {node_view.status}")
        
    # Create another node to meet
    other_node = libgossip.create_node("127.0.0.1", 7001)
    other_node.start()
    
    # Meet the other node
    node.meet(other_node)
    
    # Run gossip protocol for a few ticks
    for i in range(5):
        node.tick()
```

## API Reference

The package provides both low-level and high-level APIs:

### Low-level Classes
- `GossipCore` - Main protocol implementation
- `NodeView` - Node representation with metadata
- `GossipMessage` - Message structure for network transport
- `NodeId` - Node unique identifier
- `NodeStatus` - Node status enumeration
- `MessageType` - Message type enumeration
- `GossipStats` - Statistics about the gossip protocol

### High-level Classes
- `GossipNode` - High-level wrapper for a gossip node
- `create_node()` - Convenience function to create a GossipNode
- `create_cluster()` - Convenience function to create a cluster of nodes

### Decorators
- `@message_handler` - Decorator for message handlers
- `@event_handler` - Decorator for event handlers
- `@node.on_message` - Method decorator for message handlers
- `@node.on_event` - Method decorator for event handlers