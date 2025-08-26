#!/usr/bin/env python3
"""
Example usage of libgossip Python bindings

To run this example:
1. Build the project with Python bindings:
   cd libgossip
   mkdir build
   cd build
   cmake .. -DBUILD_PYTHON_BINDINGS=ON
   cmake --build .

2. Run the example from the bindings/python directory:
   cd bindings/python
   python example.py

For installation and regular usage:
   pip install libgossip
   python -c "import libgossip; print('Success')"
"""

import libgossip as gossip
import time
from datetime import datetime


def send_callback(msg, target):
    """Callback for sending messages"""
    print(f"Sending message of type {msg.type} to {target.ip}:{target.port}")


def event_callback(node, old_status):
    """Callback for node status changes"""
    print(f"Node {node.ip}:{node.port} changed from {old_status} to {node.status}")


def main():
    # Create a node view for ourself
    self_node = gossip.NodeView()
    self_node.id = gossip.NodeId.generate_random()
    self_node.ip = "127.0.0.1"
    self_node.port = 7000
    self_node.status = gossip.NodeStatus.ONLINE
    
    print(f"Self node: {self_node.ip}:{self_node.port}, ID: {self_node.id}")
    
    # Initialize gossip core
    core = gossip.GossipCore(self_node, send_callback, event_callback)
    
    # Create another node to meet
    other_node = gossip.NodeView()
    other_node.id = gossip.NodeId.generate_random()
    other_node.ip = "127.0.0.1"
    other_node.port = 7001
    other_node.status = gossip.NodeStatus.JOINING
    
    print(f"Other node: {other_node.ip}:{other_node.port}, ID: {other_node.id}")
    
    # Meet the other node
    print("Calling meet...")
    core.meet(other_node)
    
    # Check nodes after meet
    nodes = core.get_nodes()
    print(f"Nodes after meet: {len(nodes)}")
    
    # Run a few ticks
    for i in range(5):
        print(f"Tick {i}")
        core.tick()
        time.sleep(0.1)
    
    # Print statistics
    stats = core.get_stats()
    print(f"Known nodes: {stats.known_nodes}")
    print(f"Sent messages: {stats.sent_messages}")
    print(f"Received messages: {stats.received_messages}")
    
    # Check nodes again
    nodes = core.get_nodes()
    print(f"Final node count: {len(nodes)}")
    for node in nodes:
        print(f"  Node: {node.ip}:{node.port}")


if __name__ == "__main__":
    main()