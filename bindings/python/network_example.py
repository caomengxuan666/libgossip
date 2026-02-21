#!/usr/bin/env python3
"""
Network module example for libgossip Python bindings.

This example demonstrates real TCP and UDP communication between
server and client processes using threads.
"""

import sys
sys.path.insert(0, '.')
import time
import threading
import libgossip


def send_callback(msg, target):
    """Send callback for client."""
    print(f"[Client] Send callback: Sending message type {msg.type} to {target.ip}:{target.port}")


def event_callback(node, old_status):
    """Event callback for client."""
    print(f"[Client] Event callback: Node {node.ip}:{node.port} status changed from {old_status} to {node.status}")


def server_send_callback(msg, target):
    """Send callback for server."""
    print(f"[Server] Send callback: Sending message type {msg.type} to {target.ip}:{target.port}")


def server_event_callback(node, old_status):
    """Event callback for server."""
    print(f"[Server] Event callback: Node {node.ip}:{node.port} status changed from {old_status} to {node.status}")


def run_tcp_server(port, ready_event):
    """Run TCP server in a thread."""
    print(f"[Server] Starting TCP server on port {port}")

    # Create server node
    server_node = libgossip.NodeView()
    server_node.id = libgossip.NodeId.generate_random()
    server_node.ip = "127.0.0.1"
    server_node.port = port
    server_node.status = libgossip.NodeStatus.ONLINE

    # Create gossip core for server
    server_core = libgossip.GossipCore(
        server_node,
        server_send_callback,
        server_event_callback
    )

    # Create TCP transport
    tcp_transport = libgossip.TcpTransport("127.0.0.1", port)
    tcp_transport.set_gossip_core(server_core)
    tcp_transport.set_serializer(libgossip.JsonSerializer())

    # Start transport
    error_code = tcp_transport.start()
    if error_code != libgossip.ErrorCode.SUCCESS:
        print(f"[Server] Failed to start TCP transport: {libgossip.error_code_to_string(error_code)}")
        return

    print("[Server] TCP server started successfully")

    # Notify client that server is ready
    ready_event.set()

    # Keep server running
    print("[Server] Waiting for messages...")
    for _ in range(10):
        time.sleep(1)

    tcp_transport.stop()
    print("[Server] TCP server stopped")


def run_udp_server(port):
    """Run UDP server in a thread."""
    print(f"[Server] Starting UDP server on port {port}")

    # Create server node
    server_node = libgossip.NodeView()
    server_node.id = libgossip.NodeId.generate_random()
    server_node.ip = "127.0.0.1"
    server_node.port = port
    server_node.status = libgossip.NodeStatus.ONLINE

    # Create gossip core for server
    server_core = libgossip.GossipCore(
        server_node,
        server_send_callback,
        server_event_callback
    )

    # Create UDP transport
    udp_transport = libgossip.UdpTransport("127.0.0.1", port)
    udp_transport.set_gossip_core(server_core)
    udp_transport.set_serializer(libgossip.JsonSerializer())

    # Start transport
    error_code = udp_transport.start()
    if error_code != libgossip.ErrorCode.SUCCESS:
        print(f"[Server] Failed to start UDP transport: {libgossip.error_code_to_string(error_code)}")
        return

    print("[Server] UDP server started successfully")

    # Keep server running
    print("[Server] Waiting for messages...")
    for _ in range(20):
        time.sleep(1)

    print("[Server] UDP server stopping...")
    udp_transport.stop()
    print("[Server] UDP server stopped")


def run_tcp_client(server_port, ready_event):
    """Run TCP client."""
    print("[Client] Waiting for server to be ready...")

    # Wait for server to be ready
    ready_event.wait()
    print("[Client] Server is ready, starting TCP client")

    # Create client node
    client_node = libgossip.NodeView()
    client_node.id = libgossip.NodeId.generate_random()
    client_node.ip = "127.0.0.1"
    client_node.port = 9001
    client_node.status = libgossip.NodeStatus.ONLINE

    # Create gossip core for client
    core = libgossip.GossipCore(client_node, send_callback, event_callback)

    # Create TCP transport
    tcp_transport = libgossip.TcpTransport("127.0.0.1", 9001)
    tcp_transport.set_gossip_core(core)
    tcp_transport.set_serializer(libgossip.JsonSerializer())

    # Start transport
    error_code = tcp_transport.start()
    if error_code != libgossip.ErrorCode.SUCCESS:
        print(f"[Client] Failed to start TCP transport: {libgossip.error_code_to_string(error_code)}")
        return

    print("[Client] TCP transport started successfully")

    # Create target node (server)
    target_node = libgossip.NodeView()
    target_node.id = libgossip.NodeId.generate_random()
    target_node.ip = "127.0.0.1"
    target_node.port = server_port
    target_node.status = libgossip.NodeStatus.ONLINE

    # Create test message
    test_msg = libgossip.GossipMessage()
    test_msg.sender = client_node.id
    test_msg.type = libgossip.MessageType.PING
    test_msg.timestamp = 12345
    test_msg.entries.append(client_node)
    test_msg.entries.append(target_node)

    # Send test message synchronously
    print("\n[Client] Sending TCP message synchronously...")
    error_code = tcp_transport.send_message(test_msg, target_node)
    if error_code == libgossip.ErrorCode.SUCCESS:
        print("[Client] TCP message sent synchronously successfully")
    else:
        print(f"[Client] Failed to send TCP message synchronously: {libgossip.error_code_to_string(error_code)}")

    # Send test message asynchronously
    print("\n[Client] Sending TCP message asynchronously...")
    result = []

    def async_callback(ec):
        print("[Client] TCP async send callback executed")
        if ec == libgossip.ErrorCode.SUCCESS:
            print("[Client] TCP message sent asynchronously successfully")
        else:
            print(f"[Client] Failed to send TCP message asynchronously: {libgossip.error_code_to_string(ec)}")
        result.append(ec)

    tcp_transport.send_message_async(test_msg, target_node, async_callback)

    # Wait for async operation to complete
    for _ in range(20):
        if result:
            break
        time.sleep(0.1)

    if result:
        print(f"[Client] TCP async operation completed with error code: {libgossip.error_code_to_string(result[0])}")
    else:
        print("[Client] TCP async operation timed out")

    time.sleep(1)

    tcp_transport.stop()
    print("[Client] TCP client stopped")


def run_udp_client(server_port):
    """Run UDP client."""
    print("\n[Client] Starting UDP client")

    # Create client node
    client_node = libgossip.NodeView()
    client_node.id = libgossip.NodeId.generate_random()
    client_node.ip = "127.0.0.1"
    client_node.port = 8002
    client_node.status = libgossip.NodeStatus.ONLINE

    # Create gossip core for client
    core = libgossip.GossipCore(client_node, send_callback, event_callback)

    # Create UDP transport
    udp_transport = libgossip.UdpTransport("127.0.0.1", 8002)
    udp_transport.set_gossip_core(core)
    udp_transport.set_serializer(libgossip.JsonSerializer())

    # Start transport
    error_code = udp_transport.start()
    if error_code != libgossip.ErrorCode.SUCCESS:
        print(f"[Client] Failed to start UDP transport: {libgossip.error_code_to_string(error_code)}")
        return

    print("[Client] UDP transport started successfully")

    # Create target node (server)
    target_node = libgossip.NodeView()
    target_node.id = libgossip.NodeId.generate_random()
    target_node.ip = "127.0.0.1"
    target_node.port = server_port
    target_node.status = libgossip.NodeStatus.ONLINE

    # Create test message
    test_msg = libgossip.GossipMessage()
    test_msg.sender = client_node.id
    test_msg.type = libgossip.MessageType.PING
    test_msg.timestamp = 12345
    test_msg.entries.append(client_node)
    test_msg.entries.append(target_node)

    # Send test message synchronously
    print("\n[Client] Sending UDP message synchronously...")
    error_code = udp_transport.send_message(test_msg, target_node)
    if error_code == libgossip.ErrorCode.SUCCESS:
        print("[Client] UDP message sent synchronously successfully")
    else:
        print(f"[Client] Failed to send UDP message synchronously: {libgossip.error_code_to_string(error_code)}")

    # Send test message asynchronously
    print("\n[Client] Sending UDP message asynchronously...")
    result = []

    def async_callback(ec):
        print("[Client] Async send callback executed")
        if ec == libgossip.ErrorCode.SUCCESS:
            print("[Client] UDP message sent asynchronously successfully")
        else:
            print(f"[Client] Failed to send UDP message asynchronously: {libgossip.error_code_to_string(ec)}")
        result.append(ec)

    udp_transport.send_message_async(test_msg, target_node, async_callback)

    # Wait for async operation to complete
    for _ in range(20):
        if result:
            break
        time.sleep(0.1)

    if result:
        print(f"[Client] Async operation completed with error code: {libgossip.error_code_to_string(result[0])}")
    else:
        print("[Client] Async operation timed out")

    time.sleep(1)

    udp_transport.stop()
    print("[Client] UDP client stopped")


def main():
    """Main function to run all tests."""
    print("=" * 50)
    print("  libgossip Network Layer Example (Python)")
    print("  Testing TCP and UDP with Server/Client")
    print("=" * 50)

    # Create event for TCP server ready notification
    tcp_ready = threading.Event()

    # Start TCP server thread
    tcp_server_thread = threading.Thread(target=run_tcp_server, args=(9000, tcp_ready))
    tcp_server_thread.daemon = True
    tcp_server_thread.start()

    # Start UDP server thread
    udp_server_thread = threading.Thread(target=run_udp_server, args=(8001,))
    udp_server_thread.daemon = True
    udp_server_thread.start()

    # Wait for servers to initialize
    time.sleep(1)

    # Run TCP client
    print("\n--- Starting TCP Test ---")
    run_tcp_client(9000, tcp_ready)

    # Wait a bit before UDP test
    time.sleep(1)

    # Run UDP client
    print("\n--- Starting UDP Test ---")
    run_udp_client(8001)

    # Wait a bit for servers to process messages
    time.sleep(1)

    # Wait for both servers to finish
    tcp_server_thread.join(timeout=2)
    udp_server_thread.join(timeout=2)

    print("\n" + "=" * 50)
    print("  Network Test Complete")
    print("=" * 50)
    print("Network example completed!")

    return 0


if __name__ == "__main__":
    exit(main())