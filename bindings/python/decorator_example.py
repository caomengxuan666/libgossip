#!/usr/bin/env python3
"""
Network Decorators Example for libgossip Python bindings.

This example demonstrates how to use the new network decorators
to simplify network programming with libgossip.
"""

import time
import libgossip


# Example 1: Using retry decorator for robust network operations
@libgossip.retry_on_network_error(max_retries=3, delay=0.1)
def robust_send_message(node, msg, target):
    """Send a message with automatic retry on network errors."""
    return node.send_message(msg, target)


# Example 2: Using timeout decorator for network operations
@libgossip.with_timeout(timeout=2.0)
def time_bounded_operation(node):
    """Run a network operation with a timeout."""
    # Simulate some network operation
    for i in range(5):
        node.tick()
        time.sleep(0.1)
    return "Operation completed"


# Example 3: Using lifecycle decorator to auto-manage node
@libgossip.node_lifecycle(auto_start=True, auto_stop=True)
def create_auto_managed_node(ip, port):
    """Create a node that automatically starts and stops."""
    return libgossip.create_node(ip, port)


# Example 4: Using message handler decorator
@libgossip.message_handler
def log_outgoing_message(msg, target):
    """Log outgoing messages."""
    print(f"[OUTGOING] {msg.type} to {target.ip}:{target.port}")


# Example 5: Using event handler decorator
@libgossip.event_handler
def log_node_events(node, old_status):
    """Log node status changes."""
    print(f"[EVENT] Node {node.ip}:{node.port} changed from {old_status} to {node.status}")


def demonstrate_decorators():
    """Demonstrate the use of network decorators."""
    print("=== Network Decorators Demo ===")
    
    # Create nodes
    node1 = libgossip.create_node("127.0.0.1", 8001)
    node2 = libgossip.create_node("127.0.0.1", 8002)
    
    # Register handlers
    node1.on_message(log_outgoing_message)
    node1.on_event(log_node_events)
    node2.on_message(log_outgoing_message)
    node2.on_event(log_node_events)
    
    # Start nodes
    node1.start(libgossip.TransportType.UDP)
    node2.start(libgossip.TransportType.UDP)
    
    # Meet nodes
    node1.meet(node2)
    node2.meet(node1)
    
    # Run a few ticks
    for i in range(3):
        print(f"Tick {i+1}")
        node1.tick()
        node2.tick()
        time.sleep(0.1)
    
    # Demonstrate retry decorator
    print("\n--- Retry Decorator Demo ---")
    msg = libgossip.GossipMessage()
    msg.sender = node1.id
    msg.type = libgossip.MessageType.PING
    msg.timestamp = int(time.time())
    
    result = robust_send_message(node1, msg, node2.node_view)
    print(f"Message send result: {result}")
    
    # Demonstrate timeout decorator
    print("\n--- Timeout Decorator Demo ---")
    try:
        result = time_bounded_operation(node1)
        print(f"Time-bounded operation result: {result}")
    except TimeoutError as e:
        print(f"Operation timed out: {e}")
    
    # Demonstrate lifecycle decorator
    print("\n--- Lifecycle Decorator Demo ---")
    auto_node = create_auto_managed_node("127.0.0.1", 8003)
    print(f"Auto-managed node created: {auto_node.ip}:{auto_node.port}")
    # Node will auto-start and auto-stop when the program exits
    
    # Run ticks
    for i in range(2):
        auto_node.tick()
        time.sleep(0.1)
    
    # Stop nodes
    node1.stop()
    node2.stop()
    
    print("\nDecorator demo completed!")


def main():
    """Main function."""
    print("libgossip Network Decorators Example")
    print("=" * 40)
    
    try:
        demonstrate_decorators()
        print("\nAll demos completed successfully!")
        return 0
    except Exception as e:
        print(f"Error during demo: {e}")
        return 1


if __name__ == "__main__":
    exit(main())