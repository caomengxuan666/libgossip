#!/usr/bin/env python3
"""
Network module example for libgossip Python bindings.

This example demonstrates how to use the network module of libgossip
including transports, serializers, and the transport factory.
"""

import libgossip


def test_transport_factory():
    """Test the transport factory."""
    print("Testing transport factory...")
    
    # Create a UDP transport
    udp_transport = libgossip.TransportFactory.create_transport(
        libgossip.TransportType.UDP, "127.0.0.1", 8001)
    
    if udp_transport is not None:
        print("Successfully created UDP transport")
    else:
        print("Failed to create UDP transport")
        return False
    
    # Create a TCP transport
    tcp_transport = libgossip.TransportFactory.create_transport(
        libgossip.TransportType.TCP, "127.0.0.1", 9001)
    
    if tcp_transport is not None:
        print("Successfully created TCP transport")
    else:
        print("Failed to create TCP transport")
        return False
    
    return True


def test_json_serializer():
    """Test the JSON serializer."""
    print("\nTesting JSON serializer...")
    
    # Create a serializer
    serializer = libgossip.JsonSerializer()
    
    # Create a test message
    msg = libgossip.GossipMessage()
    msg.sender = libgossip.NodeId.generate_random()
    msg.type = libgossip.MessageType.PING
    msg.timestamp = 1234567890
    
    # Serialize the message
    error_code, data = serializer.serialize(msg)
    
    if error_code == libgossip.ErrorCode.SUCCESS:
        print(f"Successfully serialized message to {len(data)} bytes")
    else:
        print(f"Failed to serialize message: {error_code}")
        return False
    
    # Deserialize the message
    error_code, decoded_msg = serializer.deserialize(data)
    
    if error_code == libgossip.ErrorCode.SUCCESS:
        print(f"Successfully deserialized message")
        print(f"  Original type: {msg.type}")
        print(f"  Decoded type: {decoded_msg.type}")
        print(f"  Timestamp: {decoded_msg.timestamp}")
    else:
        print(f"Failed to deserialize message: {error_code}")
        return False
    
    return True


def test_udp_transport():
    """Test UDP transport creation and configuration."""
    print("\nTesting UDP transport...")
    
    # Create UDP transport
    transport = libgossip.UdpTransport("127.0.0.1", 8002)
    
    # Create a node for testing
    node = libgossip.NodeView()
    node.ip = "127.0.0.1"
    node.port = 8003
    
    # Create a core instance (required for transport)
    core = libgossip.GossipCore(node, 
                               lambda msg, target: None,  # send callback
                               lambda node, old_status: None)  # event callback
    
    # Set core and serializer for transport
    transport.set_gossip_core(core)
    transport.set_serializer(libgossip.JsonSerializer())
    
    print("Successfully configured UDP transport")
    return True


def test_tcp_transport():
    """Test TCP transport creation and configuration."""
    print("\nTesting TCP transport...")
    
    # Create TCP transport
    transport = libgossip.TcpTransport("127.0.0.1", 9002)
    
    # Create a node for testing
    node = libgossip.NodeView()
    node.ip = "127.0.0.1"
    node.port = 9003
    
    # Create a core instance (required for transport)
    core = libgossip.GossipCore(node,
                               lambda msg, target: None,  # send callback
                               lambda node, old_status: None)  # event callback
    
    # Set core and serializer for transport
    transport.set_gossip_core(core)
    transport.set_serializer(libgossip.JsonSerializer())
    
    print("Successfully configured TCP transport")
    return True


def main():
    """Main function to run all tests."""
    print("libgossip Network Module Python Bindings Example")
    print("=" * 50)
    
    # Test transport factory
    if not test_transport_factory():
        print("Transport factory test failed")
        return 1
    
    # Test JSON serializer
    if not test_json_serializer():
        print("JSON serializer test failed")
        return 1
    
    # Test UDP transport
    if not test_udp_transport():
        print("UDP transport test failed")
        return 1
    
    # Test TCP transport
    if not test_tcp_transport():
        print("TCP transport test failed")
        return 1
    
    print("\nAll tests passed!")
    return 0


if __name__ == "__main__":
    exit(main())