#!/usr/bin/env python3
"""
Debug example for libgossip Python bindings
"""

import libgossip as gossip
import time


def send_callback(msg, target):
    """Callback for sending messages"""
    print(f"DEBUG: Send callback called with message type {msg.type}")


def event_callback(node, old_status):
    """Callback for node status changes"""
    print(f"DEBUG: Event callback called for node {node.ip}:{node.port}")


def main():
    # Create a node view for ourself
    self_node = gossip.NodeView()
    self_node.id = gossip.NodeId.generate_random()
    self_node.ip = "127.0.0.1"
    self_node.port = 7000
    self_node.status = gossip.NodeStatus.ONLINE
    
    print(f"Creating GossipCore with node ID: {self_node.id}")
    core = gossip.GossipCore(self_node, send_callback, event_callback)
    
    # Check initial state
    stats = core.get_stats()
    print(f"Initial stats - Known nodes: {stats.known_nodes}")
    
    # Create another node to meet
    other_node = gossip.NodeView()
    other_node.id = gossip.NodeId.generate_random()
    other_node.ip = "127.0.0.1"
    other_node.port = 7001
    other_node.status = gossip.NodeStatus.JOINING
    
    print(f"Other node ID: {other_node.id}")
    
    # Meet the other node
    print("Calling meet...")
    core.meet(other_node)
    
    # Check state after meet
    stats = core.get_stats()
    print(f"Stats after meet - Known nodes: {stats.known_nodes}")
    
    nodes = core.get_nodes()
    print(f"Nodes after meet: {len(nodes)}")
    for i, node in enumerate(nodes):
        print(f"  Node {i}: {node.ip}:{node.port}")
    
    # Run a few ticks
    for i in range(3):
        print(f"Tick {i}")
        core.tick()
        time.sleep(0.1)
        
        # Check stats after each tick
        stats = core.get_stats()
        print(f"  Stats - Known nodes: {stats.known_nodes}, "
              f"Sent: {stats.sent_messages}, Received: {stats.received_messages}")
    
    # Final check
    stats = core.get_stats()
    print(f"Final stats - Known nodes: {stats.known_nodes}, "
          f"Sent: {stats.sent_messages}, Received: {stats.received_messages}")


if __name__ == "__main__":
    main()