#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

// Send callback function
void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "Core send callback: Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

// Event callback function
void event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "Core event callback: Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

// Server process - listens on given port
void run_tcp_server(int port) {
    std::cout << "[Server] Starting TCP server on port " << port << std::endl;

    // Create server node
    libgossip::node_view server_node;
    server_node.id = libgossip::NodeId::generate_random();
    server_node.ip = "127.0.0.1";
    server_node.port = port;
    server_node.status = libgossip::node_status::online;

    // Create gossip core for server
    auto server_core = std::make_shared<libgossip::gossip_core>(server_node, send_callback, event_callback);

    // Create TCP transport
    auto tcp_transport = transport_factory::create_transport(transport_type::tcp, "127.0.0.1", port);
    if (!tcp_transport) {
        std::cerr << "[Server] Failed to create TCP transport" << std::endl;
        return;
    }

    tcp_transport->set_gossip_core(server_core);
    tcp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = tcp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Server] Failed to start TCP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Server] TCP server started successfully" << std::endl;
    std::cout << "[Server] Waiting for messages..." << std::endl;
    
    // Keep server running for a while
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    tcp_transport->stop();
    std::cout << "[Server] TCP server stopped" << std::endl;
}

void run_udp_server(int port) {
    std::cout << "[Server] Starting UDP server on port " << port << std::endl;

    // Create server node
    libgossip::node_view server_node;
    server_node.id = libgossip::NodeId::generate_random();
    server_node.ip = "127.0.0.1";
    server_node.port = port;
    server_node.status = libgossip::node_status::online;

    // Create gossip core for server
    auto server_core = std::make_shared<libgossip::gossip_core>(server_node, send_callback, event_callback);

    // Create UDP transport
    auto udp_transport = transport_factory::create_transport(transport_type::udp, "127.0.0.1", port);
    if (!udp_transport) {
        std::cerr << "[Server] Failed to create UDP transport" << std::endl;
        return;
    }

    udp_transport->set_gossip_core(server_core);
    udp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = udp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Server] Failed to start UDP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Server] UDP server started successfully" << std::endl;
    std::cout << "[Server] Waiting for messages..." << std::endl;
    
    // Keep server running for a while
    std::this_thread::sleep_for(std::chrono::seconds(15));
    
    udp_transport->stop();
    std::cout << "[Server] UDP server stopped" << std::endl;
}

// Client process - sends messages to server
void run_tcp_client(int server_port) {
    std::cout << "[Client] Starting TCP client" << std::endl;

    // Create client node
    libgossip::node_view client_node;
    client_node.id = libgossip::NodeId::generate_random();
    client_node.ip = "127.0.0.1";
    client_node.port = 9001;
    client_node.status = libgossip::node_status::online;

    // Create gossip core for client
    auto core = std::make_shared<libgossip::gossip_core>(client_node, send_callback, event_callback);

    // Create TCP transport
    auto tcp_transport = transport_factory::create_transport(transport_type::tcp, "127.0.0.1", 9001);
    if (!tcp_transport) {
        std::cerr << "[Client] Failed to create TCP transport" << std::endl;
        return;
    }

    tcp_transport->set_gossip_core(core);
    tcp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = tcp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to start TCP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Client] TCP transport started successfully" << std::endl;

    // Create target node (server)
    libgossip::node_view target_node;
    target_node.id = libgossip::NodeId::generate_random();
    target_node.ip = "127.0.0.1";
    target_node.port = server_port;
    target_node.status = libgossip::node_status::online;

    // Create a test message
    libgossip::gossip_message test_msg;
    test_msg.sender = client_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(client_node);
    test_msg.entries.push_back(target_node);

    // Send test message synchronously
    std::cout << "\n[Client] Sending TCP message synchronously..." << std::endl;
    ec = tcp_transport->send_message(test_msg, target_node);
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to send TCP message synchronously, error code: "
                  << libgossip::enum_to_string(ec) << std::endl;
    } else {
        std::cout << "[Client] TCP message sent synchronously successfully" << std::endl;
    }

    // Send test message asynchronously
    std::cout << "\n[Client] Sending TCP message asynchronously..." << std::endl;
    std::promise<error_code> promise;
    std::future<error_code> future = promise.get_future();

    tcp_transport->send_message_async(test_msg, target_node, [&promise](error_code ec) {
        std::cout << "[Client] TCP async send callback executed" << std::endl;
        if (ec != error_code::success) {
            std::cerr << "[Client] Failed to send TCP message asynchronously, error code: "
                      << libgossip::enum_to_string(ec) << std::endl;
        } else {
            std::cout << "[Client] TCP message sent asynchronously successfully" << std::endl;
        }
        promise.set_value(ec);
    });

    // Wait for the async operation to complete
    auto result = future.wait_for(std::chrono::seconds(2));
    if (result == std::future_status::ready) {
        error_code async_ec = future.get();
        std::cout << "[Client] TCP async operation completed with error code: "
                  << libgossip::enum_to_string(async_ec) << std::endl;
    } else {
        std::cerr << "[Client] TCP async operation timed out" << std::endl;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop transport
    tcp_transport->stop();
    std::cout << "[Client] TCP client stopped" << std::endl;
}

void run_udp_client(int server_port) {
    std::cout << "[Client] Starting UDP client" << std::endl;

    // Create client node
    libgossip::node_view client_node;
    client_node.id = libgossip::NodeId::generate_random();
    client_node.ip = "127.0.0.1";
    client_node.port = 8002;
    client_node.status = libgossip::node_status::online;

    // Create gossip core for client
    auto core = std::make_shared<libgossip::gossip_core>(client_node, send_callback, event_callback);

    // Create UDP transport
    auto udp_transport = transport_factory::create_transport(transport_type::udp, "127.0.0.1", 8002);
    if (!udp_transport) {
        std::cerr << "[Client] Failed to create UDP transport" << std::endl;
        return;
    }

    udp_transport->set_gossip_core(core);
    udp_transport->set_serializer(std::make_unique<json_serializer>());

    // Start transport
    error_code ec = udp_transport->start();
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to start UDP transport: " << libgossip::enum_to_string(ec) << std::endl;
        return;
    }

    std::cout << "[Client] UDP transport started successfully" << std::endl;

    // Create target node (server)
    libgossip::node_view target_node;
    target_node.id = libgossip::NodeId::generate_random();
    target_node.ip = "127.0.0.1";
    target_node.port = server_port;
    target_node.status = libgossip::node_status::online;

    // Create a test message
    libgossip::gossip_message test_msg;
    test_msg.sender = client_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(client_node);
    test_msg.entries.push_back(target_node);

    // Send test message synchronously
    std::cout << "\n[Client] Sending UDP message synchronously..." << std::endl;
    ec = udp_transport->send_message(test_msg, target_node);
    if (ec != error_code::success) {
        std::cerr << "[Client] Failed to send UDP message synchronously, error code: "
                  << libgossip::enum_to_string(ec) << std::endl;
    } else {
        std::cout << "[Client] UDP message sent synchronously successfully" << std::endl;
    }

    // Send test message asynchronously
    std::cout << "\n[Client] Sending UDP message asynchronously..." << std::endl;
    std::promise<error_code> promise;
    std::future<error_code> future = promise.get_future();

    udp_transport->send_message_async(test_msg, target_node, [&promise](error_code ec) {
        std::cout << "[Client] Async send callback executed" << std::endl;
        if (ec != error_code::success) {
            std::cerr << "[Client] Failed to send UDP message asynchronously, error code: "
                      << libgossip::enum_to_string(ec) << std::endl;
        } else {
            std::cout << "[Client] UDP message sent asynchronously successfully" << std::endl;
        }
        promise.set_value(ec);
    });

    // Wait for the async operation to complete
    auto result = future.wait_for(std::chrono::seconds(2));
    if (result == std::future_status::ready) {
        error_code async_ec = future.get();
        std::cout << "[Client] Async operation completed with error code: "
                  << libgossip::enum_to_string(async_ec) << std::endl;
    } else {
        std::cerr << "[Client] Async operation timed out" << std::endl;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Stop transport
    udp_transport->stop();
    std::cout << "[Client] UDP client stopped" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  libgossip Network Layer Example" << std::endl;
    std::cout << "  Testing TCP and UDP with Server/Client" << std::endl;
    std::cout << "========================================" << std::endl;

    // Start TCP server in a separate thread
    std::thread tcp_server_thread(run_tcp_server, 9000);
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "\n--- Starting TCP Test ---" << std::endl;
    run_tcp_client(9000);
    
    // Wait for TCP server to finish
    tcp_server_thread.join();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  TCP Test Complete" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Start UDP server in a separate thread
    std::thread udp_server_thread(run_udp_server, 8001);
    
    // Wait for server to start
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "\n--- Starting UDP Test ---" << std::endl;
    run_udp_client(8001);
    
    // Wait for UDP server to finish
    udp_server_thread.join();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Network Test Complete" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << "\nNetwork example completed!" << std::endl;
    return 0;
}
