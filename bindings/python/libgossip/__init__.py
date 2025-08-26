"""
libgossip Python SDK - High-level Python bindings for libgossip C++ library

This package provides both low-level and high-level interfaces to the libgossip library.
"""

# Handle development environment where the module might not be installed
try:
    # Try to import the C++ extension directly
    from libgossip_py import (
        GossipCore,
        NodeView,
        GossipMessage,
        NodeId,
        NodeStatus,
        MessageType,
        GossipStats
    )
except ImportError:
    # In development environment, try to add the current directory to path
    import sys
    import os
    # Add current directory to path to find libgossip_py
    _current_dir = os.path.dirname(__file__)
    if _current_dir not in sys.path:
        sys.path.insert(0, _current_dir)
    # Try importing again
    from libgossip_py import (
        GossipCore,
        NodeView,
        GossipMessage,
        NodeId,
        NodeStatus,
        MessageType,
        GossipStats
    )

# Re-export core classes with more Pythonic names
NodeStatus = NodeStatus
MessageType = MessageType

# High-level SDK classes
class GossipNode:
    """
    High-level wrapper for a gossip node with Pythonic interface
    """
    
    def __init__(self, ip, port, status=NodeStatus.ONLINE):
        self.node_view = NodeView()
        self.node_view.id = NodeId.generate_random()
        self.node_view.ip = ip
        self.node_view.port = port
        self.node_view.status = status
        self._core = None
        self._message_handlers = []
        self._event_handlers = []
        
    @property
    def id(self):
        return self.node_view.id
        
    @property
    def ip(self):
        return self.node_view.ip
        
    @property
    def port(self):
        return self.node_view.port
        
    def on_message(self, handler):
        """Decorator for message handlers"""
        self._message_handlers.append(handler)
        return handler
        
    def on_event(self, handler):
        """Decorator for event handlers"""
        self._event_handlers.append(handler)
        return handler
        
    def _send_callback(self, msg, target):
        """Internal send callback"""
        for handler in self._message_handlers:
            handler(msg, target)
            
    def _event_callback(self, node, old_status):
        """Internal event callback"""
        for handler in self._event_handlers:
            handler(node, old_status)
            
    def start(self):
        """Start the gossip node"""
        if self._core is None:
            self._core = GossipCore(self.node_view, self._send_callback, self._event_callback)
        return self
        
    def meet(self, other_node):
        """Meet another node"""
        if isinstance(other_node, GossipNode):
            self._core.meet(other_node.node_view)
        else:
            self._core.meet(other_node)
        return self
        
    def tick(self):
        """Run one tick of the gossip protocol"""
        self._core.tick()
        return self
        
    def join(self, other_node):
        """Join another node"""
        if isinstance(other_node, GossipNode):
            self._core.join(other_node.node_view)
        else:
            self._core.join(other_node)
        return self
        
    def leave(self, node_id=None):
        """Leave the cluster"""
        if node_id is None:
            node_id = self.node_view.id
        self._core.leave(node_id)
        return self
        
    def get_nodes(self):
        """Get all known nodes"""
        return self._core.get_nodes()
        
    def get_stats(self):
        """Get node statistics"""
        return self._core.get_stats()
        
    def size(self):
        """Get number of known nodes"""
        return self._core.size()
        
    def __enter__(self):
        """Context manager entry"""
        return self.start()
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        pass


def message_handler(func):
    """
    Decorator for message handlers
    """
    return func


def event_handler(func):
    """
    Decorator for event handlers
    """
    return func


# Convenience functions
def create_node(ip, port, status=NodeStatus.ONLINE):
    """
    Create a new gossip node
    
    Args:
        ip (str): IP address
        port (int): Port number
        status (NodeStatus): Initial node status
        
    Returns:
        GossipNode: New gossip node
    """
    return GossipNode(ip, port, status)


def create_cluster(nodes):
    """
    Create a cluster of interconnected nodes
    
    Args:
        nodes (list): List of (ip, port) tuples
        
    Returns:
        list: List of interconnected GossipNode instances
    """
    gossip_nodes = [GossipNode(ip, port) for ip, port in nodes]
    
    # Connect all nodes to each other
    for i, node in enumerate(gossip_nodes):
        node.start()
        for j, other_node in enumerate(gossip_nodes):
            if i != j:
                node.meet(other_node)
                
    return gossip_nodes


# Re-export important classes and enums
__all__ = [
    'GossipCore',
    'GossipNode',
    'NodeView',
    'GossipMessage',
    'NodeId',
    'NodeStatus',
    'MessageType',
    'GossipStats',
    'message_handler',
    'event_handler',
    'create_node',
    'create_cluster'
]