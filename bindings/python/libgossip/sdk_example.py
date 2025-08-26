#!/usr/bin/env python3
"""
Example usage of libgossip Python SDK with decorators and syntax sugar
"""

import time
import sys
import os

# Add the parent directory to the path so we can import libgossip
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import libgossip as gossip


@gossip.message_handler
def log_message(msg, target):
    """Log outgoing messages"""
    print(f"[SEND] {msg.type} to {target.ip}:{target.port}")


@gossip.event_handler
def log_event(node, old_status):
    """Log node events"""
    print(f"[EVENT] {node.ip}:{node.port} changed from {old_status} to {node.status}")


def main():
    # Create and start a node using the high-level API
    with gossip.create_node("127.0.0.1", 7000) as node:
        # Register handlers using decorators
        node.on_message(log_message)
        node.on_event(log_event)
        
        # Create another node to meet
        other_node = gossip.create_node("127.0.0.1", 7001)
        other_node.start()
        
        # Meet the other node
        node.meet(other_node)
        
        print("Starting gossip protocol...")
        # Run gossip protocol for a few ticks
        for i in range(5):
            print(f"Tick {i+1}")
            node.tick()
            time.sleep(0.1)
            
        # Print statistics
        stats = node.get_stats()
        print(f"\nStatistics:")
        print(f"  Known nodes: {stats.known_nodes}")
        print(f"  Sent messages: {stats.sent_messages}")
        print(f"  Received messages: {stats.received_messages}")
        
        # List known nodes
        nodes = node.get_nodes()
        print(f"\nKnown nodes ({len(nodes)}):")
        for n in nodes:
            print(f"  {n.ip}:{n.port}")


if __name__ == "__main__":
    main()