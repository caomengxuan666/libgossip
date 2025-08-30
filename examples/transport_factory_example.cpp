/**
 * @file transport_factory_example.cpp
 * @brief Example demonstrating transport factory usage
 * 
 * This example demonstrates how to use the transport factory to create
 * different types of transport instances (UDP, TCP, etc.).
 */

#include "core/gossip_core.hpp"
#include "net/json_serializer.hpp"
#include "net/transport_factory.hpp"
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

// ========================================================================
// Callback functions
// ========================================================================

/**
 * @brief Send callback function
 * 
 * This function is called by the gossip core when it needs to send a message.
 * 
 * @param msg The message to send
 * @param target The target node
 */
void send_callback(const libgossip::gossip_message &msg, const libgossip::node_view &target) {
    std::cout << "[Core] Sending message of type "
              << static_cast<int>(msg.type) << " to "
              << target.ip << ":" << target.port << std::endl;
}

/**
 * @brief Event callback function
 * 
 * This function is called by the gossip core when a node's status changes.
 * 
 * @param node The node that changed status
 * @param old_status The previous status of the node
 */
void event_callback(const libgossip::node_view &node, libgossip::node_status old_status) {
    std::cout << "[Core] Node "
              << node.ip << ":" << node.port
              << " changed from status " << static_cast<int>(old_status)
              << " to " << static_cast<int>(node.status) << std::endl;
}

// ========================================================================
// Helper functions
// ========================================================================

/**
 * @brief Test a transport instance
 * 
 * This function tests a transport instance by starting it, sending messages,
 * and then stopping it.
 * 
 * @param transport The transport to test
 * @param transport_name The name of the transport for logging
 */
void test_transport(std::unique_ptr<gossip::net::transport> &transport,
                    const std::string &transport_name) {
    std::cout << "\n--- Testing " << transport_name << " ---" << std::endl;

    // Create gossip core
    libgossip::node_view self_node;
    self_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}};// Simple ID
    self_node.ip = "127.0.0.1";
    self_node.port = 8000;
    self_node.status = libgossip::node_status::online;

    auto core = std::make_shared<libgossip::gossip_core>(self_node, send_callback, event_callback);
    transport->set_gossip_core(core);

    // Set serializer
    transport->set_serializer(std::make_unique<gossip::net::json_serializer>());

    // Start transport
    gossip::net::error_code ec = transport->start();
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to start " << transport_name << " transport: "
                  << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Started " << transport_name << " transport successfully" << std::endl;

    // Create test message
    libgossip::node_view target_node;
    target_node.id = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2}};// Another simple ID
    target_node.ip = "127.0.0.1";
    target_node.port = 8001;
    target_node.status = libgossip::node_status::online;

    libgossip::gossip_message test_msg;
    test_msg.sender = self_node.id;
    test_msg.type = libgossip::message_type::ping;
    test_msg.timestamp = 12345;
    test_msg.entries.push_back(self_node);

    // Send message synchronously
    ec = transport->send_message(test_msg, target_node);
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to send message via " << transport_name
                  << ", error code: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "Message sent via " << transport_name << " successfully" << std::endl;
    }

    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Stop transport
    ec = transport->stop();
    if (ec != gossip::net::error_code::success) {
        std::cerr << "Failed to stop " << transport_name << " transport: "
                  << static_cast<int>(ec) << std::endl;
        return;
    }

    std::cout << "Stopped " << transport_name << " transport successfully" << std::endl;
}

// ========================================================================
// Main function
// ========================================================================

/**
 * @brief Main function demonstrating transport factory usage
 * 
 * This function demonstrates:
 * 1. Creating UDP transport using the factory
 * 2. Creating TCP transport using the factory
 * 3. Testing both transport types
 */
int main() {
    std::cout << "libgossip Transport Factory Example" << std::endl;
    std::cout << "===================================" << std::endl;

    try {
        // --------------------------------------------------------------------
        // 1. Create UDP transport using factory
        // --------------------------------------------------------------------
        std::cout << "\n--- Creating UDP Transport ---" << std::endl;
        auto udp_transport = gossip::net::transport_factory::create_transport(
                gossip::net::transport_type::udp, "127.0.0.1", 8000);

        if (udp_transport) {
            std::cout << "Successfully created UDP transport" << std::endl;
            test_transport(udp_transport, "UDP");
        } else {
            std::cerr << "Failed to create UDP transport" << std::endl;
        }

        // --------------------------------------------------------------------
        // 2. Create TCP transport using factory
        // --------------------------------------------------------------------
        std::cout << "\n--- Creating TCP Transport ---" << std::endl;
        auto tcp_transport = gossip::net::transport_factory::create_transport(
                gossip::net::transport_type::tcp, "127.0.0.1", 9000);

        if (tcp_transport) {
            std::cout << "Successfully created TCP transport" << std::endl;
            test_transport(tcp_transport, "TCP");
        } else {
            std::cerr << "Failed to create TCP transport" << std::endl;
        }

        // --------------------------------------------------------------------
        // 3. Test invalid transport type
        // --------------------------------------------------------------------
        std::cout << "\n--- Testing Invalid Transport Type ---" << std::endl;
        auto invalid_transport = gossip::net::transport_factory::create_transport(
                static_cast<gossip::net::transport_type>(999), "127.0.0.1", 10000);

        if (!invalid_transport) {
            std::cout << "Correctly rejected invalid transport type" << std::endl;
        } else {
            std::cout << "Unexpectedly created invalid transport type" << std::endl;
        }

        std::cout << "\nTransport Factory Example Completed!" << std::endl;

    } catch (const std::exception &e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}