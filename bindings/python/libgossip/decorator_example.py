#!/usr/bin/env python3
"""
Example showing decorator usage in libgossip Python SDK
"""

import time
import sys
import os

# Add the parent directory to the path so we can import libgossip
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

import libgossip as gossip


class GossipCluster:
    def __init__(self):
        self.nodes = []
        
    def add_node(self, ip, port):
        """Add a node to the cluster"""
        node = gossip.create_node(ip, port)
        
        # Add decorators for message and event handling
        @node.on_message
        def handle_message(msg, target):
            print(f"[NODE {port}] Sending {msg.type} to {target.ip}:{target.port}")
            
        @node.on_event
        def handle_event(node_view, old_status):
            print(f"[NODE {port}] Status changed: {old_status} -> {node_view.status}")
            
        node.start()
        self.nodes.append(node)
        return node
        
    def connect_all(self):
        """Connect all nodes to each other"""
        for i, node in enumerate(self.nodes):
            for j, other_node in enumerate(self.nodes):
                if i != j:
                    node.meet(other_node)
                    
    def run_ticks(self, count=3):
        """Run ticks on all nodes"""
        for i in range(count):
            print(f"\n--- Round {i+1} ---")
            for node in self.nodes:
                node.tick()
            time.sleep(0.1)


def main():
    print("Creating gossip cluster with decorators...")
    
    # Create cluster
    cluster = GossipCluster()
    
    # Add nodes
    node1 = cluster.add_node("127.0.0.1", 7000)
    node2 = cluster.add_node("127.0.0.1", 7001)
    node3 = cluster.add_node("127.0.0.1", 7002)
    
    # Connect all nodes
    cluster.connect_all()
    
    # Run protocol
    cluster.run_ticks(5)
    
    # Print final stats
    print("\n--- Final Statistics ---")
    for i, node in enumerate(cluster.nodes):
        stats = node.get_stats()
        print(f"Node {7000+i}: {stats.known_nodes} nodes, {stats.sent_messages} sent")


if __name__ == "__main__":
    main()