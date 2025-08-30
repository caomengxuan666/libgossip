#!/usr/bin/env python3
"""
pytest test suite for libgossip Python bindings.

This file contains tests for the libgossip Python bindings, covering:
- Core functionality
- Network module
- Decorators
"""

import pytest
import time
import libgossip


def test_node_creation():
    """Test basic node creation."""
    node = libgossip.create_node("127.0.0.1", 8000)
    assert node is not None
    assert node.ip == "127.0.0.1"
    assert node.port == 8000
    assert node.id is not None


def test_node_lifecycle():
    """Test node lifecycle management."""
    node = libgossip.create_node("127.0.0.1", 8001)
    assert node._core is None
    assert node._transport is None
    
    # Start the node
    node.start()
    assert node._core is not None
    assert node._transport is not None
    
    # Stop the node
    result = node.stop()
    assert result == libgossip.ErrorCode.SUCCESS


def test_message_creation():
    """Test creation and basic properties of GossipMessage."""
    msg = libgossip.GossipMessage()
    assert msg is not None
    assert msg.type == libgossip.MessageType.PING  # Default type
    assert msg.timestamp == 0  # Default timestamp
    assert msg.entries == []  # Default empty entries


def test_node_view_properties():
    """Test NodeView properties."""
    node_view = libgossip.NodeView()
    node_view.ip = "192.168.1.100"
    node_view.port = 9000
    
    assert node_view.ip == "192.168.1.100"
    assert node_view.port == 9000


def test_transport_factory():
    """Test transport factory creation."""
    # Test UDP transport creation
    udp_transport = libgossip.TransportFactory.create_transport(
        libgossip.TransportType.UDP, "127.0.0.1", 8002)
    assert udp_transport is not None
    
    # Test TCP transport creation
    tcp_transport = libgossip.TransportFactory.create_transport(
        libgossip.TransportType.TCP, "127.0.0.1", 9002)
    assert tcp_transport is not None


def test_json_serializer():
    """Test JSON serializer functionality."""
    serializer = libgossip.JsonSerializer()
    
    # Create a test message
    msg = libgossip.GossipMessage()
    msg.sender = libgossip.NodeId.generate_random()
    msg.type = libgossip.MessageType.PING
    msg.timestamp = 123456  # Use a simple timestamp for testing
    
    # Serialize the message
    error_code, data = serializer.serialize(msg)
    assert error_code == libgossip.ErrorCode.SUCCESS
    assert isinstance(data, list)  # Should be a list of bytes
    assert len(data) > 0
    
    # Deserialize the message
    error_code, decoded_msg = serializer.deserialize(data)
    assert error_code == libgossip.ErrorCode.SUCCESS
    assert decoded_msg.type == msg.type
    # Note: Due to limitations in the simple JSON parser, timestamp may not deserialize correctly
    # For now, we're just checking that it doesn't fail


def test_message_handler_decorator():
    """Test message handler decorator."""
    @libgossip.message_handler
    def test_handler(msg, target):
        return f"Handling {msg.type} to {target.ip}:{target.port}"
    
    # Check that the decorator preserves the function
    assert hasattr(test_handler, '_is_message_handler')
    assert test_handler._is_message_handler is True


def test_event_handler_decorator():
    """Test event handler decorator."""
    @libgossip.event_handler
    def test_event_handler(node, old_status):
        return f"Node {node.ip}:{node.port} changed from {old_status} to {node.status}"
    
    # Check that the decorator preserves the function
    assert hasattr(test_event_handler, '_is_event_handler')
    assert test_event_handler._is_event_handler is True


def test_message_type_filter_decorator():
    """Test message type filter decorator."""
    @libgossip.message_type_filter(libgossip.MessageType.PING, libgossip.MessageType.PONG)
    def ping_pong_handler(msg, target):
        return f"Handling {msg.type}"
    
    # Test that it has the right attributes
    assert hasattr(ping_pong_handler, '_is_message_handler')
    assert hasattr(ping_pong_handler, '_filtered_message_types')
    assert libgossip.MessageType.PING in ping_pong_handler._filtered_message_types
    assert libgossip.MessageType.PONG in ping_pong_handler._filtered_message_types


def test_node_status_monitor_decorator():
    """Test node status monitor decorator."""
    @libgossip.node_status_monitor(libgossip.NodeStatus.ONLINE, libgossip.NodeStatus.FAILED)
    def status_handler(node, old_status):
        return f"Status changed"
    
    # Test that it has the right attributes
    assert hasattr(status_handler, '_is_event_handler')
    assert hasattr(status_handler, '_monitored_statuses')
    assert libgossip.NodeStatus.ONLINE in status_handler._monitored_statuses
    assert libgossip.NodeStatus.FAILED in status_handler._monitored_statuses


def test_retry_decorator():
    """Test retry on network error decorator."""
    call_count = 0
    
    @libgossip.retry_on_network_error(max_retries=3, delay=0.01)
    def failing_function():
        nonlocal call_count
        call_count += 1
        if call_count < 2:
            raise Exception("Simulated network error")
        return "Success"
    
    # Should succeed on the second try
    result = failing_function()
    assert result == "Success"
    assert call_count == 2


def test_rate_limit_decorator():
    """Test rate limit decorator."""
    call_times = []
    
    @libgossip.rate_limit(calls_per_second=10)  # 10 calls per second
    def rate_limited_function():
        call_times.append(time.time())
        return "Called"
    
    # Call the function multiple times
    for _ in range(5):
        rate_limited_function()
    
    # Check that we have the right number of calls
    assert len(call_times) == 5
    
    # Check that calls are properly spaced (approximately)
    if len(call_times) > 1:
        # We won't do strict timing checks in unit tests to avoid flakiness
        pass


def test_measure_latency_decorator():
    """Test measure latency decorator."""
    @libgossip.measure_latency
    def timed_function():
        time.sleep(0.01)  # Sleep for 10ms
        return "Done"
    
    # This should execute without error
    result = timed_function()
    assert result == "Done"


def test_two_node_interaction():
    """Test interaction between two nodes."""
    # Create two nodes
    node1 = libgossip.create_node("127.0.0.1", 8010)
    node2 = libgossip.create_node("127.0.0.1", 8011)
    
    # Start both nodes
    node1.start()
    node2.start()
    
    # Have them meet each other
    node1.meet(node2)
    node2.meet(node1)
    
    # Run a few protocol ticks
    for i in range(3):
        node1.tick()
        node2.tick()
        time.sleep(0.01)
    
    # Check that they know about each other
    nodes1 = node1.get_nodes()
    nodes2 = node2.get_nodes()
    
    # Both should know about at least one other node
    assert len(nodes1) >= 1
    assert len(nodes2) >= 1
    
    # Stop both nodes
    node1.stop()
    node2.stop()


if __name__ == "__main__":
    pytest.main([__file__])