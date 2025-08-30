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
        GossipStats,
        # Network module classes
        Transport,
        UdpTransport,
        TcpTransport,
        TransportFactory,
        TransportType,
        MessageSerializer,
        JsonSerializer,
        ErrorCode
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
        GossipStats,
        # Network module classes
        Transport,
        UdpTransport,
        TcpTransport,
        TransportFactory,
        TransportType,
        MessageSerializer,
        JsonSerializer,
        ErrorCode
    )

# Re-export core classes with more Pythonic names
NodeStatus = NodeStatus
MessageType = MessageType
TransportType = TransportType
ErrorCode = ErrorCode

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
        self._transport = None
        self._serializer = None
        
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
            
    def start(self, transport_type=TransportType.UDP):
        """Start the gossip node with specified transport"""
        if self._core is None:
            self._core = GossipCore(self.node_view, self._send_callback, self._event_callback)
            
        # Create transport if not already created
        if self._transport is None:
            self._transport = TransportFactory.create_transport(transport_type, self.ip, self.port)
            self._serializer = JsonSerializer()
            self._transport.set_gossip_core(self._core)
            self._transport.set_serializer(self._serializer)
            
        # Start the transport
        result = self._transport.start()
        return self, result
        
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
        
    def stop(self):
        """Stop the node and its transport"""
        if self._transport:
            return self._transport.stop()
        return ErrorCode.SUCCESS
        
    def send_message(self, msg, target):
        """Send a message to target node"""
        if self._transport:
            if isinstance(target, GossipNode):
                return self._transport.send_message(msg, target.node_view)
            else:
                return self._transport.send_message(msg, target)
        return ErrorCode.NETWORK_ERROR
        
    def __enter__(self):
        """Context manager entry"""
        self.start()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop()


def message_handler(func):
    """
    Decorator for message handlers
    
    Usage:
        @message_handler
        def my_message_handler(msg, target):
            # Handle outgoing messages
            pass
    """
    func._is_message_handler = True
    return func


def event_handler(func):
    """
    Decorator for event handlers
    
    Usage:
        @event_handler
        def my_event_handler(node, old_status):
            # Handle node events
            pass
    """
    func._is_event_handler = True
    return func


def retry_on_network_error(max_retries=3, delay=0.1):
    """
    Decorator to automatically retry network operations on failure
    
    Usage:
        @retry_on_network_error(max_retries=5, delay=0.5)
        def send_message_with_retry(node, msg, target):
            return node.send_message(msg, target)
    """
    def decorator(func):
        import time
        from functools import wraps
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            last_exception = None
            for attempt in range(max_retries):
                try:
                    result = func(*args, **kwargs)
                    # If we get an error code, it might be retryable
                    if hasattr(result, 'value') and result != ErrorCode.SUCCESS:
                        if result in [ErrorCode.NETWORK_ERROR, ErrorCode.SERIALIZATION_ERROR]:
                            last_exception = result
                            if attempt < max_retries - 1:
                                time.sleep(delay * (2 ** attempt))  # Exponential backoff
                                continue
                    return result
                except Exception as e:
                    last_exception = e
                    if attempt < max_retries - 1:
                        time.sleep(delay * (2 ** attempt))  # Exponential backoff
                        continue
            raise last_exception
        return wrapper
    return decorator


def async_network_operation(func):
    """
    Decorator to run network operations asynchronously
    
    Usage:
        @async_network_operation
        def send_message_async(node, msg, target):
            return node.send_message(msg, target)
    """
    from functools import wraps
    import asyncio
    
    @wraps(func)
    def wrapper(*args, **kwargs):
        loop = asyncio.get_event_loop()
        return loop.run_in_executor(None, lambda: func(*args, **kwargs))
    return wrapper


def with_timeout(timeout=5.0):
    """
    Decorator to add timeout to network operations
    
    Usage:
        @with_timeout(timeout=10.0)
        def long_network_operation(node):
            # Some long-running network operation
            pass
    """
    def decorator(func):
        from functools import wraps
        import signal
        import platform
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            # Only works on Unix platforms
            if platform.system() != 'Windows':
                def timeout_handler(signum, frame):
                    raise TimeoutError(f"Network operation timed out after {timeout} seconds")
                
                old_handler = signal.signal(signal.SIGALRM, timeout_handler)
                signal.alarm(int(timeout))
                
                try:
                    result = func(*args, **kwargs)
                    return result
                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)
            else:
                # On Windows, just run without timeout
                return func(*args, **kwargs)
        return wrapper
    return decorator


def node_lifecycle(auto_start=True, auto_stop=True):
    """
    Decorator to automatically manage node lifecycle
    
    Usage:
        @node_lifecycle(auto_start=True, auto_stop=True)
        def run_network_operation(ip, port):
            node = create_node(ip, port)
            # Perform operations with node
            return node
    """
    def decorator(func):
        from functools import wraps
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            result = func(*args, **kwargs)
            
            # If the function returns a node, manage its lifecycle
            if isinstance(result, GossipNode):
                node = result
                if auto_start and node._transport is None:
                    node.start()
                if auto_stop:
                    import atexit
                    atexit.register(node.stop)
            return result
        return wrapper
    return decorator


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


def create_cluster(nodes, transport_type=TransportType.UDP):
    """
    Create a cluster of interconnected nodes
    
    Args:
        nodes (list): List of (ip, port) tuples
        transport_type (TransportType): Type of transport to use
        
    Returns:
        list: List of interconnected GossipNode instances
    """
    gossip_nodes = [GossipNode(ip, port) for ip, port in nodes]
    
    # Connect all nodes to each other
    for i, node in enumerate(gossip_nodes):
        node.start(transport_type)
        for j, other_node in enumerate(gossip_nodes):
            if i != j:
                node.meet(other_node)
                
    return gossip_nodes


# Network utility classes
class NetworkCluster:
    """
    High-level wrapper for managing a cluster of networked gossip nodes
    """
    
    def __init__(self, nodes_config, transport_type=TransportType.UDP):
        """
        Initialize a network cluster
        
        Args:
            nodes_config (list): List of (ip, port) tuples
            transport_type (TransportType): Type of transport to use
        """
        self.nodes = []
        self.transport_type = transport_type
        self._nodes_config = nodes_config
        
    def start(self):
        """Start all nodes in the cluster"""
        self.nodes = create_cluster(self._nodes_config, self.transport_type)
        return self
        
    def stop(self):
        """Stop all nodes in the cluster"""
        for node in self.nodes:
            node.stop()
            
    def broadcast_message(self, message):
        """
        Broadcast a message to all nodes in the cluster
        
        Args:
            message (GossipMessage): Message to broadcast
        """
        for node in self.nodes:
            for target in self.nodes:
                if node != target:
                    node.send_message(message, target)
                    
    def __enter__(self):
        """Context manager entry"""
        return self.start()
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop()
        
    def __iter__(self):
        """Make the cluster iterable over its nodes"""
        return iter(self.nodes)
        
    def __len__(self):
        """Return the number of nodes in the cluster"""
        return len(self.nodes)


# Re-export important classes and enums
__all__ = [
    'GossipCore',
    'GossipNode',
    'NetworkCluster',
    'NodeView',
    'GossipMessage',
    'NodeId',
    'NodeStatus',
    'MessageType',
    'GossipStats',
    'Transport',
    'UdpTransport',
    'TcpTransport',
    'TransportFactory',
    'TransportType',
    'MessageSerializer',
    'JsonSerializer',
    'ErrorCode',
    'message_handler',
    'event_handler',
    'message_type_filter',
    'node_status_monitor',
    'retry_on_network_error',
    'async_network_operation',
    'with_timeout',
    'node_lifecycle',
    'rate_limit',
    'circuit_breaker',
    'broadcast_to_cluster',
    'measure_latency',
    'create_node',
    'create_cluster'
]


def message_type_filter(*message_types):
    """
    Decorator to filter message handlers by message type
    
    Usage:
        @message_type_filter(MessageType.PING, MessageType.PONG)
        def handle_ping_pong(msg, target):
            # Handle only PING and PONG messages
            pass
    """
    def decorator(func):
        def wrapper(msg, target):
            if msg.type in message_types:
                return func(msg, target)
            # Ignore messages of other types
        wrapper._is_message_handler = True
        wrapper._filtered_message_types = message_types
        return wrapper
    return decorator


def node_status_monitor(*statuses):
    """
    Decorator to filter event handlers by node status changes
    
    Usage:
        @node_status_monitor(NodeStatus.ONLINE, NodeStatus.FAILED)
        def handle_status_change(node, old_status):
            # Handle only ONLINE and FAILED status changes
            pass
    """
    def decorator(func):
        def wrapper(node, old_status):
            if node.status in statuses:
                return func(node, old_status)
            # Ignore other status changes
        wrapper._is_event_handler = True
        wrapper._monitored_statuses = statuses
        return wrapper
    return decorator


def rate_limit(calls_per_second=1):
    """
    Decorator to rate limit network operations
    
    Usage:
        @rate_limit(calls_per_second=10)
        def frequent_operation(node):
            # This will be limited to 10 calls per second
            pass
    """
    def decorator(func):
        from functools import wraps
        import time
        
        # Store last call time for each function
        last_call_time = 0
        min_interval = 1.0 / calls_per_second
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            nonlocal last_call_time
            current_time = time.time()
            time_since_last_call = current_time - last_call_time
            
            if time_since_last_call < min_interval:
                time.sleep(min_interval - time_since_last_call)
            
            last_call_time = time.time()
            return func(*args, **kwargs)
        return wrapper
    return decorator


def circuit_breaker(max_failures=3, timeout=60):
    """
    Decorator implementing circuit breaker pattern for network operations
    
    Usage:
        @circuit_breaker(max_failures=5, timeout=30)
        def unreliable_network_call(node, msg, target):
            return node.send_message(msg, target)
    """
    def decorator(func):
        from functools import wraps
        import time
        
        # Circuit breaker state
        failure_count = 0
        last_failure_time = 0
        state = "CLOSED"  # CLOSED, OPEN, HALF_OPEN
        
        @wraps(func)
        def wrapper(*args, **kwargs):
            nonlocal failure_count, last_failure_time, state
            
            current_time = time.time()
            
            # Check if circuit should be reset
            if state == "OPEN" and current_time - last_failure_time >= timeout:
                state = "HALF_OPEN"
            
            # If circuit is open, fail fast
            if state == "OPEN":
                raise Exception("Circuit breaker is OPEN - operation not allowed")
            
            try:
                result = func(*args, **kwargs)
                
                # On success, reset failure count
                if state in ["HALF_OPEN", "CLOSED"]:
                    failure_count = 0
                    state = "CLOSED"
                
                return result
            except Exception as e:
                # On failure, increment failure count
                failure_count += 1
                last_failure_time = current_time
                
                # If we've hit max failures, open circuit
                if failure_count >= max_failures:
                    state = "OPEN"
                
                raise e
        return wrapper
    return decorator


def broadcast_to_cluster(exclude_self=True):
    """
    Decorator to automatically broadcast messages to all nodes in cluster
    
    Usage:
        @broadcast_to_cluster(exclude_self=True)
        def send_to_all_nodes(cluster, msg):
            # msg will be sent to all nodes in cluster
            pass
    """
    def decorator(func):
        from functools import wraps
        
        @wraps(func)
        def wrapper(cluster, msg, *args, **kwargs):
            results = {}
            
            # Get all nodes in cluster
            nodes = cluster.nodes if hasattr(cluster, 'nodes') else list(cluster)
            
            # Send message to each node
            for i, node in enumerate(nodes):
                # Skip self if requested
                if exclude_self and i == 0:  # Assuming first node is "self"
                    continue
                
                # Send to all other nodes
                for j, target in enumerate(nodes):
                    if i != j:  # Don't send to self
                        if hasattr(node, '_transport') and node._transport:
                            result = node._transport.send_message(msg, target.node_view)
                            results[f"{node.ip}:{node.port}->{target.ip}:{target.port}"] = result
            
            # Call original function if needed
            original_result = func(cluster, msg, *args, **kwargs)
            return {
                'broadcast_results': results,
                'function_result': original_result
            }
        return wrapper
    return decorator


def measure_latency(func):
    """
    Decorator to measure network operation latency
    
    Usage:
        @measure_latency
        def network_operation(node):
            # Some network operation
            pass
    """
    from functools import wraps
    import time
    
    @wraps(func)
    def wrapper(*args, **kwargs):
        start_time = time.time()
        try:
            result = func(*args, **kwargs)
            end_time = time.time()
            latency = (end_time - start_time) * 1000  # Convert to milliseconds
            print(f"[LATENCY] {func.__name__} took {latency:.2f} ms")
            return result
        except Exception as e:
            end_time = time.time()
            latency = (end_time - start_time) * 1000
            print(f"[LATENCY] {func.__name__} failed after {latency:.2f} ms with error: {e}")
            raise
    return wrapper
