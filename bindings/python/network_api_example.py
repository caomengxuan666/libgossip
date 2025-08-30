#!/usr/bin/env python3
"""
Advanced Network API Example for libgossip Python bindings.

This example demonstrates the high-level network API for libgossip,
including the NetworkCluster class and decorators.
"""

import time
import libgossip


@libgossip.message_handler
def log_message_handler(msg, target):
    """Handle outgoing messages by logging them"""
    print(f"Sending message type {msg.type} to {target.ip}:{target.port}")


@libgossip.event_handler
def log_event_handler(node, old_status):
    """Handle node events by logging them"""
    print(f"Node {node.ip}:{node.port} changed from {old_status} to {node.status}")


def demonstrate_basic_networking():
    """Demonstrate basic networking with high-level API"""
    print("=== Basic Networking Demo ===")
    
    # Create two nodes using the high-level API
    node1 = libgossip.create_node("127.0.0.1", 8001)
    node2 = libgossip.create_node("127.0.0.1", 8002)
    
    # Register handlers using decorators
    node1.on_message(log_message_handler)
    node1.on_event(log_event_handler)
    node2.on_message(log_message_handler)
    node2.on_event(log_event_handler)
    
    # Start nodes
    node1.start(libgossip.TransportType.UDP)
    node2.start(libgossip.TransportType.UDP)
    
    # Have nodes meet each other
    node1.meet(node2)
    node2.meet(node1)
    
    # Run a few ticks
    for i in range(3):
        print(f"Tick {i+1}")
        node1.tick()
        node2.tick()
        time.sleep(0.1)
    
    # Print stats
    print(f"Node 1 stats: {node1.get_stats()}")
    print(f"Node 2 stats: {node2.get_stats()}")
    
    # Stop nodes
    node1.stop()
    node2.stop()
    
    print("Basic networking demo completed\n")


def demonstrate_network_cluster():
    """Demonstrate the NetworkCluster class"""
    print("=== Network Cluster Demo ===")
    
    # Define cluster configuration
    nodes_config = [
        ("127.0.0.1", 9001),
        ("127.0.0.1", 9002),
        ("127.0.0.1", 9003)
    ]
    
    # Create and start cluster using context manager
    with libgossip.NetworkCluster(nodes_config, libgossip.TransportType.UDP) as cluster:
        print(f"Created cluster with {len(cluster)} nodes")
        
        # Run ticks on all nodes
        for i in range(3):
            print(f"Cluster tick {i+1}")
            for node in cluster:
                node.tick()
            time.sleep(0.1)
        
        # Print stats for all nodes
        for i, node in enumerate(cluster):
            stats = node.get_stats()
            print(f"Node {i+1} stats: known_nodes={stats.known_nodes}, "
                  f"sent_messages={stats.sent_messages}, "
                  f"received_messages={stats.received_messages}")
    
    print("Network cluster demo completed\n")


def demonstrate_message_sending():
    """Demonstrate sending messages between nodes"""
    print("=== Message Sending Demo ===")
    
    # Create nodes
    sender = libgossip.create_node("127.0.0.1", 10001)
    receiver = libgossip.create_node("127.0.0.1", 10002)
    
    # Start nodes
    sender.start(libgossip.TransportType.UDP)
    receiver.start(libgossip.TransportType.UDP)
    
    # Meet each other
    sender.meet(receiver)
    receiver.meet(sender)
    
    # Create a test message
    msg = libgossip.GossipMessage()
    msg.sender = sender.id
    msg.type = libgossip.MessageType.PING
    msg.timestamp = int(time.time())
    
    # Add sender to entries
    msg.entries = [sender.node_view]
    
    # Send message
    result = sender.send_message(msg, receiver)
    print(f"Message send result: {result}")
    
    # Run ticks to process message
    for i in range(2):
        sender.tick()
        receiver.tick()
        time.sleep(0.1)
    
    # Stop nodes
    sender.stop()
    receiver.stop()
    
    print("Message sending demo completed\n")


def main():
    """Main function to run all demos"""
    print("libgossip Advanced Network API Example")
    print("=" * 40)
    
    try:
        demonstrate_basic_networking()
        demonstrate_network_cluster()
        demonstrate_message_sending()
        
        print("All demos completed successfully!")
        return 0
    except Exception as e:
        print(f"Error during demo: {e}")
        return 1


if __name__ == "__main__":
    exit(main())