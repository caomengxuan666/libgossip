#!/usr/bin/env python3
"""
Two-node example usage of libgossip Python bindings
"""

import libgossip as gossip
import time
from datetime import datetime

class Node:
    def __init__(self, ip, port):
        self.node_view = gossip.NodeView()
        self.node_view.id = gossip.NodeId.generate_random()
        self.node_view.ip = ip
        self.node_view.port = port
        self.node_view.status = gossip.NodeStatus.ONLINE
        self.core = gossip.GossipCore(self.node_view, self.send_callback, self.event_callback)
        self.other_nodes = {}  # Simulated network: node_id -> node_view
        
    def send_callback(self, msg, target):
        """Callback for sending messages"""
        print(f"[{self.node_view.ip}:{self.node_view.port}] Sending message of type {msg.type} to {target.ip}:{target.port}")
        # In a real implementation, this would actually send the message over the network
        # For this example, we'll just print it
        
    def event_callback(self, node, old_status):
        """Callback for node status changes"""
        print(f"[{self.node_view.ip}:{self.node_view.port}] Node {node.ip}:{node.port} changed from {old_status} to {node.status}")
        
    def meet(self, other_node):
        """Meet another node"""
        print(f"[{self.node_view.ip}:{self.node_view.port}] Meeting {other_node.ip}:{other_node.port}")
        self.core.meet(other_node)
        
    def tick(self):
        """Run one tick of the gossip protocol"""
        self.core.tick()
        
    def get_stats(self):
        """Get node statistics"""
        return self.core.get_stats()
        
    def get_nodes(self):
        """Get known nodes"""
        return self.core.get_nodes()


def main():
    # Create two nodes
    node1 = Node("127.0.0.1", 7000)
    node2 = Node("127.0.0.1", 7001)
    
    print("Created nodes:")
    print(f"  Node 1: {node1.node_view.ip}:{node1.node_view.port}, ID: {node1.node_view.id}")
    print(f"  Node 2: {node2.node_view.ip}:{node2.node_view.port}, ID: {node2.node_view.id}")
    
    # Have node1 meet node2
    node1.meet(node2.node_view)
    
    # Run a few ticks on both nodes
    print("\nRunning gossip protocol...")
    for i in range(5):
        print(f"\n--- Round {i} ---")
        node1.tick()
        node2.tick()
        time.sleep(0.1)
    
    # Print statistics
    print("\n--- Statistics ---")
    stats1 = node1.get_stats()
    stats2 = node2.get_stats()
    
    print(f"Node 1 - Known nodes: {stats1.known_nodes}, Sent: {stats1.sent_messages}, Received: {stats1.received_messages}")
    print(f"Node 2 - Known nodes: {stats2.known_nodes}, Sent: {stats2.sent_messages}, Received: {stats2.received_messages}")
    
    # Check nodes known by each
    nodes1 = node1.get_nodes()
    nodes2 = node2.get_nodes()
    
    print(f"\nNode 1 knows {len(nodes1)} nodes:")
    for node in nodes1:
        print(f"  {node.ip}:{node.port}")
        
    print(f"Node 2 knows {len(nodes2)} nodes:")
    for node in nodes2:
        print(f"  {node.ip}:{node.port}")


if __name__ == "__main__":
    main()