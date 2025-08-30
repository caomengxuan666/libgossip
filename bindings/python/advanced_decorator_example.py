#!/usr/bin/env python3
"""
Advanced Network Decorators Example for libgossip Python bindings.

This example demonstrates the advanced network decorators
to simplify network programming with libgossip.
"""

import time
import libgossip


# Example 1: Using message type filter
@libgossip.message_type_filter(libgossip.MessageType.PING, libgossip.MessageType.PONG)
def handle_ping_pong_messages(msg, target):
    """Handle only PING and PONG messages."""
    print(f"[PING_PONG] {msg.type} from {target.ip}:{target.port}")


# Example 2: Using node status monitor
@libgossip.node_status_monitor(libgossip.NodeStatus.ONLINE, libgossip.NodeStatus.FAILED)
def handle_status_changes(node, old_status):
    """Handle only ONLINE and FAILED status changes."""
    print(f"[STATUS] {node.ip}:{node.port} {old_status} -> {node.status}")


# Example 3: Using rate limiter
@libgossip.rate_limit(calls_per_second=2)
def limited_operation(node):
    """This operation is limited to 2 calls per second."""
    print(f"[RATE_LIMITED] Operation called at {time.time()}")


# Example 4: Using circuit breaker
@libgossip.circuit_breaker(max_failures=3, timeout=10)
def protected_network_call(node, msg, target):
    """Network call with circuit breaker protection."""
    return node.send_message(msg, target)


# Example 5: Using latency measurement
@libgossip.measure_latency
def timed_network_operation(node):
    """Network operation with latency measurement."""
    for i in range(3):
        node.tick()
        time.sleep(0.05)


def demonstrate_advanced_decorators():
    """Demonstrate the use of advanced network decorators."""
    print("=== Advanced Network Decorators Demo ===")
    
    # Create nodes
    node1 = libgossip.create_node("127.0.0.1", 8101)
    node2 = libgossip.create_node("127.0.0.1", 8102)
    
    # Register filtered handlers
    node1.on_message(handle_ping_pong_messages)
    node1.on_event(handle_status_changes)
    node2.on_message(handle_ping_pong_messages)
    node2.on_event(handle_status_changes)
    
    # Start nodes
    node1.start(libgossip.TransportType.UDP)
    node2.start(libgossip.TransportType.UDP)
    
    # Meet nodes
    node1.meet(node2)
    node2.meet(node1)
    
    # Demonstrate rate limiter
    print("\n--- Rate Limiter Demo ---")
    for i in range(5):
        limited_operation(node1)
    
    # Demonstrate latency measurement
    print("\n--- Latency Measurement Demo ---")
    timed_network_operation(node1)
    
    # Demonstrate circuit breaker
    print("\n--- Circuit Breaker Demo ---")
    msg = libgossip.GossipMessage()
    msg.sender = node1.id
    msg.type = libgossip.MessageType.PING
    msg.timestamp = int(time.time())
    
    try:
        result = protected_network_call(node1, msg, node2.node_view)
        print(f"Protected call result: {result}")
    except Exception as e:
        print(f"Protected call failed: {e}")
    
    # Run a few ticks
    print("\n--- Running Protocol Ticks ---")
    for i in range(5):
        print(f"Tick {i+1}")
        node1.tick()
        node2.tick()
        time.sleep(0.1)
    
    # Stop nodes
    node1.stop()
    node2.stop()
    
    print("\nAdvanced decorator demo completed!")


def main():
    """Main function."""
    print("libgossip Advanced Network Decorators Example")
    print("=" * 50)
    
    try:
        demonstrate_advanced_decorators()
        print("\nAll demos completed successfully!")
        return 0
    except Exception as e:
        print(f"Error during demo: {e}")
        return 1


if __name__ == "__main__":
    exit(main())